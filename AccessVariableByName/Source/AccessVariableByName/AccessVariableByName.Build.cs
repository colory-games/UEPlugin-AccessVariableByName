/*!
 * AccessVariableByName
 *
 * Copyright (c) 2022 Colory Games
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

using UnrealBuildTool;

public class AccessVariableByName : ModuleRules
{
	public AccessVariableByName(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]{
			"Core",
			"CoreUObject",
			"Engine",
		});

		PrivateDependencyModuleNames.AddRange(new string[]{
			"BlueprintGraph",
			"EditorStyle",
			"GraphEditor",
			"KismetCompiler",
			"KismetWidgets",
			"Slate",
			"SlateCore",
			"ToolMenus",
			"UnrealEd",
		});
	}
}
