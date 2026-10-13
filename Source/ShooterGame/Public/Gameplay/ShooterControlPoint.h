#pragma once

#include "GameFramework/Actor.h"
#include "Gameplay/ShooterControlPointTypes.h"
#include "ShooterControlPoint.generated.h"

class APlayerCharacter;
class UBoxComponent;
class UPrimitiveComponent;
class UTextRenderComponent;

// Place exactly one in the gameplay map. Only the server tracks occupancy.
UCLASS()
class SHOOTERGAME_API AShooterControlPoint : public AActor
{
	GENERATED_BODY()

public:
	AShooterControlPoint();

	// classify the currently overlapping, living players.
	FShooterControlPointState EvaluateOccupancy() const;

	// Geometry candidates only: Capsule overlaps, not a list of living players.
	void GetOverlappingCharacters(TArray<APlayerCharacter*>& OutCharacters) const;

	// Death/Logout can remove a candidate before its Pawn is destroyed.
	void RemoveCharacter(APlayerCharacter* Character);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shooter|ControlPoint")
	TObjectPtr<UBoxComponent> CaptureVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shooter|ControlPoint")
	TObjectPtr<UTextRenderComponent> PointLabel;

private:
	void TrackCharacter(APlayerCharacter* Character);
	void NotifyOccupancyChanged();

	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex);

	UFUNCTION()
	void HandleCharacterEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason);

	TSet<TWeakObjectPtr<APlayerCharacter>> OverlappingCharacters;
};
