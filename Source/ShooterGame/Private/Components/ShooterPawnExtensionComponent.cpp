#include "Components/ShooterPawnExtensionComponent.h"

#include "AbilitySystem/ShooterGameplayTags.h"
#include "AbilitySystem/ShooterAbilitySystemComponent.h"
#include "AbilitySystemComponent.h"
#include "Character/PlayerCharacter.h"
#include "Components/ShooterCombatComponent.h"
#include "Components/ShooterHealthComponent.h"
#include "Components/ShooterMovementStateComponent.h"
#include "Components/ShooterWeaponEquipmentComponent.h"
#include "GameFramework/PlayerController.h"
#include "HUD/ShooterHUD.h"
#include "PlayerState/ShooterPlayerState.h"
#include "ShooterGame.h"

UShooterPawnExtensionComponent::UShooterPawnExtensionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

APlayerCharacter* UShooterPawnExtensionComponent::GetOwningPawn() const
{
	return Cast<APlayerCharacter>(GetOwner());
}

void UShooterPawnExtensionComponent::CheckDefaultInitialization()
{
	TryInitializePawn();
}

void UShooterPawnExtensionComponent::HandleControllerChanged()
{
	if (UAbilitySystemComponent* AbilitySystemComponent = BoundAbilitySystemComponent.Get())
	{
		if (APlayerCharacter* Pawn = GetOwningPawn(); AbilitySystemComponent->GetAvatarActor() == Pawn)
		{
			if (AbilitySystemComponent->GetOwnerActor() != nullptr)
			{
				AbilitySystemComponent->RefreshAbilityActorInfo();
			}
			else
			{
				UninitializePawn();
				return;
			}
		}
	}

	CheckDefaultInitialization();
}

void UShooterPawnExtensionComponent::TryInitializePawn()
{
	APlayerCharacter* Pawn = GetOwningPawn();
	if (Pawn == nullptr)
	{
		return;
	}

	AShooterPlayerState* ShooterPlayerState = Pawn->GetPlayerState<AShooterPlayerState>();
	UAbilitySystemComponent* AbilitySystemComponent = ShooterPlayerState != nullptr
		? ShooterPlayerState->GetAbilitySystemComponent()
		: nullptr;

	if (BoundPlayerState.Get() != ShooterPlayerState || BoundAbilitySystemComponent.Get() != AbilitySystemComponent)
	{
		if (BoundPlayerState.IsValid() || BoundAbilitySystemComponent.IsValid())
		{
			UninitializePawn();
		}
	}

	if (ShooterPlayerState == nullptr)
	{
		return;
	}

	if (AbilitySystemComponent == nullptr)
	{
		return;
	}

	if (BoundPlayerState.Get() == ShooterPlayerState
		&& BoundAbilitySystemComponent.Get() == AbilitySystemComponent
		&& AbilitySystemComponent->GetAvatarActor() == Pawn
		&& InitState == EShooterPawnInitState::GameplayReady)
	{
		if (AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Dead))
		{
			Pawn->ApplyDeathPresentation();
		}
		else
		{
			Pawn->RestoreAlivePresentation();
		}

		if (UShooterWeaponEquipmentComponent* EquipmentComponent = Pawn->GetWeaponEquipmentComponent())
		{
			EquipmentComponent->RefreshEquipmentForPawnReady();
		}

		return;
	}

	SetInitState(EShooterPawnInitState::PlayerStateReady);

	if (AActor* ExistingAvatar = AbilitySystemComponent->GetAvatarActor())
	{
		if (ExistingAvatar != Pawn)
		{
			if (APlayerCharacter* ExistingPlayerCharacter = Cast<APlayerCharacter>(ExistingAvatar))
			{
				if (UShooterPawnExtensionComponent* ExistingExtension = ExistingPlayerCharacter->GetPawnExtensionComponent())
				{
					ExistingExtension->UninitializePawn();
				}
				else if (AbilitySystemComponent->GetOwnerActor() != nullptr)
				{
					UE_LOG(LogShooterGame, Warning, TEXT("ASC %s had existing avatar %s without a ShooterPawnExtensionComponent."), *GetNameSafe(AbilitySystemComponent), *GetNameSafe(ExistingAvatar));
					AbilitySystemComponent->SetAvatarActor(nullptr);
				}
			}
			else if (AbilitySystemComponent->GetOwnerActor() != nullptr)
			{
				UE_LOG(LogShooterGame, Warning, TEXT("ASC %s had non-Shooter avatar %s; detaching it before rebinding."), *GetNameSafe(AbilitySystemComponent), *GetNameSafe(ExistingAvatar));
				AbilitySystemComponent->SetAvatarActor(nullptr);
			}
		}
	}

	if (AbilitySystemComponent->GetAvatarActor() != Pawn)
	{
		ShooterPlayerState->InitializeAbilitySystem(Pawn);
	}

	if (AbilitySystemComponent->GetAvatarActor() != Pawn)
	{
		UE_LOG(LogShooterGame, Warning, TEXT("Failed to initialize ASC %s with Pawn %s as avatar."), *GetNameSafe(AbilitySystemComponent), *GetNameSafe(Pawn));
		return;
	}

	BoundPlayerState = ShooterPlayerState;
	BoundAbilitySystemComponent = AbilitySystemComponent;
	SetInitState(EShooterPawnInitState::ASCReady);

	BindPawnComponents(AbilitySystemComponent);
	SetInitState(EShooterPawnInitState::ComponentBindingsReady);

	SetInitState(EShooterPawnInitState::GameplayReady);

	if (AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Dead))
	{
		Pawn->ApplyDeathPresentation();
	}
	else
	{
		Pawn->RestoreAlivePresentation();
	}

	if (UShooterWeaponEquipmentComponent* EquipmentComponent = Pawn->GetWeaponEquipmentComponent())
	{
		EquipmentComponent->RefreshEquipmentForPawnReady();
	}

	if (Pawn->IsLocallyControlled())
	{
		if (APlayerController* PlayerController = Cast<APlayerController>(Pawn->GetController()))
		{
			if (AShooterHUD* ShooterHUD = PlayerController->GetHUD<AShooterHUD>())
			{
				ShooterHUD->SetObservedPawn(Pawn);
			}
		}
	}
}

void UShooterPawnExtensionComponent::BindPawnComponents(UAbilitySystemComponent* InAbilitySystemComponent)
{
	APlayerCharacter* Pawn = GetOwningPawn();
	if (Pawn == nullptr || InAbilitySystemComponent == nullptr)
	{
		return;
	}

	if (BoundAbilitySystemComponent.Get() != InAbilitySystemComponent)
	{
		UnbindDeathStateTag();
		BoundAbilitySystemComponent = InAbilitySystemComponent;
	}

	if (UShooterHealthComponent* HealthComponent = Pawn->GetHealthComponent())
	{
		HealthComponent->InitializeWithAbilitySystem(InAbilitySystemComponent);
	}

	if (UShooterMovementStateComponent* MovementComponent = Pawn->GetMovementStateComponent())
	{
		MovementComponent->InitializeWithAbilitySystem(InAbilitySystemComponent);
	}

	BindDeathStateTag(InAbilitySystemComponent);
}

void UShooterPawnExtensionComponent::BindDeathStateTag(UAbilitySystemComponent* InAbilitySystemComponent)
{
	if (BoundAbilitySystemComponent.Get() == InAbilitySystemComponent && DeathStateTagChangedDelegateHandle.IsValid())
	{
		return;
	}

	UnbindDeathStateTag();

	if (InAbilitySystemComponent == nullptr)
	{
		return;
	}

	DeathStateTagChangedDelegateHandle = InAbilitySystemComponent->RegisterGameplayTagEvent(
		TAG_State_Dead,
		EGameplayTagEventType::NewOrRemoved).AddUObject(
		this,
		&UShooterPawnExtensionComponent::HandleDeathStateTagChanged);

	if (InAbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Dead))
	{
		HandleDeathStateTagChanged(TAG_State_Dead, InAbilitySystemComponent->GetTagCount(TAG_State_Dead));
	}
}

void UShooterPawnExtensionComponent::UnbindDeathStateTag()
{
	if (UAbilitySystemComponent* AbilitySystemComponent = BoundAbilitySystemComponent.Get())
	{
		if (DeathStateTagChangedDelegateHandle.IsValid())
		{
			AbilitySystemComponent->RegisterGameplayTagEvent(TAG_State_Dead).Remove(DeathStateTagChangedDelegateHandle);
		}
	}

	DeathStateTagChangedDelegateHandle.Reset();
}

void UShooterPawnExtensionComponent::HandleDeathStateTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (Tag != TAG_State_Dead)
	{
		return;
	}

	APlayerCharacter* Pawn = GetOwningPawn();
	if (Pawn == nullptr)
	{
		return;
	}

	if (NewCount > 0)
	{
		Pawn->ApplyDeathPresentation();
	}
	else if (NewCount == 0)
	{
		Pawn->RestoreAlivePresentation();
		if (IsGameplayReady())
		{
			if (UShooterWeaponEquipmentComponent* EquipmentComponent = Pawn->GetWeaponEquipmentComponent())
			{
				EquipmentComponent->RefreshEquipmentForPawnReady();
			}
		}
	}
}

void UShooterPawnExtensionComponent::UninitializePawn()
{
	APlayerCharacter* Pawn = GetOwningPawn();
	UAbilitySystemComponent* AbilitySystemComponent = BoundAbilitySystemComponent.Get();

	if (AbilitySystemComponent != nullptr && Pawn != nullptr && AbilitySystemComponent->GetAvatarActor() == Pawn)
	{
		// Phase 1 treats all active abilities as Pawn-scoped. Future PlayerState-scoped
		// abilities must opt into a survive-avatar tag and be excluded here.
		AbilitySystemComponent->CancelAbilities();
		if (UShooterAbilitySystemComponent* ShooterAbilitySystemComponent = Cast<UShooterAbilitySystemComponent>(AbilitySystemComponent))
		{
			ShooterAbilitySystemComponent->ClearAbilityInput();
		}
		AbilitySystemComponent->RemoveAllGameplayCues();
	}

	if (Pawn != nullptr)
	{
		if (UShooterCombatComponent* CombatComponent = Pawn->GetCombatComponent())
		{
			CombatComponent->UninitializeForPawn();
		}

		if (UShooterWeaponEquipmentComponent* EquipmentComponent = Pawn->GetWeaponEquipmentComponent())
		{
			EquipmentComponent->UninitializeForPawn();
		}

		if (UShooterMovementStateComponent* MovementComponent = Pawn->GetMovementStateComponent())
		{
			MovementComponent->UninitializeFromAbilitySystem();
		}

		if (UShooterHealthComponent* HealthComponent = Pawn->GetHealthComponent())
		{
			HealthComponent->UninitializeFromAbilitySystem();
		}
	}

	UnbindDeathStateTag();

	if (AbilitySystemComponent != nullptr && Pawn != nullptr && AbilitySystemComponent->GetAvatarActor() == Pawn)
	{
		if (AbilitySystemComponent->GetOwnerActor() != nullptr)
		{
			AbilitySystemComponent->SetAvatarActor(nullptr);
		}
		else
		{
			AbilitySystemComponent->ClearActorInfo();
		}
	}

	BoundAbilitySystemComponent.Reset();
	BoundPlayerState.Reset();
	SetInitState(EShooterPawnInitState::Spawned);
}

void UShooterPawnExtensionComponent::SetInitState(EShooterPawnInitState NewState)
{
	InitState = NewState;
}
