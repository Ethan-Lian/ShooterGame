#include "GameMode/ShooterGameMode.h"

#include "EngineUtils.h"
#include "Character/PlayerCharacter.h"
#include "Controller/ShooterPlayerController.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerState.h"
#include "Messages/ShooterGameplayMessageSubsystem.h"
#include "TimerManager.h"
#include "PlayerState/ShooterPlayerState.h"
#include "GameState/ShooterGameState.h"
#include "Gameplay/ShooterControlPoint.h"
#include "ShooterGame.h"
#include "AbilitySystem/ShooterAbilitySystemComponent.h"
#include "Components/ShooterCombatComponent.h"
#include "Components/ShooterMovementStateComponent.h"
#include "Engine/PlayerStartPIE.h"
#include "GameFramework/CharacterMovementComponent.h"

AShooterGameMode::AShooterGameMode()
{
	PlayerControllerClass = AShooterPlayerController::StaticClass();
	DefaultPawnClass = APlayerCharacter::StaticClass();
	PlayerStateClass = AShooterPlayerState::StaticClass();

	GameStateClass = AShooterGameState::StaticClass();

	// ReadyToStartMatch supplies the loading gate instead of a manual StartMatch call.
	bDelayedStart = false;
}

bool AShooterGameMode::TryRestartMatch(APlayerController* RequestingController)
{
	if (RequestingController == nullptr
		|| RequestingController->GetWorld() != GetWorld()
		|| !RequestingController->HasAuthority()
		|| !RequestingController->IsLocalPlayerController()
		|| GetMatchState() != MatchState::WaitingPostMatch)
	{
		return false;
	}

	const AShooterGameState* State = GetGameState<AShooterGameState>();

	if (State == nullptr || !State->GetMatchResults().bIsFinalized)
	{
		return false;
	}

	RestartGame();

	return GetMatchState() == MatchState::LeavingMap;
}

void AShooterGameMode::RestartGame()
{
	// Set on the current instance so Blueprint defaults cannot select hard travel.
	bUseSeamlessTravel = true;
	Super::RestartGame();
}

void AShooterGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (UShooterGameplayMessageSubsystem* MessageSubsystem = UShooterGameplayMessageSubsystem::Get(this))
	{
		PlayerDeathMessageListenerHandle = MessageSubsystem->RegisterPlayerDeathListener(
			FShooterPlayerDeathMessageDelegate::FDelegate::CreateUObject(
				this,
				&AShooterGameMode::HandlePlayerDeathMessage));
	}
}

void AShooterGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearRespawnTimers();
	GetWorldTimerManager().ClearTimer(ControlPointScoreTimerHandle);
	ScoringPlayer.Reset();
	ControlPoint.Reset();

	if (UShooterGameplayMessageSubsystem* MessageSubsystem = UShooterGameplayMessageSubsystem::Get(this))
	{
		MessageSubsystem->UnregisterPlayerDeathListener(PlayerDeathMessageListenerHandle);
	}

	PlayerDeathMessageListenerHandle.Reset();
	Super::EndPlay(EndPlayReason);
}

void AShooterGameMode::RequestPlayerRespawn(AController* Controller, APlayerCharacter* DeadCharacter)
{
	const AShooterGameState* ShooterGameState = GetGameState<AShooterGameState>();
	if (Controller == nullptr || !ShooterGameState->IsMatchGameplayAllowed())
	{
		return;
	}

	const TWeakObjectPtr<AController> WeakController(Controller);
	const TWeakObjectPtr<APlayerCharacter> WeakDeadCharacter(DeadCharacter);
	if (FTimerHandle* ExistingTimer = RespawnTimers.Find(WeakController))
	{
		GetWorldTimerManager().ClearTimer(*ExistingTimer);
		RespawnTimers.Remove(WeakController);
	}
	if (RespawnDelay <= 0.f)
	{
		HandleRespawnTimerExpired(WeakController, WeakDeadCharacter);
		return;
	}

	FTimerDelegate RespawnDelegate;
	RespawnDelegate.BindUObject(
		this,
		&AShooterGameMode::HandleRespawnTimerExpired,
		WeakController,
		WeakDeadCharacter);

	FTimerHandle& RespawnTimerHandle = RespawnTimers.FindOrAdd(WeakController);
	GetWorldTimerManager().SetTimer(RespawnTimerHandle, RespawnDelegate, RespawnDelay, false);
}

void AShooterGameMode::HandlePlayerDeathMessage(const FShooterPlayerDeathMessage& Message)
{
	if (AShooterControlPoint* Point = ControlPoint.Get())
	{
		Point->RemoveCharacter(Cast<APlayerCharacter>(Message.DeadPawn.Get()));
	}
	RequestPlayerRespawn(Message.Controller, Cast<APlayerCharacter>(Message.DeadPawn.Get()));
}

void AShooterGameMode::HandleRespawnTimerExpired(TWeakObjectPtr<AController> Controller, TWeakObjectPtr<APlayerCharacter> DeadCharacter)
{
	RespawnTimers.Remove(Controller);
	if (!GetGameState<AShooterGameState>()->IsMatchGameplayAllowed())
	{
		return;
	}
	AController* RespawningController = Controller.Get();
	if (RespawningController == nullptr)
	{
		return;
	}

	APawn* CurrentPawn = RespawningController->GetPawn();
	APlayerCharacter* DeadPawn = DeadCharacter.Get();
	if (DeadPawn == nullptr)
	{
		return;
	}
	if (CurrentPawn != nullptr && CurrentPawn != DeadPawn)
	{
		return;
	}

	if (DeadPawn != nullptr)
	{
		if (CurrentPawn == DeadPawn)
		{
			RespawningController->UnPossess();
		}

		DeadPawn->Destroy();
	}

	if (AShooterPlayerState* ShooterPlayerState = RespawningController->GetPlayerState<AShooterPlayerState>())
	{
		ShooterPlayerState->ResetCombatStateForRespawn();
	}

	RestartPlayer(RespawningController);
}

void AShooterGameMode::HandleMatchHasStarted()
{
	AssignRoundSpawnPoints();
	Super::HandleMatchHasStarted();

	AShooterGameState* ShooterGameState = GetGameState<AShooterGameState>();

	check(ShooterGameState);

	const double ServerTime = ShooterGameState->GetServerWorldTimeSeconds();

	ShooterGameState->SetMatchEndServerTime(ServerTime + MatchDuration);
	ShooterGameState->ResetMatchResults();

	for (APlayerState* PlayerState : ShooterGameState->PlayerArray)
	{
		if (AShooterPlayerState* ShooterPlayerState = Cast<AShooterPlayerState>(PlayerState))
		{
			ShooterPlayerState->ResetMatchScore();
		}
	}
	ScoringPlayer.Reset();
	LastScoringServerTime = ServerTime;
	RefreshControlPointScoring();
}

bool AShooterGameMode::ReadyToStartMatch_Implementation()
{
	if (GetMatchState() != MatchState::WaitingToStart
		|| NumPlayers <= 0
		|| NumPlayers > MaxRoundPlayers
		|| NumTravellingPlayers != 0)
	{
		return false;
	}

	TArray<APlayerController*> Players;
	TArray<APlayerStart*> StartPoints;
	CollectRoundSpawnData(Players, StartPoints);

	return Players.Num() == NumPlayers && StartPoints.Num() >= Players.Num();
}

bool AShooterGameMode::ReadyToEndMatch_Implementation()
{
	if (!IsMatchInProgress())
	{
		return false;
	}

	const AShooterGameState* ShooterGameState = GetGameState<AShooterGameState>();

	check(ShooterGameState);

	return ShooterGameState->HasMatchEndTime() && ShooterGameState->GetRemainingMatchTime() <= 0.0;
}

void AShooterGameMode::RegisterControlPoint(AShooterControlPoint* Point)
{
	if (ControlPoint.IsValid() && ControlPoint.Get() != Point)
	{
		UE_LOG(LogShooterGame, Error, TEXT("Only one ShooterControlPoint may exist in the gameplay map."));
		return;
	}
	ControlPoint = Point;
	RefreshControlPointScoring();
}

void AShooterGameMode::UnregisterControlPoint(AShooterControlPoint* Point)
{
	if (ControlPoint.Get() == Point)
	{
		ControlPoint.Reset();
		RefreshControlPointScoring();
	}
}

void AShooterGameMode::NotifyControlPointOccupancyChanged(AShooterControlPoint* Point)
{
	if (ControlPoint.Get() == Point)
	{
		RefreshControlPointScoring();
	}
}

void AShooterGameMode::RestartPlayer(AController* NewPlayer)
{
	if (HasMatchEnded())
	{
		return;
	}
	Super::RestartPlayer(NewPlayer);
	// Spawn overlaps can fire before Possess has supplied PlayerState/ASC.
	RefreshControlPointScoring();
}

void AShooterGameMode::Logout(AController* Exiting)
{
	const TWeakObjectPtr<AController> WeakController(Exiting);
	if (FTimerHandle* Timer = RespawnTimers.Find(WeakController))
	{
		GetWorldTimerManager().ClearTimer(*Timer);
		RespawnTimers.Remove(WeakController);
	}
	if (AShooterControlPoint* Point = ControlPoint.Get(); Point != nullptr && Exiting != nullptr)
	{
		Point->RemoveCharacter(Cast<APlayerCharacter>(Exiting->GetPawn()));
	}
	Super::Logout(Exiting);
}

void AShooterGameMode::EndMatch()
{
	if (!IsMatchInProgress())
	{
		return;
	}

	StopControlPointScoring(GetScoringServerTime());

	const FShooterMatchResults Results = BuildMatchResults();

	Super::EndMatch();

	StopMatchGameplay();

	GetGameState<AShooterGameState>()->SetMatchResults(Results);
}

FShooterMatchResults AShooterGameMode::BuildMatchResults() const
{
	const AShooterGameState* State = GetGameState<AShooterGameState>();
	
	FShooterMatchResults Results;
	Results.bIsFinalized = true;
	int32 HighestScore = 0;
	
	for (const APlayerState* player : State->PlayerArray)
	{
		const AShooterPlayerState* ShooterPlayer = Cast<AShooterPlayerState>(player);
		
		if (ShooterPlayer == nullptr
			|| ShooterPlayer->IsOnlyASpectator()
			|| ShooterPlayer->IsInactive())
		{
			continue;
		}
	
		FShooterMatchResultRow& Row = Results.PlayersResultRows.AddDefaulted_GetRef();
		Row.PlayerId = ShooterPlayer->GetPlayerId();
		Row.PlayerName = ShooterPlayer->GetPlayerName();
		Row.Score = FMath::RoundToInt(ShooterPlayer->GetScore());
		
		HighestScore = FMath::Max(HighestScore,Row.Score);
	}
	
	for (FShooterMatchResultRow& Row : Results.PlayersResultRows)
	{
		Row.bIsWinner = Row.Score == HighestScore;
	}
	
	Results.PlayersResultRows.Sort(
		[](const FShooterMatchResultRow& A,
		   const FShooterMatchResultRow& B)
		{
			return A.Score != B.Score
				? A.Score > B.Score
				: A.PlayerId < B.PlayerId;
		});

	return Results;
}

void AShooterGameMode::ClearRespawnTimers()
{
	for (auto& Entry : RespawnTimers)
	{
		GetWorldTimerManager().ClearTimer(Entry.Value);
	}
	RespawnTimers.Empty();
}

void AShooterGameMode::StopMatchGameplay()
{
	ClearRespawnTimers();
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PlayerController = It->Get();
		if (AShooterPlayerState* PlayerState = PlayerController->GetPlayerState<AShooterPlayerState>())
		{
			UShooterAbilitySystemComponent* ASC = Cast<UShooterAbilitySystemComponent>(PlayerState->GetAbilitySystemComponent());
			ASC->ClearAbilityInput();
			ASC->CancelAllAbilities();
		}
		// Ability cancellation can change possession; resolve the current Pawn afterwards.
		if (APlayerCharacter* Character = Cast<APlayerCharacter>(PlayerController->GetPawn()))
		{
			Character->GetCombatComponent()->HandleMatchEnded();
			Character->GetMovementStateComponent()->StopSprintInput();
			Character->GetCharacterMovement()->StopMovementImmediately();
			Character->GetCharacterMovement()->DisableMovement();
		}
	}
}

void AShooterGameMode::RefreshControlPointScoring()
{
	const AShooterGameState* ShooterGameState = GetGameState<AShooterGameState>();
	if (!IsMatchInProgress() || ShooterGameState == nullptr || !ShooterGameState->HasMatchEndTime())
	{
		return;
	}

	const double ServerTime = GetScoringServerTime();
	if (ShooterGameState->GetRemainingMatchTime() <= 0.0)
	{
		StopControlPointScoring(ServerTime);
		return;
	}

	const AShooterControlPoint* Point = ControlPoint.Get();
	const FShooterControlPointState NewState = Point != nullptr ? Point->EvaluateOccupancy() : FShooterControlPointState{};
	AShooterPlayerState* NewScoringPlayer = NewState.Status == EShooterControlPointStatus::Held ? NewState.SolePlayer.Get() : nullptr;
	ChangeScoringPlayer(NewScoringPlayer, ServerTime);
	GetGameState<AShooterGameState>()->SetControlPointState(NewState);
	ScheduleNextControlPointScore();
}

void AShooterGameMode::ChangeScoringPlayer(AShooterPlayerState* NewScoringPlayer, double ServerTime)
{
	SettleControlPointScore(ServerTime);
	
	ScoringPlayer = NewScoringPlayer;
	LastScoringServerTime = ServerTime;
}

void AShooterGameMode::SettleControlPointScore(double ServerTime)
{
	AShooterPlayerState* Player = ScoringPlayer.Get();
	const double ElapsedTime = FMath::Max(0.0, ServerTime - LastScoringServerTime);
	LastScoringServerTime = ServerTime;
	if (Player != nullptr && ElapsedTime > 0.0)
	{
		// Settle saved ownership, even if this player just died or left the volume.
		Player->AddControlTime(ElapsedTime);
	}
}

void AShooterGameMode::StopControlPointScoring(double ServerTime)
{
	SettleControlPointScore(ServerTime);
	GetWorldTimerManager().ClearTimer(ControlPointScoreTimerHandle);
	ScoringPlayer.Reset();
}

void AShooterGameMode::ScheduleNextControlPointScore()
{
	GetWorldTimerManager().ClearTimer(ControlPointScoreTimerHandle);
	const AShooterPlayerState* Player = ScoringPlayer.Get();
	const AShooterGameState* ShooterGameState = GetGameState<AShooterGameState>();
	if (Player == nullptr || !IsMatchInProgress() || ShooterGameState->GetRemainingMatchTime() <= 0.0)
	{
		return;
	}

	const double Delay = FMath::Min(1.0 - Player->GetControlTimeRemainder(), ShooterGameState->GetRemainingMatchTime());
	GetWorldTimerManager().SetTimer(ControlPointScoreTimerHandle, this,
		&ThisClass::HandleControlPointScoreTimer, static_cast<float>(Delay), false);
}

void AShooterGameMode::HandleControlPointScoreTimer()
{
	RefreshControlPointScoring();
}

double AShooterGameMode::GetScoringServerTime() const
{
	const AShooterGameState* ShooterGameState = GetGameState<AShooterGameState>();
	check(ShooterGameState);
	return FMath::Min(ShooterGameState->GetServerWorldTimeSeconds(), ShooterGameState->GetMatchEndServerTime());
}

void AShooterGameMode::CollectRoundSpawnData(
	TArray<APlayerController*>& OutPlayers,
	TArray<APlayerStart*>& OutStartPoints) const
{
	OutPlayers.Reset();
	OutStartPoints.Reset();

	UWorld* World = GetWorld();

	for (FConstPlayerControllerIterator It =
		World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* Controller = It->Get();
		const APlayerState* PlayerState = Controller != nullptr
			? Controller->GetPlayerState<APlayerState>()
			: nullptr;

		if (PlayerState != nullptr
			&& !PlayerState->IsOnlyASpectator()
			&& !PlayerState->IsInactive())
		{
			OutPlayers.Add(Controller);
		}
	}

	for (TActorIterator<APlayerStart> It(World); It; ++It)
	{
		APlayerStart* StartPoint = *It;

		// Exclude temporary "Play From Here" starts.
		if (!StartPoint->IsA<APlayerStartPIE>())
		{
			OutStartPoints.Add(StartPoint);
		}
	}
}

void AShooterGameMode::AssignRoundSpawnPoints()
{
	check(HasAuthority());

	TArray<APlayerController*> Players;
	TArray<APlayerStart*> StartPoints;
	CollectRoundSpawnData(Players, StartPoints);

	// ReadyToStartMatch guarantees these conditions before StartMatch.
	check(Players.Num() > 0 && Players.Num() <= MaxRoundPlayers);
	check(StartPoints.Num() >= Players.Num());

	// Fisher-Yates shuffle: each ordering has the same probability.
	for (int32 Index = StartPoints.Num() - 1; Index > 0; --Index)
	{
		const int32 RandomIndex = FMath::RandRange(0, Index);
		StartPoints.Swap(Index, RandomIndex);
	}

	for (int32 Index = 0; Index < Players.Num(); ++Index)
	{
		Players[Index]->StartSpot = StartPoints[Index];

		UE_LOG(
			LogShooterGame,
			Log,
			TEXT("Round spawn: %s -> %s"),
			*Players[Index]->GetPlayerState<APlayerState>()->GetPlayerName(),
			*StartPoints[Index]->GetName());
	}
}
