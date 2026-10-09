#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LobbyInviteSubsystem.h"
#include "LobbyFriendRowWidget.generated.h"

class UButton;
class UTextBlock;

UCLASS(Abstract)
class MULTIPLAYERSESSION_API ULobbyFriendRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Multiplayer Sessions|Lobby Invite")
	void SetupFriendRow(const FSteamFriendInviteEntry& InFriendEntry);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> FriendName;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PresenceText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> InitialText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> InviteButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> InviteLabel;

private:
	UFUNCTION()
	void HandleInviteClicked();
	void RefreshDisplayedFriend();
	FSteamFriendInviteEntry FriendEntry;
	bool bInviteSent = false;
};
