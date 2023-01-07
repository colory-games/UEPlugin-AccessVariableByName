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
#include "K2Node_CallFunction.h"
#include "K2Node_MakeStruct.h"

#include "K2Node_SetVariableByName.generated.h"

UCLASS(MinimalAPI)
class UK2Node_SetVariableByNameNode : public UK2Node
{
	GENERATED_BODY()

protected:
	// Override from UObject
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;

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
	virtual bool ShouldShowNodeProperties() const override
	{
		return true;
	}

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
	UK2Node_MakeStruct* CreateMakeStructNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph);
	UK2Node_CallFunction* CreateGetFunctionCallNode(
		FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph, UEdGraphPin* ResultPin);

public:
	UK2Node_SetVariableByNameNode(const FObjectInitializer& ObjectInitializer);

	UEdGraphPin* GetExecThenPin() const;
	UEdGraphPin* GetTargetPin() const;
	UEdGraphPin* GetVarNamePin() const;
	UEdGraphPin* GetSuccessPin() const;
	TArray<UEdGraphPin*> GetAllNewValuePins() const;
	TArray<UEdGraphPin*> GetAllResultPins() const;

	// Include variables from a generation class (UBlueprint) if true.
	UPROPERTY(EditAnywhere, Category = "Access Variable Options")
	bool bIncludeGenerationClass = false;

	// Create elements automatically if true when the element does not present.
	UPROPERTY(EditAnywhere, Category = "Container Type Access Options")
	bool bExtendIfNotPresent = false;
};
