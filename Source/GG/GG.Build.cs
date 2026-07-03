// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GG : ModuleRules
{
	public GG(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			"GameplayAbilities",
			"GameplayTasks",
			"GameplayTags",
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"GG",
			"GG/Variant_Platforming",
			"GG/Variant_Platforming/Animation",
			"GG/Variant_Combat",
			"GG/Variant_Combat/AI",
			"GG/Variant_Combat/Animation",
			"GG/Variant_Combat/Gameplay",
			"GG/Variant_Combat/Interfaces",
			"GG/Variant_Combat/UI",
			"GG/Variant_SideScrolling",
			"GG/Variant_SideScrolling/AI",
			"GG/Variant_SideScrolling/Gameplay",
			"GG/Variant_SideScrolling/Interfaces",
			"GG/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
