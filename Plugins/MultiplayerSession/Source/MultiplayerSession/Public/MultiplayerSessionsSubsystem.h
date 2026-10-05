#pragma once

#include "CoreMinimal.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MultiplayerSessionsSubsystem.generated.h"


/*
 * Declaring our own custom delegate for the menu class to bind callbacks
 */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnCreateSessionComplete,bool,bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnDestroySessionComplete,bool,bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_TwoParams(FMultiplayerOnJoinSessionComplete,EOnJoinSessionCompleteResult::Type Result,const FString& Address);


USTRUCT()
struct FSessionConfig
{
	GENERATED_BODY()
	
	int32 NumPublicConnections = 4;
	FString MatchType = TEXT("FreeForAll");
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbyInvitePanelRequested);

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

	// Opens the platform invite UI for the current Steam lobby session.
	UFUNCTION(BlueprintCallable, Category = "Multiplayer Sessions")
	bool ShowSteamInviteUI();

	UFUNCTION(BlueprintPure, Category = "Multiplayer Sessions")
	bool CanShowHostInvitePanel() const;

	// Requests that Blueprint-owned lobby UI be shown.
	UFUNCTION(BlueprintCallable, Category = "Multiplayer Sessions")
	void ShowLobbyInvitePanel();

	void RequestShowLobbyInvitePanelAfterTravel();
	
	/*
	 * Our own custom delegate
	 */
	FMultiplayerOnCreateSessionComplete MultiplayerOnCreateSessionCompleteDelegate;
	FMultiplayerOnJoinSessionComplete MultiplayerOnJoinSessionCompleteDelegate;
	FMultiplayerOnDestroySessionComplete MultiplayerOnDestroySessionCompleteDelegate;

	UPROPERTY(BlueprintAssignable, Category = "Multiplayer Sessions|Lobby Invite")
	FOnLobbyInvitePanelRequested OnLobbyInvitePanelRequested;

protected:
	
	/*
	 * Callback function
	 *  Internal callbacks for the delegates we'll add to the Online Session Interface delegate list.
	 */
	
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void OnSessionUserInviteAccepted(bool bWasSuccessful, int32 ControllerId, FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& InviteResult);
	
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

	FOnSessionUserInviteAcceptedDelegate SessionUserInviteAcceptedDelegate;
	FDelegateHandle SessionUserInviteAcceptedDelegateHandle;
	IOnlineSessionPtr SessionInterfaceWithInviteDelegate;

	FDelegateHandle PostLoadMapWithWorldDelegateHandle;
	
	FSessionConfig DesiredSessionConfig;
	TOptional<FSessionConfig> PendingRecreateConfig;
	FOnlineSessionSearchResult PendingInviteSessionResult;
	bool bJoinInviteAfterDestroy = false;
	bool bShowLobbyInvitePanelAfterTravel = false;
};
