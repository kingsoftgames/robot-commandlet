// Copyright 2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModuleManager.h"

class INetcodeRobot : public IModuleInterface
{
public:
    static inline INetcodeRobot& Get()
    {
        return FModuleManager::LoadModuleChecked<INetcodeRobot>("NetcodeRobot");
    }

    static inline bool IsAvailable()
    {
        return FModuleManager::Get().IsModuleLoaded("NetcodeRobot");
    }
};
