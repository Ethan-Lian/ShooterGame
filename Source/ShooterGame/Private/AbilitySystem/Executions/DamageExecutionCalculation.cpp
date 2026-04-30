#include "AbilitySystem/Executions/DamageExecutionCalculation.h"

#include "AbilitySystem/ShooterGameplayTags.h"
#include "AbilitySystem/Attributes/CombatAttributeSet.h"
#include "GameplayEffectTypes.h"

UDamageExecutionCalculation::UDamageExecutionCalculation()
{
}

void UDamageExecutionCalculation::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& OwningSpec = ExecutionParams.GetOwningSpec();
	const float RequestedDamage = FMath::Max(0.f, OwningSpec.GetSetByCallerMagnitude(TAG_Data_Damage, false, 0.f));

	if (RequestedDamage <= 0.f)
	{
		return;
	}

	OutExecutionOutput.AddOutputModifier(
		FGameplayModifierEvaluatedData(
			UCombatAttributeSet::GetIncomingDamageAttribute(),
			EGameplayModOp::Additive,
			RequestedDamage));
}
