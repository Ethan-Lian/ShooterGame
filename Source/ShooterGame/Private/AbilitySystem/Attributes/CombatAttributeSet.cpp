#include "AbilitySystem/Attributes/CombatAttributeSet.h"

#include "AbilitySystem/ShooterGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GameFramework/Actor.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

UCombatAttributeSet::UCombatAttributeSet()
{
}

void UCombatAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetHealthAttribute())
	{
		NewValue = ClampHealthValue(NewValue);
		return;
	}

	if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(1.f, NewValue);
	}
}

void UCombatAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		const float DamageToApply = FMath::Max(0.f, GetIncomingDamage());
		SetIncomingDamage(0.f);

		if (DamageToApply > 0.f)
		{
			const float OldHealth = GetHealth();
			const float NewHealth = ClampHealthValue(OldHealth - DamageToApply);
			SetHealth(NewHealth);

			if (OldHealth > 0.f && NewHealth <= 0.f)
			{
				AActor* TargetAvatar = Data.Target.GetAvatarActor_Direct();
				if (TargetAvatar != nullptr && TargetAvatar->HasAuthority() && !Data.Target.HasMatchingGameplayTag(TAG_State_Dead))
				{
					const FGameplayEffectContextHandle& EffectContext = Data.EffectSpec.GetContext();

					FGameplayEventData DeathEventData;
					DeathEventData.EventTag = TAG_GameplayEvent_Death;
					DeathEventData.Instigator = EffectContext.GetOriginalInstigator();
					DeathEventData.Target = TargetAvatar;
					DeathEventData.OptionalObject = EffectContext.GetSourceObject();
					DeathEventData.OptionalObject2 = EffectContext.GetEffectCauser();
					DeathEventData.ContextHandle = EffectContext;
					DeathEventData.EventMagnitude = DamageToApply;

					Data.Target.HandleGameplayEvent(TAG_GameplayEvent_Death, &DeathEventData);
				}
			}
		}
		return;
	}

	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(ClampHealthValue(GetHealth()));
		return;
	}

	if (Data.EvaluatedData.Attribute == GetMaxHealthAttribute())
	{
		SetMaxHealth(FMath::Max(1.f, GetMaxHealth()));
		SetHealth(ClampHealthValue(GetHealth()));
	}
}

void UCombatAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UCombatAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCombatAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
}

void UCombatAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCombatAttributeSet, Health, OldHealth);
}

void UCombatAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCombatAttributeSet, MaxHealth, OldMaxHealth);
}

float UCombatAttributeSet::ClampHealthValue(float InHealth) const
{
	return FMath::Clamp(InHealth, 0.f, FMath::Max(1.f, GetMaxHealth()));
}
