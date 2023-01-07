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
	static void GenericSetNestedVariableByName(UObject* Target, FName VarName, bool& Success, FProperty* ResultProperty,
		void* ResultAddr, FProperty* NewValueProperty, void* NewValueAddr, const FAccessVariableParams& Params);

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, CustomThunk, meta = (CustomStructureParam = "Result,NewValue"))
	static void SetNestedVariableByName(
		UObject* Target, FName VarName, FAccessVariableParams Params, bool& Success, UProperty*& Result, UProperty* NewValue);

	DECLARE_FUNCTION(execSetNestedVariableByName)
	{
		P_GET_OBJECT(UObject, Target);
		P_GET_PROPERTY(FNameProperty, VarName);
		P_GET_STRUCT(FAccessVariableParams, Params);
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

		GenericSetNestedVariableByName(
			Target, VarName, Success, ResultProperty, ResultAddr, NewValueProperty, NewValueAddr, Params);

		P_NATIVE_END;
		ResultProperty->DestroyValue(NewValueAddr);
	}
};
