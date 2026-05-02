#pragma once

#include "Components/ActorComponent.h"
#include "ShooterWeaponInteractionComponent.generated.h"

class AShooterWeaponPickupActor;
class UShooterCombatComponent;
class UShooterWeaponEquipmentComponent;
struct FHitResult;

UCLASS(ClassGroup = (ShooterGame), Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent))
class SHOOTERGAME_API UShooterWeaponInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UShooterWeaponInteractionComponent();

	// Re-evaluates the local crosshair-targeted pickup weapon every frame.
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Returns the currently targeted world weapon for local pickup interaction.
	UFUNCTION(BlueprintPure, Category = "Shooter|Combat")
	AShooterWeaponPickupActor* GetCurrentTargetedPickupWeapon() const { return CurrentTargetedPickupWeapon.Get(); }

	// Returns the valid world weapon currently under the view trace.
	AShooterWeaponPickupActor* FindPickupWeaponFromView() const;

	// Validates whether a weapon can currently be picked up by the owning character.
	bool CanPickupWeapon(const AShooterWeaponPickupActor* WeaponToPickup) const;

	// Resolves start and end points for crosshair-driven view traces.
	bool GetViewTracePoints(float TraceDistance, FVector& OutTraceStart, FVector& OutTraceEnd) const;

	// Returns the max distance used by crosshair-driven pickup traces.
	float GetPickupTraceMaxDistance() const { return PickupTraceMaxDistance; }

	// Clears local pickup prompt state when combat interaction becomes invalid.
	void ClearTargetedPickupWeapon();

private:
	// Resolves the combat component that owns death/combat blocking rules.
	UShooterCombatComponent* GetOwningCombatComponent() const;

	// Resolves the equipment component that owns the equipped weapon pointer.
	UShooterWeaponEquipmentComponent* GetOwningWeaponEquipmentComponent() const;

	// Performs the current pickup trace from the view direction.
	bool TracePickupView(FHitResult& OutHitResult) const;

	// Returns whether local pickup interaction should currently be suppressed.
	bool IsInteractionBlocked() const;

	// Refreshes which single world weapon may show its pickup prompt.
	void RefreshTargetedPickupWeapon();

	// Applies widget visibility changes when the targeted weapon changes.
	void SetTargetedPickupWeapon(AShooterWeaponPickupActor* NewTargetWeapon);

	// The single local world weapon currently targeted by the center-screen trace.
	TWeakObjectPtr<AShooterWeaponPickupActor> CurrentTargetedPickupWeapon;

	// Limits how far away a world weapon can be for pickup interactions.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Combat", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float PickupSearchRadius = 250.f;

	// Limits how far the crosshair trace can search for candidate pickup weapons.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Combat", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float PickupTraceMaxDistance = 3000.f;

	// Adds a small amount of forgiveness when the crosshair almost touches a weapon.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Combat", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float PickupTraceRadius = 12.f;
};
