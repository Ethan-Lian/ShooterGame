using UnrealBuildTool;

public class MultiplayerSession : ModuleRules
{
	public MultiplayerSession(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core", "CoreUObject", "Engine", "DeveloperSettings", "OnlineSubsystem", "UMG"
		});
		PrivateDependencyModuleNames.AddRange(new[]
		{
			"OnlineBase", "OnlineSubsystemUtils", "Slate", "SlateCore"
		});
	}
}
