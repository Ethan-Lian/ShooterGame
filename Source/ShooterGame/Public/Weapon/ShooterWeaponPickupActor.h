#pragma once

#include "Weapon/ShooterWeaponBase.h"
#include "ShooterWeaponPickupActor.generated.h"

class USphereComponent;
class UWidgetComponent;

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
class SHOOTERGAME_API AShooterWeaponPickupActor : public AShooterWeaponBase
{
	GENERATED_BODY()

public:
	AShooterWeaponPickupActor();

	// Replicates pickup availability and local drop presentation state.
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Returns whether pickup interaction is currently allowed for this world weapon.
	bool IsPickupInteractionEnabled() const { return bPickupInteractionEnabled && !bDropPresentationActive; }

	// Switches the actor into its world-pickup runtime state.
	void EnterWorldPickupState(const FTransform& WorldTransform);

	// Starts the local throw presentation for a server-authored world pickup.
	void SetDropPresentationData(const FWeaponDropPresentationData& NewDropPresentationData);

	// Shows or hides the local pickup prompt widget.
	void SetPickupWidgetVisible(bool bVisible);

protected:
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaSeconds) override;

	// Retained only for Blueprint compatibility; pickup selection now uses screen-space traces.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<USphereComponent> PickupTrigger;

	UPROPERTY(VisibleAnywhere, Category = "Weapon")
	TObjectPtr<UWidgetComponent> PickupWidget;

	// Legacy overlap radius kept only so existing Blueprint defaults continue to load.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0.0"))
	float PickupTriggerRadius = 250.f;

private:
	// Applies the query/physics configuration used while the weapon is in the world.
	void ApplyWorldPickupRuntimeState();

	// Starts or restarts visual-only throw interpolation from replicated data.
	void BeginDropPresentation();

	// Ends visual-only throw interpolation and enables pickup interaction.
	void FinishDropPresentation();

	// Moves only the mesh along the throw arc while the actor root stays authoritative.
	void UpdateDropPresentationVisual(float Alpha);

	UFUNCTION()
	void OnRep_DropPresentationData();

	UFUNCTION()
	void OnRep_PickupInteractionEnabled();

	UPROPERTY(ReplicatedUsing = OnRep_DropPresentationData, VisibleInstanceOnly, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	FWeaponDropPresentationData DropPresentationData;

	UPROPERTY(ReplicatedUsing = OnRep_PickupInteractionEnabled, VisibleInstanceOnly, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	bool bPickupInteractionEnabled = true;

	bool bDropPresentationActive = false;

	float DropPresentationElapsedTime = 0.f;
};
