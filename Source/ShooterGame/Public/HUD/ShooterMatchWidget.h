#pragma once

#include "Blueprint/UserWidget.h"
#include "ShooterMatchWidget.generated.h"

class UTextBlock;
class AShooterGameState;
class UBorder;
class UShooterMatchResultsWidget;

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

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> LeadingScoreText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> MatchPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UShooterMatchResultsWidget> ResultsWidget;

private:
	void RefreshMatchDisplay();
	void RefreshControlPointDisplay(const AShooterGameState* ShooterGameState);
	// Switch HUD, pending results, and finalized results.
	void RefreshResultsDisplay(const AShooterGameState* State);
};
