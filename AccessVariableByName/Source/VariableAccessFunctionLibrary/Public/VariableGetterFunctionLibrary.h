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

#include "VariableGetterFunctionLibrary.generated.h"

UCLASS()
class VARIABLEACCESSFUNCTIONLIBRARY_API UVariableGetterFunctionLibarary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static void GenericGetNestedVariableByName(UObject* Target, FName VarName, bool& Success, FProperty* ResultProperty,
		void* ResultAddr, const FAccessVariableParams& Params);

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, CustomThunk, meta = (CustomStructureParam = "Result"))
	static void GetNestedVariableByName(
		UObject* Target, FName VarName, FAccessVariableParams Params, bool& Success, UProperty*& Result);

	DECLARE_FUNCTION(execGetNestedVariableByName)
	{
		P_GET_OBJECT(UObject, Target);
		P_GET_PROPERTY(FNameProperty, VarName);
		P_GET_STRUCT(FAccessVariableParams, Params);
		P_GET_PROPERTY_REF(FBoolProperty, Success);

		Stack.StepCompiledIn<FMapProperty>(NULL);
		void* ResultAddr = Stack.MostRecentPropertyAddress;
		FProperty* ResultProperty = Stack.MostRecentProperty;
		P_FINISH;

		P_NATIVE_BEGIN;

		GenericGetNestedVariableByName(Target, VarName, Success, ResultProperty, ResultAddr, Params);

		P_NATIVE_END;
	}

	UFUNCTION(BlueprintPure, BlueprintInternalUseOnly, CustomThunk, meta = (CustomStructureParam = "Result"))
	static void GetNestedVariableByNamePure(
		UObject* Target, FName VarName, FAccessVariableParams Params, bool& Success, UProperty*& Result);

	DECLARE_FUNCTION(execGetNestedVariableByNamePure)
	{
		P_GET_OBJECT(UObject, Target);
		P_GET_PROPERTY(FNameProperty, VarName);
		P_GET_STRUCT(FAccessVariableParams, Params);
		P_GET_PROPERTY_REF(FBoolProperty, Success);

		Stack.StepCompiledIn<FMapProperty>(NULL);
		void* ResultAddr = Stack.MostRecentPropertyAddress;
		FProperty* ResultProperty = Stack.MostRecentProperty;
		P_FINISH;

		P_NATIVE_BEGIN;

		GenericGetNestedVariableByName(Target, VarName, Success, ResultProperty, ResultAddr, Params);

		P_NATIVE_END;
	}
};
