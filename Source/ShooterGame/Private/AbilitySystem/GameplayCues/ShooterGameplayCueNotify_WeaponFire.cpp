#include "AbilitySystem/GameplayCues/ShooterGameplayCueNotify_WeaponFire.h"
#include "AbilitySystem/ShooterGameplayTags.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "ShooterGame.h"
#include "Weapon/ShooterWeaponBase.h"
#include "Weapon/WeaponDataAsset.h"

UShooterGameplayCueNotify_WeaponFire::UShooterGameplayCueNotify_WeaponFire()
{
	GameplayCueTag = TAG_GameplayCue_Weapon_Fire;
}

bool UShooterGameplayCueNotify_WeaponFire::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	Super::OnExecute_Implementation(MyTarget, Parameters);

	AShooterWeaponBase* Weapon = nullptr;
	UStaticMeshComponent* WeaponMesh = nullptr;
	if (!ResolveWeaponCueTarget(Parameters, Weapon, WeaponMesh))
	{
		UE_LOG(LogShooterGame, Warning, TEXT("WeaponFire GameplayCue skipped: EffectCauser is not a valid ShooterWeaponBase with a weapon mesh."));
		return false;
	}

	const FWeaponFireConfig& FireConfig = Weapon->GetFireConfig();
	if (FireConfig.MuzzleSocketName.IsNone() || !WeaponMesh->DoesSocketExist(FireConfig.MuzzleSocketName))
	{
		UE_LOG(
			LogShooterGame,
			Warning,
			TEXT("WeaponFire GameplayCue skipped: weapon %s mesh %s does not have muzzle socket '%s'."),
			*GetNameSafe(Weapon),
			*GetNameSafe(WeaponMesh),
			*FireConfig.MuzzleSocketName.ToString());
		return false;
	}

	bool bSpawnedCueEffect = false;
	if (FireConfig.MuzzleFlashEffect != nullptr)
	{
		UNiagaraFunctionLibrary::SpawnSystemAttached(
			FireConfig.MuzzleFlashEffect,
			WeaponMesh,
			FireConfig.MuzzleSocketName,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			true);
		bSpawnedCueEffect = true;
	}

	if (FireConfig.FireSound != nullptr)
	{
		UGameplayStatics::SpawnSoundAttached(
			FireConfig.FireSound,
			WeaponMesh,
			FireConfig.MuzzleSocketName,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget);
		bSpawnedCueEffect = true;
	}

	if (!bSpawnedCueEffect)
	{
		UE_LOG(LogShooterGame, Warning, TEXT("WeaponFire GameplayCue executed but weapon %s has no muzzle flash or fire sound configured."), *GetNameSafe(Weapon));
	}

	return bSpawnedCueEffect;
}

bool UShooterGameplayCueNotify_WeaponFire::ResolveWeaponCueTarget(
	const FGameplayCueParameters& Parameters,
	AShooterWeaponBase*& OutWeapon,
	UStaticMeshComponent*& OutWeaponMesh)
{
	OutWeapon = Cast<AShooterWeaponBase>(Parameters.EffectCauser.Get());
	OutWeaponMesh = OutWeapon != nullptr ? OutWeapon->GetWeaponMesh() : nullptr;
	return OutWeapon != nullptr && OutWeaponMesh != nullptr;
}
