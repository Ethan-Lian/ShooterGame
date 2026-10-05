#include "LobbyInviteSubsystem.h"

#include "Online.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlinePresenceInterface.h"

DEFINE_LOG_CATEGORY_STATIC(LogLobbyInviteSubsystem, Log, All);

namespace LobbyInvite
{
	const FString SteamDefaultFriendsListName(EFriendsLists::ToString(EFriendsLists::Default));
}

ULobbyInviteSubsystem::ULobbyInviteSubsystem()
	: ReadSteamFriendsCompleteDelegate(FOnReadFriendsListComplete::CreateUObject(this, &ThisClass::OnReadSteamFriendsComplete))
{
	LobbyInviteStatusText = FText::FromString(TEXT("Steam friends not loaded."));
}

void ULobbyInviteSubsystem::RefreshSteamFriendsList()
{
	IOnlineFriendsPtr FriendsInterface = Online::GetFriendsInterface(GetWorld());
	if (!FriendsInterface.IsValid())
	{
		CachedSteamFriends.Reset();
		BroadcastSteamFriendsListUpdated();
		SetLobbyInviteStatus(FText::FromString(TEXT("Steam friends interface is unavailable.")));
		UE_LOG(LogLobbyInviteSubsystem, Warning, TEXT("Cannot refresh Steam friends: friends interface is unavailable."));
		return;
	}

	SetLobbyInviteStatus(FText::FromString(TEXT("Reading Steam friends...")));
	const bool bReadStarted = FriendsInterface->ReadFriendsList(
		0,
		LobbyInvite::SteamDefaultFriendsListName,
		ReadSteamFriendsCompleteDelegate);

	if (!bReadStarted)
	{
		CachedSteamFriends.Reset();
		BroadcastSteamFriendsListUpdated();
		SetLobbyInviteStatus(FText::FromString(TEXT("Failed to start Steam friends refresh.")));
		UE_LOG(LogLobbyInviteSubsystem, Warning, TEXT("ReadFriendsList failed to start."));
	}
}

bool ULobbyInviteSubsystem::SendSteamInviteToFriendByIdString(const FString& FriendIdString)
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

bool ULobbyInviteSubsystem::SendSteamInviteToFriend(const FUniqueNetId& FriendId)
{
	IOnlineSessionPtr OnlineSessionInterface = Online::GetSessionInterface(GetWorld());
	if (!OnlineSessionInterface.IsValid())
	{
		UE_LOG(LogLobbyInviteSubsystem, Warning, TEXT("Cannot send Steam invite: session interface is unavailable."));
		return false;
	}

	const FNamedOnlineSession* CurrentSession = OnlineSessionInterface->GetNamedSession(NAME_GameSession);
	if (CurrentSession == nullptr || !CurrentSession->SessionInfo.IsValid())
	{
		UE_LOG(LogLobbyInviteSubsystem, Warning, TEXT("Cannot send Steam invite: no valid game session exists."));
		return false;
	}

	return OnlineSessionInterface->SendSessionInviteToFriend(0, NAME_GameSession, FriendId);
}

TArray<FSteamFriendInviteEntry> ULobbyInviteSubsystem::GetCachedSteamFriends() const
{
	return CachedSteamFriends;
}

FText ULobbyInviteSubsystem::GetLobbyInviteStatus() const
{
	return LobbyInviteStatusText;
}

void ULobbyInviteSubsystem::OnReadSteamFriendsComplete(
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
		UE_LOG(LogLobbyInviteSubsystem, Warning, TEXT("ReadFriendsList failed. List=%s Error=%s"), *ListName, *ErrorStr);
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

void ULobbyInviteSubsystem::BroadcastSteamFriendsListUpdated()
{
	OnSteamFriendsListUpdated.Broadcast(CachedSteamFriends);
}

void ULobbyInviteSubsystem::SetLobbyInviteStatus(const FText& StatusText)
{
	LobbyInviteStatusText = StatusText;
	OnLobbyInviteStatusChanged.Broadcast(StatusText);
}
