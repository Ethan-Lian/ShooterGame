#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "ShooterAnimInstance.generated.h"

class APlayerCharacter;

UCLASS(Blueprintable)
class SHOOTERGAME_API UShooterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

 public:
	// Caches the owning player character when the animation instance is created.
	virtual void NativeInitializeAnimation() override;

	// Refreshes locomotion state used by the animation blueprint every frame.
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	// Returns the cached player character that currently owns this animation instance.
	UFUNCTION(BlueprintPure, Category = "Animation")
	APlayerCharacter* GetOwningPlayerCharacter() const;

protected:
	// Horizontal movement speed used by idle, walk, and run state machines.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Locomotion")
	float GroundSpeed = 0.f;

	// True while the character movement component reports an airborne movement mode.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Locomotion")
	bool bIsInAir = false;

	// True while combat wants the locomotion graph to present an aiming pose.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Combat")
	bool bIsAiming = false;

	// True while the character currently has a weapon equipped and should use equipped locomotion.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Combat")
	bool bIsEquipped = false;

	// True while the owning character is currently crouched.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Locomotion")
	bool bIsCrouching = false;

	// True while the owning character is currently sprinting.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Locomotion")
	bool bIsSprinting = false;

	// World-weapon grip expressed in hand_r bone space for the third-person FABRIK node.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|WeaponIK")
	FTransform LeftHandIKTransform = FTransform::Identity;

	// Zero for first-person meshes, dead pawns, or weapons without a LeftHandGrip socket.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|WeaponIK")
	float LeftHandIKAlpha = 0.f;

private:
	void UpdateLeftHandIK(const APlayerCharacter* PlayerCharacter);

	// Reacquires the owning player character when the cached weak pointer becomes stale.
	void RefreshOwningPlayerCharacter();

	// Non-owning cache of the character that drives this animation instance.
	TWeakObjectPtr<APlayerCharacter> OwningPlayerCharacter;
};
