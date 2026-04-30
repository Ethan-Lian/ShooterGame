#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ShooterGameMode.generated.h"

class APlayerCharacter;
class AController;

UCLASS()
class SHOOTERGAME_API AShooterGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	// Sets the project-default controller and pawn classes for gameplay maps.
	AShooterGameMode();

	// Schedules an automatic respawn for a controller whose pawn has entered death.
	void RequestPlayerRespawn(AController* Controller, APlayerCharacter* DeadCharacter);

protected:
	// Delay between death starting and the next pawn being spawned.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shooter|Respawn", meta = (ClampMin = "0.0"))
	float RespawnDelay = 5.f;

private:
	// Clears the old pawn and restarts the player if the controller is still valid.
	void HandleRespawnTimerExpired(TWeakObjectPtr<AController> Controller, TWeakObjectPtr<APlayerCharacter> DeadCharacter);
};
