/*!
 * AccessVariableByName
 *
 * Copyright (c) 2022 Colory Games
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "K2Node_DynamicGetVariableByName.h"

#include "AccessVariableByNameUtils.h"
#include "BlueprintNodeSpawner.h"
#include "EdGraphUtilities.h"
#include "EditorCategoryUtils.h"
#include "GraphEditorSettings.h"
#include "Internationalization/Regex.h"
#include "K2Node_CallFunction.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "KismetCompiler.h"
#include "Misc/EngineVersionComparison.h"
#include "VariableGetterFunctionLibrary.h"

#define LOCTEXT_NAMESPACE "K2Node"

class FKCHandler_DynamicGetVariableByName : public FNodeHandlingFunctor
{
	TMap<FString, FBPTerminal*> TermMap;

public:
	FKCHandler_DynamicGetVariableByName(FKismetCompilerContext& InCompilerContext) : FNodeHandlingFunctor(InCompilerContext)
	{
	}

	bool IsSupport(const UEdGraphPin* Pin) const
	{
#ifdef AVBN_FREE_VERSION

		FEdGraphPinType PinType = Pin->PinType;

		if (PinType.ContainerType == EPinContainerType::Array || PinType.ContainerType == EPinContainerType::Set ||
			PinType.ContainerType == EPinContainerType::Map)
		{
			return false;
		}

		if (PinType.PinCategory == UEdGraphSchema_K2::PC_Byte)
		{
			if (PinType.PinSubCategoryObject != nullptr)
			{
				return false;
			}
		}
		else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Struct)
		{
			return false;
		}

#endif

		return true;
	}

	void RegisterNodePinNets(FKismetFunctionContext& Context, UK2Node_DynamicGetVariableByNameNode* Node)
	{
		for (auto Pin : Node->Pins)
		{
			if (Pin && Pin->Direction == EGPD_Input)
			{
				if (Pin->LinkedTo.Num() == 0)
				{
					if (Pin == Node->GetTargetPin())
					{
						FBPTerminal* Term = new FBPTerminal();
						Term->CopyFromPin(Pin, Pin->PinName);
						Term->bIsLiteral = true;
						Term->Type.PinCategory = UEdGraphSchema_K2::PC_Object;
						Term->Type.PinSubCategory = UEdGraphSchema_K2::PSC_Self;
						Term->Type.PinSubCategoryObject = nullptr;

						Context.Literals.Add(Term);
						Context.NetMap.Add(Pin, Term);
					}
					else if (Pin == Node->GetVarNamePin())
					{
						RegisterLiteral(Context, Pin);
					}
				}
			}

			if (Pin && Pin->Direction == EGPD_Output)
			{
				if (Pin == Node->GetSuccessPin() || Pin == Node->GetAllResultPins()[0])
				{
					FString NewName = FString::Format(TEXT("{0}_{1}"), {*Node->GetName(), *Pin->PinName.ToString()});
					FBPTerminal* Term = Context.CreateLocalTerminalFromPinAutoChooseScope(Pin, NewName);
					Context.NetMap.Add(Pin, Term);
				}
			}
		}
	}

	void RegisterTemporaryNets(FKismetFunctionContext& Context, UK2Node_DynamicGetVariableByNameNode* Node)
	{
		FBPTerminal* AccessVariableParams = Context.CreateLocalTerminal();
		AccessVariableParams->Type.PinCategory = UEdGraphSchema_K2::PC_Struct;
		AccessVariableParams->Type.PinSubCategoryObject = FAccessVariableParams::StaticStruct();
		AccessVariableParams->Name = Context.NetNameMap->MakeValidName(Node, TEXT("AccessVariableParamsTemp"));
		AccessVariableParams->Source = Node;
		TermMap.Add("AccessVariableParamsTemp", AccessVariableParams);
	}

	void RegisterParameterNets(FKismetFunctionContext& Context, UK2Node_DynamicGetVariableByNameNode* Node)
	{
		FBPTerminal* IncludeGenerationClass = Context.CreateLocalTerminal(ETerminalSpecification::TS_Literal);
		IncludeGenerationClass->bIsLiteral = true;
		IncludeGenerationClass->Type.PinCategory = UEdGraphSchema_K2::PC_Boolean;
		IncludeGenerationClass->Name = Node->bIncludeGenerationClass ? TEXT("true") : TEXT("false");
		TermMap.Add("bIncludeGenerationClass", IncludeGenerationClass);

		FBPTerminal* ExtendIfNotPresent = Context.CreateLocalTerminal(ETerminalSpecification::TS_Literal);
		ExtendIfNotPresent->bIsLiteral = true;
		ExtendIfNotPresent->Type.PinCategory = UEdGraphSchema_K2::PC_Boolean;
		ExtendIfNotPresent->Name = TEXT("false");
		TermMap.Add("bExtendIfNotPresent", ExtendIfNotPresent);
	}

	virtual void RegisterNets(FKismetFunctionContext& Context, UEdGraphNode* Node) override
	{
		UK2Node_DynamicGetVariableByNameNode* DynamicGetVariableByNameNode =
			CastChecked<UK2Node_DynamicGetVariableByNameNode>(Node);

		FNodeHandlingFunctor::RegisterNets(Context, Node);

		RegisterNodePinNets(Context, DynamicGetVariableByNameNode);
		RegisterTemporaryNets(Context, DynamicGetVariableByNameNode);
		RegisterParameterNets(Context, DynamicGetVariableByNameNode);
	}

	void CreateMakeStructStatement(FKismetFunctionContext& Context, UK2Node_DynamicGetVariableByNameNode* Node)
	{
		UClass* FunctionClass = UVariableAccessUtilLibrary::StaticClass();
		UFunction* FunctionPtr = FindUField<UFunction>(FunctionClass, TEXT("MakeAccessVariableParams"));
		check(FunctionPtr);

		FBlueprintCompiledStatement& Statement = Context.AppendStatementForNode(Node);
		Statement.Type = KCST_CallFunction;
		Statement.FunctionToCall = FunctionPtr;
		Statement.bIsParentContext = false;
		Statement.LHS = TermMap.FindRef("AccessVariableParamsTemp");
		Statement.RHS.Add(TermMap.FindRef("bIncludeGenerationClass"));
		Statement.RHS.Add(TermMap.FindRef("bExtendIfNotPresent"));
	}

	void CreateGetFunctionCallStatement(FKismetFunctionContext& Context, UK2Node_DynamicGetVariableByNameNode* Node)
	{
		UEdGraphPin* TargetPin = Node->GetTargetPin();
		FBPTerminal* TargetTerm = nullptr;
		if (TargetPin->LinkedTo.Num() >= 1)
		{
			UEdGraphPin* TargetNet = FEdGraphUtilities::GetNetFromPin(TargetPin);
			TargetTerm = Context.NetMap.FindRef(TargetNet);
		}
		else
		{
			TargetTerm = Context.NetMap.FindRef(TargetPin);
		}

		UEdGraphPin* VarNamePin = Node->GetVarNamePin();
		UEdGraphPin* VarNameNet = FEdGraphUtilities::GetNetFromPin(VarNamePin);
		FBPTerminal* VarNameTerm = Context.NetMap.FindRef(VarNameNet);

		UEdGraphPin* SuccessPin = Node->GetSuccessPin();
		UEdGraphPin* SuccessNet = FEdGraphUtilities::GetNetFromPin(SuccessPin);
		FBPTerminal* SuccessTerm = Context.NetMap.FindRef(SuccessNet);

		UEdGraphPin* ResultPin = Node->GetAllResultPins()[0];
		UEdGraphPin* ResultNet = FEdGraphUtilities::GetNetFromPin(ResultPin);
		FBPTerminal* ResultTerm = Context.NetMap.FindRef(ResultNet);
		if (!IsSupport(ResultPin))
		{
			// clang-format off
			CompilerContext.MessageLog.Error(*LOCTEXT("NotSupported",
				"Property types 'Struct', 'Enum', 'Array', 'Set', 'Map' are not supported on the free version. "
				"Please consider to buy full version at Marketplace.")
				.ToString());
			// clang-format on
			return;
		}

		UEdGraphPin* FunctionPin = Node->GetFunctionPin();
		FBPTerminal* FunctionContext = Context.NetMap.FindRef(FunctionPin);
		UClass* FunctionClass = Cast<UClass>(FunctionPin->PinType.PinSubCategoryObject.Get());
		UFunction* FunctionPtr = FindUField<UFunction>(FunctionClass, FunctionPin->PinName);
		check(FunctionPtr);

		FBlueprintCompiledStatement& Statement = Context.AppendStatementForNode(Node);
		Statement.Type = KCST_CallFunction;
		Statement.FunctionToCall = FunctionPtr;
		Statement.FunctionContext = FunctionContext;
		Statement.bIsParentContext = false;
		Statement.RHS.Add(TargetTerm);
		Statement.RHS.Add(VarNameTerm);
		Statement.RHS.Add(TermMap.FindRef("AccessVariableParamsTemp"));
		Statement.RHS.Add(SuccessTerm);
		Statement.RHS.Add(ResultTerm);
	}

	virtual void Compile(FKismetFunctionContext& Context, UEdGraphNode* Node) override
	{
		UK2Node_DynamicGetVariableByNameNode* DynamicGetVariableByNameNode =
			CastChecked<UK2Node_DynamicGetVariableByNameNode>(Node);

		CreateMakeStructStatement(Context, DynamicGetVariableByNameNode);
		CreateGetFunctionCallStatement(Context, DynamicGetVariableByNameNode);

		if (!DynamicGetVariableByNameNode->bPureNode)
		{
			UEdGraphPin* ExecThenPin = DynamicGetVariableByNameNode->GetExecThenPin();

			GenerateSimpleThenGoto(Context, *DynamicGetVariableByNameNode, ExecThenPin);
		}
	}
};

UK2Node_DynamicGetVariableByNameNode::UK2Node_DynamicGetVariableByNameNode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InternalCallFuncClass = UVariableGetterFunctionLibarary::StaticClass();

	if (bPureNode)
	{
		InternalCallFuncName = FName("GetNestedVariableByNamePure");
	}
	else
	{
		InternalCallFuncName = FName("GetNestedVariableByName");
	}

	VariantPinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
}

void UK2Node_DynamicGetVariableByNameNode::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.ChangeType == EPropertyChangeType::Unspecified)
	{
		return;
	}

	// Get old variant pin type.
	TArray<UEdGraphPin*> OldPins = MoveTemp(Pins);
	FEdGraphPinType OldVariantPinType;
	for (auto& Pin : OldPins)
	{
		if (IsResultPin(Pin))
		{
			OldVariantPinType = Pin->PinType;
		}
	}

	// Allocate new pins.
	AllocateDefaultPins();
	for (auto& Pin : Pins)
	{
		if (IsResultPin(Pin))
		{
			Pin->PinType = VariantPinType;
#if !UE_VERSION_OLDER_THAN(5, 0, 0)
			if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Real)
			{
				Pin->PinType.PinSubCategory = bSinglePrecision ? UEdGraphSchema_K2::PC_Float : UEdGraphSchema_K2::PC_Double;
			}
#endif
		}
	}

	// Restore connection.
	RestoreSplitPins(OldPins);
	RewireOldPinsToNewPins(OldPins, Pins, nullptr);

	// Break connection if the variant pin type is changed.
	if (OldVariantPinType != VariantPinType)
	{
		for (auto& Pin : Pins)
		{
			if (IsResultPin(Pin))
			{
				Pin->BreakAllPinLinks();
			}
		}
	}

	// Remove old pins.
	for (auto& Pin : OldPins)
	{
		RemovePin(Pin);
	}

	UEdGraph* Graph = GetGraph();
	Graph->NotifyGraphChanged();

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
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
	if (!bPureNode)
	{
		CreateExecTriggeringPin();
		CreateExecThenPin();
	}
	CreateTargetPin();
	CreateVarNamePin();
	CreateSuccessPin();

	FEdGraphPinType InitialPinType = VariantPinType;
#if !UE_VERSION_OLDER_THAN(5, 0, 0)
	if (InitialPinType.PinCategory == UEdGraphSchema_K2::PC_Real)
	{
		InitialPinType.PinSubCategory = bSinglePrecision ? UEdGraphSchema_K2::PC_Float : UEdGraphSchema_K2::PC_Double;
	}
#endif

	CreateResultPin(InitialPinType, 0);

	Super::AllocateDefaultPins();
}

void UK2Node_DynamicGetVariableByNameNode::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
	// Get previous result pin.
	UEdGraphPin* OldResultPin = nullptr;
	for (auto& Pin : OldPins)
	{
		if (IsResultPin(Pin))
		{
			OldResultPin = Pin;
			break;
		}
	}

	CreateFunctionPin();
	if (!bPureNode)
	{
		CreateExecTriggeringPin();
		CreateExecThenPin();
	}
	CreateTargetPin();
	CreateVarNamePin();
	CreateSuccessPin();

	// [Backward compatibility] Change pin type to the previous result pin type.
	FEdGraphPinType InitialPinType = VariantPinType;
	if (OldResultPin != nullptr)
	{
		if (InitialPinType.PinCategory == UEdGraphSchema_K2::PC_Boolean)
		{
			InitialPinType = OldResultPin->PinType;
		}
	}

#if !UE_VERSION_OLDER_THAN(5, 0, 0)
	if (InitialPinType.PinCategory == UEdGraphSchema_K2::PC_Real)
	{
		InitialPinType.PinSubCategory = bSinglePrecision ? UEdGraphSchema_K2::PC_Float : UEdGraphSchema_K2::PC_Double;
	}
#endif

	CreateResultPin(InitialPinType, 0);

	RestoreSplitPins(OldPins);

	VariantPinType = InitialPinType;
}

FText UK2Node_DynamicGetVariableByNameNode::GetTooltipText() const
{
	return LOCTEXT("DynamicGetVariableByNameNode_Tooltip", "Get Variable by Name (Dynamic)");
}

FLinearColor UK2Node_DynamicGetVariableByNameNode::GetNodeTitleColor() const
{
	if (bPureNode)
	{
		return GetDefault<UGraphEditorSettings>()->PureFunctionCallNodeTitleColor;
	}
	else
	{
		return GetDefault<UGraphEditorSettings>()->FunctionCallNodeTitleColor;
	}
}

FText UK2Node_DynamicGetVariableByNameNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("Get Variable by Name (Dynamic)", "Get Variable by Name (Dynamic)");
}

FSlateIcon UK2Node_DynamicGetVariableByNameNode::GetIconAndTint(FLinearColor& OutColor) const
{
	static FSlateIcon Icon("EditorStyle", "GraphEditor.Function_16x");

	if (bPureNode)
	{
		OutColor = GetDefault<UGraphEditorSettings>()->PureFunctionCallNodeTitleColor;
	}
	else
	{
		OutColor = GetDefault<UGraphEditorSettings>()->FunctionCallNodeTitleColor;
	}

	return Icon;
}

void UK2Node_DynamicGetVariableByNameNode::CreateFunctionPin()
{
	FCreatePinParams Params;
	Params.Index = 0;

	if (bPureNode)
	{
		InternalCallFuncName = FName("GetNestedVariableByNamePure");
	}
	else
	{
		InternalCallFuncName = FName("GetNestedVariableByName");
	}

	UClass* FunctionClass = UVariableGetterFunctionLibarary::StaticClass();
	UFunction* FunctionPtr = FunctionClass->FindFunctionByName(InternalCallFuncName);
	check(FunctionPtr);

	UEdGraphPin* FunctionPin =
		CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Object, InternalCallFuncClass, InternalCallFuncName, Params);
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
	UEdGraphPin* Pin =
		CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Object, UObject::StaticClass(), UEdGraphSchema_K2::PN_Self, Params);
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
	FName ResultPinName = FName(FString::Format(TEXT("{0}_{1}"), {*ResultPinNamePrefix.ToString(), Index}));
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

	FRegexPattern Pattern = FRegexPattern(FString::Format(TEXT("^{0}_[0-9]*$"), {*ResultPinNamePrefix.ToString()}));
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
	if (bPureNode)
	{
		return FindPinChecked(FName("GetNestedVariableByNamePure"));
	}
	else
	{
		return FindPinChecked(FName("GetNestedVariableByName"));
	}
}

UEdGraphPin* UK2Node_DynamicGetVariableByNameNode::GetExecThenPin() const
{
	if (bPureNode)
	{
		return FindPin(ExecThenPinName);
	}
	else
	{
		return FindPinChecked(ExecThenPinName);
	}
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

#undef LOCTEXT_NAMESPACE
