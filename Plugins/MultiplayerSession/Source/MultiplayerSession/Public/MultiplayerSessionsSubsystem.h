#pragma once

#include "CoreMinimal.h"
#include "Interfaces/OnlineFriendsInterface.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MultiplayerSessionsSubsystem.generated.h"


/*
 * Declaring our own custom delegate for the menu class to bind callbacks
 */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnCreateSessionComplete,bool,bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnDestroySessionComplete,bool,bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnStartSessionComplete,bool,bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_TwoParams(FMultiplayerOnJoinSessionComplete,EOnJoinSessionCompleteResult::Type Result,const FString& Address);


USTRUCT()
struct FSessionConfig
{
	GENERATED_BODY()
	
	int32 NumPublicConnections = 4;
	FString MatchType = TEXT("FreeForAll");
};

USTRUCT(BlueprintType)
struct FSteamFriendInviteEntry
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

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbyInvitePanelRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSteamFriendsListUpdated, const TArray<FSteamFriendInviteEntry>&, Friends);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLobbyInviteStatusChanged, FText, StatusText);

UCLASS()
class MULTIPLAYERSESSION_API UMultiplayerSessionsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	UMultiplayerSessionsSubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	/*
	 * To handle session functionality.The Menu class will call these
	 */
	
	void CreateSession(int32 NumPublicConnections, FString MatchType);
	void JoinSession(const FOnlineSessionSearchResult& SessionResult);
	void DestroySession();
	void StartSession();

	// Opens the platform invite UI for the current Steam lobby session.
	UFUNCTION(BlueprintCallable, Category = "Multiplayer Sessions")
	bool ShowSteamInviteUI();

	// Reads the local Steam account's default friends list.
	UFUNCTION(BlueprintCallable, Category = "Multiplayer Sessions")
	void RefreshSteamFriendsList();

	// Sends a Steam session invite to a cached friend row.
	UFUNCTION(BlueprintCallable, Category = "Multiplayer Sessions")
	bool SendSteamInviteToFriendByIndex(int32 FriendIndex);

	UFUNCTION(BlueprintCallable, Category = "Multiplayer Sessions")
	bool SendSteamInviteToFriendByIdString(const FString& FriendIdString);

	bool SendSteamInviteToFriend(const FUniqueNetId& FriendId);

	UFUNCTION(BlueprintPure, Category = "Multiplayer Sessions")
	TArray<FSteamFriendInviteEntry> GetCachedSteamFriends() const;

	UFUNCTION(BlueprintPure, Category = "Multiplayer Sessions")
	FText GetLobbyInviteStatus() const;

	UFUNCTION(BlueprintPure, Category = "Multiplayer Sessions")
	bool CanShowHostInvitePanel() const;

	UFUNCTION(BlueprintCallable, Category = "Multiplayer Sessions")
	bool ConsumePendingLobbyInvitePanelRequest();

	// Requests that Blueprint-owned lobby UI be shown.
	UFUNCTION(BlueprintCallable, Category = "Multiplayer Sessions")
	void ShowLobbyInvitePanel();

	UFUNCTION(BlueprintCallable, Category = "Multiplayer Sessions")
	void HideLobbyInvitePanel();

	// Starts the hosted lobby by server-traveling to the gameplay map.
	UFUNCTION(BlueprintCallable, Category = "Multiplayer Sessions")
	bool StartHostedGame();

	void RequestShowLobbyInvitePanelAfterTravel();

	// Defers opening the invite UI until the next map finishes loading.
	void RequestOpenInviteUIAfterTravel();
	
	/*
	 * Our own custom delegate
	 */
	FMultiplayerOnCreateSessionComplete MultiplayerOnCreateSessionCompleteDelegate;
	FMultiplayerOnJoinSessionComplete MultiplayerOnJoinSessionCompleteDelegate;
	FMultiplayerOnDestroySessionComplete MultiplayerOnDestroySessionCompleteDelegate;
	FMultiplayerOnStartSessionComplete MultiplayerOnStartSessionCompleteDelegate;

	UPROPERTY(BlueprintAssignable, Category = "Multiplayer Sessions|Lobby Invite")
	FOnLobbyInvitePanelRequested OnLobbyInvitePanelRequested;

	UPROPERTY(BlueprintAssignable, Category = "Multiplayer Sessions|Lobby Invite")
	FOnSteamFriendsListUpdated OnSteamFriendsListUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Multiplayer Sessions|Lobby Invite")
	FOnLobbyInviteStatusChanged OnLobbyInviteStatusChanged;

protected:
	
	/*
	 * Callback function
	 *  Internal callbacks for the delegates we'll add to the Online Session Interface delegate list.
	 */
	
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void OnStartSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnSessionUserInviteAccepted(bool bWasSuccessful, int32 ControllerId, FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& InviteResult);
	void OnReadSteamFriendsComplete(int32 LocalUserNum, bool bWasSuccessful, const FString& ListName, const FString& ErrorStr);
	
private:
	// Resolve the correct OSS session interface for the active world context.
	bool RefreshOnlineSessionInterface();
	FName GetCurrentSubsystemName() const;
	bool IsUsingNullSubsystem() const;
	void BindSessionInviteAcceptedDelegate();
	void ClearSessionInviteAcceptedDelegate();
	void JoinAcceptedInvite(const FOnlineSessionSearchResult& InviteResult);
	void TravelToJoinedSession(const FString& ConnectAddress) const;
	void OnPostLoadMapWithWorld(UWorld* LoadedWorld);
	void TryShowPendingLobbyInvitePanel();
	void BroadcastSteamFriendsListUpdated();
	void SetLobbyInviteStatus(const FText& StatusText);

	// Actually issues the OSS create call once we're ready to create.
	void CreateSessionInternal(const FSessionConfig& SessionConfig);
	
private:
	IOnlineSessionPtr OnlineSessionInterface;
	
	TSharedPtr<FOnlineSessionSettings> SessionSettings;
	
	/*
	 * Delegate and handle
	 */
	
	FOnCreateSessionCompleteDelegate CreateSessionCompleteDelegate;
	FDelegateHandle CreateSessionCompleteDelegateHandle;
	
	FOnJoinSessionCompleteDelegate JoinSessionCompleteDelegate;
	FDelegateHandle JoinSessionCompleteDelegateHandle;
	
	FOnDestroySessionCompleteDelegate DestroySessionCompleteDelegate;
	FDelegateHandle DestroySessionCompleteDelegateHandle;
	
	FOnStartSessionCompleteDelegate StartSessionCompleteDelegate;
	FDelegateHandle StartSessionCompleteDelegateHandle;

	FOnSessionUserInviteAcceptedDelegate SessionUserInviteAcceptedDelegate;
	FDelegateHandle SessionUserInviteAcceptedDelegateHandle;
	IOnlineSessionPtr SessionInterfaceWithInviteDelegate;

	FOnReadFriendsListComplete ReadSteamFriendsCompleteDelegate;

	FDelegateHandle PostLoadMapWithWorldDelegateHandle;

	TArray<FSteamFriendInviteEntry> CachedSteamFriends;
	FText LobbyInviteStatusText;
	
	FSessionConfig DesiredSessionConfig;
	TOptional<FSessionConfig> PendingRecreateConfig;
	FOnlineSessionSearchResult PendingInviteSessionResult;
	bool bJoinInviteAfterDestroy = false;
	bool bShowLobbyInvitePanelAfterTravel = false;
	bool bPendingLobbyInvitePanelRequest = false;
};
