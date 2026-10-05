#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LobbyInviteSubsystem.h"
#include "LobbyFriendRowWidget.generated.h"

class UButton;
class UTextBlock;
class ULobbyInviteSubsystem;

UCLASS()
class MULTIPLAYERSESSION_API ULobbyFriendRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Multiplayer Sessions|Lobby Invite")
	void SetupFriendRow(const FSteamFriendInviteEntry& InFriendEntry);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NativeNameText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NativeStatusText;

	UPROPERTY(Transient)
	TObjectPtr<UButton> NativeInviteButton;

	UPROPERTY(BlueprintReadOnly, Category = "Multiplayer Sessions|Lobby Invite")
	FSteamFriendInviteEntry FriendEntry;

private:
	UFUNCTION()
	void HandleInviteClicked();

	ULobbyInviteSubsystem* GetLobbyInviteSubsystem() const;
	void BuildNativeWidgetTree();
	void RefreshDisplayedFriend();
};
