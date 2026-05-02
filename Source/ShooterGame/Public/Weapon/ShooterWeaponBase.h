#pragma once

#include "GameFramework/Actor.h"
#include "Weapon/WeaponDataAsset.h"
#include "ShooterWeaponBase.generated.h"

class AShooterWeaponEquipmentActor;
class AShooterWeaponPickupActor;
class USceneComponent;
class UStaticMeshComponent;

UCLASS()
class SHOOTERGAME_API AShooterWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	AShooterWeaponBase();

	// Replicates the logical weapon snapshot consumed by pickup and equipment actors.
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Returns the resolved data-asset definition currently mirrored by this presentation actor.
	UWeaponDataAsset* GetWeaponDataAsset() const { return GetPickupData().WeaponDefinition; }

	// Returns the logical weapon snapshot currently mirrored by this presentation actor.
	FWeaponPickupData GetPickupData() const;

	// Mirrors a logical weapon snapshot onto this presentation actor.
	void SetPickupData(const FWeaponPickupData& NewPickupData);

	// Returns the pickup actor class to spawn when this weapon becomes a world item.
	TSubclassOf<AShooterWeaponPickupActor> GetPickupActorClass() const;

	// Returns the equipment actor class to spawn when this weapon is equipped.
	TSubclassOf<AShooterWeaponEquipmentActor> GetEquipmentActorClass() const;

	// Returns the resolved fire config, preferring data assets over native fallback values.
	const FWeaponFireConfig& GetFireConfig() const;

	// Returns the resolved ammo config, preferring data assets over native fallback values.
	const FWeaponAmmoConfig& GetAmmoConfig() const;

	// Returns the muzzle transform used for projectile spawn.
	FTransform GetMuzzleTransform() const;

	// Returns the socket or bone name used for character attachment.
	FName GetAttachSocketName() const;

	// Returns the runtime transform used when the weapon is attached to a character.
	FTransform GetEquippedRelativeTransform() const;

	// Returns the mesh transform used while the weapon rests as a world pickup.
	FTransform GetDroppedMeshRelativeTransform() const;

	// Returns the visible weapon mesh component.
	UStaticMeshComponent* GetWeaponMesh() const;

protected:
	// Applies any data-asset-provided presentation overrides after initialization.
	virtual void BeginPlay() override;

	// Pushes mesh overrides from the optional data asset into the runtime actor.
	void ApplyDataAssetPresentation();

	// Seeds the replicated weapon snapshot from class defaults when no runtime state exists yet.
	void SeedPickupDataFromDefaults();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<USceneComponent> SceneRootComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UStaticMeshComponent> WeaponMeshComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UWeaponDataAsset> WeaponDataAsset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<AShooterWeaponPickupActor> PickupActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<AShooterWeaponEquipmentActor> EquipmentActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName CharacterAttachSocketName = TEXT("hand_r");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FTransform WeaponMeshRelativeTransform = FTransform::Identity;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FTransform DroppedMeshRelativeTransform = FTransform::Identity;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FWeaponFireConfig FallbackFireConfig;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FWeaponAmmoConfig FallbackAmmoConfig;

private:
	// Reapplies data-driven presentation when the mirrored pickup snapshot changes.
	UFUNCTION()
	void OnRep_PickupData();

	// Replicated logical weapon snapshot mirrored by this presentation actor.
	UPROPERTY(ReplicatedUsing = OnRep_PickupData, VisibleInstanceOnly, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	FWeaponPickupData PickupData;
};
