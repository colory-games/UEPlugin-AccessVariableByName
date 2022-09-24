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
const FName ResultPinNamePrefix(TEXT("Result_"));
const FName NewValuePinNamePrefix(TEXT("NewValue_"));

const FString ExecThenPinFriendlyName(TEXT(" "));
const FString TargetPinFriendlyName(TEXT("Target"));
const FString VarNamePinFriendlyName(TEXT("Var Name"));
const FString SuccessPinFriendlyName(TEXT("Success"));

FProperty* GetTerminalProperty(const TArray<FVarDescription>& VarDescs, int32 VarDepth, UScriptStruct* OuterClass);

FProperty* GetTerminalPropertyInternal(const TArray<FVarDescription>& VarDescs, int32 VarDepth, FProperty* Property)
{
	const FVarDescription& Desc = VarDescs[VarDepth];

	if (Desc.VarType == EContainerType::None)
	{
		if (VarDescs.Num() == VarDepth + 1)
		{
			return Property;
		}

		if (!Property->IsA<FStructProperty>())
		{
			return nullptr;
		}

		FStructProperty* StructProperty = CastChecked<FStructProperty>(Property);
		UScriptStruct* ScriptStruct = StructProperty->Struct;

		return GetTerminalProperty(VarDescs, VarDepth + 1, ScriptStruct);
	}
	else if (Desc.VarType == EContainerType::Array)
	{
		if (!Property->IsA<FArrayProperty>())
		{
			return nullptr;
		}
		FArrayProperty* ArrayProperty = CastChecked<FArrayProperty>(Property);

		if (VarDescs.Num() == VarDepth + 1)
		{
			return ArrayProperty->Inner;
		}

		FProperty* InnerProperty = ArrayProperty->Inner;
		if (!InnerProperty->IsA<FStructProperty>())
		{
			return nullptr;
		}
		FStructProperty* StructProperty = CastChecked<FStructProperty>(InnerProperty);
		UScriptStruct* ScriptStruct = StructProperty->Struct;

		return GetTerminalProperty(VarDescs, VarDepth + 1, ScriptStruct);
	}
	else if (Desc.VarType == EContainerType::Map)
	{
		if (!Property->IsA<FMapProperty>())
		{
			return nullptr;
		}
		FMapProperty* MapProperty = CastChecked<FMapProperty>(Property);

		if (VarDescs.Num() == VarDepth + 1)
		{
			return MapProperty->ValueProp;
		}

		FProperty* ValueProperty = MapProperty->ValueProp;
		if (!ValueProperty->IsA<FStructProperty>())
		{
			return nullptr;
		}
		FStructProperty* StructProperty = CastChecked<FStructProperty>(ValueProperty);
		UScriptStruct* ScriptStruct = StructProperty->Struct;

		return GetTerminalProperty(VarDescs, VarDepth + 1, ScriptStruct);
	}

	return nullptr;
}

FProperty* GetTerminalProperty(const TArray<FVarDescription>& VarDescs, int32 VarDepth, UScriptStruct* OuterClass)
{
	const FVarDescription& Desc = VarDescs[VarDepth];

	if (!Desc.bIsValid)
	{
		return nullptr;
	}

	FProperty* Property = FindFProperty<FProperty>(OuterClass, *Desc.VarName);
	if (Property == nullptr)
	{
		return nullptr;
	}

	return GetTerminalPropertyInternal(VarDescs, VarDepth, Property);
}

FProperty* GetTerminalProperty(const TArray<FVarDescription>& VarDescs, int32 VarDepth, UClass* OuterClass)
{
	const FVarDescription& Desc = VarDescs[VarDepth];

	if (!Desc.bIsValid)
	{
		return nullptr;
	}

	FProperty* Property = FindFProperty<FProperty>(OuterClass, *Desc.VarName);
	if (Property == nullptr)
	{
		return nullptr;
	}

	return GetTerminalPropertyInternal(VarDescs, VarDepth, Property);
}

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