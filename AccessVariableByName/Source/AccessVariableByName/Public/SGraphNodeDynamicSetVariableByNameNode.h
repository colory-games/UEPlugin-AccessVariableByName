/*!
 * AccessVariableByName
 *
 * Copyright (c) 2022 Colory Games
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#pragma once

#include "KismetNodes/SGraphNodeK2Base.h"

class UK2Node_DynamicSetVariableByNameNode;

class SGraphNodeDynamicSetVariableByNameNode : public SGraphNodeK2Base
{
	SLATE_BEGIN_ARGS(SGraphNodeDynamicSetVariableByNameNode)
	{
	}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UK2Node_DynamicSetVariableByNameNode* InNode);

	virtual void CreatePinWidgets() override;

protected:
	FEdGraphPinType OnGetPinInfo() const;
	void OnPrePinInfoChanged(const FEdGraphPinType& PinType);
	void OnPinInfoChanged(const FEdGraphPinType& PinType);
	ECheckBoxState OnGetSinglePrecision() const;
	void OnGetSinglePrecisionChanged(ECheckBoxState CheckState);
};
