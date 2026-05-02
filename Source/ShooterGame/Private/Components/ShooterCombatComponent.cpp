#include "Components/ShooterCombatComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/ShooterAbilitySystemComponent.h"
#include "AbilitySystem/ShooterGameplayTags.h"
#include "Components/ShooterWeaponEquipmentComponent.h"
#include "Components/ShooterWeaponInteractionComponent.h"
#include "Interfaces/ShooterCombatInterface.h"
#include "Interfaces/ShooterEquipmentInterface.h"
#include "Net/UnrealNetwork.h"

UShooterCombatComponent::UShooterCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(true);
}

void UShooterCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UShooterCombatComponent, bIsAiming);
}

bool UShooterCombatComponent::StartFireInput()
{
	if (bIsFireInputPressed)
	{
		return false;
	}

	bIsFireInputPressed = true;

	AActor* OwnerActor = GetOwner();
	if (OwnerActor != nullptr && OwnerActor->HasAuthority())
	{
		HandleFireInputPressed();
	}
	else
	{
		ServerStartFire();
	}

	return true;
}

bool UShooterCombatComponent::StopFireInput()
{
	if (!bIsFireInputPressed)
	{
		return false;
	}

	bIsFireInputPressed = false;

	AActor* OwnerActor = GetOwner();
	if (OwnerActor != nullptr && OwnerActor->HasAuthority())
	{
		HandleFireInputReleased();
	}
	else
	{
		ServerStopFire();
	}

	return true;
}

bool UShooterCombatComponent::StartAimInput()
{
	if (bIsAiming || bOwnerDeathHandled)
	{
		return false;
	}

	if (const UAbilitySystemComponent* AbilitySystemComponent = GetOwningAbilitySystemComponent())
	{
		if (AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Dead))
		{
			return false;
		}
	}

	SetAimInputPressed(true);

	AActor* OwnerActor = GetOwner();
	if (OwnerActor != nullptr && !OwnerActor->HasAuthority())
	{
		ServerStartAim();
	}

	return true;
}

bool UShooterCombatComponent::StopAimInput()
{
	if (!bIsAiming)
	{
		return false;
	}

	SetAimInputPressed(false);

	AActor* OwnerActor = GetOwner();
	if (OwnerActor != nullptr && !OwnerActor->HasAuthority())
	{
		ServerStopAim();
	}

	return true;
}

bool UShooterCombatComponent::HandleOwnerDeath()
{
	if (bOwnerDeathHandled)
	{
		return false;
	}

	bOwnerDeathHandled = true;

	const bool bFireInputStopped = bIsFireInputPressed;
	bIsFireInputPressed = false;
	SetAimInputPressed(false);

	if (UShooterWeaponEquipmentComponent* EquipmentComponent = GetOwningWeaponEquipmentComponent())
	{
		EquipmentComponent->HandleOwnerDeath();
	}

	if (UShooterWeaponInteractionComponent* InteractionComponent = GetOwningWeaponInteractionComponent())
	{
		InteractionComponent->ClearTargetedPickupWeapon();
	}

	return bFireInputStopped;
}

UAbilitySystemComponent* UShooterCombatComponent::GetOwningAbilitySystemComponent() const
{
	const IShooterCombatInterface* CombatOwner = Cast<IShooterCombatInterface>(GetOwner());
	return CombatOwner != nullptr ? CombatOwner->GetShooterAbilitySystemComponent() : nullptr;
}

UShooterWeaponEquipmentComponent* UShooterCombatComponent::GetOwningWeaponEquipmentComponent() const
{
	const IShooterEquipmentInterface* EquipmentOwner = Cast<IShooterEquipmentInterface>(GetOwner());
	return EquipmentOwner != nullptr ? EquipmentOwner->GetShooterWeaponEquipmentComponent() : nullptr;
}

UShooterWeaponInteractionComponent* UShooterCombatComponent::GetOwningWeaponInteractionComponent() const
{
	const IShooterEquipmentInterface* EquipmentOwner = Cast<IShooterEquipmentInterface>(GetOwner());
	return EquipmentOwner != nullptr ? EquipmentOwner->GetShooterWeaponInteractionComponent() : nullptr;
}

void UShooterCombatComponent::HandleFireInputPressed()
{
	UShooterAbilitySystemComponent* ShooterASC = Cast<UShooterAbilitySystemComponent>(GetOwningAbilitySystemComponent());
	if (ShooterASC == nullptr || ShooterASC->HasMatchingGameplayTag(TAG_State_Dead))
	{
		return;
	}

	ShooterASC->AbilityInputTagPressed(TAG_Input_Fire);
}

void UShooterCombatComponent::HandleFireInputReleased()
{
	UShooterAbilitySystemComponent* ShooterASC = Cast<UShooterAbilitySystemComponent>(GetOwningAbilitySystemComponent());
	if (ShooterASC == nullptr)
	{
		return;
	}

	ShooterASC->AbilityInputTagReleased(TAG_Input_Fire);
}

void UShooterCombatComponent::SetAimInputPressed(bool bNewIsAiming)
{
	if (bIsAiming == bNewIsAiming)
	{
		return;
	}

	bIsAiming = bNewIsAiming;

	if (IShooterCombatInterface* CombatOwner = Cast<IShooterCombatInterface>(GetOwner()))
	{
		CombatOwner->HandleShooterAimStateChanged(bIsAiming);
	}
}

bool UShooterCombatComponent::IsWeaponInteractionBlocked() const
{
	if (bOwnerDeathHandled)
	{
		return true;
	}

	const UAbilitySystemComponent* AbilitySystemComponent = GetOwningAbilitySystemComponent();
	return AbilitySystemComponent != nullptr && AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Dead);
}

void UShooterCombatComponent::OnRep_IsAiming()
{
	if (IShooterCombatInterface* CombatOwner = Cast<IShooterCombatInterface>(GetOwner()))
	{
		CombatOwner->HandleShooterAimStateChanged(bIsAiming);
	}
}

void UShooterCombatComponent::ServerStartFire_Implementation()
{
	HandleFireInputPressed();
}

void UShooterCombatComponent::ServerStopFire_Implementation()
{
	HandleFireInputReleased();
}

void UShooterCombatComponent::ServerStartAim_Implementation()
{
	SetAimInputPressed(true);
}

void UShooterCombatComponent::ServerStopAim_Implementation()
{
	SetAimInputPressed(false);
}
