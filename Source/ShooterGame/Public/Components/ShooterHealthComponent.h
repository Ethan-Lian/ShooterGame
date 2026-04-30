#pragma once

#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "ShooterHealthComponent.generated.h"

class UAbilitySystemComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FShooterHealthChangedSignature, float, OldValue, float, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FShooterDeathStartedSignature);

UCLASS(ClassGroup = (ShooterGame), Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent))
class SHOOTERGAME_API UShooterHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UShooterHealthComponent();

	// Binds the component to a live ASC so it can mirror health state. 
	void InitializeWithAbilitySystem(UAbilitySystemComponent* InAbilitySystemComponent);

	// Clears any existing bindings before the ASC changes or the component dies. 
	void UninitializeFromAbilitySystem();

	// Returns the current health value from the bound ASC. 
	UFUNCTION(BlueprintPure, Category = "Shooter|Health")
	float GetCurrentHealth() const;

	// Returns the current max health value from the bound ASC. 
	UFUNCTION(BlueprintPure, Category = "Shooter|Health")
	float GetMaxHealth() const;

	// Returns health normalized into the [0, 1] range. 
	UFUNCTION(BlueprintPure, Category = "Shooter|Health")
	float GetHealthNormalized() const;

	UPROPERTY(BlueprintAssignable, Category = "Shooter|Health")
	FShooterHealthChangedSignature OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Shooter|Health")
	FShooterDeathStartedSignature OnDeathStarted;

protected:
	// Releases delegate handles during teardown. 
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// Processes health change notifications from the bound ASC. 
	void HandleHealthChanged(const FOnAttributeChangeData& ChangeData);

	// Keeps death edge-triggered if max health changes later. 
	void HandleMaxHealthChanged(const FOnAttributeChangeData& ChangeData);

	// Returns whether the current bound ASC is already dead. 
	bool IsCurrentlyDead() const;

	TWeakObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	
	FDelegateHandle HealthChangedHandle;
	
	FDelegateHandle MaxHealthChangedHandle;
	
	bool bDeathEventBroadcast = false;
};
