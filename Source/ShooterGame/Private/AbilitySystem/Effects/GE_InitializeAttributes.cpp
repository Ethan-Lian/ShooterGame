#include "AbilitySystem/Effects/GE_InitializeAttributes.h"

#include "AbilitySystem/Attributes/CombatAttributeSet.h"
#include "AbilitySystem/Attributes/MovementAttributeSet.h"

UGE_InitializeAttributes::UGE_InitializeAttributes()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo MaxHealthModifier;
	MaxHealthModifier.Attribute = UCombatAttributeSet::GetMaxHealthAttribute();
	MaxHealthModifier.ModifierOp = EGameplayModOp::Override;
	MaxHealthModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(100.f));
	Modifiers.Add(MaxHealthModifier);

	FGameplayModifierInfo HealthModifier;
	HealthModifier.Attribute = UCombatAttributeSet::GetHealthAttribute();
	HealthModifier.ModifierOp = EGameplayModOp::Override;
	HealthModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(100.f));
	Modifiers.Add(HealthModifier);

	FGameplayModifierInfo MaxWalkSpeedModifier;
	MaxWalkSpeedModifier.Attribute = UMovementAttributeSet::GetMaxWalkSpeedAttribute();
	MaxWalkSpeedModifier.ModifierOp = EGameplayModOp::Override;
	MaxWalkSpeedModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(600.f));
	Modifiers.Add(MaxWalkSpeedModifier);
}
