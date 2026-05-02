#pragma once

#include "Abilities/GameplayAbility.h"
#include "GA_FireWeapon.generated.h"

class APawn;
class AShooterWeaponBase;
class UAbilityTask_WaitDelay;
class UAbilitySystemComponent;
class UGameplayEffect;
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
	// Resolves the currently equipped logical weapon for the avatar.
	UShooterWeaponInstance* GetEquippedWeaponInstance() const;

	// Commits optional fire cost/cooldown before one shot leaves the weapon.
	bool CommitFireShot();

	// Validates the avatar/weapon chain and fires once using the configured weapon mode.
	bool FireSingleShot();

	// Starts the next delay task for automatic fire.
	void QueueNextShot(float FireInterval);

	// Resolves the center-screen aim point from the player's authoritative view.
	bool ResolveAimPoint(
		const APawn* ShooterPawn,
		const AShooterWeaponBase* EquippedWeaponActor,
		const FWeaponFireConfig& FireConfig,
		FVector& OutAimPoint) const;

	// Applies immediate damage through a muzzle-to-aim hitscan trace.
	void FireHitscanShot(
		APawn* ShooterPawn,
		AShooterWeaponBase* EquippedWeaponActor,
		UAbilitySystemComponent* SourceAbilitySystem,
		const FWeaponFireConfig& FireConfig,
		const FTransform& MuzzleTransform);

	// Spawns the configured projectile for slower physical weapons.
	void FireProjectileShot(
		APawn* ShooterPawn,
		AShooterWeaponBase* EquippedWeaponActor,
		UAbilitySystemComponent* SourceAbilitySystem,
		const FWeaponFireConfig& FireConfig,
		const FTransform& MuzzleTransform);

	// Sends the existing fire cue with a direction that matches the gameplay shot.
	void ExecuteFireCue(
		APawn* ShooterPawn,
		AShooterWeaponBase* EquippedWeaponActor,
		UAbilitySystemComponent* SourceAbilitySystem,
		const FVector& MuzzleLocation,
		const FVector& ShotDirection) const;

	// Continues automatic fire using the current weapon config.
	UFUNCTION()
	void HandleRepeatedFire();

	// Optional cost GE consumed once for every shot.
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Cost")
	TSubclassOf<UGameplayEffect> FireCostGameplayEffectClass;

	// Optional cooldown GE applied once for every shot.
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Cooldown")
	TSubclassOf<UGameplayEffect> FireCooldownGameplayEffectClass;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitDelay> FireDelayTask;
};
