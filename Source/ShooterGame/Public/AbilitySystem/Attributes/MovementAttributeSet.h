#pragma once

#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "MovementAttributeSet.generated.h"

#define MOVEMENT_ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class SHOOTERGAME_API UMovementAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UMovementAttributeSet();

	// Clamps movement attributes before the new value becomes authoritative.
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	// Replicates persistent movement attributes to simulated clients.
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxWalkSpeed, Category = "Attributes|Movement")
	FGameplayAttributeData MaxWalkSpeed;
	MOVEMENT_ATTRIBUTE_ACCESSORS(UMovementAttributeSet, MaxWalkSpeed)

protected:
	// Preserves GAS replication callbacks for max walk speed.
	UFUNCTION()
	void OnRep_MaxWalkSpeed(const FGameplayAttributeData& OldMaxWalkSpeed) const;
};
