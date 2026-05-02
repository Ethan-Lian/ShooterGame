#pragma once

#include "UObject/Interface.h"
#include "ShooterEquipmentInterface.generated.h"

class UShooterInventoryComponent;
class UShooterWeaponEquipmentComponent;
class UShooterWeaponInteractionComponent;

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

	// Returns the component that owns equipped weapon state and equip/drop behavior.
	virtual UShooterWeaponEquipmentComponent* GetShooterWeaponEquipmentComponent() const = 0;

	// Returns the component that owns local pickup targeting and prompt state.
	virtual UShooterWeaponInteractionComponent* GetShooterWeaponInteractionComponent() const = 0;
};
