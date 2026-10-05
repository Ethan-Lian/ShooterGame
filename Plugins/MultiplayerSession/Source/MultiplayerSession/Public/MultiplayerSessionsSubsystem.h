#pragma once

#include "CoreMinimal.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MultiplayerSessionsSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FMultiplayerOnCreateSessionComplete, bool);
DECLARE_MULTICAST_DELEGATE_OneParam(FMultiplayerOnDestroySessionComplete, bool);
DECLARE_MULTICAST_DELEGATE_TwoParams(FMultiplayerOnJoinSessionComplete, EOnJoinSessionCompleteResult::Type, const FString&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FMultiplayerOnInviteAccepted, bool, const FOnlineSessionSearchResult&);

// Owns OSS operations. Map travel and frontend decisions belong to the flow subsystem.
UCLASS()
class MULTIPLAYERSESSION_API UMultiplayerSessionsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// True means admitted, including synchronous completion. Rejected requests emit no completion.
	bool CreateSession(int32 NumPublicConnections, const FString& MatchType);
	bool JoinSession(const FOnlineSessionSearchResult& SessionResult);
	bool DestroySession();
	bool IsBusy() const { return ActiveOperationId != 0; }
	bool HasSession() const;
	bool RefreshOnlineSessionInterface();

	UFUNCTION(BlueprintCallable, Category = "Multiplayer Sessions")
	bool ShowSteamInviteUI();

	FMultiplayerOnCreateSessionComplete MultiplayerOnCreateSessionCompleteDelegate;
	FMultiplayerOnJoinSessionComplete MultiplayerOnJoinSessionCompleteDelegate;
	FMultiplayerOnDestroySessionComplete MultiplayerOnDestroySessionCompleteDelegate;
	FMultiplayerOnInviteAccepted OnInviteAccepted;

private:
	enum class EOperation : uint8 { None, Create, Join, Destroy };

	uint64 BeginOperation(EOperation Operation);
	bool OwnsOperation(EOperation Operation, uint64 OperationId) const;
	void ClearActiveOperation();
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful, uint64 OperationId);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result, uint64 OperationId);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful, uint64 OperationId);
	void OnSessionUserInviteAccepted(bool bWasSuccessful, int32 ControllerId, FUniqueNetIdPtr UserId,
		const FOnlineSessionSearchResult& InviteResult);
	void ClearSessionInviteAcceptedDelegate();

	IOnlineSessionPtr OnlineSessionInterface;
	IOnlineSessionPtr OperationSessionInterface;
	IOnlineSessionPtr SessionInterfaceWithInviteDelegate;
	FDelegateHandle OperationDelegateHandle;
	FDelegateHandle SessionUserInviteAcceptedDelegateHandle;
	EOperation ActiveOperation = EOperation::None;
	uint64 ActiveOperationId = 0;
	uint64 NextOperationId = 0;
	bool bDeinitializing = false;
};
