#pragma once

#include "GameplayCueNotify_Burst.h"
#include "ShooterGameplayCueNotify_WeaponFire.generated.h"

class AShooterWeaponBase;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class SHOOTERGAME_API UShooterGameplayCueNotify_WeaponFire : public UGameplayCueNotify_Burst
{
	GENERATED_BODY()

public:
	UShooterGameplayCueNotify_WeaponFire();

	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;

private:
	static bool ResolveWeaponCueTarget(const FGameplayCueParameters& Parameters, AShooterWeaponBase*& OutWeapon, UStaticMeshComponent*& OutWeaponMesh);
};
