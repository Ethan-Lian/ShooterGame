#pragma once

#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "ShooterPawnExtensionComponent.generated.h"

class APlayerCharacter;
class AShooterPlayerState;
class UAbilitySystemComponent;

UENUM(BlueprintType)
enum class EShooterPawnInitState : uint8
{
	Spawned,
	PlayerStateReady,
	ASCReady,
	ComponentBindingsReady,
	GameplayReady
};

UCLASS(ClassGroup = (ShooterGame), BlueprintType, meta = (BlueprintSpawnableComponent))
class SHOOTERGAME_API UShooterPawnExtensionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UShooterPawnExtensionComponent();

	// Re-checks all Pawn lifecycle prerequisites. Safe to call from multiple engine callbacks.
	void CheckDefaultInitialization();

	// Removes Pawn-scoped bindings and presentation without clearing PlayerState-owned ASC state.
	void UninitializePawn();

	// Refreshes the ASC actor info after the Pawn's Controller changes.
	void HandleControllerChanged();

	UFUNCTION(BlueprintPure, Category = "Shooter|PawnLifecycle")
	EShooterPawnInitState GetInitState() const { return InitState; }

	UFUNCTION(BlueprintPure, Category = "Shooter|PawnLifecycle")
	bool IsGameplayReady() const { return InitState == EShooterPawnInitState::GameplayReady; }

private:
	APlayerCharacter* GetOwningPawn() const;

	void TryInitializePawn();
	void BindPawnComponents(UAbilitySystemComponent* InAbilitySystemComponent);
	void BindDeathStateTag(UAbilitySystemComponent* InAbilitySystemComponent);
	void UnbindDeathStateTag();
	void SetInitState(EShooterPawnInitState NewState);

	void HandleDeathStateTagChanged(const FGameplayTag Tag, int32 NewCount);

	EShooterPawnInitState InitState = EShooterPawnInitState::Spawned;

	TWeakObjectPtr<AShooterPlayerState> BoundPlayerState;
	TWeakObjectPtr<UAbilitySystemComponent> BoundAbilitySystemComponent;

	FDelegateHandle DeathStateTagChangedDelegateHandle;
};
