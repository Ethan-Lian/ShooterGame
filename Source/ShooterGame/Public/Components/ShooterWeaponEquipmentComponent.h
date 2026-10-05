#pragma once

#include "Components/ActorComponent.h"
#include "ShooterWeaponEquipmentComponent.generated.h"

class ACharacter;
class AShooterWeaponEquipmentActor;
class UShooterCombatComponent;
class UShooterInventoryComponent;
class UShooterWeaponInstance;
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

	// Returns the logical weapon instance currently resolved from the equipped inventory entry.
	UFUNCTION(BlueprintPure, Category = "Shooter|Combat")
	UShooterWeaponInstance* GetEquippedWeaponInstance() const { return EquippedWeaponInstance; }

	// Returns the logical inventory item currently equipped by this pawn.
	UFUNCTION(BlueprintPure, Category = "Shooter|Combat")
	int32 GetEquippedItemId() const { return EquippedItemId; }

	// Destroys the equipped weapon and removes its inventory entry without dropping it.
	bool HandleOwnerDeath();

	// Rebuilds owner-side cached weapon state after replicated inventory changes arrive.
	void HandleInventoryReplicated();

	// Grants/equips the default weapon on authority; clients only refresh presentation.
	void RefreshEquipmentForPawnReady();

	// Clears Pawn-scoped presentation and transient instance state without changing PlayerState inventory.
	void UninitializeForPawn();

private:
	// The single Hitscan weapon granted when a live Pawn becomes ready.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWeaponDataAsset> DefaultWeaponDefinition;

	// Resolves the owning character used for attachment and character movement data.
	ACharacter* GetOwningCharacter() const;

	// Resolves the combat component that owns combat-state blocking rules.
	UShooterCombatComponent* GetOwningCombatComponent() const;

	// Resolves the PlayerState-owned inventory component that owns logical weapons.
	UShooterInventoryComponent* GetOwningInventoryComponent() const;

	// Returns whether equipment interactions should currently be blocked.
	bool IsEquipmentInteractionBlocked() const;

	// Equips the logical inventory entry addressed by its stable item id.
	bool EquipInventoryItemById(int32 ItemId);

	// Spawns a fresh equipped presentation actor for the supplied logical inventory entry.
	AShooterWeaponEquipmentActor* SpawnEquippedWeaponActor(const FWeaponInventoryEntry& Entry);

	// Destroys the current equipped presentation actor when it is no longer needed.
	void DestroyEquippedWeaponActor();

	// Removes a stale local equipped presentation when replicated state has moved on.
	void ClearLocalEquippedWeaponPresentation(AShooterWeaponEquipmentActor* WeaponToClear = nullptr);

	// Rebuilds the logical weapon instance from the current equipped inventory item.
	void RefreshEquippedWeaponInstance();

	// Clears the logical weapon instance when nothing is equipped.
	void ClearEquippedWeaponInstance();

	// Reattaches the replicated weapon pointer on remote clients.
	UFUNCTION()
	void OnRep_EquippedWeapon(AShooterWeaponEquipmentActor* OldEquippedWeapon);

	// Rebuilds the owner-side logical weapon view when the equipped item id changes.
	UFUNCTION()
	void OnRep_EquippedItemId();

	// Replicated weapon pointer used by remote attachment logic and weapon presentation.
	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon, VisibleInstanceOnly, BlueprintReadOnly, Category = "Player|Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AShooterWeaponEquipmentActor> EquippedWeapon;

	// Replicated logical inventory id currently equipped by this pawn.
	UPROPERTY(ReplicatedUsing = OnRep_EquippedItemId, VisibleInstanceOnly, BlueprintReadOnly, Category = "Player|Combat", meta = (AllowPrivateAccess = "true"))
	int32 EquippedItemId = INDEX_NONE;

	// Transient logical weapon instance rebuilt from inventory truth for local gameplay queries.
	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category = "Player|Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UShooterWeaponInstance> EquippedWeaponInstance;

};
