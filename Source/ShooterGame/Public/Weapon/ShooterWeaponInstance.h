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

	// Returns the copied ammo config used to seed future runtime ammo rules.
	const FWeaponAmmoConfig& GetAmmoConfig() const { return AmmoConfig; }

	// Returns the stable logical inventory id that owns this runtime instance.
	int32 GetItemId() const { return ItemId; }

	// Returns the logical slot index that currently owns this runtime instance.
	int32 GetSlotIndex() const { return SlotIndex; }

	// Returns the currently equipped presentation actor backing this instance.
	AShooterWeaponEquipmentActor* GetEquippedWeaponActor() const { return EquippedWeaponActor.Get(); }

	// Updates which world/equipped actor currently presents this logical weapon instance.
	void SetEquippedWeaponActor(AShooterWeaponEquipmentActor* NewEquippedWeaponActor);

	// Clears the current equipped actor reference when the weapon leaves the avatar.
	void ClearEquippedWeaponActor();

	// Returns the best-known muzzle transform for spawning projectiles.
	FTransform GetMuzzleTransform() const;

	// Returns the copied attachment socket name used by equipped presentation.
	FName GetAttachSocketName() const { return CharacterAttachSocketName; }

	// Returns the copied equipped relative transform used by equipped presentation.
	FTransform GetEquippedRelativeTransform() const { return WeaponMeshRelativeTransform; }

	// Returns the seeded magazine ammo that future reload logic will mutate.
	int32 GetCurrentMagazineAmmo() const { return CurrentMagazineAmmo; }

	// Returns the seeded reserve ammo that future reload logic will mutate.
	int32 GetCurrentReserveAmmo() const { return CurrentReserveAmmo; }

private:
	// Stable logical inventory id copied from the owning inventory entry.
	UPROPERTY(Transient)
	int32 ItemId = INDEX_NONE;

	// Logical slot index copied from the owning inventory entry.
	UPROPERTY(Transient)
	int32 SlotIndex = INDEX_NONE;

	// Optional source definition asset when the weapon came from a data asset.
	UPROPERTY(Transient)
	TObjectPtr<UWeaponDataAsset> WeaponDefinition;

	// The actor class used when this logical weapon needs a presentation actor.
	UPROPERTY(Transient)
	TSubclassOf<AShooterWeaponEquipmentActor> EquipmentActorClass;

	// Static fire config snapshot copied at equip time for definition-driven logic.
	UPROPERTY(Transient)
	FWeaponFireConfig FireConfig;

	// Static ammo config snapshot copied at equip time for future runtime ammo logic.
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

	// Current magazine ammo seeded from the definition for later gameplay expansion.
	UPROPERTY(Transient)
	int32 CurrentMagazineAmmo = 0;

	// Current reserve ammo seeded from the definition for later gameplay expansion.
	UPROPERTY(Transient)
	int32 CurrentReserveAmmo = 0;
};
