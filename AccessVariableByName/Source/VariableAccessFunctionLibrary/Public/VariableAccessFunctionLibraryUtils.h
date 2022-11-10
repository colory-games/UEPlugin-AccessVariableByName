/*!
 * AccessVariableByName
 *
 * Copyright (c) 2022 Colory Games
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#pragma once

enum EArrayAccessType
{
	ArrayAccessType_None,
	ArrayAccessType_Integer,
	ArrayAccessType_String,
};

struct FArrayAccessValue
{
	int32 Integer;
	FString String;
};

struct FVarDescription
{
	bool bIsValid;
	FString VarName;
	EArrayAccessType ArrayAccessType;
	FArrayAccessValue ArrayAccessValue;
};

namespace FVariableAccessFunctionLibraryUtils
{
	VARIABLEACCESSFUNCTIONLIBRARY_API FProperty* GetScriptStructProperty(UScriptStruct* ScriptStruct, FString VarName);
	VARIABLEACCESSFUNCTIONLIBRARY_API bool HandleTerminalProperty(const TArray<FVarDescription>& VarDescs, int32 VarDepth, UObject* OuterObject, FProperty* Dest,
		void* DestAddr, FProperty* NewValue, void* NewValueAddr);
	VARIABLEACCESSFUNCTIONLIBRARY_API void SplitVarName(const FString& In, TArray<FString>* Out);
	VARIABLEACCESSFUNCTIONLIBRARY_API void AnalyzeVarNames(const TArray<FString>& VarNames, TArray<FVarDescription>* VarDescs);
}
