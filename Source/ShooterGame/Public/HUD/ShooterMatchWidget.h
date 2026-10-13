#pragma once

#include "Blueprint/UserWidget.h"
#include "ShooterMatchWidget.generated.h"

class UTextBlock;
class AShooterGameState;

// Layout lives in WBP_MatchStatus; this class only reads replicated match state.
UCLASS(Abstract)
class SHOOTERGAME_API UShooterMatchWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> MatchPhaseText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> MatchTimeText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ControlPointText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PlayerScoreText;

private:
	void RefreshMatchDisplay();
	void RefreshControlPointDisplay(const AShooterGameState* ShooterGameState);
};
