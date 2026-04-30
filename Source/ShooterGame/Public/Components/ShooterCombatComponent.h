#pragma once

#include "Components/ActorComponent.h"
#include "ShooterCombatComponent.generated.h"

class APlayerCharacter;
class UAbilitySystemComponent;
class UShooterWeaponEquipmentComponent;
class UShooterWeaponInteractionComponent;

UCLASS(ClassGroup = (ShooterGame), Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent))
class SHOOTERGAME_API UShooterCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UShooterCombatComponent();

	// Replicates combat stance state used by remote animation and gameplay.
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Returns whether the owner currently treats combat input as aiming.
	UFUNCTION(BlueprintPure, Category = "Shooter|Combat")
	bool IsAiming() const { return bIsAiming; }

	// Returns whether pickup/drop interactions should currently be blocked.
	UFUNCTION(BlueprintPure, Category = "Shooter|Combat")
	bool IsWeaponInteractionBlocked() const;

	// Records the fire press and requests server-authoritative fire activation.
	bool StartFireInput();

	// Records the fire release and requests server-authoritative fire cancellation.
	bool StopFireInput();

	// Records the aim press and requests server-authoritative combat stance changes.
	bool StartAimInput();

	// Records the aim release and requests server-authoritative combat stance changes.
	bool StopAimInput();

	// Flushes combat state during death handling and returns whether fire input changed.
	bool HandleOwnerDeath();

private:
	// Resolves the typed owning character helper.
	APlayerCharacter* GetOwningPlayerCharacter() const;

	// Resolves the owning character's PlayerState-hosted ASC.
	UAbilitySystemComponent* GetOwningAbilitySystemComponent() const;

	// Resolves the component that owns the equipped weapon and equip/drop logic.
	UShooterWeaponEquipmentComponent* GetOwningWeaponEquipmentComponent() const;

	// Resolves the component that owns local pickup targeting and view traces.
	UShooterWeaponInteractionComponent* GetOwningWeaponInteractionComponent() const;

	// Activates the startup fire ability on the authority path.
	void HandleFireInputPressed();

	// Cancels the fire ability on the authority path.
	void HandleFireInputReleased();

	// Updates the replicated aim state and notifies the owner character presentation layer.
	void SetAimInputPressed(bool bNewIsAiming);

	// Applies owner-facing changes when the replicated aim state changes.
	UFUNCTION()
	void OnRep_IsAiming();

	// Sends the start-fire request to the authority path.
	UFUNCTION(Server, Reliable)
	void ServerStartFire();

	// Sends the stop-fire request to the authority path.
	UFUNCTION(Server, Reliable)
	void ServerStopFire();

	// Sends the start-aim request to the authority path.
	UFUNCTION(Server, Reliable)
	void ServerStartAim();

	// Sends the stop-aim request to the authority path.
	UFUNCTION(Server, Reliable)
	void ServerStopAim();

	// Tracks the last known fire-button state to avoid duplicate requests.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Combat", meta = (AllowPrivateAccess = "true"))
	bool bIsFireInputPressed = false;

	// Tracks the replicated hold-to-aim combat state used by gameplay and animation.
	UPROPERTY(ReplicatedUsing = OnRep_IsAiming, VisibleAnywhere, BlueprintReadOnly, Category = "Player|Combat", meta = (AllowPrivateAccess = "true"))
	bool bIsAiming = false;

	// Blocks combat interactions after death handling has started.
	bool bOwnerDeathHandled = false;
};
