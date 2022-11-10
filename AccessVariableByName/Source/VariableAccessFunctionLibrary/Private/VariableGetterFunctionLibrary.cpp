/*!
 * AccessVariableByName
 *
 * Copyright (c) 2022 Colory Games
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "VariableGetterFunctionLibrary.h"

#include "UObject/Field.h"
#include "VariableAccessFunctionLibraryUtils.h"

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

// @remove-start UE_VERSION=4.26.0,4.27.0
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
// @remove-end

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

void UVariableGetterFunctionLibarary::GenericGetNestedVariableByName(
	UObject* Target, FName VarName, bool& Success, UProperty* ResultProperty, void* ResultAddr)
{
	TArray<FString> Vars;
	TArray<FVarDescription> VarDescs;
	FVariableAccessFunctionLibraryUtils::SplitVarName(VarName.ToString(), &Vars);
	FVariableAccessFunctionLibraryUtils::AnalyzeVarNames(Vars, &VarDescs);

#ifdef AVBN_FREE_VERSION
	if (VarDescs.Num() >= 2)
	{
		UE_LOG(LogTemp, Error,
			TEXT("Nested property is not supported on the free version. Please consider to buy full version at Marketplace."));
		return;
	}
	if (VarDescs.Num() == 1 && VarDescs[0].ArrayAccessType != ArrayAccessType_None)
	{
		UE_LOG(LogTemp, Error,
			TEXT("The access of Array/Map's element is not supported on the free version. "
				 "Please consider to buy full version at Marketplace."));
		return;
	}
#endif

	Success = FVariableAccessFunctionLibraryUtils::HandleTerminalProperty(
		VarDescs, 0, Target, ResultProperty, ResultAddr, nullptr, nullptr);
}
