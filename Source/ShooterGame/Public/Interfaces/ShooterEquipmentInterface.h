#pragma once

#include "UObject/Interface.h"
#include "ShooterEquipmentInterface.generated.h"

class UShooterInventoryComponent;
class UShooterWeaponEquipmentComponent;

UINTERFACE(MinimalAPI)
class UShooterEquipmentInterface : public UInterface
{
	GENERATED_BODY()
};

class SHOOTERGAME_API IShooterEquipmentInterface
{
	GENERATED_BODY()

public:
	// Returns the long-lived logical weapon inventory for this avatar.
	virtual UShooterInventoryComponent* GetShooterInventoryComponent() const = 0;

	// Returns the component that owns fixed weapon state and presentation.
	virtual UShooterWeaponEquipmentComponent* GetShooterWeaponEquipmentComponent() const = 0;

};
