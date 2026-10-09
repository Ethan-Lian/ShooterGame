#include "LobbyInvitePanelWidget.h"

#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "LobbyFriendRowWidget.h"

#define LOCTEXT_NAMESPACE "LobbyInvitePanel"

void ULobbyInvitePanelWidget::NativeConstruct()
{
	Super::NativeConstruct();
	Invites = GetGameInstance()->GetSubsystem<ULobbyInviteSubsystem>();
	Flow = GetGameInstance()->GetSubsystem<UMultiplayerSessionFlowSubsystem>();
	SetIsFocusable(true);
	SetDesiredFocusWidget(GetWorld()->GetNetMode() == NM_ListenServer ? StartButton.Get() : LeaveButton.Get());
	RefreshButton->OnClicked.AddUniqueDynamic(this, &ThisClass::RefreshFriends);
	StartButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleStartClicked);
	LeaveButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleLeaveClicked);
	Invites->OnSteamFriendsListUpdated.AddUniqueDynamic(this, &ThisClass::HandleFriendsListUpdated);
	Invites->OnLobbyInviteStatusChanged.AddUniqueDynamic(this, &ThisClass::HandleInviteStatusChanged);
	Flow->OnStateChanged.AddUniqueDynamic(this, &ThisClass::HandleFlowStateChanged);
	HandleFriendsListUpdated(Invites->GetCachedSteamFriends());
	HandleFlowStateChanged(Flow->GetState(), Flow->GetLastError());
	if (Flow->CanInviteFriends())
	{
		RefreshFriends();
	}
}

void ULobbyInvitePanelWidget::NativeDestruct()
{
	RefreshButton->OnClicked.RemoveDynamic(this, &ThisClass::RefreshFriends);
	StartButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleStartClicked);
	LeaveButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleLeaveClicked);
	Invites->OnSteamFriendsListUpdated.RemoveDynamic(this, &ThisClass::HandleFriendsListUpdated);
	Invites->OnLobbyInviteStatusChanged.RemoveDynamic(this, &ThisClass::HandleInviteStatusChanged);
	Flow->OnStateChanged.RemoveDynamic(this, &ThisClass::HandleFlowStateChanged);
	Super::NativeDestruct();
}

void ULobbyInvitePanelWidget::RefreshFriends()
{
	Invites->RefreshSteamFriendsList();
}

void ULobbyInvitePanelWidget::HandleStartClicked()
{
	Flow->StartHostedGame();
}

void ULobbyInvitePanelWidget::HandleLeaveClicked()
{
	Flow->LeaveSession();
}

void ULobbyInvitePanelWidget::HandleFriendsListUpdated(const TArray<FSteamFriendInviteEntry>& Friends)
{
	FriendList->ClearChildren();
	int32 OnlineCount = 0;
	for (const FSteamFriendInviteEntry& Friend : Friends)
	{
		OnlineCount += Friend.bIsOnline ? 1 : 0;
		ULobbyFriendRowWidget* Row = CreateWidget<ULobbyFriendRowWidget>(this, FriendRowWidgetClass);
		if (Row)
		{
			Row->SetupFriendRow(Friend);
			FriendList->AddChild(Row);
		}
	}
	FriendCountText->SetText(FText::Format(LOCTEXT("OnlineCount", "{0} 位好友在线"), OnlineCount));
	UpdateActions();
}

void ULobbyInvitePanelWidget::HandleInviteStatusChanged(FText Message)
{
	StatusText->SetText(Flow->GetLastError().IsEmpty() ? Message : Flow->GetLastError());
	UpdateActions();
}

void ULobbyInvitePanelWidget::HandleFlowStateChanged(EMultiplayerSessionFlowState State, FText Error)
{
	const bool bHost = GetWorld()->GetNetMode() == NM_ListenServer;
	RoomStatusText->SetText(Flow->IsBusy() ? LOCTEXT("Connecting", "正在处理，请稍候…")
		: bHost ? LOCTEXT("HostReady", "房间已就绪 · 邀请好友后即可开始")
		: LOCTEXT("WaitForHost", "已加入房间 · 等待房主开始游戏"));
	StatusText->SetText(!Error.IsEmpty() ? Error : bHost ? Invites->GetLobbyInviteStatus() : FText::GetEmpty());
	UpdateActions();
}

void ULobbyInvitePanelWidget::UpdateActions()
{
	const bool bHost = GetWorld()->GetNetMode() == NM_ListenServer;
	const bool bCanInvite = Flow->CanInviteFriends();
	RefreshButton->SetVisibility(bHost ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	RefreshButton->SetIsEnabled(bCanInvite && !Invites->IsRefreshingFriends());
	FriendList->SetIsEnabled(bCanInvite);
	StartButton->SetVisibility(bHost ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	StartButton->SetIsEnabled(Flow->GetState() == EMultiplayerSessionFlowState::Lobby && bCanInvite);
	LeaveButton->SetIsEnabled(!Flow->IsBusy());
	const bool bEmpty = !bHost || FriendList->GetChildrenCount() == 0;
	EmptyText->SetVisibility(bEmpty ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	EmptyText->SetText(bHost ? LOCTEXT("NoFriends", "暂时没有可显示的好友\n登录 Steam 后刷新好友列表")
		: LOCTEXT("ClientHint", "由房主发送邀请\n你可以留在这里等待游戏开始"));
	FriendCountText->SetVisibility(bHost ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	FriendList->SetVisibility(bHost ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

#undef LOCTEXT_NAMESPACE
