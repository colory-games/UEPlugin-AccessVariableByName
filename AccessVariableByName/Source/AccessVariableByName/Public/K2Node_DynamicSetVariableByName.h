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
	// Override from UObject
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;

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
	virtual bool ShouldShowNodeProperties() const override
	{
		return true;
	}

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
	UEdGraphPin* GetExecThenPin() const;
	UEdGraphPin* GetTargetPin() const;
	UEdGraphPin* GetVarNamePin() const;
	UEdGraphPin* GetSuccessPin() const;
	TArray<UEdGraphPin*> GetAllNewValuePins() const;
	TArray<UEdGraphPin*> GetAllResultPins() const;

	UPROPERTY()
	FEdGraphPinType VariantPinType;

	// Include variables from a generation class (UBlueprint) if true.
	UPROPERTY(EditAnywhere, Category = "Access Variable Options")
	bool bIncludeGenerationClass = false;

	// Access float variable as a single precision float variable.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Access Variable Options")
	bool bSinglePrecision = false;

	// Create elements automatically if true when the element does not present.
	UPROPERTY(EditAnywhere, Category = "Container Type Access Options")
	bool bExtendIfNotPresent = false;

	friend class SGraphNodeDynamicSetVariableByNameNode;
};
