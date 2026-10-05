#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LobbyInviteSubsystem.h"
#include "LobbyInvitePanelWidget.generated.h"

class UButton;
class UPanelWidget;
class UTextBlock;
class ULobbyFriendRowWidget;
class ULobbyInviteSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbyInvitePanelCloseRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbyInvitePanelStartRequested);

UCLASS()
class MULTIPLAYERSESSION_API ULobbyInvitePanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Multiplayer Sessions|Lobby Invite")
	void RefreshFriends();

	UFUNCTION(BlueprintCallable, Category = "Multiplayer Sessions|Lobby Invite")
	void RebuildFriendRows();

	UPROPERTY(BlueprintAssignable, Category = "Multiplayer Sessions|Lobby Invite")
	FOnLobbyInvitePanelCloseRequested OnCloseRequested;

	UPROPERTY(BlueprintAssignable, Category = "Multiplayer Sessions|Lobby Invite")
	FOnLobbyInvitePanelStartRequested OnStartRequested;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Multiplayer Sessions|Lobby Invite")
	TSubclassOf<ULobbyFriendRowWidget> FriendRowWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NativeStatusText;

	UPROPERTY(Transient)
	TObjectPtr<UPanelWidget> NativeFriendListBox;

	UPROPERTY(Transient)
	TObjectPtr<UButton> NativeRefreshButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> NativeStartButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> NativeCloseButton;

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

	bool ResolveLobbyInviteSubsystem();
	void BuildNativeWidgetTree();
	void BindButtonDelegates();
	void UnbindButtonDelegates();
	void BindSubsystemDelegates();
	void UnbindSubsystemDelegates();
	void RebuildFriendRowsFromList(const TArray<FSteamFriendInviteEntry>& Friends);
	void SetStatusMessage(const FText& StatusMessage);

	UPROPERTY(Transient)
	TObjectPtr<ULobbyInviteSubsystem> LobbyInviteSubsystem;
};
