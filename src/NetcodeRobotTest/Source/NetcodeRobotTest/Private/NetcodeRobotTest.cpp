// Copyright 2016 Seasun Games, Inc. All Rights Reserved.

#include "NetcodeRobotTestPCH.h"

#include "INetcodeRobotTest.h"

ERobotLogType GActiveLogTypeFlags = ERobotLogType::None;

//#if !UE_BUILD_SHIPPING
//TMap<void*, InternalProcessEventCallback> ActiveRobotProcessEventCallbacks;
//#endif

/**
 * Definitions/implementations
 */

DEFINE_LOG_CATEGORY(LogRobotTest);

/**
 * Module implementation
 */
class FNetcodeRobotTest : public INetcodeRobotTest
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

FWorldDelegates::FWorldInitializationEvent::FDelegate FNetcodeRobotTest::OnWorldCreatedDelegate = NULL;

FDelegateHandle FNetcodeRobotTest::OnWorldCreatedDelegateHandle;

// Essential for getting the .dll to compile, and for the package to be loadable
IMPLEMENT_MODULE(FNetcodeRobotTest, NetcodeRobotTest);
