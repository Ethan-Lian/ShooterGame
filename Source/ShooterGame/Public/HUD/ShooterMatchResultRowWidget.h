#pragma once

#include "Blueprint/UserWidget.h"
#include "ShooterMatchResultRowWidget.generated.h"

class UBorder;
class UTextBlock;
struct FShooterMatchResultRow;

UCLASS(Abstract)
class SHOOTERGAME_API UShooterMatchResultRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetResult(const FShooterMatchResultRow& Result, int32 DisplayIndex, bool bIsLocalPlayer);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> ResultRowBackground;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> RankText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PlayerNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ResultScoreText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> WinnerText;
};
