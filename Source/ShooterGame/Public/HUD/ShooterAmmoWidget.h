#pragma once

#include "Blueprint/UserWidget.h"
#include "ShooterAmmoWidget.generated.h"

class UBorder;
class UTextBlock;

// Layout lives in WBP_ShooterAmmoWidget; gameplay ammo remains in Inventory.
UCLASS(Abstract)
class SHOOTERGAME_API UShooterAmmoWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> AmmoPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> AmmoCountText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ReloadHintText;

private:
	void RefreshAmmoDisplay();
};
