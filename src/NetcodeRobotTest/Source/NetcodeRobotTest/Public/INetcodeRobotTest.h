// Copyright 2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModuleManager.h"

class INetcodeRobotTest : public IModuleInterface
{
public:
    static inline INetcodeRobotTest& Get()
    {
        return FModuleManager::LoadModuleChecked<INetcodeRobotTest>("NetcodeRobotTest");
    }

    static inline bool IsAvailable()
    {
        return FModuleManager::Get().IsModuleLoaded("NetcodeRobotTest");
    }
};
