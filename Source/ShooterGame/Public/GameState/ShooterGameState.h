#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "ShooterGameState.generated.h"

UCLASS()
class SHOOTERGAME_API AShooterGameState : public AGameState
{
	GENERATED_BODY()

public:

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Called by GameMode on the server.
	void SetMatchEndServerTime(double NewEndTime);

	UFUNCTION(BlueprintPure, Category = "Shooter|Match")
	double GetRemainingMatchTime() const;

	UFUNCTION(BlueprintPure, Category = "Shooter|Match")
	FName GetMatchPhase() const;

	// MatchState and the end time can arrive in separate replication updates.
	bool HasMatchEndTime() const;

private:
	UPROPERTY(Replicated)
	double MatchEndServerTime = 0.0;
};
