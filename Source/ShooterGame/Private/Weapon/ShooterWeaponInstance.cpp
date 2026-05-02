#include "Weapon/ShooterWeaponInstance.h"

#include "Weapon/ShooterWeaponEquipmentActor.h"

void UShooterWeaponInstance::InitializeFromInventoryEntry(const FWeaponInventoryEntry& SourceEntry, AShooterWeaponEquipmentActor* InEquippedWeaponActor)
{
	ItemId = SourceEntry.ItemId;
	SlotIndex = SourceEntry.SlotIndex;
	WeaponDefinition = SourceEntry.WeaponDefinition;
	FireConfig = FWeaponFireConfig();
	AmmoConfig = FWeaponAmmoConfig();
	CharacterAttachSocketName = NAME_None;
	WeaponMeshRelativeTransform = FTransform::Identity;
	EquipmentActorClass = SourceEntry.EquipmentActorClass;
	EquippedWeaponActor = InEquippedWeaponActor;
	CurrentMagazineAmmo = SourceEntry.CurrentMagazineAmmo;
	CurrentReserveAmmo = SourceEntry.CurrentReserveAmmo;

	if (WeaponDefinition == nullptr)
	{
		return;
	}

	FireConfig = WeaponDefinition->FireConfig;
	AmmoConfig = WeaponDefinition->AmmoConfig;
	CharacterAttachSocketName = WeaponDefinition->CharacterAttachSocketName;
	WeaponMeshRelativeTransform = WeaponDefinition->WeaponMeshRelativeTransform;
}

void UShooterWeaponInstance::SetEquippedWeaponActor(AShooterWeaponEquipmentActor* NewEquippedWeaponActor)
{
	EquippedWeaponActor = NewEquippedWeaponActor;
}

void UShooterWeaponInstance::ClearEquippedWeaponActor()
{
	EquippedWeaponActor = nullptr;
}

FTransform UShooterWeaponInstance::GetMuzzleTransform() const
{
	if (EquippedWeaponActor != nullptr)
	{
		return EquippedWeaponActor->GetMuzzleTransform();
	}

	return FireConfig.MuzzleFallbackTransform;
}
