#include "Input/ShooterInputConfig.h"

#include "InputAction.h"
#include "ShooterGame.h"

const UInputAction* UShooterInputConfig::FindNativeInputActionForTag(const FGameplayTag& InputTag, bool bLogIfNotFound) const
{
	for (const FShooterTaggedInputAction& Action : NativeInputActions)
	{
		if (Action.InputAction != nullptr && Action.InputTag == InputTag)
		{
			return Action.InputAction;
		}
	}

	if (bLogIfNotFound)
	{
		UE_LOG(LogShooterGame, Warning, TEXT("InputConfig %s is missing native action for tag %s."), *GetNameSafe(this), *InputTag.ToString());
	}

	return nullptr;
}

const UInputAction* UShooterInputConfig::FindAbilityInputActionForTag(const FGameplayTag& InputTag, bool bLogIfNotFound) const
{
	for (const FShooterTaggedInputAction& Action : AbilityInputActions)
	{
		if (Action.InputAction != nullptr && Action.InputTag == InputTag)
		{
			return Action.InputAction;
		}
	}

	if (bLogIfNotFound)
	{
		UE_LOG(LogShooterGame, Warning, TEXT("InputConfig %s is missing ability action for tag %s."), *GetNameSafe(this), *InputTag.ToString());
	}

	return nullptr;
}
