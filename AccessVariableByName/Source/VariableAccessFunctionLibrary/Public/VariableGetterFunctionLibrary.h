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

#include "VariableGetterFunctionLibrary.generated.h"

UCLASS()
class VARIABLEACCESSFUNCTIONLIBRARY_API UVariableGetterFunctionLibarary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
	static void GetBooleanVariableByName(UObject* Target, FName VarName, bool& Success, bool& Result);

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
	static void GetByteVariableByName(UObject* Target, FName VarName, bool& Success, uint8& Result);

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
	static void GetClassVariableByName(UObject* Target, FName VarName, bool& Success, UClass*& Result);

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
	static void GetIntVariableByName(UObject* Target, FName VarName, bool& Success, int32& Result);

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
	static void GetInt64VariableByName(UObject* Target, FName VarName, bool& Success, int64& Result);

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
	static void GetFloatVariableByName(UObject* Target, FName VarName, bool& Success, float& Result);

	// @remove-start UE_VERSION=4.26.0,4.27.0
	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
	static void GetFloat64VariableByName(UObject* Target, FName VarName, bool& Success, double& Result);
	// @remove-end

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
	static void GetNameVariableByName(UObject* Target, FName VarName, bool& Success, FName& Result);

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
	static void GetObjectVariableByName(UObject* Target, FName VarName, bool& Success, UObject*& Result);

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
	static void GetStringVariableByName(UObject* Target, FName VarName, bool& Success, FString& Result);

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
	static void GetTextVariableByName(UObject* Target, FName VarName, bool& Success, FText& Result);

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, CustomThunk, meta = (CustomStructureParam = "Result"))
	static void GetStructVariableByName(UObject* Target, FName VarName, bool& Success, UProperty*& Result);

	static void GenericGetStructVariableByName(UObject* Target, FName VarName, bool& Success, void* Result);

	DECLARE_FUNCTION(execGetStructVariableByName)
	{
		P_GET_OBJECT(UObject, Target);
		P_GET_PROPERTY(FNameProperty, VarName);
		P_GET_PROPERTY_REF(FBoolProperty, Success);

		Stack.StepCompiledIn<FStructProperty>(NULL);
		void* Result = Stack.MostRecentPropertyAddress;
		P_FINISH;
		P_NATIVE_BEGIN;

		GenericGetStructVariableByName(Target, VarName, Success, Result);

		P_NATIVE_END;
	}

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, CustomThunk, meta = (CustomStructureParam = "Result"))
	static void GetEnumVariableByName(UObject* Target, FName VarName, bool& Success, UProperty*& Result);

	static void GenericGetEnumVariableByName(UObject* Target, FName VarName, bool& Success, void* Result);

	DECLARE_FUNCTION(execGetEnumVariableByName)
	{
		P_GET_OBJECT(UObject, Target);
		P_GET_PROPERTY(FNameProperty, VarName);
		P_GET_PROPERTY_REF(FBoolProperty, Success);

		Stack.StepCompiledIn<FByteProperty>(NULL);
		void* Result = Stack.MostRecentPropertyAddress;
		P_FINISH;
		P_NATIVE_BEGIN;

		GenericGetEnumVariableByName(Target, VarName, Success, Result);

		P_NATIVE_END;
	}

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, CustomThunk, meta = (CustomStructureParam = "Result"))
	static void GetArrayVariableByName(UObject* Target, FName VarName, bool& Success, UProperty*& Result);

	static void GenericGetArrayVariableByName(UObject* Target, FName VarName, bool& Success, void* Result);

	DECLARE_FUNCTION(execGetArrayVariableByName)
	{
		P_GET_OBJECT(UObject, Target);
		P_GET_PROPERTY(FNameProperty, VarName);
		P_GET_PROPERTY_REF(FBoolProperty, Success);

		Stack.StepCompiledIn<FArrayProperty>(NULL);
		void* Result = Stack.MostRecentPropertyAddress;
		P_FINISH;
		P_NATIVE_BEGIN;

		GenericGetArrayVariableByName(Target, VarName, Success, Result);

		P_NATIVE_END;
	}

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, CustomThunk, meta = (CustomStructureParam = "Result"))
	static void GetSetVariableByName(UObject* Target, FName VarName, bool& Success, UProperty*& Result);

	static void GenericGetSetVariableByName(UObject* Target, FName VarName, bool& Success, void* Result);

	DECLARE_FUNCTION(execGetSetVariableByName)
	{
		P_GET_OBJECT(UObject, Target);
		P_GET_PROPERTY(FNameProperty, VarName);
		P_GET_PROPERTY_REF(FBoolProperty, Success);

		Stack.StepCompiledIn<FSetProperty>(NULL);
		void* Result = Stack.MostRecentPropertyAddress;
		P_FINISH;
		P_NATIVE_BEGIN;

		GenericGetSetVariableByName(Target, VarName, Success, Result);

		P_NATIVE_END;
	}

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, CustomThunk, meta = (CustomStructureParam = "Result"))
	static void GetMapVariableByName(UObject* Target, FName VarName, bool& Success, UProperty*& Result);

	static void GenericGetMapVariableByName(UObject* Target, FName VarName, bool& Success, void* Result);

	DECLARE_FUNCTION(execGetMapVariableByName)
	{
		P_GET_OBJECT(UObject, Target);
		P_GET_PROPERTY(FNameProperty, VarName);
		P_GET_PROPERTY_REF(FBoolProperty, Success);

		Stack.StepCompiledIn<FMapProperty>(NULL);
		void* Result = Stack.MostRecentPropertyAddress;
		P_FINISH;
		P_NATIVE_BEGIN;

		GenericGetMapVariableByName(Target, VarName, Success, Result);

		P_NATIVE_END;
	}

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, CustomThunk, meta = (CustomStructureParam = "Result"))
	static void GetNestedVariableByName(UObject* Target, FName VarName, bool& Success, UProperty*& Result);

	static void GenericGetNestedVariableByName(
		UObject* Target, FName VarName, bool& Success, UProperty* ResultProperty, void* ResultAddr);

	DECLARE_FUNCTION(execGetNestedVariableByName)
	{
		P_GET_OBJECT(UObject, Target);
		P_GET_PROPERTY(FNameProperty, VarName);
		P_GET_PROPERTY_REF(FBoolProperty, Success);

		Stack.StepCompiledIn<FMapProperty>(NULL);
		void* ResultAddr = Stack.MostRecentPropertyAddress;
		FProperty* ResultProperty = Stack.MostRecentProperty;
		P_FINISH;

		P_NATIVE_BEGIN;

		GenericGetNestedVariableByName(Target, VarName, Success, ResultProperty, ResultAddr);

		P_NATIVE_END;
	}
};
