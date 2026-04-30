#include "LobbyFriendRowWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"

void ULobbyFriendRowWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (InviteButton)
	{
		InviteButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleInviteClicked);
	}

	RefreshDisplayedFriend();
}

void ULobbyFriendRowWidget::NativeDestruct()
{
	if (InviteButton)
	{
		InviteButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleInviteClicked);
	}

	Super::NativeDestruct();
}

void ULobbyFriendRowWidget::SetupFriendRow(const FSteamFriendInviteEntry& InFriendEntry)
{
	FriendEntry = InFriendEntry;
	RefreshDisplayedFriend();
}

void ULobbyFriendRowWidget::HandleInviteClicked()
{
	UMultiplayerSessionsSubsystem* SessionsSubsystem = GetMultiplayerSessionsSubsystem();
	if (!SessionsSubsystem || FriendEntry.FriendIdString.IsEmpty())
	{
		return;
	}

	SessionsSubsystem->SendSteamInviteToFriendByIdString(FriendEntry.FriendIdString);
}

UMultiplayerSessionsSubsystem* ULobbyFriendRowWidget::GetMultiplayerSessionsSubsystem() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>() : nullptr;
}

void ULobbyFriendRowWidget::RefreshDisplayedFriend()
{
	if (NameText)
	{
		NameText->SetText(FText::FromString(FriendEntry.DisplayName));
	}

	if (StatusText)
	{
		const FText StatusLabel = FriendEntry.bIsOnline
			? FText::FromString(TEXT("Online"))
			: FText::FromString(TEXT("Offline"));
		const FLinearColor StatusColor = FriendEntry.bIsOnline
			? FLinearColor(0.25f, 0.9f, 0.45f, 1.0f)
			: FLinearColor(0.55f, 0.55f, 0.55f, 1.0f);

		StatusText->SetText(StatusLabel);
		StatusText->SetColorAndOpacity(FSlateColor(StatusColor));
	}
}
