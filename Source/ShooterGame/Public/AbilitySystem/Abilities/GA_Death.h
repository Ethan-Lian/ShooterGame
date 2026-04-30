#pragma once

#include "Abilities/GameplayAbility.h"
#include "GA_Death.generated.h"

class UGameplayEffect;

UCLASS()
class SHOOTERGAME_API UGA_Death : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Death();

	// Rejects duplicate death events once the ASC already owns the dead state.
	virtual bool ShouldAbilityRespondToEvent(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayEventData* Payload) const override;

	// Runs the server-authoritative death transition.
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	// GameplayEffect that grants State.Dead until the GameMode resets the player for respawn.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shooter|Death")
	TSubclassOf<UGameplayEffect> DeathStateEffectClass;
};
