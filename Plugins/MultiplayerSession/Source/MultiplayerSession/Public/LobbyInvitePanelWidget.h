#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MultiplayerSessionsSubsystem.h"
#include "LobbyInvitePanelWidget.generated.h"

class UButton;
class UPanelWidget;
class UTextBlock;
class ULobbyFriendRowWidget;
class UMultiplayerSessionsSubsystem;

UCLASS(Blueprintable)
class MULTIPLAYERSESSION_API ULobbyInvitePanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Multiplayer Sessions|Lobby Invite")
	void RefreshFriends();

	UFUNCTION(BlueprintCallable, Category = "Multiplayer Sessions|Lobby Invite")
	void RebuildFriendRows();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Multiplayer Sessions|Lobby Invite")
	TSubclassOf<ULobbyFriendRowWidget> FriendRowWidgetClass;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Multiplayer Sessions|Lobby Invite")
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Multiplayer Sessions|Lobby Invite")
	TObjectPtr<UPanelWidget> FriendListBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Multiplayer Sessions|Lobby Invite")
	TObjectPtr<UButton> RefreshButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Multiplayer Sessions|Lobby Invite")
	TObjectPtr<UButton> StartButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Multiplayer Sessions|Lobby Invite")
	TObjectPtr<UButton> CloseButton;

private:
	UFUNCTION()
	void HandleRefreshClicked();

	UFUNCTION()
	void HandleStartClicked();

	UFUNCTION()
	void HandleCloseClicked();

	UFUNCTION()
	void HandleFriendsListUpdated(const TArray<FSteamFriendInviteEntry>& Friends);

	UFUNCTION()
	void HandleInviteStatusChanged(FText StatusMessage);

	bool ResolveMultiplayerSessionsSubsystem();
	void BindButtonDelegates();
	void UnbindButtonDelegates();
	void BindSubsystemDelegates();
	void UnbindSubsystemDelegates();
	void RebuildFriendRowsFromList(const TArray<FSteamFriendInviteEntry>& Friends);
	void SetStatusMessage(const FText& StatusMessage);

	UPROPERTY(Transient)
	TObjectPtr<UMultiplayerSessionsSubsystem> MultiplayerSessionsSubsystem;
};
