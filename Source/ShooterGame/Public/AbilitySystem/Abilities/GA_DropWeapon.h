#pragma once

#include "Abilities/GameplayAbility.h"
#include "GA_DropWeapon.generated.h"

UCLASS()
class SHOOTERGAME_API UGA_DropWeapon : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_DropWeapon();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
