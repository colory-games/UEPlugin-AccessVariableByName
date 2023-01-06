/*!
 * AccessVariableByName
 *
 * Copyright (c) 2022 Colory Games
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#pragma once

#include "VariableAccessFunctionLibraryUtils.h"

extern const FName ExecThenPinName;
extern const FName VarNamePinName;
extern const FName ExtendIfNotPresentPinName;
extern const FName SuccessPinName;
extern const FName ResultPinNamePrefix;
extern const FName NewValuePinNamePrefix;

extern const FString ExecThenPinFriendlyName;
extern const FString TargetPinFriendlyName;
extern const FString VarNamePinFriendlyName;
extern const FString ExtendIfNotPresentPinFriendlyName;
extern const FString SuccessPinFriendlyName;

struct TerminalProperty
{
	EPinContainerType ContainerType = EPinContainerType::None;
	FProperty* Property = nullptr;
};

FEdGraphPinType CreateDefaultPinType();
UClass* GetClassFromNode(const UEdGraphNode* Node);
TerminalProperty GetTerminalProperty(
	const TArray<FVarDescription>& VarDescs, int32 VarDepth, UClass* OuterClass, const FAccessVariableParams& Params);
