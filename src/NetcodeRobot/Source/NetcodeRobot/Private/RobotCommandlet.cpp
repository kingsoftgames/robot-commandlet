// Copyright 2016 Seasun Games, Inc. All Rights Reserved.

#include "NetcodeRobotPCH.h"
#include "NRTUtilNet.h"

#include "RobotCommandlet.h"
#include "AssertionMacros.h"

// @todo JohnB: Fix StandaloneRenderer support for static builds
#if !IS_MONOLITHIC
#include "StandaloneRenderer.h"
#endif

#include "Engine/GameInstance.h"

#include "UnrealClient.h"
#include "SlateBasics.h"

UGameInstance* URobotCommandlet::GRobotGameInstance = nullptr;
URobotCommandlet* URobotCommandlet::GRobotCommandletInstance = nullptr;
FTimerHandle URobotCommandlet::TriggerTimerHandle;

FWorldDelegates::FWorldInitializationEvent::FDelegate URobotCommandlet::OnWorldCreatedDelegate = NULL;
FDelegateHandle URobotCommandlet::OnWorldCreatedDelegateHandle;

// @todo JohnB: If you later end up doing test client instances that are unit tested against client netcode
//              (same way as you do server instances at the moment, for testing server netcode),
//              then this commandlet code is probably an excellent base for setting up minimal standalone clients like that,
//              as it's intended to strip-down running unit tests, from a minimal client itself

// Enable access to the private UGameEngine.GameInstance value, using the GET_PRIVATE macro
IMPLEMENT_GET_PRIVATE_VAR(UGameEngine, GameInstance, UGameInstance*);

/**
 * URobotCommandlet
 */
URobotCommandlet::URobotCommandlet(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // Start off with GIsClient set to false, so we don't init viewport etc., but turn it on later for netcode
    IsClient = false;
    IsServer = false;
    IsEditor = false;

    LogToConsole = true;
    ShowErrorCount = true;

    GRobotCommandletInstance = this;
}

void URobotCommandlet::OnWorldCreated(UWorld* UnrealWorld, const UWorld::InitializationValues IVS)
{
    //
}

void URobotCommandlet::OnTimer()
{
    if (GRobotGameInstance)
    {
        UWorld* InWorld = GRobotGameInstance->GetWorld();

        if(InWorld)
            InWorld->OnLevelsLoadedMulticast.Broadcast();
    }
}

void URobotCommandlet::NotifyPostLoadMap()
{
    if (GRobotGameInstance)
    {
        //Wait for Robot program load the level's resource, and then broadcast the event.
        static const float GLevelsLoadedDelayTime = 3.0f;

        UWorld* InWorld = GRobotGameInstance->GetWorld();

        if (InWorld)
            InWorld->GetTimerManager().SetTimer(TriggerTimerHandle, GRobotCommandletInstance, &URobotCommandlet::OnTimer, GLevelsLoadedDelayTime);
    }
}

int32 URobotCommandlet::Main(const FString& Params)
{
#if IS_MONOLITHIC
    UE_LOG(LogRobot, Log, TEXT("NetcodeRobot commandlet not currently supported in static/monolithic builds"));
#else
	
    FString Token = "NetcodeRobot.RobotCommandlet";
    UClass* CommandletClass = FindObject<UClass>(ANY_PACKAGE, *Token, false);
    UCommandlet* Default = CastChecked<UCommandlet>(CommandletClass->GetDefaultObject());

    OnWorldCreatedDelegate = FWorldDelegates::FWorldInitializationEvent::FDelegate::CreateStatic(&URobotCommandlet::OnWorldCreated);
    OnWorldCreatedDelegateHandle = FWorldDelegates::OnPreWorldInitialization.Add(OnWorldCreatedDelegate);

    FCoreUObjectDelegates::PostLoadMap.AddStatic(&URobotCommandlet::NotifyPostLoadMap);

    GIsRequestingExit = false;
    GIsClient = true;

    // @todo JohnB: Steam detection doesn't seem to work this early on, but does work further down the line;
    //              try to find a way, to detect it as early as possible

    // NetcodeUnitTest is not compatible with Steam; if Steam is running/detected, abort immediately
    // @todo JohnB: Add support for Steam
    if (NRTNet::IsSteamNetDriverAvailable())
    {
        UE_LOG(LogRobot, Log, TEXT("NetcodeRobotTest does not currently support Steam. Close Steam before running."));
        GIsRequestingExit = true;
    }

    if (!GIsRequestingExit)
    {
        UWorld* RobotWorld = NULL;

        // Hack-set the engine GameViewport, so that setting GIsClient, doesn't cause an auto-exit
        // @todo JohnB: If you later remove the GIsClient setting code below, remove this as well
        if (GEngine->GameViewport == NULL)
        {
            GIsRunning = true;

            UGameEngine* GameEngine = Cast<UGameEngine>(GEngine);
            GRobotGameInstance = GET_PRIVATE(UGameEngine, GameEngine, GameInstance);

            if (GameEngine != NULL)
            {
                UGameViewportClient* NewViewport = NewObject<UGameViewportClient>(GameEngine);
                FWorldContext* CurContext = GRobotGameInstance->GetWorldContext();

                GameEngine->GameViewport = NewViewport;
                NewViewport->Init(*CurContext, GRobotGameInstance);

                // Set the internal FViewport, for the new game viewport, to avoid another bit of auto-exit code
                NewViewport->Viewport = new FDummyViewport(NewViewport);

                // Set the main engine world context game viewport, to match the newly created viewport, in order to prevent crashes
                CurContext->GameViewport = NewViewport;

                RobotWorld = GRobotGameInstance->GetWorldContext()->World();

                FNetworkNotifyRobotHook* RobotNotify = nullptr;
                UNetDriver* RobotNetDriver = nullptr;

                FUniqueNetIdRepl* InNetID = nullptr;
                FString ServerBeaconType;

                if (RobotNetDriver == NULL)
                {
                    RobotNetDriver = (UNetDriver*)NRTNet::CreateRobotNetDriver(RobotWorld);
                }

                FString Error;
                if (NewViewport->SetupInitialLocalPlayer(Error) == NULL)
                {
                    UE_LOG(LogRobot, Fatal, TEXT("%s"), *Error);

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
                            UE_LOG(LogRobot, Fatal, TEXT("Couldn't spawn player: %s"), *Error2);
                            return 0;
                        }
                    }
                }

                const TCHAR* ParamsRef = *Params;
                FString RobotServer = TEXT("127.0.0.1");
                FString RobotMap = TEXT("");

                FParse::Value(ParamsRef, TEXT("RobotServer="), RobotServer);
                FParse::Value(ParamsRef, TEXT("RobotMap="), RobotMap);

                if (RobotMap.IsEmpty())
                {
                    UE_LOG(LogRobot, Fatal, TEXT("Do not have \"-RobotTestMap=\" to indeicate the server map that you wanted test."));
                    return 0;
                }

                const UGameMapsSettings* GameMapsSettings = GetDefault<UGameMapsSettings>();
                GameMapsSettings->SetGameDefaultMap(RobotMap);

                APlayerController* PlayerController = GRobotGameInstance->GetFirstLocalPlayerController();

                if (PlayerController)
                    GRobotGameInstance->GetFirstLocalPlayerController()->ClientTravel(RobotServer, ETravelType::TRAVEL_Absolute);
            }
        }

        UWorld *CurWorld = NULL;

        // NOTE: This main loop is partly based off of FileServerCommandlet
        while (GIsRunning && !GIsRequestingExit)
        {
            if ((CurWorld = GRobotGameInstance->GetWorld()) != nullptr)
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
                GEngine->Exec(RobotWorld, *GEngine->DeferredCommands[DeferredCommandsIndex], *GLog);
            }

            GEngine->DeferredCommands.Empty();

            FPlatformProcess::Sleep(0);
        }

        // Cleanup the robot test world
        NRTNet::MarkRobotWorldForCleanup(RobotWorld, true);

        GIsRunning = false;
    }
#endif

    return 0;// (GWarn->Errors.Num() == 0 ? 0 : 1);
}

void URobotCommandlet::CreateCustomEngine(const FString& Params)
{
    //
}
