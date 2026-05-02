#pragma once

#include "Abilities/GameplayAbility.h"
#include "GA_InteractWeapon.generated.h"

UCLASS()
class SHOOTERGAME_API UGA_InteractWeapon : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_InteractWeapon();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
