/*!
 * AccessVariableByName
 *
 * Copyright (c) 2022 Colory Games
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "VariableGetterFunctionLibrary.h"

#include "Common.h"


bool CopyTerminalProperty(const TArray<FVarDescription>& VarDescs, int32 VarDepth, FStructProperty* OuterProperty, void* OuterAddr, FProperty* Dest, void* DestAddr)
{
	const FVarDescription& Desc = VarDescs[VarDepth];

	if (!Desc.bIsValid)
	{
		return false;
	}

	UScriptStruct* ScriptStruct = OuterProperty->Struct;
	FProperty* Property = nullptr;
	if (ScriptStruct->IsNative())
	{
		Property = ScriptStruct->FindPropertyByName(*Desc.VarName);
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

			if (PropertyName == Desc.VarName)
			{
				Property = ScriptStruct->FindPropertyByName(*FieldName);
				break;
			}

			Field = Field->Next;
		}
	}
	if (Property == nullptr)
	{
		return false;
	}

	if (Desc.VarType == EContainerType::None)
	{
		if (VarDescs.Num() == VarDepth + 1)
		{
			void* SrcAddr = Property->ContainerPtrToValuePtr<void>(OuterAddr);
			Property->CopyCompleteValue(DestAddr, SrcAddr);
			return true;
		}

		if (!Property->IsA<FStructProperty>())
		{
			return false;
		}

		FStructProperty* StructProperty = CastChecked<FStructProperty>(Property);
		void* StructAddr = Property->ContainerPtrToValuePtr<void>(OuterAddr);

		return CopyTerminalProperty(VarDescs, VarDepth + 1, StructProperty, StructAddr, Dest, DestAddr);
	}
	else if (Desc.VarType == EContainerType::Array)
	{
		if (!Property->IsA<FArrayProperty>())
		{
			return false;
		}

		FArrayProperty* ArrayProperty = CastChecked<FArrayProperty>(Property);
		void* ArrayAddr = ArrayProperty->ContainerPtrToValuePtr<void>(OuterAddr);
		auto ArrayPtr = ArrayProperty->GetPropertyValuePtr(ArrayAddr);

		if (VarDescs.Num() == VarDepth + 1)
		{
			int32 Index = Desc.ArrayIndex;

			if (Index >= ArrayPtr->Num())
			{
				return false;
			}

			int32 Stride = Dest->ArrayDim * Dest->ElementSize;
			int8* InnerAddr = static_cast<int8*>(ArrayPtr->GetData());
			void* InnerItemAddr = InnerAddr + Index * Stride;

			Dest->CopyCompleteValue(DestAddr, InnerItemAddr);

			return true;
		}

		FProperty* InnerProperty = ArrayProperty->Inner;
		if (!InnerProperty->IsA<FStructProperty>())
		{
			return false;
		}

		FStructProperty* StructProperty = CastChecked<FStructProperty>(InnerProperty);
		int32 Index = Desc.ArrayIndex;

		if (Index >= ArrayPtr->Num())
		{
			return false;
		}

		int32 Stride = StructProperty->ArrayDim * StructProperty->ElementSize;
		int8* InnerAddr = static_cast<int8*>(ArrayPtr->GetData());
		void* InnerItemAddr = InnerAddr + Index * Stride;

		return CopyTerminalProperty(VarDescs, VarDepth + 1, StructProperty, InnerItemAddr, Dest, DestAddr);
	}
	else if (Desc.VarType == EContainerType::Map)
	{
		if (!Property->IsA<FMapProperty>())
		{
			return false;
		}

		FMapProperty* MapProperty = CastChecked<FMapProperty>(Property);
		FProperty* KeyProperty = MapProperty->KeyProp;
		if (!KeyProperty->IsA<FStrProperty>())
		{
			return false;
		}

		if (VarDescs.Num() == VarDepth + 1)
		{
			void* MapAddr = MapProperty->ContainerPtrToValuePtr<void>(OuterAddr);
			auto MapPtr = MapProperty->GetPropertyValuePtr(MapAddr);
			FString Key = Desc.MapKey;
			uint8* ValueAddr = MapPtr->FindValue(
				&Key,
				MapProperty->MapLayout,
				[KeyProperty](const void* Key) { return KeyProperty->GetValueTypeHash(Key); },
				[KeyProperty](const void* A, const void* B) { return KeyProperty->Identical(A, B); }
			);
			if (ValueAddr == nullptr)
			{
				return false;
			}

			Dest->CopyCompleteValue(DestAddr, ValueAddr);

			return true;
		}

		FProperty* ValueProperty = MapProperty->ValueProp;
		if (!ValueProperty->IsA<FStructProperty>())
		{
			return false;
		}

		FStructProperty* StructProperty = CastChecked<FStructProperty>(ValueProperty);
		void* MapAddr = MapProperty->ContainerPtrToValuePtr<void>(OuterAddr);
		auto MapPtr = MapProperty->GetPropertyValuePtr(MapAddr);
		FString Key = Desc.MapKey;
		uint8* ValueAddr = MapPtr->FindValue(
			&Key,
			MapProperty->MapLayout,
			[KeyProperty](const void* Key) { return KeyProperty->GetValueTypeHash(Key); },
			[KeyProperty](const void* A, const void* B) { return KeyProperty->Identical(A, B); }
		);
		if (ValueAddr == nullptr)
		{
			return false;
		}

		return CopyTerminalProperty(VarDescs, VarDepth + 1, StructProperty, ValueAddr, Dest, DestAddr);
	}

	return false;
}

bool CopyTerminalProperty(const TArray<FVarDescription>& VarDescs, int32 VarDepth, UObject* OuterObject, FProperty* Dest, void* DestAddr)
{
	const FVarDescription& Desc = VarDescs[VarDepth];

	if (!Desc.bIsValid)
	{
		return false;
	}

	FProperty* Property = FindFProperty<FProperty>(OuterObject->GetClass(), *Desc.VarName);
	if (Property == nullptr)
	{
		return false;
	}

	if (Desc.VarType == EContainerType::None)
	{
		if (VarDescs.Num() == VarDepth + 1)
		{
			void* SrcAddr = Property->ContainerPtrToValuePtr<void>(OuterObject);
			Property->CopyCompleteValue(DestAddr, SrcAddr);
			return true;
		}

		if (!Property->IsA<FStructProperty>())
		{
			return false;
		}

		FStructProperty* StructProperty = CastChecked<FStructProperty>(Property);
		void* StructAddr = Property->ContainerPtrToValuePtr<void>(OuterObject);

		return CopyTerminalProperty(VarDescs, VarDepth + 1, StructProperty, StructAddr, Dest, DestAddr);
	}
	else if (Desc.VarType == EContainerType::Array)
	{
		if (!Property->IsA<FArrayProperty>())
		{
			return false;
		}

		FArrayProperty* ArrayProperty = CastChecked<FArrayProperty>(Property);
		void* ArrayAddr = ArrayProperty->ContainerPtrToValuePtr<void>(OuterObject);
		auto ArrayPtr = ArrayProperty->GetPropertyValuePtr(ArrayAddr);

		if (VarDescs.Num() == VarDepth + 1)
		{
			int32 Index = Desc.ArrayIndex;
			if (Index >= ArrayPtr->Num())
			{
				return false;
			}

			int32 Stride = Dest->ArrayDim * Dest->ElementSize;
			int8* InnerAddr = static_cast<int8*>(ArrayPtr->GetData());
			void* InnerItemAddr = InnerAddr + Index * Stride;

			Dest->CopyCompleteValue(DestAddr, InnerItemAddr);

			return true;
		}

		FProperty* InnerProperty = ArrayProperty->Inner;
		if (!InnerProperty->IsA<FStructProperty>())
		{
			return false;
		}

		FStructProperty* StructProperty = CastChecked<FStructProperty>(InnerProperty);
		int32 Index = Desc.ArrayIndex;
		if (Index >= ArrayPtr->Num())
		{
			return false;
		}

		int32 Stride = StructProperty->ArrayDim * StructProperty->ElementSize;
		int8* InnerAddr = static_cast<int8*>(ArrayPtr->GetData());
		void* InnerItemAddr = InnerAddr + Index * Stride;

		return CopyTerminalProperty(VarDescs, VarDepth + 1, StructProperty, InnerItemAddr, Dest, DestAddr);
	}
	else if (Desc.VarType == EContainerType::Map)
	{
		if (!Property->IsA<FMapProperty>())
		{
			return false;
		}

		FMapProperty* MapProperty = CastChecked<FMapProperty>(Property);
		FProperty* KeyProperty = MapProperty->KeyProp;
		if (!KeyProperty->IsA<FStrProperty>())
		{
			return false;
		}

		if (VarDescs.Num() == VarDepth + 1)
		{
			void* MapAddr = MapProperty->ContainerPtrToValuePtr<void>(OuterObject);
			auto MapPtr = MapProperty->GetPropertyValuePtr(MapAddr);
			FString Key = Desc.MapKey;
			uint8* ValueAddr = MapPtr->FindValue(
				&Key,
				MapProperty->MapLayout,
				[KeyProperty](const void* Key) { return KeyProperty->GetValueTypeHash(Key); },
				[KeyProperty](const void* A, const void* B) { return KeyProperty->Identical(A, B); }
			);
			if (ValueAddr == nullptr)
			{
				return false;
			}

			Dest->CopyCompleteValue(DestAddr, ValueAddr);

			return true;
		}

		FProperty* ValueProperty = MapProperty->ValueProp;
		if (!ValueProperty->IsA<FStructProperty>())
		{
			return false;
		}

		FStructProperty* StructProperty = CastChecked<FStructProperty>(ValueProperty);
		void* MapAddr = MapProperty->ContainerPtrToValuePtr<void>(OuterObject);
		auto MapPtr = MapProperty->GetPropertyValuePtr(MapAddr);
		FString Key = Desc.MapKey;
		uint8* ValueAddr = MapPtr->FindValue(
			&Key,
			MapProperty->MapLayout,
			[KeyProperty](const void* Key) { return KeyProperty->GetValueTypeHash(Key); },
			[KeyProperty](const void* A, const void* B) { return KeyProperty->Identical(A, B); }
		);
		if (ValueAddr == nullptr)
		{
			return false;
		}

		return CopyTerminalProperty(VarDescs, VarDepth + 1, StructProperty, ValueAddr, Dest, DestAddr);
	}

	return false;
}

void UVariableGetterFunctionLibarary::GetBooleanVariableByName(UObject* Target, FName VarName, bool& Success, bool& Result)
{
	Success = false;

	if (Target == nullptr)
	{
		return;
	}

	FBoolProperty* Property = FindFProperty<FBoolProperty>(Target->GetClass(), VarName);
	if (Property == nullptr)
	{
		return;
	}

	Result = Property->GetPropertyValue_InContainer(Target);
	Success = true;
}

void UVariableGetterFunctionLibarary::GetByteVariableByName(UObject* Target, FName VarName, bool& Success, uint8& Result)
{
	Success = false;

	if (Target == nullptr)
	{
		return;
	}

	FByteProperty* Property = FindFProperty<FByteProperty>(Target->GetClass(), VarName);
	if (Property == nullptr)
	{
		return;
	}

	Result = Property->GetPropertyValue_InContainer(Target);
	Success = true;
}

void UVariableGetterFunctionLibarary::GetClassVariableByName(UObject* Target, FName VarName, bool& Success, UClass*& Result)
{
	Success = false;

	if (Target == nullptr)
	{
		return;
	}

	FClassProperty* Property = FindFProperty<FClassProperty>(Target->GetClass(), VarName);
	if (Property == nullptr)
	{
		return;
	}

	Result = Property->GetPropertyValue_InContainer(Target)->StaticClass();
	Success = true;
}

void UVariableGetterFunctionLibarary::GetIntVariableByName(UObject* Target, FName VarName, bool& Success, int32& Result)
{
	Success = false;

	if (Target == nullptr)
	{
		return;
	}

	FIntProperty* Property = FindFProperty<FIntProperty>(Target->GetClass(), VarName);
	if (Property == nullptr)
	{
		return;
	}

	Result = Property->GetPropertyValue_InContainer(Target);
	Success = true;
}

void UVariableGetterFunctionLibarary::GetInt64VariableByName(UObject* Target, FName VarName, bool& Success, int64& Result)
{
	Success = false;

	if (Target == nullptr)
	{
		return;
	}

	FInt64Property* Property = FindFProperty<FInt64Property>(Target->GetClass(), VarName);
	if (Property == nullptr)
	{
		return;
	}

	Result = Property->GetPropertyValue_InContainer(Target);
	Success = true;
}

void UVariableGetterFunctionLibarary::GetFloatVariableByName(UObject* Target, FName VarName, bool& Success, float& Result)
{
	Success = false;

	if (Target == nullptr)
	{
		return;
	}

	FFloatProperty* Property = FindFProperty<FFloatProperty>(Target->GetClass(), VarName);
	if (Property == nullptr)
	{
		return;
	}

	Result = Property->GetPropertyValue_InContainer(Target);
	Success = true;
}

void UVariableGetterFunctionLibarary::GetFloat64VariableByName(UObject* Target, FName VarName, bool& Success, double& Result)
{
	Success = false;

	if (Target == nullptr)
	{
		return;
	}

	FDoubleProperty* Property = FindFProperty<FDoubleProperty>(Target->GetClass(), VarName);
	if (Property == nullptr)
	{
		return;
	}

	Result = Property->GetPropertyValue_InContainer(Target);
	Success = true;
}

void UVariableGetterFunctionLibarary::GetNameVariableByName(UObject* Target, FName VarName, bool& Success, FName& Result)
{
	Success = false;

	if (Target == nullptr)
	{
		return;
	}

	FNameProperty* Property = FindFProperty<FNameProperty>(Target->GetClass(), VarName);
	if (Property == nullptr)
	{
		return;
	}

	Result = Property->GetPropertyValue_InContainer(Target);
	Success = true;
}

void UVariableGetterFunctionLibarary::GetObjectVariableByName(UObject* Target, FName VarName, bool& Success, UObject*& Result)
{
	Success = false;

	if (Target == nullptr)
	{
		return;
	}

	FObjectProperty* Property = FindFProperty<FObjectProperty>(Target->GetClass(), VarName);
	if (Property == nullptr)
	{
		return;
	}

	Result = Property->GetPropertyValue_InContainer(Target);
	Success = true;
}

void UVariableGetterFunctionLibarary::GetStringVariableByName(UObject* Target, FName VarName, bool& Success, FString& Result)
{
	Success = false;

	if (Target == nullptr)
	{
		return;
	}

	FStrProperty* Property = FindFProperty<FStrProperty>(Target->GetClass(), VarName);
	if (Property == nullptr)
	{
		return;
	}

	Result = Property->GetPropertyValue_InContainer(Target);
	Success = true;
}

void UVariableGetterFunctionLibarary::GetTextVariableByName(UObject* Target, FName VarName, bool& Success, FText& Result)
{
	Success = false;

	if (Target == nullptr)
	{
		return;
	}

	FTextProperty* Property = FindFProperty<FTextProperty>(Target->GetClass(), VarName);
	if (Property == nullptr)
	{
		return;
	}

	Result = Property->GetPropertyValue_InContainer(Target);
	Success = true;
}

void UVariableGetterFunctionLibarary::GetStructVariableByName(UObject* Target, FName VarName, bool& Success, UProperty*& Result)
{
	check(0);
}

void UVariableGetterFunctionLibarary::GenericGetStructVariableByName(UObject* Target, FName VarName, bool& Success, void* Result)
{
	Success = false;

	if (Target == nullptr)
	{
		return;
	}

	FStructProperty* Property = FindFProperty<FStructProperty>(Target->GetClass(), VarName);
	if (Property == nullptr)
	{
		return;
	}

	void* StructProperty = Property->ContainerPtrToValuePtr<void>(Target);
	if (StructProperty == nullptr)
	{
		return;
	}

	Property->Struct->CopyScriptStruct(Result, StructProperty);
	Success = true;
}

void UVariableGetterFunctionLibarary::GetEnumVariableByName(UObject* Target, FName VarName, bool& Success, UProperty*& Result)
{
	check(0);
}

void UVariableGetterFunctionLibarary::GenericGetEnumVariableByName(UObject* Target, FName VarName, bool& Success, void* Result)
{
	Success = false;

	if (Target == nullptr)
	{
		return;
	}

	FByteProperty* Property = FindFProperty<FByteProperty>(Target->GetClass(), VarName);
	if (Property == nullptr)
	{
		return;
	}

	void* EnumProperty = Property->ContainerPtrToValuePtr<void>(Target);
	if (EnumProperty == nullptr)
	{
		return;
	}

	Property->CopyCompleteValue(Result, EnumProperty);
	Success = true;
}

void UVariableGetterFunctionLibarary::GetArrayVariableByName(UObject* Target, FName VarName, bool& Success, UProperty*& Result)
{
	check(0);
}

void UVariableGetterFunctionLibarary::GenericGetArrayVariableByName(UObject* Target, FName VarName, bool& Success, void* Result)
{
	Success = false;

	if (Target == nullptr)
	{
		return;
	}

	FArrayProperty* Property = FindFProperty<FArrayProperty>(Target->GetClass(), VarName);
	if (Property == nullptr)
	{
		return;
	}

	void* ArrayProperty = Property->ContainerPtrToValuePtr<void>(Target);
	if (ArrayProperty == nullptr)
	{
		return;
	}

	Property->CopyCompleteValue(Result, ArrayProperty);
	Success = true;
}

void UVariableGetterFunctionLibarary::GetSetVariableByName(UObject* Target, FName VarName, bool& Success, UProperty*& Result)
{
	check(0);
}

void UVariableGetterFunctionLibarary::GenericGetSetVariableByName(UObject* Target, FName VarName, bool& Success, void* Result)
{
	Success = false;

	if (Target == nullptr)
	{
		return;
	}

	FSetProperty* Property = FindFProperty<FSetProperty>(Target->GetClass(), VarName);
	if (Property == nullptr)
	{
		return;
	}

	void* SetProperty = Property->ContainerPtrToValuePtr<void>(Target);
	if (SetProperty == nullptr)
	{
		return;
	}

	Property->CopyCompleteValue(Result, SetProperty);
	Success = true;
}

void UVariableGetterFunctionLibarary::GetMapVariableByName(UObject* Target, FName VarName, bool& Success, UProperty*& Result)
{
	check(0);
}

void UVariableGetterFunctionLibarary::GenericGetMapVariableByName(UObject* Target, FName VarName, bool& Success, void* Result)
{
	Success = false;

	if (Target == nullptr)
	{
		return;
	}

	FMapProperty* Property = FindFProperty<FMapProperty>(Target->GetClass(), VarName);
	if (Property == nullptr)
	{
		return;
	}

	void* MapProperty = Property->ContainerPtrToValuePtr<void>(Target);
	if (MapProperty == nullptr)
	{
		return;
	}

	Property->CopyCompleteValue(Result, MapProperty);
	Success = true;
}

void UVariableGetterFunctionLibarary::GetNestedVariableByName(UObject* Target, FName VarName, bool& Success, UProperty*& Result)
{
	check(0);
}

void UVariableGetterFunctionLibarary::GenericGetNestedVariableByName(UObject* Target, FName VarName, bool& Success, UProperty* ResultProperty, void* ResultAddr)
{
	TArray<FString> Vars;
	TArray<FVarDescription> VarDescs;
	SplitVarName(VarName.ToString(), &Vars);
	AnalyzeVarNames(Vars, &VarDescs);

	Success = CopyTerminalProperty(VarDescs, 0, Target, ResultProperty, ResultAddr);
}