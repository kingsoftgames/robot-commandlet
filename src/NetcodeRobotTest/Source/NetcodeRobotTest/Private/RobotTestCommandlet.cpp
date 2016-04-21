// Copyright 2016 Seasun Games, Inc. All Rights Reserved.

#include "NetcodeRobotTestPCH.h"
#include "NRTUtilNet.h"

#include "RobotTestCommandlet.h"
#include "AssertionMacros.h"

// @todo JohnB: Fix StandaloneRenderer support for static builds
#if !IS_MONOLITHIC
#include "StandaloneRenderer.h"
#endif

#include "Engine/GameInstance.h"

#include "UnrealClient.h"
#include "SlateBasics.h"

UGameInstance* URobotTestCommandlet::GRobotTestGameInstance = nullptr;
URobotTestCommandlet* URobotTestCommandlet::GRobotTestCommandletInstance = nullptr;
FTimerHandle URobotTestCommandlet::TriggerTimerHandle;

FWorldDelegates::FWorldInitializationEvent::FDelegate URobotTestCommandlet::OnWorldCreatedDelegate = NULL;
FDelegateHandle URobotTestCommandlet::OnWorldCreatedDelegateHandle;

// @todo JohnB: If you later end up doing test client instances that are unit tested against client netcode
//              (same way as you do server instances at the moment, for testing server netcode),
//              then this commandlet code is probably an excellent base for setting up minimal standalone clients like that,
//              as it's intended to strip-down running unit tests, from a minimal client itself

// Enable access to the private UGameEngine.GameInstance value, using the GET_PRIVATE macro
IMPLEMENT_GET_PRIVATE_VAR(UGameEngine, GameInstance, UGameInstance*);

/**
 * URobotTestCommandlet
 */
URobotTestCommandlet::URobotTestCommandlet(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // Start off with GIsClient set to false, so we don't init viewport etc., but turn it on later for netcode
    IsClient = false;
    IsServer = false;
    IsEditor = false;

    LogToConsole = true;
    ShowErrorCount = true;

    GRobotTestCommandletInstance = this;
}

void URobotTestCommandlet::OnWorldCreated(UWorld* UnrealWorld, const UWorld::InitializationValues IVS)
{
    //
}

void URobotTestCommandlet::OnTimer()
{
    if (GRobotTestGameInstance)
    {
        UWorld* InWorld = GRobotTestGameInstance->GetWorld();

        if(InWorld)
            InWorld->OnLevelsLoadedMulticast.Broadcast();
    }
}

void URobotTestCommandlet::NotifyPostLoadMap()
{
    if (GRobotTestGameInstance)
    {
        //Wait for Robot program load the level's resource, and then broadcast the event.
        static const float GLevelsLoadedDelayTime = 3.0f;

        UWorld* InWorld = GRobotTestGameInstance->GetWorld();

        if (InWorld)
            InWorld->GetTimerManager().SetTimer(TriggerTimerHandle, GRobotTestCommandletInstance, &URobotTestCommandlet::OnTimer, GLevelsLoadedDelayTime);
    }
}

int32 URobotTestCommandlet::Main(const FString& Params)
{
#if IS_MONOLITHIC
    UE_LOG(LogRobotTest, Log, TEXT("NetcodeRobotTest commandlet not currently supported in static/monolithic builds"));
#else
	
    FString Token = "NetcodeRobotTest.RobotTestCommandlet";
    UClass* CommandletClass = FindObject<UClass>(ANY_PACKAGE, *Token, false);
    UCommandlet* Default = CastChecked<UCommandlet>(CommandletClass->GetDefaultObject());

    OnWorldCreatedDelegate = FWorldDelegates::FWorldInitializationEvent::FDelegate::CreateStatic(&URobotTestCommandlet::OnWorldCreated);
    OnWorldCreatedDelegateHandle = FWorldDelegates::OnPreWorldInitialization.Add(OnWorldCreatedDelegate);

    FCoreUObjectDelegates::PostLoadMap.AddStatic(&URobotTestCommandlet::NotifyPostLoadMap);

    GIsRequestingExit = false;
    GIsClient = true;

    // @todo JohnB: Steam detection doesn't seem to work this early on, but does work further down the line;
    //              try to find a way, to detect it as early as possible

    // NetcodeUnitTest is not compatible with Steam; if Steam is running/detected, abort immediately
    // @todo JohnB: Add support for Steam
    if (NRTNet::IsSteamNetDriverAvailable())
    {
        UE_LOG(LogRobotTest, Log, TEXT("NetcodeRobotTest does not currently support Steam. Close Steam before running."));
        GIsRequestingExit = true;
    }

    if (!GIsRequestingExit)
    {
        UWorld* RobotTestWorld = NULL;

        // Hack-set the engine GameViewport, so that setting GIsClient, doesn't cause an auto-exit
        // @todo JohnB: If you later remove the GIsClient setting code below, remove this as well
        if (GEngine->GameViewport == NULL)
        {
            GIsRunning = true;

            UGameEngine* GameEngine = Cast<UGameEngine>(GEngine);
            GRobotTestGameInstance = GET_PRIVATE(UGameEngine, GameEngine, GameInstance);

            if (GameEngine != NULL)
            {
                UGameViewportClient* NewViewport = NewObject<UGameViewportClient>(GameEngine);
                FWorldContext* CurContext = GRobotTestGameInstance->GetWorldContext();

                GameEngine->GameViewport = NewViewport;
                NewViewport->Init(*CurContext, GRobotTestGameInstance);

                // Set the internal FViewport, for the new game viewport, to avoid another bit of auto-exit code
                NewViewport->Viewport = new FDummyViewport(NewViewport);

                // Set the main engine world context game viewport, to match the newly created viewport, in order to prevent crashes
                CurContext->GameViewport = NewViewport;

                RobotTestWorld = GRobotTestGameInstance->GetWorldContext()->World();

                FNetworkNotifyRobotHook* RobotNotify = nullptr;
                UNetDriver* RobotNetDriver = nullptr;

                FUniqueNetIdRepl* InNetID = nullptr;
                FString ServerBeaconType;

                if (RobotNetDriver == NULL)
                {
                    RobotNetDriver = (UNetDriver*)NRTNet::CreateRobotTestNetDriver(RobotTestWorld);
                }

                FString Error;
                if (NewViewport->SetupInitialLocalPlayer(Error) == NULL)
                {
                    UE_LOG(LogRobotTest, Fatal, TEXT("%s"), *Error);

                    return 0;
                }

                // Spawn play actors for client travel
                if (CurContext->OwningGameInstance != NULL)
				{
                    for (auto It = CurContext->OwningGameInstance->GetLocalPlayerIterator(); It; ++It)
                    {
                        FString Error2,URL;

                        if (!(*It)->SpawnPlayActor(URL, Error2, CurContext->World()))
                        {
                            UE_LOG(LogRobotTest, Fatal, TEXT("Couldn't spawn player: %s"), *Error2);
                            return 0;
                        }
                    }
                }

                const TCHAR* ParamsRef = *Params;
                FString RobotTestServer = TEXT("127.0.0.1");
                FString RobotTestMap = TEXT("");

                FParse::Value(ParamsRef, TEXT("RobotTestServer="), RobotTestServer);
                FParse::Value(ParamsRef, TEXT("RobotTestMap="), RobotTestMap);

                if (RobotTestMap.IsEmpty())
                {
                    UE_LOG(LogRobotTest, Fatal, TEXT("Do not have \"-RobotTestMap=\" to indeicate the server map that you wanted test."));
                    return 0;
                }

                const UGameMapsSettings* GameMapsSettings = GetDefault<UGameMapsSettings>();
                GameMapsSettings->SetGameDefaultMap(RobotTestMap);

                APlayerController* PlayerController = GRobotTestGameInstance->GetFirstLocalPlayerController();

                if (PlayerController)
                    GRobotTestGameInstance->GetFirstLocalPlayerController()->ClientTravel(RobotTestServer, ETravelType::TRAVEL_Absolute);
            }
        }

        UWorld *CurWorld = NULL;

        // NOTE: This main loop is partly based off of FileServerCommandlet
        while (GIsRunning && !GIsRequestingExit)
        {
            if ((CurWorld = GRobotTestGameInstance->GetWorld()) != nullptr)
            {
                //To make sure the current level resource will be loaded.
                CurWorld->FlushLevelStreaming(EFlushLevelStreamingType::Full);
            }

            GEngine->UpdateTimeAndHandleMaxTickRate();
            GEngine->Tick(FApp::GetDeltaTime(), false);

            if (FSlateApplication::IsInitialized())
            {
                FSlateApplication::Get().PumpMessages();
                FSlateApplication::Get().Tick();
            }

            // Execute deferred commands
            for (int32 DeferredCommandsIndex=0; DeferredCommandsIndex<GEngine->DeferredCommands.Num(); DeferredCommandsIndex++)
            {
                GEngine->Exec(RobotTestWorld, *GEngine->DeferredCommands[DeferredCommandsIndex], *GLog);
            }

            GEngine->DeferredCommands.Empty();

            FPlatformProcess::Sleep(0);
        }

        // Cleanup the robot test world
        NRTNet::MarkRobotTestWorldForCleanup(RobotTestWorld, true);

        GIsRunning = false;
    }
#endif

    return 0;// (GWarn->Errors.Num() == 0 ? 0 : 1);
}

void URobotTestCommandlet::CreateCustomEngine(const FString& Params)
{
    //
}
