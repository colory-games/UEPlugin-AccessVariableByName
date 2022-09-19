/*!
 * AccessVariableByName
 *
 * Copyright (c) 2022 Colory Games
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "Common.h"

#include "Internationalization/Regex.h"

const FName ExecThenPinName(TEXT("ExecThen"));
const FName TargetPinName(TEXT("Target"));
const FName VarNamePinName(TEXT("VarName"));
const FName SuccessPinName(TEXT("Success"));
const FName NewValuePinName(TEXT("NewValue"));
const FName ResultPinNamePrefix(TEXT("Result_"));

const FString ExecThenPinFriendlyName(TEXT(" "));
const FString TargetPinFriendlyName(TEXT("Target"));
const FString VarNamePinFriendlyName(TEXT("Var Name"));
const FString SuccessPinFriendlyName(TEXT("Success"));
const FString NewValuePinFriendlyName(TEXT("New Value"));

void SplitVarNameInternal(const FString& In, int32 StartIndex, TArray<FString>* Out)
{
	bool bInString = false;
	int32 Index = StartIndex;
	for (; Index < In.Len(); ++Index)
	{
		FString Ch = In.Mid(Index, 1);
		if (Ch == "\"")
		{
			bInString = !bInString;
		}

		if (!bInString && Ch == ".")
		{
			Out->Add(In.Mid(StartIndex, Index - StartIndex));
			SplitVarNameInternal(In, Index + 1, Out);
			break;
		}
	}
	if (Index == In.Len())
	{
		Out->Add(In.Mid(StartIndex, Index - StartIndex));
	}
}

void SplitVarName(const FString& In, TArray<FString>* Out)
{
	SplitVarNameInternal(In, 0, Out);
	*Out = Out->FilterByPredicate([](const FString& S) { return !S.IsEmpty(); });
}

void AnalyzeVarNames(const TArray<FString>& VarNames, TArray<FVarDescription>* VarDescs)
{
	for (auto& Var : VarNames)
	{
		// Map pattern.
		{
			FRegexPattern Pattern = FRegexPattern("^([a-zA-Z][a-zA-Z0-9]*)\\[\"(\\S+)\"\\]$");
			FRegexMatcher Matcher(Pattern, Var);
			if (Matcher.FindNext())
			{
				FVarDescription Desc;
				Desc.bIsValid = true;
				Desc.VarName = Matcher.GetCaptureGroup(1);
				Desc.VarType = EContainerType::Map;
				Desc.ArrayIndex = -1;
				Desc.MapKey = Matcher.GetCaptureGroup(2);
				VarDescs->Add(Desc);
				continue;
			}
		}

		// Array pattern.
		{
			FRegexPattern Pattern = FRegexPattern("^([a-zA-Z][a-zA-Z0-9]*)\\[([0-9]+)\\]$");
			FRegexMatcher Matcher(Pattern, Var);
			if (Matcher.FindNext())
			{
				FVarDescription Desc;
				Desc.bIsValid = true;
				Desc.VarName = Matcher.GetCaptureGroup(1);
				Desc.VarType = EContainerType::Array;
				Desc.ArrayIndex = FCString::Atoi(*Matcher.GetCaptureGroup(2));
				Desc.MapKey = "";
				VarDescs->Add(Desc);
				continue;
			}
		}

		// None pattern.
		{
			FRegexPattern Pattern = FRegexPattern("^([a-zA-Z][a-zA-Z0-9]*)$");
			FRegexMatcher Matcher(Pattern, Var);
			if (Matcher.FindNext())
			{
				FVarDescription Desc;
				Desc.bIsValid = true;
				Desc.VarName = Matcher.GetCaptureGroup(1);
				Desc.VarType = EContainerType::None;
				Desc.ArrayIndex = -1;
				Desc.MapKey = "";
				VarDescs->Add(Desc);
				continue;
			}
		}

		FVarDescription Desc;
		Desc.bIsValid = false;
		VarDescs->Add(Desc);
	}
}