#pragma once

#include "CoreMinimal.h"
#include "ShooterMatchResultTypes.generated.h"

// Value snapshots remain readable after a PlayerState leaves the world.
USTRUCT(BlueprintType)
struct SHOOTERGAME_API FShooterMatchResultRow
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 PlayerId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly)
	FString PlayerName;

	UPROPERTY(BlueprintReadOnly)
	int32 Score = 0;

	UPROPERTY(BlueprintReadOnly)
	bool bIsWinner = false;

	bool operator==(const FShooterMatchResultRow& Other) const
	{
		return PlayerId == Other.PlayerId && PlayerName == Other.PlayerName
			&& Score == Other.Score && bIsWinner == Other.bIsWinner;
	}
};

USTRUCT(BlueprintType)
struct SHOOTERGAME_API FShooterMatchResults
{
	GENERATED_BODY()

	// An empty finalized result differs from a snapshot that has not arrived yet.
	UPROPERTY(BlueprintReadOnly)
	bool bIsFinalized = false;

	UPROPERTY(BlueprintReadOnly)
	TArray<FShooterMatchResultRow> PlayersResultRows;
};
