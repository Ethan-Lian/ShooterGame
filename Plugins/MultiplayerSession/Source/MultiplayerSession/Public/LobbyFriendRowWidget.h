#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MultiplayerSessionsSubsystem.h"
#include "LobbyFriendRowWidget.generated.h"

class UButton;
class UTextBlock;
class UMultiplayerSessionsSubsystem;

UCLASS(Blueprintable)
class MULTIPLAYERSESSION_API ULobbyFriendRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Multiplayer Sessions|Lobby Invite")
	void SetupFriendRow(const FSteamFriendInviteEntry& InFriendEntry);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Multiplayer Sessions|Lobby Invite")
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Multiplayer Sessions|Lobby Invite")
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Multiplayer Sessions|Lobby Invite")
	TObjectPtr<UButton> InviteButton;

	UPROPERTY(BlueprintReadOnly, Category = "Multiplayer Sessions|Lobby Invite")
	FSteamFriendInviteEntry FriendEntry;

private:
	UFUNCTION()
	void HandleInviteClicked();

	UMultiplayerSessionsSubsystem* GetMultiplayerSessionsSubsystem() const;
	void RefreshDisplayedFriend();
};
