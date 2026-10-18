#include "HUD/ShooterHUD.h"

#include "Blueprint/UserWidget.h"
#include "Character/PlayerCharacter.h"
#include "Components/ShooterHealthComponent.h"
#include "Controller/ShooterPlayerController.h"
#include "HUD/ShooterAmmoWidget.h"
#include "HUD/ShooterHealthWidget.h"
#include "HUD/ShooterMatchWidget.h"
#include "ShooterGame.h"

AShooterHUD::AShooterHUD()
{
}

void AShooterHUD::SetObservedPawn(APawn* NewPawn)
{
	if (ObservedPawn == NewPawn)
	{
		RefreshHealthBinding();
		return;
	}

	ObservedPawn = NewPawn;
	RefreshHealthBinding();
}

void AShooterHUD::BeginPlay()
{
	Super::BeginPlay();

	if (PlayerOwner != nullptr && PlayerOwner->IsLocalController())
	{
		// A seamless-travel controller may still have the previous results screen's mode.
		// Set this before Widget construction, which can immediately enter results again.
		PlayerOwner->SetInputMode(FInputModeGameOnly());
		PlayerOwner->SetShowMouseCursor(false);
	}

	CreateHUDWidgets();
	SetObservedPawn(PlayerOwner != nullptr ? PlayerOwner->GetPawn() : nullptr);
}

void AShooterHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bMatchEndedPresentation && PlayerOwner != nullptr)
	{
		// Balance only this HUD's ignore-input requests; the next world owns its mode.
		PlayerOwner->SetIgnoreMoveInput(false);
		PlayerOwner->SetIgnoreLookInput(false);
		bMatchEndedPresentation = false;
	}
	RemoveHUDWidgets();
	Super::EndPlay(EndPlayReason);
}

void AShooterHUD::SetMatchEndedPresentation(bool bMatchEnded, UUserWidget* FocusWidget)
{
	if (bMatchEndedPresentation == bMatchEnded || PlayerOwner == nullptr || !PlayerOwner->IsLocalController())
	{
		return;
	}
	bMatchEndedPresentation = bMatchEnded;
	PlayerOwner->SetIgnoreMoveInput(bMatchEnded);
	PlayerOwner->SetIgnoreLookInput(bMatchEnded);
	PlayerOwner->SetShowMouseCursor(bMatchEnded);
	if (bMatchEnded)
	{
		CastChecked<AShooterPlayerController>(PlayerOwner)->ClearMatchInput();
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(FocusWidget->TakeWidget());
		PlayerOwner->SetInputMode(InputMode);
	}
	else
	{
		PlayerOwner->SetInputMode(FInputModeGameOnly());
	}
	const ESlateVisibility Visibility = bMatchEnded ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible;
	if (CrosshairWidgetInstance != nullptr)
	{
		CrosshairWidgetInstance->SetVisibility(Visibility);
	}
	if (HealthWidgetInstance != nullptr)
	{
		HealthWidgetInstance->SetVisibility(Visibility);
	}
	if (AmmoWidgetInstance != nullptr)
	{
		AmmoWidgetInstance->SetVisibility(Visibility);
	}
}

void AShooterHUD::CreateHUDWidgets()
{
	if (PlayerOwner == nullptr || !PlayerOwner->IsLocalController())
	{
		return;
	}

	// Keep match UI independent of optional health/crosshair configuration.
	if (MatchWidgetInstance == nullptr)
	{
		if (MatchWidgetClass == nullptr)
		{
			UE_LOG(LogShooterGame, Warning, TEXT("%s has no MatchWidgetClass assigned."), *GetName());
		}
		else
		{
			MatchWidgetInstance = CreateWidget<UShooterMatchWidget>(PlayerOwner, MatchWidgetClass);
			if (MatchWidgetInstance != nullptr)
			{
				MatchWidgetInstance->AddToPlayerScreen();
			}
		}
	}

	if (AmmoWidgetInstance == nullptr)
	{
		if (AmmoWidgetClass == nullptr)
		{
			UE_LOG(LogShooterGame, Warning, TEXT("%s has no AmmoWidgetClass assigned."), *GetName());
		}
		else
		{
			AmmoWidgetInstance = CreateWidget<UShooterAmmoWidget>(PlayerOwner, AmmoWidgetClass);
			if (AmmoWidgetInstance != nullptr)
			{
				AmmoWidgetInstance->AddToPlayerScreen();
				AmmoWidgetInstance->SetVisibility(bMatchEndedPresentation
					? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
			}
		}
	}

	if (CrosshairWidgetInstance == nullptr)
	{
		if (CrosshairWidgetClass == nullptr)
		{
			UE_LOG(LogShooterGame, Warning, TEXT("%s has no CrosshairWidgetClass assigned."), *GetName());
		}
		else
		{
			CrosshairWidgetInstance = CreateWidget<UUserWidget>(PlayerOwner, CrosshairWidgetClass);
			if (CrosshairWidgetInstance != nullptr)
			{
				CrosshairWidgetInstance->AddToPlayerScreen();
			}
		}
	}

	if (HealthWidgetInstance == nullptr)
	{
		if (HealthWidgetClass == nullptr)
		{
			UE_LOG(LogShooterGame, Warning, TEXT("%s has no HealthWidgetClass assigned."), *GetName());
			return;
		}

		HealthWidgetInstance = CreateWidget<UShooterHealthWidget>(PlayerOwner, HealthWidgetClass);
		if (HealthWidgetInstance != nullptr)
		{
			HealthWidgetInstance->AddToPlayerScreen();
			RefreshHealthBinding();
		}
	}

}

void AShooterHUD::RemoveHUDWidgets()
{
	if (AmmoWidgetInstance != nullptr)
	{
		AmmoWidgetInstance->RemoveFromParent();
		AmmoWidgetInstance = nullptr;
	}

	if (HealthWidgetInstance != nullptr)
	{
		HealthWidgetInstance->ClearHealthBinding();
		HealthWidgetInstance->RemoveFromParent();
		HealthWidgetInstance = nullptr;
	}

	if (CrosshairWidgetInstance != nullptr)
	{
		CrosshairWidgetInstance->RemoveFromParent();
		CrosshairWidgetInstance = nullptr;
	}

	if (MatchWidgetInstance != nullptr)
	{
		MatchWidgetInstance->RemoveFromParent();
		MatchWidgetInstance = nullptr;
	}
}

void AShooterHUD::RefreshHealthBinding()
{
	if (HealthWidgetInstance == nullptr)
	{
		return;
	}

	const APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(ObservedPawn);
	UShooterHealthComponent* HealthComponent = PlayerCharacter != nullptr ? PlayerCharacter->GetHealthComponent() : nullptr;
	HealthWidgetInstance->InitializeFromHealthComponent(HealthComponent);
}
