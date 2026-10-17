#pragma once

#include "Blueprint/UserWidget.h"
#include "Gameplay/ShooterMatchResultTypes.h"
#include "ShooterMatchResultsWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class UShooterMatchResultRowWidget;

UCLASS(Abstract)
class SHOOTERGAME_API UShooterMatchResultsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UShooterMatchResultsWidget(const FObjectInitializer& ObjectInitializer);

	void ShowPendingResults();
	void ShowResults(const FShooterMatchResults& Results);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(EditDefaultsOnly, Category = "Shooter|Match")
	TSubclassOf<UShooterMatchResultRowWidget> ResultRowWidgetClass;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ResultsTitleText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ResultsSubtitleText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> ResultsRows;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ResultsStatusText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> HostStatusText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> PlayAgainButton;

private:
	void RefreshPlayAgainState();

	UFUNCTION()
	void HandlePlayAgainClicked();

	FText PlayAgainError;
	TArray<FShooterMatchResultRow> RenderedRows;
	int32 RenderedLocalPlayerId = INDEX_NONE;
	bool bHasRenderedResults = false;
};
