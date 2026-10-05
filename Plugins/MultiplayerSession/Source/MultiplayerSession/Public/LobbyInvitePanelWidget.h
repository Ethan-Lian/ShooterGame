#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LobbyInviteSubsystem.h"
#include "MultiplayerSessionFlowSubsystem.h"
#include "LobbyInvitePanelWidget.generated.h"

class UButton;
class UPanelWidget;
class UTextBlock;
class ULobbyFriendRowWidget;

UCLASS(Abstract)
class MULTIPLAYERSESSION_API ULobbyInvitePanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Multiplayer Sessions|Lobby Invite")
	void RefreshFriends();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(EditDefaultsOnly, Category = "Multiplayer Sessions|Lobby Invite")
	TSubclassOf<ULobbyFriendRowWidget> FriendRowWidgetClass;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> RoomStatusText;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> FriendCountText;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> EmptyText;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> FriendList;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> RefreshButton;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> StartButton;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> LeaveButton;

private:
	UFUNCTION()
	void HandleStartClicked();
	UFUNCTION()
	void HandleLeaveClicked();
	UFUNCTION()
	void HandleFriendsListUpdated(const TArray<FSteamFriendInviteEntry>& Friends);
	UFUNCTION()
	void HandleInviteStatusChanged(FText Message);
	UFUNCTION()
	void HandleFlowStateChanged(EMultiplayerSessionFlowState State, FText Error);

	void UpdateActions();

	UPROPERTY(Transient)
	TObjectPtr<ULobbyInviteSubsystem> Invites;
	UPROPERTY(Transient)
	TObjectPtr<UMultiplayerSessionFlowSubsystem> Flow;
};
