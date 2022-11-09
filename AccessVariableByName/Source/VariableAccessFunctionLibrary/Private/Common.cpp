/*!
 * AccessVariableByName
 *
 * Copyright (c) 2022 Colory Games
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "Common.h"

#include "UObject/UnrealType.h"
#include "Internationalization/Regex.h"

FProperty* GetTerminalProperty(const TArray<FVarDescription>& VarDescs, int32 VarDepth, UScriptStruct* OuterClass);
bool HandleTerminalProperty(const TArray<FVarDescription>& VarDescs, int32 VarDepth, FStructProperty* OuterProperty,
	void* OuterAddr, FProperty* Dest, void* DestAddr, FProperty* NewValue, void* NewValueAddr);
bool HandleTerminalProperty(const TArray<FVarDescription>& VarDescs, int32 VarDepth, UObject* OuterObject, FProperty* Dest,
	void* DestAddr, FProperty* NewValue, void* NewValueAddr);

FProperty* GetScriptStructProperty(UScriptStruct* ScriptStruct, FString VarName)
{
	FProperty* Property = nullptr;

	if (ScriptStruct->IsNative())
	{
		Property = ScriptStruct->FindPropertyByName(*VarName);
	}
	else
	{
		FField* Field = ScriptStruct->ChildProperties;
		while (Field)
		{
			FString FieldName = Field->GetName();
			int32 Index = FieldName.Len();
			for (int Iter = 0; Iter < 2; ++Iter)
			{
				Index = FieldName.Find(FString("_"), ESearchCase::CaseSensitive, ESearchDir::FromEnd, Index);
			}
			FString PropertyName = FieldName.Left(Index);

			if (PropertyName == VarName)
			{
				Property = ScriptStruct->FindPropertyByName(*FieldName);
				break;
			}

			Field = Field->Next;
		}
	}

	return Property;
}

bool HandleTerminalPropertyInternal(const TArray<FVarDescription>& VarDescs, int32 VarDepth, FProperty* Property, void* OuterAddr,
	FProperty* Dest, void* DestAddr, FProperty* NewValue, void* NewValueAddr)
{
	const FVarDescription& Desc = VarDescs[VarDepth];

	if (Desc.ArrayAccessType == EArrayAccessType::ArrayAccessType_None)
	{
		if (VarDescs.Num() == VarDepth + 1)
		{
			if (!Property->SameType(Dest))
			{
				return false;
			}

			void* SrcAddr = Property->ContainerPtrToValuePtr<void>(OuterAddr);
			if (NewValue != nullptr)
			{
				Property->CopyCompleteValue(SrcAddr, NewValueAddr);
			}
			Property->CopyCompleteValue(DestAddr, SrcAddr);
			return true;
		}

		if (Property->IsA<FStructProperty>())
		{
			FStructProperty* StructProperty = CastChecked<FStructProperty>(Property);
			void* StructAddr = Property->ContainerPtrToValuePtr<void>(OuterAddr);

			return HandleTerminalProperty(
				VarDescs, VarDepth + 1, StructProperty, StructAddr, Dest, DestAddr, NewValue, NewValueAddr);
		}
		else if (Property->IsA<FObjectProperty>())
		{
			FObjectProperty* ObjectProperty = CastChecked<FObjectProperty>(Property);
			UObject* Object = ObjectProperty->GetPropertyValue_InContainer(OuterAddr);
			void* ObjectAddr = ObjectProperty->ContainerPtrToValuePtr<void>(OuterAddr);

			return HandleTerminalProperty(VarDescs, VarDepth + 1, Object, Dest, DestAddr, NewValue, NewValueAddr);
		}

		return false;
	}
	else if (Desc.ArrayAccessType == EArrayAccessType::ArrayAccessType_Integer)
	{
		if (Property->IsA<FArrayProperty>())
		{
			FArrayProperty* ArrayProperty = CastChecked<FArrayProperty>(Property);
			void* ArrayAddr = ArrayProperty->ContainerPtrToValuePtr<void>(OuterAddr);
			auto ArrayPtr = ArrayProperty->GetPropertyValuePtr(ArrayAddr);
			FProperty* InnerProperty = ArrayProperty->Inner;

			if (VarDescs.Num() == VarDepth + 1)
			{
				int32 Index = Desc.ArrayAccessValue.Integer;
				if (Index >= ArrayPtr->Num())
				{
					return false;
				}

				int32 Stride = Dest->ArrayDim * Dest->ElementSize;
				int8* InnerAddr = static_cast<int8*>(ArrayPtr->GetData());
				void* InnerItemAddr = InnerAddr + Index * Stride;

				if (!InnerProperty->SameType(Dest))
				{
					return false;
				}

				if (NewValue != nullptr)
				{
					Dest->CopyCompleteValue(InnerItemAddr, NewValueAddr);
				}
				Dest->CopyCompleteValue(DestAddr, InnerItemAddr);

				return true;
			}

			if (InnerProperty->IsA<FStructProperty>())
			{
				FStructProperty* StructProperty = CastChecked<FStructProperty>(InnerProperty);

				int32 Index = Desc.ArrayAccessValue.Integer;
				if (Index >= ArrayPtr->Num())
				{
					return false;
				}

				int32 Stride = StructProperty->ArrayDim * StructProperty->ElementSize;
				int8* InnerAddr = static_cast<int8*>(ArrayPtr->GetData());
				void* InnerItemAddr = InnerAddr + Index * Stride;

				return HandleTerminalProperty(
					VarDescs, VarDepth + 1, StructProperty, InnerItemAddr, Dest, DestAddr, NewValue, NewValueAddr);
			}
			else if (InnerProperty->IsA<FObjectProperty>())
			{
				FObjectProperty* ObjectProperty = CastChecked<FObjectProperty>(InnerProperty);

				int32 Index = Desc.ArrayAccessValue.Integer;
				if (Index >= ArrayPtr->Num())
				{
					return false;
				}

				int32 Stride = ObjectProperty->ArrayDim * ObjectProperty->ElementSize;
				int8* InnerAddr = static_cast<int8*>(ArrayPtr->GetData());
				void* InnerItemAddr = InnerAddr + Index * Stride;
				UObject* Object = ObjectProperty->GetPropertyValue_InContainer(InnerItemAddr);

				return HandleTerminalProperty(VarDescs, VarDepth + 1, Object, Dest, DestAddr, NewValue, NewValueAddr);
			}

			return false;
		}
		else if (Property->IsA<FMapProperty>())
		{
			FMapProperty* MapProperty = CastChecked<FMapProperty>(Property);

			FProperty* KeyProperty = MapProperty->KeyProp;
			FProperty* ValueProperty = MapProperty->ValueProp;
			if (!KeyProperty->IsA<FIntProperty>() && !KeyProperty->IsA<FInt64Property>())
			{
				return false;
			}

			if (VarDescs.Num() == VarDepth + 1)
			{
				void* MapAddr = MapProperty->ContainerPtrToValuePtr<void>(OuterAddr);
				auto MapPtr = MapProperty->GetPropertyValuePtr(MapAddr);
				int32 Key = Desc.ArrayAccessValue.Integer;
				uint8* ValueAddr = MapPtr->FindValue(
					&Key, MapProperty->MapLayout, [KeyProperty](const void* Key) { return KeyProperty->GetValueTypeHash(Key); },
					[KeyProperty](const void* A, const void* B) { return KeyProperty->Identical(A, B); });
				if (ValueAddr == nullptr)
				{
					return false;
				}

				if (!ValueProperty->SameType(Dest))
				{
					return false;
				}

				if (NewValue != nullptr)
				{
					Dest->CopyCompleteValue(ValueAddr, NewValueAddr);
				}
				Dest->CopyCompleteValue(DestAddr, ValueAddr);

				return true;
			}

			if (ValueProperty->IsA<FStructProperty>())
			{
				FStructProperty* StructProperty = CastChecked<FStructProperty>(ValueProperty);
				void* MapAddr = MapProperty->ContainerPtrToValuePtr<void>(OuterAddr);
				auto MapPtr = MapProperty->GetPropertyValuePtr(MapAddr);
				int32 Key = Desc.ArrayAccessValue.Integer;
				uint8* ValueAddr = MapPtr->FindValue(
					&Key, MapProperty->MapLayout, [KeyProperty](const void* Key) { return KeyProperty->GetValueTypeHash(Key); },
					[KeyProperty](const void* A, const void* B) { return KeyProperty->Identical(A, B); });
				if (ValueAddr == nullptr)
				{
					return false;
				}

				return HandleTerminalProperty(
					VarDescs, VarDepth + 1, StructProperty, ValueAddr, Dest, DestAddr, NewValue, NewValueAddr);
			}
			else if (ValueProperty->IsA<FObjectProperty>())
			{
				FObjectProperty* ObjectProperty = CastChecked<FObjectProperty>(ValueProperty);
				void* MapAddr = MapProperty->ContainerPtrToValuePtr<void>(OuterAddr);
				auto MapPtr = MapProperty->GetPropertyValuePtr(MapAddr);
				int32 Key = Desc.ArrayAccessValue.Integer;
				void* ValueAddr = MapPtr->FindValue(
					&Key, MapProperty->MapLayout, [KeyProperty](const void* Key) { return KeyProperty->GetValueTypeHash(Key); },
					[KeyProperty](const void* A, const void* B) { return KeyProperty->Identical(A, B); });
				if (ValueAddr == nullptr)
				{
					return false;
				}
				UObject* Object = ObjectProperty->GetPropertyValue(ValueAddr);

				return HandleTerminalProperty(VarDescs, VarDepth + 1, Object, Dest, DestAddr, NewValue, NewValueAddr);
			}

			return false;
		}

		return false;
	}
	else if (Desc.ArrayAccessType == EArrayAccessType::ArrayAccessType_String)
	{
		if (!Property->IsA<FMapProperty>())
		{
			return false;
		}
		FMapProperty* MapProperty = CastChecked<FMapProperty>(Property);

		FProperty* KeyProperty = MapProperty->KeyProp;
		FProperty* ValueProperty = MapProperty->ValueProp;
		if (!KeyProperty->IsA<FStrProperty>())
		{
			return false;
		}

		if (VarDescs.Num() == VarDepth + 1)
		{
			void* MapAddr = MapProperty->ContainerPtrToValuePtr<void>(OuterAddr);
			auto MapPtr = MapProperty->GetPropertyValuePtr(MapAddr);
			FString Key = Desc.ArrayAccessValue.String;
			uint8* ValueAddr = MapPtr->FindValue(
				&Key, MapProperty->MapLayout, [KeyProperty](const void* Key) { return KeyProperty->GetValueTypeHash(Key); },
				[KeyProperty](const void* A, const void* B) { return KeyProperty->Identical(A, B); });
			if (ValueAddr == nullptr)
			{
				return false;
			}

			if (!ValueProperty->SameType(Dest))
			{
				return false;
			}

			if (NewValue != nullptr)
			{
				Dest->CopyCompleteValue(ValueAddr, NewValueAddr);
			}
			Dest->CopyCompleteValue(DestAddr, ValueAddr);

			return true;
		}

		if (ValueProperty->IsA<FStructProperty>())
		{
			FStructProperty* StructProperty = CastChecked<FStructProperty>(ValueProperty);
			void* MapAddr = MapProperty->ContainerPtrToValuePtr<void>(OuterAddr);
			auto MapPtr = MapProperty->GetPropertyValuePtr(MapAddr);
			FString Key = Desc.ArrayAccessValue.String;
			uint8* ValueAddr = MapPtr->FindValue(
				&Key, MapProperty->MapLayout, [KeyProperty](const void* Key) { return KeyProperty->GetValueTypeHash(Key); },
				[KeyProperty](const void* A, const void* B) { return KeyProperty->Identical(A, B); });
			if (ValueAddr == nullptr)
			{
				return false;
			}

			return HandleTerminalProperty(
				VarDescs, VarDepth + 1, StructProperty, ValueAddr, Dest, DestAddr, NewValue, NewValueAddr);
		}
		else if (ValueProperty->IsA<FObjectProperty>())
		{
			FObjectProperty* ObjectProperty = CastChecked<FObjectProperty>(ValueProperty);
			void* MapAddr = MapProperty->ContainerPtrToValuePtr<void>(OuterAddr);
			auto MapPtr = MapProperty->GetPropertyValuePtr(MapAddr);
			FString Key = Desc.ArrayAccessValue.String;
			void* ValueAddr = MapPtr->FindValue(
				&Key, MapProperty->MapLayout, [KeyProperty](const void* Key) { return KeyProperty->GetValueTypeHash(Key); },
				[KeyProperty](const void* A, const void* B) { return KeyProperty->Identical(A, B); });
			if (ValueAddr == nullptr)
			{
				return false;
			}
			UObject* Object = ObjectProperty->GetPropertyValue(ValueAddr);

			return HandleTerminalProperty(VarDescs, VarDepth + 1, Object, Dest, DestAddr, NewValue, NewValueAddr);
		}

		return false;
	}

	return false;
}

bool HandleTerminalProperty(const TArray<FVarDescription>& VarDescs, int32 VarDepth, FStructProperty* OuterProperty,
	void* OuterAddr, FProperty* Dest, void* DestAddr, FProperty* NewValue, void* NewValueAddr)
{
	const FVarDescription& Desc = VarDescs[VarDepth];

	if (!Desc.bIsValid)
	{
		return false;
	}

	UScriptStruct* ScriptStruct = OuterProperty->Struct;
	FProperty* Property = GetScriptStructProperty(ScriptStruct, Desc.VarName);
	if (Property == nullptr)
	{
		return false;
	}

	return HandleTerminalPropertyInternal(VarDescs, VarDepth, Property, OuterAddr, Dest, DestAddr, NewValue, NewValueAddr);
}

bool HandleTerminalProperty(const TArray<FVarDescription>& VarDescs, int32 VarDepth, UObject* OuterObject, FProperty* Dest,
	void* DestAddr, FProperty* NewValue, void* NewValueAddr)
{
	const FVarDescription& Desc = VarDescs[VarDepth];

	if (!Desc.bIsValid)
	{
		return false;
	}

	if (OuterObject == nullptr)
	{
		return false;
	}

	FProperty* Property = FindFProperty<FProperty>(OuterObject->GetClass(), *Desc.VarName);
	if (Property == nullptr)
	{
		return false;
	}

	return HandleTerminalPropertyInternal(VarDescs, VarDepth, Property, OuterObject, Dest, DestAddr, NewValue, NewValueAddr);
}

FProperty* GetTerminalPropertyInternal(const TArray<FVarDescription>& VarDescs, int32 VarDepth, FProperty* Property)
{
	const FVarDescription& Desc = VarDescs[VarDepth];

	if (Desc.ArrayAccessType == EArrayAccessType::ArrayAccessType_None)
	{
		if (VarDescs.Num() == VarDepth + 1)
		{
			return Property;
		}

		if (Property->IsA<FStructProperty>())
		{
			FStructProperty* StructProperty = CastChecked<FStructProperty>(Property);
			UScriptStruct* ScriptStruct = StructProperty->Struct;

			return GetTerminalProperty(VarDescs, VarDepth + 1, ScriptStruct);
		}
		else if (Property->IsA<FObjectProperty>())
		{
			FObjectProperty* ObjectProperty = CastChecked<FObjectProperty>(Property);
			UClass* Class = ObjectProperty->PropertyClass;

			return GetTerminalProperty(VarDescs, VarDepth + 1, Class);
		}

		return nullptr;
	}
	else if (Desc.ArrayAccessType == EArrayAccessType::ArrayAccessType_Integer)
	{
		if (Property->IsA<FArrayProperty>())
		{
			FArrayProperty* ArrayProperty = CastChecked<FArrayProperty>(Property);

			if (VarDescs.Num() == VarDepth + 1)
			{
				return ArrayProperty->Inner;
			}

			FProperty* InnerProperty = ArrayProperty->Inner;
			if (InnerProperty->IsA<FStructProperty>())
			{
				FStructProperty* StructProperty = CastChecked<FStructProperty>(InnerProperty);
				UScriptStruct* ScriptStruct = StructProperty->Struct;

				return GetTerminalProperty(VarDescs, VarDepth + 1, ScriptStruct);
			}
			else if (InnerProperty->IsA<FObjectProperty>())
			{
				FObjectProperty* ObjectProperty = CastChecked<FObjectProperty>(InnerProperty);
				UClass* Class = ObjectProperty->PropertyClass;

				return GetTerminalProperty(VarDescs, VarDepth + 1, Class);
			}

			return nullptr;
		}
		else if (Property->IsA<FMapProperty>())
		{
			FMapProperty* MapProperty = CastChecked<FMapProperty>(Property);

			if (VarDescs.Num() == VarDepth + 1)
			{
				return MapProperty->ValueProp;
			}

			FProperty* ValueProperty = MapProperty->ValueProp;
			if (ValueProperty->IsA<FStructProperty>())
			{
				FStructProperty* StructProperty = CastChecked<FStructProperty>(ValueProperty);
				UScriptStruct* ScriptStruct = StructProperty->Struct;

				return GetTerminalProperty(VarDescs, VarDepth + 1, ScriptStruct);
			}
			else if (ValueProperty->IsA<FObjectProperty>())
			{
				FObjectProperty* ObjectProperty = CastChecked<FObjectProperty>(ValueProperty);
				UClass* Class = ObjectProperty->PropertyClass;

				return GetTerminalProperty(VarDescs, VarDepth + 1, Class);
			}
		}

		return nullptr;
	}
	else if (Desc.ArrayAccessType == EArrayAccessType::ArrayAccessType_String)
	{
		if (!Property->IsA<FMapProperty>())
		{
			return nullptr;
		}
		FMapProperty* MapProperty = CastChecked<FMapProperty>(Property);

		if (VarDescs.Num() == VarDepth + 1)
		{
			return MapProperty->ValueProp;
		}

		FProperty* ValueProperty = MapProperty->ValueProp;
		if (ValueProperty->IsA<FStructProperty>())
		{
			FStructProperty* StructProperty = CastChecked<FStructProperty>(ValueProperty);
			UScriptStruct* ScriptStruct = StructProperty->Struct;

			return GetTerminalProperty(VarDescs, VarDepth + 1, ScriptStruct);
		}
		else if (ValueProperty->IsA<FObjectProperty>())
		{
			FObjectProperty* ObjectProperty = CastChecked<FObjectProperty>(ValueProperty);
			UClass* Class = ObjectProperty->PropertyClass;

			return GetTerminalProperty(VarDescs, VarDepth + 1, Class);
		}

		return nullptr;
	}

	return nullptr;
}

FProperty* GetTerminalProperty(const TArray<FVarDescription>& VarDescs, int32 VarDepth, UScriptStruct* OuterClass)
{
	const FVarDescription& Desc = VarDescs[VarDepth];

	if (!Desc.bIsValid)
	{
		return nullptr;
	}

	FProperty* Property = GetScriptStructProperty(OuterClass, Desc.VarName);
	if (Property == nullptr)
	{
		return nullptr;
	}

	return GetTerminalPropertyInternal(VarDescs, VarDepth, Property);
}

FProperty* GetTerminalProperty(const TArray<FVarDescription>& VarDescs, int32 VarDepth, UClass* OuterClass)
{
	const FVarDescription& Desc = VarDescs[VarDepth];

	if (!Desc.bIsValid)
	{
		return nullptr;
	}

	FProperty* Property = FindFProperty<FProperty>(OuterClass, *Desc.VarName);
	if (Property == nullptr)
	{
		return nullptr;
	}

	return GetTerminalPropertyInternal(VarDescs, VarDepth, Property);
}

void SplitVarNameInternal(const FString& In, int32 StartIndex, TArray<FString>* Out)
{
	bool bInString = false;
	int32 Index = StartIndex;
	for (; Index < In.Len(); ++Index)
	{
		FString Ch = In.Mid(Index, 1);
		if (Ch == "\"")
		{
			bInString = !bInString;
		}

		if (!bInString && Ch == ".")
		{
			Out->Add(In.Mid(StartIndex, Index - StartIndex));
			SplitVarNameInternal(In, Index + 1, Out);
			break;
		}
	}
	if (Index == In.Len())
	{
		Out->Add(In.Mid(StartIndex, Index - StartIndex));
	}
}

void SplitVarName(const FString& In, TArray<FString>* Out)
{
	SplitVarNameInternal(In, 0, Out);
	*Out = Out->FilterByPredicate([](const FString& S) { return !S.IsEmpty(); });
}

void AnalyzeVarNames(const TArray<FString>& VarNames, TArray<FVarDescription>* VarDescs)
{
	for (auto& Var : VarNames)
	{
		// String pattern.
		{
			FRegexPattern Pattern = FRegexPattern("^([a-zA-Z_][a-zA-Z0-9_]*)\\[\"(\\S+)\"\\]$");
			FRegexMatcher Matcher(Pattern, Var);
			if (Matcher.FindNext())
			{
				FVarDescription Desc;
				Desc.bIsValid = true;
				Desc.VarName = Matcher.GetCaptureGroup(1);
				Desc.ArrayAccessType = EArrayAccessType::ArrayAccessType_String;
				Desc.ArrayAccessValue.Integer = -1;
				Desc.ArrayAccessValue.String = Matcher.GetCaptureGroup(2);
				VarDescs->Add(Desc);
				continue;
			}
		}

		// Integer pattern.
		{
			FRegexPattern Pattern = FRegexPattern("^([a-zA-Z_][a-zA-Z0-9_]*)\\[([0-9]+)\\]$");
			FRegexMatcher Matcher(Pattern, Var);
			if (Matcher.FindNext())
			{
				FVarDescription Desc;
				Desc.bIsValid = true;
				Desc.VarName = Matcher.GetCaptureGroup(1);
				Desc.ArrayAccessType = EArrayAccessType::ArrayAccessType_Integer;
				Desc.ArrayAccessValue.Integer = FCString::Atoi(*Matcher.GetCaptureGroup(2));
				Desc.ArrayAccessValue.String = "";
				VarDescs->Add(Desc);
				continue;
			}
		}

		// None pattern.
		{
			FRegexPattern Pattern = FRegexPattern("^([a-zA-Z_][a-zA-Z0-9_]*)$");
			FRegexMatcher Matcher(Pattern, Var);
			if (Matcher.FindNext())
			{
				FVarDescription Desc;
				Desc.bIsValid = true;
				Desc.VarName = Matcher.GetCaptureGroup(1);
				Desc.ArrayAccessType = EArrayAccessType::ArrayAccessType_None;
				Desc.ArrayAccessValue.Integer = -1;
				Desc.ArrayAccessValue.String = "";
				VarDescs->Add(Desc);
				continue;
			}
		}

		FVarDescription Desc;
		Desc.bIsValid = false;
		VarDescs->Add(Desc);
	}
}

