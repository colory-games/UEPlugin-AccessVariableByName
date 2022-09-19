/*!
 * AccessVariableByName
 *
 * Copyright (c) 2022 Colory Games
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#pragma once

#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "FAccessVariableByNameModule"

class FGraphPanelNodeFactory_AccessVariableByName;

class FAccessVariableByNameModule : public IModuleInterface
{
	TSharedPtr<FGraphPanelNodeFactory_AccessVariableByName> GraphPanelNodeFactory_AccessVariableByName;

public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	virtual bool SupportsDynamicReloading() override;
};

#undef LOCTEXT_NAMESPACE
