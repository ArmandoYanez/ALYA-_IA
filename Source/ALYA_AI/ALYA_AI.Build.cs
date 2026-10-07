// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ALYA_AI : ModuleRules
{
	public ALYA_AI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"NavigationSystem",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"OnlineSubsystem",
			"OnlineSubsystemUtils",
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"ALYA_AI",
			"ALYA_AI/Variant_Horror",
			"ALYA_AI/Variant_Horror/UI",
			"ALYA_AI/Variant_Shooter",
			"ALYA_AI/Variant_Shooter/AI",
			"ALYA_AI/Variant_Shooter/UI",
			"ALYA_AI/Variant_Shooter/Weapons"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
