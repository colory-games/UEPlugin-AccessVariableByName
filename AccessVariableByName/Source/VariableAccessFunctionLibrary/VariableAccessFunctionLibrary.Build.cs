/*!
 * AccessVariableByName
 *
 * Copyright (c) 2022 Colory Games
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

using UnrealBuildTool;

public class VariableAccessFunctionLibrary : ModuleRules
{
	public VariableAccessFunctionLibrary(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]{
			"Core",
			"CoreUObject",
			"Engine",
		});

		PrivateDependencyModuleNames.AddRange(new string[]{});

		// @remove-start FULL_VERSION=true
		PublicDefinitions.Add("AVBN_FREE_VERSION");
		// @remove-end
	}
}
