// Copyright 2016 Seasun Games, Inc. All Rights Reserved.

#include "NetcodeRobotPCH.h"

#include "INetcodeRobot.h"

ERobotLogType GActiveLogTypeFlags = ERobotLogType::None;

//#if !UE_BUILD_SHIPPING
//TMap<void*, InternalProcessEventCallback> ActiveRobotProcessEventCallbacks;
//#endif

/**
 * Definitions/implementations
 */

DEFINE_LOG_CATEGORY(LogRobot);

/**
 * Module implementation
 */
class FNetcodeRobot : public INetcodeRobot
{
private:
    static FWorldDelegates::FWorldInitializationEvent::FDelegate OnWorldCreatedDelegate;
    static FDelegateHandle OnWorldCreatedDelegateHandle;

public:
    /**
     * Called upon loading of the NetcodeUnitTest library
     */
    virtual void StartupModule() override
    {
        static bool bSetDelegate = false;

        if (!bSetDelegate)
        {
            //
        }
    }

    /**
     * Called immediately prior to unloading of the NetcodeUnitTest library
     */
    virtual void ShutdownModule() override
    {
        //
    }
    
    /**
     * Delegate implementation, for adding NUTActor to ServerActors, early on in engine startup
     */
    static void OnWorldCreated(UWorld* UnrealWorld, const UWorld::InitializationValues IVS)
    {
        // If NUTActor isn't already in RuntimeServerActors, add it now
        if (GEngine != NULL)
        {
            //
        }

        // Now remove it, so it's only called once
        FWorldDelegates::OnPreWorldInitialization.Remove(OnWorldCreatedDelegateHandle);
    }
};

FWorldDelegates::FWorldInitializationEvent::FDelegate FNetcodeRobot::OnWorldCreatedDelegate = NULL;

FDelegateHandle FNetcodeRobot::OnWorldCreatedDelegateHandle;

// Essential for getting the .dll to compile, and for the package to be loadable
IMPLEMENT_MODULE(FNetcodeRobot, NetcodeRobot);
