#pragma once

#include "Weapon/ShooterWeaponBase.h"
#include "ShooterWeaponEquipmentActor.generated.h"

class ACharacter;

UCLASS()
class SHOOTERGAME_API AShooterWeaponEquipmentActor : public AShooterWeaponBase
{
	GENERATED_BODY()

public:
	AShooterWeaponEquipmentActor();

	// Switches the actor into its character-equipped runtime state.
	void EnterEquippedState(ACharacter* NewOwnerCharacter);

protected:
	virtual void BeginPlay() override;

private:
	// Applies the non-physical equipped presentation that follows a character mesh.
	void ApplyEquippedRuntimeState();
};
