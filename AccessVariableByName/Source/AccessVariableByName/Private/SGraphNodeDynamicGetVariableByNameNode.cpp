/*!
 * AccessVariableByName
 *
 * Copyright (c) 2022 Colory Games
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#pragma once

#include "SGraphNodeDynamicGetVariableByNameNode.h"

#include "GraphEditorSettings.h"
#include "K2Node_DynamicGetVariableByName.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "NodeFactory.h"
#include "SPinTypeSelector.h"
#include "DetailLayoutBuilder.h"
#include "Common.h"

void SGraphNodeDynamicGetVariableByNameNode::Construct(const FArguments& InArgs, UK2Node_DynamicGetVariableByNameNode* InNode)
{
	this->GraphNode = InNode;
	this->SetCursor(EMouseCursor::CardinalCross);
	this->UpdateGraphNode();
}

void SGraphNodeDynamicGetVariableByNameNode::CreatePinWidgets()
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

	LeftNodeBox->AddSlot()
		.AutoHeight()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.Padding(5.0f, 8.0f, 8.0f, 5.0f)[SNew(SImage).Image(FEditorStyle::GetBrush("Graph.Pin.DefaultPinSeparator"))];

	LeftNodeBox->AddSlot()
		.AutoHeight()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.Padding(10.0f, 8.0f, 8.0f, 10.0f)[SNew(SVerticalBox) +
					SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Left)
						.VAlign(VAlign_Center)
						.Padding(0.0f, 2.0f)[SNew(STextBlock).Text(FText::FromString("Variable Type"))] +
					SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Left)
						.VAlign(VAlign_Center)
						.Padding(0.0f, 2.0f)[SNew(SPinTypeSelector, FGetPinTypeTree::CreateUObject(K2Schema, &UEdGraphSchema_K2::GetVariableTypeTree))
								.Schema(K2Schema)
								.TargetPinType(this, &SGraphNodeDynamicGetVariableByNameNode::OnGetPinInfo)
								.OnPinTypePreChanged(this, &SGraphNodeDynamicGetVariableByNameNode::OnPrePinInfoChanged)
								.OnPinTypeChanged(this, &SGraphNodeDynamicGetVariableByNameNode::OnPinInfoChanged)
								.TypeTreeFilter(ETypeTreeFilter::None)
								.Font(IDetailLayoutBuilder::GetDetailFont())
						]
					];
}

FEdGraphPinType SGraphNodeDynamicGetVariableByNameNode::OnGetPinInfo() const
{
	UK2Node_DynamicGetVariableByNameNode* Node = Cast<UK2Node_DynamicGetVariableByNameNode>(GraphNode);
	if (Node == nullptr)
	{
		return CreateDefaultPinType();
	}

	return Node->GetResultPinType();
}

void SGraphNodeDynamicGetVariableByNameNode::OnPrePinInfoChanged(const FEdGraphPinType& PinType)
{
}

void SGraphNodeDynamicGetVariableByNameNode::OnPinInfoChanged(const FEdGraphPinType& PinType)
{
	UK2Node_DynamicGetVariableByNameNode* Node = Cast<UK2Node_DynamicGetVariableByNameNode>(GraphNode);
	if (Node == nullptr)
	{
		return;
	}

	Node->ChangeResultPinType(PinType);
}
