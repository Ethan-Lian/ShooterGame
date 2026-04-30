#include "Weapon/ShooterWeaponInstance.h"

#include "Weapon/ShooterWeaponBase.h"

void UShooterWeaponInstance::InitializeFromInventoryEntry(const FWeaponInventoryEntry& SourceEntry, AShooterWeaponBase* InEquippedWeaponActor)
{
	ItemId = SourceEntry.ItemId;
	SlotIndex = SourceEntry.SlotIndex;
	WeaponDefinition = SourceEntry.WeaponDefinition;
	FireConfig = FWeaponFireConfig();
	AmmoConfig = FWeaponAmmoConfig();
	CharacterAttachSocketName = NAME_None;
	WeaponMeshRelativeTransform = FTransform::Identity;
	WeaponActorClass = SourceEntry.WeaponActorClass;
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

void UShooterWeaponInstance::SetEquippedWeaponActor(AShooterWeaponBase* NewEquippedWeaponActor)
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
