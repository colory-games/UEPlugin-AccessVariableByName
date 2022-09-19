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
extern const FName NewValuePinName;
extern const FName ResultPinNamePrefix;

extern const FString ExecThenPinFriendlyName;
extern const FString TargetPinFriendlyName;
extern const FString VarNamePinFriendlyName;
extern const FString SuccessPinFriendlyName;
extern const FString NewValuePinFriendlyName;

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

void SplitVarName(const FString& In, TArray<FString>* Out);
void AnalyzeVarNames(const TArray<FString>& VarNames, TArray<FVarDescription>* VarDescs);
