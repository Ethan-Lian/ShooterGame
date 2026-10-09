#include "AbilitySystem/Abilities/GA_Sprint.h"

#include "AbilitySystem/ShooterGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Effects/GE_SprintSpeed.h"
#include "Components/ShooterCombatComponent.h"
#include "Components/ShooterMovementStateComponent.h"
#include "GameFramework/Character.h"
#include "Interfaces/ShooterCombatInterface.h"

UGA_Sprint::UGA_Sprint()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	FGameplayTagContainer AbilityTagContainer;
	AbilityTagContainer.AddTag(TAG_Ability_Movement_Sprint);
	SetAssetTags(AbilityTagContainer);

	ActivationBlockedTags.AddTag(TAG_State_Dead);

	SprintSpeedEffectClass = UGE_SprintSpeed::StaticClass();
}

void UGA_Sprint::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UObject* AvatarObject = GetAvatarActorFromActorInfo();
	IShooterCombatInterface* CombatOwner = Cast<IShooterCombatInterface>(AvatarObject);

	UShooterMovementStateComponent* MovementStateComponent = CombatOwner != nullptr
		? CombatOwner->GetShooterMovementStateComponent()
		: nullptr;
	
	if (CombatOwner == nullptr
		|| MovementStateComponent == nullptr
		|| !MovementStateComponent->IsSprintDirectionAllowed()
		|| SprintSpeedEffectClass == nullptr
		|| !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (UShooterCombatComponent* CombatComponent = CombatOwner->GetShooterCombatComponent())
	{
		CombatComponent->StopAimInput();
	}

	if (ACharacter* AvatarCharacter = Cast<ACharacter>(AvatarObject))
	{
		AvatarCharacter->UnCrouch();
	}

	const FGameplayEffectSpecHandle SprintSpeedSpecHandle = MakeOutgoingGameplayEffectSpec(
		SprintSpeedEffectClass,
		GetAbilityLevel(Handle, ActorInfo));
	if (!SprintSpeedSpecHandle.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ActiveSprintSpeedEffectHandle = ApplyGameplayEffectSpecToOwner(
		Handle,
		ActorInfo,
		ActivationInfo,
		SprintSpeedSpecHandle);
	if (!ActiveSprintSpeedEffectHandle.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}

void UGA_Sprint::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo())
	{
		if (ActiveSprintSpeedEffectHandle.IsValid())
		{
			AbilitySystemComponent->RemoveActiveGameplayEffect(ActiveSprintSpeedEffectHandle);
			ActiveSprintSpeedEffectHandle.Invalidate();
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
