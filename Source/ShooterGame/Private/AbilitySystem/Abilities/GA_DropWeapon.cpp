#include "AbilitySystem/Abilities/GA_DropWeapon.h"
#include "AbilitySystem/ShooterGameplayTags.h"
#include "Components/ShooterWeaponEquipmentComponent.h"
#include "Interfaces/ShooterEquipmentInterface.h"

UGA_DropWeapon::UGA_DropWeapon()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	ActivationBlockedTags.AddTag(TAG_State_Dead);
}

void UGA_DropWeapon::ActivateAbility(
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

	UShooterWeaponEquipmentComponent* EquipmentComponent = EquipmentOwner->GetShooterWeaponEquipmentComponent();
	if (EquipmentComponent != nullptr)
	{
		EquipmentComponent->DropEquippedWeapon();
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
