#pragma once

#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "CombatAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class SHOOTERGAME_API UCombatAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UCombatAttributeSet();

	// Clamps attributes before the new value becomes authoritative.
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	// Resolves meta damage into persistent health.
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	// Replicates persistent attributes to simulated clients.
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Attributes|Vital")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UCombatAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "Attributes|Vital")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UCombatAttributeSet, MaxHealth)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Meta")
	FGameplayAttributeData IncomingDamage;
	ATTRIBUTE_ACCESSORS(UCombatAttributeSet, IncomingDamage)

protected:
	// Preserves GAS replication callbacks for health.
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldHealth) const;

	// Preserves GAS replication callbacks for max health.
	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth) const;

private:
	// Returns a health value clamped against the current max health.
	float ClampHealthValue(float InHealth) const;
};
