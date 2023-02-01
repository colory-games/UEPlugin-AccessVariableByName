/*!
 * AccessVariableByName
 *
 * Copyright (c) 2022 Colory Games
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "K2Node_DynamicSetVariableByName.h"

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
#include "VariableSetterFunctionLibrary.h"

#define LOCTEXT_NAMESPACE "K2Node"

class FKCHandler_DynamicSetVariableByName : public FNodeHandlingFunctor
{
	TMap<FString, FBPTerminal*> TermMap;

public:
	FKCHandler_DynamicSetVariableByName(FKismetCompilerContext& InCompilerContext) : FNodeHandlingFunctor(InCompilerContext)
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

	void RegisterNodePinNets(FKismetFunctionContext& Context, UK2Node_DynamicSetVariableByNameNode* Node)
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
					else if (Pin == Node->GetVarNamePin() || Pin == Node->GetAllNewValuePins()[0])
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

	void RegisterTemporaryNets(FKismetFunctionContext& Context, UK2Node_DynamicSetVariableByNameNode* Node)
	{
		FBPTerminal* AccessVariableParams = Context.CreateLocalTerminal();
		AccessVariableParams->Type.PinCategory = UEdGraphSchema_K2::PC_Struct;
		AccessVariableParams->Type.PinSubCategoryObject = FAccessVariableParams::StaticStruct();
		AccessVariableParams->Name = Context.NetNameMap->MakeValidName(Node, TEXT("AccessVariableParamsTemp"));
		AccessVariableParams->Source = Node;
		TermMap.Add("AccessVariableParamsTemp", AccessVariableParams);
	}

	void RegisterParameterNets(FKismetFunctionContext& Context, UK2Node_DynamicSetVariableByNameNode* Node)
	{
		FBPTerminal* IncludeGenerationClass = Context.CreateLocalTerminal(ETerminalSpecification::TS_Literal);
		IncludeGenerationClass->bIsLiteral = true;
		IncludeGenerationClass->Type.PinCategory = UEdGraphSchema_K2::PC_Boolean;
		IncludeGenerationClass->Name = Node->bIncludeGenerationClass ? TEXT("true") : TEXT("false");
		TermMap.Add("bIncludeGenerationClass", IncludeGenerationClass);

		FBPTerminal* ExtendIfNotPresent = Context.CreateLocalTerminal(ETerminalSpecification::TS_Literal);
		ExtendIfNotPresent->bIsLiteral = true;
		ExtendIfNotPresent->Type.PinCategory = UEdGraphSchema_K2::PC_Boolean;
		ExtendIfNotPresent->Name = Node->bExtendIfNotPresent ? TEXT("true") : TEXT("false");
		TermMap.Add("bExtendIfNotPresent", ExtendIfNotPresent);
	}

	virtual void RegisterNets(FKismetFunctionContext& Context, UEdGraphNode* Node) override
	{
		UK2Node_DynamicSetVariableByNameNode* DynamicSetVariableByNameNode =
			CastChecked<UK2Node_DynamicSetVariableByNameNode>(Node);

		FNodeHandlingFunctor::RegisterNets(Context, Node);

		RegisterNodePinNets(Context, DynamicSetVariableByNameNode);
		RegisterTemporaryNets(Context, DynamicSetVariableByNameNode);
		RegisterParameterNets(Context, DynamicSetVariableByNameNode);
	}

	void CreateMakeStructStatement(FKismetFunctionContext& Context, UK2Node_DynamicSetVariableByNameNode* Node)
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

	void CreateGetFunctionCallStatement(FKismetFunctionContext& Context, UK2Node_DynamicSetVariableByNameNode* Node)
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

		UEdGraphPin* NewValuePin = Node->GetAllNewValuePins()[0];
		UEdGraphPin* NewValueNet = FEdGraphUtilities::GetNetFromPin(NewValuePin);
		FBPTerminal* NewValueTerm = Context.NetMap.FindRef(NewValueNet);

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

		FBlueprintCompiledStatement& CallFuncStatement = Context.AppendStatementForNode(Node);
		CallFuncStatement.Type = KCST_CallFunction;
		CallFuncStatement.FunctionToCall = FunctionPtr;
		CallFuncStatement.FunctionContext = FunctionContext;
		CallFuncStatement.bIsParentContext = false;
		CallFuncStatement.RHS.Add(TargetTerm);
		CallFuncStatement.RHS.Add(VarNameTerm);
		CallFuncStatement.RHS.Add(TermMap.FindRef("AccessVariableParamsTemp"));
		CallFuncStatement.RHS.Add(SuccessTerm);
		CallFuncStatement.RHS.Add(ResultTerm);
		CallFuncStatement.RHS.Add(NewValueTerm);	// Argument order is different from pin index.
	}

	virtual void Compile(FKismetFunctionContext& Context, UEdGraphNode* Node) override
	{
		UK2Node_DynamicSetVariableByNameNode* DynamicSetVariableByNameNode =
			CastChecked<UK2Node_DynamicSetVariableByNameNode>(Node);

		CreateMakeStructStatement(Context, DynamicSetVariableByNameNode);
		CreateGetFunctionCallStatement(Context, DynamicSetVariableByNameNode);

		UEdGraphPin* ExecThenPin = DynamicSetVariableByNameNode->GetExecThenPin();

		GenerateSimpleThenGoto(Context, *DynamicSetVariableByNameNode, ExecThenPin);
	}
};

UK2Node_DynamicSetVariableByNameNode::UK2Node_DynamicSetVariableByNameNode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InternalCallFuncClass = UVariableSetterFunctionLibarary::StaticClass();
	InternalCallFuncName = FName("SetNestedVariableByName");

	VariantPinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
}

void UK2Node_DynamicSetVariableByNameNode::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
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
		if (IsNewValuePin(Pin) || IsResultPin(Pin))
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
			if (IsNewValuePin(Pin) || IsResultPin(Pin))
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
	//   N: Number of result pin
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

	FEdGraphPinType InitialPinType = VariantPinType;
#if !UE_VERSION_OLDER_THAN(5, 0, 0)
	if (InitialPinType.PinCategory == UEdGraphSchema_K2::PC_Real)
	{
		InitialPinType.PinSubCategory = bSinglePrecision ? UEdGraphSchema_K2::PC_Float : UEdGraphSchema_K2::PC_Double;
	}
#endif

	CreateNewValuePin(InitialPinType, 0);
	CreateResultPin(InitialPinType, 0);

	Super::AllocateDefaultPins();
}

void UK2Node_DynamicSetVariableByNameNode::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
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
	CreateExecTriggeringPin();
	CreateExecThenPin();
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

	CreateNewValuePin(InitialPinType, 0);
	CreateResultPin(InitialPinType, 0);

	RestoreSplitPins(OldPins);

	VariantPinType = InitialPinType;
}

FText UK2Node_DynamicSetVariableByNameNode::GetTooltipText() const
{
	return LOCTEXT("DynamicSetVariableByNameNode_Tooltip", "Set Variable by Name (Dynamic)");
}

FLinearColor UK2Node_DynamicSetVariableByNameNode::GetNodeTitleColor() const
{
	return GetDefault<UGraphEditorSettings>()->FunctionCallNodeTitleColor;
}

FText UK2Node_DynamicSetVariableByNameNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("Set Variable by Name (Dynamic)", "Set Variable by Name (Dynamic)");
}

FSlateIcon UK2Node_DynamicSetVariableByNameNode::GetIconAndTint(FLinearColor& OutColor) const
{
	static FSlateIcon Icon("EditorStyle", "GraphEditor.Function_16x");

	OutColor = GetDefault<UGraphEditorSettings>()->FunctionCallNodeTitleColor;

	return Icon;
}

void UK2Node_DynamicSetVariableByNameNode::CreateFunctionPin()
{
	FCreatePinParams Params;
	Params.Index = 0;

	UClass* FunctionClass = UVariableSetterFunctionLibarary::StaticClass();
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
	UEdGraphPin* Pin =
		CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Object, UObject::StaticClass(), UEdGraphSchema_K2::PN_Self, Params);
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
	FName NewValuePinName = FName(FString::Format(TEXT("{0}_{1}"), {*NewValuePinNamePrefix.ToString(), Index}));
	FString NewValuePinFriendlyName = "New Value";

	FCreatePinParams Params;
	Params.Index = 6 + Index * 2;
	UEdGraphPin* Pin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Boolean, NewValuePinName, Params);
	Pin->PinFriendlyName = FText::AsCultureInvariant(NewValuePinFriendlyName);
	Pin->PinType = PinType;
}

void UK2Node_DynamicSetVariableByNameNode::CreateResultPin(const FEdGraphPinType& PinType, int32 Index)
{
	FName ResultPinName = FName(FString::Format(TEXT("{0}_{1}"), {*ResultPinNamePrefix.ToString(), Index}));
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

	FRegexPattern Pattern = FRegexPattern(FString::Format(TEXT("^{0}_[0-9]*$"), {*NewValuePinNamePrefix.ToString()}));
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

	FRegexPattern Pattern = FRegexPattern(FString::Format(TEXT("^{0}_[0-9]*$"), {*ResultPinNamePrefix.ToString()}));
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
	return FindPin(UEdGraphSchema_K2::PN_Self);
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

#undef LOCTEXT_NAMESPACE
