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

#include "VariableSetterFunctionLibrary.generated.h"

UCLASS()
class VARIABLEACCESSFUNCTIONLIBRARY_API UVariableSetterFunctionLibarary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
	static void SetBooleanVariableByName(UObject* Target, FName VarName, bool NewValue, bool& Success, bool& Result);

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
	static void SetByteVariableByName(UObject* Target, FName VarName, uint8 NewValue, bool& Success, uint8& Result);

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
	static void SetClassVariableByName(UObject* Target, FName VarName, UClass* NewValue, bool& Success, UClass*& Result);

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
	static void SetIntVariableByName(UObject* Target, FName VarName, int32 NewValue, bool& Success, int32& Result);

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
	static void SetInt64VariableByName(UObject* Target, FName VarName, int64 NewValue, bool& Success, int64& Result);

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
	static void SetFloatVariableByName(UObject* Target, FName VarName, float NweValue, bool& Success, float& Result);


	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
	static void SetNameVariableByName(UObject* Target, FName VarName, FName NewValue, bool& Success, FName& Result);

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
	static void SetObjectVariableByName(UObject* Target, FName VarName, UObject* NewValue, bool& Success, UObject*& Result);

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
	static void SetStringVariableByName(UObject* Target, FName VarName, FString NewValue, bool& Success, FString& Result);

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
	static void SetTextVariableByName(UObject* Target, FName VarName, FText NewValue, bool& Success, FText& Result);

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, CustomThunk, meta = (CustomStructureParam = "Result,NewValue"))
	static void SetStructVariableByName(UObject* Target, FName VarName, bool& Success, UProperty*& Result, UProperty* NewValue);

	static void GenericSetStructVariableByName(UObject* Target, FName VarName, bool& Success, void* Result, void* NewValue);

	DECLARE_FUNCTION(execSetStructVariableByName)
	{
		P_GET_OBJECT(UObject, Target);
		P_GET_PROPERTY(FNameProperty, VarName);

		P_GET_PROPERTY_REF(FBoolProperty, Success);

		Stack.StepCompiledIn<FStructProperty>(NULL);
		void* Result = Stack.MostRecentPropertyAddress;

		FProperty* ResultProperty = Stack.MostRecentProperty;
		int32 PropertySize = ResultProperty->ElementSize * ResultProperty->ArrayDim;
		void* NewValueAddr = FMemory_Alloca(PropertySize);
		ResultProperty->InitializeValue(NewValueAddr);
		Stack.MostRecentPropertyAddress = NULL;
		Stack.StepCompiledIn<FProperty>(NewValueAddr);
		P_FINISH;
		P_NATIVE_BEGIN;

		GenericSetStructVariableByName(Target, VarName, Success, Result, NewValueAddr);

		P_NATIVE_END;
		ResultProperty->DestroyValue(NewValueAddr);
	}

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, CustomThunk, meta = (CustomStructureParam = "Result,NewValue"))
	static void SetEnumVariableByName(UObject* Target, FName VarName, bool& Success, UProperty*& Result, UProperty* NewValue);

	static void GenericSetEnumVariableByName(UObject* Target, FName VarName, bool& Success, void* Result, void* NewValue);

	DECLARE_FUNCTION(execSetEnumVariableByName)
	{
		P_GET_OBJECT(UObject, Target);
		P_GET_PROPERTY(FNameProperty, VarName);

		P_GET_PROPERTY_REF(FBoolProperty, Success);

		Stack.StepCompiledIn<FByteProperty>(NULL);
		void* Result = Stack.MostRecentPropertyAddress;

		FProperty* ResultProperty = Stack.MostRecentProperty;
		int32 PropertySize = ResultProperty->ElementSize * ResultProperty->ArrayDim;
		void* NewValue = FMemory_Alloca(PropertySize);
		ResultProperty->InitializeValue(NewValue);
		Stack.MostRecentPropertyAddress = NULL;
		Stack.StepCompiledIn<FProperty>(NewValue);
		P_FINISH;
		P_NATIVE_BEGIN;

		GenericSetEnumVariableByName(Target, VarName, Success, Result, NewValue);

		P_NATIVE_END;
		ResultProperty->DestroyValue(NewValue);
	}

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, CustomThunk, meta = (CustomStructureParam = "Result,NewValue"))
	static void SetArrayVariableByName(UObject* Target, FName VarName, bool& Success, UProperty*& Result, UProperty* NewValue);

	static void GenericSetArrayVariableByName(UObject* Target, FName VarName, bool& Success, void* Result, void* NewValue);

	DECLARE_FUNCTION(execSetArrayVariableByName)
	{
		P_GET_OBJECT(UObject, Target);
		P_GET_PROPERTY(FNameProperty, VarName);
		P_GET_PROPERTY_REF(FBoolProperty, Success);

		Stack.StepCompiledIn<FArrayProperty>(NULL);
		void* Result = Stack.MostRecentPropertyAddress;

		FProperty* ResultProperty = Stack.MostRecentProperty;
		int32 PropertySize = ResultProperty->ElementSize * ResultProperty->ArrayDim;
		void* NewValue = FMemory_Alloca(PropertySize);
		ResultProperty->InitializeValue(NewValue);
		Stack.MostRecentPropertyAddress = NULL;
		Stack.StepCompiledIn<FProperty>(NewValue);
		P_FINISH;
		P_NATIVE_BEGIN;

		GenericSetArrayVariableByName(Target, VarName, Success, Result, NewValue);

		P_NATIVE_END;
		ResultProperty->DestroyValue(NewValue);
	}

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, CustomThunk, meta = (CustomStructureParam = "Result,NewValue"))
	static void SetSetVariableByName(UObject* Target, FName VarName, bool& Success, UProperty*& Result, UProperty* NewValue);

	static void GenericSetSetVariableByName(UObject* Target, FName VarName, bool& Success, void* Result, void* NewValue);

	DECLARE_FUNCTION(execSetSetVariableByName)
	{
		P_GET_OBJECT(UObject, Target);
		P_GET_PROPERTY(FNameProperty, VarName);
		P_GET_PROPERTY_REF(FBoolProperty, Success);

		Stack.StepCompiledIn<FSetProperty>(NULL);
		void* Result = Stack.MostRecentPropertyAddress;

		FProperty* ResultProperty = Stack.MostRecentProperty;
		int32 PropertySize = ResultProperty->ElementSize * ResultProperty->ArrayDim;
		void* NewValue = FMemory_Alloca(PropertySize);
		ResultProperty->InitializeValue(NewValue);
		Stack.MostRecentPropertyAddress = NULL;
		Stack.StepCompiledIn<FProperty>(NewValue);
		P_FINISH;
		P_NATIVE_BEGIN;

		GenericSetSetVariableByName(Target, VarName, Success, Result, NewValue);

		P_NATIVE_END;
		ResultProperty->DestroyValue(NewValue);
	}

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, CustomThunk, meta = (CustomStructureParam = "Result,NewValue"))
	static void SetMapVariableByName(UObject* Target, FName VarName, bool& Success, UProperty*& Result, UProperty* NewValue);

	static void GenericSetMapVariableByName(UObject* Target, FName VarName, bool& Success, void* Result, void* NewValue);

	DECLARE_FUNCTION(execSetMapVariableByName)
	{
		P_GET_OBJECT(UObject, Target);
		P_GET_PROPERTY(FNameProperty, VarName);
		P_GET_PROPERTY_REF(FBoolProperty, Success);

		Stack.StepCompiledIn<FMapProperty>(NULL);
		void* Result = Stack.MostRecentPropertyAddress;

		FProperty* ResultProperty = Stack.MostRecentProperty;
		int32 PropertySize = ResultProperty->ElementSize * ResultProperty->ArrayDim;
		void* NewValue = FMemory_Alloca(PropertySize);
		ResultProperty->InitializeValue(NewValue);
		Stack.MostRecentPropertyAddress = NULL;
		Stack.StepCompiledIn<FProperty>(NewValue);
		P_FINISH;
		P_NATIVE_BEGIN;

		GenericSetMapVariableByName(Target, VarName, Success, Result, NewValue);

		P_NATIVE_END;
		ResultProperty->DestroyValue(NewValue);
	}

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly, CustomThunk, meta = (CustomStructureParam = "Result,NewValue"))
	static void SetNestedVariableByName(UObject* Target, FName VarName, bool& Success, UProperty*& Result, UProperty* NewValue);

	static void GenericSetNestedVariableByName(UObject* Target, FName VarName, bool& Success, UProperty* ResultProperty,
		void* ResultAddr, UProperty* NewValueProperty, void* NewValueAddr);

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

		GenericSetNestedVariableByName(Target, VarName, Success, ResultProperty, ResultAddr, NewValueProperty, NewValueAddr);

		P_NATIVE_END;
		ResultProperty->DestroyValue(NewValueAddr);
	}
};
