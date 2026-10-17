#include "HUD/ShooterMatchResultsWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Controller/ShooterPlayerController.h"
#include "Engine/World.h"
#include "GameFramework/GameMode.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameState/ShooterGameState.h"
#include "HUD/ShooterMatchResultRowWidget.h"
#include "ShooterGame.h"

UShooterMatchResultsWidget::UShooterMatchResultsWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);
}

void UShooterMatchResultsWidget::NativeConstruct()
{
	Super::NativeConstruct();
	PlayAgainButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandlePlayAgainClicked);
	RefreshPlayAgainState();
}

void UShooterMatchResultsWidget::NativeDestruct()
{
	PlayAgainButton->OnClicked.RemoveDynamic(this, &ThisClass::HandlePlayAgainClicked);
	Super::NativeDestruct();
}

void UShooterMatchResultsWidget::RefreshPlayAgainState()
{
	const AShooterPlayerController* Controller = GetOwningPlayer<AShooterPlayerController>();
	const AShooterGameState* State = GetWorld()->GetGameState<AShooterGameState>();
	const bool bIsHost = Controller != nullptr && Controller->HasAuthority() && Controller->IsLocalPlayerController();
	const bool bLeavingMap = State != nullptr && State->GetMatchPhase() == MatchState::LeavingMap;
	const bool bCanRestart = State != nullptr && State->GetMatchPhase() == MatchState::WaitingPostMatch
		&& State->GetMatchResults().bIsFinalized;

	PlayAgainButton->SetVisibility(bIsHost ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	PlayAgainButton->SetIsEnabled(bIsHost && bCanRestart);

	FText Status;
	if (bLeavingMap)
	{
		Status = NSLOCTEXT("ShooterMatch", "RestartLoading", "Loading the next match...");
	}
	else if (!bIsHost)
	{
		Status = NSLOCTEXT("ShooterMatch", "WaitingForHost", "Waiting for the host");
	}
	else if (!PlayAgainError.IsEmpty())
	{
		Status = PlayAgainError;
	}
	else
	{
		Status = bCanRestart
			? NSLOCTEXT("ShooterMatch", "HostCanRestart", "Play again when you are ready")
			: NSLOCTEXT("ShooterMatch", "HostWaitingForResults", "Waiting for final scores...");
	}
	HostStatusText->SetText(Status);
}

void UShooterMatchResultsWidget::HandlePlayAgainClicked()
{
	AShooterPlayerController* Controller = GetOwningPlayer<AShooterPlayerController>();
	if (Controller == nullptr)
	{
		return;
	}

	// Prepare local presentation before travel can invoke external callbacks.
	PlayAgainError = FText::GetEmpty();
	PlayAgainButton->SetIsEnabled(false);
	HostStatusText->SetText(NSLOCTEXT("ShooterMatch", "RestartLoading", "Loading the next match..."));
	if (!Controller->RequestPlayAgain())
	{
		PlayAgainError = NSLOCTEXT("ShooterMatch", "RestartRejected", "Could not restart the match. Try again.");
		RefreshPlayAgainState();
	}
}

void UShooterMatchResultsWidget::ShowPendingResults()
{
	RefreshPlayAgainState();
	ResultsTitleText->SetText(NSLOCTEXT("ShooterMatch", "MatchComplete", "MATCH COMPLETE"));
	ResultsSubtitleText->SetText(NSLOCTEXT("ShooterMatch", "SyncingResults", "Synchronizing final scores"));
	ResultsStatusText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	ResultsStatusText->SetText(NSLOCTEXT("ShooterMatch", "ResultsPending", "Waiting for final scores..."));
}

void UShooterMatchResultsWidget::ShowResults(const FShooterMatchResults& Results)
{
	// MatchState can change while the cached result rows remain identical.
	RefreshPlayAgainState();
	const APlayerState* LocalPlayerState = GetOwningPlayerState();
	const int32 LocalPlayerId = LocalPlayerState != nullptr ? LocalPlayerState->GetPlayerId() : INDEX_NONE;
	if (bHasRenderedResults && RenderedRows == Results.PlayersResultRows && RenderedLocalPlayerId == LocalPlayerId)
	{
		return;
	}
	if (ResultRowWidgetClass == nullptr)
	{
		UE_LOG(LogShooterGame, Warning, TEXT("%s requires ResultRowWidgetClass."), *GetName());
		return;
	}

	ResultsRows->ClearChildren();
	int32 WinnerCount = 0;
	bool bLocalPlayerWon = false;
	for (int32 Index = 0; Index < Results.PlayersResultRows.Num(); ++Index)
	{
		const FShooterMatchResultRow& Result = Results.PlayersResultRows[Index];
		const bool bIsLocalPlayer = LocalPlayerId != INDEX_NONE && Result.PlayerId == LocalPlayerId;
		UShooterMatchResultRowWidget* Row = CreateWidget<UShooterMatchResultRowWidget>(GetOwningPlayer(), ResultRowWidgetClass);
		UVerticalBoxSlot* RowSlot = ResultsRows->AddChildToVerticalBox(Row);
		RowSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
		Row->SetResult(Result, Index + 1, bIsLocalPlayer);
		WinnerCount += Result.bIsWinner ? 1 : 0;
		bLocalPlayerWon |= bIsLocalPlayer && Result.bIsWinner;
	}

	FText Title = NSLOCTEXT("ShooterMatch", "MatchComplete", "MATCH COMPLETE");
	if (bLocalPlayerWon)
	{
		Title = WinnerCount > 1
			? NSLOCTEXT("ShooterMatch", "JointVictory", "JOINT VICTORY")
			: NSLOCTEXT("ShooterMatch", "YouWin", "YOU WIN");
	}
	ResultsTitleText->SetText(Title);
	const int32 TopScore = Results.PlayersResultRows.IsEmpty() ? 0 : Results.PlayersResultRows[0].Score;
	ResultsSubtitleText->SetText(FText::Format(
		NSLOCTEXT("ShooterMatch", "FinalScoreSummary", "Highest score: {0}  |  Winners: {1}"),
		FText::AsNumber(TopScore), FText::AsNumber(WinnerCount)));
	ResultsStatusText->SetText(NSLOCTEXT("ShooterMatch", "NoParticipants", "No participating players"));
	ResultsStatusText->SetVisibility(Results.PlayersResultRows.IsEmpty()
		? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);

	RenderedRows = Results.PlayersResultRows;
	RenderedLocalPlayerId = LocalPlayerId;
	bHasRenderedResults = true;
}
