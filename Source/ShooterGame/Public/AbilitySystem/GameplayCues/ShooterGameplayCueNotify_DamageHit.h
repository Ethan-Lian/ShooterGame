#pragma once

#include "GameplayCueNotify_Burst.h"
#include "ShooterGameplayCueNotify_DamageHit.generated.h"

class UNiagaraSystem;
class USoundBase;

UCLASS(Blueprintable)
class SHOOTERGAME_API UShooterGameplayCueNotify_DamageHit : public UGameplayCueNotify_Burst
{
	GENERATED_BODY()

public:
	UShooterGameplayCueNotify_DamageHit();

	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameplayCue|Effects")
	TObjectPtr<UNiagaraSystem> ImpactEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameplayCue|Effects")
	TObjectPtr<USoundBase> ImpactSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameplayCue|Effects")
	FVector ImpactEffectScale = FVector(1.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameplayCue|Effects", meta = (ClampMin = "0.0"))
	float ImpactSoundVolumeMultiplier = 1.f;

private:
	static bool ResolveImpactCueTarget(const FGameplayCueParameters& Parameters, FVector& OutImpactPoint, FVector& OutImpactNormal);
};
