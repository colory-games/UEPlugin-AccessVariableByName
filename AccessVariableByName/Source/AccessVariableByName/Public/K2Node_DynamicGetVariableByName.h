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
#include "VariableAccessFunctionLibraryUtils.h"

#include "K2Node_DynamicGetVariableByName.generated.h"

UCLASS(MinimalAPI)
class UK2Node_DynamicGetVariableByNameNode : public UK2Node
{
	GENERATED_BODY()

protected:
	// Override from UObject
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;

	// Override from UK2Node
	virtual FText GetMenuCategory() const override;
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual FNodeHandlingFunctor* CreateNodeHandler(FKismetCompilerContext& CompilerContext) const override;
	virtual bool IsNodePure() const override
	{
		return bPureNode;
	}

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
	void CreateResultPin(const FEdGraphPinType& PinType, int32 Index);
	bool IsResultPin(const UEdGraphPin* Pin) const;
	void RecreateVariantPins(const FEdGraphPinType& PinType);

protected:
	TSubclassOf<class UObject> InternalCallFuncClass;
	FName InternalCallFuncName;

public:
	UK2Node_DynamicGetVariableByNameNode(const FObjectInitializer& ObjectInitializer);

	UEdGraphPin* GetFunctionPin() const;
	UEdGraphPin* GetExecThenPin() const;
	UEdGraphPin* GetTargetPin() const;
	UEdGraphPin* GetVarNamePin() const;
	UEdGraphPin* GetSuccessPin() const;
	TArray<UEdGraphPin*> GetAllResultPins() const;

	UPROPERTY()
	FEdGraphPinType VariantPinType;

	// Make node a pure node if true.
	UPROPERTY(EditAnywhere, Category = "Node Options")
	bool bPureNode = true;

	// Access float variable as a single precision float variable.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Access Variable Options")
	bool bSinglePrecision = false;

	// Include variables from a generation class (UBlueprint) if true.
	UPROPERTY(EditAnywhere, Category = "Access Variable Options")
	bool bIncludeGenerationClass = false;

	friend class SGraphNodeDynamicGetVariableByNameNode;
};
