/*!
 * AccessVariableByName
 *
 * Copyright (c) 2022 Colory Games
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#pragma once

extern const FName ExecThenPinName;
extern const FName TargetPinName;
extern const FName VarNamePinName;
extern const FName SuccessPinName;
extern const FName ResultPinNamePrefix;
extern const FName NewValuePinNamePrefix;

extern const FString ExecThenPinFriendlyName;
extern const FString TargetPinFriendlyName;
extern const FString VarNamePinFriendlyName;
extern const FString SuccessPinFriendlyName;

enum EContainerType
{
	None,
	Array,
	Map,
};

struct FVarDescription
{
	bool bIsValid;
	FString VarName;
	EContainerType VarType;
	int32 ArrayIndex;
	FString MapKey;
};

FProperty* GetTerminalProperty(const TArray<FVarDescription>& VarDescs, int32 VarDepth, UClass* OuterClass);
void SplitVarName(const FString& In, TArray<FString>* Out);
void AnalyzeVarNames(const TArray<FString>& VarNames, TArray<FVarDescription>* VarDescs);
