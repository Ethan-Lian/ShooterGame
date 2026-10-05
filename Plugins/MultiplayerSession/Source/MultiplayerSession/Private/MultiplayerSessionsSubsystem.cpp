#include "MultiplayerSessionsSubsystem.h"
#include "Online.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystemUtils.h"
#include "GameFramework/PlayerController.h"
#include "Interfaces/OnlineExternalUIInterface.h"
#include "Misc/App.h"
#include "TimerManager.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogMultiplayerSessions, Log, All);

namespace MultiplayerSessionMetadataKeys
{
	const FName SessionProject(TEXT("SessionProject"));
	const FName SessionBuildId(TEXT("SessionBuildId"));
}

UMultiplayerSessionsSubsystem::UMultiplayerSessionsSubsystem() :   
	CreateSessionCompleteDelegate(FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateSessionComplete)),
	JoinSessionCompleteDelegate(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionComplete)),
	DestroySessionCompleteDelegate(FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroySessionComplete)),
	SessionUserInviteAcceptedDelegate(FOnSessionUserInviteAcceptedDelegate::CreateUObject(this, &ThisClass::OnSessionUserInviteAccepted))
{
}

void UMultiplayerSessionsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	PostLoadMapWithWorldDelegateHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
		this,
		&ThisClass::OnPostLoadMapWithWorld);
	RefreshOnlineSessionInterface();
}

void UMultiplayerSessionsSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
	}

	if (PostLoadMapWithWorldDelegateHandle.IsValid())
	{
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapWithWorldDelegateHandle);
		PostLoadMapWithWorldDelegateHandle.Reset();
	}

	ClearSessionInviteAcceptedDelegate();

	Super::Deinitialize();
}

void UMultiplayerSessionsSubsystem::CreateSession(int32 NumPublicConnections, FString MatchType)
{
	DesiredSessionConfig.NumPublicConnections = NumPublicConnections;
	DesiredSessionConfig.MatchType = MatchType;
	
	if (!RefreshOnlineSessionInterface())
	{
		MultiplayerOnCreateSessionCompleteDelegate.Broadcast(false);
		return;
	}
	
	auto ExistingSession = OnlineSessionInterface->GetNamedSession(NAME_GameSession);
	if (ExistingSession != nullptr)
	{
		// The Steam OSS only allows one named game session, so recreate after destroy completes.
		PendingRecreateConfig = DesiredSessionConfig;
		DestroySession();
		return;
	}
	
	CreateSessionInternal(DesiredSessionConfig);
}

void UMultiplayerSessionsSubsystem::JoinSession(const FOnlineSessionSearchResult& SessionResult)
{
	if (!RefreshOnlineSessionInterface())
	{
		UE_LOG(LogMultiplayerSessions, Warning, TEXT("Cannot join session: online session interface is unavailable."));
		MultiplayerOnJoinSessionCompleteDelegate.Broadcast(EOnJoinSessionCompleteResult::UnknownError, FString());
		return;
	}
	
	JoinSessionCompleteDelegateHandle = OnlineSessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!LocalPlayer || !LocalPlayer->GetPreferredUniqueNetId().IsValid())
	{
		OnlineSessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
		JoinSessionCompleteDelegateHandle.Reset();
		UE_LOG(LogMultiplayerSessions, Warning, TEXT("Cannot join session: local player id is invalid."));
		MultiplayerOnJoinSessionCompleteDelegate.Broadcast(EOnJoinSessionCompleteResult::UnknownError, FString());
		return;
	}

	const FString SessionIdString = SessionResult.GetSessionIdStr();
	UE_LOG(
		LogMultiplayerSessions,
		Log,
		TEXT("Joining session. Owner=%s SessionId=%s UsesLobby=%d HasSessionInfo=%d"),
		*SessionResult.Session.OwningUserName,
		SessionIdString.IsEmpty() ? TEXT("None") : *SessionIdString,
		SessionResult.Session.SessionSettings.bUseLobbiesIfAvailable,
		SessionResult.Session.SessionInfo.IsValid());

	const bool bJoinStarted = OnlineSessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, SessionResult);
	
	if (!bJoinStarted)
	{
		OnlineSessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
		JoinSessionCompleteDelegateHandle.Reset();
		UE_LOG(LogMultiplayerSessions, Warning, TEXT("JoinSession did not start."));
		MultiplayerOnJoinSessionCompleteDelegate.Broadcast(EOnJoinSessionCompleteResult::UnknownError, FString());
	}
}

void UMultiplayerSessionsSubsystem::DestroySession()
{
	if (!RefreshOnlineSessionInterface())
	{
		OnDestroySessionComplete(NAME_GameSession, false);
		return;
	}

	if (OnlineSessionInterface->GetNamedSession(NAME_GameSession) == nullptr)
	{
		OnDestroySessionComplete(NAME_GameSession, true);
		return;
	}
	
	DestroySessionCompleteDelegateHandle = OnlineSessionInterface->AddOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegate);
	
	bool bDestroysuccessful = OnlineSessionInterface->DestroySession(NAME_GameSession);
	if (!bDestroysuccessful)
	{
		OnlineSessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
		DestroySessionCompleteDelegateHandle.Reset();
		OnDestroySessionComplete(NAME_GameSession, false);
	}
}

bool UMultiplayerSessionsSubsystem::ShowSteamInviteUI()
{
	if (!RefreshOnlineSessionInterface())
	{
		UE_LOG(LogMultiplayerSessions, Warning, TEXT("Cannot show invite UI: online session interface is unavailable."));
		return false;
	}

	const FNamedOnlineSession* CurrentSession = OnlineSessionInterface->GetNamedSession(NAME_GameSession);
	if (CurrentSession == nullptr || !CurrentSession->SessionInfo.IsValid())
	{
		UE_LOG(LogMultiplayerSessions, Warning, TEXT("Cannot show invite UI: no valid game session exists."));
		return false;
	}

	IOnlineExternalUIPtr ExternalUI = Online::GetExternalUIInterface(GetWorld());
	if (!ExternalUI.IsValid())
	{
		UE_LOG(LogMultiplayerSessions, Warning, TEXT("Cannot show invite UI: external UI interface is unavailable."));
		return false;
	}

	const bool bOpened = ExternalUI->ShowInviteUI(0, NAME_GameSession);
	if (!bOpened)
	{
		UE_LOG(LogMultiplayerSessions, Warning, TEXT("Steam invite UI failed to open."));
	}

	return bOpened;
}

bool UMultiplayerSessionsSubsystem::CanShowHostInvitePanel() const
{
	UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_Client || World->GetNetMode() == NM_DedicatedServer)
	{
		return false;
	}

	const IOnlineSessionPtr SessionInterface = Online::GetSessionInterface(World);
	return SessionInterface.IsValid() && SessionInterface->GetNamedSession(NAME_GameSession) != nullptr;
}

void UMultiplayerSessionsSubsystem::ShowLobbyInvitePanel()
{
	if (!CanShowHostInvitePanel())
	{
		UE_LOG(LogMultiplayerSessions, Verbose, TEXT("Skipping lobby invite panel: local player is not the host or no session exists."));
		return;
	}

	OnLobbyInvitePanelRequested.Broadcast();
}

void UMultiplayerSessionsSubsystem::RequestShowLobbyInvitePanelAfterTravel()
{
	bShowLobbyInvitePanelAfterTravel = true;
}


/*
 * Callbacks function
 */

void UMultiplayerSessionsSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (OnlineSessionInterface)
	{
		OnlineSessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
		CreateSessionCompleteDelegateHandle.Reset();
	}
	
	MultiplayerOnCreateSessionCompleteDelegate.Broadcast(bWasSuccessful);
}

void UMultiplayerSessionsSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (OnlineSessionInterface)
	{
		OnlineSessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
		JoinSessionCompleteDelegateHandle.Reset();
	}

	FString ConnectAddress;
	EOnJoinSessionCompleteResult::Type BroadcastResult = Result;
	if (Result == EOnJoinSessionCompleteResult::Success && OnlineSessionInterface.IsValid())
	{
		// Resolve the final travel address inside the session layer so UI code stays OSS-agnostic.
		const bool bHasConnectString = OnlineSessionInterface->GetResolvedConnectString(SessionName, ConnectAddress);
		if (!bHasConnectString)
		{
			BroadcastResult = EOnJoinSessionCompleteResult::UnknownError;
			UE_LOG(LogMultiplayerSessions, Warning, TEXT("Join session succeeded but no connect string was resolved."));
		}
	}

	UE_LOG(
		LogMultiplayerSessions,
		Log,
		TEXT("Join session complete. Session=%s Result=%d Address=%s"),
		*SessionName.ToString(),
		static_cast<int32>(BroadcastResult),
		ConnectAddress.IsEmpty() ? TEXT("None") : *ConnectAddress);

	MultiplayerOnJoinSessionCompleteDelegate.Broadcast(BroadcastResult, ConnectAddress);

	if (BroadcastResult == EOnJoinSessionCompleteResult::Success && !ConnectAddress.IsEmpty())
	{
		TravelToJoinedSession(ConnectAddress);
	}
}

void UMultiplayerSessionsSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (DestroySessionCompleteDelegateHandle.IsValid())
	{
		if (OnlineSessionInterface)
		{
			OnlineSessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
		}
		DestroySessionCompleteDelegateHandle.Reset();
	}

	if (bJoinInviteAfterDestroy)
	{
		const FOnlineSessionSearchResult InviteResult = PendingInviteSessionResult;
		PendingInviteSessionResult = FOnlineSessionSearchResult();
		bJoinInviteAfterDestroy = false;

		if (bWasSuccessful && InviteResult.IsValid())
		{
			JoinSession(InviteResult);
		}
		else
		{
			UE_LOG(LogMultiplayerSessions, Warning, TEXT("Failed to destroy existing session before joining Steam invite."));
			MultiplayerOnJoinSessionCompleteDelegate.Broadcast(EOnJoinSessionCompleteResult::UnknownError, FString());
		}

		MultiplayerOnDestroySessionCompleteDelegate.Broadcast(bWasSuccessful);
		return;
	}
	
	if (PendingRecreateConfig.IsSet())
	{
		const FSessionConfig CreateSessionConfig = PendingRecreateConfig.GetValue();
		PendingRecreateConfig.Reset();

		if (bWasSuccessful)
		{
			CreateSession(CreateSessionConfig.NumPublicConnections, CreateSessionConfig.MatchType);
		}
		else
		{
			MultiplayerOnCreateSessionCompleteDelegate.Broadcast(false);
		}

		MultiplayerOnDestroySessionCompleteDelegate.Broadcast(bWasSuccessful);
		return;
	}

	MultiplayerOnDestroySessionCompleteDelegate.Broadcast(bWasSuccessful);
}

void UMultiplayerSessionsSubsystem::OnSessionUserInviteAccepted(
	bool bWasSuccessful,
	int32 ControllerId,
	FUniqueNetIdPtr /*UserId*/,
	const FOnlineSessionSearchResult& InviteResult)
{
	UE_LOG(
		LogMultiplayerSessions,
		Log,
		TEXT("Steam session invite accepted. ControllerId=%d Success=%d"),
		ControllerId,
		bWasSuccessful);

	if (!bWasSuccessful || !InviteResult.IsValid())
	{
		UE_LOG(LogMultiplayerSessions, Warning, TEXT("Steam invite accepted without a valid session result."));
		MultiplayerOnJoinSessionCompleteDelegate.Broadcast(EOnJoinSessionCompleteResult::UnknownError, FString());
		return;
	}

	JoinAcceptedInvite(InviteResult);
}

/*
 * private function
 */
void UMultiplayerSessionsSubsystem::CreateSessionInternal(const FSessionConfig& SessionConfig)
{
	CreateSessionCompleteDelegateHandle = OnlineSessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);
	
	SessionSettings = MakeShareable(new FOnlineSessionSettings());
	SessionSettings->bIsLANMatch = IsUsingNullSubsystem();
	SessionSettings->NumPublicConnections = SessionConfig.NumPublicConnections;
	SessionSettings->bAllowJoinInProgress = true;
	SessionSettings->bAllowJoinViaPresence = true;
	SessionSettings->bShouldAdvertise = true;
	SessionSettings->bUsesPresence = true;
	SessionSettings->bUseLobbiesIfAvailable = true;
	SessionSettings->bAllowInvites = true;
	SessionSettings->BuildUniqueId = GetBuildUniqueId();
	SessionSettings->Set(
		MultiplayerSessionMetadataKeys::SessionProject,
		FString(FApp::GetProjectName()),
		EOnlineDataAdvertisementType::ViaOnlineServiceAndPing
	);
	SessionSettings->Set(
		MultiplayerSessionMetadataKeys::SessionBuildId,
		GetBuildUniqueId(),
		EOnlineDataAdvertisementType::ViaOnlineServiceAndPing
	);
	SessionSettings->Set(
		FName("MatchType"),
		SessionConfig.MatchType,
		EOnlineDataAdvertisementType::ViaOnlineServiceAndPing
	);
	
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!LocalPlayer || !LocalPlayer->GetPreferredUniqueNetId().IsValid())
	{
		OnlineSessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
		CreateSessionCompleteDelegateHandle.Reset();
		MultiplayerOnCreateSessionCompleteDelegate.Broadcast(false);
		return;
	}
	
	const bool bCreateStart = OnlineSessionInterface->CreateSession(
		*LocalPlayer->GetPreferredUniqueNetId(),NAME_GameSession,*SessionSettings);

	if (!bCreateStart)
	{
		OnlineSessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
		CreateSessionCompleteDelegateHandle.Reset();
		MultiplayerOnCreateSessionCompleteDelegate.Broadcast(false);
	}
}

bool UMultiplayerSessionsSubsystem::RefreshOnlineSessionInterface()
{
	OnlineSessionInterface = Online::GetSessionInterface(GetWorld());
	if (OnlineSessionInterface.IsValid())
	{
		BindSessionInviteAcceptedDelegate();
		return true;
	}
	
	return false;
}

FName UMultiplayerSessionsSubsystem::GetCurrentSubsystemName() const
{
	if (const IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(GetWorld()))
	{
		return OnlineSubsystem->GetSubsystemName();
	}

	return NAME_None;
}

bool UMultiplayerSessionsSubsystem::IsUsingNullSubsystem() const
{
	return GetCurrentSubsystemName() == FName(TEXT("NULL"));
}

void UMultiplayerSessionsSubsystem::BindSessionInviteAcceptedDelegate()
{
	if (!OnlineSessionInterface.IsValid())
	{
		return;
	}

	if (SessionUserInviteAcceptedDelegateHandle.IsValid())
	{
		if (SessionInterfaceWithInviteDelegate == OnlineSessionInterface)
		{
			return;
		}

		ClearSessionInviteAcceptedDelegate();
	}

	SessionInterfaceWithInviteDelegate = OnlineSessionInterface;
	SessionUserInviteAcceptedDelegateHandle =
		OnlineSessionInterface->AddOnSessionUserInviteAcceptedDelegate_Handle(SessionUserInviteAcceptedDelegate);
}

void UMultiplayerSessionsSubsystem::ClearSessionInviteAcceptedDelegate()
{
	if (SessionInterfaceWithInviteDelegate.IsValid() && SessionUserInviteAcceptedDelegateHandle.IsValid())
	{
		SessionInterfaceWithInviteDelegate->ClearOnSessionUserInviteAcceptedDelegate_Handle(SessionUserInviteAcceptedDelegateHandle);
	}

	SessionUserInviteAcceptedDelegateHandle.Reset();
	SessionInterfaceWithInviteDelegate.Reset();
}

void UMultiplayerSessionsSubsystem::JoinAcceptedInvite(const FOnlineSessionSearchResult& InviteResult)
{
	PendingRecreateConfig.Reset();

	if (!RefreshOnlineSessionInterface())
	{
		MultiplayerOnJoinSessionCompleteDelegate.Broadcast(EOnJoinSessionCompleteResult::UnknownError, FString());
		return;
	}

	if (OnlineSessionInterface->GetNamedSession(NAME_GameSession) != nullptr)
	{
		PendingInviteSessionResult = InviteResult;
		bJoinInviteAfterDestroy = true;
		DestroySession();
		return;
	}

	JoinSession(InviteResult);
}

void UMultiplayerSessionsSubsystem::TravelToJoinedSession(const FString& ConnectAddress) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	APlayerController* PlayerController = World->GetFirstPlayerController();
	if (!PlayerController)
	{
		UE_LOG(LogMultiplayerSessions, Warning, TEXT("Cannot travel to joined session: no local player controller."));
		return;
	}

	UE_LOG(LogMultiplayerSessions, Log, TEXT("ClientTravel to joined session: %s"), *ConnectAddress);
	PlayerController->ClientTravel(ConnectAddress, ETravelType::TRAVEL_Absolute);
}

void UMultiplayerSessionsSubsystem::OnPostLoadMapWithWorld(UWorld* LoadedWorld)
{
	if (!bShowLobbyInvitePanelAfterTravel || LoadedWorld == nullptr || LoadedWorld->GetGameInstance() != GetGameInstance())
	{
		return;
	}

	bShowLobbyInvitePanelAfterTravel = false;
	LoadedWorld->GetTimerManager().SetTimerForNextTick(
		FTimerDelegate::CreateUObject(this, &ThisClass::TryShowPendingLobbyInvitePanel));
}

void UMultiplayerSessionsSubsystem::TryShowPendingLobbyInvitePanel()
{
	ShowLobbyInvitePanel();
}
