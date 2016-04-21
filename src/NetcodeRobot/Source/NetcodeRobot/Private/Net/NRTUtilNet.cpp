#include "NetcodeRobotPCH.h"
#include "NRTUtil.h"
#include "Net/RobotNetDriver.h"
#include "Net/NRTUtilNet.h"
#include "UnrealNetwork.h"
#include "EngineVersion.h"
#include "DataChannel.h"

// Forward declarations
class FWorldTickHook;

/** Active robot worlds */
TArray<UWorld*> RobotWorlds;

/** Robot worlds, pending cleanup */
TArray<UWorld*> PendingRobotWorldCleanup;

/** Active world tick hooks */
TArray<FWorldTickHook*> ActiveRobotTickHooks;


/**
 * FNetworkNotifyRobotHook
 */

void FNetworkNotifyRobotHook::NotifyHandleClientPlayer(APlayerController* PC, UNetConnection* Connection)
{
    NotifyHandleClientPlayerDelegate.ExecuteIfBound(PC, Connection);
}

EAcceptConnection::Type FNetworkNotifyRobotHook::NotifyAcceptingConnection()
{
    EAcceptConnection::Type ReturnVal = EAcceptConnection::Ignore;

    if (NotifyAcceptingConnectionDelegate.IsBound())
    {
        ReturnVal = NotifyAcceptingConnectionDelegate.Execute();
    }

    // Until I have a use-case for doing otherwise, make the original authoritative
    if (HookedNotify != NULL)
    {
        ReturnVal = HookedNotify->NotifyAcceptingConnection();
    }

    return ReturnVal;
}

void FNetworkNotifyRobotHook::NotifyAcceptedConnection(UNetConnection* Connection)
{
    NotifyAcceptedConnectionDelegate.ExecuteIfBound(Connection);

    if (HookedNotify != NULL)
    {
        HookedNotify->NotifyAcceptedConnection(Connection);
    }
}

bool FNetworkNotifyRobotHook::NotifyAcceptingChannel(UChannel* Channel)
{
    bool bReturnVal = false;

    if (NotifyAcceptingChannelDelegate.IsBound())
    {
        bReturnVal = NotifyAcceptingChannelDelegate.Execute(Channel);
    }

    // Until I have a use-case for doing otherwise, make the original authoritative
    if (HookedNotify != NULL)
    {
        bReturnVal = HookedNotify->NotifyAcceptingChannel(Channel);
    }

    return bReturnVal;
}

void FNetworkNotifyRobotHook::NotifyControlMessage(UNetConnection* Connection, uint8 MessageType, FInBunch& Bunch)
{
    bool bHandled = false;

    if (NotifyControlMessageDelegate.IsBound())
    {
        bHandled = NotifyControlMessageDelegate.Execute(Connection, MessageType, Bunch);
    }

    // Only pass on to original, if the control message was not handled
    if (!bHandled && HookedNotify != NULL)
    {
        HookedNotify->NotifyControlMessage(Connection, MessageType, Bunch);
    }
}

/**
 * NRTNet
 */
int32 NRTNet::GetFreeChannelIndex(UNetConnection* InConnection, int32 InChIndex/*=INDEX_NONE*/)
{
    int32 ReturnVal = INDEX_NONE;

    for (int i=(InChIndex == INDEX_NONE ? 0 : InChIndex); i<ARRAY_COUNT(InConnection->Channels); i++)
    {
        if (InConnection->Channels[i] == NULL)
        {
            ReturnVal = i;
            break;
        }
    }
    
    return ReturnVal;
}

FOutBunch* NRTNet::CreateChannelBunch(int32& BunchSequence, UNetConnection* InConnection, EChannelType InChType,
                                        int32 InChIndex/*=INDEX_NONE*/, bool bGetNextFreeChan/*=false*/)
{
    FOutBunch* ReturnVal = NULL;

    BunchSequence++;

    // If the channel already exists, the input sequence needs to be overridden, and the channel sequence incremented
    if (InChIndex != INDEX_NONE && InConnection != NULL && InConnection->Channels[InChIndex] != NULL)
    {
        BunchSequence = ++InConnection->OutReliable[InChIndex];
    }

    if (InConnection != NULL && InConnection->Channels[0] != NULL)
    {
        ReturnVal = new FOutBunch(InConnection->Channels[0], false);

        // Fix for uninitialized members (just one is causing a problem, but may as well do them all)
        ReturnVal->Next = NULL;
        ReturnVal->Time = 0.0;
        ReturnVal->ReceivedAck = false; // This one was the problem - was triggering an assert
        ReturnVal->ChSequence = 0;
		ReturnVal->PacketId = 0;
        ReturnVal->bDormant = false;
    }

    if (ReturnVal != NULL)
    {
        if (InChIndex == INDEX_NONE || bGetNextFreeChan)
        {
            InChIndex = GetFreeChannelIndex(InConnection, InChIndex);
        }

        ReturnVal->Channel = NULL;
        ReturnVal->ChIndex = InChIndex;
        ReturnVal->ChType = InChType;

        // NOTE: Might not cover all bOpen or 'channel already open' cases
        ReturnVal->bOpen = (uint8)(InConnection == NULL || InConnection->Channels[InChIndex] == NULL ||
                                    InConnection->Channels[InChIndex]->OpenPacketId.First == INDEX_NONE);

        if (ReturnVal->bOpen && InConnection != NULL && InConnection->Channels[InChIndex] != NULL)
        {
            InConnection->Channels[InChIndex]->OpenPacketId.First = BunchSequence;
            InConnection->Channels[InChIndex]->OpenPacketId.Last = BunchSequence;
        }

        ReturnVal->bReliable = 1;
        ReturnVal->ChSequence = BunchSequence;
    }
    
    return ReturnVal;
}

void NRTNet::SendControlBunch(UNetConnection* InConnection, FOutBunch& ControlChanBunch)
{
    if (InConnection != NULL && InConnection->Channels[0] != NULL)
    {
        // Since it's the unit test control channel, sending the packet abnormally, append to OutRec manually
        if (ControlChanBunch.bReliable)
        {
            for (FOutBunch* CurOut=InConnection->Channels[0]->OutRec; CurOut!=NULL; CurOut=CurOut->Next)
            {
                if (CurOut->Next == NULL)
                {
                    CurOut->Next = &ControlChanBunch;
                    break;
                }
            }
        }

        InConnection->SendRawBunch(ControlChanBunch, true);
    }
}

UNetDriver* NRTNet::CreateRobotNetDriver(UWorld* InWorld)
{
    UNetDriver* ReturnVal = NULL;
    UGameEngine* GameEngine = Cast<UGameEngine>(GEngine);

    if (GameEngine != NULL)
    {
        static int RobotNetDriverCount = 0;

        // Setup a new driver name entry
        bool bFoundDef = false;
        FName UnitDefName = TEXT("RobotNetDriver");

        for (int i=0; i<GameEngine->NetDriverDefinitions.Num(); i++)
        {
            if (GameEngine->NetDriverDefinitions[i].DefName == UnitDefName)
            {
                bFoundDef = true;
                break;
            }
        }
        
        if (!bFoundDef)
        {
            FNetDriverDefinition NewDriverEntry;

            NewDriverEntry.DefName = UnitDefName;
            NewDriverEntry.DriverClassName = *URobotNetDriver::StaticClass()->GetPathName();
            NewDriverEntry.DriverClassNameFallback = *UNetDriver::StaticClass()->GetPathName();

            GameEngine->NetDriverDefinitions.Add(NewDriverEntry);
        }

        FName NewDriverName = *FString::Printf(TEXT("RobotNetDriver_%i"), RobotNetDriverCount++);

        // Now create a reference to the driver
        if (GameEngine->CreateNamedNetDriver(InWorld, NewDriverName, UnitDefName))
        {
            ReturnVal = Cast<UNetDriver>(GameEngine->FindNamedNetDriver(InWorld, NewDriverName));
        }

        if (ReturnVal != NULL)
        {
            ReturnVal->SetWorld(InWorld);
            InWorld->SetNetDriver(ReturnVal);

            UE_LOG(LogRobot, Log, TEXT("CreateRobotNetDriver: Created named net driver: %s, NetDriverName: %s"), *ReturnVal->GetFullName(), *ReturnVal->NetDriverName.ToString());
        }
        else
        {
            UE_LOG(LogRobot, Log, TEXT("CreateRobotNetDriver: CreateNamedNetDriver failed"));
        }
    }
    else
    {
        UE_LOG(LogRobot, Log, TEXT("CreateRobotNetDriver: GameEngine is NULL"));
    }

    return ReturnVal;
}

bool NRTNet::CreateFakePlayer(UWorld* InWorld, UNetDriver*& InNetDriver, FString ServerIP, FNetworkNotify* InNotify/*=NULL*/,
                                bool bSkipJoin/*=false*/, FUniqueNetIdRepl* InNetID/*=NULL*/, bool bBeaconConnect/*=false*/,
                                FString InBeaconType/*=TEXT("")*/)
{
    bool bSuccess = false;

    return bSuccess;
}

void NRTNet::DisconnectFakePlayer(UWorld* PlayerWorld, UNetDriver* InNetDriver)
{
    GEngine->DestroyNamedNetDriver(PlayerWorld, InNetDriver->NetDriverName);
}

void NRTNet::HandleBeaconReplicate(AOnlineBeaconClient* InBeacon, UNetConnection* InConnection)
{
    //
}

UWorld* NRTNet::CreateRobotWorld(bool bHookTick/*=true*/)
{
    UWorld* ReturnVal = NULL;

    // Unfortunately, this hack is needed, to avoid a crash when running as commandlet
    // NOTE: Sometimes, depending upon build settings, PRIVATE_GIsRunningCommandlet is a define instead of a global
#ifndef PRIVATE_GIsRunningCommandlet
    bool bIsCommandlet = PRIVATE_GIsRunningCommandlet;

    PRIVATE_GIsRunningCommandlet = false;
#endif
    GIsServer = false;
    GIsClient = true;

    ReturnVal = UWorld::CreateWorld(EWorldType::Game, false);

    if (ReturnVal->IsServer())
    {
        GIsServer = true;
    }
    
#ifndef PRIVATE_GIsRunningCommandlet
    PRIVATE_GIsRunningCommandlet = bIsCommandlet;
#endif

    int n = 0;

    if (ReturnVal != NULL)
    {
        if (ReturnVal->IsServer())
        {
            n = 1;
        }

        RobotWorlds.Add(ReturnVal);

        // Hook the new worlds 'tick' event, so that we can capture logging
        if (bHookTick)
        {
            FWorldTickHook* TickHook = new FWorldTickHook(ReturnVal);

            ActiveRobotTickHooks.Add(TickHook);

            TickHook->Init();
        }

        // Hack-mark the world as having begun play (when it has not)
        ReturnVal->bBegunPlay = true;

        // Hack-mark the world as having initialized actors (to allow RPC hooks)
        ReturnVal->bActorsInitialized = true;

        // Enable pause, using the PlayerController of the primary world (unless we're in the editor)
        if (!GIsEditor)
        {
            AWorldSettings* CurSettings = ReturnVal->GetWorldSettings();

            if (CurSettings != NULL)
            {
                ULocalPlayer* PrimLocPlayer = GEngine->GetFirstGamePlayer(NRTUtil::GetPrimaryWorld());
                APlayerController* PrimPC = (PrimLocPlayer != NULL ? PrimLocPlayer->PlayerController : NULL);
                APlayerState* PrimState = (PrimPC != NULL ? PrimPC->PlayerState : NULL);

                if (PrimState != NULL)
                {
                    CurSettings->Pauser = PrimState;
                }
            }
        }

        // Create a blank world context, to prevent crashes
        FWorldContext& CurContext = GEngine->CreateNewWorldContext(EWorldType::Game);
        CurContext.SetCurrentWorld(ReturnVal);
    }
    
    return ReturnVal;
}

void NRTNet::MarkRobotWorldForCleanup(UWorld* CleanupWorld, bool bImmediate/*=false*/)
{
    RobotWorlds.Remove(CleanupWorld);
    PendingRobotWorldCleanup.Add(CleanupWorld);

    if (!bImmediate)
    {
        GEngine->DeferredCommands.AddUnique(TEXT("CleanupRobotWorlds"));
    }
    else
    {
        CleanupRobotWorlds();
    }
}

void NRTNet::CleanupRobotWorlds()
{
    for (auto It=PendingRobotWorldCleanup.CreateIterator(); It; ++It)
    {
        UWorld* CurWorld = *It;

        // Remove the tick-hook, for this world
        int32 TickHookIdx = ActiveRobotTickHooks.IndexOfByPredicate(
            [&CurWorld](const FWorldTickHook* CurTickHook)
            {
                return CurTickHook != NULL && CurTickHook->AttachedWorld == CurWorld;
            });

        if (TickHookIdx != INDEX_NONE)
        {
            ActiveRobotTickHooks.RemoveAt(TickHookIdx);
        }
        
        GEngine->DestroyWorldContext(CurWorld);
		CurWorld->DestroyWorld(false);
    }

    PendingRobotWorldCleanup.Empty();

    // Immediately garbage collect remaining objects, to finish net driver cleanup
    CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS, true);
}

bool NRTNet::IsRobotWorld(UWorld* InWorld)
{
    return RobotWorlds.Contains(InWorld);
}

bool NRTNet::IsSteamNetDriverAvailable()
{
    bool bReturnVal = false;
    UGameEngine* GameEngine = Cast<UGameEngine>(GEngine);

    if (GameEngine != NULL)
    {
        bool bFoundSteamDriver = false;
        const TCHAR* SteamDriverClassName = TEXT("OnlineSubsystemSteam.SteamNetDriver");

        for (int i=0; i<GameEngine->NetDriverDefinitions.Num(); i++)
        {
            if (GameEngine->NetDriverDefinitions[i].DefName == NAME_GameNetDriver)
            {
                if (GameEngine->NetDriverDefinitions[i].DriverClassName == SteamDriverClassName)
                {
                    bFoundSteamDriver = true;
                }

                break;
            }
        }

        if (bFoundSteamDriver)
        {
            UClass* SteamNetDriverClass = StaticLoadClass(UNetDriver::StaticClass(), NULL, SteamDriverClassName, NULL, LOAD_Quiet);

            if (SteamDriverClassName != NULL)
            {
                UNetDriver* SteamNetDriverDef = Cast<UNetDriver>(SteamNetDriverClass->GetDefaultObject());

                bReturnVal = SteamNetDriverDef != NULL && SteamNetDriverDef->IsAvailable();
            }
        }
    }

    return bReturnVal;
}
