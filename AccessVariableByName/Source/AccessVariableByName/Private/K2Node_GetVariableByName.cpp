/*!
 * AccessVariableByName
 *
 * Copyright (c) 2022 Colory Games
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "K2Node_GetVariableByName.h"

#include "BlueprintNodeSpawner.h"
#include "Common.h"
#include "EditorCategoryUtils.h"
#include "GraphEditorSettings.h"
#include "K2Node_CallFunction.h"
#include "K2Node_Self.h"
#include "KismetCompiler.h"
#include "VariableGetterFunctionLibrary.h"
#include "Internationalization/Regex.h"

#define LOCTEXT_NAMESPACE "K2Node"


UK2Node_GetVariableByNameNode::UK2Node_GetVariableByNameNode(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
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

void UK2Node_GetVariableByNameNode::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	UEdGraphPin* ExecTriggeringPin = GetExecPin();
	UEdGraphPin* ExecThenPin = GetExecThenPin();
	UEdGraphPin* TargetPin = GetTargetPin();
	UEdGraphPin* VarNamePin = GetVarNamePin();
	UEdGraphPin* SuccessPin = GetSuccessPin();
	TArray<UEdGraphPin*> ResultPins = GetAllResultPins();

	if (VarNamePin->LinkedTo.Num() != 0)
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("InvalidVarNameInput", "Var Name pin only supports literal value. Consider to use 'Get Variable by Name (Dynamic)' node instead").ToString());
		return;
	}

	if (ResultPins.Num() == 0)
	{
		return;
	}
	UEdGraphPin* ResultPin = ResultPins[0];

	UFunction* GetterFunction = FindGetterFunction(ResultPin);
	if (GetterFunction == nullptr)
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("FunctionNotFound", "The getter function is not found.").ToString());
		return;
	}

	UK2Node_CallFunction* CallFunction = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	CallFunction->SetFromFunction(GetterFunction);
	CallFunction->AllocateDefaultPins();

	UEdGraphPin* FunctionExecTriggeringPin = CallFunction->GetExecPin();
	UEdGraphPin* FunctionExecThenPin = CallFunction->GetThenPin();
	UEdGraphPin* FunctionTargetPin = CallFunction->FindPinChecked(TEXT("Target"));
	UEdGraphPin* FunctionVarNamePin = CallFunction->FindPinChecked(TEXT("VarName"));
	UEdGraphPin* FunctionSuccessPin = CallFunction->FindPinChecked(TEXT("Success"));
	UEdGraphPin* FunctionResultPin = CallFunction->FindPinChecked(TEXT("Result"));

	FunctionResultPin->PinType = ResultPin->PinType;
	FunctionResultPin->PinType.PinSubCategory = ResultPin->PinType.PinSubCategory;
	FunctionResultPin->PinType.PinSubCategoryObject = ResultPin->PinType.PinSubCategoryObject;
	FunctionResultPin->PinType.PinValueType = ResultPin->PinType.PinValueType;

	CompilerContext.MovePinLinksToIntermediate(*ExecTriggeringPin, *FunctionExecTriggeringPin);
	CompilerContext.MovePinLinksToIntermediate(*ExecThenPin, *FunctionExecThenPin);
	CompilerContext.MovePinLinksToIntermediate(*TargetPin, *FunctionTargetPin);
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

	CreateExecTriggeringPin();
	CreateExecThenPin();
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
		if (Pin->GetFName() == TargetPinName)
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
	RecreateResultPinInternal(TargetClass, VarName);

	RestoreSplitPins(OldPins);
}

FText UK2Node_GetVariableByNameNode::GetTooltipText() const
{
	return LOCTEXT("GetVariableByNameNode_Tooltip", "Get Variable by Name");
}

FLinearColor UK2Node_GetVariableByNameNode::GetNodeTitleColor() const
{
	return GetDefault<UGraphEditorSettings>()->ObjectPinTypeColor;
}

FText UK2Node_GetVariableByNameNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("Get Variable by Name", "Get Variable by Name");
}

FSlateIcon UK2Node_GetVariableByNameNode::GetIconAndTint(FLinearColor& OutColor) const
{
	static FSlateIcon Icon("EditorStyle", "GraphEditor.Function_16x");
	return Icon;
}

void UK2Node_GetVariableByNameNode::PinDefaultValueChanged(UEdGraphPin* Pin)
{
	if (Pin == nullptr)
	{
		return;
	}

	if (Pin->PinName == TargetPinName)
	{
		RecreateResultPin();
	}
	else if (Pin->PinName == VarNamePinName)
	{
		TArray<UEdGraphPin*> ResultPins = GetAllResultPins();
		for (auto& ResultPin : ResultPins)
		{
			ResultPin->BreakAllPinLinks();
		}
		RecreateResultPin();
	}
}

void UK2Node_GetVariableByNameNode::PinConnectionListChanged(UEdGraphPin* Pin)
{
	if (Pin == nullptr)
	{
		return;
	}

	if (Pin->PinName == TargetPinName)
	{
		RecreateResultPin();
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
	UEdGraphPin* Pin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Object, UObject::StaticClass(), TargetPinName, Params);
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
	FName ResultPinName = FName(FString::Format(TEXT("{0}{1}"), {*ResultPinNamePrefix.ToString(), *PropertyName}));
	FString ResultPinFriendlyName = PropertyName;

	FCreatePinParams Params;
	Params.Index = 5 + Index;
	UEdGraphPin* Pin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Boolean, ResultPinName, Params);
	Pin->PinFriendlyName = FText::AsCultureInvariant(ResultPinFriendlyName);
	Pin->PinType = PinType;
}

void UK2Node_GetVariableByNameNode::RecreateResultPinInternal(UClass* TargetClass, const FString& VarName)
{
	if (TargetClass == nullptr)
	{
		return;
	}

	TArray<FString> Vars;
	SplitVarName(VarName, &Vars);
	TArray<FVarDescription> VarDescs;
	AnalyzeVarNames(Vars, &VarDescs);

	if (VarDescs.Num() == 0)
	{
		bIsNestedVarName = false;
	}
	else
	{
		bIsNestedVarName = !(VarDescs.Num() == 1 && VarDescs[0].VarType == EContainerType::None);

		FProperty* Property = GetTerminalProperty(VarDescs, 0, TargetClass);
		if (Property != nullptr)
		{
			const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

			FEdGraphPinType PinType;
			Schema->ConvertPropertyToPinType(Property, PinType);
			CreateResultPin(PinType, Property->GetName(), 0);
		}
	}
}

void UK2Node_GetVariableByNameNode::RecreateResultPin()
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
	RecreateResultPinInternal(TargetClass, VarName);

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

	if (TargetPin->DefaultObject != nullptr && TargetPin->LinkedTo.Num() == 0)
	{
		TargetClass = CastChecked<UClass>(TargetPin->DefaultObject);
	}
	else if (TargetPin->LinkedTo.Num() > 0)
	{
		UEdGraphPin* LinkedPin = TargetPin->LinkedTo[0];
		if (LinkedPin != nullptr)
		{
			if (LinkedPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Object &&
				LinkedPin->PinType.PinSubCategory == "self")
			{
				UEdGraphNode* OwningNode = LinkedPin->GetOwningNode();
				UK2Node_Self* SelfNode = CastChecked<UK2Node_Self>(OwningNode);
				UEdGraph* Graph = SelfNode->GetGraph();
				UObject* GraphOwner = Graph->GetOutermostObject();
				UBlueprint* Blueprint = Cast<UBlueprint>(GraphOwner);
				if (Blueprint != nullptr)
				{
					TargetClass = Blueprint->GeneratedClass;
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
		if (Blueprint->SkeletonGeneratedClass)
		{
			TargetClass = Blueprint->SkeletonGeneratedClass;
		}
	}

	return TargetClass;
}

UFunction* UK2Node_GetVariableByNameNode::FindGetterFunction(UEdGraphPin* Pin)
{
	FEdGraphPinType PinType = Pin->PinType;
	UClass* FunctionLibrary = UVariableGetterFunctionLibarary::StaticClass();

	if (bIsNestedVarName)
	{
		return FunctionLibrary->FindFunctionByName(FName("GetNestedVariableByName"));
	}

	switch (PinType.ContainerType)
	{
		case EPinContainerType::Array:
			return FunctionLibrary->FindFunctionByName(FName("GetArrayVariableByName"));
		case EPinContainerType::Set:
			return FunctionLibrary->FindFunctionByName(FName("GetSetVariableByName"));
		case EPinContainerType::Map:
			return FunctionLibrary->FindFunctionByName(FName("GetMapVariableByName"));
	}

	if (PinType.PinCategory == UEdGraphSchema_K2::PC_Boolean)
	{
		return FunctionLibrary->FindFunctionByName(FName("GetBooleanVariableByName"));
	}
	else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Byte)
	{
		if (PinType.PinSubCategoryObject == nullptr)
		{
			return FunctionLibrary->FindFunctionByName(FName("GetByteVariableByName"));
		}
		else
		{
			return FunctionLibrary->FindFunctionByName(FName("GetEnumVariableByName"));
		}
	}
	else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Class)
	{
		return FunctionLibrary->FindFunctionByName(FName("GetClassVariableByName"));
	}
	else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Int)
	{
		return FunctionLibrary->FindFunctionByName(FName("GetIntVariableByName"));
	}
	else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Int64)
	{
		return FunctionLibrary->FindFunctionByName(FName("GetInt64VariableByName"));
	}
	else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Float)
	{
		return FunctionLibrary->FindFunctionByName(FName("GetFloatVariableByName"));
	}
	else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Double)
	{
		return FunctionLibrary->FindFunctionByName(FName("GetFloat64VariableByName"));
	}
	else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Real)
	{
		if (PinType.PinSubCategory == "double")
		{
			return FunctionLibrary->FindFunctionByName(FName("GetFloat64VariableByName"));
		}
	}
	else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Name)
	{
		return FunctionLibrary->FindFunctionByName(FName("GetNameVariableByName"));
	}
	else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Object)
	{
		return FunctionLibrary->FindFunctionByName(FName("GetObjectVariableByName"));
	}
	else if (PinType.PinCategory == UEdGraphSchema_K2::PC_String)
	{
		return FunctionLibrary->FindFunctionByName(FName("GetStringVariableByName"));
	}
	else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Text)
	{
		return FunctionLibrary->FindFunctionByName(FName("GetTextVariableByName"));
	}
	else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Struct)
	{
		return FunctionLibrary->FindFunctionByName(FName("GetStructVariableByName"));
	}

	return nullptr;
}

bool UK2Node_GetVariableByNameNode::IsResultPin(const UEdGraphPin* Pin) const
{
	FString PinName = Pin->GetFName().ToString();

	FRegexPattern Pattern = FRegexPattern(FString::Format(TEXT("^{0}.*$"), {*ResultPinNamePrefix.ToString()}));
	FRegexMatcher Matcher(Pattern, PinName);
	if (Matcher.FindNext())
	{
		return true;
	}

	return false;
}

UEdGraphPin* UK2Node_GetVariableByNameNode::GetExecThenPin()
{
	return FindPinChecked(ExecThenPinName);
}

UEdGraphPin* UK2Node_GetVariableByNameNode::GetTargetPin()
{
	return FindPin(TargetPinName);
}

UEdGraphPin* UK2Node_GetVariableByNameNode::GetVarNamePin()
{
	return FindPinChecked(VarNamePinName);
}

UEdGraphPin* UK2Node_GetVariableByNameNode::GetSuccessPin()
{
	return FindPinChecked(SuccessPinName);
}

TArray<UEdGraphPin*> UK2Node_GetVariableByNameNode::GetAllResultPins()
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
