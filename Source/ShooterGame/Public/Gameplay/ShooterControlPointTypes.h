#pragma once

#include "CoreMinimal.h"
#include "ShooterControlPointTypes.generated.h"

class AShooterPlayerState;

UENUM(BlueprintType)
enum class EShooterControlPointStatus : uint8
{
	Empty,
	Held,
	Contested
};

// Public snapshot; the server's timing execution stays in GameMode.
USTRUCT(BlueprintType)
struct SHOOTERGAME_API FShooterControlPointState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Shooter|ControlPoint")
	EShooterControlPointStatus Status = EShooterControlPointStatus::Empty;

	// Valid only for Held. Empty and Contested must leave this null.
	UPROPERTY(BlueprintReadOnly, Category = "Shooter|ControlPoint")
	TObjectPtr<AShooterPlayerState> SolePlayer = nullptr;
};
