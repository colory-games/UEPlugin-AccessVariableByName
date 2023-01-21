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

#include "AccessVariableByNameUtils.h"
#include "DetailLayoutBuilder.h"
#include "EditorStyleSet.h"
#include "GraphEditorSettings.h"
#include "K2Node_DynamicGetVariableByName.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "NodeFactory.h"
#include "SPinTypeSelector.h"

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

	// clang-format off
	LeftNodeBox->AddSlot()
		.AutoHeight()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.Padding(5.0f, 8.0f, 8.0f, 5.0f)
		[
			SNew(SImage).Image(FEditorStyle::GetBrush("Graph.Pin.DefaultPinSeparator"))
		];

	LeftNodeBox->AddSlot()
		.AutoHeight()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.Padding(10.0f, 8.0f, 8.0f, 10.0f)
		[
			SNew(SVerticalBox) +
				SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Center)
					.Padding(0.0f, 2.0f)
					[
						SNew(STextBlock).Text(FText::FromString("Variable Type"))
					] +
				SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Center)
					.Padding(0.0f, 2.0f)
					[
						SNew(SPinTypeSelector, FGetPinTypeTree::CreateUObject(K2Schema, &UEdGraphSchema_K2::GetVariableTypeTree))
							.Schema(K2Schema)
							.TargetPinType(this, &SGraphNodeDynamicGetVariableByNameNode::OnGetPinInfo)
							.OnPinTypePreChanged(this, &SGraphNodeDynamicGetVariableByNameNode::OnPrePinInfoChanged)
							.OnPinTypeChanged(this, &SGraphNodeDynamicGetVariableByNameNode::OnPinInfoChanged)
							.TypeTreeFilter(ETypeTreeFilter::None)
							.Font(IDetailLayoutBuilder::GetDetailFont())
					]
		];
	
	if (OnGetPinInfo().PinCategory == UEdGraphSchema_K2::PC_Real)
	{
		LeftNodeBox->AddSlot()
			.AutoHeight()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			.Padding(10.0, 4.0f, 8.0f, 10.0f)
			[
				SNew(SCheckBox)
					.IsChecked(this, &SGraphNodeDynamicGetVariableByNameNode::OnGetSinglePrecision)
					.OnCheckStateChanged(this, &SGraphNodeDynamicGetVariableByNameNode::OnGetSinglePrecisionChanged)
					[
						SNew(STextBlock)
							.Font(IDetailLayoutBuilder::GetDetailFont())
							.Text(FText::FromString("Single Precision"))
					]
			];
	}
	// clang-format on
}

FEdGraphPinType SGraphNodeDynamicGetVariableByNameNode::OnGetPinInfo() const
{
	UK2Node_DynamicGetVariableByNameNode* Node = Cast<UK2Node_DynamicGetVariableByNameNode>(GraphNode);
	if (Node == nullptr)
	{
		return CreateDefaultPinType();
	}

	return Node->VariantPinType;
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

	Node->VariantPinType = PinType;

	FProperty* ChangedProperty = Node->StaticClass()->FindPropertyByName("VariantPinType");
	FPropertyChangedEvent Event(ChangedProperty, EPropertyChangeType::ValueSet);
	Node->PostEditChangeProperty(Event);
}

ECheckBoxState SGraphNodeDynamicGetVariableByNameNode::OnGetSinglePrecision() const
{
	UK2Node_DynamicGetVariableByNameNode* Node = Cast<UK2Node_DynamicGetVariableByNameNode>(GraphNode);
	if (Node == nullptr)
	{
		return ECheckBoxState::Unchecked;
	}

	return Node->bSinglePrecision ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SGraphNodeDynamicGetVariableByNameNode::OnGetSinglePrecisionChanged(ECheckBoxState CheckState)
{
	UK2Node_DynamicGetVariableByNameNode* Node = Cast<UK2Node_DynamicGetVariableByNameNode>(GraphNode);
	if (Node == nullptr)
	{
		return;
	}

	if (CheckState == ECheckBoxState::Checked)
	{
		Node->bSinglePrecision = true;
	}
	else if (CheckState == ECheckBoxState::Unchecked)
	{
		Node->bSinglePrecision = false;
	}

	FProperty* ChangedProperty = Node->StaticClass()->FindPropertyByName("bSinglePrecision");
	FPropertyChangedEvent Event(ChangedProperty, EPropertyChangeType::ValueSet);
	Node->PostEditChangeProperty(Event);
}