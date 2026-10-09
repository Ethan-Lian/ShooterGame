#include "AbilitySystem/GameplayCues/ShooterGameplayCueNotify_WeaponFire.h"
#include "AbilitySystem/ShooterGameplayTags.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "ShooterGame.h"
#include "Weapon/ShooterWeaponBase.h"
#include "Weapon/WeaponDataAsset.h"
#include "Character/PlayerCharacter.h"

UShooterGameplayCueNotify_WeaponFire::UShooterGameplayCueNotify_WeaponFire()
{
	GameplayCueTag = TAG_GameplayCue_Weapon_Fire;
}

bool UShooterGameplayCueNotify_WeaponFire::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	Super::OnExecute_Implementation(MyTarget, Parameters);
	if (APlayerCharacter* Pawn = Cast<APlayerCharacter>(Parameters.Instigator.Get()))
	{
		Pawn->PlayFirstPersonFire();
	}

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

	USceneComponent* CueMesh = WeaponMesh;
	FName CueSocket = FireConfig.MuzzleSocketName;
	FRotator CueRotation = FRotator::ZeroRotator;
	bool bFirstPersonCue = false;
	if (APlayerCharacter* Pawn = Cast<APlayerCharacter>(Parameters.Instigator.Get());
		Pawn != nullptr && Pawn->IsLocallyControlled() && Pawn->GetFirstPersonWeapon()->IsVisible())
	{
		CueMesh = Pawn->GetFirstPersonWeapon();
		CueSocket = TEXT("FP_Muzzle");
		// The existing flash emits along +Z; the template rifle's barrel is +Y.
		CueRotation = FRotator(0.f, 0.f, 90.f);
		bFirstPersonCue = true;
	}
	bool bSpawnedCueEffect = false;
	if (FireConfig.MuzzleFlashEffect != nullptr)
	{
		UNiagaraComponent* Effect = UNiagaraFunctionLibrary::SpawnSystemAttached(
			FireConfig.MuzzleFlashEffect,
			CueMesh,
			CueSocket,
			FVector::ZeroVector,
			CueRotation,
			EAttachLocation::SnapToTarget,
			true,
			false);
		if (Effect != nullptr)
		{
			if (bFirstPersonCue)
			{
				Effect->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
				Effect->SetOnlyOwnerSee(true);
				// The world-scale example obscures the view at first-person distance.
				Effect->SetVariableFloat(TEXT("User.Global Scale"), 0.08f);
				Effect->SetVariableBool(TEXT("User.Use Smoke"), false);
			}
			Effect->Activate();
		}
		bSpawnedCueEffect = true;
	}

	if (FireConfig.FireSound != nullptr)
	{
		UGameplayStatics::SpawnSoundAttached(
			FireConfig.FireSound,
			CueMesh,
			CueSocket,
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
