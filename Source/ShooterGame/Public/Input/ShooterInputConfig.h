#pragma once

#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ShooterInputConfig.generated.h"

class UInputAction;
class UInputMappingContext;

USTRUCT(BlueprintType)
struct FShooterTaggedInputAction
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<const UInputAction> InputAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (Categories = "Input"))
	FGameplayTag InputTag;
};

UCLASS(BlueprintType, Const)
class SHOOTERGAME_API UShooterInputConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	// Finds a directly handled pawn/controller input action by gameplay tag.
	const UInputAction* FindNativeInputActionForTag(const FGameplayTag& InputTag, bool bLogIfNotFound = true) const;

	// Finds an ability-routed input action by gameplay tag.
	const UInputAction* FindAbilityInputActionForTag(const FGameplayTag& InputTag, bool bLogIfNotFound = true) const;

	// Mapping contexts pushed onto the local Enhanced Input subsystem.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TArray<TObjectPtr<const UInputMappingContext>> MappingContexts;

	// Actions handled directly by controller/pawn code.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (TitleProperty = "InputAction"))
	TArray<FShooterTaggedInputAction> NativeInputActions;

	// Actions routed through ASC input tags.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (TitleProperty = "InputAction"))
	TArray<FShooterTaggedInputAction> AbilityInputActions;

	// Priority used when adding mapping contexts to the local player subsystem.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	int32 MappingPriority = 0;
};
