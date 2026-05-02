#pragma once
#include "GameFramework/Actor.h"
#include "Weapon/WeaponDataAsset.h"
#include "ShooterWeaponBase.generated.h"
class UWidgetComponent;
class USphereComponent;
class ACharacter;
class USceneComponent;
class UStaticMeshComponent;

USTRUCT(BlueprintType)
struct FWeaponDropPresentationData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Drop")
	FVector StartLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Drop")
	FVector EndLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Drop")
	float Duration = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Drop")
	float ArcHeight = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Drop")
	FTransform FinalMeshRelativeTransform = FTransform::Identity;

	bool IsValid() const
	{
		return Duration > 0.f;
	}
};

UCLASS()
class SHOOTERGAME_API AShooterWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	AShooterWeaponBase();

	// Replicates world-pickup state changes so clients can mirror runtime behavior.
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Returns the resolved data-asset definition currently mirrored by this presentation actor.
	UWeaponDataAsset* GetWeaponDataAsset() const { return GetPickupData().WeaponDefinition != nullptr ? GetPickupData().WeaponDefinition : WeaponDataAsset; }

	// Returns the logical pickup snapshot currently mirrored by this presentation actor.
	FWeaponPickupData GetPickupData() const;

	// Mirrors a logical pickup snapshot onto this presentation actor.
	void SetPickupData(const FWeaponPickupData& NewPickupData);

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

	// Returns whether the weapon currently lives in the world and can be picked up.
	bool IsWorldPickup() const { return bIsWorldPickup; }

	// Returns whether pickup interaction is currently allowed for this world weapon.
	bool IsPickupInteractionEnabled() const { return bIsWorldPickup && bPickupInteractionEnabled && !bDropPresentationActive; }

	// Switches the weapon into its character-equipped runtime state.
	void EnterEquippedState(ACharacter* NewOwnerCharacter);

	// Switches the weapon into its world-pickup runtime state.
	void EnterWorldPickupState(const FTransform& WorldTransform);

	// Starts the local throw presentation for a server-authored world pickup.
	void SetDropPresentationData(const FWeaponDropPresentationData& NewDropPresentationData);

	// Shows or hides the local pickup prompt widget.
	void SetPickupWidgetVisible(bool bVisible);

protected:
	// Applies any data-asset-provided presentation overrides after initialization.
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<USceneComponent> SceneRootComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UStaticMeshComponent> WeaponMeshComponent;

	// Retained only for Blueprint compatibility; pickup selection now uses screen-space traces.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<USphereComponent> PickupTrigger;

	// Pick up widget to notify user to pick up weapon
	UPROPERTY(VisibleAnywhere,Category = "Weapon")
	UWidgetComponent* PickupWidget;

	// Legacy overlap radius kept only so existing Blueprint defaults continue to load.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0.0"))
	float PickupTriggerRadius = 250.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UWeaponDataAsset> WeaponDataAsset;

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
	// Pushes mesh overrides from the optional data asset into the runtime actor.
	void ApplyDataAssetPresentation();

	// Seeds the replicated pickup snapshot from class defaults when no runtime state exists yet.
	void SeedPickupDataFromDefaults();

	// Applies the non-physical equipped presentation that follows a character mesh.
	void ApplyEquippedRuntimeState();

	// Applies the query/physics configuration used while the weapon is in the world.
	void ApplyWorldPickupRuntimeState();

	// Starts or restarts visual-only throw interpolation from replicated data.
	void BeginDropPresentation();

	// Ends visual-only throw interpolation and enables pickup interaction.
	void FinishDropPresentation();

	// Moves only the mesh along the throw arc while the actor root stays authoritative.
	void UpdateDropPresentationVisual(float Alpha);

	// Reapplies world-pickup state on clients when the replicated flag changes.
	UFUNCTION()
	void OnRep_IsWorldPickup();

	// Reapplies data-driven presentation when the mirrored pickup snapshot changes.
	UFUNCTION()
	void OnRep_PickupData();

	UFUNCTION()
	void OnRep_DropPresentationData();

	UFUNCTION()
	void OnRep_PickupInteractionEnabled();

	// Replicated logical weapon snapshot mirrored by this presentation actor.
	UPROPERTY(ReplicatedUsing = OnRep_PickupData, VisibleInstanceOnly, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	FWeaponPickupData PickupData;

	UPROPERTY(ReplicatedUsing = OnRep_DropPresentationData, VisibleInstanceOnly, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	FWeaponDropPresentationData DropPresentationData;

	// Tracks whether this actor should behave as a world pickup or as an equipped weapon.
	UPROPERTY(ReplicatedUsing = OnRep_IsWorldPickup, VisibleInstanceOnly, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	bool bIsWorldPickup = true;

	UPROPERTY(ReplicatedUsing = OnRep_PickupInteractionEnabled, VisibleInstanceOnly, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	bool bPickupInteractionEnabled = true;

	bool bDropPresentationActive = false;

	float DropPresentationElapsedTime = 0.f;
};
