#pragma once

#include "CoreMinimal.h"
#include "Interfaces/OnlineFriendsInterface.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LobbyInviteSubsystem.generated.h"

USTRUCT(BlueprintType)
struct MULTIPLAYERSESSION_API FSteamFriendInviteEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Multiplayer Sessions|Lobby Invite")
	FString DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Multiplayer Sessions|Lobby Invite")
	FString FriendIdString;

	UPROPERTY(BlueprintReadOnly, Category = "Multiplayer Sessions|Lobby Invite")
	bool bIsOnline = false;

	FUniqueNetIdPtr FriendId;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSteamFriendsListUpdated, const TArray<FSteamFriendInviteEntry>&, Friends);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLobbyInviteStatusChanged, FText, StatusText);

UCLASS()
class MULTIPLAYERSESSION_API ULobbyInviteSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	ULobbyInviteSubsystem();
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Multiplayer Sessions|Lobby Invite")
	void RefreshSteamFriendsList();

	UFUNCTION(BlueprintCallable, Category = "Multiplayer Sessions|Lobby Invite")
	bool SendSteamInviteToFriendByIdString(const FString& FriendIdString);

	bool SendSteamInviteToFriend(const FUniqueNetId& FriendId);

	UFUNCTION(BlueprintPure, Category = "Multiplayer Sessions|Lobby Invite")
	TArray<FSteamFriendInviteEntry> GetCachedSteamFriends() const;

	UFUNCTION(BlueprintPure, Category = "Multiplayer Sessions|Lobby Invite")
	FText GetLobbyInviteStatus() const;

	bool IsRefreshingFriends() const { return ActiveRefreshId != 0; }

	UPROPERTY(BlueprintAssignable, Category = "Multiplayer Sessions|Lobby Invite")
	FOnSteamFriendsListUpdated OnSteamFriendsListUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Multiplayer Sessions|Lobby Invite")
	FOnLobbyInviteStatusChanged OnLobbyInviteStatusChanged;

private:
	void OnReadSteamFriendsComplete(int32 LocalUserNum, bool bWasSuccessful, const FString& ListName, const FString& ErrorStr, uint64 RefreshId);
	void BroadcastSteamFriendsListUpdated();
	void SetLobbyInviteStatus(const FText& StatusText);

	IOnlineFriendsPtr RefreshFriendsInterface;
	uint64 ActiveRefreshId = 0;
	uint64 NextRefreshId = 0;
	bool bDeinitializing = false;

	TArray<FSteamFriendInviteEntry> CachedSteamFriends;
	FText LobbyInviteStatusText;
};
