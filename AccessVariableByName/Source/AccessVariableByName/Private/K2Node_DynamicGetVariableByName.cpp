/*!
 * AccessVariableByName
 *
 * Copyright (c) 2022 Colory Games
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "K2Node_DynamicGetVariableByName.h"

#include "BlueprintNodeSpawner.h"
#include "Common.h"
#include "EdGraphUtilities.h"
#include "EditorCategoryUtils.h"
#include "GraphEditorSettings.h"
#include "K2Node_CallFunction.h"
#include "KismetCompiler.h"
#include "VariableGetterFunctionLibrary.h"
#include "Internationalization/Regex.h"

#define LOCTEXT_NAMESPACE "K2Node"

class FKCHandler_DynamicGetVariableByName : public FNodeHandlingFunctor
{
public:
	FKCHandler_DynamicGetVariableByName(FKismetCompilerContext& InCompilerContext) : FNodeHandlingFunctor(InCompilerContext)
	{
	}

	virtual void RegisterNets(FKismetFunctionContext& Context, UEdGraphNode* Node) override
	{
		UK2Node_DynamicGetVariableByNameNode* DynamicGetVariableByNameNode = CastChecked<UK2Node_DynamicGetVariableByNameNode>(Node);

		FNodeHandlingFunctor::RegisterNets(Context, Node);

		for (auto Pin : DynamicGetVariableByNameNode->Pins)
		{
			if (Pin && Pin->Direction == EGPD_Input)
			{
				if (Pin->LinkedTo.Num() == 0)
				{
					if (Pin == DynamicGetVariableByNameNode->GetVarNamePin())
					{
						RegisterLiteral(Context, Pin);
					}
				}
			}

			if (Pin && Pin->Direction == EGPD_Output)
			{
				if (Pin == DynamicGetVariableByNameNode->GetSuccessPin() ||
					Pin == DynamicGetVariableByNameNode->GetAllResultPins()[0])
				{
					FBPTerminal* Term = Context.CreateLocalTerminalFromPinAutoChooseScope(Pin, Pin->PinName.ToString());
					Context.NetMap.Add(Pin, Term);
				}
			}
		}
	}

	virtual void Compile(FKismetFunctionContext& Context, UEdGraphNode* Node) override
	{
		UK2Node_DynamicGetVariableByNameNode* DynamicGetVariableByNameNode = CastChecked<UK2Node_DynamicGetVariableByNameNode>(Node);

		UEdGraphPin* FunctionPin = DynamicGetVariableByNameNode->GetFunctionPin();
		FBPTerminal* FunctionContext = Context.NetMap.FindRef(FunctionPin);
		UClass* FunctionClass = Cast<UClass>(FunctionPin->PinType.PinSubCategoryObject.Get());
		UFunction* FunctionPtr = FindUField<UFunction>(FunctionClass, FunctionPin->PinName);
		check(FunctionPtr);

		UEdGraphPin* ExecThenPin = DynamicGetVariableByNameNode->GetExecThenPin();

		UEdGraphPin* TargetPin = DynamicGetVariableByNameNode->GetTargetPin();
		UEdGraphPin* TargetNet = FEdGraphUtilities::GetNetFromPin(TargetPin);
		FBPTerminal* TargetTerm = Context.NetMap.FindRef(TargetNet);

		UEdGraphPin* VarNamePin = DynamicGetVariableByNameNode->GetVarNamePin();
		UEdGraphPin* VarNameNet = FEdGraphUtilities::GetNetFromPin(VarNamePin);
		FBPTerminal* VarNameTerm = Context.NetMap.FindRef(VarNameNet);

		UEdGraphPin* SuccessPin = DynamicGetVariableByNameNode->GetSuccessPin();
		UEdGraphPin* SuccessNet = FEdGraphUtilities::GetNetFromPin(SuccessPin);
		FBPTerminal* SuccessTerm = Context.NetMap.FindRef(SuccessNet);

		UEdGraphPin* ResultPin = DynamicGetVariableByNameNode->GetAllResultPins()[0];
		UEdGraphPin* ResultNet = FEdGraphUtilities::GetNetFromPin(ResultPin);
		FBPTerminal* ResultTerm = Context.NetMap.FindRef(ResultNet);

		FBlueprintCompiledStatement& CallFuncStatement = Context.AppendStatementForNode(DynamicGetVariableByNameNode);
		CallFuncStatement.Type = KCST_CallFunction;
		CallFuncStatement.FunctionToCall = FunctionPtr;
		CallFuncStatement.FunctionContext = FunctionContext;
		CallFuncStatement.bIsParentContext = false;
		CallFuncStatement.RHS.Add(TargetTerm);
		CallFuncStatement.RHS.Add(VarNameTerm);
		CallFuncStatement.RHS.Add(SuccessTerm);
		CallFuncStatement.RHS.Add(ResultTerm);

		GenerateSimpleThenGoto(Context, *DynamicGetVariableByNameNode, ExecThenPin);
	}
};

UK2Node_DynamicGetVariableByNameNode::UK2Node_DynamicGetVariableByNameNode(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	InternalCallFuncClass = UVariableGetterFunctionLibarary::StaticClass();
	InternalCallFuncName = FName("GetNestedVariableByName");
}

FText UK2Node_DynamicGetVariableByNameNode::GetMenuCategory() const
{
	return FEditorCategoryUtils::GetCommonCategory(FCommonEditorCategory::Variables);
}

void UK2Node_DynamicGetVariableByNameNode::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	UClass* ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);

		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

FNodeHandlingFunctor* UK2Node_DynamicGetVariableByNameNode::CreateNodeHandler(FKismetCompilerContext& CompilerContext) const
{
	return new FKCHandler_DynamicGetVariableByName(CompilerContext);
}

void UK2Node_DynamicGetVariableByNameNode::AllocateDefaultPins()
{
	// Pin structure
	//   N: Number of result pin
	// -----
	// 0: Internal function (Hidden, Object)
	// 1: Execution Triggering (In, Exec)
	// 2: Execution Then (Out, Exec)
	// 3: Target (In, Object Reference)
	// 4: Var Name (In, FName)
	// 5: Success (Out, Boolean)
	// 6-: Result (Out, *)

	CreateFunctionPin();
	CreateExecTriggeringPin();
	CreateExecThenPin();
	CreateTargetPin();
	CreateVarNamePin();
	CreateSuccessPin();
	CreateResultPin(CreateDefaultPinType(), 0);

	Super::AllocateDefaultPins();
}

void UK2Node_DynamicGetVariableByNameNode::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
	FEdGraphPinType ResultPinType;
	for (auto& Pin : OldPins)
	{
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
	CreateResultPin(ResultPinType, 0);

	RestoreSplitPins(OldPins);
}

FText UK2Node_DynamicGetVariableByNameNode::GetTooltipText() const
{
	return LOCTEXT("DynamicGetVariableByNameNode_Tooltip", "Get Variable by Name (Dynamic)");
}

FLinearColor UK2Node_DynamicGetVariableByNameNode::GetNodeTitleColor() const
{
	return GetDefault<UGraphEditorSettings>()->FunctionCallNodeTitleColor;
}

FText UK2Node_DynamicGetVariableByNameNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("Get Variable by Name (Dynamic)", "Get Variable by Name (Dynamic)");
}

FSlateIcon UK2Node_DynamicGetVariableByNameNode::GetIconAndTint(FLinearColor& OutColor) const
{
	static FSlateIcon Icon("EditorStyle", "GraphEditor.Function_16x");

	OutColor = GetDefault<UGraphEditorSettings>()->FunctionCallNodeTitleColor;

	return Icon;
}

void UK2Node_DynamicGetVariableByNameNode::CreateFunctionPin()
{
	FCreatePinParams Params;
	Params.Index = 0;

	UClass* FunctionClass = UVariableGetterFunctionLibarary::StaticClass();
	UFunction* FunctionPtr = FunctionClass->FindFunctionByName(InternalCallFuncName);
	check(FunctionPtr);

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

void UK2Node_DynamicGetVariableByNameNode::CreateExecTriggeringPin()
{
	FCreatePinParams Params;
	Params.Index = 1;
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute, Params);
}

void UK2Node_DynamicGetVariableByNameNode::CreateExecThenPin()
{
	FCreatePinParams Params;
	Params.Index = 2;
	UEdGraphPin* DefaultExecPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, ExecThenPinName, Params);
	DefaultExecPin->PinFriendlyName = FText::AsCultureInvariant(ExecThenPinFriendlyName);
}

void UK2Node_DynamicGetVariableByNameNode::CreateTargetPin()
{
	FCreatePinParams Params;
	Params.Index = 3;
	UEdGraphPin* Pin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Object, UObject::StaticClass(), UEdGraphSchema_K2::PN_Self, Params);
	Pin->PinFriendlyName = FText::AsCultureInvariant(TargetPinFriendlyName);
}

void UK2Node_DynamicGetVariableByNameNode::CreateVarNamePin()
{
	FCreatePinParams Params;
	Params.Index = 4;
	UEdGraphPin* Pin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Name, VarNamePinName, Params);
	Pin->PinFriendlyName = FText::AsCultureInvariant(VarNamePinFriendlyName);
}

void UK2Node_DynamicGetVariableByNameNode::CreateSuccessPin()
{
	FCreatePinParams Params;
	Params.Index = 5;
	UEdGraphPin* Pin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Boolean, SuccessPinName, Params);
	Pin->PinFriendlyName = FText::AsCultureInvariant(SuccessPinFriendlyName);
}

void UK2Node_DynamicGetVariableByNameNode::CreateResultPin(const FEdGraphPinType& PinType, int32 Index)
{
	FName ResultPinName = FName(FString::Format(TEXT("{0}"), {*ResultPinNamePrefix.ToString()}));
	FString ResultPinFriendlyName = "Result";

	FCreatePinParams Params;
	Params.Index = 6 + Index;
	UEdGraphPin* Pin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Boolean, ResultPinName, Params);
	Pin->PinFriendlyName = FText::AsCultureInvariant(ResultPinFriendlyName);
	Pin->PinType = PinType;
}

bool UK2Node_DynamicGetVariableByNameNode::IsResultPin(const UEdGraphPin* Pin) const
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

void UK2Node_DynamicGetVariableByNameNode::RecreateVariantPins(const FEdGraphPinType& PinType)
{
	Modify();

	TArray<UEdGraphPin*> UnusedPins = MoveTemp(Pins);
	for (int32 Index = 0; Index < UnusedPins.Num(); ++Index)
	{
		UEdGraphPin* OldPin = UnusedPins[Index];
		if (!IsResultPin(OldPin))
		{
			UnusedPins.RemoveAt(Index--, 1, false);
			Pins.Add(OldPin);
		}
	}

	CreateResultPin(PinType, 0);

	RestoreSplitPins(UnusedPins);
	RewireOldPinsToNewPins(UnusedPins, Pins, nullptr);

	UEdGraph* Graph = GetGraph();
	Graph->NotifyGraphChanged();
}

UEdGraphPin* UK2Node_DynamicGetVariableByNameNode::GetFunctionPin() const
{
	return FindPinChecked(InternalCallFuncName);
}

UEdGraphPin* UK2Node_DynamicGetVariableByNameNode::GetExecThenPin() const
{
	return FindPinChecked(ExecThenPinName);
}

UEdGraphPin* UK2Node_DynamicGetVariableByNameNode::GetTargetPin() const
{
	return FindPin(UEdGraphSchema_K2::PN_Self);
}

UEdGraphPin* UK2Node_DynamicGetVariableByNameNode::GetVarNamePin() const
{
	return FindPinChecked(VarNamePinName);
}

UEdGraphPin* UK2Node_DynamicGetVariableByNameNode::GetSuccessPin() const
{
	return FindPinChecked(SuccessPinName);
}

TArray<UEdGraphPin*> UK2Node_DynamicGetVariableByNameNode::GetAllResultPins() const
{
	TArray<UEdGraphPin*> Results;

	for (auto& Pin : Pins)
	{
		if (IsResultPin(Pin))
		{
			Results.Add(Pin);
		}
	}

	return Results;
}

void UK2Node_DynamicGetVariableByNameNode::ChangeResultPinType(const FEdGraphPinType& PinType)
{
	RecreateVariantPins(PinType);
}

FEdGraphPinType UK2Node_DynamicGetVariableByNameNode::GetResultPinType() const
{
	TArray<UEdGraphPin*> ResultPins = GetAllResultPins();
	if (ResultPins.Num() == 0)
	{
		return CreateDefaultPinType();
	}

	return ResultPins[0]->PinType;
}

#undef LOCTEXT_NAMESPACE
