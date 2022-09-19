/*!
 * AccessVariableByName
 *
 * Copyright (c) 2022 Colory Games
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#pragma once

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
