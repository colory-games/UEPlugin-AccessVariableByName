/*!
 * AccessVariableByName
 *
 * Copyright (c) 2022 Colory Games
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "K2Node_SetVariableByName.h"

#include "BlueprintNodeSpawner.h"
#include "Common.h"
#include "EditorCategoryUtils.h"
#include "GraphEditorSettings.h"
#include "K2Node_CallFunction.h"
#include "KismetCompiler.h"
#include "VariableSetterFunctionLibrary.h"

#define LOCTEXT_NAMESPACE "K2Node"

const FName SetResultPinName(TEXT("Result"));
const FString SetResultPinFriendlyName(TEXT("Result"));

UK2Node_SetVariableByNameNode::UK2Node_SetVariableByNameNode(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

FText UK2Node_SetVariableByNameNode::GetMenuCategory() const
{
	return FEditorCategoryUtils::GetCommonCategory(FCommonEditorCategory::Variables);
}

void UK2Node_SetVariableByNameNode::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	UClass* ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);

		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

void UK2Node_SetVariableByNameNode::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	UEdGraphPin* ExecTriggeringPin = GetExecPin();
	UEdGraphPin* ExecThenPin = GetExecThenPin();
	UEdGraphPin* TargetPin = GetTargetPin();
	UEdGraphPin* VarNamePin = GetVarNamePin();
	UEdGraphPin* NewValuePin = GetNewValuePin();
	UEdGraphPin* SuccessPin = GetSuccessPin();
	UEdGraphPin* ResultPin = GetResultPin();

	if (ResultPin == nullptr || NewValuePin == nullptr)
	{
		return;
	}

	UFunction* SetterFunction = FindSetterFunction(ResultPin);
	if (SetterFunction == nullptr)
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("FunctionNotFound", "The function is not found.").ToString());
		return;
	}

	UK2Node_CallFunction* CallFunction = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	CallFunction->SetFromFunction(SetterFunction);
	CallFunction->AllocateDefaultPins();

	UEdGraphPin* FunctionExecTriggeringPin = CallFunction->GetExecPin();
	UEdGraphPin* FunctionExecThenPin = CallFunction->GetThenPin();
	UEdGraphPin* FunctionTargetPin = CallFunction->FindPinChecked(TEXT("Target"));
	UEdGraphPin* FunctionVarNamePin = CallFunction->FindPinChecked(TEXT("VarName"));
	UEdGraphPin* FunctionNewValuePin = CallFunction->FindPinChecked(TEXT("NewValue"));
	UEdGraphPin* FunctionSuccessPin = CallFunction->FindPinChecked(TEXT("Success"));
	UEdGraphPin* FunctionResultPin = CallFunction->FindPinChecked(TEXT("Result"));

	FunctionNewValuePin->PinType = NewValuePin->PinType;
	FunctionNewValuePin->PinType.PinSubCategory = NewValuePin->PinType.PinSubCategory;
	FunctionNewValuePin->PinType.PinSubCategoryObject = NewValuePin->PinType.PinSubCategoryObject;
	FunctionNewValuePin->PinType.PinValueType = NewValuePin->PinType.PinValueType;
	FunctionResultPin->PinType = ResultPin->PinType;
	FunctionResultPin->PinType.PinSubCategory = ResultPin->PinType.PinSubCategory;
	FunctionResultPin->PinType.PinSubCategoryObject = ResultPin->PinType.PinSubCategoryObject;
	FunctionResultPin->PinType.PinValueType = ResultPin->PinType.PinValueType;

	CompilerContext.MovePinLinksToIntermediate(*ExecTriggeringPin, *FunctionExecTriggeringPin);
	CompilerContext.MovePinLinksToIntermediate(*ExecThenPin, *FunctionExecThenPin);
	CompilerContext.MovePinLinksToIntermediate(*TargetPin, *FunctionTargetPin);
	CompilerContext.MovePinLinksToIntermediate(*VarNamePin, *FunctionVarNamePin);
	CompilerContext.MovePinLinksToIntermediate(*NewValuePin, *FunctionNewValuePin);
	CompilerContext.MovePinLinksToIntermediate(*SuccessPin, *FunctionSuccessPin);
	CompilerContext.MovePinLinksToIntermediate(*ResultPin, *FunctionResultPin);

	BreakAllNodeLinks();
}

void UK2Node_SetVariableByNameNode::AllocateDefaultPins()
{
	// Pin structure
	//   N: Number of case pin pair
	// -----
	// 0: Execution Triggering (In, Exec)
	// 1: Execution Then (Out, Exec)
	// 2: Target (In, Object Reference)
	// 3: Var Name (In, FName)
	// 4: New Value (In, *)
	// 5: Success (Out, Boolean)
	// 6: Result (Out, *)

	CreateExecTriggeringPin();
	CreateExecThenPin();
	CreateTargetPin();
	CreateVarNamePin();
	CreateSuccessPin();

	Super::AllocateDefaultPins();
}

void UK2Node_SetVariableByNameNode::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
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

	// TODO: check

	AllocateDefaultPins();

	UClass* TargetClass = GetTargetClass(OldTargetPin);
	if (TargetClass != nullptr)
	{
		FProperty* Property = FindFProperty<FProperty>(TargetClass, *OldVarNamePin->DefaultValue);
		if (Property != nullptr)
		{
			const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

			FEdGraphPinType PinType;
			Schema->ConvertPropertyToPinType(Property, PinType);
			if (PinType.PinCategory == UEdGraphSchema_K2::PC_Struct || PinType.PinCategory == UEdGraphSchema_K2::PC_Object ||
				(PinType.PinCategory == UEdGraphSchema_K2::PC_Byte && PinType.PinSubCategoryObject != nullptr))
			{
				CreateNewValuePin(
					PinType.PinCategory, PinType.PinSubCategoryObject.Get(), PinType.ContainerType, PinType.PinValueType);
				CreateResultPin(
					PinType.PinCategory, PinType.PinSubCategoryObject.Get(), PinType.ContainerType, PinType.PinValueType);
			}
			else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Real && PinType.PinSubCategory == "double")
			{
				CreateNewValuePin(PinType.PinCategory, PinType.PinSubCategory, PinType.ContainerType, PinType.PinValueType);
				CreateResultPin(PinType.PinCategory, PinType.PinSubCategory, PinType.ContainerType, PinType.PinValueType);
			}
			else
			{
				CreateNewValuePin(PinType.PinCategory, PinType.ContainerType, PinType.PinValueType);
				CreateResultPin(PinType.PinCategory, PinType.ContainerType, PinType.PinValueType);
			}
		}
	}

	RestoreSplitPins(OldPins);
}

FText UK2Node_SetVariableByNameNode::GetTooltipText() const
{
	return LOCTEXT("SetPropertyByNameNode_Tooltip", "Set Variable by Name");
}

FLinearColor UK2Node_SetVariableByNameNode::GetNodeTitleColor() const
{
	return GetDefault<UGraphEditorSettings>()->DefaultPinTypeColor;
}

FText UK2Node_SetVariableByNameNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("Set Variable by Name", "Set Variable by Name");
}

FSlateIcon UK2Node_SetVariableByNameNode::GetIconAndTint(FLinearColor& OutColor) const
{
	static FSlateIcon Icon("EditorStyle", "GraphEditor.Switch_16x");
	return Icon;
}

void UK2Node_SetVariableByNameNode::PinDefaultValueChanged(UEdGraphPin* Pin)
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
		UEdGraphPin* ResultPin = GetResultPin();
		if (ResultPin != nullptr)
		{
			ResultPin->BreakAllPinLinks();
		}
		RecreateResultPin();
	}
}

void UK2Node_SetVariableByNameNode::PinConnectionListChanged(UEdGraphPin* Pin)
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

void UK2Node_SetVariableByNameNode::CreateExecTriggeringPin()
{
	FCreatePinParams Params;
	Params.Index = 0;
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute, Params);
}

void UK2Node_SetVariableByNameNode::CreateExecThenPin()
{
	FCreatePinParams Params;
	Params.Index = 1;
	UEdGraphPin* DefaultExecPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, ExecThenPinName, Params);
	DefaultExecPin->PinFriendlyName = FText::AsCultureInvariant(ExecThenPinFriendlyName);
}

void UK2Node_SetVariableByNameNode::CreateTargetPin()
{
	FCreatePinParams Params;
	Params.Index = 2;
	UEdGraphPin* Pin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Object, UObject::StaticClass(), TargetPinName, Params);
	Pin->PinFriendlyName = FText::AsCultureInvariant(TargetPinFriendlyName);
}

void UK2Node_SetVariableByNameNode::CreateVarNamePin()
{
	FCreatePinParams Params;
	Params.Index = 3;
	UEdGraphPin* Pin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Name, VarNamePinName, Params);
	Pin->PinFriendlyName = FText::AsCultureInvariant(VarNamePinFriendlyName);
}

void UK2Node_SetVariableByNameNode::CreateNewValuePin(
	FName PinCategory, EPinContainerType PinContainerType, const FEdGraphTerminalType& PinValueType)
{
	FCreatePinParams Params;
	Params.Index = 4;
	Params.ContainerType = PinContainerType;
	Params.ValueTerminalType = PinValueType;
	UEdGraphPin* Pin = CreatePin(EGPD_Input, PinCategory, NewValuePinName, Params);
	Pin->PinFriendlyName = FText::AsCultureInvariant(NewValuePinFriendlyName);
}

void UK2Node_SetVariableByNameNode::CreateNewValuePin(
	FName PinCategory, FName PinSubCategory, EPinContainerType PinContainerType, const FEdGraphTerminalType& PinValueType)
{
	FCreatePinParams Params;
	Params.Index = 4;
	Params.ContainerType = PinContainerType;
	Params.ValueTerminalType = PinValueType;
	UEdGraphPin* Pin = CreatePin(EGPD_Input, PinCategory, PinSubCategory, NewValuePinName, Params);
	Pin->PinFriendlyName = FText::AsCultureInvariant(NewValuePinFriendlyName);
}

void UK2Node_SetVariableByNameNode::CreateNewValuePin(
	FName PinCategory, UObject* PinSubCategoryObject, EPinContainerType PinContainerType, const FEdGraphTerminalType& PinValueType)
{
	FCreatePinParams Params;
	Params.Index = 4;
	Params.ContainerType = PinContainerType;
	Params.ValueTerminalType = PinValueType;
	UEdGraphPin* Pin = CreatePin(EGPD_Input, PinCategory, PinSubCategoryObject, NewValuePinName, Params);
	Pin->PinFriendlyName = FText::AsCultureInvariant(NewValuePinFriendlyName);
}

void UK2Node_SetVariableByNameNode::CreateSuccessPin()
{
	FCreatePinParams Params;
	Params.Index = 5;
	UEdGraphPin* Pin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Boolean, SuccessPinName, Params);
	Pin->PinFriendlyName = FText::AsCultureInvariant(SuccessPinFriendlyName);
}

void UK2Node_SetVariableByNameNode::CreateResultPin(
	FName PinCategory, EPinContainerType PinContainerType, const FEdGraphTerminalType& PinValueType)
{
	FCreatePinParams Params;
	Params.Index = 6;
	Params.ContainerType = PinContainerType;
	Params.ValueTerminalType = PinValueType;
	UEdGraphPin* Pin = CreatePin(EGPD_Output, PinCategory, SetResultPinName, Params);
	Pin->PinFriendlyName = FText::AsCultureInvariant(SetResultPinFriendlyName);
}

void UK2Node_SetVariableByNameNode::CreateResultPin(
	FName PinCategory, FName PinSubCategory, EPinContainerType PinContainerType, const FEdGraphTerminalType& PinValueType)
{
	FCreatePinParams Params;
	Params.Index = 6;
	Params.ContainerType = PinContainerType;
	Params.ValueTerminalType = PinValueType;
	UEdGraphPin* Pin = CreatePin(EGPD_Output, PinCategory, PinSubCategory, SetResultPinName, Params);
	Pin->PinFriendlyName = FText::AsCultureInvariant(SetResultPinFriendlyName);
}

void UK2Node_SetVariableByNameNode::CreateResultPin(
	FName PinCategory, UObject* PinSubCategoryObject, EPinContainerType PinContainerType, const FEdGraphTerminalType& PinValueType)
{
	FCreatePinParams Params;
	Params.Index = 6;
	Params.ContainerType = PinContainerType;
	Params.ValueTerminalType = PinValueType;
	UEdGraphPin* Pin = CreatePin(EGPD_Output, PinCategory, PinSubCategoryObject, SetResultPinName, Params);
	Pin->PinFriendlyName = FText::AsCultureInvariant(SetResultPinFriendlyName);
}

void UK2Node_SetVariableByNameNode::RecreateResultPin()
{
	Modify();

	TArray<UEdGraphPin*> UnusedPins = MoveTemp(Pins);
	for (int32 Index = 0; Index < UnusedPins.Num(); ++Index)
	{
		UEdGraphPin* OldPin = UnusedPins[Index];
		if (OldPin->GetFName() != NewValuePinName && OldPin->GetFName() != SetResultPinName)
		{
			UnusedPins.RemoveAt(Index--, 1, false);
			Pins.Add(OldPin);
		}
	}

	// Create new result pin.
	UClass* TargetClass = GetTargetClass();
	FProperty* Property = FindFProperty<FProperty>(TargetClass, *GetVarNamePin()->DefaultValue);
	if (Property != nullptr)
	{
		// TODO: support input from independent variable
		const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

		FEdGraphPinType PinType;
		Schema->ConvertPropertyToPinType(Property, PinType);
		if (PinType.PinCategory == UEdGraphSchema_K2::PC_Struct || PinType.PinCategory == UEdGraphSchema_K2::PC_Object ||
			(PinType.PinCategory == UEdGraphSchema_K2::PC_Byte && PinType.PinSubCategoryObject != nullptr))
		{
			CreateNewValuePin(PinType.PinCategory, PinType.PinSubCategoryObject.Get(), PinType.ContainerType, PinType.PinValueType);
			CreateResultPin(PinType.PinCategory, PinType.PinSubCategoryObject.Get(), PinType.ContainerType, PinType.PinValueType);
		}
		else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Real && PinType.PinSubCategory == "double")
		{
			CreateNewValuePin(PinType.PinCategory, PinType.PinSubCategory, PinType.ContainerType, PinType.PinValueType);
			CreateResultPin(PinType.PinCategory, PinType.PinSubCategory, PinType.ContainerType, PinType.PinValueType);
		}
		else
		{
			CreateNewValuePin(PinType.PinCategory, PinType.ContainerType, PinType.PinValueType);
			CreateResultPin(PinType.PinCategory, PinType.ContainerType, PinType.PinValueType);
		}
	}

	// Restore connection.
	RestoreSplitPins(UnusedPins);
	RewireOldPinsToNewPins(UnusedPins, Pins, nullptr);

	UEdGraph* Graph = GetGraph();
	Graph->NotifyGraphChanged();
}

UClass* UK2Node_SetVariableByNameNode::GetTargetClass(UEdGraphPin* Pin)
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
			// TODO: Handle self node
			TargetClass = Cast<UClass>(LinkedPin->PinType.PinSubCategoryObject.Get());
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

UFunction* UK2Node_SetVariableByNameNode::FindSetterFunction(UEdGraphPin* Pin)
{
	FEdGraphPinType PinType = Pin->PinType;
	UClass* FunctionLibrary = UVariableSetterFunctionLibarary::StaticClass();

	switch (PinType.ContainerType)
	{
		case EPinContainerType::Array:
			return FunctionLibrary->FindFunctionByName(FName("SetArrayVariableByName"));
		case EPinContainerType::Set:
			return FunctionLibrary->FindFunctionByName(FName("SetSetVariableByName"));
		case EPinContainerType::Map:
			return FunctionLibrary->FindFunctionByName(FName("SetMapVariableByName"));
	}

	if (PinType.PinCategory == UEdGraphSchema_K2::PC_Boolean)
	{
		return FunctionLibrary->FindFunctionByName(FName("SetBooleanVariableByName"));
	}
	else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Byte)
	{
		if (PinType.PinSubCategoryObject == nullptr)
		{
			return FunctionLibrary->FindFunctionByName(FName("SetByteVariableByName"));
		}
		else
		{
			return FunctionLibrary->FindFunctionByName(FName("SetEnumVariableByName"));
		}
	}
	else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Class)
	{
		return FunctionLibrary->FindFunctionByName(FName("SetClassVariableByName"));
	}
	else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Int)
	{
		return FunctionLibrary->FindFunctionByName(FName("SetIntVariableByName"));
	}
	else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Int64)
	{
		return FunctionLibrary->FindFunctionByName(FName("SetInt64VariableByName"));
	}
	else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Float)
	{
		return FunctionLibrary->FindFunctionByName(FName("SetFloatVariableByName"));
	}
	else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Double)
	{
		return FunctionLibrary->FindFunctionByName(FName("SetFloat64VariableByName"));
	}
	else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Real)
	{
		if (PinType.PinSubCategory == "double")
		{
			return FunctionLibrary->FindFunctionByName(FName("SetFloat64VariableByName"));
		}
	}
	else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Name)
	{
		return FunctionLibrary->FindFunctionByName(FName("SetNameVariableByName"));
	}
	else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Object)
	{
		return FunctionLibrary->FindFunctionByName(FName("SetObjectVariableByName"));
	}
	else if (PinType.PinCategory == UEdGraphSchema_K2::PC_String)
	{
		return FunctionLibrary->FindFunctionByName(FName("SetStringVariableByName"));
	}
	else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Text)
	{
		return FunctionLibrary->FindFunctionByName(FName("SetTextVariableByName"));
	}
	else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Struct)
	{
		return FunctionLibrary->FindFunctionByName(FName("SetStructVariableByName"));
	}

	return nullptr;
}

UEdGraphPin* UK2Node_SetVariableByNameNode::GetExecThenPin()
{
	return FindPinChecked(ExecThenPinName);
}

UEdGraphPin* UK2Node_SetVariableByNameNode::GetTargetPin()
{
	return FindPin(TargetPinName);
}

UEdGraphPin* UK2Node_SetVariableByNameNode::GetVarNamePin()
{
	return FindPinChecked(VarNamePinName);
}

UEdGraphPin* UK2Node_SetVariableByNameNode::GetNewValuePin()
{
	return FindPin(NewValuePinName);
}

UEdGraphPin* UK2Node_SetVariableByNameNode::GetSuccessPin()
{
	return FindPinChecked(SuccessPinName);
}

UEdGraphPin* UK2Node_SetVariableByNameNode::GetResultPin()
{
	return FindPin(SetResultPinName);
}

#undef LOCTEXT_NAMESPACE
