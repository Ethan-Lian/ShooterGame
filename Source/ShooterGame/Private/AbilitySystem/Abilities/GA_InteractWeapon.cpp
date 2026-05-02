#include "AbilitySystem/Abilities/GA_InteractWeapon.h"
#include "AbilitySystem/ShooterGameplayTags.h"
#include "Components/ShooterWeaponEquipmentComponent.h"
#include "Components/ShooterWeaponInteractionComponent.h"
#include "Interfaces/ShooterEquipmentInterface.h"
#include "Weapon/ShooterWeaponPickupActor.h"

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

	const IShooterEquipmentInterface* EquipmentOwner = Cast<IShooterEquipmentInterface>(GetAvatarActorFromActorInfo());
	if (EquipmentOwner == nullptr)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UShooterWeaponInteractionComponent* InteractionComponent = EquipmentOwner->GetShooterWeaponInteractionComponent();
	UShooterWeaponEquipmentComponent* EquipmentComponent = EquipmentOwner->GetShooterWeaponEquipmentComponent();
	if (InteractionComponent == nullptr || EquipmentComponent == nullptr)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AShooterWeaponPickupActor* TargetWeapon = InteractionComponent->FindPickupWeaponFromView();
	if (TargetWeapon != nullptr)
	{
		EquipmentComponent->TryPickupTargetWeapon(TargetWeapon);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
