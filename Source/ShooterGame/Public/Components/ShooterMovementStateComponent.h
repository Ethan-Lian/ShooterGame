#pragma once

#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "ShooterMovementStateComponent.generated.h"

class UAbilitySystemComponent;

UCLASS(ClassGroup = (ShooterGame), Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent))
class SHOOTERGAME_API UShooterMovementStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UShooterMovementStateComponent();

	// Caches movement input so sprint can enforce its forward-only rule.
	void HandleMoveInput(const FVector2D& InputValue);

	// Records sprint input and tries to activate the sprint ability when movement direction allows it.
	bool StartSprintInput();

	// Clears sprint input and cancels any active sprint ability.
	bool StopSprintInput();

	// Cancels movement ability state during owner death handling.
	void HandleOwnerDeath();

	// Binds movement attributes from the PlayerState-hosted ASC to this avatar's movement component.
	void InitializeWithAbilitySystem(UAbilitySystemComponent* InAbilitySystemComponent);

	// Returns whether current input is forward-only enough to enter or keep sprinting.
	UFUNCTION(BlueprintPure, Category = "Shooter|Movement")
	bool IsSprintDirectionAllowed() const;

	// Returns whether the ASC currently owns the sprinting state tag.
	UFUNCTION(BlueprintPure, Category = "Shooter|Movement")
	bool IsSprinting() const;

private:
	// Returns whether this component is running on the authoritative owner actor.
	bool IsOwnerAuthority() const;

	// Applies the latest movement speed attribute to CharacterMovement.
	void ApplyMaxWalkSpeed(float NewMaxWalkSpeed) const;

	// Reacts when GAS changes the authoritative movement speed attribute.
	void HandleMaxWalkSpeedChanged(const FOnAttributeChangeData& ChangeData);

	// Tries to match the active sprint ability to current input and direction state.
	bool RefreshSprintAbilityState();

	// Activates the sprint ability by GameplayTag.
	bool ActivateSprintAbility() const;

	// Cancels the sprint ability by GameplayTag.
	bool CancelSprintAbility() const;

	// Sends sprint press/release state to the server-authoritative sprint path.
	UFUNCTION(Server, Reliable)
	void ServerSetSprintInputPressed(bool bNewSprintInputPressed, FVector2D MoveInput);

	// Keeps the server-side forward-only sprint rule matched to the owning client's move input.
	UFUNCTION(Server, Reliable)
	void ServerUpdateSprintMoveInput(FVector2D MoveInput);

	UPROPERTY(Transient)
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	FDelegateHandle MaxWalkSpeedChangedDelegateHandle;

	FVector2D LastMoveInput = FVector2D::ZeroVector;

	bool bSprintInputPressed = false;
};
