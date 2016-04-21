// Copyright 2016 Seasun Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
    public class NetcodeRobotTest : ModuleRules
    {
        public NetcodeRobotTest(TargetInfo Target)
        {
            PrivateIncludePaths.Add("NetcodeRobotTest/Private");

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
