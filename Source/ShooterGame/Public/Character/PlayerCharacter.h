#pragma once

#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameplayEffectTypes.h"
#include "Interfaces/ShooterCombatInterface.h"
#include "Interfaces/ShooterEquipmentInterface.h"
#include "PlayerCharacter.generated.h"

class UWidgetComponent;
class AShooterPlayerState;
class AShooterWeaponEquipmentActor;
class UAnimMontage;
class UAbilitySystemComponent;
class UCameraComponent;
class UShooterCombatComponent;
class UShooterHealthComponent;
class UShooterInventoryComponent;
class UShooterMovementStateComponent;
class UShooterWeaponEquipmentComponent;
class UShooterWeaponInteractionComponent;
class UShooterWeaponInstance;
class USpringArmComponent;
struct FGameplayEventData;

UCLASS()
class SHOOTERGAME_API APlayerCharacter : public ACharacter, public IAbilitySystemInterface, public IShooterCombatInterface, public IShooterEquipmentInterface
{
	GENERATED_BODY()

public:
	APlayerCharacter();

	// Applies camera-relative movement input coming from the owning controller.
	UFUNCTION(BlueprintCallable, Category = "Player|Movement")
	void Move(const FVector2D& InputValue);

	// Applies look input by rotating the owning controller.
	UFUNCTION(BlueprintCallable, Category = "Player|Movement")
	void Look(const FVector2D& InputValue);

	// Returns the combat ASC hosted by this player's PlayerState.
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// Returns the combat ASC through the avatar combat interface.
	virtual UAbilitySystemComponent* GetShooterAbilitySystemComponent() const override;

	// Initializes ASC actor info when the character becomes possessed.
	virtual void PossessedBy(AController* NewController) override;

	// Initializes ASC actor info on clients once PlayerState replicates in.
	virtual void OnRep_PlayerState() override;

public:
	// Returns the camera boom used to position the third-person camera.
	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	// Returns the gameplay camera attached to the spring arm.
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	// Returns the currently equipped weapon actor.
	UFUNCTION(BlueprintPure, Category = "Player|Combat")
	AShooterWeaponEquipmentActor* GetEquippedWeapon() const;

	// Returns the logical weapon instance currently driving combat rules.
	UFUNCTION(BlueprintPure, Category = "Player|Combat")
	UShooterWeaponInstance* GetEquippedWeaponInstance() const;

	// Returns the health adapter component used by UI and gameplay code.
	UFUNCTION(BlueprintPure, Category = "Player|Combat")
	UShooterHealthComponent* GetHealthComponent() const { return HealthComponent; }

	// Returns the long-lived logical inventory owned by this player's PlayerState.
	UFUNCTION(BlueprintPure, Category = "Player|Combat")
	UShooterInventoryComponent* GetInventoryComponent() const;

	// Returns the logical inventory through the equipment interface.
	virtual UShooterInventoryComponent* GetShooterInventoryComponent() const override;

	// Returns the component that owns fire/aim state and combat blocking rules.
	UFUNCTION(BlueprintPure, Category = "Player|Combat")
	UShooterCombatComponent* GetCombatComponent() const { return CombatComponent; }

	// Returns the component that owns fire/aim state through the combat interface.
	virtual UShooterCombatComponent* GetShooterCombatComponent() const override { return CombatComponent; }

	// Returns the component that owns sprint input state and movement-speed attributes.
	UFUNCTION(BlueprintPure, Category = "Player|Movement")
	UShooterMovementStateComponent* GetMovementStateComponent() const { return MovementStateComponent; }

	// Returns the sprint/movement component through the combat interface.
	virtual UShooterMovementStateComponent* GetShooterMovementStateComponent() const override { return MovementStateComponent; }

	// Returns the component that owns the equipped weapon, equip/drop, and attachment replication.
	UFUNCTION(BlueprintPure, Category = "Player|Combat")
	UShooterWeaponEquipmentComponent* GetWeaponEquipmentComponent() const { return WeaponEquipmentComponent; }

	// Returns the equipment component through the equipment interface.
	virtual UShooterWeaponEquipmentComponent* GetShooterWeaponEquipmentComponent() const override { return WeaponEquipmentComponent; }

	// Returns the component that owns local pickup targeting and pickup prompt state.
	UFUNCTION(BlueprintPure, Category = "Player|Combat")
	UShooterWeaponInteractionComponent* GetWeaponInteractionComponent() const { return WeaponInteractionComponent; }

	// Returns the interaction component through the equipment interface.
	virtual UShooterWeaponInteractionComponent* GetShooterWeaponInteractionComponent() const override { return WeaponInteractionComponent; }

	// Returns whether combat currently treats this character as aiming.
	UFUNCTION(BlueprintPure, Category = "Player|Combat")
	bool IsAiming() const;

	// Returns whether the character currently has an equipped weapon presentation actor.
	UFUNCTION(BlueprintPure, Category = "Player|Combat")
	bool IsEquipped() const;

	// Returns whether movement currently treats this character as sprinting.
	UFUNCTION(BlueprintPure, Category = "Player|Movement")
	bool IsSprinting() const;

	// Returns whether this pawn is currently in its death presentation state.
	UFUNCTION(BlueprintPure, Category = "Player|Combat")
	bool IsDead() const;

	// Applies the local movement-facing rules that match the combat aim state.
	void HandleAimStateChanged(bool bIsNowAiming);

	// Applies aim presentation through the combat interface.
	virtual void HandleShooterAimStateChanged(bool bIsNowAiming) override;

protected:
	// Binds health delegates once components are ready.
	virtual void BeginPlay() override;

	// Optional animation montage played once when this pawn enters the dead state.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Death")
	TObjectPtr<UAnimMontage> DeathMontage;
private:
	
	// Keeps the third-person camera behind the character while following controller rotation.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	// Provides the actual player view at the end of the spring arm.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;

	// Adapts ASC health attributes into a simpler gameplay-facing component API.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UShooterHealthComponent> HealthComponent;

	// Owns fire/aim state and combat blocking rules for this avatar.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UShooterCombatComponent> CombatComponent;

	// Owns sprint input state and applies GAS movement attributes to CharacterMovement.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Movement", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UShooterMovementStateComponent> MovementStateComponent;

	// Owns the replicated equipped weapon pointer plus equip/drop behavior.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UShooterWeaponEquipmentComponent> WeaponEquipmentComponent;

	// Owns local pickup traces and the currently highlighted world weapon.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UShooterWeaponInteractionComponent> WeaponInteractionComponent;
	
	// Overhead Widget 
	UPROPERTY(EditAnywhere,BlueprintReadOnly,meta=(AllowPrivateAccess = true))
	UWidgetComponent* OverheadWidget;
	
private:
	// Resolves the typed Shooter PlayerState helper.
	AShooterPlayerState* GetShooterPlayerState() const;
	
	// Links the pawn avatar to the PlayerState-owned ASC and initializes health bindings.
	void InitializeAbilitySystemActorInfo();

	// Rebinds the dead-state tag listener after ASC actor info changes.
	void BindDeathStateTagListener(UAbilitySystemComponent* AbilitySystemComponent);

	// Mirrors health changes for logging or future HUD hooks.
	UFUNCTION()
	void HandleHealthChanged(float OldValue, float NewValue);

	// Applies local presentation and gameplay-facing pawn shutdown for death.
	void ApplyDeathPresentation();

	// Plays the configured death montage if the mesh has an animation instance.
	void PlayDeathMontage();

	// Reacts when the replicated ASC dead tag is added to this avatar.
	void HandleDeathStateTagChanged(const FGameplayTag Tag, int32 NewCount);

	// Scales horizontal look input before it reaches the controller.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Camera", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float LookYawSensitivity = 0.4f;

	// Scales vertical look input so pitch can be tuned independently from yaw.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Camera", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float LookPitchSensitivity = 0.25f;

	// Prevents death presentation from running more than once per pawn.
	bool bDeathHandled = false;

	TWeakObjectPtr<UAbilitySystemComponent> BoundDeathStateAbilitySystemComponent;

	FDelegateHandle DeathStateTagChangedDelegateHandle;
};
