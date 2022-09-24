/*!
 * AccessVariableByName
 *
 * Copyright (c) 2022 Colory Games
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#pragma once

#include "SGraphNodeDynamicSetVariableByNameNode.h"

#include "GraphEditorSettings.h"
#include "K2Node_DynamicSetVariableByName.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "NodeFactory.h"
#include "SPinTypeSelector.h"
#include "DetailLayoutBuilder.h"

void SGraphNodeDynamicSetVariableByNameNode::Construct(const FArguments& InArgs, UK2Node_DynamicSetVariableByNameNode* InNode)
{
	this->GraphNode = InNode;
	this->SetCursor(EMouseCursor::CardinalCross);
	this->UpdateGraphNode();
}

void SGraphNodeDynamicSetVariableByNameNode::CreatePinWidgets()
{
	auto K2Schema = GetDefault<UEdGraphSchema_K2>();

	for (auto It = GraphNode->Pins.CreateConstIterator(); It; ++It)
	{
		UEdGraphPin* Pin = *It;
		if (!Pin->bHidden)
		{
			TSharedPtr<SGraphPin> NewPin = FNodeFactory::CreatePinWidget(Pin);
			check(NewPin.IsValid());

			this->AddPin(NewPin.ToSharedRef());
		}
	}

	LeftNodeBox->AddSlot().AutoHeight()[
		SNew(SPinTypeSelector, FGetPinTypeTree::CreateUObject(K2Schema, &UEdGraphSchema_K2::GetVariableTypeTree))
			.Schema(K2Schema)
			.TargetPinType(this, &SGraphNodeDynamicSetVariableByNameNode::OnGetPinInfo)
			.OnPinTypePreChanged(this, &SGraphNodeDynamicSetVariableByNameNode::OnPrePinInfoChanged)
			.OnPinTypeChanged(this, &SGraphNodeDynamicSetVariableByNameNode::OnPinInfoChanged)
			.TypeTreeFilter(ETypeTreeFilter::None)
			.Font(IDetailLayoutBuilder::GetDetailFont())
	];
}

FEdGraphPinType SGraphNodeDynamicSetVariableByNameNode::OnGetPinInfo() const
{
	UK2Node_DynamicSetVariableByNameNode* Node = Cast<UK2Node_DynamicSetVariableByNameNode>(GraphNode);
	if (Node == nullptr)
	{
		return FEdGraphPinType();
	}

	return Node->GetVariantPinType();
}

void SGraphNodeDynamicSetVariableByNameNode::OnPrePinInfoChanged(const FEdGraphPinType& PinType)
{
}

void SGraphNodeDynamicSetVariableByNameNode::OnPinInfoChanged(const FEdGraphPinType& PinType)
{
	UK2Node_DynamicSetVariableByNameNode* Node = Cast<UK2Node_DynamicSetVariableByNameNode>(GraphNode);
	if (Node == nullptr)
	{
		return;
	}

	Node->ChangeVariantPinType(PinType);
}
