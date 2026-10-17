#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "ShooterGameMode.generated.h"

class APlayerCharacter;
class AController;
class APlayerController;
class APlayerStart;
class AShooterControlPoint;
class AShooterPlayerState;
struct FShooterPlayerDeathMessage;
struct FShooterMatchResults;

UCLASS()
class SHOOTERGAME_API AShooterGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	// Sets the project-default controller and pawn classes for gameplay maps.
	AShooterGameMode();

	// Schedules an automatic respawn for a controller whose pawn has entered death.
	void RequestPlayerRespawn(AController* Controller, APlayerCharacter* DeadCharacter);

	// validate the local host and finalized results, then request travel.
	// True means travel entered LeavingMap, not that the next match has loaded.
	bool TryRestartMatch(APlayerController* RequestingController);

	void RegisterControlPoint(AShooterControlPoint* Point);
	void UnregisterControlPoint(AShooterControlPoint* Point);
	void NotifyControlPointOccupancyChanged(AShooterControlPoint* Point);

	virtual void RestartPlayer(AController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual void EndMatch() override;
	virtual void RestartGame() override;

	// Subscribes to gameplay messages once the world subsystem is available.
	virtual void BeginPlay() override;

	// Clears message subscriptions before this GameMode leaves the world.
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	// Delay between death starting and the next pawn being spawned.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shooter|Respawn", meta = (ClampMin = "0.0"))
	float RespawnDelay = 3.f;

	// Assigns this round's starts before the parent spawns Pawns and the clock begins.
	virtual void HandleMatchHasStarted() override;

	// wait for the existing room members to finish map travel.
	virtual bool ReadyToStartMatch_Implementation() override;

	// AGameMode polls this during InProgress and calls EndMatch when it returns true.
	virtual bool ReadyToEndMatch_Implementation() override;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Shooter|Match",
		meta = (ClampMin = "1.0"))
	float MatchDuration = 180.f;

private:
	static constexpr int32 MaxRoundPlayers = 4;

	// Frame-local borrows of participants and map-authored PlayerStarts.
	void CollectRoundSpawnData(
		TArray<APlayerController*>& OutPlayers,
		TArray<APlayerStart*>& OutStartPoints) const;

	// Runs once per round; default respawn reuses each Controller's StartSpot.
	void AssignRoundSpawnPoints();

	// Responds to the authoritative player-death message.
	void HandlePlayerDeathMessage(const FShooterPlayerDeathMessage& Message);

	// Clears the old pawn and restarts the player if the controller is still valid.
	void HandleRespawnTimerExpired(TWeakObjectPtr<AController> Controller, TWeakObjectPtr<APlayerCharacter> DeadCharacter);

	// Collect integer scores, mark every tied leader, then sort.
	FShooterMatchResults BuildMatchResults() const;

	// Called after Super::EndMatch closes the gameplay phase.
	void StopMatchGameplay();
	void ClearRespawnTimers();

	void RefreshControlPointScoring();

	// settle the old player before changing timing ownership.
	void ChangeScoringPlayer(AShooterPlayerState* NewScoringPlayer, double ServerTime);

	// Does not require InProgress: EndMatch settles before changing phase.
	void SettleControlPointScore(double ServerTime);
	void StopControlPointScoring(double ServerTime);
	void ScheduleNextControlPointScore();
	void HandleControlPointScoreTimer();
	double GetScoringServerTime() const;

	TWeakObjectPtr<AShooterControlPoint> ControlPoint;
	TWeakObjectPtr<AShooterPlayerState> ScoringPlayer;
	double LastScoringServerTime = 0.0;
	FTimerHandle ControlPointScoreTimerHandle;
	TMap<TWeakObjectPtr<AController>, FTimerHandle> RespawnTimers;

	FDelegateHandle PlayerDeathMessageListenerHandle;
};
