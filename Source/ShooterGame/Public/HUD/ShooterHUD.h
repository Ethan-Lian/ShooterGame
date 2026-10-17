#pragma once

#include "GameFramework/HUD.h"
#include "ShooterHUD.generated.h"

class APawn;
class UShooterAmmoWidget;
class UShooterHealthWidget;
class UShooterMatchWidget;
class UUserWidget;

UCLASS()
class SHOOTERGAME_API AShooterHUD : public AHUD
{
	GENERATED_BODY()

public:
	AShooterHUD();

	// Rebinds local HUD data to the pawn currently owned by this player's controller.
	void SetObservedPawn(APawn* NewPawn);

	// Idempotent local presentation; does not decide or change match results.
	void SetMatchEndedPresentation(bool bMatchEnded, UUserWidget* FocusWidget);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Widget class used for the persistent center-screen crosshair.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD")
	TSubclassOf<UUserWidget> CrosshairWidgetClass;

	// Blueprint widget class used for the local player's health display.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD")
	TSubclassOf<UShooterHealthWidget> HealthWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD")
	TSubclassOf<UShooterMatchWidget> MatchWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD")
	TSubclassOf<UShooterAmmoWidget> AmmoWidgetClass;

private:
	// Creates all local screen-space HUD widgets once.
	void CreateHUDWidgets();

	// Removes screen-space HUD widgets owned by this HUD.
	void RemoveHUDWidgets();

	// Binds the health widget to the currently observed pawn, if possible.
	void RefreshHealthBinding();

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> CrosshairWidgetInstance;

	UPROPERTY(Transient)
	TObjectPtr<UShooterHealthWidget> HealthWidgetInstance;

	UPROPERTY(Transient)
	TObjectPtr<APawn> ObservedPawn;

	UPROPERTY(Transient)
	TObjectPtr<UShooterMatchWidget> MatchWidgetInstance;

	UPROPERTY(Transient)
	TObjectPtr<UShooterAmmoWidget> AmmoWidgetInstance;

	bool bMatchEndedPresentation = false;
};
