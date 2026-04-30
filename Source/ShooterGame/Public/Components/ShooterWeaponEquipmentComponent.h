#pragma once

#include "Components/ActorComponent.h"
#include "ShooterWeaponEquipmentComponent.generated.h"

class APlayerCharacter;
class AShooterWeaponBase;
class UShooterCombatComponent;
class UShooterInventoryComponent;
class UShooterWeaponInteractionComponent;
class UShooterWeaponInstance;
struct FWeaponInventoryEntry;

enum class EShooterWeaponDropMode : uint8
{
	ManualThrow,
	DeathInPlace
};

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
	AShooterWeaponBase* GetEquippedWeapon() const { return EquippedWeapon; }

	// Returns the logical weapon instance currently resolved from the equipped inventory entry.
	UFUNCTION(BlueprintPure, Category = "Shooter|Combat")
	UShooterWeaponInstance* GetEquippedWeaponInstance() const { return EquippedWeaponInstance; }

	// Returns the logical inventory item currently equipped by this pawn.
	UFUNCTION(BlueprintPure, Category = "Shooter|Combat")
	int32 GetEquippedItemId() const { return EquippedItemId; }

	// Requests that the authority path equips the current valid world weapon.
	bool StartPickupInput();

	// Requests that the authority path drops the currently equipped weapon.
	bool StartDropInput();

	// Requests that the authority path equips the weapon occupying the supplied slot.
	bool EquipInventorySlot(int32 SlotIndex);

	// Cycles to the next occupied logical weapon slot.
	bool EquipNextInventorySlot();

	// Cycles to the previous occupied logical weapon slot.
	bool EquipPreviousInventorySlot();

	// Flushes equipment state during death handling.
	bool HandleOwnerDeath();

	// Rebuilds owner-side cached weapon state after replicated inventory changes arrive.
	void HandleInventoryReplicated();

private:
	// Resolves the typed owning character helper.
	APlayerCharacter* GetOwningPlayerCharacter() const;

	// Resolves the combat component that owns combat-state blocking rules.
	UShooterCombatComponent* GetOwningCombatComponent() const;

	// Resolves the PlayerState-owned inventory component that owns logical weapons.
	UShooterInventoryComponent* GetOwningInventoryComponent() const;

	// Resolves the interaction component that owns pickup targeting and view traces.
	UShooterWeaponInteractionComponent* GetOwningWeaponInteractionComponent() const;

	// Returns whether equipment interactions should currently be blocked.
	bool IsEquipmentInteractionBlocked() const;

	// Tries to add and equip the world weapon currently validated under the crosshair.
	bool TryPickupTargetWeapon(AShooterWeaponBase* TargetWeapon);

	// Equips the logical inventory entry addressed by its stable item id.
	bool EquipInventoryItemById(int32 ItemId, AShooterWeaponBase* ExistingPresentationActor = nullptr);

	// Drops the equipped weapon into the world on the authority path.
	bool DropEquippedWeapon();

	// Spawns a fresh equipped presentation actor for the supplied logical inventory entry.
	AShooterWeaponBase* SpawnEquippedWeaponActor(const FWeaponInventoryEntry& Entry);

	// Spawns a fresh world pickup actor from the supplied logical inventory entry.
	bool SpawnWorldPickupFromEntry(const FWeaponInventoryEntry& Entry, const FTransform& DropTransform, EShooterWeaponDropMode DropMode);

	// Projects a server-authored drop transform onto walkable world geometry.
	FTransform ResolveGroundedDropTransform(const FTransform& DropTransform) const;

	// Predicts the server-authoritative landing transform by sweeping a ballistic arc.
	FTransform ResolveBallisticDropTransform(const FTransform& StartTransform) const;

	// Clamps the requested drop point in front of blocking world geometry.
	FTransform ResolveReachableDropTransform(const FTransform& DropTransform) const;

	// Destroys the current equipped presentation actor when it is no longer needed.
	void DestroyEquippedWeaponActor();

	// Removes a stale local equipped presentation when replicated state has moved on.
	void ClearLocalEquippedWeaponPresentation(AShooterWeaponBase* WeaponToClear = nullptr);

	// Rebuilds the logical weapon instance from the current equipped inventory item.
	void RefreshEquippedWeaponInstance();

	// Clears the logical weapon instance when nothing is equipped.
	void ClearEquippedWeaponInstance();

	// Builds the server-authoritative drop transform for world weapon placement.
	FTransform GetWeaponDropTransform() const;

	// Builds the server-authoritative in-place drop transform used during death.
	FTransform GetWeaponDeathDropTransform() const;

	// Validates that a weapon is still a legitimate world pickup within interaction range.
	bool IsValidWorldPickupForPickup(const AShooterWeaponBase* Weapon) const;

	// Reattaches the replicated weapon pointer on remote clients.
	UFUNCTION()
	void OnRep_EquippedWeapon(AShooterWeaponBase* OldEquippedWeapon);

	// Rebuilds the owner-side logical weapon view when the equipped item id changes.
	UFUNCTION()
	void OnRep_EquippedItemId();

	// Sends the crosshair-targeted pickup request to the authority path.
	UFUNCTION(Server, Reliable)
	void ServerTryPickupTargetWeapon(AShooterWeaponBase* TargetWeapon);

	// Sends the drop-weapon request to the authority path.
	UFUNCTION(Server, Reliable)
	void ServerDropEquippedWeapon();

	// Sends the equip-slot request to the authority path.
	UFUNCTION(Server, Reliable)
	void ServerEquipInventorySlot(int32 SlotIndex);

	// Replicated weapon pointer used by remote attachment logic and weapon presentation.
	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon, VisibleInstanceOnly, BlueprintReadOnly, Category = "Player|Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AShooterWeaponBase> EquippedWeapon;

	// Replicated logical inventory id currently equipped by this pawn.
	UPROPERTY(ReplicatedUsing = OnRep_EquippedItemId, VisibleInstanceOnly, BlueprintReadOnly, Category = "Player|Combat", meta = (AllowPrivateAccess = "true"))
	int32 EquippedItemId = INDEX_NONE;

	// Transient logical weapon instance rebuilt from inventory truth for local gameplay queries.
	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category = "Player|Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UShooterWeaponInstance> EquippedWeaponInstance;

	// Moves dropped weapons forward so they clear the character capsule.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Combat", meta = (AllowPrivateAccess = "true"))
	float WeaponDropForwardOffset = 80.f;

	// Raises the requested drop point before it is projected down to the floor.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Combat", meta = (AllowPrivateAccess = "true"))
	float WeaponDropUpOffset = 30.f;

	// Initial forward speed used by the server-side ballistic drop prediction.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Combat", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float WeaponDropForwardSpeed = 260.f;

	// Initial upward speed used by the server-side ballistic drop prediction.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Combat", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float WeaponDropUpSpeed = 340.f;

	// Duration of the visual-only throw arc played by the dropped weapon mesh.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Combat", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float WeaponDropPresentationDuration = 0.45f;

	// Extra height added to the visual-only throw arc.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Combat", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float WeaponDropPresentationArcHeight = 80.f;
};
