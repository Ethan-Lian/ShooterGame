#include "MultiplayerInviteConfirmationWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "MultiplayerSessionFlowSubsystem.h"

#define LOCTEXT_NAMESPACE "MultiplayerInviteConfirmation"

void UMultiplayerInviteConfirmationWidget::SetInvitingHost(const FString& HostName)
{
	MessageText->SetText(FText::Format(LOCTEXT("Message", "加入 {0} 的房间？\n确认后会离开当前房间。"), FText::FromString(HostName)));
}

void UMultiplayerInviteConfirmationWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	SetDesiredFocusWidget(CancelButton);
	ConfirmButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleConfirm);
	CancelButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleCancel);
}

void UMultiplayerInviteConfirmationWidget::NativeDestruct()
{
	ConfirmButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleConfirm);
	CancelButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleCancel);
	Super::NativeDestruct();
}

void UMultiplayerInviteConfirmationWidget::HandleConfirm()
{
	GetGameInstance()->GetSubsystem<UMultiplayerSessionFlowSubsystem>()->ConfirmPendingInvite();
}

void UMultiplayerInviteConfirmationWidget::HandleCancel()
{
	GetGameInstance()->GetSubsystem<UMultiplayerSessionFlowSubsystem>()->DeclinePendingInvite();
}

#undef LOCTEXT_NAMESPACE
