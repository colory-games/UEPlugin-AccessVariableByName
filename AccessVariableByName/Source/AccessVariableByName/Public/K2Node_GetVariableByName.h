/*!
 * AccessVariableByName
 *
 * Copyright (c) 2022 Colory Games
 */

#pragma once

#include "BlueprintActionDatabaseRegistrar.h"
#include "K2Node.h"

#include "K2Node_GetVariableByName.generated.h"

UCLASS(MinimalAPI)
class UK2Node_GetVariableByNameNode : public UK2Node
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
	void CreateSuccessPin();
	void CreateResultPin(const FEdGraphPinType& PinType, FString PropertyName, int32 Index);
	void RecreateResultPin();
	void RecreateResultPinInternal(UClass* TargetClass, const FString& VarName);
	UClass* GetTargetClass(UEdGraphPin* Pin = nullptr);
	UFunction* FindGetterFunction(UEdGraphPin* Pin);
	bool IsResultPin(const UEdGraphPin* Pin) const;
	bool IsSupport(const UEdGraphPin* Pin) const;

public:
	UK2Node_GetVariableByNameNode(const FObjectInitializer& ObjectInitializer);

	UEdGraphPin* GetExecThenPin();
	UEdGraphPin* GetTargetPin();
	UEdGraphPin* GetVarNamePin();
	UEdGraphPin* GetSuccessPin();
	TArray<UEdGraphPin*> GetAllResultPins();

	UPROPERTY()
	bool bIsNestedVarName = false;
};
