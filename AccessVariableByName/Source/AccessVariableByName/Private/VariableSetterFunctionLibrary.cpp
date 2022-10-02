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

// @remove-start UE_VERSION=4.26.0,4.27.0
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
// @remove-end

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

#ifdef AVBN_FREE_VERSION
	if (VarDescs.Num() >= 2)
	{
		UE_LOG(LogTemp, Error, TEXT("Nested property is not supported on the free version. Please consider to buy full version at Marketplace."));
		return;
	}
#endif // AVBN_FREE_VERSION

	Success = HandleTerminalProperty(VarDescs, 0, Target, ResultProperty, ResultAddr, NewValue, NewValueAddr);
}