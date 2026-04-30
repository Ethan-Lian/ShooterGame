#include "GameMode/ShooterGameMode.h"
#include "Character/PlayerCharacter.h"
#include "Controller/ShooterPlayerController.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "TimerManager.h"
#include "PlayerState/ShooterPlayerState.h"

AShooterGameMode::AShooterGameMode()
{
	PlayerControllerClass = AShooterPlayerController::StaticClass();
	DefaultPawnClass = APlayerCharacter::StaticClass();
	PlayerStateClass = AShooterPlayerState::StaticClass();
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

void AShooterGameMode::HandleRespawnTimerExpired(TWeakObjectPtr<AController> Controller, TWeakObjectPtr<APlayerCharacter> DeadCharacter)
{
	AController* RespawningController = Controller.Get();
	if (RespawningController == nullptr)
	{
		return;
	}

	APawn* CurrentPawn = RespawningController->GetPawn();
	APlayerCharacter* DeadPawn = DeadCharacter.Get();
	if (CurrentPawn != nullptr && CurrentPawn != DeadPawn)
	{
		return;
	}

	if (AShooterPlayerState* ShooterPlayerState = RespawningController->GetPlayerState<AShooterPlayerState>())
	{
		ShooterPlayerState->ResetCombatStateForRespawn();
	}

	if (DeadPawn != nullptr)
	{
		if (CurrentPawn == DeadPawn)
		{
			RespawningController->UnPossess();
		}

		DeadPawn->Destroy();
	}

	RestartPlayer(RespawningController);
}
