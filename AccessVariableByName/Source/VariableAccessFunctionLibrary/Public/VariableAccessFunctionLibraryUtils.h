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

#include "VariableAccessFunctionLibraryUtils.generated.h"

enum EArrayAccessType
{
	ArrayAccessType_None,
	ArrayAccessType_Integer,
	ArrayAccessType_String,
};

struct FArrayAccessValue
{
	int32 Integer = -1;
	FString String = "";
};

struct FVarDescription
{
	bool bIsValid = false;
	FString VarName;
	EArrayAccessType ArrayAccessType;
	FArrayAccessValue ArrayAccessValue;
};

USTRUCT(BlueprintType)
struct VARIABLEACCESSFUNCTIONLIBRARY_API FAccessVariableParams
{
	GENERATED_BODY()

	// Common.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIncludeGenerationClass = false;

	// Set.
	UPROPERTY(EditAnywhere, BLueprintReadWrite)
	bool bExtendIfNotPresent = false;
};

UCLASS()
class VARIABLEACCESSFUNCTIONLIBRARY_API UVariableAccessUtilLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
	static FAccessVariableParams MakeAccessVariableParams(bool bIncludeGenerationClass, bool bExtendIfNotPresent);
};

namespace FVariableAccessFunctionLibraryUtils
{
VARIABLEACCESSFUNCTIONLIBRARY_API FProperty* GetScriptStructProperty(UScriptStruct* ScriptStruct, FString VarName);
VARIABLEACCESSFUNCTIONLIBRARY_API bool HandleTerminalProperty(const TArray<FVarDescription>& VarDescs, int32 VarDepth,
	UObject* OuterObject, FProperty* Dest, void* DestAddr, FProperty* NewValue, void* NewValueAddr,
	const FAccessVariableParams& Params);
VARIABLEACCESSFUNCTIONLIBRARY_API void SplitVarName(const FString& In, TArray<FString>* Out);
VARIABLEACCESSFUNCTIONLIBRARY_API void AnalyzeVarNames(const TArray<FString>& VarNames, TArray<FVarDescription>* VarDescs);
}	 // namespace FVariableAccessFunctionLibraryUtils
