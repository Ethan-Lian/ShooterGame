#include "HUD/ShooterMatchWidget.h"

#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "GameFramework/GameMode.h"
#include "GameState/ShooterGameState.h"
#include "ShooterGame.h"
#include "PlayerState/ShooterPlayerState.h"

void UShooterMatchWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (MatchPhaseText == nullptr || MatchTimeText == nullptr || ControlPointText == nullptr || PlayerScoreText == nullptr)
	{
		UE_LOG(LogShooterGame, Warning,
			TEXT("%s requires MatchPhaseText, MatchTimeText, ControlPointText and PlayerScoreText."), *GetName());
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	RefreshMatchDisplay();
}

void UShooterMatchWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RefreshMatchDisplay();
}

void UShooterMatchWidget::RefreshMatchDisplay()
{
	if (MatchPhaseText == nullptr || MatchTimeText == nullptr)
	{
		return;
	}

	const AShooterGameState* ShooterGameState = GetWorld()->GetGameState<AShooterGameState>();
	const FName Phase = ShooterGameState != nullptr ? ShooterGameState->GetMatchPhase() : NAME_None;
	FText PhaseText = NSLOCTEXT("ShooterMatch", "Waiting", "Waiting for match");
	FText TimeText = FText::FromString(TEXT("--:--"));

	if (Phase == MatchState::InProgress)
	{
		if (ShooterGameState->HasMatchEndTime())
		{
			PhaseText = NSLOCTEXT("ShooterMatch", "InProgress", "Match in progress");
			const int32 SecondsLeft = FMath::CeilToInt(ShooterGameState->GetRemainingMatchTime());
			TimeText = FText::FromString(FString::Printf(TEXT("%02d:%02d"), SecondsLeft / 60, SecondsLeft % 60));
		}
		else
		{
			PhaseText = NSLOCTEXT("ShooterMatch", "Syncing", "Syncing match time");
		}
	}
	else if (Phase == MatchState::WaitingPostMatch)
	{
		PhaseText = NSLOCTEXT("ShooterMatch", "Ended", "Match ended");
		TimeText = FText::FromString(TEXT("00:00"));
	}
	else if (Phase == MatchState::LeavingMap)
	{
		PhaseText = NSLOCTEXT("ShooterMatch", "Leaving", "Loading next map");
	}
	else if (Phase == MatchState::Aborted)
	{
		PhaseText = NSLOCTEXT("ShooterMatch", "Aborted", "Match stopped");
	}

	MatchPhaseText->SetText(PhaseText);
	MatchTimeText->SetText(TimeText);
	RefreshControlPointDisplay(ShooterGameState);
}

void UShooterMatchWidget::RefreshControlPointDisplay(const AShooterGameState* ShooterGameState)
{
	if (ControlPointText == nullptr || PlayerScoreText == nullptr)
	{
		return;
	}

	FText PointText = NSLOCTEXT("ShooterMatch", "PointEmpty", "Point empty");
	if (ShooterGameState != nullptr)
	{
		const FShooterControlPointState State = ShooterGameState->GetControlPointState();
		if (State.Status == EShooterControlPointStatus::Held)
		{
			PointText = NSLOCTEXT("ShooterMatch", "PointHeld", "Point held");
		}
		else if (State.Status == EShooterControlPointStatus::Contested)
		{
			PointText = NSLOCTEXT("ShooterMatch", "PointContested", "Point contested - scoring paused");
		}
	}
	ControlPointText->SetText(PointText);

	const AShooterPlayerState* PlayerState = GetOwningPlayerState<AShooterPlayerState>();
	const FText Score = PlayerState != nullptr ? FText::AsNumber(PlayerState->GetScore()) : FText::FromString(TEXT("--"));
	PlayerScoreText->SetText(FText::Format(NSLOCTEXT("ShooterMatch", "PlayerScore", "Your score: {0}"), Score));
}
