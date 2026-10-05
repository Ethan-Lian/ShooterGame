#include "MultiplayerSessionsSubsystem.h"

#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Interfaces/OnlineExternalUIInterface.h"
#include "Misc/App.h"
#include "Online.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogMultiplayerSessions, Log, All);

void UMultiplayerSessionsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	bDeinitializing = false;
	RefreshOnlineSessionInterface();
}

void UMultiplayerSessionsSubsystem::Deinitialize()
{
	bDeinitializing = true;
	ClearActiveOperation();
	ClearSessionInviteAcceptedDelegate();
	OnlineSessionInterface.Reset();
	Super::Deinitialize();
}

bool UMultiplayerSessionsSubsystem::CreateSession(int32 NumPublicConnections, const FString& MatchType)
{
	UWorld* World = GetWorld();
	const ULocalPlayer* LocalPlayer = World ? World->GetFirstLocalPlayerFromController() : nullptr;
	if (!LocalPlayer || !LocalPlayer->GetPreferredUniqueNetId().IsValid() || NumPublicConnections < 1)
	{
		return false;
	}
	const FUniqueNetIdRepl LocalUserId = LocalPlayer->GetPreferredUniqueNetId();
	if (!RefreshOnlineSessionInterface() || HasSession())
	{
		return false;
	}
	const uint64 OperationId = BeginOperation(EOperation::Create);
	if (OperationId == 0)
	{
		return false;
	}

	FOnlineSessionSettings Settings;
	const IOnlineSubsystem* Subsystem = Online::GetSubsystem(World);
	Settings.bIsLANMatch = Subsystem && Subsystem->GetSubsystemName() == FName(TEXT("NULL"));
	Settings.NumPublicConnections = NumPublicConnections;
	Settings.bAllowJoinInProgress = true;
	Settings.bAllowJoinViaPresence = true;
	Settings.bShouldAdvertise = true;
	Settings.bUsesPresence = true;
	Settings.bUseLobbiesIfAvailable = true;
	Settings.bAllowInvites = true;
	Settings.BuildUniqueId = GetBuildUniqueId();
	Settings.Set(FName(TEXT("SessionProject")), FString(FApp::GetProjectName()), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	Settings.Set(FName(TEXT("SessionBuildId")), GetBuildUniqueId(), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	Settings.Set(FName(TEXT("MatchType")), MatchType, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	const IOnlineSessionPtr SessionInterface = OperationSessionInterface;
	OperationDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateSessionComplete, OperationId));
	const bool bStarted = SessionInterface->CreateSession(*LocalUserId, NAME_GameSession, Settings);
	// Steam can invoke completion synchronously and then return false.
	if (!bStarted && OwnsOperation(EOperation::Create, OperationId))
	{
		OnCreateSessionComplete(NAME_GameSession, false, OperationId);
	}
	return true;
}

bool UMultiplayerSessionsSubsystem::JoinSession(const FOnlineSessionSearchResult& SessionResult)
{
	UWorld* World = GetWorld();
	const ULocalPlayer* LocalPlayer = World ? World->GetFirstLocalPlayerFromController() : nullptr;
	if (!SessionResult.IsValid() || !LocalPlayer || !LocalPlayer->GetPreferredUniqueNetId().IsValid())
	{
		return false;
	}
	const FUniqueNetIdRepl LocalUserId = LocalPlayer->GetPreferredUniqueNetId();
	if (!RefreshOnlineSessionInterface() || HasSession())
	{
		return false;
	}
	const uint64 OperationId = BeginOperation(EOperation::Join);
	if (OperationId == 0)
	{
		return false;
	}

	const IOnlineSessionPtr SessionInterface = OperationSessionInterface;
	OperationDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionComplete, OperationId));
	const bool bStarted = SessionInterface->JoinSession(*LocalUserId, NAME_GameSession, SessionResult);
	if (!bStarted && OwnsOperation(EOperation::Join, OperationId))
	{
		OnJoinSessionComplete(NAME_GameSession, EOnJoinSessionCompleteResult::UnknownError, OperationId);
	}
	return true;
}

bool UMultiplayerSessionsSubsystem::DestroySession()
{
	const uint64 OperationId = BeginOperation(EOperation::Destroy);
	if (OperationId == 0)
	{
		return false;
	}
	const IOnlineSessionPtr SessionInterface = OperationSessionInterface;
	if (!SessionInterface->GetNamedSession(NAME_GameSession))
	{
		OnDestroySessionComplete(NAME_GameSession, true, OperationId);
		return true;
	}

	OperationDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
		FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroySessionComplete, OperationId));
	const bool bStarted = SessionInterface->DestroySession(NAME_GameSession);
	if (!bStarted && OwnsOperation(EOperation::Destroy, OperationId))
	{
		OnDestroySessionComplete(NAME_GameSession, false, OperationId);
	}
	return true;
}

uint64 UMultiplayerSessionsSubsystem::BeginOperation(EOperation Operation)
{
	if (bDeinitializing || IsBusy() || !RefreshOnlineSessionInterface())
	{
		return 0;
	}
	ActiveOperation = Operation;
	ActiveOperationId = ++NextOperationId;
	OperationSessionInterface = OnlineSessionInterface;
	return ActiveOperationId;
}

bool UMultiplayerSessionsSubsystem::OwnsOperation(EOperation Operation, uint64 OperationId) const
{
	return !bDeinitializing && ActiveOperation == Operation && ActiveOperationId == OperationId;
}

void UMultiplayerSessionsSubsystem::ClearActiveOperation()
{
	if (OperationSessionInterface.IsValid() && OperationDelegateHandle.IsValid())
	{
		switch (ActiveOperation)
		{
		case EOperation::Create:
			OperationSessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(OperationDelegateHandle);
			break;
		case EOperation::Join:
			OperationSessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(OperationDelegateHandle);
			break;
		case EOperation::Destroy:
			OperationSessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(OperationDelegateHandle);
			break;
		default:
			break;
		}
	}
	OperationDelegateHandle.Reset();
	OperationSessionInterface.Reset();
	ActiveOperation = EOperation::None;
	ActiveOperationId = 0;
}

void UMultiplayerSessionsSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful, uint64 OperationId)
{
	if (SessionName != NAME_GameSession || !OwnsOperation(EOperation::Create, OperationId))
	{
		return;
	}
	ClearActiveOperation();
	MultiplayerOnCreateSessionCompleteDelegate.Broadcast(bWasSuccessful);
}

void UMultiplayerSessionsSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result, uint64 OperationId)
{
	if (SessionName != NAME_GameSession || !OwnsOperation(EOperation::Join, OperationId))
	{
		return;
	}
	FString Address;
	if (Result == EOnJoinSessionCompleteResult::Success
		&& (!OperationSessionInterface->GetResolvedConnectString(SessionName, Address) || Address.IsEmpty()))
	{
		Result = EOnJoinSessionCompleteResult::CouldNotRetrieveAddress;
	}
	ClearActiveOperation();
	MultiplayerOnJoinSessionCompleteDelegate.Broadcast(Result, Address);
}

void UMultiplayerSessionsSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful, uint64 OperationId)
{
	if (SessionName != NAME_GameSession || !OwnsOperation(EOperation::Destroy, OperationId))
	{
		return;
	}
	// Verify the backend removed the local entry before admitting another create or join.
	bWasSuccessful = bWasSuccessful && OperationSessionInterface->GetNamedSession(NAME_GameSession) == nullptr;
	ClearActiveOperation();
	MultiplayerOnDestroySessionCompleteDelegate.Broadcast(bWasSuccessful);
}

bool UMultiplayerSessionsSubsystem::HasSession() const
{
	return OnlineSessionInterface.IsValid() && OnlineSessionInterface->GetNamedSession(NAME_GameSession) != nullptr;
}

bool UMultiplayerSessionsSubsystem::RefreshOnlineSessionInterface()
{
	if (bDeinitializing)
	{
		return false;
	}
	// Keep the known interface during world replacement so pending cleanup retains its owner.
	if (UWorld* World = GetWorld())
	{
		const IOnlineSessionPtr CurrentInterface = Online::GetSessionInterface(World);
		if (CurrentInterface.IsValid())
		{
			OnlineSessionInterface = CurrentInterface;
		}
	}
	if (!OnlineSessionInterface.IsValid())
	{
		return false;
	}
	if (SessionInterfaceWithInviteDelegate != OnlineSessionInterface)
	{
		ClearSessionInviteAcceptedDelegate();
		SessionInterfaceWithInviteDelegate = OnlineSessionInterface;
		SessionUserInviteAcceptedDelegateHandle = OnlineSessionInterface->AddOnSessionUserInviteAcceptedDelegate_Handle(
			FOnSessionUserInviteAcceptedDelegate::CreateUObject(this, &ThisClass::OnSessionUserInviteAccepted));
	}
	return true;
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

void UMultiplayerSessionsSubsystem::OnSessionUserInviteAccepted(bool bWasSuccessful, int32 ControllerId,
	FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& InviteResult)
{
	if (bDeinitializing)
	{
		return;
	}
	const ULocalPlayer* LocalPlayer = GetWorld() ? GetWorld()->GetFirstLocalPlayerFromController() : nullptr;
	if (!LocalPlayer || (UserId.IsValid() && LocalPlayer->GetPreferredUniqueNetId().IsValid()
		&& *UserId != *LocalPlayer->GetPreferredUniqueNetId()))
	{
		return;
	}
	UE_LOG(LogMultiplayerSessions, Log, TEXT("Session invite accepted: controller=%d success=%d"), ControllerId, bWasSuccessful);
	OnInviteAccepted.Broadcast(bWasSuccessful && InviteResult.IsValid(), InviteResult);
}

bool UMultiplayerSessionsSubsystem::ShowSteamInviteUI()
{
	if (IsBusy() || !RefreshOnlineSessionInterface() || !GetWorld())
	{
		return false;
	}
	const FNamedOnlineSession* Session = OnlineSessionInterface->GetNamedSession(NAME_GameSession);
	if (!Session || !Session->SessionInfo.IsValid())
	{
		return false;
	}
	const IOnlineExternalUIPtr ExternalUI = Online::GetExternalUIInterface(GetWorld());
	return ExternalUI.IsValid() && ExternalUI->ShowInviteUI(0, NAME_GameSession);
}
