// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class VRComfortKit : ModuleRules
{
	public VRComfortKit(ReadOnlyTargetRules Target) : base(Target)
	{
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "EnhancedInput",
            "HeadMountedDisplay",
            "XRBase",
            "UMG",
            "SlateCore",
            "DeveloperSettings"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {

        });

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(new string[]
            {
                "UnrealEd",
            });
        }
    }
}
