/*!
 * AccessVariableByName
 *
 * Copyright (c) 2022 Colory Games
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "Common.h"

#include "UObject/UnrealType.h"
#include "Internationalization/Regex.h"

const FName ExecThenPinName(TEXT("ExecThen"));
const FName VarNamePinName(TEXT("VarName"));
const FName SuccessPinName(TEXT("Success"));
const FName ResultPinNamePrefix(TEXT("Result_"));
const FName NewValuePinNamePrefix(TEXT("NewValue_"));

const FString ExecThenPinFriendlyName(TEXT(" "));
const FString TargetPinFriendlyName(TEXT("Target"));
const FString VarNamePinFriendlyName(TEXT("Var Name"));
const FString SuccessPinFriendlyName(TEXT("Success"));

FProperty* GetTerminalProperty(const TArray<FVarDescription>& VarDescs, int32 VarDepth, UScriptStruct* OuterClass);

FEdGraphPinType CreateDefaultPinType()
{
	FEdGraphPinType PinType;
	PinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;

	return PinType;
}

UClass* GetClassFromNode(const UEdGraphNode* Node)
{
	UClass* Class = nullptr;

	UEdGraph* Graph = Node->GetGraph();
	UObject* GraphOwner = Graph->GetOutermostObject();
	UBlueprint* Blueprint = Cast<UBlueprint>(GraphOwner);
	if (Blueprint != nullptr)
	{
		Class = Blueprint->GeneratedClass;
	}

	return Class;
}

FProperty* GetScriptStructProperty(UScriptStruct* ScriptStruct, FString VarName)
{
	FProperty* Property = nullptr;

	if (ScriptStruct->IsNative())
	{
		Property = ScriptStruct->FindPropertyByName(*VarName);
	}
	else
	{
		FField* Field = ScriptStruct->ChildProperties;
		while (Field)
		{
			FString FieldName = Field->GetName();
			int32 Index = FieldName.Len();
			for (int Iter = 0; Iter < 2; ++Iter)
			{
				Index = FieldName.Find(FString("_"), ESearchCase::CaseSensitive, ESearchDir::FromEnd, Index);
			}
			FString PropertyName = FieldName.Left(Index);

			if (PropertyName == VarName)
			{
				Property = ScriptStruct->FindPropertyByName(*FieldName);
				break;
			}

			Field = Field->Next;
		}
	}

	return Property;
}

FProperty* GetTerminalPropertyInternal(const TArray<FVarDescription>& VarDescs, int32 VarDepth, FProperty* Property)
{
	const FVarDescription& Desc = VarDescs[VarDepth];

	if (Desc.ArrayAccessType == EArrayAccessType::ArrayAccessType_None)
	{
		if (VarDescs.Num() == VarDepth + 1)
		{
			return Property;
		}

		if (Property->IsA<FStructProperty>())
		{
			FStructProperty* StructProperty = CastChecked<FStructProperty>(Property);
			UScriptStruct* ScriptStruct = StructProperty->Struct;

			return GetTerminalProperty(VarDescs, VarDepth + 1, ScriptStruct);
		}
		else if (Property->IsA<FObjectProperty>())
		{
			FObjectProperty* ObjectProperty = CastChecked<FObjectProperty>(Property);
			UClass* Class = ObjectProperty->PropertyClass;

			return GetTerminalProperty(VarDescs, VarDepth + 1, Class);
		}

		return nullptr;
	}
	else if (Desc.ArrayAccessType == EArrayAccessType::ArrayAccessType_Integer)
	{
		if (Property->IsA<FArrayProperty>())
		{
			FArrayProperty* ArrayProperty = CastChecked<FArrayProperty>(Property);

			if (VarDescs.Num() == VarDepth + 1)
			{
				return ArrayProperty->Inner;
			}

			FProperty* InnerProperty = ArrayProperty->Inner;
			if (InnerProperty->IsA<FStructProperty>())
			{
				FStructProperty* StructProperty = CastChecked<FStructProperty>(InnerProperty);
				UScriptStruct* ScriptStruct = StructProperty->Struct;

				return GetTerminalProperty(VarDescs, VarDepth + 1, ScriptStruct);
			}
			else if (InnerProperty->IsA<FObjectProperty>())
			{
				FObjectProperty* ObjectProperty = CastChecked<FObjectProperty>(InnerProperty);
				UClass* Class = ObjectProperty->PropertyClass;

				return GetTerminalProperty(VarDescs, VarDepth + 1, Class);
			}

			return nullptr;
		}
		else if (Property->IsA<FMapProperty>())
		{
			FMapProperty* MapProperty = CastChecked<FMapProperty>(Property);

			if (VarDescs.Num() == VarDepth + 1)
			{
				return MapProperty->ValueProp;
			}

			FProperty* ValueProperty = MapProperty->ValueProp;
			if (ValueProperty->IsA<FStructProperty>())
			{
				FStructProperty* StructProperty = CastChecked<FStructProperty>(ValueProperty);
				UScriptStruct* ScriptStruct = StructProperty->Struct;

				return GetTerminalProperty(VarDescs, VarDepth + 1, ScriptStruct);
			}
			else if (ValueProperty->IsA<FObjectProperty>())
			{
				FObjectProperty* ObjectProperty = CastChecked<FObjectProperty>(ValueProperty);
				UClass* Class = ObjectProperty->PropertyClass;

				return GetTerminalProperty(VarDescs, VarDepth + 1, Class);
			}
		}

		return nullptr;
	}
	else if (Desc.ArrayAccessType == EArrayAccessType::ArrayAccessType_String)
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
		if (ValueProperty->IsA<FStructProperty>())
		{
			FStructProperty* StructProperty = CastChecked<FStructProperty>(ValueProperty);
			UScriptStruct* ScriptStruct = StructProperty->Struct;

			return GetTerminalProperty(VarDescs, VarDepth + 1, ScriptStruct);
		}
		else if (ValueProperty->IsA<FObjectProperty>())
		{
			FObjectProperty* ObjectProperty = CastChecked<FObjectProperty>(ValueProperty);
			UClass* Class = ObjectProperty->PropertyClass;

			return GetTerminalProperty(VarDescs, VarDepth + 1, Class);
		}

		return nullptr;
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

	FProperty* Property = GetScriptStructProperty(OuterClass, Desc.VarName);
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
		// String pattern.
		{
			FRegexPattern Pattern = FRegexPattern("^([a-zA-Z_][a-zA-Z0-9_]*)\\[\"(\\S+)\"\\]$");
			FRegexMatcher Matcher(Pattern, Var);
			if (Matcher.FindNext())
			{
				FVarDescription Desc;
				Desc.bIsValid = true;
				Desc.VarName = Matcher.GetCaptureGroup(1);
				Desc.ArrayAccessType = EArrayAccessType::ArrayAccessType_String;
				Desc.ArrayAccessValue.Integer = -1;
				Desc.ArrayAccessValue.String = Matcher.GetCaptureGroup(2);
				VarDescs->Add(Desc);
				continue;
			}
		}

		// Integer pattern.
		{
			FRegexPattern Pattern = FRegexPattern("^([a-zA-Z_][a-zA-Z0-9_]*)\\[([0-9]+)\\]$");
			FRegexMatcher Matcher(Pattern, Var);
			if (Matcher.FindNext())
			{
				FVarDescription Desc;
				Desc.bIsValid = true;
				Desc.VarName = Matcher.GetCaptureGroup(1);
				Desc.ArrayAccessType = EArrayAccessType::ArrayAccessType_Integer;
				Desc.ArrayAccessValue.Integer = FCString::Atoi(*Matcher.GetCaptureGroup(2));
				Desc.ArrayAccessValue.String = "";
				VarDescs->Add(Desc);
				continue;
			}
		}

		// None pattern.
		{
			FRegexPattern Pattern = FRegexPattern("^([a-zA-Z_][a-zA-Z0-9_]*)$");
			FRegexMatcher Matcher(Pattern, Var);
			if (Matcher.FindNext())
			{
				FVarDescription Desc;
				Desc.bIsValid = true;
				Desc.VarName = Matcher.GetCaptureGroup(1);
				Desc.ArrayAccessType = EArrayAccessType::ArrayAccessType_None;
				Desc.ArrayAccessValue.Integer = -1;
				Desc.ArrayAccessValue.String = "";
				VarDescs->Add(Desc);
				continue;
			}
		}

		FVarDescription Desc;
		Desc.bIsValid = false;
		VarDescs->Add(Desc);
	}
}
