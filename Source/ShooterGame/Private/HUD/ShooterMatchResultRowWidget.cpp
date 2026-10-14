#include "HUD/ShooterMatchResultRowWidget.h"

#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Gameplay/ShooterMatchResultTypes.h"

void UShooterMatchResultRowWidget::SetResult(const FShooterMatchResultRow& Result, int32 DisplayIndex, bool bIsLocalPlayer)
{
	RankText->SetText(FText::AsNumber(DisplayIndex));
	PlayerNameText->SetText(FText::FromString(Result.PlayerName));
	ResultScoreText->SetText(FText::AsNumber(Result.Score));
	WinnerText->SetText(Result.bIsWinner
		? NSLOCTEXT("ShooterMatch", "Winner", "WINNER") : FText::GetEmpty());
	ResultRowBackground->SetBrushColor(bIsLocalPlayer
		? FLinearColor(0.025f, 0.085f, 0.105f, 1.f) : FLinearColor(0.028f, 0.043f, 0.06f, 1.f));
}
