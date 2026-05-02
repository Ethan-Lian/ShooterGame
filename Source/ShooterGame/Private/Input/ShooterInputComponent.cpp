#include "Input/ShooterInputComponent.h"

#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"

UShooterInputComponent::UShooterInputComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UShooterInputComponent::AddInputMappings(
	const UShooterInputConfig* InputConfig,
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem) const
{
	if (InputConfig == nullptr || InputSubsystem == nullptr)
	{
		return;
	}

	for (const UInputMappingContext* MappingContext : InputConfig->MappingContexts)
	{
		if (MappingContext != nullptr)
		{
			InputSubsystem->AddMappingContext(MappingContext, InputConfig->MappingPriority);
		}
	}
}

void UShooterInputComponent::RemoveBinds(TArray<uint32>& BindHandles)
{
	for (const uint32 BindHandle : BindHandles)
	{
		RemoveBindingByHandle(BindHandle);
	}

	BindHandles.Reset();
}
