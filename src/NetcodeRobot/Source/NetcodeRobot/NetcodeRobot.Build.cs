// Copyright 2016 Seasun Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
    public class NetcodeRobot : ModuleRules
    {
        public NetcodeRobot(TargetInfo Target)
        {
            PrivateIncludePaths.Add("NetcodeRobot/Private");

            PublicDependencyModuleNames.AddRange
            (
                new string[]
                {
                    "Core",
                    "CoreUObject",
                    "Engine",
                    "EngineSettings",
                    "OnlineSubsystem",
                    "OnlineSubsystemUtils",
                    "InputCore",
                    "Serialization",
                    "SlateCore",
                    "Slate"
                }
            );

            // @todo JohnB: Currently don't support standalone commandlet, with static builds (can't get past linker error in Win32)
            if (!Target.IsMonolithic)
            {
                PrivateDependencyModuleNames.AddRange
                (
                    new string[]
                    {
                        "StandaloneRenderer"
                    }
                );
            }
        }
    }
}
