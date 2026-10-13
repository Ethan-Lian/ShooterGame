#include "GameMode/ShooterGameMode.h"
#include "Character/PlayerCharacter.h"
#include "Controller/ShooterPlayerController.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "Messages/ShooterGameplayMessageSubsystem.h"
#include "TimerManager.h"
#include "PlayerState/ShooterPlayerState.h"
#include "GameState/ShooterGameState.h"

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
