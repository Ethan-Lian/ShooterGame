#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "ShooterPlayerController.generated.h"

class ACharacter;
class APlayerCharacter;
class UAbilitySystemComponent;
class UShooterAbilitySystemComponent;
class UShooterInputConfig;
class UShooterCombatComponent;
class UShooterMovementStateComponent;
struct FInputActionValue;

UCLASS()
class SHOOTERGAME_API AShooterPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AShooterPlayerController();

	// Adds configured input mapping contexts for the locally controlled player.
	virtual void BeginPlay() override;

	// Binds Enhanced Input actions from the configured input data asset.
	virtual void SetupInputComponent() override;

	// Notifies the screen HUD when local pawn ownership changes.
	virtual void SetPawn(APawn* InPawn) override;

protected:
	// Data asset that owns mapping contexts plus native and ability input actions.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Input")
	TObjectPtr<UShooterInputConfig> InputConfig;

private:
	// Pushes the current pawn into the HUD after local possession changes.
	void NotifyHUDObservedPawnChanged();

	// Resolves the controlled pawn as the project-specific player character type.
	APlayerCharacter* GetPlayerCharacter() const;

	// Resolves the controlled pawn as a character for built-in movement actions.
	ACharacter* GetControlledCharacter() const;

	// Resolves the controlled pawn's ASC through the combat interface.
	UAbilitySystemComponent* GetControlledAbilitySystemComponent() const;

	// Resolves the controlled pawn's Shooter ASC when generic ability input can be forwarded directly.
	UShooterAbilitySystemComponent* GetControlledShooterAbilitySystemComponent() const;

	// Resolves the controlled pawn's combat component through the combat interface.
	UShooterCombatComponent* GetControlledCombatComponent() const;

	// Resolves the controlled pawn's movement-state component through the combat interface.
	UShooterMovementStateComponent* GetControlledMovementStateComponent() const;

	// Returns whether input should be blocked because the pawn is already dead.
	bool IsControlledPawnDead() const;

	// Reads the movement vector from the action payload and forwards it to the character.
	void HandleMove(const FInputActionValue& InputValue);

	// Clears movement input when the move action is released or canceled.
	void HandleMoveCompleted();

	// Reads the look vector from the action payload and forwards it to the character.
	void HandleLook(const FInputActionValue& InputValue);

	// Routes an ability-tagged input press to the component that owns that gameplay state.
	void HandleAbilityInputPressed(FGameplayTag InputTag);

	// Routes an ability-tagged input release to the component that owns that gameplay state.
	void HandleAbilityInputReleased(FGameplayTag InputTag);

	// Starts the built-in jump state on the controlled character.
	void HandleJumpStarted();

	// Stops the built-in jump state on the controlled character.
	void HandleJumpCompleted();

	// Starts the combat aim state while the input stays held.
	void HandleAimStarted();

	// Stops the combat aim state when the input is released or canceled.
	void HandleAimCompleted();

	// Starts the built-in crouch state while the input stays held.
	void HandleCrouchStarted();

	// Stops the built-in crouch state when the input is released or canceled.
	void HandleCrouchCompleted();

	TArray<uint32> InputBindHandles;
};
