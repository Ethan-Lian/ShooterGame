#include "AbilitySystem/Abilities/GA_InteractWeapon.h"
#include "AbilitySystem/ShooterGameplayTags.h"
#include "Character/PlayerCharacter.h"
#include "Components/ShooterWeaponEquipmentComponent.h"
#include "Components/ShooterWeaponInteractionComponent.h"

UGA_InteractWeapon::UGA_InteractWeapon()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	ActivationBlockedTags.AddTag(TAG_State_Dead);
}

void UGA_InteractWeapon::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	APlayerCharacter* Character = Cast<APlayerCharacter>(GetAvatarActorFromActorInfo());
	if (Character == nullptr)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UShooterWeaponInteractionComponent* InteractionComponent = Character->GetWeaponInteractionComponent();
	UShooterWeaponEquipmentComponent* EquipmentComponent = Character->GetWeaponEquipmentComponent();
	if (InteractionComponent == nullptr || EquipmentComponent == nullptr)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AShooterWeaponBase* TargetWeapon = InteractionComponent->FindPickupWeaponFromView();
	if (TargetWeapon != nullptr)
	{
		EquipmentComponent->TryPickupTargetWeapon(TargetWeapon);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
