#include "AbilitySystem/Effects/GE_DamageInstant.h"
#include "AbilitySystem/Executions/DamageExecutionCalculation.h"
#include "AbilitySystem/ShooterGameplayTags.h"

UGE_DamageInstant::UGE_DamageInstant()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayEffectExecutionDefinition DamageExecution;
	DamageExecution.CalculationClass = UDamageExecutionCalculation::StaticClass();
	Executions.Add(DamageExecution);

	GameplayCues.Add(FGameplayEffectCue(TAG_GameplayCue_Damage_Hit, 1.f, 1.f));
}
