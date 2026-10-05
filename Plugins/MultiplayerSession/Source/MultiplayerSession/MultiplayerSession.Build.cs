// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MultiplayerSession : ModuleRules
{
	public MultiplayerSession(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
			
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"OnlineSubsystem",
				"OnlineSubsystemSteam",
				"UMG",
				"Slate",
				"SlateCore"
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"OnlineBase",
				"OnlineSubsystemUtils",
				"Slate",
				"SlateCore",
			}
			);
	}
}
