#include "AbilitySystem/Effects/GE_SprintSpeed.h"

#include "AbilitySystem/Attributes/MovementAttributeSet.h"
#include "AbilitySystem/ShooterGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_SprintSpeed::UGE_SprintSpeed()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	FGameplayModifierInfo SprintSpeedModifier;
	SprintSpeedModifier.Attribute = UMovementAttributeSet::GetMaxWalkSpeedAttribute();
	SprintSpeedModifier.ModifierOp = EGameplayModOp::Additive;
	SprintSpeedModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(250.f));
	Modifiers.Add(SprintSpeedModifier);

	FInheritedTagContainer GrantedTags;
	GrantedTags.AddTag(TAG_State_Movement_Sprinting);

	UTargetTagsGameplayEffectComponent* TargetTagsComponent =
		CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTagsGameplayEffectComponent"));
	GEComponents.Add(TargetTagsComponent);
	TargetTagsComponent->SetAndApplyTargetTagChanges(GrantedTags);
}
