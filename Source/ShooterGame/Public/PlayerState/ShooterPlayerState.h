#pragma once

#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "ShooterPlayerState.generated.h"

class UShooterAbilitySystemComponent;
class UCombatAttributeSet;
class UGameplayAbility;
class UGameplayEffect;
class UMovementAttributeSet;
class UShooterInventoryComponent;

UCLASS()
class SHOOTERGAME_API AShooterPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AShooterPlayerState();

	// Returns the authoritative combat ASC for this player.
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// Returns the project's combat attributes.
	UCombatAttributeSet* GetCombatAttributeSet() const;

	// Returns the project's movement attributes.
	UMovementAttributeSet* GetMovementAttributeSet() const;

	// Returns the long-lived logical weapon inventory owned by this player.
	UShooterInventoryComponent* GetInventoryComponent() const;

	// Initializes avatar linkage and one-time startup combat state.
	void InitializeAbilitySystem(AActor* InAvatarActor);

	// Clears death state and restores combat attributes before GameMode creates the next pawn.
	void ResetCombatStateForRespawn();

protected:
	// Lets Blueprint children override the startup fire ability if needed.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shooter|Abilities")
	TSubclassOf<UGameplayAbility> FireAbilityClass;

	// Lets Blueprint children override the death flow ability if needed.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shooter|Abilities")
	TSubclassOf<UGameplayAbility> DeathAbilityClass;

	// Lets Blueprint children override the sprint movement ability if needed.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shooter|Abilities")
	TSubclassOf<UGameplayAbility> SprintAbilityClass;

	// Lets Blueprint children override the startup attribute initializer.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shooter|Abilities")
	TSubclassOf<UGameplayEffect> InitializeAttributesEffectClass;

	// Lets Blueprint children override the interact ability triggered by Input.Interact.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shooter|Abilities")
	TSubclassOf<UGameplayAbility> InteractWeaponAbilityClass;

	// Lets Blueprint children override the drop ability triggered by Input.Drop.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shooter|Abilities")
	TSubclassOf<UGameplayAbility> DropWeaponAbilityClass;

private:
	// Grants startup abilities once on the authority path.
	void GrantStartupAbilitiesIfNeeded();

	// Applies startup attributes once on the authority path.
	void ApplyStartupEffectsIfNeeded();

	// Applies the attribute initializer regardless of previous startup state.
	void ApplyStartupAttributes();

	UPROPERTY(VisibleAnywhere, Category = "Shooter|Abilities")
	TObjectPtr<UShooterAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, Category = "Shooter|Abilities")
	TObjectPtr<UCombatAttributeSet> CombatAttributeSet;

	UPROPERTY(VisibleAnywhere, Category = "Shooter|Abilities")
	TObjectPtr<UMovementAttributeSet> MovementAttributeSet;

	// The long-lived weapon inventory component, which persists in the player state. It will be used by the future equipment system to manage the player's weapons and items.
	// 长期武器库存组件,在玩家状态中持久存在.后续装备系统会使用它来管理玩家的武器和物品.
	UPROPERTY(VisibleAnywhere, Category = "Shooter|Inventory")
	TObjectPtr<UShooterInventoryComponent> InventoryComponent;

	bool bStartupAbilitiesGranted = false;
	
	bool bStartupEffectsApplied = false;
};
