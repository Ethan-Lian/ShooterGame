#include "MultiplayerSessionsSubsystem.h"
#include "Online.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystemUtils.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "Interfaces/OnlineExternalUIInterface.h"
#include "Interfaces/OnlinePresenceInterface.h"
#include "Misc/App.h"
#include "TimerManager.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogMultiplayerSessions, Log, All);

namespace MultiplayerSessionMetadataKeys
{
	const FName SessionProject(TEXT("SessionProject"));
	const FName SessionBuildId(TEXT("SessionBuildId"));
}

namespace MultiplayerSessionUI
{
	const FString SteamDefaultFriendsListName(EFriendsLists::ToString(EFriendsLists::Default));
	const FString GameplayMapPath(TEXT("/Game/ShooterGameContent/Maps/GameLevel?listen"));
}

UMultiplayerSessionsSubsystem::UMultiplayerSessionsSubsystem() :   
	CreateSessionCompleteDelegate(FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateSessionComplete)),
	JoinSessionCompleteDelegate(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionComplete)),
	DestroySessionCompleteDelegate(FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroySessionComplete)),
	StartSessionCompleteDelegate(FOnStartSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnStartSessionComplete)),
	SessionUserInviteAcceptedDelegate(FOnSessionUserInviteAcceptedDelegate::CreateUObject(this, &ThisClass::OnSessionUserInviteAccepted)),
	ReadSteamFriendsCompleteDelegate(FOnReadFriendsListComplete::CreateUObject(this, &ThisClass::OnReadSteamFriendsComplete))
{
	LobbyInviteStatusText = FText::FromString(TEXT("Steam friends not loaded."));
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

	HideLobbyInvitePanel();

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

void UMultiplayerSessionsSubsystem::StartSession()
{
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

void UMultiplayerSessionsSubsystem::RefreshSteamFriendsList()
{
	IOnlineFriendsPtr FriendsInterface = Online::GetFriendsInterface(GetWorld());
	if (!FriendsInterface.IsValid())
	{
		CachedSteamFriends.Reset();
		BroadcastSteamFriendsListUpdated();
		SetLobbyInviteStatus(FText::FromString(TEXT("Steam friends interface is unavailable.")));
		UE_LOG(LogMultiplayerSessions, Warning, TEXT("Cannot refresh Steam friends: friends interface is unavailable."));
		return;
	}

	SetLobbyInviteStatus(FText::FromString(TEXT("Reading Steam friends...")));
	const bool bReadStarted = FriendsInterface->ReadFriendsList(
		0,
		MultiplayerSessionUI::SteamDefaultFriendsListName,
		ReadSteamFriendsCompleteDelegate);

	if (!bReadStarted)
	{
		CachedSteamFriends.Reset();
		BroadcastSteamFriendsListUpdated();
		SetLobbyInviteStatus(FText::FromString(TEXT("Failed to start Steam friends refresh.")));
		UE_LOG(LogMultiplayerSessions, Warning, TEXT("ReadFriendsList failed to start."));
	}
}

bool UMultiplayerSessionsSubsystem::SendSteamInviteToFriendByIndex(int32 FriendIndex)
{
	if (!CachedSteamFriends.IsValidIndex(FriendIndex) || !CachedSteamFriends[FriendIndex].FriendId.IsValid())
	{
		SetLobbyInviteStatus(FText::FromString(TEXT("Invalid friend selection.")));
		return false;
	}

	const FSteamFriendInviteEntry& FriendEntry = CachedSteamFriends[FriendIndex];
	const bool bSent = SendSteamInviteToFriend(*FriendEntry.FriendId);
	SetLobbyInviteStatus(FText::FromString(FString::Printf(
		TEXT("%s %s."),
		bSent ? TEXT("Invite sent to") : TEXT("Failed to invite"),
		*FriendEntry.DisplayName)));
	return bSent;
}

bool UMultiplayerSessionsSubsystem::SendSteamInviteToFriendByIdString(const FString& FriendIdString)
{
	const FSteamFriendInviteEntry* FriendEntry = CachedSteamFriends.FindByPredicate(
		[&FriendIdString](const FSteamFriendInviteEntry& Candidate)
		{
			return Candidate.FriendIdString == FriendIdString;
		});

	if (FriendEntry == nullptr || !FriendEntry->FriendId.IsValid())
	{
		SetLobbyInviteStatus(FText::FromString(TEXT("Invalid friend selection.")));
		return false;
	}

	const bool bSent = SendSteamInviteToFriend(*FriendEntry->FriendId);
	SetLobbyInviteStatus(FText::FromString(FString::Printf(
		TEXT("%s %s."),
		bSent ? TEXT("Invite sent to") : TEXT("Failed to invite"),
		*FriendEntry->DisplayName)));
	return bSent;
}

bool UMultiplayerSessionsSubsystem::SendSteamInviteToFriend(const FUniqueNetId& FriendId)
{
	if (!RefreshOnlineSessionInterface())
	{
		UE_LOG(LogMultiplayerSessions, Warning, TEXT("Cannot send Steam invite: session interface is unavailable."));
		return false;
	}

	const FNamedOnlineSession* CurrentSession = OnlineSessionInterface->GetNamedSession(NAME_GameSession);
	if (CurrentSession == nullptr || !CurrentSession->SessionInfo.IsValid())
	{
		UE_LOG(LogMultiplayerSessions, Warning, TEXT("Cannot send Steam invite: no valid game session exists."));
		return false;
	}

	return OnlineSessionInterface->SendSessionInviteToFriend(0, NAME_GameSession, FriendId);
}

TArray<FSteamFriendInviteEntry> UMultiplayerSessionsSubsystem::GetCachedSteamFriends() const
{
	return CachedSteamFriends;
}

FText UMultiplayerSessionsSubsystem::GetLobbyInviteStatus() const
{
	return LobbyInviteStatusText;
}

bool UMultiplayerSessionsSubsystem::CanShowHostInvitePanel() const
{
	UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_Client)
	{
		return false;
	}

	const IOnlineSessionPtr SessionInterface = Online::GetSessionInterface(World);
	return SessionInterface.IsValid() && SessionInterface->GetNamedSession(NAME_GameSession) != nullptr;
}

bool UMultiplayerSessionsSubsystem::ConsumePendingLobbyInvitePanelRequest()
{
	if (!bPendingLobbyInvitePanelRequest)
	{
		return false;
	}

	if (!CanShowHostInvitePanel())
	{
		bPendingLobbyInvitePanelRequest = false;
		return false;
	}

	bPendingLobbyInvitePanelRequest = false;
	return true;
}

void UMultiplayerSessionsSubsystem::ShowLobbyInvitePanel()
{
	if (!CanShowHostInvitePanel())
	{
		UE_LOG(LogMultiplayerSessions, Verbose, TEXT("Skipping lobby invite panel: local player is not the host or no session exists."));
		return;
	}

	bPendingLobbyInvitePanelRequest = true;
	OnLobbyInvitePanelRequested.Broadcast();
	RefreshSteamFriendsList();
}

void UMultiplayerSessionsSubsystem::HideLobbyInvitePanel()
{
	bPendingLobbyInvitePanelRequest = false;
}

bool UMultiplayerSessionsSubsystem::StartHostedGame()
{
	UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_Client)
	{
		SetLobbyInviteStatus(FText::FromString(TEXT("Only the host can start the game.")));
		return false;
	}

	HideLobbyInvitePanel();

	if (AGameModeBase* GameMode = World->GetAuthGameMode())
	{
		GameMode->bUseSeamlessTravel = true;
	}

	return World->ServerTravel(MultiplayerSessionUI::GameplayMapPath);
}

void UMultiplayerSessionsSubsystem::RequestShowLobbyInvitePanelAfterTravel()
{
	bShowLobbyInvitePanelAfterTravel = true;
}

void UMultiplayerSessionsSubsystem::RequestOpenInviteUIAfterTravel()
{
	RequestShowLobbyInvitePanelAfterTravel();
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

void UMultiplayerSessionsSubsystem::OnStartSessionComplete(FName SessionName, bool bWasSuccessful)
{
	
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

void UMultiplayerSessionsSubsystem::OnReadSteamFriendsComplete(
	int32 LocalUserNum,
	bool bWasSuccessful,
	const FString& ListName,
	const FString& ErrorStr)
{
	CachedSteamFriends.Reset();

	if (!bWasSuccessful)
	{
		BroadcastSteamFriendsListUpdated();
		SetLobbyInviteStatus(FText::FromString(FString::Printf(TEXT("Steam friends refresh failed: %s"), *ErrorStr)));
		UE_LOG(LogMultiplayerSessions, Warning, TEXT("ReadFriendsList failed. List=%s Error=%s"), *ListName, *ErrorStr);
		return;
	}

	IOnlineFriendsPtr FriendsInterface = Online::GetFriendsInterface(GetWorld());
	if (!FriendsInterface.IsValid())
	{
		BroadcastSteamFriendsListUpdated();
		SetLobbyInviteStatus(FText::FromString(TEXT("Steam friends interface became unavailable.")));
		return;
	}

	TArray<TSharedRef<FOnlineFriend>> Friends;
	if (!FriendsInterface->GetFriendsList(LocalUserNum, ListName, Friends))
	{
		BroadcastSteamFriendsListUpdated();
		SetLobbyInviteStatus(FText::FromString(TEXT("Steam friends list is not available yet.")));
		return;
	}

	for (const TSharedRef<FOnlineFriend>& Friend : Friends)
	{
		FSteamFriendInviteEntry FriendEntry;
		FriendEntry.DisplayName = Friend->GetDisplayName();
		FriendEntry.FriendId = Friend->GetUserId();
		FriendEntry.FriendIdString = FriendEntry.FriendId.IsValid() ? FriendEntry.FriendId->ToString() : FString();
		FriendEntry.bIsOnline = Friend->GetPresence().bIsOnline;
		CachedSteamFriends.Add(FriendEntry);
	}

	CachedSteamFriends.Sort([](const FSteamFriendInviteEntry& Left, const FSteamFriendInviteEntry& Right)
	{
		if (Left.bIsOnline != Right.bIsOnline)
		{
			return Left.bIsOnline;
		}

		return Left.DisplayName < Right.DisplayName;
	});

	BroadcastSteamFriendsListUpdated();
	SetLobbyInviteStatus(FText::FromString(FString::Printf(TEXT("%d Steam friends loaded."), CachedSteamFriends.Num())));
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

void UMultiplayerSessionsSubsystem::BroadcastSteamFriendsListUpdated()
{
	OnSteamFriendsListUpdated.Broadcast(CachedSteamFriends);
}

void UMultiplayerSessionsSubsystem::SetLobbyInviteStatus(const FText& StatusText)
{
	LobbyInviteStatusText = StatusText;
	OnLobbyInviteStatusChanged.Broadcast(StatusText);
}
