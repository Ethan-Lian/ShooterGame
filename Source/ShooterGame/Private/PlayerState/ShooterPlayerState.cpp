#include "PlayerState/ShooterPlayerState.h"

#include "AbilitySystem/ShooterAbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystem/ShooterGameplayTags.h"
#include "AbilitySystem/Abilities/GA_Death.h"
#include "AbilitySystem/Abilities/GA_FireWeapon.h"
#include "AbilitySystem/Abilities/GA_DropWeapon.h"
#include "AbilitySystem/Abilities/GA_InteractWeapon.h"
#include "AbilitySystem/Abilities/GA_Sprint.h"
#include "AbilitySystem/Attributes/CombatAttributeSet.h"
#include "AbilitySystem/Attributes/MovementAttributeSet.h"
#include "AbilitySystem/Effects/GE_InitializeAttributes.h"
#include "Components/ShooterInventoryComponent.h"
#include "ShooterGame.h"

AShooterPlayerState::AShooterPlayerState()
{
	// PlayerState defaults to a low update rate, which makes replicated attributes feel delayed.
	SetNetUpdateFrequency(100.f);

	AbilitySystemComponent = CreateDefaultSubobject<UShooterAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	CombatAttributeSet = CreateDefaultSubobject<UCombatAttributeSet>(TEXT("CombatAttributeSet"));
	MovementAttributeSet = CreateDefaultSubobject<UMovementAttributeSet>(TEXT("MovementAttributeSet"));
	InventoryComponent = CreateDefaultSubobject<UShooterInventoryComponent>(TEXT("InventoryComponent"));

	FireAbilityClass = UGA_FireWeapon::StaticClass();
	DeathAbilityClass = UGA_Death::StaticClass();
	SprintAbilityClass = UGA_Sprint::StaticClass();
	InteractWeaponAbilityClass = UGA_InteractWeapon::StaticClass();
	DropWeaponAbilityClass = UGA_DropWeapon::StaticClass();
	InitializeAttributesEffectClass = UGE_InitializeAttributes::StaticClass();
}

UAbilitySystemComponent* AShooterPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UCombatAttributeSet* AShooterPlayerState::GetCombatAttributeSet() const
{
	return CombatAttributeSet;
}

UMovementAttributeSet* AShooterPlayerState::GetMovementAttributeSet() const
{
	return MovementAttributeSet;
}

UShooterInventoryComponent* AShooterPlayerState::GetInventoryComponent() const
{
	return InventoryComponent;
}

void AShooterPlayerState::InitializeAbilitySystem(AActor* InAvatarActor)
{
	if (AbilitySystemComponent == nullptr) return;

	AbilitySystemComponent->InitAbilityActorInfo(this, InAvatarActor);

	if (HasAuthority())
	{
		GrantStartupAbilitiesIfNeeded();
		ApplyStartupEffectsIfNeeded();
	}
}

void AShooterPlayerState::ResetCombatStateForRespawn()
{
	if (!HasAuthority() || AbilitySystemComponent == nullptr)
	{
		return;
	}

	AbilitySystemComponent->CancelAllAbilities();

	FGameplayTagContainer DeadStateTags;
	DeadStateTags.AddTag(TAG_State_Dead);
	DeadStateTags.AddTag(TAG_State_Movement_Sprinting);
	AbilitySystemComponent->RemoveActiveEffectsWithGrantedTags(DeadStateTags);
	AbilitySystemComponent->SetLooseGameplayTagCount(TAG_State_Dead, 0, EGameplayTagReplicationState::CountToOwner);
	AbilitySystemComponent->SetLooseGameplayTagCount(TAG_State_Movement_Sprinting, 0, EGameplayTagReplicationState::CountToOwner);

	ApplyStartupAttributes();
}

void AShooterPlayerState::GrantStartupAbilitiesIfNeeded()
{
	if (bStartupAbilitiesGranted || AbilitySystemComponent == nullptr)
	{
		return;
	}

	if (FireAbilityClass != nullptr)
	{
		FGameplayAbilitySpec FireAbilitySpec(FireAbilityClass, 1, INDEX_NONE, this);
		FireAbilitySpec.GetDynamicSpecSourceTags().AddTag(TAG_Input_Fire);
		AbilitySystemComponent->GiveAbility(FireAbilitySpec);
	}

	if (DeathAbilityClass != nullptr)
	{
		AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(DeathAbilityClass, 1, INDEX_NONE, this));
	}

	if (SprintAbilityClass != nullptr)
	{
		FGameplayAbilitySpec SprintAbilitySpec(SprintAbilityClass, 1, INDEX_NONE, this);
		SprintAbilitySpec.GetDynamicSpecSourceTags().AddTag(TAG_Input_Sprint);
		AbilitySystemComponent->GiveAbility(SprintAbilitySpec);
	}

	if (InteractWeaponAbilityClass != nullptr)
	{
		FGameplayAbilitySpec InteractAbilitySpec(InteractWeaponAbilityClass, 3, INDEX_NONE, this);
		InteractAbilitySpec.GetDynamicSpecSourceTags().AddTag(TAG_Input_Interact);
		AbilitySystemComponent->GiveAbility(InteractAbilitySpec);
	}

	if (DropWeaponAbilityClass != nullptr)
	{
		FGameplayAbilitySpec DropAbilitySpec(DropWeaponAbilityClass, 4, INDEX_NONE, this);
		DropAbilitySpec.GetDynamicSpecSourceTags().AddTag(TAG_Input_Drop);
		AbilitySystemComponent->GiveAbility(DropAbilitySpec);
	}

	bStartupAbilitiesGranted = true;
}

void AShooterPlayerState::ApplyStartupEffectsIfNeeded()
{
	if (bStartupEffectsApplied || AbilitySystemComponent == nullptr || InitializeAttributesEffectClass == nullptr) return;

	ApplyStartupAttributes();
	bStartupEffectsApplied = true;
}

void AShooterPlayerState::ApplyStartupAttributes()
{
	if (AbilitySystemComponent == nullptr || InitializeAttributesEffectClass == nullptr)
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	const FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(
		InitializeAttributesEffectClass,
		1.f,
		EffectContext);

	if (!SpecHandle.IsValid())
	{
		UE_LOG(LogShooterGame, Warning, TEXT("%s failed to create startup attribute spec."), *GetName());
		return;
	}

	AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}
