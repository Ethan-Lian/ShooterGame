#include "LobbyFriendRowWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"

#define LOCTEXT_NAMESPACE "LobbyFriendRow"

void ULobbyFriendRowWidget::NativeConstruct()
{
	Super::NativeConstruct();
	InviteButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleInviteClicked);
	RefreshDisplayedFriend();
}

void ULobbyFriendRowWidget::NativeDestruct()
{
	InviteButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleInviteClicked);
	Super::NativeDestruct();
}

void ULobbyFriendRowWidget::SetupFriendRow(const FSteamFriendInviteEntry& InFriendEntry)
{
	FriendEntry = InFriendEntry;
	bInviteSent = false;
	RefreshDisplayedFriend();
}

void ULobbyFriendRowWidget::HandleInviteClicked()
{
	ULobbyInviteSubsystem* Invites = GetGameInstance()->GetSubsystem<ULobbyInviteSubsystem>();
	if (Invites->SendSteamInviteToFriendByIdString(FriendEntry.FriendIdString))
	{
		bInviteSent = true;
		RefreshDisplayedFriend();
	}
}

void ULobbyFriendRowWidget::RefreshDisplayedFriend()
{
	// Setup can precede construction; BindWidget members are initialized by CreateWidget.
	FriendName->SetText(FText::FromString(FriendEntry.DisplayName));
	InitialText->SetText(FText::FromString(FriendEntry.DisplayName.Left(1).ToUpper()));
	PresenceText->SetText(FriendEntry.bIsOnline ? LOCTEXT("Online", "在线") : LOCTEXT("Offline", "离线"));
	PresenceText->SetColorAndOpacity(FSlateColor(FriendEntry.bIsOnline
		? FLinearColor(0.24f, 0.78f, 0.64f) : FLinearColor(0.38f, 0.43f, 0.5f)));
	InviteButton->SetIsEnabled(FriendEntry.bIsOnline && !bInviteSent);
	InviteLabel->SetText(bInviteSent ? LOCTEXT("Sent", "已发送") : LOCTEXT("Invite", "邀请"));
}

#undef LOCTEXT_NAMESPACE
