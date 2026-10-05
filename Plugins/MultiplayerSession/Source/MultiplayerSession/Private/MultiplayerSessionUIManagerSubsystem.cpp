#include "MultiplayerSessionUIManagerSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameModeBase.h"
#include "LobbyInvitePanelWidget.h"
#include "Menu.h"
#include "MultiplayerSessionRootWidget.h"
#include "MultiplayerSessionsSubsystem.h"
#include "Subsystems/SubsystemCollection.h"

void UMultiplayerSessionUIManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	MultiplayerSessionsSubsystem = Collection.InitializeDependency<UMultiplayerSessionsSubsystem>();
	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->OnLobbyInvitePanelRequested.AddUniqueDynamic(
			this,
			&ThisClass::HandleLobbyInvitePanelRequested);
	}
}

void UMultiplayerSessionUIManagerSubsystem::Deinitialize()
{
	ClearLobbyInvitePanelWidget();
	ClearMenuWidget();
	ClearRootWidget();

	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->OnLobbyInvitePanelRequested.RemoveDynamic(
			this,
			&ThisClass::HandleLobbyInvitePanelRequested);
	}

	Super::Deinitialize();
}

void UMultiplayerSessionUIManagerSubsystem::ShowLobbyInvitePanel()
{
	if (!ResolveMultiplayerSessionsSubsystem() || !MultiplayerSessionsSubsystem->CanShowHostInvitePanel())
	{
		return;
	}

	APlayerController* PlayerController = GetLocalPlayerController();
	if (!PlayerController)
	{
		return;
	}

	UMultiplayerSessionRootWidget* Root = GetOrCreateRootWidget(PlayerController);
	if (!Root)
	{
		return;
	}

	if (LobbyInvitePanelWidget && !DoesWidgetBelongToPlayer(LobbyInvitePanelWidget, PlayerController))
	{
		ClearLobbyInvitePanelWidget();
	}

	if (!LobbyInvitePanelWidget)
	{
		LobbyInvitePanelWidget = CreateWidget<ULobbyInvitePanelWidget>(
			PlayerController,
			ULobbyInvitePanelWidget::StaticClass());
		if (!LobbyInvitePanelWidget)
		{
			return;
		}

		LobbyInvitePanelWidget->OnCloseRequested.AddUniqueDynamic(
			this,
			&ThisClass::HandleLobbyInvitePanelCloseRequested);
		LobbyInvitePanelWidget->OnStartRequested.AddUniqueDynamic(
			this,
			&ThisClass::HandleLobbyInvitePanelStartRequested);
	}

	Root->AddWidgetToLayer(LobbyInvitePanelWidget, 100);

	ApplyLobbyInviteInputMode();
}

void UMultiplayerSessionUIManagerSubsystem::HideLobbyInvitePanel()
{
	ClearLobbyInvitePanelWidget();

	if (MenuWidget && DoesWidgetBelongToPlayer(MenuWidget, GetLocalPlayerController()))
	{
		ApplyMenuInputMode(MenuWidget);
	}
	else
	{
		RestoreGameInputMode();
	}
}

void UMultiplayerSessionUIManagerSubsystem::ShowMenu(UMenu* InMenuWidget)
{
	if (!InMenuWidget)
	{
		return;
	}

	APlayerController* PlayerController = GetLocalPlayerController();
	if (!PlayerController)
	{
		return;
	}

	if (!DoesWidgetBelongToPlayer(InMenuWidget, PlayerController))
	{
		return;
	}

	UMultiplayerSessionRootWidget* Root = GetOrCreateRootWidget(PlayerController);
	if (!Root)
	{
		return;
	}

	if (MenuWidget && MenuWidget != InMenuWidget)
	{
		ClearMenuWidget();
	}

	MenuWidget = InMenuWidget;
	MenuWidget->SetVisibility(ESlateVisibility::Visible);
	MenuWidget->SetIsFocusable(true);
	Root->AddWidgetToLayer(MenuWidget, 0);

	ApplyMenuInputMode(MenuWidget);
}

void UMultiplayerSessionUIManagerSubsystem::HideMenu(UMenu* InMenuWidget)
{
	if (!InMenuWidget || MenuWidget != InMenuWidget)
	{
		return;
	}

	if (RootWidget)
	{
		RootWidget->RemoveWidgetFromLayer(InMenuWidget);
	}

	MenuWidget = nullptr;
	if (LobbyInvitePanelWidget && DoesWidgetBelongToPlayer(LobbyInvitePanelWidget, GetLocalPlayerController()))
	{
		ApplyLobbyInviteInputMode();
	}
	else
	{
		RestoreGameInputMode();
	}
}

void UMultiplayerSessionUIManagerSubsystem::HandleLobbyInvitePanelRequested()
{
	ShowLobbyInvitePanel();
}

void UMultiplayerSessionUIManagerSubsystem::HandleLobbyInvitePanelCloseRequested()
{
	HideLobbyInvitePanel();
}

void UMultiplayerSessionUIManagerSubsystem::HandleLobbyInvitePanelStartRequested()
{
	StartHostedGame();
}

bool UMultiplayerSessionUIManagerSubsystem::StartHostedGame()
{
	UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_Client || World->GetNetMode() == NM_DedicatedServer)
	{
		return false;
	}

	if (!GetLocalPlayerController())
	{
		return false;
	}

	HideLobbyInvitePanel();

	if (AGameModeBase* GameMode = World->GetAuthGameMode())
	{
		GameMode->bUseSeamlessTravel = true;
	}

	return World->ServerTravel(GameplayMapPath);
}

bool UMultiplayerSessionUIManagerSubsystem::ResolveMultiplayerSessionsSubsystem()
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

bool UMultiplayerSessionUIManagerSubsystem::IsUsableLocalPlayerController(const APlayerController* PlayerController) const
{
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return false;
	}

	const UWorld* World = GetWorld();
	return World && World->GetNetMode() != NM_DedicatedServer && PlayerController->GetWorld() == World;
}

bool UMultiplayerSessionUIManagerSubsystem::DoesWidgetBelongToPlayer(
	const UUserWidget* Widget,
	const APlayerController* PlayerController) const
{
	return Widget && IsUsableLocalPlayerController(PlayerController) && Widget->GetOwningPlayer() == PlayerController
		&& Widget->GetWorld() == PlayerController->GetWorld();
}

APlayerController* UMultiplayerSessionUIManagerSubsystem::GetLocalPlayerController() const
{
	const UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_DedicatedServer)
	{
		return nullptr;
	}

	APlayerController* PlayerController = World->GetFirstPlayerController();
	return IsUsableLocalPlayerController(PlayerController) ? PlayerController : nullptr;
}

UMultiplayerSessionRootWidget* UMultiplayerSessionUIManagerSubsystem::GetOrCreateRootWidget(APlayerController* PlayerController)
{
	if (!IsUsableLocalPlayerController(PlayerController))
	{
		return nullptr;
	}

	if (RootWidget && !DoesWidgetBelongToPlayer(RootWidget, PlayerController))
	{
		ClearRootWidget();
	}

	if (!RootWidget)
	{
		RootWidget = CreateWidget<UMultiplayerSessionRootWidget>(
			PlayerController,
			UMultiplayerSessionRootWidget::StaticClass());
	}

	if (RootWidget && !RootWidget->IsInViewport())
	{
		RootWidget->AddToViewport(1000);
	}

	return RootWidget;
}

void UMultiplayerSessionUIManagerSubsystem::ClearLobbyInvitePanelWidget()
{
	if (!LobbyInvitePanelWidget)
	{
		return;
	}

	if (RootWidget)
	{
		RootWidget->RemoveWidgetFromLayer(LobbyInvitePanelWidget);
	}
	else
	{
		LobbyInvitePanelWidget->RemoveFromParent();
	}

	LobbyInvitePanelWidget->OnCloseRequested.RemoveDynamic(
		this,
		&ThisClass::HandleLobbyInvitePanelCloseRequested);
	LobbyInvitePanelWidget->OnStartRequested.RemoveDynamic(
		this,
		&ThisClass::HandleLobbyInvitePanelStartRequested);
	LobbyInvitePanelWidget = nullptr;
}

void UMultiplayerSessionUIManagerSubsystem::ClearMenuWidget()
{
	if (!MenuWidget)
	{
		return;
	}

	if (RootWidget)
	{
		RootWidget->RemoveWidgetFromLayer(MenuWidget);
	}
	else
	{
		MenuWidget->RemoveFromParent();
	}

	MenuWidget = nullptr;
}

void UMultiplayerSessionUIManagerSubsystem::ClearRootWidget()
{
	ClearLobbyInvitePanelWidget();
	ClearMenuWidget();

	if (RootWidget)
	{
		RootWidget->RemoveFromParent();
		RootWidget = nullptr;
	}
}

void UMultiplayerSessionUIManagerSubsystem::ApplyMenuInputMode(UMenu* InMenuWidget) const
{
	APlayerController* PlayerController = GetLocalPlayerController();
	if (!DoesWidgetBelongToPlayer(InMenuWidget, PlayerController))
	{
		return;
	}

	FInputModeUIOnly InputModeData;
	InputModeData.SetWidgetToFocus(InMenuWidget->TakeWidget());
	InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

	PlayerController->SetInputMode(InputModeData);
	PlayerController->SetShowMouseCursor(true);
}

void UMultiplayerSessionUIManagerSubsystem::ApplyLobbyInviteInputMode()
{
	APlayerController* PlayerController = GetLocalPlayerController();
	if (!DoesWidgetBelongToPlayer(LobbyInvitePanelWidget, PlayerController))
	{
		return;
	}

	FInputModeGameAndUI InputModeData;
	InputModeData.SetWidgetToFocus(LobbyInvitePanelWidget->TakeWidget());
	InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

	PlayerController->SetInputMode(InputModeData);
	PlayerController->SetShowMouseCursor(true);
}

void UMultiplayerSessionUIManagerSubsystem::RestoreGameInputMode() const
{
	APlayerController* PlayerController = GetLocalPlayerController();
	if (!PlayerController)
	{
		return;
	}

	FInputModeGameOnly InputModeData;
	PlayerController->SetInputMode(InputModeData);
	PlayerController->SetShowMouseCursor(false);
}
