#pragma once

#include "GameFramework/Actor.h"
#include "ShooterProjectileBase.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class UProjectileMovementComponent;
class USphereComponent;

UCLASS()
class SHOOTERGAME_API AShooterProjectileBase : public AActor
{
	GENERATED_BODY()

public:
	AShooterProjectileBase();

	// Supplies the authoritative damage payload after the projectile is spawned.
	void InitializeProjectile(UAbilitySystemComponent* InSourceAbilitySystem, TSubclassOf<UGameplayEffect> InDamageEffectClass, float InBaseDamage);

protected:
	// Hooks the projectile stop delegate once movement is live.
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovementComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "0.0"))
	float InitialSpeed = 12000.f;

private:
	// Applies damage or handles world impact when the projectile stops.
	UFUNCTION()
	void HandleProjectileStop(const FHitResult& ImpactResult);

	// Builds and applies the configured damage effect to a valid target.
	void ApplyImpactDamage(const FHitResult& ImpactResult);

	TWeakObjectPtr<UAbilitySystemComponent> SourceAbilitySystemComponent;
	TSubclassOf<UGameplayEffect> DamageEffectClass;
	float BaseDamage = 0.f;
	bool bHasProcessedImpact = false;
};
