#pragma once

#include "Abilities/GameplayAbility.h"
#include "GA_FireWeapon.generated.h"

class APlayerCharacter;
class AShooterWeaponBase;
class UAbilitySystemComponent;
class UShooterWeaponInstance;
struct FWeaponFireConfig;

UCLASS()
class SHOOTERGAME_API UGA_FireWeapon : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_FireWeapon();

	// Starts the server-authoritative automatic fire loop.
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	// Stops the repeating fire loop when input is released or canceled.
	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

private:
	// Validates the avatar/weapon chain and fires once using the configured weapon mode.
	void FireSingleShot();

	// Resolves the center-screen aim point from the player's authoritative view.
	bool ResolveAimPoint(
		const APlayerCharacter* ShooterCharacter,
		const AShooterWeaponBase* EquippedWeaponActor,
		const FWeaponFireConfig& FireConfig,
		FVector& OutAimPoint) const;

	// Applies immediate damage through a muzzle-to-aim hitscan trace.
	void FireHitscanShot(
		APlayerCharacter* ShooterCharacter,
		AShooterWeaponBase* EquippedWeaponActor,
		UAbilitySystemComponent* SourceAbilitySystem,
		const FWeaponFireConfig& FireConfig,
		const FTransform& MuzzleTransform);

	// Spawns the configured projectile for slower physical weapons.
	void FireProjectileShot(
		APlayerCharacter* ShooterCharacter,
		AShooterWeaponBase* EquippedWeaponActor,
		UAbilitySystemComponent* SourceAbilitySystem,
		const FWeaponFireConfig& FireConfig,
		const FTransform& MuzzleTransform);

	// Sends the existing fire cue with a direction that matches the gameplay shot.
	void ExecuteFireCue(
		APlayerCharacter* ShooterCharacter,
		AShooterWeaponBase* EquippedWeaponActor,
		UAbilitySystemComponent* SourceAbilitySystem,
		const FVector& MuzzleLocation,
		const FVector& ShotDirection) const;

	// Continues automatic fire using the current weapon config.
	void HandleRepeatedFire();

	FTimerHandle RepeatingFireTimerHandle;
};
