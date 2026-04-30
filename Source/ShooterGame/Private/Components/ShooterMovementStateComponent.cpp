#include "Components/ShooterMovementStateComponent.h"

#include "AbilitySystem/Attributes/MovementAttributeSet.h"
#include "AbilitySystem/ShooterGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Character/PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

UShooterMovementStateComponent::UShooterMovementStateComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(true);
}

void UShooterMovementStateComponent::HandleMoveInput(const FVector2D& InputValue)
{
	LastMoveInput = InputValue;

	if (bSprintInputPressed)
	{
		RefreshSprintAbilityState();

		if (!IsOwnerAuthority())
		{
			ServerUpdateSprintMoveInput(LastMoveInput);
		}
	}
}

bool UShooterMovementStateComponent::StartSprintInput()
{
	if (bSprintInputPressed)
	{
		return false;
	}

	bSprintInputPressed = true;
	const bool bSprintStateChanged = RefreshSprintAbilityState();

	if (!IsOwnerAuthority())
	{
		ServerSetSprintInputPressed(true, LastMoveInput);
	}

	return bSprintStateChanged;
}

bool UShooterMovementStateComponent::StopSprintInput()
{
	if (!bSprintInputPressed)
	{
		return false;
	}

	bSprintInputPressed = false;
	const bool bSprintStateChanged = CancelSprintAbility();

	if (!IsOwnerAuthority())
	{
		ServerSetSprintInputPressed(false, LastMoveInput);
	}

	return bSprintStateChanged;
}

void UShooterMovementStateComponent::HandleOwnerDeath()
{
	bSprintInputPressed = false;
	LastMoveInput = FVector2D::ZeroVector;
	CancelSprintAbility();
}

void UShooterMovementStateComponent::InitializeWithAbilitySystem(UAbilitySystemComponent* InAbilitySystemComponent)
{
	if (AbilitySystemComponent == InAbilitySystemComponent)
	{
		return;
	}

	if (AbilitySystemComponent != nullptr && MaxWalkSpeedChangedDelegateHandle.IsValid())
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			UMovementAttributeSet::GetMaxWalkSpeedAttribute()).Remove(MaxWalkSpeedChangedDelegateHandle);
		MaxWalkSpeedChangedDelegateHandle.Reset();
	}

	AbilitySystemComponent = InAbilitySystemComponent;
	if (AbilitySystemComponent == nullptr)
	{
		return;
	}

	MaxWalkSpeedChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		UMovementAttributeSet::GetMaxWalkSpeedAttribute()).AddUObject(
			this,
			&UShooterMovementStateComponent::HandleMaxWalkSpeedChanged);

	ApplyMaxWalkSpeed(AbilitySystemComponent->GetNumericAttribute(UMovementAttributeSet::GetMaxWalkSpeedAttribute()));
}

bool UShooterMovementStateComponent::IsSprintDirectionAllowed() const
{
	constexpr float ForwardThreshold = 0.5f;
	constexpr float SidewaysTolerance = 0.25f;

	return LastMoveInput.Y > ForwardThreshold && FMath::Abs(LastMoveInput.X) <= SidewaysTolerance;
}

bool UShooterMovementStateComponent::IsSprinting() const
{
	return AbilitySystemComponent != nullptr
		&& AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Movement_Sprinting);
}

APlayerCharacter* UShooterMovementStateComponent::GetOwningPlayerCharacter() const
{
	return Cast<APlayerCharacter>(GetOwner());
}

bool UShooterMovementStateComponent::IsOwnerAuthority() const
{
	const AActor* OwnerActor = GetOwner();
	return OwnerActor != nullptr && OwnerActor->HasAuthority();
}

void UShooterMovementStateComponent::ApplyMaxWalkSpeed(float NewMaxWalkSpeed) const
{
	const APlayerCharacter* OwnerCharacter = GetOwningPlayerCharacter();
	UCharacterMovementComponent* CharacterMovement = OwnerCharacter != nullptr
		? OwnerCharacter->GetCharacterMovement()
		: nullptr;
	if (CharacterMovement == nullptr)
	{
		return;
	}

	CharacterMovement->MaxWalkSpeed = FMath::Max(0.f, NewMaxWalkSpeed);
}

void UShooterMovementStateComponent::HandleMaxWalkSpeedChanged(const FOnAttributeChangeData& ChangeData)
{
	ApplyMaxWalkSpeed(ChangeData.NewValue);
}

bool UShooterMovementStateComponent::RefreshSprintAbilityState()
{
	if (!bSprintInputPressed || !IsSprintDirectionAllowed())
	{
		return CancelSprintAbility();
	}

	if (IsSprinting())
	{
		return false;
	}

	return ActivateSprintAbility();
}

bool UShooterMovementStateComponent::ActivateSprintAbility() const
{
	if (AbilitySystemComponent == nullptr)
	{
		return false;
	}

	FGameplayTagContainer SprintAbilityTags;
	SprintAbilityTags.AddTag(TAG_Ability_Movement_Sprint);
	return AbilitySystemComponent->TryActivateAbilitiesByTag(SprintAbilityTags, true);
}

bool UShooterMovementStateComponent::CancelSprintAbility() const
{
	if (AbilitySystemComponent == nullptr)
	{
		return false;
	}

	FGameplayTagContainer SprintAbilityTags;
	SprintAbilityTags.AddTag(TAG_Ability_Movement_Sprint);
	AbilitySystemComponent->CancelAbilities(&SprintAbilityTags, nullptr, nullptr);
	return true;
}

void UShooterMovementStateComponent::ServerSetSprintInputPressed_Implementation(bool bNewSprintInputPressed, FVector2D MoveInput)
{
	bSprintInputPressed = bNewSprintInputPressed;
	LastMoveInput = MoveInput;

	if (bSprintInputPressed)
	{
		RefreshSprintAbilityState();
	}
	else
	{
		CancelSprintAbility();
	}
}

void UShooterMovementStateComponent::ServerUpdateSprintMoveInput_Implementation(FVector2D MoveInput)
{
	LastMoveInput = MoveInput;

	if (bSprintInputPressed)
	{
		RefreshSprintAbilityState();
	}
}
