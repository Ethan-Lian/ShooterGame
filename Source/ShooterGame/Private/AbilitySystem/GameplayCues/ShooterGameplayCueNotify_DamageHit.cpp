#include "AbilitySystem/GameplayCues/ShooterGameplayCueNotify_DamageHit.h"
#include "AbilitySystem/ShooterGameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "ShooterGame.h"

UShooterGameplayCueNotify_DamageHit::UShooterGameplayCueNotify_DamageHit()
{
	GameplayCueTag = TAG_GameplayCue_Damage_Hit;
}

bool UShooterGameplayCueNotify_DamageHit::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	Super::OnExecute_Implementation(MyTarget, Parameters);

	FVector ImpactPoint = FVector::ZeroVector;
	FVector ImpactNormal = FVector::ZeroVector;
	if (!ResolveImpactCueTarget(Parameters, ImpactPoint, ImpactNormal))
	{
		UE_LOG(LogShooterGame, Warning, TEXT("DamageHit GameplayCue skipped: cue parameters do not contain a valid HitResult impact normal."));
		return false;
	}

	const FRotator ImpactRotation = ImpactNormal.Rotation();
	bool bSpawnedCueEffect = false;
	if (ImpactEffect != nullptr)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			MyTarget != nullptr ? MyTarget->GetWorld() : nullptr,
			ImpactEffect,
			ImpactPoint,
			ImpactRotation,
			ImpactEffectScale);
		bSpawnedCueEffect = true;
	}

	if (ImpactSound != nullptr)
	{
		UGameplayStatics::PlaySoundAtLocation(
			MyTarget,
			ImpactSound,
			ImpactPoint,
			ImpactSoundVolumeMultiplier);
		bSpawnedCueEffect = true;
	}

	if (!bSpawnedCueEffect)
	{
		UE_LOG(LogShooterGame, Warning, TEXT("DamageHit GameplayCue executed but no impact Niagara or sound is configured on the Cue asset."));
	}

	return bSpawnedCueEffect;
}

bool UShooterGameplayCueNotify_DamageHit::ResolveImpactCueTarget(
	const FGameplayCueParameters& Parameters,
	FVector& OutImpactPoint,
	FVector& OutImpactNormal)
{
	if (const FHitResult* HitResult = Parameters.EffectContext.GetHitResult())
	{
		OutImpactPoint = HitResult->ImpactPoint;
		OutImpactNormal = HitResult->ImpactNormal;
		return !OutImpactNormal.IsNearlyZero();
	}

	return false;
}
