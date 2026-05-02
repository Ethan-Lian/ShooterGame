#pragma once

#include "Weapon/ShooterWeaponEquipmentActor.h"
#include "Weapon/ShooterWeaponPickupActor.h"
#include "AK47Weapon.generated.h"

UCLASS()
class SHOOTERGAME_API AAK47WeaponEquipmentActor : public AShooterWeaponEquipmentActor
{
	GENERATED_BODY()

public:
	AAK47WeaponEquipmentActor();
};

UCLASS()
class SHOOTERGAME_API AAK47Weapon : public AShooterWeaponPickupActor
{
	GENERATED_BODY()

public:
	AAK47Weapon();
};
