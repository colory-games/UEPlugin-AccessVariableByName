/*!
 * AccessVariableByName
 *
 * Copyright (c) 2022 Colory Games
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "VariableSetterFunctionLibrary.h"

#include "Common.h"

bool SetTerminalProperty(const TArray<FVarDescription>& VarDescs, int32 VarDepth, FStructProperty* OuterProperty, void* OuterAddr,
	FProperty* Dest, void* DestAddr, FProperty* NewValue, void* NewValueAddr);

bool SetTerminalPropertyInternal(const TArray<FVarDescription>& VarDescs, int32 VarDepth, FProperty* Property, void* OuterAddr,
	FProperty* Dest, void* DestAddr, FProperty* NewValue, void* NewValueAddr)
{
	const FVarDescription& Desc = VarDescs[VarDepth];

	if (Desc.VarType == EContainerType::None)
	{
		if (VarDescs.Num() == VarDepth + 1)
		{
			void* SrcAddr = Property->ContainerPtrToValuePtr<void>(OuterAddr);
			Property->CopyCompleteValue(SrcAddr, NewValueAddr);
			Property->CopyCompleteValue(DestAddr, SrcAddr);
			return true;
		}

		if (!Property->IsA<FStructProperty>())
		{
			return false;
		}
		FStructProperty* StructProperty = CastChecked<FStructProperty>(Property);
		void* StructAddr = Property->ContainerPtrToValuePtr<void>(OuterAddr);

		return SetTerminalProperty(VarDescs, VarDepth + 1, StructProperty, StructAddr, Dest, DestAddr, NewValue, NewValueAddr);
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

			Dest->CopyCompleteValue(InnerItemAddr, NewValueAddr);
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

		return SetTerminalProperty(VarDescs, VarDepth + 1, StructProperty, InnerItemAddr, Dest, DestAddr, NewValue, NewValueAddr);
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
				&Key, MapProperty->MapLayout, [KeyProperty](const void* Key) { return KeyProperty->GetValueTypeHash(Key); },
				[KeyProperty](const void* A, const void* B) { return KeyProperty->Identical(A, B); });
			if (ValueAddr == nullptr)
			{
				return false;
			}

			Dest->CopyCompleteValue(ValueAddr, NewValueAddr);
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
			&Key, MapProperty->MapLayout, [KeyProperty](const void* Key) { return KeyProperty->GetValueTypeHash(Key); },
			[KeyProperty](const void* A, const void* B) { return KeyProperty->Identical(A, B); });
		if (ValueAddr == nullptr)
		{
			return false;
		}

		return SetTerminalProperty(VarDescs, VarDepth + 1, StructProperty, ValueAddr, Dest, DestAddr, NewValue, NewValueAddr);
	}

	return false;
}

bool SetTerminalProperty(const TArray<FVarDescription>& VarDescs, int32 VarDepth, FStructProperty* OuterProperty, void* OuterAddr,
	FProperty* Dest, void* DestAddr, FProperty* NewValue, void* NewValueAddr)
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

	return SetTerminalPropertyInternal(VarDescs, VarDepth, Property, OuterAddr, Dest, DestAddr, NewValue, NewValueAddr);
}

bool SetTerminalProperty(
	const TArray<FVarDescription>& VarDescs, int32 VarDepth, UObject* OuterObject, FProperty* Dest, void* DestAddr, FProperty* NewValue, void* NewValueAddr)
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

	return SetTerminalPropertyInternal(VarDescs, VarDepth, Property, OuterObject, Dest, DestAddr, NewValue, NewValueAddr);
}

void UVariableSetterFunctionLibarary::SetBooleanVariableByName(
	UObject* Target, FName VarName, bool NewValue, bool& Success, bool& Result)
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

	Property->SetPropertyValue_InContainer(Target, NewValue);
	Result = Property->GetPropertyValue_InContainer(Target);
	Success = true;
}

void UVariableSetterFunctionLibarary::SetByteVariableByName(
	UObject* Target, FName VarName, uint8 NewValue, bool& Success, uint8& Result)
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

	Property->SetPropertyValue_InContainer(Target, NewValue);
	Result = Property->GetPropertyValue_InContainer(Target);
	Success = true;
}

void UVariableSetterFunctionLibarary::SetClassVariableByName(
	UObject* Target, FName VarName, UClass* NewValue, bool& Success, UClass*& Result)
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

	Property->SetPropertyValue_InContainer(Target, NewValue);
	Result = Property->GetPropertyValue_InContainer(Target)->StaticClass();
	Success = true;
}

void UVariableSetterFunctionLibarary::SetIntVariableByName(
	UObject* Target, FName VarName, int32 NewValue, bool& Success, int32& Result)
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

	Property->SetPropertyValue_InContainer(Target, NewValue);
	Result = Property->GetPropertyValue_InContainer(Target);
	Success = true;
}

void UVariableSetterFunctionLibarary::SetInt64VariableByName(
	UObject* Target, FName VarName, int64 NewValue, bool& Success, int64& Result)
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

	Property->SetPropertyValue_InContainer(Target, NewValue);
	Result = Property->GetPropertyValue_InContainer(Target);
	Success = true;
}

void UVariableSetterFunctionLibarary::SetFloatVariableByName(
	UObject* Target, FName VarName, float NewValue, bool& Success, float& Result)
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

	Property->SetPropertyValue_InContainer(Target, NewValue);
	Result = Property->GetPropertyValue_InContainer(Target);
	Success = true;
}

void UVariableSetterFunctionLibarary::SetFloat64VariableByName(
	UObject* Target, FName VarName, double NewValue, bool& Success, double& Result)
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

	Property->SetPropertyValue_InContainer(Target, NewValue);
	Result = Property->GetPropertyValue_InContainer(Target);
	Success = true;
}

void UVariableSetterFunctionLibarary::SetNameVariableByName(
	UObject* Target, FName VarName, FName NewValue, bool& Success, FName& Result)
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

	Property->SetPropertyValue_InContainer(Target, NewValue);
	Result = Property->GetPropertyValue_InContainer(Target);
	Success = true;
}

void UVariableSetterFunctionLibarary::SetObjectVariableByName(
	UObject* Target, FName VarName, UObject* NewValue, bool& Success, UObject*& Result)
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

	Property->SetPropertyValue_InContainer(Target, NewValue);
	Result = Property->GetPropertyValue_InContainer(Target);
	Success = true;
}

void UVariableSetterFunctionLibarary::SetStringVariableByName(
	UObject* Target, FName VarName, FString NewValue, bool& Success, FString& Result)
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

	Property->SetPropertyValue_InContainer(Target, NewValue);
	Result = Property->GetPropertyValue_InContainer(Target);
	Success = true;
}

void UVariableSetterFunctionLibarary::SetTextVariableByName(
	UObject* Target, FName VarName, FText NewValue, bool& Success, FText& Result)
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

	Property->SetPropertyValue_InContainer(Target, NewValue);
	Result = Property->GetPropertyValue_InContainer(Target);
	Success = true;
}

void UVariableSetterFunctionLibarary::SetStructVariableByName(
	UObject* Target, FName VarName, bool& Success, UProperty*& Result, UProperty* NewValue)
{
	check(0);
}

void UVariableSetterFunctionLibarary::GenericSetStructVariableByName(
	UObject* Target, FName VarName, bool& Success, void* Result, void* NewValue)
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

	Property->Struct->CopyScriptStruct(StructProperty, NewValue);
	Property->Struct->CopyScriptStruct(Result, StructProperty);
	Success = true;
}

void UVariableSetterFunctionLibarary::SetEnumVariableByName(
	UObject* Target, FName VarName, bool& Success, UProperty*& Result, UProperty* NewValue)
{
	check(0);
}

void UVariableSetterFunctionLibarary::GenericSetEnumVariableByName(
	UObject* Target, FName VarName, bool& Success, void* Result, void* NewValue)
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

	Property->CopyCompleteValue(EnumProperty, NewValue);
	Property->CopyCompleteValue(Result, EnumProperty);
	Success = true;
}

void UVariableSetterFunctionLibarary::SetArrayVariableByName(
	UObject* Target, FName VarName, bool& Success, UProperty*& Result, UProperty* NewValue)
{
	check(0);
}

void UVariableSetterFunctionLibarary::GenericSetArrayVariableByName(
	UObject* Target, FName VarName, bool& Success, void* Result, void* NewValue)
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

	Property->CopyCompleteValue(ArrayProperty, NewValue);
	Property->CopyCompleteValue(Result, ArrayProperty);
	Success = true;
}

void UVariableSetterFunctionLibarary::SetSetVariableByName(
	UObject* Target, FName VarName, bool& Success, UProperty*& Result, UProperty* NewValue)
{
	check(0);
}

void UVariableSetterFunctionLibarary::GenericSetSetVariableByName(
	UObject* Target, FName VarName, bool& Success, void* Result, void* NewValue)
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

	Property->CopyCompleteValue(SetProperty, NewValue);
	Property->CopyCompleteValue(Result, SetProperty);
	Success = true;
}

void UVariableSetterFunctionLibarary::SetMapVariableByName(
	UObject* Target, FName VarName, bool& Success, UProperty*& Result, UProperty* NewValue)
{
	check(0);
}

void UVariableSetterFunctionLibarary::GenericSetMapVariableByName(
	UObject* Target, FName VarName, bool& Success, void* Result, void* NewValue)
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

	Property->CopyCompleteValue(MapProperty, NewValue);
	Property->CopyCompleteValue(Result, MapProperty);
	Success = true;
}

void UVariableSetterFunctionLibarary::SetNestedVariableByName(UObject* Target, FName VarName, bool& Success, UProperty*& Result, UProperty* NewValue)
{
	check(0);
}

void UVariableSetterFunctionLibarary::GenericSetNestedVariableByName(
	UObject* Target, FName VarName, bool& Success, UProperty* ResultProperty, void* ResultAddr, UProperty* NewValue, void* NewValueAddr)
{
	TArray<FString> Vars;
	TArray<FVarDescription> VarDescs;
	SplitVarName(VarName.ToString(), &Vars);
	AnalyzeVarNames(Vars, &VarDescs);

	Success = SetTerminalProperty(VarDescs, 0, Target, ResultProperty, ResultAddr, NewValue, NewValueAddr);
}