#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "Subsystems/WorldSubsystem.h"
#include "ShooterGameplayMessageSubsystem.generated.h"

class AController;
class APawn;

USTRUCT(BlueprintType)
struct SHOOTERGAME_API FShooterPlayerDeathMessage
{
	GENERATED_BODY()

	// Message channel used by listeners that consume death events.
	UPROPERTY(BlueprintReadOnly, Category = "Shooter|Messages")
	FGameplayTag Channel;

	// Controller that owned the pawn at the moment death was handled.
	UPROPERTY(BlueprintReadOnly, Category = "Shooter|Messages")
	TObjectPtr<AController> Controller = nullptr;

	// Pawn that entered the dead state.
	UPROPERTY(BlueprintReadOnly, Category = "Shooter|Messages")
	TObjectPtr<APawn> DeadPawn = nullptr;

	// GAS context that produced the death state, when available.
	UPROPERTY(BlueprintReadOnly, Category = "Shooter|Messages")
	FGameplayEffectContextHandle DeathContext;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FShooterPlayerDeathMessageDelegate, const FShooterPlayerDeathMessage&);

UCLASS()
class SHOOTERGAME_API UShooterGameplayMessageSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// Resolves the message subsystem for a gameplay object that has world context.
	static UShooterGameplayMessageSubsystem* Get(const UObject* WorldContextObject);

	// Registers a native listener for Player.Death messages.
	FDelegateHandle RegisterPlayerDeathListener(const FShooterPlayerDeathMessageDelegate::FDelegate& Listener);

	// Removes a previously registered Player.Death listener.
	void UnregisterPlayerDeathListener(FDelegateHandle ListenerHandle);

	// Broadcasts that a pawn has entered its authoritative dead state.
	void BroadcastPlayerDeath(const FShooterPlayerDeathMessage& Message);

private:
	FShooterPlayerDeathMessageDelegate PlayerDeathMessageDelegate;
};
