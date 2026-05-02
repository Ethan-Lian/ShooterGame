#include "AbilitySystem/ShooterAbilitySystemComponent.h"

void UShooterAbilitySystemComponent::AbilityInputTagPressed(FGameplayTag InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	ABILITYLIST_SCOPE_LOCK();
	for (FGameplayAbilitySpec& Spec : ActivatableAbilities.Items)
	{
		if (!Spec.GetDynamicSpecSourceTags().HasTag(InputTag) || Spec.Ability == nullptr)
		{
			continue;
		}

		Spec.InputPressed = true;
		if (Spec.IsActive())
		{
			AbilitySpecInputPressed(Spec);
			continue;
		}

		TryActivateAbility(Spec.Handle);
	}
}

void UShooterAbilitySystemComponent::AbilityInputTagReleased(FGameplayTag InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	ABILITYLIST_SCOPE_LOCK();
	for (FGameplayAbilitySpec& Spec : ActivatableAbilities.Items)
	{
		if (!Spec.GetDynamicSpecSourceTags().HasTag(InputTag))
		{
			continue;
		}

		Spec.InputPressed = false;
		if (Spec.IsActive())
		{
			AbilitySpecInputReleased(Spec);
			CancelAbilityHandle(Spec.Handle);
		}
	}
}
