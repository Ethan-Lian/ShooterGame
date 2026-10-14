#pragma once

#include "Components/ActorComponent.h"
#include "ShooterWeaponEquipmentComponent.generated.h"

class ACharacter;
class AShooterWeaponEquipmentActor;
class UShooterInventoryComponent;
class UWeaponDataAsset;
struct FWeaponInventoryEntry;

UCLASS(ClassGroup = (ShooterGame), Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent))
class SHOOTERGAME_API UShooterWeaponEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UShooterWeaponEquipmentComponent();

	// Replicates the equipped weapon pointer for remote attachment state.
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Returns the weapon presentation actor currently attached to the owning character.
	UFUNCTION(BlueprintPure, Category = "Shooter|Combat")
	AShooterWeaponEquipmentActor* GetEquippedWeapon() const { return EquippedWeapon; }

	// Borrows the current Inventory entry; do not retain it across inventory mutations.
	const FWeaponInventoryEntry* GetEquippedInventoryEntry() const;

	// Returns the logical inventory item currently equipped by this pawn.
	UFUNCTION(BlueprintPure, Category = "Shooter|Combat")
	int32 GetEquippedItemId() const { return EquippedItemId; }

	// Destroys the equipped weapon and removes its inventory entry without dropping it.
	bool HandleOwnerDeath();

	// Grants/equips the default weapon on authority; clients only refresh presentation.
	void RefreshEquipmentForPawnReady();

	// Clears Pawn-scoped presentation without changing PlayerState inventory.
	void UninitializeForPawn();

private:
	// The single Hitscan weapon granted when a live Pawn becomes ready.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWeaponDataAsset> DefaultWeaponDefinition;

	// Resolves the owning character used for attachment and character movement data.
	ACharacter* GetOwningCharacter() const;

	// Resolves the PlayerState-owned inventory component that owns logical weapons.
	UShooterInventoryComponent* GetOwningInventoryComponent() const;

	// Checks owner/death state independently of match phase; readiness is checked before initialization.
	bool IsEquipmentInteractionBlocked() const;

	// Equips the logical inventory entry addressed by its stable item id.
	bool EquipInventoryItemById(int32 ItemId);

	// Spawns a fresh equipped presentation actor for the supplied logical inventory entry.
	AShooterWeaponEquipmentActor* SpawnEquippedWeaponActor(const FWeaponInventoryEntry& Entry);

	// Destroys the current equipped presentation actor when it is no longer needed.
	void DestroyEquippedWeaponActor();

	// Removes a stale local equipped presentation when replicated state has moved on.
	void ClearLocalEquippedWeaponPresentation(AShooterWeaponEquipmentActor* WeaponToClear = nullptr);

	// Reattaches the replicated weapon pointer on remote clients.
	UFUNCTION()
	void OnRep_EquippedWeapon(AShooterWeaponEquipmentActor* OldEquippedWeapon);

	// Replicated weapon pointer used by remote attachment logic and weapon presentation.
	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon, VisibleInstanceOnly, BlueprintReadOnly, Category = "Player|Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AShooterWeaponEquipmentActor> EquippedWeapon;

	// Replicated logical inventory id currently equipped by this pawn.
	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Player|Combat", meta = (AllowPrivateAccess = "true"))
	int32 EquippedItemId = INDEX_NONE;
};
