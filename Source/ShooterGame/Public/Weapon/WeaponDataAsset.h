#pragma once

#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "WeaponDataAsset.generated.h"

class AShooterProjectileBase;
class AShooterWeaponEquipmentActor;
class AShooterWeaponPickupActor;
class UGameplayEffect;
class UNiagaraSystem;
class USoundBase;
class UStaticMesh;

UENUM(BlueprintType)
enum class EWeaponFireMode : uint8
{
	Hitscan,
	Projectile
};

USTRUCT(BlueprintType)
struct FWeaponFireConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FGameplayTag WeaponTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	EWeaponFireMode FireMode = EWeaponFireMode::Hitscan;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<AShooterProjectileBase> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0.0"))
	float BaseDamage = 25.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0.01"))
	float FireInterval = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0.0"))
	float AimTraceDistance = 100000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName MuzzleSocketName = TEXT("Muzzle");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FTransform MuzzleFallbackTransform = FTransform(FRotator::ZeroRotator, FVector(60.f, 0.f, 8.f));

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Effects")
	TObjectPtr<UNiagaraSystem> MuzzleFlashEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Effects")
	TObjectPtr<USoundBase> FireSound;
};

USTRUCT(BlueprintType)
struct FWeaponAmmoConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0"))
	int32 MagazineSize = 30;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0"))
	int32 InitialReserveAmmo = 90;
};

USTRUCT(BlueprintType)
struct FWeaponPickupData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UWeaponDataAsset> WeaponDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0"))
	int32 CurrentMagazineAmmo = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0"))
	int32 CurrentReserveAmmo = 0;

	// Returns whether the snapshot carries enough data to rebuild a logical weapon entry.
	bool HasValidDefinition() const
	{
		return WeaponDefinition != nullptr;
	}
};

UCLASS(BlueprintType)
class SHOOTERGAME_API UWeaponDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TSoftObjectPtr<UStaticMesh> WeaponMesh;

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
	FWeaponFireConfig FireConfig;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FWeaponAmmoConfig AmmoConfig;
};
