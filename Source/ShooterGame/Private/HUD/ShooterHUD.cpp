#include "HUD/ShooterHUD.h"

#include "Blueprint/UserWidget.h"
#include "Character/PlayerCharacter.h"
#include "Components/ShooterHealthComponent.h"
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

	CreateHUDWidgets();
	SetObservedPawn(PlayerOwner != nullptr ? PlayerOwner->GetPawn() : nullptr);
}

void AShooterHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveHUDWidgets();
	Super::EndPlay(EndPlayReason);
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
