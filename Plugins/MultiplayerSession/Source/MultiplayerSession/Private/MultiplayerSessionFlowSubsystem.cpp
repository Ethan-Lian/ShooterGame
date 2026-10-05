#include "MultiplayerSessionFlowSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/NetConnection.h"
#include "Engine/NetDriver.h"
#include "Engine/PendingNetGame.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "MultiplayerSessionSettings.h"
#include "MultiplayerSessionsSubsystem.h"
#include "Subsystems/SubsystemCollection.h"
#include "UObject/UObjectGlobals.h"

#define LOCTEXT_NAMESPACE "MultiplayerSessionFlow"

DEFINE_LOG_CATEGORY_STATIC(LogMultiplayerSessionFlow, Log, All);

namespace
{
	bool IsWorldMap(const UWorld* World, const FString& MapPath)
	{
		return World && !MapPath.IsEmpty() && UWorld::RemovePIEPrefix(World->GetOutermost()->GetName()) == MapPath;
	}
}

void UMultiplayerSessionFlowSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	bDeinitializing = false;
	Sessions = Collection.InitializeDependency<UMultiplayerSessionsSubsystem>();
	Sessions->MultiplayerOnCreateSessionCompleteDelegate.AddUObject(this, &ThisClass::HandleCreateSessionComplete);
	Sessions->MultiplayerOnJoinSessionCompleteDelegate.AddUObject(this, &ThisClass::HandleJoinSessionComplete);
	Sessions->MultiplayerOnDestroySessionCompleteDelegate.AddUObject(this, &ThisClass::HandleDestroySessionComplete);
	Sessions->OnInviteAccepted.AddUObject(this, &ThisClass::HandleInviteAccepted);
	BindLocalPlayerControllerChanged();
	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::HandlePostLoadMap);
	if (GEngine)
	{
		NetworkFailureHandle = GEngine->OnNetworkFailure().AddUObject(this, &ThisClass::HandleNetworkFailure);
		TravelFailureHandle = GEngine->OnTravelFailure().AddUObject(this, &ThisClass::HandleTravelFailure);
	}
	if (GetWorld() && GetWorld()->HasBegunPlay())
	{
		HandlePostLoadMap(GetWorld());
	}
}

void UMultiplayerSessionFlowSubsystem::Deinitialize()
{
	bDeinitializing = true;
	FTSTicker::RemoveTicker(WorldReadyTickerHandle);
	FTSTicker::RemoveTicker(RecoveryTickerHandle);
	WorldReadyTickerHandle.Reset();
	RecoveryTickerHandle.Reset();
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
	if (ULocalPlayer* LocalPlayer = ControllerEventLocalPlayer.Get())
	{
		LocalPlayer->OnPlayerControllerChanged().Remove(PlayerControllerChangedHandle);
	}
	ControllerEventLocalPlayer.Reset();
	if (GEngine)
	{
		GEngine->OnNetworkFailure().Remove(NetworkFailureHandle);
		GEngine->OnTravelFailure().Remove(TravelFailureHandle);
	}
	if (Sessions)
	{
		Sessions->MultiplayerOnCreateSessionCompleteDelegate.RemoveAll(this);
		Sessions->MultiplayerOnJoinSessionCompleteDelegate.RemoveAll(this);
		Sessions->MultiplayerOnDestroySessionCompleteDelegate.RemoveAll(this);
		Sessions->OnInviteAccepted.RemoveAll(this);
	}
	PendingInvite = FOnlineSessionSearchResult();
	Sessions = nullptr;
	Super::Deinitialize();
}

bool UMultiplayerSessionFlowSubsystem::IsBusy() const
{
	return bDeinitializing || bRecoveryRequested || (Sessions && Sessions->IsBusy())
		|| State == EMultiplayerSessionFlowState::CreatingSession
		|| State == EMultiplayerSessionFlowState::JoiningSession
		|| State == EMultiplayerSessionFlowState::Traveling
		|| State == EMultiplayerSessionFlowState::Leaving;
}

APlayerController* UMultiplayerSessionFlowSubsystem::GetLocalPlayerController() const
{
	UGameInstance* GameInstance = GetGameInstance();
	APlayerController* Controller = GameInstance ? GameInstance->GetFirstLocalPlayerController(GetWorld()) : nullptr;
	return Controller && Controller->IsLocalController() && Controller->GetWorld() == GetWorld() ? Controller : nullptr;
}

bool UMultiplayerSessionFlowSubsystem::HasGameConnection() const
{
	const UWorld* World = GetWorld();
	const FWorldContext* Context = World && GEngine ? GEngine->GetWorldContextFromWorld(World) : nullptr;
	return (World && World->GetNetMode() != NM_Standalone) || (Context && Context->PendingNetGame);
}

bool UMultiplayerSessionFlowSubsystem::HostGame(int32 NumPublicConnections, FString MatchType, FString LobbyMapPath)
{
	if (IsBusy() || State != EMultiplayerSessionFlowState::Idle || !GetLocalPlayerController()
		|| HasGameConnection() || NumPublicConnections < 1)
	{
		ReportError(LOCTEXT("CannotHost", "Cannot create a room in the current state."));
		return false;
	}
	Sessions->RefreshOnlineSessionInterface();
	if (Sessions->HasSession())
	{
		ReportError(LOCTEXT("SessionStillExists", "Leave the existing session before creating another room."));
		SetState(EMultiplayerSessionFlowState::CleanupFailed);
		return false;
	}
	HostedLobbyMap = LobbyMapPath.IsEmpty()
		? GetDefault<UMultiplayerSessionSettings>()->LobbyMap.ToSoftObjectPath().GetLongPackageName() : LobbyMapPath;
	if (!ValidateMap(HostedLobbyMap))
	{
		return false;
	}
	LastError = FText::GetEmpty();
	PendingInvite = FOnlineSessionSearchResult();
	SetState(EMultiplayerSessionFlowState::CreatingSession);
	if (!Sessions->CreateSession(NumPublicConnections, MatchType))
	{
		ReportError(LOCTEXT("CreateNotAdmitted", "Could not start session creation. Check the online service and local player."));
		SetState(EMultiplayerSessionFlowState::Idle);
		return false;
	}
	return true;
}

void UMultiplayerSessionFlowSubsystem::HandleCreateSessionComplete(bool bWasSuccessful)
{
	if (State != EMultiplayerSessionFlowState::CreatingSession || ContinueRecoveryIfRequested())
	{
		return;
	}
	if (!bWasSuccessful)
	{
		RequestRecovery(LOCTEXT("CreateFailed", "Session creation failed."));
		return;
	}
	UWorld* World = GetWorld();
	if (!World || !GetLocalPlayerController())
	{
		RequestRecovery(LOCTEXT("HostWorldMissing", "The local world is unavailable after session creation."));
		return;
	}
	BeginTravel(ETravelDestination::Lobby, HostedLobbyMap);
	if (!World->ServerTravel(HostedLobbyMap + TEXT("?listen"))
		&& State == EMultiplayerSessionFlowState::Traveling && !bRecoveryRequested)
	{
		RequestRecovery(LOCTEXT("LobbyTravelRejected", "Could not open the lobby."));
	}
}

bool UMultiplayerSessionFlowSubsystem::StartHostedGame()
{
	if (IsBusy() || State != EMultiplayerSessionFlowState::Lobby || !CanInviteFriends())
	{
		ReportError(LOCTEXT("CannotStart", "Only the room host can start the game from the lobby."));
		return false;
	}
	const FString MapPath = GetDefault<UMultiplayerSessionSettings>()->GameplayMap.ToSoftObjectPath().GetLongPackageName();
	if (!ValidateMap(MapPath))
	{
		return false;
	}
	UWorld* World = GetWorld();
	AGameModeBase* GameMode = World->GetAuthGameMode();
	if (!GameMode)
	{
		ReportError(LOCTEXT("GameModeMissing", "The host game mode is unavailable."));
		return false;
	}
	LastError = FText::GetEmpty();
	const bool bPreviousSeamlessTravel = GameMode->bUseSeamlessTravel;
	GameMode->bUseSeamlessTravel = true;
	BeginTravel(ETravelDestination::Game, MapPath);
	if (!World->ServerTravel(MapPath) && State == EMultiplayerSessionFlowState::Traveling && !bRecoveryRequested)
	{
		GameMode->bUseSeamlessTravel = bPreviousSeamlessTravel;
		TravelDestination = ETravelDestination::None;
		ExpectedMap.Reset();
		ReportError(LOCTEXT("GameTravelRejected", "The game could not start. Try again from the lobby."));
		SetState(EMultiplayerSessionFlowState::Lobby);
		return false;
	}
	return true;
}

bool UMultiplayerSessionFlowSubsystem::CanInviteFriends() const
{
	const UWorld* World = GetWorld();
	return !IsBusy() && (State == EMultiplayerSessionFlowState::Lobby || State == EMultiplayerSessionFlowState::InGame)
		&& World && World->GetNetMode() == NM_ListenServer && GetLocalPlayerController()
		&& Sessions && Sessions->HasSession();
}

void UMultiplayerSessionFlowSubsystem::HandleInviteAccepted(bool bWasSuccessful, const FOnlineSessionSearchResult& InviteResult)
{
	if (bDeinitializing)
	{
		return;
	}
	if (!bWasSuccessful)
	{
		ReportError(LOCTEXT("InvalidInvite", "The Steam invitation is no longer valid."));
		return;
	}
	if (IsBusy() || State == EMultiplayerSessionFlowState::CleanupFailed)
	{
		ReportError(LOCTEXT("InviteWhileBusy", "Finish the current session operation before accepting another invitation."));
		return;
	}
	if (State == EMultiplayerSessionFlowState::Idle && !Sessions->HasSession() && !HasGameConnection())
	{
		BeginJoinSession(InviteResult);
		return;
	}
	// Never tear down an active room merely because the platform emitted an invite event.
	PendingInvite = InviteResult;
	ReportError(LOCTEXT("InviteNeedsConfirmation", "An invitation is pending. Confirm leaving the current room to join it."));
	OnInviteConfirmationRequested.Broadcast(InviteResult.Session.OwningUserName);
}

bool UMultiplayerSessionFlowSubsystem::ConfirmPendingInvite()
{
	if (IsBusy() || State == EMultiplayerSessionFlowState::CleanupFailed || !PendingInvite.IsValid())
	{
		return false;
	}
	LastError = FText::GetEmpty();
	AfterLeave = EAfterLeave::JoinInvite;
	BeginCleanup();
	return true;
}

void UMultiplayerSessionFlowSubsystem::DeclinePendingInvite()
{
	if (AfterLeave == EAfterLeave::JoinInvite || !PendingInvite.IsValid())
	{
		return;
	}
	PendingInvite = FOnlineSessionSearchResult();
	LastError = FText::GetEmpty();
	OnStateChanged.Broadcast(State, LastError);
}

bool UMultiplayerSessionFlowSubsystem::BeginJoinSession(const FOnlineSessionSearchResult& InviteResult)
{
	if (IsBusy() || State != EMultiplayerSessionFlowState::Idle || !GetLocalPlayerController()
		|| HasGameConnection() || Sessions->HasSession())
	{
		ReportError(LOCTEXT("CannotJoin", "Leave the current connection before joining another room."));
		return false;
	}
	LastError = FText::GetEmpty();
	SetState(EMultiplayerSessionFlowState::JoiningSession);
	if (!Sessions->JoinSession(InviteResult))
	{
		ReportError(LOCTEXT("JoinNotAdmitted", "Could not start joining the room."));
		SetState(EMultiplayerSessionFlowState::Idle);
		return false;
	}
	return true;
}

void UMultiplayerSessionFlowSubsystem::HandleJoinSessionComplete(EOnJoinSessionCompleteResult::Type Result, const FString& Address)
{
	if (State != EMultiplayerSessionFlowState::JoiningSession || ContinueRecoveryIfRequested())
	{
		return;
	}
	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		switch (Result)
		{
		case EOnJoinSessionCompleteResult::SessionIsFull:
			RequestRecovery(LOCTEXT("RoomFull", "The room is full."));
			break;
		case EOnJoinSessionCompleteResult::SessionDoesNotExist:
			RequestRecovery(LOCTEXT("RoomGone", "The room no longer exists."));
			break;
		case EOnJoinSessionCompleteResult::CouldNotRetrieveAddress:
			RequestRecovery(LOCTEXT("AddressMissing", "Could not resolve the room connection address."));
			break;
		default:
			RequestRecovery(LOCTEXT("JoinFailed", "Joining the room failed."));
			break;
		}
		return;
	}
	APlayerController* Controller = GetLocalPlayerController();
	if (!Controller || Address.IsEmpty())
	{
		RequestRecovery(LOCTEXT("JoinTravelUnavailable", "The local player cannot connect to the room."));
		return;
	}
	BeginTravel(ETravelDestination::Session, FString());
	Controller->ClientTravel(Address, TRAVEL_Absolute);
}

bool UMultiplayerSessionFlowSubsystem::LeaveSession()
{
	if (IsBusy())
	{
		return false;
	}
	LastError = FText::GetEmpty();
	PendingInvite = FOnlineSessionSearchResult();
	AfterLeave = EAfterLeave::Menu;
	BeginCleanup();
	return true;
}

void UMultiplayerSessionFlowSubsystem::BeginCleanup()
{
	bRecoveryRequested = false;
	bSessionCleanupFailed = false;
	TravelDestination = ETravelDestination::None;
	ExpectedMap.Reset();
	SetState(EMultiplayerSessionFlowState::Leaving);
	if (!Sessions->DestroySession())
	{
		HandleDestroySessionComplete(false);
	}
}

void UMultiplayerSessionFlowSubsystem::HandleDestroySessionComplete(bool bWasSuccessful)
{
	if (bDeinitializing || State != EMultiplayerSessionFlowState::Leaving)
	{
		return;
	}
	bSessionCleanupFailed = !bWasSuccessful;
	if (!bWasSuccessful)
	{
		PendingInvite = FOnlineSessionSearchResult();
		AfterLeave = EAfterLeave::Menu;
		const FText CleanupError = LOCTEXT("CleanupFailed", "Session cleanup failed. Retry leaving before joining again.");
		ReportError(LastError.IsEmpty() ? CleanupError
			: FText::Format(LOCTEXT("FailureAndCleanup", "{0}\n{1}"), LastError, CleanupError));
	}
	// Returning locally also closes the gameplay connection, even if OSS cleanup needs a retry.
	ReturnToMenu();
}

void UMultiplayerSessionFlowSubsystem::ReturnToMenu()
{
	const FString MenuMap = GetDefault<UMultiplayerSessionSettings>()->MenuMap.ToSoftObjectPath().GetLongPackageName();
	UWorld* World = GetWorld();
	if (!World || !ValidateMap(MenuMap))
	{
		AfterLeave = EAfterLeave::Menu;
		SetState(EMultiplayerSessionFlowState::CleanupFailed);
		return;
	}
	const FWorldContext* Context = GEngine ? GEngine->GetWorldContextFromWorld(World) : nullptr;
	if (IsWorldMap(World, MenuMap) && World->GetNetMode() == NM_Standalone && (!Context || !Context->PendingNetGame))
	{
		FinishReturnToMenu();
		return;
	}
	BeginTravel(ETravelDestination::Menu, MenuMap);
	UGameplayStatics::OpenLevel(World, FName(*MenuMap));
}

void UMultiplayerSessionFlowSubsystem::FinishReturnToMenu()
{
	TravelDestination = ETravelDestination::None;
	ExpectedMap.Reset();
	HostedLobbyMap.Reset();
	if (bSessionCleanupFailed)
	{
		SetState(EMultiplayerSessionFlowState::CleanupFailed);
		return;
	}
	const bool bJoinInvite = AfterLeave == EAfterLeave::JoinInvite;
	const FOnlineSessionSearchResult InviteResult = PendingInvite;
	PendingInvite = FOnlineSessionSearchResult();
	AfterLeave = EAfterLeave::Menu;
	if (bJoinInvite)
	{
		// Reserve the next stage without publishing an idle gap that could admit another command.
		State = EMultiplayerSessionFlowState::Idle;
		if (!BeginJoinSession(InviteResult))
		{
			SetState(EMultiplayerSessionFlowState::Idle);
		}
	}
	else
	{
		SetState(EMultiplayerSessionFlowState::Idle);
	}
}

void UMultiplayerSessionFlowSubsystem::RequestRecovery(const FText& Error)
{
	if (bDeinitializing || bRecoveryRequested || State == EMultiplayerSessionFlowState::Leaving)
	{
		return;
	}
	bRecoveryRequested = true;
	PendingInvite = FOnlineSessionSearchResult();
	AfterLeave = EAfterLeave::Menu;
	ReportError(Error);
	if (bDeinitializing)
	{
		return;
	}
	// Engine failure delegates run before default disconnect handling; defer across worlds.
	RecoveryTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &ThisClass::TickRecovery));
}

bool UMultiplayerSessionFlowSubsystem::TickRecovery(float DeltaTime)
{
	RecoveryTickerHandle.Reset();
	if (!bDeinitializing && bRecoveryRequested && !Sessions->IsBusy())
	{
		BeginCleanup();
	}
	return false;
}

bool UMultiplayerSessionFlowSubsystem::ContinueRecoveryIfRequested()
{
	if (!bRecoveryRequested)
	{
		return false;
	}
	if (!RecoveryTickerHandle.IsValid())
	{
		BeginCleanup();
	}
	return true;
}

void UMultiplayerSessionFlowSubsystem::BeginTravel(ETravelDestination Destination, const FString& MapPath)
{
	FTSTicker::RemoveTicker(WorldReadyTickerHandle);
	WorldReadyTickerHandle.Reset();
	ReadyWorld.Reset();
	TravelDestination = Destination;
	ExpectedMap = MapPath;
	SetState(EMultiplayerSessionFlowState::Traveling);
}

void UMultiplayerSessionFlowSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
	if (bDeinitializing || !LoadedWorld || LoadedWorld->GetGameInstance() != GetGameInstance())
	{
		return;
	}
	Sessions->RefreshOnlineSessionInterface();
	BindLocalPlayerControllerChanged();
	FTSTicker::RemoveTicker(WorldReadyTickerHandle);
	ReadyWorld = LoadedWorld;
	WorldReadyTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &ThisClass::TickWorldReady));
}

void UMultiplayerSessionFlowSubsystem::BindLocalPlayerControllerChanged()
{
	ULocalPlayer* LocalPlayer = GetGameInstance()->GetFirstGamePlayer();
	if (LocalPlayer == ControllerEventLocalPlayer.Get())
	{
		return;
	}
	if (ULocalPlayer* PreviousPlayer = ControllerEventLocalPlayer.Get())
	{
		PreviousPlayer->OnPlayerControllerChanged().Remove(PlayerControllerChangedHandle);
	}
	ControllerEventLocalPlayer = LocalPlayer;
	PlayerControllerChangedHandle.Reset();
	if (LocalPlayer)
	{
		PlayerControllerChangedHandle = LocalPlayer->OnPlayerControllerChanged().AddUObject(
			this, &ThisClass::HandleLocalPlayerControllerChanged);
	}
}

void UMultiplayerSessionFlowSubsystem::HandleLocalPlayerControllerChanged(APlayerController* Controller)
{
	if (!bDeinitializing && Controller && Controller->GetWorld() == ReadyWorld.Get())
	{
		// SetPlayer broadcasts before NetConnection finishes assigning the real controller.
		// A controller change in the old world is not destination-map completion.
		FTSTicker::RemoveTicker(WorldReadyTickerHandle);
		WorldReadyTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateUObject(this, &ThisClass::TickWorldReady));
	}
}

bool UMultiplayerSessionFlowSubsystem::TickWorldReady(float DeltaTime)
{
	WorldReadyTickerHandle.Reset();
	UWorld* World = ReadyWorld.Get();
	if (bDeinitializing || bRecoveryRequested || !World || World != GetWorld())
	{
		return false;
	}
	if (State == EMultiplayerSessionFlowState::Traveling)
	{
		if (!ExpectedMap.IsEmpty() && !IsWorldMap(World, ExpectedMap))
		{
			return false;
		}
		APlayerController* Controller = GetLocalPlayerController();
		const ETravelDestination Destination = TravelDestination;
		if (Destination == ETravelDestination::Session)
		{
			const UNetDriver* NetDriver = World->GetNetDriver();
			if (World->GetNetMode() != NM_Client || !NetDriver || !NetDriver->ServerConnection)
			{
				RequestRecovery(LOCTEXT("ConnectionNotReady", "The destination world is not connected to the room."));
				return false;
			}
			if (!Controller || NetDriver->ServerConnection->PlayerController != Controller)
			{
				// Wait for OnPlayerControllerChanged; the first local controller is a placeholder.
				return false;
			}
		}
		if (!Controller)
		{
			RequestRecovery(LOCTEXT("PlayerNotReady", "The destination world has no usable local player."));
			return false;
		}
		if (Destination == ETravelDestination::Menu)
		{
			FinishReturnToMenu();
			return false;
		}
		TravelDestination = ETravelDestination::None;
		ExpectedMap.Reset();
		const FString LobbyMap = GetDefault<UMultiplayerSessionSettings>()->LobbyMap.ToSoftObjectPath().GetLongPackageName();
		SetState(Destination == ETravelDestination::Lobby || IsWorldMap(World, LobbyMap)
			? EMultiplayerSessionFlowState::Lobby : EMultiplayerSessionFlowState::InGame);
	}
	else if (!IsBusy() && State != EMultiplayerSessionFlowState::CleanupFailed && GetLocalPlayerController())
	{
		const UMultiplayerSessionSettings* Settings = GetDefault<UMultiplayerSessionSettings>();
		if (IsWorldMap(World, Settings->MenuMap.ToSoftObjectPath().GetLongPackageName()))
		{
			SetState(HasGameConnection() ? EMultiplayerSessionFlowState::InGame : EMultiplayerSessionFlowState::Idle);
		}
		else if (IsWorldMap(World, Settings->LobbyMap.ToSoftObjectPath().GetLongPackageName()))
		{
			SetState(EMultiplayerSessionFlowState::Lobby);
		}
		else if (IsWorldMap(World, Settings->GameplayMap.ToSoftObjectPath().GetLongPackageName()))
		{
			SetState(EMultiplayerSessionFlowState::InGame);
		}
	}
	return false;
}

bool UMultiplayerSessionFlowSubsystem::IsFailureForThisGameInstance(UWorld* World, UNetDriver* NetDriver) const
{
	if (World && World->GetGameInstance() == GetGameInstance())
	{
		return true;
	}
	if (GEngine && NetDriver)
	{
		const FWorldContext* Context = GEngine->GetWorldContextFromPendingNetGameNetDriver(NetDriver);
		return Context && Context->OwningGameInstance == GetGameInstance();
	}
	return false;
}

void UMultiplayerSessionFlowSubsystem::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver,
	ENetworkFailure::Type FailureType, const FString& Error)
{
	if (bDeinitializing || !NetDriver || !IsFailureForThisGameInstance(World, NetDriver)
		|| (NetDriver->NetDriverName != NAME_GameNetDriver && NetDriver->NetDriverName != NAME_PendingNetDriver))
	{
		return;
	}
	const bool bPeerFailure = FailureType == ENetworkFailure::ConnectionLost
		|| FailureType == ENetworkFailure::ConnectionTimeout || FailureType == ENetworkFailure::NetGuidMismatch
		|| FailureType == ENetworkFailure::NetChecksumMismatch;
	if (bPeerFailure && NetDriver->GetNetMode() != NM_Client)
	{
		return; // A departing client must not close the host's room.
	}
	if (State == EMultiplayerSessionFlowState::Leaving
		|| (State == EMultiplayerSessionFlowState::Traveling && TravelDestination == ETravelDestination::Menu))
	{
		return;
	}
	RequestRecovery(FText::Format(LOCTEXT("NetworkFailure", "The multiplayer connection failed: {0}"), FText::FromString(Error)));
}

void UMultiplayerSessionFlowSubsystem::HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& Error)
{
	if (bDeinitializing || !IsFailureForThisGameInstance(World, nullptr))
	{
		return;
	}
	const FText Message = FText::Format(LOCTEXT("TravelFailure", "Map travel failed: {0}"), FText::FromString(Error));
	if (TravelDestination == ETravelDestination::Menu)
	{
		TravelDestination = ETravelDestination::None;
		AfterLeave = EAfterLeave::Menu;
		ReportError(Message);
		SetState(EMultiplayerSessionFlowState::CleanupFailed);
		return;
	}
	RequestRecovery(Message);
}

bool UMultiplayerSessionFlowSubsystem::ValidateMap(const FString& MapPath)
{
	if (!FPackageName::IsValidLongPackageName(MapPath) || !FPackageName::DoesPackageExist(MapPath))
	{
		ReportError(FText::Format(LOCTEXT("InvalidMap", "Configure an existing map in Multiplayer Session settings: {0}"), FText::FromString(MapPath)));
		return false;
	}
	return true;
}

void UMultiplayerSessionFlowSubsystem::SetState(EMultiplayerSessionFlowState NewState)
{
	State = NewState;
	UE_LOG(LogMultiplayerSessionFlow, Log, TEXT("Session flow state=%d"), static_cast<int32>(State));
	OnStateChanged.Broadcast(State, LastError);
}

void UMultiplayerSessionFlowSubsystem::ReportError(const FText& Error)
{
	LastError = Error;
	UE_LOG(LogMultiplayerSessionFlow, Warning, TEXT("%s"), *Error.ToString());
	OnStateChanged.Broadcast(State, LastError);
}

#undef LOCTEXT_NAMESPACE
