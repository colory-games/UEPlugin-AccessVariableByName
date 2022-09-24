/*!
 * AccessVariableByName
 *
 * Copyright (c) 2022 Colory Games
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "K2Node_DynamicSetVariableByName.h"

#include "BlueprintNodeSpawner.h"
#include "Common.h"
#include "EdGraphUtilities.h"
#include "EditorCategoryUtils.h"
#include "GraphEditorSettings.h"
#include "K2Node_CallFunction.h"
#include "KismetCompiler.h"
#include "VariableSetterFunctionLibrary.h"
#include "Internationalization/Regex.h"

#define LOCTEXT_NAMESPACE "K2Node"

class FKCHandler_DynamicSetVariableByName : public FNodeHandlingFunctor
{
public:
	FKCHandler_DynamicSetVariableByName(FKismetCompilerContext& InCompilerContext) : FNodeHandlingFunctor(InCompilerContext)
	{
	}

	virtual void RegisterNets(FKismetFunctionContext& Context, UEdGraphNode* InNode) override
	{
		UK2Node_DynamicSetVariableByNameNode* DynamicSetVariableByNameNode = CastChecked<UK2Node_DynamicSetVariableByNameNode>(InNode);
		check(DynamicSetVariableByNameNode);

		for (auto Pin : DynamicSetVariableByNameNode->Pins)
		{
			if (Pin && Pin->Direction == EGPD_Input)
			{
				if (Pin->LinkedTo.Num() == 0)
				{
					if (Pin == DynamicSetVariableByNameNode->GetVarNamePin() ||
						Pin == DynamicSetVariableByNameNode->GetAllNewValuePins()[0])
					{
						RegisterLiteral(Context, Pin);
					}
				}
			}

			if (Pin && Pin->Direction == EGPD_Output)
			{
				if (Pin == DynamicSetVariableByNameNode->GetSuccessPin() ||
					Pin == DynamicSetVariableByNameNode->GetAllResultPins()[0])
				{
					FBPTerminal* Term = Context.CreateLocalTerminalFromPinAutoChooseScope(Pin, Pin->PinName.ToString());
					Context.NetMap.Add(Pin, Term);
				}
			}
		}
	}

	virtual void Compile(FKismetFunctionContext& Context, UEdGraphNode* Node) override
	{
		UK2Node_DynamicSetVariableByNameNode* DynamicSetVariableByNameNode = CastChecked<UK2Node_DynamicSetVariableByNameNode>(Node);

		// TODO: Error handling

		//UEdGraphPin* ExecTriggeringPin = DynamicSetVariableByNameNode->GetExecTriggeringPin();

		UEdGraphPin* FunctionPin = DynamicSetVariableByNameNode->GetFunctionPin();
		FBPTerminal* FunctionContext = Context.NetMap.FindRef(FunctionPin);
		UClass* FunctionClass = Cast<UClass>(FunctionPin->PinType.PinSubCategoryObject.Get());
		UFunction* FunctionPtr = FindUField<UFunction>(FunctionClass, FunctionPin->PinName);
		check(FunctionPtr);

		UEdGraphPin* ExecThenPin = DynamicSetVariableByNameNode->GetExecThenPin();

		UEdGraphPin* TargetPin = DynamicSetVariableByNameNode->GetTargetPin();
		UEdGraphPin* TargetNet = FEdGraphUtilities::GetNetFromPin(TargetPin);
		FBPTerminal* TargetTerm = Context.NetMap.FindRef(TargetNet);

		UEdGraphPin* VarNamePin = DynamicSetVariableByNameNode->GetVarNamePin();
		UEdGraphPin* VarNameNet = FEdGraphUtilities::GetNetFromPin(VarNamePin);
		FBPTerminal* VarNameTerm = Context.NetMap.FindRef(VarNameNet);

		UEdGraphPin* NewValuePin = DynamicSetVariableByNameNode->GetAllNewValuePins()[0];
		UEdGraphPin* NewValueNet = FEdGraphUtilities::GetNetFromPin(NewValuePin);
		FBPTerminal* NewValueTerm = Context.NetMap.FindRef(NewValueNet);

		UEdGraphPin* SuccessPin = DynamicSetVariableByNameNode->GetSuccessPin();
		UEdGraphPin* SuccessNet = FEdGraphUtilities::GetNetFromPin(SuccessPin);
		FBPTerminal* SuccessTerm = Context.NetMap.FindRef(SuccessNet);

		UEdGraphPin* ResultPin = DynamicSetVariableByNameNode->GetAllResultPins()[0];
		UEdGraphPin* ResultNet = FEdGraphUtilities::GetNetFromPin(ResultPin);
		FBPTerminal* ResultTerm = Context.NetMap.FindRef(ResultNet);

		FBlueprintCompiledStatement& CallFuncStatement = Context.AppendStatementForNode(DynamicSetVariableByNameNode);
		CallFuncStatement.Type = KCST_CallFunction;
		CallFuncStatement.FunctionToCall = FunctionPtr;
		CallFuncStatement.FunctionContext = FunctionContext;
		CallFuncStatement.bIsParentContext = false;
		CallFuncStatement.RHS.Add(TargetTerm);
		CallFuncStatement.RHS.Add(VarNameTerm);
		CallFuncStatement.RHS.Add(SuccessTerm);
		CallFuncStatement.RHS.Add(ResultTerm);
		CallFuncStatement.RHS.Add(NewValueTerm);		// Argument order is different from pin index.

		GenerateSimpleThenGoto(Context, *DynamicSetVariableByNameNode, ExecThenPin);
	}
};

UK2Node_DynamicSetVariableByNameNode::UK2Node_DynamicSetVariableByNameNode(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	InternalCallFuncClass = UVariableSetterFunctionLibarary::StaticClass();
	InternalCallFuncName = FName("SetNestedVariableByName");
}

FText UK2Node_DynamicSetVariableByNameNode::GetMenuCategory() const
{
	return FEditorCategoryUtils::GetCommonCategory(FCommonEditorCategory::Variables);
}

void UK2Node_DynamicSetVariableByNameNode::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	UClass* ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);

		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

FNodeHandlingFunctor* UK2Node_DynamicSetVariableByNameNode::CreateNodeHandler(FKismetCompilerContext& CompilerContext) const
{
	return new FKCHandler_DynamicSetVariableByName(CompilerContext);
}

void UK2Node_DynamicSetVariableByNameNode::AllocateDefaultPins()
{
	// Pin structure
	//   N: Number of case pin pair
	// -----
	// 0: Internal function (Hidden, Object)
	// 1: Execution Triggering (In, Exec)
	// 2: Execution Then (Out, Exec)
	// 3: Target (In, Object Reference)
	// 4: Var Name (In, FName)
	// 5: Success (Out, Boolean)
	// 6+(N*2): New Value (In, *)
	// 6+(N*2)+1: Result (Out, *)

	CreateFunctionPin();
	CreateExecTriggeringPin();
	CreateExecThenPin();
	CreateTargetPin();
	CreateVarNamePin();
	CreateSuccessPin();
	
	FEdGraphPinType InitialPinType;
	InitialPinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
	CreateNewValuePin(InitialPinType, 0);
	CreateResultPin(InitialPinType, 0);

	Super::AllocateDefaultPins();
}

void UK2Node_DynamicSetVariableByNameNode::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
	FEdGraphPinType NewValuePinType;
	FEdGraphPinType ResultPinType;
	for (auto& Pin : OldPins)
	{
		if (IsNewValuePin(Pin))
		{
			NewValuePinType = Pin->PinType;
		}
		if (IsResultPin(Pin))
		{
			ResultPinType = Pin->PinType;
			break;
		}
	}

	CreateFunctionPin();
	CreateExecTriggeringPin();
	CreateExecThenPin();
	CreateTargetPin();
	CreateVarNamePin();
	CreateSuccessPin();
	CreateNewValuePin(NewValuePinType, 0);
	CreateResultPin(ResultPinType, 0);

	RestoreSplitPins(OldPins);
}

FText UK2Node_DynamicSetVariableByNameNode::GetTooltipText() const
{
	return LOCTEXT("DynamicSetVariableByNameNode_Tooltip", "Dynamic Set Variable by Name");
}

FLinearColor UK2Node_DynamicSetVariableByNameNode::GetNodeTitleColor() const
{
	return GetDefault<UGraphEditorSettings>()->DefaultPinTypeColor;
}

FText UK2Node_DynamicSetVariableByNameNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("Dynamic Set Variable by Name", "Dynamic Set Variable by Name");
}

FSlateIcon UK2Node_DynamicSetVariableByNameNode::GetIconAndTint(FLinearColor& OutColor) const
{
	static FSlateIcon Icon("EditorStyle", "GraphEditor.Switch_16x");
	return Icon;
}

void UK2Node_DynamicSetVariableByNameNode::CreateFunctionPin()
{
	FCreatePinParams Params;
	Params.Index = 0;

	UClass* FunctionClass = UVariableSetterFunctionLibarary::StaticClass();
	UFunction* FunctionPtr = FunctionClass->FindFunctionByName(InternalCallFuncName);

	UEdGraphPin* FunctionPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Object, InternalCallFuncClass, InternalCallFuncName, Params);
	FunctionPin->bDefaultValueIsReadOnly = true;
	FunctionPin->bNotConnectable = true;
	FunctionPin->bHidden = true;

	if (FunctionPtr != nullptr && FunctionPtr->HasAllFunctionFlags(FUNC_Static))
	{
		UBlueprint* Blueprint = GetBlueprint();
		if (Blueprint != nullptr)
		{
			UClass* FunctionOwnerClass = FunctionPtr->GetOuterUClass();
			if (!Blueprint->SkeletonGeneratedClass->IsChildOf(FunctionOwnerClass))
			{
				FunctionPin->DefaultObject = FunctionOwnerClass->GetDefaultObject();
			}
		}
	}
}

void UK2Node_DynamicSetVariableByNameNode::CreateExecTriggeringPin()
{
	FCreatePinParams Params;
	Params.Index = 1;
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute, Params);
}

void UK2Node_DynamicSetVariableByNameNode::CreateExecThenPin()
{
	FCreatePinParams Params;
	Params.Index = 2;
	UEdGraphPin* DefaultExecPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, ExecThenPinName, Params);
	DefaultExecPin->PinFriendlyName = FText::AsCultureInvariant(ExecThenPinFriendlyName);
}

void UK2Node_DynamicSetVariableByNameNode::CreateTargetPin()
{
	FCreatePinParams Params;
	Params.Index = 3;
	UEdGraphPin* Pin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Object, UObject::StaticClass(), TargetPinName, Params);
	Pin->PinFriendlyName = FText::AsCultureInvariant(TargetPinFriendlyName);
}

void UK2Node_DynamicSetVariableByNameNode::CreateVarNamePin()
{
	FCreatePinParams Params;
	Params.Index = 4;
	UEdGraphPin* Pin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Name, VarNamePinName, Params);
	Pin->PinFriendlyName = FText::AsCultureInvariant(VarNamePinFriendlyName);
}

void UK2Node_DynamicSetVariableByNameNode::CreateSuccessPin()
{
	FCreatePinParams Params;
	Params.Index = 5;
	UEdGraphPin* Pin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Boolean, SuccessPinName, Params);
	Pin->PinFriendlyName = FText::AsCultureInvariant(SuccessPinFriendlyName);
}

void UK2Node_DynamicSetVariableByNameNode::CreateNewValuePin(const FEdGraphPinType& PinType, int32 Index)
{
	FName NewValuePinName = FName(FString::Format(TEXT("{0}"), {*NewValuePinNamePrefix.ToString()}));
	FString NewValuePinFriendlyName = "New Value";

	FCreatePinParams Params;
	Params.Index = 6 + Index * 2;
	UEdGraphPin* Pin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Boolean, NewValuePinName, Params);
	Pin->PinFriendlyName = FText::AsCultureInvariant(NewValuePinFriendlyName);
	Pin->PinType = PinType;
}

void UK2Node_DynamicSetVariableByNameNode::CreateResultPin(const FEdGraphPinType& PinType, int32 Index)
{
	FName ResultPinName = FName(FString::Format(TEXT("{0}"), {*ResultPinNamePrefix.ToString()}));
	FString ResultPinFriendlyName = "Result";

	FCreatePinParams Params;
	Params.Index = 6 + Index * 2 + 1;
	UEdGraphPin* Pin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Boolean, ResultPinName, Params);
	Pin->PinFriendlyName = FText::AsCultureInvariant(ResultPinFriendlyName);
	Pin->PinType = PinType;
}

bool UK2Node_DynamicSetVariableByNameNode::IsNewValuePin(const UEdGraphPin* Pin) const
{
	FString PinName = Pin->GetFName().ToString();

	FRegexPattern Pattern = FRegexPattern(FString::Format(TEXT("^{0}[0-9]*$"), {*NewValuePinNamePrefix.ToString()}));
	FRegexMatcher Matcher(Pattern, PinName);
	if (Matcher.FindNext())
	{
		return true;
	}

	return false;
}

bool UK2Node_DynamicSetVariableByNameNode::IsResultPin(const UEdGraphPin* Pin) const
{
	FString PinName = Pin->GetFName().ToString();

	FRegexPattern Pattern = FRegexPattern(FString::Format(TEXT("^{0}[0-9]*$"), {*ResultPinNamePrefix.ToString()}));
	FRegexMatcher Matcher(Pattern, PinName);
	if (Matcher.FindNext())
	{
		return true;
	}

	return false;
}

void UK2Node_DynamicSetVariableByNameNode::RecreateVariantPins(const FEdGraphPinType& PinType)
{
	Modify();

	TArray<UEdGraphPin*> UnusedPins = MoveTemp(Pins);
	for (int32 Index = 0; Index < UnusedPins.Num(); ++Index)
	{
		UEdGraphPin* OldPin = UnusedPins[Index];
		if (!IsNewValuePin(OldPin) && !IsResultPin(OldPin))
		{
			UnusedPins.RemoveAt(Index--, 1, false);
			Pins.Add(OldPin);
		}
	}

	CreateResultPin(PinType, 0);
	CreateNewValuePin(PinType, 0);

	RestoreSplitPins(UnusedPins);
	RewireOldPinsToNewPins(UnusedPins, Pins, nullptr);

	UEdGraph* Graph = GetGraph();
	Graph->NotifyGraphChanged();
}

UEdGraphPin* UK2Node_DynamicSetVariableByNameNode::GetFunctionPin() const
{
	return FindPinChecked(InternalCallFuncName);
}

UEdGraphPin* UK2Node_DynamicSetVariableByNameNode::GetExecThenPin() const
{
	return FindPinChecked(ExecThenPinName);
}

UEdGraphPin* UK2Node_DynamicSetVariableByNameNode::GetTargetPin() const
{
	return FindPin(TargetPinName);
}

UEdGraphPin* UK2Node_DynamicSetVariableByNameNode::GetVarNamePin() const
{
	return FindPinChecked(VarNamePinName);
}

UEdGraphPin* UK2Node_DynamicSetVariableByNameNode::GetSuccessPin() const
{
	return FindPinChecked(SuccessPinName);
}

TArray<UEdGraphPin*> UK2Node_DynamicSetVariableByNameNode::GetAllNewValuePins() const
{
	TArray<UEdGraphPin*> NewValuePins;

	for (auto& Pin : Pins)
	{
		if (IsNewValuePin(Pin))
		{
			NewValuePins.Add(Pin);
		}
	}

	return NewValuePins;
}

TArray<UEdGraphPin*> UK2Node_DynamicSetVariableByNameNode::GetAllResultPins() const
{
	TArray<UEdGraphPin*> ResultPins;

	for (auto& Pin : Pins)
	{
		if (IsResultPin(Pin))
		{
			ResultPins.Add(Pin);
		}
	}

	return ResultPins;
}

void UK2Node_DynamicSetVariableByNameNode::ChangeVariantPinType(const FEdGraphPinType& PinType)
{
	RecreateVariantPins(PinType);
}

FEdGraphPinType UK2Node_DynamicSetVariableByNameNode::GetVariantPinType() const
{
	TArray<UEdGraphPin*> ResultPins = GetAllResultPins();
	if (ResultPins.Num() == 0)
	{
		return CreateDefaultPinType();
	}

	return ResultPins[0]->PinType;
}

#undef LOCTEXT_NAMESPACE
