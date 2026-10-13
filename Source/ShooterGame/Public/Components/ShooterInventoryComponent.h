#pragma once

#include "Components/ActorComponent.h"
#include "Weapon/WeaponDataAsset.h"
#include "ShooterInventoryComponent.generated.h"

USTRUCT(BlueprintType)
struct FWeaponInventoryEntry
{
	GENERATED_BODY()

	// Stable server-assigned id used to reference this weapon across inventory/equipment.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Shooter|Inventory")
	int32 ItemId = INDEX_NONE;

	// The static definition that describes this weapon's mesh and gameplay config.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Shooter|Inventory")
	TObjectPtr<UWeaponDataAsset> WeaponDefinition;

	// The current ammo stored in the active magazine.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Shooter|Inventory")
	int32 CurrentMagazineAmmo = 0;

	// The current ammo stored outside the active magazine.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Shooter|Inventory")
	int32 CurrentReserveAmmo = 0;

	// Returns whether this entry contains a usable logical weapon.
	bool IsValid() const;
};

UCLASS(ClassGroup = (ShooterGame), Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent))
class SHOOTERGAME_API UShooterInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UShooterInventoryComponent();

	// Replicates owner-facing logical weapon inventory entries.
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Returns the first logical weapon entry that matches the supplied item id.
	const FWeaponInventoryEntry* GetInventoryEntryByItemId(int32 ItemId) const;

	// Returns the single default weapon entry, or nullptr before it is granted.
	const FWeaponInventoryEntry* GetDefaultWeaponEntry() const;

	// Grants the single default weapon with default ammo on the server.
	bool AddWeaponFromDefinition(UWeaponDataAsset* WeaponDefinition, int32& OutItemId);

	// Removes a logical weapon entry from the inventory by item id.
	bool RemoveWeaponByItemId(int32 ItemId, FWeaponInventoryEntry& OutRemovedEntry);

	// Runtime ammo is stored and mutated only here on authority.
	bool ConsumeMagazineRound(int32 ItemId);
	bool CanReload(int32 ItemId) const;
	bool ReloadMagazine(int32 ItemId);

private:
	// Returns the array index that owns the supplied item id.
	int32 FindEntryArrayIndexByItemId(int32 ItemId) const;

	// The owner-facing logical inventory entries.
	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Shooter|Inventory", meta = (AllowPrivateAccess = "true"))
	TArray<FWeaponInventoryEntry> InventoryEntries;

	// Generates stable runtime item ids for newly granted weapons.
	int32 NextItemId = 1;
};
