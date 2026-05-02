#include "AbilitySystem/Abilities/GA_DropWeapon.h"
#include "AbilitySystem/ShooterGameplayTags.h"
#include "Character/PlayerCharacter.h"
#include "Components/ShooterWeaponEquipmentComponent.h"

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

	APlayerCharacter* Character = Cast<APlayerCharacter>(GetAvatarActorFromActorInfo());
	if (Character == nullptr)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UShooterWeaponEquipmentComponent* EquipmentComponent = Character->GetWeaponEquipmentComponent();
	if (EquipmentComponent != nullptr)
	{
		EquipmentComponent->DropEquippedWeapon();
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
