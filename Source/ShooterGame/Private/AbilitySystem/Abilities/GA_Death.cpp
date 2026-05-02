#include "AbilitySystem/Abilities/GA_Death.h"

#include "AbilitySystem/ShooterGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Effects/GE_DeathState.h"
#include "Character/PlayerCharacter.h"
#include "GameMode/ShooterGameMode.h"

UGA_Death::UGA_Death()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	FGameplayTagContainer AbilityTagContainer;
	AbilityTagContainer.AddTag(TAG_Ability_Death);
	SetAssetTags(AbilityTagContainer);

	FAbilityTriggerData DeathTrigger;
	DeathTrigger.TriggerTag = TAG_GameplayEvent_Death;
	DeathTrigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(DeathTrigger);

	DeathStateEffectClass = UGE_DeathState::StaticClass();
}

bool UGA_Death::ShouldAbilityRespondToEvent(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayEventData* Payload) const
{
	const UAbilitySystemComponent* AbilitySystemComponent = ActorInfo != nullptr ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	return AbilitySystemComponent != nullptr && !AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Dead);
}

void UGA_Death::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo();
	APlayerCharacter* DeadCharacter = Cast<APlayerCharacter>(GetAvatarActorFromActorInfo());
	if (AbilitySystemComponent == nullptr || DeadCharacter == nullptr || DeathStateEffectClass == nullptr)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FGameplayTagContainer DeathAbilityTags;
	DeathAbilityTags.AddTag(TAG_Ability_Death);
	AbilitySystemComponent->CancelAbilities(nullptr, &DeathAbilityTags, this);

	FGameplayEffectContextHandle EffectContext = TriggerEventData != nullptr && TriggerEventData->ContextHandle.IsValid()
		? TriggerEventData->ContextHandle
		: AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	const FGameplayEffectSpecHandle DeathStateSpecHandle = AbilitySystemComponent->MakeOutgoingSpec(
		DeathStateEffectClass,
		GetAbilityLevel(Handle, ActorInfo),
		EffectContext);
	if (DeathStateSpecHandle.IsValid())
	{
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*DeathStateSpecHandle.Data.Get());
	}

	if (UWorld* World = GetWorld())
	{
		if (AShooterGameMode* ShooterGameMode = World->GetAuthGameMode<AShooterGameMode>())
		{
			ShooterGameMode->RequestPlayerRespawn(Cast<AController>(DeadCharacter->GetController()), DeadCharacter);
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
