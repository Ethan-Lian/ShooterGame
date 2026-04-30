#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ShooterPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

UCLASS()
class SHOOTERGAME_API AShooterPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AShooterPlayerController();

	// Adds the default input mapping context for the locally controlled player.
	virtual void BeginPlay() override;

	// Binds Enhanced Input actions to thin forwarding functions on the controlled character.
	virtual void SetupInputComponent() override;

	// Notifies the screen HUD when local pawn ownership changes.
	virtual void SetPawn(APawn* InPawn) override;

protected:
	// Stores the default player mapping context configured by a Blueprint child class.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	// Stores the move action configured by a Blueprint child class.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Input")
	TObjectPtr<UInputAction> MoveAction;

	// Stores the look action configured by a Blueprint child class.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Input")
	TObjectPtr<UInputAction> LookAction;

	// Stores the fire action configured by a Blueprint child class.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Input")
	TObjectPtr<UInputAction> FireAction;

	// Stores the jump action configured by a Blueprint child class.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Input")
	TObjectPtr<UInputAction> JumpAction;

	// Stores the hold-to-aim action configured by a Blueprint child class.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Input")
	TObjectPtr<UInputAction> AimAction;

	// Stores the hold-to-crouch action configured by a Blueprint child class.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Input")
	TObjectPtr<UInputAction> CrouchAction;

	// Stores the hold-to-sprint action configured by a Blueprint child class.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Input")
	TObjectPtr<UInputAction> SprintAction;

	// Stores the pickup action configured by a Blueprint child class.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Input")
	TObjectPtr<UInputAction> PickupWeaponAction;

	// Stores the drop action configured by a Blueprint child class.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Input")
	TObjectPtr<UInputAction> DropWeaponAction;

private:
	// Creates runtime fallback input assets and key mappings when cooked assets are missing.
	void EnsureRuntimeInputBindings();

	// Pushes the current pawn into the HUD after local possession changes.
	void NotifyHUDObservedPawnChanged();

	// Returns whether a specific mapping context already exposes the same action/key pair.
	bool HasActionMapped(const UInputMappingContext* MappingContext, UInputAction* Action, const struct FKey& Key) const;

	// Adds a mapping only when no existing context already contains the same action/key pair.
	void EnsureActionMapped(UInputMappingContext* MappingContext, UInputAction* Action, const struct FKey& Key) const;

	// Stores the runtime-only mapping context that injects fallback actions not authored in assets.
	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> RuntimeInputMappingContext;

	// Resolves the controlled pawn as the project-specific player character type.
	class APlayerCharacter* GetPlayerCharacter() const;

	// Reads the movement vector from the action payload and forwards it to the character.
	void HandleMove(const FInputActionValue& InputValue);

	// Clears movement input when the move action is released or canceled.
	void HandleMoveCompleted();

	// Reads the look vector from the action payload and forwards it to the character.
	void HandleLook(const FInputActionValue& InputValue);

	// Starts the fire-input state on the character.
	void HandleFireStarted();

	// Stops the fire-input state on the character.
	void HandleFireCompleted();

	// Starts the jump state on the character.
	void HandleJumpStarted();

	// Stops the jump state on the character.
	void HandleJumpCompleted();

	// Starts the aim state on the character while the input stays held.
	void HandleAimStarted();

	// Stops the aim state on the character when the input is released or canceled.
	void HandleAimCompleted();

	// Starts the crouch state on the character while the input stays held.
	void HandleCrouchStarted();

	// Stops the crouch state on the character when the input is released or canceled.
	void HandleCrouchCompleted();

	// Starts the sprint state on the character while forward movement input is held.
	void HandleSprintStarted();

	// Stops the sprint state on the character when the input is released or canceled.
	void HandleSprintCompleted();

	// Requests pickup of the world weapon currently targeted by the screen crosshair.
	void HandlePickupStarted();

	// Requests dropping of the current weapon using the crosshair-driven throw direction.
	void HandleDropStarted();
};
