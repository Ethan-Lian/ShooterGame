#include "AbilitySystem/Effects/GE_DeathState.h"

#include "AbilitySystem/ShooterGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_DeathState::UGE_DeathState()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	FInheritedTagContainer GrantedTags;
	GrantedTags.AddTag(TAG_State_Dead);

	UTargetTagsGameplayEffectComponent* TargetTagsComponent =
		CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTagsGameplayEffectComponent"));
	GEComponents.Add(TargetTagsComponent);
	TargetTagsComponent->SetAndApplyTargetTagChanges(GrantedTags);
}
