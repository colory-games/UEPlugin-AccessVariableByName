/*!
 * AccessVariableByName
 *
 * Copyright (c) 2022 Colory Games
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "K2Node_GetVariableByName.h"

#include "AccessVariableByNameUtils.h"
#include "BlueprintNodeSpawner.h"
#include "EditorCategoryUtils.h"
#include "GraphEditorSettings.h"
#include "Internationalization/Regex.h"
#include "K2Node_Self.h"
#include "KismetCompiler.h"
#include "Misc/EngineVersionComparison.h"
#include "VariableGetterFunctionLibrary.h"

#define LOCTEXT_NAMESPACE "K2Node"

UK2Node_GetVariableByNameNode::UK2Node_GetVariableByNameNode(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UK2Node_GetVariableByNameNode::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.ChangeType == EPropertyChangeType::Unspecified)
	{
		return;
	}

	TArray<UEdGraphPin*> OldPins = MoveTemp(Pins);
	UEdGraphPin* OldTargetPin = nullptr;
	UEdGraphPin* OldVarNamePin = nullptr;
	for (auto& Pin : OldPins)
	{
		if (Pin->GetFName() == UEdGraphSchema_K2::PN_Self)
		{
			OldTargetPin = Pin;
		}
		else if (Pin->GetFName() == VarNamePinName)
		{
			OldVarNamePin = Pin;
		}
	}

	AllocateDefaultPins();

	UClass* TargetClass = GetTargetClass(OldTargetPin);
	FString VarName = OldVarNamePin->DefaultValue;
	RecreateVariantPinInternal(TargetClass, VarName);

	// Restore connection.
	RestoreSplitPins(OldPins);
	RewireOldPinsToNewPins(OldPins, Pins, nullptr);

	for (auto& Pin : OldPins)
	{
		RemovePin(Pin);
	}

	UEdGraph* Graph = GetGraph();
	Graph->NotifyGraphChanged();
}

FText UK2Node_GetVariableByNameNode::GetMenuCategory() const
{
	return FEditorCategoryUtils::GetCommonCategory(FCommonEditorCategory::Variables);
}

void UK2Node_GetVariableByNameNode::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	UClass* ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);

		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

UK2Node_MakeStruct* UK2Node_GetVariableByNameNode::CreateMakeStructNode(
	FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	UK2Node_MakeStruct* MakeStruct = CompilerContext.SpawnIntermediateNode<UK2Node_MakeStruct>(this, SourceGraph);
	MakeStruct->StructType = FAccessVariableParams::StaticStruct();
	MakeStruct->AllocateDefaultPins();
	MakeStruct->bMadeAfterOverridePinRemoval = true;
	MakeStruct->GetSchema()->TrySetDefaultValue(
		*MakeStruct->FindPinChecked(GET_MEMBER_NAME_STRING_CHECKED(FAccessVariableParams, bIncludeGenerationClass)),
		bIncludeGenerationClass ? TEXT("true") : TEXT("false"));
	MakeStruct->GetSchema()->TrySetDefaultValue(
		*MakeStruct->FindPinChecked(GET_MEMBER_NAME_STRING_CHECKED(FAccessVariableParams, bExtendIfNotPresent)), TEXT("false"));

	return MakeStruct;
}

UK2Node_CallFunction* UK2Node_GetVariableByNameNode::CreateGetFunctionCallNode(
	FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph, UEdGraphPin* ResultPin)
{
	UFunction* GetterFunction = FindGetterFunction(ResultPin);
	if (GetterFunction == nullptr)
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("FunctionNotFound", "The getter function is not found.").ToString());
		return nullptr;
	}
	UK2Node_CallFunction* CallFunction = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	CallFunction->SetFromFunction(GetterFunction);
	CallFunction->AllocateDefaultPins();

	return CallFunction;
}

void UK2Node_GetVariableByNameNode::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	UEdGraphPin* TargetPin = GetTargetPin();
	UEdGraphPin* VarNamePin = GetVarNamePin();
	UEdGraphPin* SuccessPin = GetSuccessPin();
	TArray<UEdGraphPin*> ResultPins = GetAllResultPins();

	if (VarNamePin->LinkedTo.Num() != 0)
	{
		// clang-format off
		CompilerContext.MessageLog.Error(*LOCTEXT("InvalidVarNameInput",
			"Var Name pin only supports literal value. Consider to use 'Get Variable by Name (Dynamic)' node instead")
			.ToString());
		// clang-format on
		return;
	}
	if (ResultPins.Num() == 0)
	{
		return;
	}

	UEdGraphPin* ResultPin = ResultPins[0];
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

	// Create intermidiate nodes.
	UK2Node_MakeStruct* MakeStruct = CreateMakeStructNode(CompilerContext, SourceGraph);
	UK2Node_CallFunction* CallFunction = CreateGetFunctionCallNode(CompilerContext, SourceGraph, ResultPin);
	if (CallFunction == nullptr)
	{
		return;
	}

	UEdGraphPin* FunctionTargetPin = CallFunction->FindPinChecked(TEXT("Target"));
	UEdGraphPin* FunctionVarNamePin = CallFunction->FindPinChecked(TEXT("VarName"));
	UEdGraphPin* FunctionParamsPin = CallFunction->FindPinChecked(TEXT("Params"));
	UEdGraphPin* FunctionSuccessPin = CallFunction->FindPinChecked(TEXT("Success"));
	UEdGraphPin* FunctionResultPin = CallFunction->FindPinChecked(TEXT("Result"));

	// Change result pin type along to node's one.
	FunctionResultPin->PinType = ResultPin->PinType;
	FunctionResultPin->PinType.PinSubCategory = ResultPin->PinType.PinSubCategory;
	FunctionResultPin->PinType.PinSubCategoryObject = ResultPin->PinType.PinSubCategoryObject;
	FunctionResultPin->PinType.PinValueType = ResultPin->PinType.PinValueType;

	// Link between Makestruct node and CallFunction node.
	UEdGraphPin* AccessVariableParamsPin = MakeStruct->FindPinChecked(TEXT("AccessVariableParams"));
	AccessVariableParamsPin->MakeLinkTo(FunctionParamsPin);

	// Link execution pins.
	if (!bPureNode)
	{
		UEdGraphPin* ExecTriggeringPin = GetExecPin();
		UEdGraphPin* ExecThenPin = GetExecThenPin();
		UEdGraphPin* FunctionExecTriggeringPin = CallFunction->GetExecPin();
		UEdGraphPin* FunctionExecThenPin = CallFunction->GetThenPin();

		CompilerContext.MovePinLinksToIntermediate(*ExecTriggeringPin, *FunctionExecTriggeringPin);
		CompilerContext.MovePinLinksToIntermediate(*ExecThenPin, *FunctionExecThenPin);
	}

	// Link target pin.
	if (TargetPin->LinkedTo.Num() >= 1)
	{
		CompilerContext.MovePinLinksToIntermediate(*TargetPin, *FunctionTargetPin);
	}
	else
	{
		// Handle self node case.
		UK2Node_Self* SelfNode = CompilerContext.SpawnIntermediateNode<UK2Node_Self>(this, SourceGraph);
		SelfNode->AllocateDefaultPins();

		UEdGraphPin* OutPin = SelfNode->Pins[0];
		OutPin->MakeLinkTo(FunctionTargetPin);
	}

	// Link rest pins.
	CompilerContext.MovePinLinksToIntermediate(*VarNamePin, *FunctionVarNamePin);
	CompilerContext.MovePinLinksToIntermediate(*SuccessPin, *FunctionSuccessPin);
	CompilerContext.MovePinLinksToIntermediate(*ResultPin, *FunctionResultPin);

	BreakAllNodeLinks();
}

void UK2Node_GetVariableByNameNode::AllocateDefaultPins()
{
	// Pin structure
	//   N: Number of result pin
	// -----
	// 0: Execution Triggering (In, Exec)
	// 1: Execution Then (Out, Exec)
	// 2: Target (In, Object Reference)
	// 3: Var Name (In, FName)
	// 4: Success (Out, Boolean)
	// 5-: Result (Out, *)

	if (!bPureNode)
	{
		CreateExecTriggeringPin();
		CreateExecThenPin();
	}
	CreateTargetPin();
	CreateVarNamePin();
	CreateSuccessPin();

	Super::AllocateDefaultPins();
}

void UK2Node_GetVariableByNameNode::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
	UEdGraphPin* OldTargetPin = nullptr;
	UEdGraphPin* OldVarNamePin = nullptr;

	for (auto& Pin : OldPins)
	{
		if (Pin->GetFName() == UEdGraphSchema_K2::PN_Self)
		{
			OldTargetPin = Pin;
		}
		else if (Pin->GetFName() == VarNamePinName)
		{
			OldVarNamePin = Pin;
		}
	}

	AllocateDefaultPins();

	if (OldTargetPin == nullptr || OldVarNamePin == nullptr)
	{
		RestoreSplitPins(OldPins);
		return;
	}

	UClass* TargetClass = GetTargetClass(OldTargetPin);
	FString VarName = OldVarNamePin->DefaultValue;
	RecreateVariantPinInternal(TargetClass, VarName);

	RestoreSplitPins(OldPins);
}

FText UK2Node_GetVariableByNameNode::GetTooltipText() const
{
	return LOCTEXT("GetVariableByNameNode_Tooltip", "Get Variable by Name");
}

FLinearColor UK2Node_GetVariableByNameNode::GetNodeTitleColor() const
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

FText UK2Node_GetVariableByNameNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("Get Variable by Name", "Get Variable by Name");
}

FSlateIcon UK2Node_GetVariableByNameNode::GetIconAndTint(FLinearColor& OutColor) const
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

void UK2Node_GetVariableByNameNode::PinDefaultValueChanged(UEdGraphPin* Pin)
{
	if (Pin == nullptr)
	{
		return;
	}

	if (Pin->PinName == UEdGraphSchema_K2::PN_Self)
	{
		RecreateVariantPin();
	}
	else if (Pin->PinName == VarNamePinName)
	{
		TArray<UEdGraphPin*> ResultPins = GetAllResultPins();
		for (auto& ResultPin : ResultPins)
		{
			ResultPin->BreakAllPinLinks();
		}
		RecreateVariantPin();
	}
}

void UK2Node_GetVariableByNameNode::PinConnectionListChanged(UEdGraphPin* Pin)
{
	if (Pin == nullptr)
	{
		return;
	}

	if (Pin->PinName == UEdGraphSchema_K2::PN_Self)
	{
		RecreateVariantPin();
	}
}

void UK2Node_GetVariableByNameNode::CreateExecTriggeringPin()
{
	FCreatePinParams Params;
	Params.Index = 0;
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute, Params);
}

void UK2Node_GetVariableByNameNode::CreateExecThenPin()
{
	FCreatePinParams Params;
	Params.Index = 1;
	UEdGraphPin* DefaultExecPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, ExecThenPinName, Params);
	DefaultExecPin->PinFriendlyName = FText::AsCultureInvariant(ExecThenPinFriendlyName);
}

void UK2Node_GetVariableByNameNode::CreateTargetPin()
{
	FCreatePinParams Params;
	Params.Index = 2;
	UEdGraphPin* Pin =
		CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Object, UObject::StaticClass(), UEdGraphSchema_K2::PN_Self, Params);
	Pin->PinFriendlyName = FText::AsCultureInvariant(TargetPinFriendlyName);
}

void UK2Node_GetVariableByNameNode::CreateVarNamePin()
{
	FCreatePinParams Params;
	Params.Index = 3;
	UEdGraphPin* Pin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Name, VarNamePinName, Params);
	Pin->PinFriendlyName = FText::AsCultureInvariant(VarNamePinFriendlyName);
}

void UK2Node_GetVariableByNameNode::CreateSuccessPin()
{
	FCreatePinParams Params;
	Params.Index = 4;
	UEdGraphPin* Pin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Boolean, SuccessPinName, Params);
	Pin->PinFriendlyName = FText::AsCultureInvariant(SuccessPinFriendlyName);
}

void UK2Node_GetVariableByNameNode::CreateResultPin(const FEdGraphPinType& PinType, FString PropertyName, int32 Index)
{
	FName ResultPinName = FName(FString::Format(TEXT("{0}_{1}"), {*ResultPinNamePrefix.ToString(), Index}));
	FString ResultPinFriendlyName = PropertyName;

	FCreatePinParams Params;
	Params.Index = 5 + Index;
	UEdGraphPin* Pin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Boolean, ResultPinName, Params);
	Pin->PinFriendlyName = FText::AsCultureInvariant(ResultPinFriendlyName);
	Pin->PinType = PinType;
}

void UK2Node_GetVariableByNameNode::RecreateVariantPinInternal(UClass* TargetClass, const FString& VarName)
{
	if (TargetClass == nullptr)
	{
		return;
	}

	TArray<FString> Vars;
	FVariableAccessFunctionLibraryUtils::SplitVarName(VarName, &Vars);
	TArray<FVarDescription> VarDescs;
	FVariableAccessFunctionLibraryUtils::AnalyzeVarNames(Vars, &VarDescs);

	FAccessVariableParams Params;
	Params.bIncludeGenerationClass = bIncludeGenerationClass;
	TerminalProperty TP = GetTerminalProperty(VarDescs, 0, TargetClass, Params);
	if (TP.Property != nullptr)
	{
		const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

		FEdGraphPinType PinType;
		Schema->ConvertPropertyToPinType(TP.Property, PinType);
		CreateResultPin(PinType, TP.Property->GetAuthoredName(), 0);
	}
}

void UK2Node_GetVariableByNameNode::RecreateVariantPin()
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

	// Create new result pin.
	UClass* TargetClass = GetTargetClass();
	FString VarName = GetVarNamePin()->DefaultValue;
	RecreateVariantPinInternal(TargetClass, VarName);

	// Restore connection.
	RestoreSplitPins(UnusedPins);
	RewireOldPinsToNewPins(UnusedPins, Pins, nullptr);

	UEdGraph* Graph = GetGraph();
	Graph->NotifyGraphChanged();
}

UClass* UK2Node_GetVariableByNameNode::GetTargetClass(UEdGraphPin* Pin)
{
	UClass* TargetClass = nullptr;
	UEdGraphPin* TargetPin = Pin;

	if (TargetPin == nullptr)
	{
		TargetPin = GetTargetPin();
	}

	if (TargetPin == nullptr)
	{
		return nullptr;
	}

	if (TargetPin->PinName == UEdGraphSchema_K2::PN_Self && TargetPin->LinkedTo.Num() == 0)
	{
		UEdGraphNode* OwningNode = TargetPin->GetOwningNode();
		UClass* Class = GetClassFromNode(OwningNode);
		if (Class != nullptr)
		{
			TargetClass = Class;
		}
	}
	else if (TargetPin->LinkedTo.Num() > 0)
	{
		UEdGraphPin* LinkedPin = TargetPin->LinkedTo[0];
		if (LinkedPin != nullptr)
		{
			if (LinkedPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Object && LinkedPin->PinType.PinSubCategory == "self")
			{
				UEdGraphNode* OwningNode = LinkedPin->GetOwningNode();
				UK2Node_Self* SelfNode = CastChecked<UK2Node_Self>(OwningNode);
				UClass* Class = GetClassFromNode(SelfNode);
				if (Class != nullptr)
				{
					TargetClass = Class;
				}
			}
			else
			{
				TargetClass = Cast<UClass>(LinkedPin->PinType.PinSubCategoryObject.Get());
			}
		}
	}

	if (TargetClass)
	{
		UBlueprint* Blueprint = Cast<UBlueprint>(TargetClass->ClassGeneratedBy);
		if (Blueprint != nullptr && Blueprint->SkeletonGeneratedClass)
		{
			TargetClass = Blueprint->SkeletonGeneratedClass;
		}
	}

	return TargetClass;
}

UFunction* UK2Node_GetVariableByNameNode::FindGetterFunction(UEdGraphPin* Pin)
{
	UClass* FunctionLibrary = UVariableGetterFunctionLibarary::StaticClass();

	if (bPureNode)
	{
		return FunctionLibrary->FindFunctionByName(FName("GetNestedVariableByNamePure"));
	}

	return FunctionLibrary->FindFunctionByName(FName("GetNestedVariableByName"));
}

bool UK2Node_GetVariableByNameNode::IsResultPin(const UEdGraphPin* Pin) const
{
	FString PinName = Pin->GetFName().ToString();

	FRegexPattern Pattern = FRegexPattern(FString::Format(TEXT("^{0}_[0-9]+$"), {*ResultPinNamePrefix.ToString()}));
	FRegexMatcher Matcher(Pattern, PinName);
	if (Matcher.FindNext())
	{
		return true;
	}

	return false;
}

bool UK2Node_GetVariableByNameNode::IsSupport(const UEdGraphPin* Pin) const
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

UEdGraphPin* UK2Node_GetVariableByNameNode::GetExecThenPin() const
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

UEdGraphPin* UK2Node_GetVariableByNameNode::GetTargetPin() const
{
	return FindPin(UEdGraphSchema_K2::PN_Self);
}

UEdGraphPin* UK2Node_GetVariableByNameNode::GetVarNamePin() const
{
	return FindPinChecked(VarNamePinName);
}

UEdGraphPin* UK2Node_GetVariableByNameNode::GetSuccessPin() const
{
	return FindPinChecked(SuccessPinName);
}

TArray<UEdGraphPin*> UK2Node_GetVariableByNameNode::GetAllResultPins() const
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
