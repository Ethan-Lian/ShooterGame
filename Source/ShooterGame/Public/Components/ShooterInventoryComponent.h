#pragma once

#include "Components/ActorComponent.h"
#include "Weapon/WeaponDataAsset.h"
#include "ShooterInventoryComponent.generated.h"

class AShooterWeaponEquipmentActor;
class AShooterWeaponPickupActor;

USTRUCT(BlueprintType)
struct FWeaponInventoryEntry
{
	GENERATED_BODY()

	// Stable server-assigned id used to reference this weapon across inventory/equipment.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Shooter|Inventory")
	int32 ItemId = INDEX_NONE;

	// The logical quick-slot occupied by this weapon.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Shooter|Inventory")
	int32 SlotIndex = INDEX_NONE;

	// The static definition that describes this weapon's mesh and gameplay config.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Shooter|Inventory")
	TObjectPtr<UWeaponDataAsset> WeaponDefinition;

	// The actor class used when this logical weapon becomes a world pickup.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Shooter|Inventory")
	TSubclassOf<AShooterWeaponPickupActor> PickupActorClass;

	// The actor class used when this logical weapon becomes equipped presentation.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Shooter|Inventory")
	TSubclassOf<AShooterWeaponEquipmentActor> EquipmentActorClass;

	// The current ammo stored in the active magazine.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Shooter|Inventory")
	int32 CurrentMagazineAmmo = 0;

	// The current ammo stored outside the active magazine.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Shooter|Inventory")
	int32 CurrentReserveAmmo = 0;

	// Returns whether this entry contains a usable logical weapon.
	bool IsValid() const;

	// Builds the world/equipment pickup snapshot consumed by presentation actors.
	FWeaponPickupData ToPickupData() const;
};

UCLASS(ClassGroup = (ShooterGame), Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent))
class SHOOTERGAME_API UShooterInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UShooterInventoryComponent();

	// Replicates owner-facing logical weapon inventory entries.
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Returns the current logical inventory snapshot.
	const TArray<FWeaponInventoryEntry>& GetInventoryEntries() const { return InventoryEntries; }

	// Returns the current configured max weapon slots for this player.
	UFUNCTION(BlueprintPure, Category = "Shooter|Inventory")
	int32 GetMaxWeaponSlots() const { return MaxWeaponSlots; }

	// Returns the first logical weapon entry that matches the supplied item id.
	const FWeaponInventoryEntry* GetInventoryEntryByItemId(int32 ItemId) const;

	// Returns the first logical weapon entry that occupies the supplied slot index.
	const FWeaponInventoryEntry* GetInventoryEntryBySlotIndex(int32 SlotIndex) const;

	// Returns the first occupied inventory slot, or INDEX_NONE when empty.
	int32 FindFirstOccupiedSlotIndex() const;

	// Returns the next occupied slot while wrapping around the available range.
	int32 FindNextOccupiedSlotIndex(int32 CurrentSlotIndex, bool bForward) const;

	// Adds a world pickup actor into the owner's logical inventory.
	bool AddWeaponFromPickup(AShooterWeaponPickupActor* PickupWeapon, int32& OutItemId, int32& OutSlotIndex);

	// Grants a weapon directly from its definition with default ammo on the server.
	bool AddWeaponFromDefinition(UWeaponDataAsset* WeaponDefinition, int32& OutItemId);

	// Removes a logical weapon entry from the inventory by item id.
	bool RemoveWeaponByItemId(int32 ItemId, FWeaponInventoryEntry& OutRemovedEntry);

private:
	// Returns the mutable entry pointer for a given item id on the authority path.
	FWeaponInventoryEntry* GetMutableInventoryEntryByItemId(int32 ItemId);

	// Returns the array index that owns the supplied item id.
	int32 FindEntryArrayIndexByItemId(int32 ItemId) const;

	// Returns the array index that owns the supplied slot index.
	int32 FindEntryArrayIndexBySlotIndex(int32 SlotIndex) const;

	// Finds the first free logical slot that can accept a newly picked weapon.
	int32 FindFirstFreeSlotIndex() const;

	// Builds a logical inventory entry from a replicated world pickup actor.
	bool BuildInventoryEntryFromPickup(AShooterWeaponPickupActor* PickupWeapon, int32 SlotIndex, FWeaponInventoryEntry& OutEntry) const;

	// Refreshes owner-side cached weapon views after the replicated inventory changes.
	UFUNCTION()
	void OnRep_InventoryEntries();

	// The owner-facing logical inventory entries.
	UPROPERTY(ReplicatedUsing = OnRep_InventoryEntries, VisibleInstanceOnly, BlueprintReadOnly, Category = "Shooter|Inventory", meta = (AllowPrivateAccess = "true"))
	TArray<FWeaponInventoryEntry> InventoryEntries;

	// Limits how many weapon slots can be occupied at one time.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shooter|Inventory", meta = (AllowPrivateAccess = "true", ClampMin = "1"))
	int32 MaxWeaponSlots = 2;

	// Generates stable runtime item ids for new pickups.
	int32 NextItemId = 1;
};
