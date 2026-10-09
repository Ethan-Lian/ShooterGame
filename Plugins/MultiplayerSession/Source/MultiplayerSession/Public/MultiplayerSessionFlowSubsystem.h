#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Engine/EngineBaseTypes.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MultiplayerSessionFlowSubsystem.generated.h"

class APlayerController;
class UNetDriver;
class ULocalPlayer;
class UMultiplayerSessionsSubsystem;

UENUM(BlueprintType)
enum class EMultiplayerSessionFlowState : uint8
{
	Idle,
	CreatingSession,
	JoiningSession,
	Traveling,
	Lobby,
	InGame,
	Leaving,
	CleanupFailed
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMultiplayerSessionFlowStateChanged, EMultiplayerSessionFlowState, State, FText, Error);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMultiplayerInviteConfirmationRequested, FString, HostName);

// Coordinates local session lifecycle and travel; widgets only submit commands and observe state.
UCLASS()
class MULTIPLAYERSESSION_API UMultiplayerSessionFlowSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Return values describe admission, not eventual completion. Observe OnStateChanged for results.
	UFUNCTION(BlueprintCallable, Category = "Multiplayer Sessions|Flow")
	bool HostGame(int32 NumPublicConnections = 4, FString MatchType = TEXT("FreeForAll"), FString LobbyMapPath = TEXT(""));

	UFUNCTION(BlueprintCallable, Category = "Multiplayer Sessions|Flow")
	bool StartHostedGame();

	UFUNCTION(BlueprintCallable, Category = "Multiplayer Sessions|Flow")
	bool LeaveSession();

	UFUNCTION(BlueprintCallable, Category = "Multiplayer Sessions|Flow")
	bool ConfirmPendingInvite();

	UFUNCTION(BlueprintCallable, Category = "Multiplayer Sessions|Flow")
	void DeclinePendingInvite();

	UFUNCTION(BlueprintPure, Category = "Multiplayer Sessions|Flow")
	EMultiplayerSessionFlowState GetState() const { return State; }

	UFUNCTION(BlueprintPure, Category = "Multiplayer Sessions|Flow")
	FText GetLastError() const { return LastError; }

	UFUNCTION(BlueprintPure, Category = "Multiplayer Sessions|Flow")
	bool IsBusy() const;

	UFUNCTION(BlueprintPure, Category = "Multiplayer Sessions|Flow")
	bool CanInviteFriends() const;

	UFUNCTION(BlueprintPure, Category = "Multiplayer Sessions|Flow")
	bool HasPendingInvite() const { return PendingInvite.IsValid(); }

	UPROPERTY(BlueprintAssignable, Category = "Multiplayer Sessions|Flow")
	FOnMultiplayerSessionFlowStateChanged OnStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Multiplayer Sessions|Flow")
	FOnMultiplayerInviteConfirmationRequested OnInviteConfirmationRequested;

private:
	enum class ETravelDestination : uint8 { None, Lobby, Session, Game, Menu };
	enum class EAfterLeave : uint8 { Menu, JoinInvite };

	void HandleCreateSessionComplete(bool bWasSuccessful);
	void HandleJoinSessionComplete(EOnJoinSessionCompleteResult::Type Result, const FString& Address);
	void HandleDestroySessionComplete(bool bWasSuccessful);
	void HandleInviteAccepted(bool bWasSuccessful, const FOnlineSessionSearchResult& InviteResult);
	void HandlePostLoadMap(UWorld* LoadedWorld);
	void BindLocalPlayerControllerChanged();
	void HandleLocalPlayerControllerChanged(APlayerController* Controller);
	void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& Error);
	void HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& Error);
	bool TickWorldReady(float DeltaTime);
	bool TickRecovery(float DeltaTime);
	bool IsFailureForThisGameInstance(UWorld* World, UNetDriver* NetDriver) const;
	bool BeginJoinSession(const FOnlineSessionSearchResult& InviteResult);
	void BeginCleanup();
	void ReturnToMenu();
	void FinishReturnToMenu();
	void RequestRecovery(const FText& Error);
	bool ContinueRecoveryIfRequested();
	bool ValidateMap(const FString& MapPath);
	void BeginTravel(ETravelDestination Destination, const FString& MapPath);
	void SetState(EMultiplayerSessionFlowState NewState);
	void ReportError(const FText& Error);
	APlayerController* GetLocalPlayerController() const;
	bool HasGameConnection() const;

	UPROPERTY(Transient)
	TObjectPtr<UMultiplayerSessionsSubsystem> Sessions;

	EMultiplayerSessionFlowState State = EMultiplayerSessionFlowState::Idle;
	FText LastError;
	FOnlineSessionSearchResult PendingInvite;
	FString HostedLobbyMap;
	FString ExpectedMap;
	ETravelDestination TravelDestination = ETravelDestination::None;
	EAfterLeave AfterLeave = EAfterLeave::Menu;
	TWeakObjectPtr<UWorld> ReadyWorld;
	TWeakObjectPtr<ULocalPlayer> ControllerEventLocalPlayer;
	FDelegateHandle PlayerControllerChangedHandle;
	FTSTicker::FDelegateHandle WorldReadyTickerHandle;
	FTSTicker::FDelegateHandle RecoveryTickerHandle;
	FDelegateHandle PostLoadMapHandle;
	FDelegateHandle NetworkFailureHandle;
	FDelegateHandle TravelFailureHandle;
	bool bRecoveryRequested = false;
	bool bSessionCleanupFailed = false;
	bool bDeinitializing = false;
};
