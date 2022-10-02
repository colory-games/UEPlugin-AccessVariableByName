/*!
 * AccessVariableByName
 *
 * Copyright (c) 2022 Colory Games
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#pragma once

#include "BlueprintActionDatabaseRegistrar.h"
#include "K2Node.h"

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
	void CreateNewValuePin(const FEdGraphPinType& PinType, FString PropertyName, int32 Index);
	void CreateSuccessPin();
	void CreateResultPin(const FEdGraphPinType& PinType, FString PropertyName, int32 Index);
	void RecreateVariantPinInternal(UClass* TargetClass, const FString& VarName);
	void RecreateVariantPin();
	UClass* GetTargetClass(UEdGraphPin* Pin = nullptr);
	UFunction* FindSetterFunction(UEdGraphPin* Pin);
	bool IsNewValuePin(const UEdGraphPin* Pin) const;
	bool IsResultPin(const UEdGraphPin* Pin) const;
	bool IsSupport(const UEdGraphPin* Pin) const;

public:
	UK2Node_SetVariableByNameNode(const FObjectInitializer& ObjectInitializer);

	UEdGraphPin* GetExecThenPin() const;
	UEdGraphPin* GetTargetPin() const;
	UEdGraphPin* GetVarNamePin() const;
	UEdGraphPin* GetSuccessPin() const;
	TArray<UEdGraphPin*> GetAllNewValuePins() const;
	TArray<UEdGraphPin*> GetAllResultPins() const;

	UPROPERTY()
	bool bIsNestedVarName = false;
};
