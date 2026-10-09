#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ShooterGameMode.generated.h"

class APlayerCharacter;
class AController;
struct FShooterPlayerDeathMessage;

UCLASS()
class SHOOTERGAME_API AShooterGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	// Sets the project-default controller and pawn classes for gameplay maps.
	AShooterGameMode();

	// Schedules an automatic respawn for a controller whose pawn has entered death.
	void RequestPlayerRespawn(AController* Controller, APlayerCharacter* DeadCharacter);

	// Subscribes to gameplay messages once the world subsystem is available.
	virtual void BeginPlay() override;

	// Clears message subscriptions before this GameMode leaves the world.
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	// Delay between death starting and the next pawn being spawned.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shooter|Respawn", meta = (ClampMin = "0.0"))
	float RespawnDelay = 5.f;

private:
	// Responds to the authoritative player-death message.
	void HandlePlayerDeathMessage(const FShooterPlayerDeathMessage& Message);

	// Clears the old pawn and restarts the player if the controller is still valid.
	void HandleRespawnTimerExpired(TWeakObjectPtr<AController> Controller, TWeakObjectPtr<APlayerCharacter> DeadCharacter);

	FDelegateHandle PlayerDeathMessageListenerHandle;
};
