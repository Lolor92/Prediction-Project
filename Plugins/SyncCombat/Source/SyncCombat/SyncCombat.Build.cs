// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SyncCombat : ModuleRules
{
	public SyncCombat(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"GameplayAbilities",
				"GameplayTags",
				"GameplayTasks"
			});
	}
}
