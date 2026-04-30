#include "LobbyInvitePanelWidget.h"

#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "LobbyFriendRowWidget.h"

void ULobbyInvitePanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BindButtonDelegates();

	if (!ResolveMultiplayerSessionsSubsystem())
	{
		SetStatusMessage(FText::FromString(TEXT("Multiplayer sessions subsystem is unavailable.")));
		return;
	}

	BindSubsystemDelegates();
	SetStatusMessage(MultiplayerSessionsSubsystem->GetLobbyInviteStatus());
	RebuildFriendRowsFromList(MultiplayerSessionsSubsystem->GetCachedSteamFriends());
	MultiplayerSessionsSubsystem->RefreshSteamFriendsList();
}

void ULobbyInvitePanelWidget::NativeDestruct()
{
	UnbindSubsystemDelegates();
	UnbindButtonDelegates();

	Super::NativeDestruct();
}

void ULobbyInvitePanelWidget::RefreshFriends()
{
	if (!ResolveMultiplayerSessionsSubsystem())
	{
		SetStatusMessage(FText::FromString(TEXT("Multiplayer sessions subsystem is unavailable.")));
		return;
	}

	MultiplayerSessionsSubsystem->RefreshSteamFriendsList();
}

void ULobbyInvitePanelWidget::RebuildFriendRows()
{
	if (!ResolveMultiplayerSessionsSubsystem())
	{
		RebuildFriendRowsFromList(TArray<FSteamFriendInviteEntry>());
		return;
	}

	RebuildFriendRowsFromList(MultiplayerSessionsSubsystem->GetCachedSteamFriends());
}

void ULobbyInvitePanelWidget::HandleRefreshClicked()
{
	RefreshFriends();
}

void ULobbyInvitePanelWidget::HandleStartClicked()
{
	if (ResolveMultiplayerSessionsSubsystem())
	{
		MultiplayerSessionsSubsystem->StartHostedGame();
	}
}

void ULobbyInvitePanelWidget::HandleCloseClicked()
{
	if (ResolveMultiplayerSessionsSubsystem())
	{
		MultiplayerSessionsSubsystem->HideLobbyInvitePanel();
	}

	RemoveFromParent();
}

void ULobbyInvitePanelWidget::HandleFriendsListUpdated(const TArray<FSteamFriendInviteEntry>& Friends)
{
	RebuildFriendRowsFromList(Friends);
}

void ULobbyInvitePanelWidget::HandleInviteStatusChanged(FText StatusMessage)
{
	SetStatusMessage(StatusMessage);
}

bool ULobbyInvitePanelWidget::ResolveMultiplayerSessionsSubsystem()
{
	if (MultiplayerSessionsSubsystem)
	{
		return true;
	}

	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return false;
	}

	MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
	return MultiplayerSessionsSubsystem != nullptr;
}

void ULobbyInvitePanelWidget::BindButtonDelegates()
{
	if (RefreshButton)
	{
		RefreshButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleRefreshClicked);
	}

	if (StartButton)
	{
		StartButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleStartClicked);
	}

	if (CloseButton)
	{
		CloseButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleCloseClicked);
	}
}

void ULobbyInvitePanelWidget::UnbindButtonDelegates()
{
	if (RefreshButton)
	{
		RefreshButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleRefreshClicked);
	}

	if (StartButton)
	{
		StartButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleStartClicked);
	}

	if (CloseButton)
	{
		CloseButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleCloseClicked);
	}
}

void ULobbyInvitePanelWidget::BindSubsystemDelegates()
{
	if (!MultiplayerSessionsSubsystem)
	{
		return;
	}

	MultiplayerSessionsSubsystem->OnSteamFriendsListUpdated.AddUniqueDynamic(
		this,
		&ThisClass::HandleFriendsListUpdated);
	MultiplayerSessionsSubsystem->OnLobbyInviteStatusChanged.AddUniqueDynamic(
		this,
		&ThisClass::HandleInviteStatusChanged);
}

void ULobbyInvitePanelWidget::UnbindSubsystemDelegates()
{
	if (!MultiplayerSessionsSubsystem)
	{
		return;
	}

	MultiplayerSessionsSubsystem->OnSteamFriendsListUpdated.RemoveDynamic(
		this,
		&ThisClass::HandleFriendsListUpdated);
	MultiplayerSessionsSubsystem->OnLobbyInviteStatusChanged.RemoveDynamic(
		this,
		&ThisClass::HandleInviteStatusChanged);
}

void ULobbyInvitePanelWidget::RebuildFriendRowsFromList(const TArray<FSteamFriendInviteEntry>& Friends)
{
	if (!FriendListBox)
	{
		return;
	}

	FriendListBox->ClearChildren();

	if (Friends.Num() == 0)
	{
		return;
	}

	if (!FriendRowWidgetClass)
	{
		SetStatusMessage(FText::FromString(TEXT("Friend row widget class is not assigned.")));
		return;
	}

	for (const FSteamFriendInviteEntry& FriendEntry : Friends)
	{
		ULobbyFriendRowWidget* FriendRowWidget = CreateWidget<ULobbyFriendRowWidget>(this, FriendRowWidgetClass);
		if (!FriendRowWidget)
		{
			continue;
		}

		FriendRowWidget->SetupFriendRow(FriendEntry);
		FriendListBox->AddChild(FriendRowWidget);
	}
}

void ULobbyInvitePanelWidget::SetStatusMessage(const FText& StatusMessage)
{
	if (StatusText)
	{
		StatusText->SetText(StatusMessage);
	}
}
