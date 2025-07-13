// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class blackhole : ModuleRules
{
	public blackhole(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { 
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore", 
			"EnhancedInput",
			"UMG",
			"Slate",
			"SlateCore",
			"GameplayTags",
			"AIModule",
			"NavigationSystem"
		});

		// Specify the new directory structure
		PublicIncludePaths.AddRange(new string[] {
			"blackhole/Public"
		});

		PrivateIncludePaths.AddRange(new string[] {
			"blackhole/Private"
		});
	}
}
