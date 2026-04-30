#include "Components/ShooterHealthComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/CombatAttributeSet.h"

UShooterHealthComponent::UShooterHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UShooterHealthComponent::InitializeWithAbilitySystem(UAbilitySystemComponent* InAbilitySystemComponent)
{
	if (AbilitySystemComponent.Get() == InAbilitySystemComponent)
	{
		return;
	}

	UninitializeFromAbilitySystem();
	AbilitySystemComponent = InAbilitySystemComponent;

	if (InAbilitySystemComponent == nullptr)
	{
		return;
	}

	HealthChangedHandle = InAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		UCombatAttributeSet::GetHealthAttribute()).AddUObject(this, &UShooterHealthComponent::HandleHealthChanged);

	MaxHealthChangedHandle = InAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		UCombatAttributeSet::GetMaxHealthAttribute()).AddUObject(this, &UShooterHealthComponent::HandleMaxHealthChanged);

	bDeathEventBroadcast = IsCurrentlyDead();
}

void UShooterHealthComponent::UninitializeFromAbilitySystem()
{
	if (UAbilitySystemComponent* BoundAbilitySystem = AbilitySystemComponent.Get())
	{
		if (HealthChangedHandle.IsValid())
		{
			BoundAbilitySystem->GetGameplayAttributeValueChangeDelegate(
				UCombatAttributeSet::GetHealthAttribute()).Remove(HealthChangedHandle);
			HealthChangedHandle.Reset();
		}

		if (MaxHealthChangedHandle.IsValid())
		{
			BoundAbilitySystem->GetGameplayAttributeValueChangeDelegate(
				UCombatAttributeSet::GetMaxHealthAttribute()).Remove(MaxHealthChangedHandle);
			MaxHealthChangedHandle.Reset();
		}
	}

	AbilitySystemComponent.Reset();
	bDeathEventBroadcast = false;
}

float UShooterHealthComponent::GetCurrentHealth() const
{
	const UAbilitySystemComponent* BoundAbilitySystem = AbilitySystemComponent.Get();
	return BoundAbilitySystem != nullptr
		? BoundAbilitySystem->GetNumericAttribute(UCombatAttributeSet::GetHealthAttribute())
		: 0.f;
}

float UShooterHealthComponent::GetMaxHealth() const
{
	const UAbilitySystemComponent* BoundAbilitySystem = AbilitySystemComponent.Get();
	return BoundAbilitySystem != nullptr
		? BoundAbilitySystem->GetNumericAttribute(UCombatAttributeSet::GetMaxHealthAttribute())
		: 0.f;
}

float UShooterHealthComponent::GetHealthNormalized() const
{
	const float CurrentMaxHealth = GetMaxHealth();
	return CurrentMaxHealth > 0.f ? GetCurrentHealth() / CurrentMaxHealth : 0.f;
}

void UShooterHealthComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UninitializeFromAbilitySystem();
	Super::EndPlay(EndPlayReason);
}

void UShooterHealthComponent::HandleHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	OnHealthChanged.Broadcast(ChangeData.OldValue, ChangeData.NewValue);

	if (!bDeathEventBroadcast && ChangeData.NewValue <= 0.f)
	{
		bDeathEventBroadcast = true;
		OnDeathStarted.Broadcast();
		return;
	}

	if (ChangeData.NewValue > 0.f)
	{
		bDeathEventBroadcast = false;
	}
}

void UShooterHealthComponent::HandleMaxHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	if (ChangeData.NewValue > 0.f && GetCurrentHealth() > 0.f)
	{
		bDeathEventBroadcast = false;
	}
}

bool UShooterHealthComponent::IsCurrentlyDead() const
{
	return GetCurrentHealth() <= 0.f;
}
