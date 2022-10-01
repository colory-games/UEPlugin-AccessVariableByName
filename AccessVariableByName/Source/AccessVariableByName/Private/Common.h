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
extern const FName VarNamePinName;
extern const FName SuccessPinName;
extern const FName ResultPinNamePrefix;
extern const FName NewValuePinNamePrefix;

extern const FString ExecThenPinFriendlyName;
extern const FString TargetPinFriendlyName;
extern const FString VarNamePinFriendlyName;
extern const FString SuccessPinFriendlyName;

enum EArrayAccessType
{
	None,
	Integer,
	String,
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

FProperty* GetTerminalProperty(const TArray<FVarDescription>& VarDescs, int32 VarDepth, UClass* OuterClass);
bool HandleTerminalProperty(const TArray<FVarDescription>& VarDescs, int32 VarDepth, UObject* OuterObject, FProperty* Dest, void* DestAddr, FProperty* NewValue, void* NewValueAddr);
void SplitVarName(const FString& In, TArray<FString>* Out);
void AnalyzeVarNames(const TArray<FString>& VarNames, TArray<FVarDescription>* VarDescs);

FEdGraphPinType CreateDefaultPinType();
UClass* GetClassFromNode(const UEdGraphNode* Node);

