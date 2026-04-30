#pragma once

#include "Abilities/GameplayAbility.h"
#include "GameplayEffectTypes.h"
#include "GA_Sprint.generated.h"

class UGameplayEffect;

UCLASS()
class SHOOTERGAME_API UGA_Sprint : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Sprint();

	// Starts the predicted sprint state and applies its movement-speed GameplayEffect.
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	// Clears the sprint speed effect when input, movement direction, or death cancels sprinting.
	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	// GameplayEffect that grants sprint state and raises MaxWalkSpeed while the ability is active.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shooter|Movement")
	TSubclassOf<UGameplayEffect> SprintSpeedEffectClass;

private:
	FActiveGameplayEffectHandle ActiveSprintSpeedEffectHandle;
};
