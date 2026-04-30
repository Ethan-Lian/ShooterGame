#pragma once

#include "GameFramework/HUD.h"
#include "ShooterHUD.generated.h"

class APawn;
class UShooterHealthWidget;
class UUserWidget;

UCLASS()
class SHOOTERGAME_API AShooterHUD : public AHUD
{
	GENERATED_BODY()

public:
	AShooterHUD();

	// Rebinds local HUD data to the pawn currently owned by this player's controller.
	void SetObservedPawn(APawn* NewPawn);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Widget class used for the persistent center-screen crosshair.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD")
	TSubclassOf<UUserWidget> CrosshairWidgetClass;

	// Blueprint widget class used for the local player's health display.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD")
	TSubclassOf<UShooterHealthWidget> HealthWidgetClass;

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
};
