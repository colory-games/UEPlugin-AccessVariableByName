/*!
 * AccessVariableByName
 *
 * Copyright (c) 2022 Colory Games
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "VariableAccessFunctionLibraryUtils.h"

#include "VariableSetterFunctionLibrary.generated.h"

UCLASS()
class VARIABLEACCESSFUNCTIONLIBRARY_API UVariableSetterFunctionLibarary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static void GenericSetNestedVariableByName(UObject* Target, FName VarName, bool& Success, UProperty* ResultProperty,
		void* ResultAddr, UProperty* NewValueProperty, void* NewValueAddr, const SetVariableParams& Params);

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, CustomThunk, meta = (CustomStructureParam = "Result,NewValue"))
	static void SetNestedVariableByName(UObject* Target, FName VarName, bool& Success, UProperty*& Result, UProperty* NewValue);

	DECLARE_FUNCTION(execSetNestedVariableByName)
	{
		P_GET_OBJECT(UObject, Target);
		P_GET_PROPERTY(FNameProperty, VarName);
		P_GET_PROPERTY_REF(FBoolProperty, Success);

		Stack.StepCompiledIn<FProperty>(NULL);
		void* ResultAddr = Stack.MostRecentPropertyAddress;
		FProperty* ResultProperty = Stack.MostRecentProperty;

		int32 PropertySize = ResultProperty->ElementSize * ResultProperty->ArrayDim;
		void* NewValueAddr = FMemory_Alloca(PropertySize);
		ResultProperty->InitializeValue(NewValueAddr);
		Stack.MostRecentPropertyAddress = NULL;
		Stack.StepCompiledIn<FProperty>(NewValueAddr);
		FProperty* NewValueProperty = Stack.MostRecentProperty;

		P_FINISH;

		P_NATIVE_BEGIN;

		SetVariableParams Params;
		GenericSetNestedVariableByName(
			Target, VarName, Success, ResultProperty, ResultAddr, NewValueProperty, NewValueAddr, Params);

		P_NATIVE_END;
		ResultProperty->DestroyValue(NewValueAddr);
	}

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, CustomThunk, meta = (CustomStructureParam = "Result,NewValue"))
	static void SetNestedVariableByNameForArray(
		UObject* Target, FName VarName, bool bSizeToFit, bool& Success, UProperty*& Result, UProperty* NewValue);

	DECLARE_FUNCTION(execSetNestedVariableByNameForArray)
	{
		P_GET_OBJECT(UObject, Target);
		P_GET_PROPERTY(FNameProperty, VarName);
		P_GET_PROPERTY(FBoolProperty, bSizeToFit);
		P_GET_PROPERTY_REF(FBoolProperty, Success);

		Stack.StepCompiledIn<FProperty>(NULL);
		void* ResultAddr = Stack.MostRecentPropertyAddress;
		FProperty* ResultProperty = Stack.MostRecentProperty;

		int32 PropertySize = ResultProperty->ElementSize * ResultProperty->ArrayDim;
		void* NewValueAddr = FMemory_Alloca(PropertySize);
		ResultProperty->InitializeValue(NewValueAddr);
		Stack.MostRecentPropertyAddress = NULL;
		Stack.StepCompiledIn<FProperty>(NewValueAddr);
		FProperty* NewValueProperty = Stack.MostRecentProperty;

		P_FINISH;

		P_NATIVE_BEGIN;

		SetVariableParams Params;
		Params.bSizeToFit = bSizeToFit;
		GenericSetNestedVariableByName(
			Target, VarName, Success, ResultProperty, ResultAddr, NewValueProperty, NewValueAddr, Params);

		P_NATIVE_END;
		ResultProperty->DestroyValue(NewValueAddr);
	}

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, CustomThunk, meta = (CustomStructureParam = "Result,NewValue"))
	static void SetNestedVariableByNameForAllTypes(
		UObject* Target, FName VarName, bool bSizeToFit, bool& Success, UProperty*& Result, UProperty* NewValue);

	DECLARE_FUNCTION(execSetNestedVariableByNameForAllTypes)
	{
		P_GET_OBJECT(UObject, Target);
		P_GET_PROPERTY(FNameProperty, VarName);
		P_GET_PROPERTY(FBoolProperty, bSizeToFit);
		P_GET_PROPERTY_REF(FBoolProperty, Success);

		Stack.StepCompiledIn<FProperty>(NULL);
		void* ResultAddr = Stack.MostRecentPropertyAddress;
		FProperty* ResultProperty = Stack.MostRecentProperty;

		int32 PropertySize = ResultProperty->ElementSize * ResultProperty->ArrayDim;
		void* NewValueAddr = FMemory_Alloca(PropertySize);
		ResultProperty->InitializeValue(NewValueAddr);
		Stack.MostRecentPropertyAddress = NULL;
		Stack.StepCompiledIn<FProperty>(NewValueAddr);
		FProperty* NewValueProperty = Stack.MostRecentProperty;

		P_FINISH;

		P_NATIVE_BEGIN;

		SetVariableParams Params;
		Params.bSizeToFit = bSizeToFit;
		GenericSetNestedVariableByName(
			Target, VarName, Success, ResultProperty, ResultAddr, NewValueProperty, NewValueAddr, Params);

		P_NATIVE_END;
		ResultProperty->DestroyValue(NewValueAddr);
	}
};
