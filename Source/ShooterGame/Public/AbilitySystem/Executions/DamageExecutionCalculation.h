#pragma once

#include "GameplayEffectExecutionCalculation.h"
#include "DamageExecutionCalculation.generated.h"

UCLASS()
class SHOOTERGAME_API UDamageExecutionCalculation : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UDamageExecutionCalculation();

	// Converts the incoming damage payload into the target meta attribute.
	virtual void Execute_Implementation(
		const FGameplayEffectCustomExecutionParameters& ExecutionParams,
		FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
