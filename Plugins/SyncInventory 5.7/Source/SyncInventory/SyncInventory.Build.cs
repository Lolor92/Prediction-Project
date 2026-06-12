// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SyncInventory : ModuleRules
{
	public SyncInventory(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore",
				"NetCore",
				"GameplayTags",
				"UMG",
				"GameplayAbilities",
				"GameplayTasks"
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Slate",
				"SlateCore"
			}
			);
	}
}
