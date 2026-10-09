#include "LobbyInviteSubsystem.h"

#include "Engine/GameInstance.h"
#include "Interfaces/OnlinePresenceInterface.h"
#include "MultiplayerSessionFlowSubsystem.h"
#include "Online.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystemUtils.h"

#define LOCTEXT_NAMESPACE "LobbyInvite"

DEFINE_LOG_CATEGORY_STATIC(LogLobbyInviteSubsystem, Log, All);

namespace
{
	const FString SteamDefaultFriendsListName(EFriendsLists::ToString(EFriendsLists::Default));
}

ULobbyInviteSubsystem::ULobbyInviteSubsystem()
{
	LobbyInviteStatusText = LOCTEXT("NotLoaded", "尚未加载 Steam 好友。");
}

void ULobbyInviteSubsystem::Deinitialize()
{
	bDeinitializing = true;
	ActiveRefreshId = 0;
	RefreshFriendsInterface.Reset();
	Super::Deinitialize();
}

void ULobbyInviteSubsystem::RefreshSteamFriendsList()
{
	if (bDeinitializing || ActiveRefreshId != 0)
	{
		return;
	}
	const uint64 RefreshId = ++NextRefreshId;
	ActiveRefreshId = RefreshId;
	RefreshFriendsInterface = GetWorld() ? Online::GetFriendsInterface(GetWorld()) : nullptr;
	if (!RefreshFriendsInterface.IsValid())
	{
		OnReadSteamFriendsComplete(0, false, SteamDefaultFriendsListName, TEXT("Steam friends interface is unavailable."), RefreshId);
		return;
	}
	const IOnlineFriendsPtr FriendsInterface = RefreshFriendsInterface;
	SetLobbyInviteStatus(LOCTEXT("Reading", "正在加载 Steam 好友…"));
	if (bDeinitializing || ActiveRefreshId != RefreshId)
	{
		return;
	}
	const bool bReadStarted = FriendsInterface->ReadFriendsList(0, SteamDefaultFriendsListName,
		FOnReadFriendsListComplete::CreateUObject(this, &ThisClass::OnReadSteamFriendsComplete, RefreshId));
	if (!bReadStarted && ActiveRefreshId == RefreshId && !bDeinitializing)
	{
		OnReadSteamFriendsComplete(0, false, SteamDefaultFriendsListName, TEXT("Could not start reading Steam friends."), RefreshId);
	}
}

bool ULobbyInviteSubsystem::SendSteamInviteToFriendByIdString(const FString& FriendIdString)
{
	const FSteamFriendInviteEntry* FriendEntry = CachedSteamFriends.FindByPredicate(
		[&FriendIdString](const FSteamFriendInviteEntry& Candidate)
		{
			return Candidate.FriendIdString == FriendIdString;
		});
	if (!FriendEntry || !FriendEntry->FriendId.IsValid())
	{
		SetLobbyInviteStatus(LOCTEXT("InvalidFriend", "好友信息已失效，请刷新列表。"));
		return false;
	}
	// Platform calls may reenter application code; do not retain a pointer into the mutable cache.
	const FString DisplayName = FriendEntry->DisplayName;
	const FUniqueNetIdPtr FriendId = FriendEntry->FriendId;
	const bool bSent = SendSteamInviteToFriend(*FriendId);
	SetLobbyInviteStatus(FText::Format(bSent ? LOCTEXT("InviteSent", "已向 {0} 发送邀请。")
		: LOCTEXT("InviteFailed", "向 {0} 发送邀请失败，请稍后重试。"), FText::FromString(DisplayName)));
	return bSent;
}

bool ULobbyInviteSubsystem::SendSteamInviteToFriend(const FUniqueNetId& FriendId)
{
	const UMultiplayerSessionFlowSubsystem* Flow = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UMultiplayerSessionFlowSubsystem>() : nullptr;
	if (bDeinitializing || !Flow || !Flow->CanInviteFriends())
	{
		return false;
	}
	const IOnlineSessionPtr SessionInterface = Online::GetSessionInterface(GetWorld());
	const FNamedOnlineSession* Session = SessionInterface.IsValid() ? SessionInterface->GetNamedSession(NAME_GameSession) : nullptr;
	return Session && Session->SessionInfo.IsValid()
		&& SessionInterface->SendSessionInviteToFriend(0, NAME_GameSession, FriendId);
}

TArray<FSteamFriendInviteEntry> ULobbyInviteSubsystem::GetCachedSteamFriends() const
{
	return CachedSteamFriends;
}

FText ULobbyInviteSubsystem::GetLobbyInviteStatus() const
{
	return LobbyInviteStatusText;
}

void ULobbyInviteSubsystem::OnReadSteamFriendsComplete(int32 LocalUserNum, bool bWasSuccessful,
	const FString& ListName, const FString& ErrorStr, uint64 RefreshId)
{
	if (bDeinitializing || ActiveRefreshId != RefreshId)
	{
		return;
	}
	TArray<TSharedRef<FOnlineFriend>> Friends;
	bWasSuccessful = bWasSuccessful && RefreshFriendsInterface.IsValid()
		&& RefreshFriendsInterface->GetFriendsList(LocalUserNum, ListName, Friends);
	CachedSteamFriends.Reset();
	if (bWasSuccessful)
	{
		for (const TSharedRef<FOnlineFriend>& Friend : Friends)
		{
			FSteamFriendInviteEntry Entry;
			Entry.DisplayName = Friend->GetDisplayName();
			Entry.FriendId = Friend->GetUserId();
			Entry.FriendIdString = Entry.FriendId->ToString();
			Entry.bIsOnline = Friend->GetPresence().bIsOnline;
			CachedSteamFriends.Add(MoveTemp(Entry));
		}
		CachedSteamFriends.Sort([](const FSteamFriendInviteEntry& Left, const FSteamFriendInviteEntry& Right)
		{
			return Left.bIsOnline != Right.bIsOnline ? Left.bIsOnline : Left.DisplayName < Right.DisplayName;
		});
		LobbyInviteStatusText = FText::Format(LOCTEXT("Loaded", "已加载 {0} 位好友，可以发送邀请。"), FText::AsNumber(CachedSteamFriends.Num()));
	}
	else
	{
		LobbyInviteStatusText = LOCTEXT("ReadFailed", "好友列表加载失败，请确认 Steam 已登录后重试。");
		UE_LOG(LogLobbyInviteSubsystem, Warning, TEXT("Friends refresh failed: %s"), *ErrorStr);
	}
	ActiveRefreshId = 0;
	RefreshFriendsInterface.Reset();
	const FText CompletedStatus = LobbyInviteStatusText;
	BroadcastSteamFriendsListUpdated();
	if (!bDeinitializing && NextRefreshId == RefreshId)
	{
		OnLobbyInviteStatusChanged.Broadcast(CompletedStatus);
	}
}

void ULobbyInviteSubsystem::BroadcastSteamFriendsListUpdated()
{
	const TArray<FSteamFriendInviteEntry> Snapshot = CachedSteamFriends;
	OnSteamFriendsListUpdated.Broadcast(Snapshot);
}

void ULobbyInviteSubsystem::SetLobbyInviteStatus(const FText& StatusText)
{
	if (!bDeinitializing)
	{
		LobbyInviteStatusText = StatusText;
		OnLobbyInviteStatusChanged.Broadcast(StatusText);
	}
}

#undef LOCTEXT_NAMESPACE
