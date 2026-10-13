#include "GameMode/ShooterGameMode.h"
#include "Character/PlayerCharacter.h"
#include "Controller/ShooterPlayerController.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "Messages/ShooterGameplayMessageSubsystem.h"
#include "TimerManager.h"
#include "PlayerState/ShooterPlayerState.h"
#include "GameState/ShooterGameState.h"
#include "Gameplay/ShooterControlPoint.h"
#include "ShooterGame.h"

AShooterGameMode::AShooterGameMode()
{
	PlayerControllerClass = AShooterPlayerController::StaticClass();
	DefaultPawnClass = APlayerCharacter::StaticClass();
	PlayerStateClass = AShooterPlayerState::StaticClass();

	GameStateClass = AShooterGameState::StaticClass();

	// This first slice uses automatic start; the travel readiness gate comes next.
	bDelayedStart = false;
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
	if (Controller == nullptr)
	{
		return;
	}

	const TWeakObjectPtr<AController> WeakController(Controller);
	const TWeakObjectPtr<APlayerCharacter> WeakDeadCharacter(DeadCharacter);
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

	FTimerHandle RespawnTimerHandle;
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
	Super::HandleMatchHasStarted();

	AShooterGameState* ShooterGameState = GetGameState<AShooterGameState>();

	check(ShooterGameState);

	const double ServerTime = ShooterGameState->GetServerWorldTimeSeconds();

	ShooterGameState->SetMatchEndServerTime(ServerTime + MatchDuration);

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
	Super::RestartPlayer(NewPlayer);
	// Spawn overlaps can fire before Possess has supplied PlayerState/ASC.
	RefreshControlPointScoring();
}

void AShooterGameMode::Logout(AController* Exiting)
{
	if (AShooterControlPoint* Point = ControlPoint.Get(); Point != nullptr && Exiting != nullptr)
	{
		Point->RemoveCharacter(Cast<APlayerCharacter>(Exiting->GetPawn()));
	}
	Super::Logout(Exiting);
}

void AShooterGameMode::EndMatch()
{
	if (IsMatchInProgress())
	{
		StopControlPointScoring(GetScoringServerTime());
	}
	Super::EndMatch();
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
