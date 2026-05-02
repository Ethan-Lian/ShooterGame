#pragma once

#include "AbilitySystemComponent.h"
#include "ShooterAbilitySystemComponent.generated.h"

UCLASS()
class SHOOTERGAME_API UShooterAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	void AbilityInputTagPressed(FGameplayTag InputTag);
	void AbilityInputTagReleased(FGameplayTag InputTag);
};
