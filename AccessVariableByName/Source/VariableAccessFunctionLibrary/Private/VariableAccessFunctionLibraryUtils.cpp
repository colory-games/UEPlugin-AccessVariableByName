/*!
 * AccessVariableByName
 *
 * Copyright (c) 2022 Colory Games
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "VariableAccessFunctionLibraryUtils.h"

#include "Internationalization/Regex.h"
#include "UObject/UnrealType.h"

FAccessVariableParams UVariableAccessUtilLibrary::MakeAccessVariableParams(bool bIncludeGenerationClass, bool bExtendIfNotPresent)
{
	FAccessVariableParams Params;

	Params.bIncludeGenerationClass = bIncludeGenerationClass;
	Params.bExtendIfNotPresent = bExtendIfNotPresent;

	return Params;
}

namespace FVariableAccessFunctionLibraryUtils
{
bool HandleTerminalProperty(const TArray<FVarDescription>& VarDescs, int32 VarDepth, FStructProperty* OuterProperty,
	void* OuterAddr, FProperty* Dest, void* DestAddr, FProperty* NewValue, void* NewValueAddr, const FAccessVariableParams& Params);
bool HandleTerminalProperty(const TArray<FVarDescription>& VarDescs, int32 VarDepth, UObject* OuterObject, FProperty* Dest,
	void* DestAddr, FProperty* NewValue, void* NewValueAddr, const FAccessVariableParams& Params);

void* GetInnerItemAddrFromArray(FArrayProperty* ArrayProperty, void* OuterAddr, int32 Index, bool bExtendIfNotPresent)
{
	void* ArrayAddr = ArrayProperty->ContainerPtrToValuePtr<void>(OuterAddr);
	FScriptArrayHelper ArrayHelper(ArrayProperty, ArrayAddr);

	if (!ArrayHelper.IsValidIndex(Index) && bExtendIfNotPresent && (Index >= 0))
	{
		ArrayHelper.ExpandForIndex(Index);
	}
	if (!ArrayHelper.IsValidIndex(Index))
	{
		return nullptr;
	}

	return ArrayHelper.GetRawPtr(Index);
}

template <typename T>
T* GetKeyAddrFromMap(FMapProperty* MapProperty, void* OuterAddr, T Key, bool bExtendIfNotPresent)
{
	void* MapAddr = MapProperty->ContainerPtrToValuePtr<void>(OuterAddr);
	auto MapPtr = MapProperty->GetPropertyValuePtr(MapAddr);
	FScriptMapHelper MapHelper(MapProperty, MapAddr);

	for (FScriptMapHelper::FIterator MapIt = MapHelper.CreateIterator(); MapIt; ++MapIt)
	{
		T* KeyPtr = (T*) MapHelper.GetKeyPtr(*MapIt);
		T ActualKey = *KeyPtr;
		if (Key == ActualKey)
		{
			return KeyPtr;
		}
	}

	return nullptr;
}

template <>
FName* GetKeyAddrFromMap<FName>(FMapProperty* MapProperty, void* OuterAddr, FName Key, bool bExtendIfNotPresent)
{
	void* MapAddr = MapProperty->ContainerPtrToValuePtr<void>(OuterAddr);
	auto MapPtr = MapProperty->GetPropertyValuePtr(MapAddr);
	FScriptMapHelper MapHelper(MapProperty, MapAddr);

	for (FScriptMapHelper::FIterator MapIt = MapHelper.CreateIterator(); MapIt; ++MapIt)
	{
		FName* KeyPtr = (FName*) MapHelper.GetKeyPtr(*MapIt);
		FString KeyString = Key.ToString();
		FString ActualKeyString = KeyPtr->ToString();
		if (KeyString == ActualKeyString)
		{
			return KeyPtr;
		}
	}

	return nullptr;
}

template <typename T>
void* GetValueAddrFromMap(FMapProperty* MapProperty, void* OuterAddr, T Key, bool bExtendIfNotPresent)
{
	void* MapAddr = MapProperty->ContainerPtrToValuePtr<void>(OuterAddr);
	auto MapPtr = MapProperty->GetPropertyValuePtr(MapAddr);
	FScriptMapHelper MapHelper(MapProperty, MapAddr);
	T* KeyAddr = GetKeyAddrFromMap(MapProperty, OuterAddr, Key, bExtendIfNotPresent);
	if (KeyAddr == nullptr)
	{
		return nullptr;
	}

	if (bExtendIfNotPresent)
	{
		return MapHelper.FindOrAdd(KeyAddr);
	}

	return MapHelper.FindValueFromHash(KeyAddr);
}

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
			FString PropertyName = Field->GetAuthoredName();

			if (PropertyName.Equals(VarName))
			{
				Property = ScriptStruct->FindPropertyByName(*FieldName);
				break;
			}

			Field = Field->Next;
		}
	}

	return Property;
}

TTuple<FProperty*, UObject*> GetObjectProperty(UObject* Object, FString VarName, bool bFindGeneratedBy)
{
	const TTuple<FProperty*, UObject*> NullReturn(nullptr, nullptr);

	UClass* TargetClass = Object->GetClass();
	FProperty* Property = FindFProperty<FProperty>(TargetClass, *VarName);
	if (Property != nullptr)
	{
		return TTuple<FProperty*, UObject*>(Property, Object);
	}

#if WITH_EDITORONLY_DATA
	if (bFindGeneratedBy)
	{
		UBlueprint* Blueprint = Cast<UBlueprint>(TargetClass->ClassGeneratedBy);
		if (Blueprint == nullptr)
		{
			return NullReturn;
		}

		Property = FindFProperty<FProperty>(Blueprint->GetClass(), *VarName);
		if (Property == nullptr)
		{
			return NullReturn;
		}

		return TTuple<FProperty*, UObject*>(Property, Blueprint);
	}
#endif

	return NullReturn;
}

bool HandleTerminalPropertyInternal(const TArray<FVarDescription>& VarDescs, int32 VarDepth, FProperty* Property, void* OuterAddr,
	FProperty* Dest, void* DestAddr, FProperty* NewValue, void* NewValueAddr, const FAccessVariableParams& Params)
{
	if (VarDescs.Num() <= VarDepth)
	{
		return false;
	}

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
			FStructProperty* StructProperty = CastFieldChecked<FStructProperty>(Property);
			void* StructAddr = Property->ContainerPtrToValuePtr<void>(OuterAddr);

			return HandleTerminalProperty(
				VarDescs, VarDepth + 1, StructProperty, StructAddr, Dest, DestAddr, NewValue, NewValueAddr, Params);
		}
		else if (Property->IsA<FObjectProperty>())
		{
			FObjectProperty* ObjectProperty = CastFieldChecked<FObjectProperty>(Property);
			UObject* Object = ObjectProperty->GetPropertyValue_InContainer(OuterAddr);
			void* ObjectAddr = ObjectProperty->ContainerPtrToValuePtr<void>(OuterAddr);

			return HandleTerminalProperty(VarDescs, VarDepth + 1, Object, Dest, DestAddr, NewValue, NewValueAddr, Params);
		}

		return false;
	}
	else if (Desc.ArrayAccessType == EArrayAccessType::ArrayAccessType_Integer)
	{
		if (Property->IsA<FArrayProperty>())
		{
			FArrayProperty* ArrayProperty = CastFieldChecked<FArrayProperty>(Property);
			FProperty* InnerProperty = ArrayProperty->Inner;

			if (VarDescs.Num() == VarDepth + 1)
			{
				if (!InnerProperty->SameType(Dest))
				{
					return false;
				}

				void* InnerItemAddr =
					GetInnerItemAddrFromArray(ArrayProperty, OuterAddr, Desc.ArrayAccessValue.Integer, Params.bExtendIfNotPresent);
				if (InnerItemAddr == nullptr)
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
				void* InnerItemAddr =
					GetInnerItemAddrFromArray(ArrayProperty, OuterAddr, Desc.ArrayAccessValue.Integer, Params.bExtendIfNotPresent);
				if (InnerItemAddr == nullptr)
				{
					return false;
				}

				FStructProperty* StructProperty = CastFieldChecked<FStructProperty>(InnerProperty);

				return HandleTerminalProperty(
					VarDescs, VarDepth + 1, StructProperty, InnerItemAddr, Dest, DestAddr, NewValue, NewValueAddr, Params);
			}
			else if (InnerProperty->IsA<FObjectProperty>())
			{
				void* InnerItemAddr =
					GetInnerItemAddrFromArray(ArrayProperty, OuterAddr, Desc.ArrayAccessValue.Integer, Params.bExtendIfNotPresent);
				if (InnerItemAddr == nullptr)
				{
					return false;
				}

				FObjectProperty* ObjectProperty = CastFieldChecked<FObjectProperty>(InnerProperty);
				UObject* Object = ObjectProperty->GetPropertyValue_InContainer(InnerItemAddr);

				return HandleTerminalProperty(VarDescs, VarDepth + 1, Object, Dest, DestAddr, NewValue, NewValueAddr, Params);
			}

			return false;
		}
		else if (Property->IsA<FMapProperty>())
		{
			FMapProperty* MapProperty = CastFieldChecked<FMapProperty>(Property);

			FProperty* KeyProperty = MapProperty->KeyProp;
			FProperty* ValueProperty = MapProperty->ValueProp;
			if (!KeyProperty->IsA<FByteProperty>() && !KeyProperty->IsA<FIntProperty>() && !KeyProperty->IsA<FInt64Property>())
			{
				return false;
			}

			if (VarDescs.Num() == VarDepth + 1)
			{
				if (!ValueProperty->SameType(Dest))
				{
					return false;
				}

				void* ValueAddr =
					GetValueAddrFromMap(MapProperty, OuterAddr, Desc.ArrayAccessValue.Integer, Params.bExtendIfNotPresent);
				if (ValueAddr == nullptr)
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
				void* ValueAddr =
					GetValueAddrFromMap(MapProperty, OuterAddr, Desc.ArrayAccessValue.Integer, Params.bExtendIfNotPresent);
				if (ValueAddr == nullptr)
				{
					return false;
				}

				FStructProperty* StructProperty = CastFieldChecked<FStructProperty>(ValueProperty);

				return HandleTerminalProperty(
					VarDescs, VarDepth + 1, StructProperty, ValueAddr, Dest, DestAddr, NewValue, NewValueAddr, Params);
			}
			else if (ValueProperty->IsA<FObjectProperty>())
			{
				void* ValueAddr =
					GetValueAddrFromMap(MapProperty, OuterAddr, Desc.ArrayAccessValue.Integer, Params.bExtendIfNotPresent);
				if (ValueAddr == nullptr)
				{
					return false;
				}

				FObjectProperty* ObjectProperty = CastFieldChecked<FObjectProperty>(ValueProperty);
				UObject* Object = ObjectProperty->GetPropertyValue(ValueAddr);

				return HandleTerminalProperty(VarDescs, VarDepth + 1, Object, Dest, DestAddr, NewValue, NewValueAddr, Params);
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
		FMapProperty* MapProperty = CastFieldChecked<FMapProperty>(Property);

		FProperty* KeyProperty = MapProperty->KeyProp;
		FProperty* ValueProperty = MapProperty->ValueProp;
		if (!KeyProperty->IsA<FStrProperty>() && !KeyProperty->IsA<FNameProperty>() && !KeyProperty->IsA<FTextProperty>())
		{
			return false;
		}

		if (VarDescs.Num() == VarDepth + 1)
		{
			if (!ValueProperty->SameType(Dest))
			{
				return false;
			}

			void* ValueAddr = GetValueAddrFromMap(MapProperty, OuterAddr, Desc.ArrayAccessValue.String, Params.bExtendIfNotPresent);
			if (ValueAddr == nullptr)
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
			void* ValueAddr = nullptr;
			if (KeyProperty->IsA<FStrProperty>())
			{
				ValueAddr = GetValueAddrFromMap(MapProperty, OuterAddr, Desc.ArrayAccessValue.String, Params.bExtendIfNotPresent);
			}
			else if (KeyProperty->IsA<FNameProperty>())
			{
				ValueAddr =
					GetValueAddrFromMap(MapProperty, OuterAddr, FName(*Desc.ArrayAccessValue.String), Params.bExtendIfNotPresent);
			}
			if (ValueAddr == nullptr)
			{
				return false;
			}

			FStructProperty* StructProperty = CastFieldChecked<FStructProperty>(ValueProperty);

			return HandleTerminalProperty(
				VarDescs, VarDepth + 1, StructProperty, ValueAddr, Dest, DestAddr, NewValue, NewValueAddr, Params);
		}
		else if (ValueProperty->IsA<FObjectProperty>())
		{
			void* ValueAddr = nullptr;
			if (KeyProperty->IsA<FStrProperty>())
			{
				ValueAddr =
					GetValueAddrFromMap<FString>(MapProperty, OuterAddr, Desc.ArrayAccessValue.String, Params.bExtendIfNotPresent);
			}
			else if (KeyProperty->IsA<FNameProperty>())
			{
				ValueAddr =
					GetValueAddrFromMap<FName>(MapProperty, OuterAddr, FName(*Desc.ArrayAccessValue.String), Params.bExtendIfNotPresent);
			}
			if (ValueAddr == nullptr)
			{
				return false;
			}

			FObjectProperty* ObjectProperty = CastFieldChecked<FObjectProperty>(ValueProperty);
			UObject* Object = ObjectProperty->GetPropertyValue(ValueAddr);

			return HandleTerminalProperty(VarDescs, VarDepth + 1, Object, Dest, DestAddr, NewValue, NewValueAddr, Params);
		}

		return false;
	}

	return false;
}

bool HandleTerminalProperty(const TArray<FVarDescription>& VarDescs, int32 VarDepth, FStructProperty* OuterProperty,
	void* OuterAddr, FProperty* Dest, void* DestAddr, FProperty* NewValue, void* NewValueAddr, const FAccessVariableParams& Params)
{
	if (VarDescs.Num() <= VarDepth)
	{
		return false;
	}

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

	return HandleTerminalPropertyInternal(VarDescs, VarDepth, Property, OuterAddr, Dest, DestAddr, NewValue, NewValueAddr, Params);
}

bool HandleTerminalProperty(const TArray<FVarDescription>& VarDescs, int32 VarDepth, UObject* OuterObject, FProperty* Dest,
	void* DestAddr, FProperty* NewValue, void* NewValueAddr, const FAccessVariableParams& Params)
{
	if (VarDescs.Num() <= VarDepth)
	{
		return false;
	}

	const FVarDescription& Desc = VarDescs[VarDepth];

	if (!Desc.bIsValid)
	{
		return false;
	}

	if (OuterObject == nullptr)
	{
		return false;
	}

	TTuple<FProperty*, UObject*> Result = GetObjectProperty(OuterObject, Desc.VarName, Params.bIncludeGenerationClass);
	if (Result.Get<0>() == nullptr || Result.Get<1>() == nullptr)
	{
		return false;
	}

	return HandleTerminalPropertyInternal(
		VarDescs, VarDepth, Result.Get<0>(), Result.Get<1>(), Dest, DestAddr, NewValue, NewValueAddr, Params);
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
			FRegexPattern Pattern = FRegexPattern("^([a-zA-Z_][a-zA-Z0-9_ ]*)\\[\"(\\S+)\"\\]$");
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
			FRegexPattern Pattern = FRegexPattern("^([a-zA-Z_][a-zA-Z0-9_ ]*)\\[([0-9]+)\\]$");
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
			FRegexPattern Pattern = FRegexPattern("^([a-zA-Z_][a-zA-Z0-9_ ]*)$");
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
}	 // namespace FVariableAccessFunctionLibraryUtils