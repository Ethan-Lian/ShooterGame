#include "MultiplayerSessionUIManagerSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "LobbyInvitePanelWidget.h"
#include "Menu.h"
#include "MultiplayerSessionRootWidget.h"
#include "MultiplayerSessionSettings.h"
#include "MultiplayerInviteConfirmationWidget.h"
#include "Engine/GameInstance.h"
#include "Subsystems/SubsystemCollection.h"
#include "UObject/UObjectGlobals.h"

void UMultiplayerSessionUIManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	SessionFlow = Collection.InitializeDependency<UMultiplayerSessionFlowSubsystem>();
	SessionFlow->OnStateChanged.AddUniqueDynamic(this, &ThisClass::HandleFlowStateChanged);
	SessionFlow->OnInviteConfirmationRequested.AddUniqueDynamic(this, &ThisClass::HandleInviteConfirmationRequested);
	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::HandlePostLoadMap);
}

void UMultiplayerSessionUIManagerSubsystem::Deinitialize()
{
	ClearLobbyInvitePanelWidget();
	ClearMenuWidget();
	ClearRootWidget();

	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
	if (SessionFlow)
	{
		SessionFlow->OnStateChanged.RemoveDynamic(this, &ThisClass::HandleFlowStateChanged);
		SessionFlow->OnInviteConfirmationRequested.RemoveDynamic(this, &ThisClass::HandleInviteConfirmationRequested);
	}
	SessionFlow = nullptr;

	Super::Deinitialize();
}

void UMultiplayerSessionUIManagerSubsystem::ShowLobbyInvitePanel()
{
	if (!SessionFlow || (SessionFlow->GetState() != EMultiplayerSessionFlowState::Lobby && !SessionFlow->CanInviteFriends()))
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
			GetDefault<UMultiplayerSessionSettings>()->LobbyPanelClass.LoadSynchronous());
		if (!LobbyInvitePanelWidget)
		{
			return;
		}
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

void UMultiplayerSessionUIManagerSubsystem::HandleFlowStateChanged(EMultiplayerSessionFlowState State, FText Error)
{
	if (MenuWidget)
	{
		MenuWidget->SetIsEnabled(!SessionFlow->IsBusy());
	}
	if (InviteConfirmationWidget && (!SessionFlow->HasPendingInvite() || SessionFlow->IsBusy()))
	{
		ClearInviteConfirmation();
		RestoreVisibleWidgetInputMode();
	}
	if (State == EMultiplayerSessionFlowState::Lobby)
	{
		ShowLobbyInvitePanel();
	}
	else if (State == EMultiplayerSessionFlowState::InGame)
	{
		HideLobbyInvitePanel();
	}
}

void UMultiplayerSessionUIManagerSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
	if (LoadedWorld && LoadedWorld->GetGameInstance() == GetGameInstance() && RootWidget
		&& !DoesWidgetBelongToPlayer(RootWidget, GetLocalPlayerController()))
	{
		ClearRootWidget();
	}
}

void UMultiplayerSessionUIManagerSubsystem::HandleInviteConfirmationRequested(FString HostName)
{
	APlayerController* PlayerController = GetLocalPlayerController();
	UMultiplayerSessionRootWidget* Root = GetOrCreateRootWidget(PlayerController);
	if (!Root)
	{
		return;
	}
	ClearInviteConfirmation();
	InviteConfirmationWidget = CreateWidget<UMultiplayerInviteConfirmationWidget>(PlayerController,
		GetDefault<UMultiplayerSessionSettings>()->InviteConfirmationClass.LoadSynchronous());
	if (!InviteConfirmationWidget)
	{
		return;
	}
	InviteConfirmationWidget->SetInvitingHost(HostName);
	Root->AddWidgetToLayer(InviteConfirmationWidget, 200);
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(InviteConfirmationWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);
	PlayerController->SetShowMouseCursor(true);
}

void UMultiplayerSessionUIManagerSubsystem::ClearInviteConfirmation()
{
	if (InviteConfirmationWidget)
	{
		InviteConfirmationWidget->RemoveFromParent();
		InviteConfirmationWidget = nullptr;
	}
}

void UMultiplayerSessionUIManagerSubsystem::RestoreVisibleWidgetInputMode()
{
	if (MenuWidget)
	{
		ApplyMenuInputMode(MenuWidget);
	}
	else if (LobbyInvitePanelWidget)
	{
		ApplyLobbyInviteInputMode();
	}
	else
	{
		RestoreGameInputMode();
	}
}

bool UMultiplayerSessionUIManagerSubsystem::StartHostedGame()
{
	return SessionFlow && SessionFlow->StartHostedGame();
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
	// UserWidget's LocalPlayer context follows travel; its getters cannot identify the world it was created for.
	if (Widget == RootWidget && (RootOwnerWorld.Get() != GetWorld() || RootOwnerController.Get() != PlayerController))
	{
		return false;
	}
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

	APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController(GetWorld());
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
		RootOwnerWorld = PlayerController->GetWorld();
		RootOwnerController = PlayerController;
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
	ClearInviteConfirmation();
	ClearLobbyInvitePanelWidget();
	ClearMenuWidget();

	if (RootWidget)
	{
		RootWidget->RemoveFromParent();
		RootWidget = nullptr;
	}
	RootOwnerWorld.Reset();
	RootOwnerController.Reset();
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
	if (InviteConfirmationWidget)
	{
		return;
	}
	APlayerController* PlayerController = GetLocalPlayerController();
	if (!DoesWidgetBelongToPlayer(LobbyInvitePanelWidget, PlayerController))
	{
		return;
	}

	FInputModeUIOnly InputModeData;
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
