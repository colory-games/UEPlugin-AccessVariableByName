/*!
 * AccessVariableByName
 *
 * Copyright (c) 2022 Colory Games
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "VariableSetterFunctionLibrary.h"

void UVariableSetterFunctionLibarary::SetNestedVariableByName(
	UObject* Target, FName VarName, FAccessVariableParams Params, bool& Success, UProperty*& Result, UProperty* NewValue)
{
	check(0);
}

void UVariableSetterFunctionLibarary::GenericSetNestedVariableByName(UObject* Target, FName VarName, bool& Success,
	FProperty* ResultProperty, void* ResultAddr, FProperty* NewValueProperty, void* NewValueAddr,
	const FAccessVariableParams& Params)
{
	TArray<FString> Vars;
	TArray<FVarDescription> VarDescs;
	FVariableAccessFunctionLibraryUtils::SplitVarName(VarName.ToString(), &Vars);
	FVariableAccessFunctionLibraryUtils::AnalyzeVarNames(Vars, &VarDescs);

#ifdef AVBN_FREE_VERSION
	if (VarDescs.Num() >= 2)
	{
		UE_LOG(LogTemp, Error,
			TEXT("Nested property is not supported on the free version. Please consider to buy full version at Marketplace."));
		return;
	}
	if (VarDescs.Num() == 1 && VarDescs[0].ArrayAccessType != ArrayAccessType_None)
	{
		UE_LOG(LogTemp, Error,
			TEXT("The access of Array/Map's element is not supported on the free version. "
				 "Please consider to buy full version at Marketplace."));
		return;
	}
#endif

	Success = FVariableAccessFunctionLibraryUtils::HandleTerminalProperty(
		VarDescs, 0, Target, ResultProperty, ResultAddr, NewValueProperty, NewValueAddr, Params);
}
