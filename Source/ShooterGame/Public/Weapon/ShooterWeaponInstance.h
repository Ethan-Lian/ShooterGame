#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Components/ShooterInventoryComponent.h"
#include "ShooterWeaponInstance.generated.h"

class AShooterWeaponEquipmentActor;

UCLASS(BlueprintType)
class SHOOTERGAME_API UShooterWeaponInstance : public UObject
{
	GENERATED_BODY()

public:
	// Initializes the instance from the current logical inventory entry plus presentation actor.
	void InitializeFromInventoryEntry(const FWeaponInventoryEntry& SourceEntry, AShooterWeaponEquipmentActor* InEquippedWeaponActor);

	// Returns the source definition asset when the weapon originated from data.
	const UWeaponDataAsset* GetWeaponDefinition() const { return WeaponDefinition; }

	// Returns the copied fire config used by abilities while the weapon is equipped.
	const FWeaponFireConfig& GetFireConfig() const { return FireConfig; }

	// Returns the copied ammo configuration; Inventory owns the runtime counts.
	const FWeaponAmmoConfig& GetAmmoConfig() const { return AmmoConfig; }

	// Returns the stable logical inventory id that owns this runtime instance.
	int32 GetItemId() const { return ItemId; }

	// Returns the currently equipped presentation actor backing this instance.
	AShooterWeaponEquipmentActor* GetEquippedWeaponActor() const { return EquippedWeaponActor.Get(); }

	// Clears the current equipped actor reference when the weapon leaves the avatar.
	void ClearEquippedWeaponActor();

	// Returns the best-known muzzle transform for Hitscan traces and fire feedback.
	FTransform GetMuzzleTransform() const;

	// Returns the copied attachment socket name used by equipped presentation.
	FName GetAttachSocketName() const { return CharacterAttachSocketName; }

	// Returns the copied equipped relative transform used by equipped presentation.
	FTransform GetEquippedRelativeTransform() const { return WeaponMeshRelativeTransform; }

	// Returns the magazine count refreshed from the owning Inventory entry.
	int32 GetCurrentMagazineAmmo() const { return CurrentMagazineAmmo; }

	// Returns the reserve count refreshed from the owning Inventory entry.
	int32 GetCurrentReserveAmmo() const { return CurrentReserveAmmo; }

private:
	// Stable logical inventory id copied from the owning inventory entry.
	UPROPERTY(Transient)
	int32 ItemId = INDEX_NONE;

	// Optional source definition asset when the weapon came from a data asset.
	UPROPERTY(Transient)
	TObjectPtr<UWeaponDataAsset> WeaponDefinition;

	// The actor class used when this logical weapon needs a presentation actor.
	UPROPERTY(Transient)
	TSubclassOf<AShooterWeaponEquipmentActor> EquipmentActorClass;

	// Static fire config snapshot copied at equip time for definition-driven logic.
	UPROPERTY(Transient)
	FWeaponFireConfig FireConfig;

	// Static ammo configuration copied from the Inventory entry's definition.
	UPROPERTY(Transient)
	FWeaponAmmoConfig AmmoConfig;

	// Attachment socket snapshot copied at equip time.
	UPROPERTY(Transient)
	FName CharacterAttachSocketName = NAME_None;

	// Equipped transform snapshot copied at equip time.
	UPROPERTY(Transient)
	FTransform WeaponMeshRelativeTransform = FTransform::Identity;

	// The actor currently presenting this weapon while equipped on the avatar.
	UPROPERTY(Transient)
	TObjectPtr<AShooterWeaponEquipmentActor> EquippedWeaponActor;

	// Magazine count mirrored from Inventory; do not mutate this snapshot.
	UPROPERTY(Transient)
	int32 CurrentMagazineAmmo = 0;

	// Reserve count mirrored from Inventory; do not mutate this snapshot.
	UPROPERTY(Transient)
	int32 CurrentReserveAmmo = 0;
};
