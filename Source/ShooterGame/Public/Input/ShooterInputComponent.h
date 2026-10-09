#pragma once

#include "EnhancedInputComponent.h"
#include "Input/ShooterInputConfig.h"
#include "ShooterInputComponent.generated.h"

class UEnhancedInputLocalPlayerSubsystem;

UCLASS(Config = Input)
class SHOOTERGAME_API UShooterInputComponent : public UEnhancedInputComponent
{
	GENERATED_BODY()

public:
	UShooterInputComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Adds every mapping context declared by the input config to the local player.
	void AddInputMappings(const UShooterInputConfig* InputConfig, UEnhancedInputLocalPlayerSubsystem* InputSubsystem) const;

	template<class UserClass, typename FuncType>
	void BindNativeAction(
		const UShooterInputConfig* InputConfig,
		const FGameplayTag& InputTag,
		ETriggerEvent TriggerEvent,
		UserClass* Object,
		FuncType Func,
		TArray<uint32>& BindHandles,
		bool bLogIfNotFound = true);

	template<class UserClass, typename PressedFuncType, typename ReleasedFuncType>
	void BindAbilityActions(
		const UShooterInputConfig* InputConfig,
		UserClass* Object,
		PressedFuncType PressedFunc,
		ReleasedFuncType ReleasedFunc,
		TArray<uint32>& BindHandles);

	// Removes previously recorded Enhanced Input bindings.
	void RemoveBinds(TArray<uint32>& BindHandles);
};

template<class UserClass, typename FuncType>
void UShooterInputComponent::BindNativeAction(
	const UShooterInputConfig* InputConfig,
	const FGameplayTag& InputTag,
	ETriggerEvent TriggerEvent,
	UserClass* Object,
	FuncType Func,
	TArray<uint32>& BindHandles,
	bool bLogIfNotFound)
{
	check(InputConfig);

	if (const UInputAction* InputAction = InputConfig->FindNativeInputActionForTag(InputTag, bLogIfNotFound))
	{
		BindHandles.Add(BindAction(InputAction, TriggerEvent, Object, Func).GetHandle());
	}
}

template<class UserClass, typename PressedFuncType, typename ReleasedFuncType>
void UShooterInputComponent::BindAbilityActions(
	const UShooterInputConfig* InputConfig,
	UserClass* Object,
	PressedFuncType PressedFunc,
	ReleasedFuncType ReleasedFunc,
	TArray<uint32>& BindHandles)
{
	check(InputConfig);

	for (const FShooterTaggedInputAction& Action : InputConfig->AbilityInputActions)
	{
		if (Action.InputAction == nullptr || !Action.InputTag.IsValid())
		{
			continue;
		}

		if (PressedFunc)
		{
			BindHandles.Add(BindAction(Action.InputAction, ETriggerEvent::Started, Object, PressedFunc, Action.InputTag).GetHandle());
		}

		if (ReleasedFunc)
		{
			BindHandles.Add(BindAction(Action.InputAction, ETriggerEvent::Completed, Object, ReleasedFunc, Action.InputTag).GetHandle());
			BindHandles.Add(BindAction(Action.InputAction, ETriggerEvent::Canceled, Object, ReleasedFunc, Action.InputTag).GetHandle());
		}
	}
}
