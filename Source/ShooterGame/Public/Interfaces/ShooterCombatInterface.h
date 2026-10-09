#pragma once

#include "UObject/Interface.h"
#include "ShooterCombatInterface.generated.h"

class UAbilitySystemComponent;
class UShooterCombatComponent;
class UShooterMovementStateComponent;

UINTERFACE(MinimalAPI)
class UShooterCombatInterface : public UInterface
{
	GENERATED_BODY()
};

class SHOOTERGAME_API IShooterCombatInterface
{
	GENERATED_BODY()

public:
	// Returns the ASC that owns combat abilities, attributes, and state tags.
	virtual UAbilitySystemComponent* GetShooterAbilitySystemComponent() const = 0;

	// Returns the component that owns fire and aim state for this avatar.
	virtual UShooterCombatComponent* GetShooterCombatComponent() const = 0;

	// Returns the component that owns sprint input and movement attribute binding.
	virtual UShooterMovementStateComponent* GetShooterMovementStateComponent() const = 0;

	// Lets combat state changes update avatar-specific facing presentation.
	virtual void HandleShooterAimStateChanged(bool bIsNowAiming) = 0;
};
