#pragma once

#include "Blueprint/UserWidget.h"
#include "ShooterHealthWidget.generated.h"

class UProgressBar;
class UShooterHealthComponent;
class UTextBlock;

UCLASS()
class SHOOTERGAME_API UShooterHealthWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Binds this widget to the local player's health adapter component.
	void InitializeFromHealthComponent(UShooterHealthComponent* InHealthComponent);

	// Removes any health delegate binding before the observed pawn changes.
	void ClearHealthBinding();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// Bound by the Blueprint widget; expected name: HealthProgressBar.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> HealthProgressBar;

	// Bound by the Blueprint widget; expected name: HealthPercentText.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> HealthPercentText;

private:
	// Updates the progress bar and percentage text from the currently bound health component.
	void RefreshHealthDisplay();

	// Receives health changes from the bound component.
	UFUNCTION()
	void HandleHealthChanged(float OldValue, float NewValue);

	// Returns whether the Blueprint supplied all required named widgets.
	bool HasRequiredBindings() const;

	TWeakObjectPtr<UShooterHealthComponent> ObservedHealthComponent;
};
