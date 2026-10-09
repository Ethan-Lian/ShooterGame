#include "Menu.h"
#include "MultiplayerSessionUIManagerSubsystem.h"
#include "Engine/GameInstance.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

#define LOCTEXT_NAMESPACE "MultiplayerMenu"

void UMenu::MenuSetup(int32 NumberOfPublicConnections, FString TypeOfMatch, FString Path)
{
	bIsTearingDown = false;
	NumPublicConnections = NumberOfPublicConnections;
	MatchType = TypeOfMatch;
	LobbyPath = Path;
	
	if (UMultiplayerSessionUIManagerSubsystem* UIManagerSubsystem = GetUIManagerSubsystem())
	{
		UIManagerSubsystem->ShowMenu(this);
	}

	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance)
	{
		SessionFlow = GameInstance->GetSubsystem<UMultiplayerSessionFlowSubsystem>();
	}
	
	if (SessionFlow)
	{
		SessionFlow->OnStateChanged.AddUniqueDynamic(this, &ThisClass::HandleFlowStateChanged);
		HandleFlowStateChanged(SessionFlow->GetState(), SessionFlow->GetLastError());
	}
}

void UMenu::MenuTearDown()
{
	if (bIsTearingDown)
	{
		return;
	}

	bIsTearingDown = true;

	if (SessionFlow)
	{
		SessionFlow->OnStateChanged.RemoveDynamic(this, &ThisClass::HandleFlowStateChanged);
	}

	if (UMultiplayerSessionUIManagerSubsystem* UIManagerSubsystem = GetUIManagerSubsystem())
	{
		UIManagerSubsystem->HideMenu(this);
	}
}

bool UMenu::Initialize()
{
	const bool bSuccess = Super::Initialize();
	if (!bSuccess)
	{
		return false;
	}

	if (HostButton)
	{
		HostButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HostButtonClicked);
		SetDesiredFocusWidget(HostButton);
	}
	if (RetryButton)
	{
		RetryButton->OnClicked.AddUniqueDynamic(this, &ThisClass::RetryButtonClicked);
	}

	return true;
}

void UMenu::NativeDestruct()
{
	MenuTearDown();
	Super::NativeDestruct();
}

void UMenu::HostButtonClicked()
{
	if (SessionFlow)
	{
		SessionFlow->HostGame(NumPublicConnections, MatchType, LobbyPath);
	}
}

void UMenu::HandleFlowStateChanged(EMultiplayerSessionFlowState State, FText Error)
{
	if (HostButton)
	{
		HostButton->SetIsEnabled(State == EMultiplayerSessionFlowState::Idle && !SessionFlow->IsBusy());
	}
	if (StatusText)
	{
		StatusText->SetText(!Error.IsEmpty() ? Error : State == EMultiplayerSessionFlowState::Idle
			? LOCTEXT("Ready", "创建房间后，通过 Steam 邀请好友加入。")
			: LOCTEXT("Busy", "正在处理联机请求，请稍候…"));
	}
	if (RetryButton)
	{
		RetryButton->SetVisibility(State == EMultiplayerSessionFlowState::CleanupFailed
			? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UMenu::RetryButtonClicked()
{
	SessionFlow->LeaveSession();
}

UMultiplayerSessionUIManagerSubsystem* UMenu::GetUIManagerSubsystem() const
{
	UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UMultiplayerSessionUIManagerSubsystem>() : nullptr;
}

#undef LOCTEXT_NAMESPACE
