/*!
 * AccessVariableByName
 *
 * Copyright (c) 2022 Colory Games
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "VariableGetterFunctionLibrary.h"

#include "UObject/TextProperty.h"
#include "VariableAccessFunctionLibraryUtils.h"

void UVariableGetterFunctionLibarary::GenericGetNestedVariableByName(
	UObject* Target, FName VarName, bool& Success, FProperty* ResultProperty, void* ResultAddr, const FAccessVariableParams& Params)
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
		VarDescs, 0, Target, ResultProperty, ResultAddr, nullptr, nullptr, Params);
}

void UVariableGetterFunctionLibarary::GetNestedVariableByName(
	UObject* Target, FName VarName, FAccessVariableParams Params, bool& Success, UProperty*& Result)
{
	check(0);
}

void UVariableGetterFunctionLibarary::GetNestedVariableByNamePure(
	UObject* Target, FName VarName, FAccessVariableParams Params, bool& Success, UProperty*& Result)
{
	check(0);
}
