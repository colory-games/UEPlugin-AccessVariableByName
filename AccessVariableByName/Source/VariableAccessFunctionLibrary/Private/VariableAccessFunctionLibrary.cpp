/*!
 * AccessVariableByName
 *
 * Copyright (c) 2022 Colory Games
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "VariableAccessFunctionLibraryModule.h"

#define LOCTEXT_NAMESPACE "FVariableAccessFunctionLibrary"

void FVariableAccessFunctionLibraryModule::StartupModule()
{
}

void FVariableAccessFunctionLibraryModule::ShutdownModule()
{
}

bool FVariableAccessFunctionLibraryModule::SupportsDynamicReloading()
{
	return true;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FVariableAccessFunctionLibraryModule, VariableAccessFunctionLibrary);
