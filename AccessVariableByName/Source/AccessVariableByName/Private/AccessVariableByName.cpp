/*!
 * AccessVariableByName
 *
 * Copyright (c) 2022 Colory Games
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "AccessVariableByNameModule.h"

#include "EdGraphUtilities.h"

#define LOCTEXT_NAMESPACE "FAccessVariableByNameModule"

class FGraphPanelNodeFactory_AccessVariableByName : public FGraphPanelNodeFactory
{
	virtual TSharedPtr<SGraphNode> CreateNode(UEdGraphNode* Node) const override
	{
		return nullptr;
	}
};

void FAccessVariableByNameModule::StartupModule()
{
	GraphPanelNodeFactory_AccessVariableByName = MakeShareable(new FGraphPanelNodeFactory_AccessVariableByName());
	FEdGraphUtilities::RegisterVisualNodeFactory(GraphPanelNodeFactory_AccessVariableByName);
}

void FAccessVariableByNameModule::ShutdownModule()
{
	if (GraphPanelNodeFactory_AccessVariableByName.IsValid())
	{
		FEdGraphUtilities::UnregisterVisualNodeFactory(GraphPanelNodeFactory_AccessVariableByName);
		GraphPanelNodeFactory_AccessVariableByName.Reset();
	}
}

bool FAccessVariableByNameModule::SupportsDynamicReloading()
{
	return true;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAccessVariableByNameModule, AccessVariableByName);
