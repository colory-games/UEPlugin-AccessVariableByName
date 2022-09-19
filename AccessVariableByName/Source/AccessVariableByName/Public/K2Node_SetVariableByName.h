/*!
 * AccessVariableByName
 *
 * Copyright (c) 2022 Colory Games
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#pragma once

#include "K2Node.h"
#include "BlueprintActionDatabaseRegistrar.h"

#include "K2Node_SetVariableByName.generated.h"


UCLASS(MinimalAPI)
class UK2Node_SetVariableByNameNode : public UK2Node
{
	GENERATED_BODY()

protected:
	// Override from UK2Node
	virtual FText GetMenuCategory() const override;
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual void ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;

	// Override from UEdGraphNode
	virtual void AllocateDefaultPins() override;
	virtual void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins) override;
	virtual FText GetTooltipText() const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	virtual void PinDefaultValueChanged(UEdGraphPin* Pin) override;
	virtual void PinConnectionListChanged(UEdGraphPin* Pin) override;

	// Internal
	void CreateExecTriggeringPin();
	void CreateExecThenPin();
	void CreateTargetPin();
	void CreateVarNamePin();
	void CreateNewValuePin(FName PinCategory, EPinContainerType PinContainerType, const FEdGraphTerminalType& PinValueType);
	void CreateNewValuePin(FName PinCategory, FName PinSubCategory, EPinContainerType PinContainerType, const FEdGraphTerminalType& PinValueType);
	void CreateNewValuePin(FName PinCategory, UObject* PinSubCategoryObject, EPinContainerType PinContainerType, const FEdGraphTerminalType& PinValueType);
	void CreateSuccessPin();
	void CreateResultPin(FName PinCategory, EPinContainerType PinContainerType, const FEdGraphTerminalType& PinValueType);
	void CreateResultPin(FName PinCategory, FName PinSubCategory, EPinContainerType PinContainerType, const FEdGraphTerminalType& PinValueType);
	void CreateResultPin(FName PinCategory, UObject* PinSubCategoryObject, EPinContainerType PinContainerType, const FEdGraphTerminalType& PinValueType);
	void RecreateResultPin();
	UClass* GetTargetClass(UEdGraphPin* Pin = nullptr);
	UFunction* FindSetterFunction(UEdGraphPin* Pin);

public:
	UK2Node_SetVariableByNameNode(const FObjectInitializer& ObjectInitializer);

	UEdGraphPin* GetExecThenPin();
	UEdGraphPin* GetTargetPin();
	UEdGraphPin* GetVarNamePin();
	UEdGraphPin* GetNewValuePin();
	UEdGraphPin* GetSuccessPin();
	UEdGraphPin* GetResultPin();
};
