#include "AbilitySystem/Attributes/MovementAttributeSet.h"

#include "Net/UnrealNetwork.h"

UMovementAttributeSet::UMovementAttributeSet()
{
	InitMaxWalkSpeed(600.f);
}

void UMovementAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetMaxWalkSpeedAttribute())
	{
		NewValue = FMath::Max(0.f, NewValue);
	}
}

void UMovementAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UMovementAttributeSet, MaxWalkSpeed, COND_None, REPNOTIFY_Always);
}

void UMovementAttributeSet::OnRep_MaxWalkSpeed(const FGameplayAttributeData& OldMaxWalkSpeed) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMovementAttributeSet, MaxWalkSpeed, OldMaxWalkSpeed);
}
