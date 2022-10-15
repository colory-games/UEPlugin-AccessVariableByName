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

#include "K2Node_DynamicSetVariableByName.generated.h"

UCLASS(MinimalAPI)
class UK2Node_DynamicSetVariableByNameNode : public UK2Node
{
	GENERATED_BODY()

protected:
	// Override from UK2Node
	virtual FText GetMenuCategory() const override;
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual FNodeHandlingFunctor* CreateNodeHandler(FKismetCompilerContext& CompilerContext) const override;

	// Override from UEdGraphNode
	virtual void AllocateDefaultPins() override;
	virtual void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins) override;
	virtual FText GetTooltipText() const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;

	// Internal
	void CreateFunctionPin();
	void CreateExecTriggeringPin();
	void CreateExecThenPin();
	void CreateTargetPin();
	void CreateVarNamePin();
	void CreateSuccessPin();
	void CreateNewValuePin(const FEdGraphPinType& PinType, int32 Index);
	void CreateResultPin(const FEdGraphPinType& PinType, int32 Index);
	bool IsNewValuePin(const UEdGraphPin* Pin) const;
	bool IsResultPin(const UEdGraphPin* Pin) const;
	void RecreateVariantPins(const FEdGraphPinType& PinType);

protected:
	TSubclassOf<class UObject> InternalCallFuncClass;
	FName InternalCallFuncName;

public:
	UK2Node_DynamicSetVariableByNameNode(const FObjectInitializer& ObjectInitializer);

	UEdGraphPin* GetFunctionPin() const;
	UEdGraphPin* GetExecTriggeringPin() const;
	UEdGraphPin* GetExecThenPin() const;
	UEdGraphPin* GetTargetPin() const;
	UEdGraphPin* GetVarNamePin() const;
	UEdGraphPin* GetSuccessPin() const;
	TArray<UEdGraphPin*> GetAllNewValuePins() const;
	TArray<UEdGraphPin*> GetAllResultPins() const;

	void ChangeVariantPinType(const FEdGraphPinType& PinType);
	FEdGraphPinType GetVariantPinType() const;
};
