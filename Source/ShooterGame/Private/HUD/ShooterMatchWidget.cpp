#include "HUD/ShooterMatchWidget.h"

#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "GameFramework/PlayerController.h"
#include "HUD/ShooterHUD.h"
#include "HUD/ShooterMatchResultsWidget.h"
#include "Engine/World.h"
#include "GameFramework/GameMode.h"
#include "GameState/ShooterGameState.h"
#include "ShooterGame.h"
#include "PlayerState/ShooterPlayerState.h"

void UShooterMatchWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (MatchPhaseText == nullptr || MatchTimeText == nullptr || ControlPointText == nullptr || PlayerScoreText == nullptr
		|| LeadingScoreText == nullptr || MatchPanel == nullptr || ResultsWidget == nullptr)
	{
		UE_LOG(LogShooterGame, Warning,
			TEXT("%s is missing a required match HUD or results widget binding."), *GetName());
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	ResultsWidget->SetVisibility(ESlateVisibility::Collapsed);
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
	RefreshResultsDisplay(ShooterGameState);
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

	int32 LeadingScore = 0;
	if (ShooterGameState != nullptr)
	{
		for (const APlayerState* Candidate : ShooterGameState->PlayerArray)
		{
			if (Candidate != nullptr && !Candidate->IsOnlyASpectator() && !Candidate->IsInactive())
			{
				LeadingScore = FMath::Max(LeadingScore, FMath::RoundToInt(Candidate->GetScore()));
			}
		}
	}
	LeadingScoreText->SetText(FText::Format(NSLOCTEXT("ShooterMatch", "LeadingScore", "Leading score: {0}"), FText::AsNumber(LeadingScore)));
}

void UShooterMatchWidget::RefreshResultsDisplay(const AShooterGameState* State)
{
	const bool bMatchEnded =
		State != nullptr
		&& State->GetMatchPhase() == MatchState::WaitingPostMatch;

	MatchPanel->SetVisibility(
		bMatchEnded
			? ESlateVisibility::Collapsed
			: ESlateVisibility::SelfHitTestInvisible);

	ResultsWidget->SetVisibility(
		bMatchEnded
			? ESlateVisibility::SelfHitTestInvisible
			: ESlateVisibility::Collapsed);
	
	if (APlayerController* Controller = GetOwningPlayer())
	{
		if (AShooterHUD* HUD = Controller->GetHUD<AShooterHUD>())
		{
			HUD->SetMatchEndedPresentation(bMatchEnded,ResultsWidget);
		}
	}
	
	if (!bMatchEnded)
	{
		return;
	}

	const FShooterMatchResults& Results = State->GetMatchResults();

	if (!Results.bIsFinalized)
	{
		ResultsWidget->ShowPendingResults();
		return;
	}

	ResultsWidget->ShowResults(Results);
}
