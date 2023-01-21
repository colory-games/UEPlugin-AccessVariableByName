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

	// Include variables from a generation class (UBlueprint) if true.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Access Variable Options")
	bool bIncludeGenerationClass = false;

	// Set.

	// Create elements automatically if true when the element does not present.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Container Type Access Options")
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
