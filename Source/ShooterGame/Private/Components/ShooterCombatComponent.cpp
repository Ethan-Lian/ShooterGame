#include "Components/ShooterCombatComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/ShooterAbilitySystemComponent.h"
#include "AbilitySystem/ShooterGameplayTags.h"
#include "Components/ShooterWeaponEquipmentComponent.h"
#include "Interfaces/ShooterCombatInterface.h"
#include "Interfaces/ShooterEquipmentInterface.h"
#include "Net/UnrealNetwork.h"
#include "Character/PlayerCharacter.h"
#include "Components/ShooterInventoryComponent.h"
#include "GameFramework/GameStateBase.h"
#include "TimerManager.h"
#include "Engine/World.h"

UShooterCombatComponent::UShooterCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(true);
}

void UShooterCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UShooterCombatComponent, bIsAiming);
	DOREPLIFETIME(UShooterCombatComponent, ReloadStartServerTime);
}

bool UShooterCombatComponent::StartFireInput()
{
	if (bIsFireInputPressed || IsWeaponInteractionBlocked() || IsReloading())
	{
		return false;
	}

	bIsFireInputPressed = true;

	AActor* OwnerActor = GetOwner();
	if (OwnerActor != nullptr && OwnerActor->HasAuthority())
	{
		HandleFireInputPressed();
	}
	else
	{
		ServerStartFire();
	}

	return true;
}

bool UShooterCombatComponent::StopFireInput()
{
	if (!bIsFireInputPressed)
	{
		return false;
	}

	bIsFireInputPressed = false;

	AActor* OwnerActor = GetOwner();
	if (OwnerActor != nullptr && OwnerActor->HasAuthority())
	{
		HandleFireInputReleased();
	}
	else
	{
		ServerStopFire();
	}

	return true;
}

bool UShooterCombatComponent::StartAimInput()
{
	if (bIsAiming || bOwnerDeathHandled || IsReloading())
	{
		return false;
	}

	if (const UAbilitySystemComponent* AbilitySystemComponent = GetOwningAbilitySystemComponent())
	{
		if (AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Dead))
		{
			return false;
		}
	}

	SetAimInputPressed(true);

	AActor* OwnerActor = GetOwner();
	if (OwnerActor != nullptr && !OwnerActor->HasAuthority())
	{
		ServerStartAim();
	}

	return true;
}

bool UShooterCombatComponent::StopAimInput()
{
	if (!bIsAiming)
	{
		return false;
	}

	SetAimInputPressed(false);

	AActor* OwnerActor = GetOwner();
	if (OwnerActor != nullptr && !OwnerActor->HasAuthority())
	{
		ServerStopAim();
	}

	return true;
}

bool UShooterCombatComponent::HandleOwnerDeath()
{
	if (bOwnerDeathHandled)
	{
		return false;
	}

	bOwnerDeathHandled = true;
	CancelReload();

	const bool bFireInputStopped = bIsFireInputPressed;
	bIsFireInputPressed = false;
	SetAimInputPressed(false);

	if (UShooterWeaponEquipmentComponent* EquipmentComponent = GetOwningWeaponEquipmentComponent())
	{
		EquipmentComponent->HandleOwnerDeath();
	}

	return bFireInputStopped;
}

void UShooterCombatComponent::HandleOwnerRespawn()
{
	bOwnerDeathHandled = false;
	bIsFireInputPressed = false;
	SetAimInputPressed(false);
}

void UShooterCombatComponent::UninitializeForPawn()
{
	CancelReload();
	bOwnerDeathHandled = false;
	bIsFireInputPressed = false;
	SetAimInputPressed(false);
}

UAbilitySystemComponent* UShooterCombatComponent::GetOwningAbilitySystemComponent() const
{
	const IShooterCombatInterface* CombatOwner = Cast<IShooterCombatInterface>(GetOwner());
	return CombatOwner != nullptr ? CombatOwner->GetShooterAbilitySystemComponent() : nullptr;
}

UShooterWeaponEquipmentComponent* UShooterCombatComponent::GetOwningWeaponEquipmentComponent() const
{
	const IShooterEquipmentInterface* EquipmentOwner = Cast<IShooterEquipmentInterface>(GetOwner());
	return EquipmentOwner != nullptr ? EquipmentOwner->GetShooterWeaponEquipmentComponent() : nullptr;
}

void UShooterCombatComponent::HandleFireInputPressed()
{
	if (IsWeaponInteractionBlocked() || IsReloading())
	{
		return;
	}
	UShooterAbilitySystemComponent* ShooterASC = Cast<UShooterAbilitySystemComponent>(GetOwningAbilitySystemComponent());
	if (ShooterASC == nullptr || ShooterASC->HasMatchingGameplayTag(TAG_State_Dead))
	{
		return;
	}

	ShooterASC->AbilityInputTagPressed(TAG_Input_Fire);
}

void UShooterCombatComponent::HandleFireInputReleased()
{
	UShooterAbilitySystemComponent* ShooterASC = Cast<UShooterAbilitySystemComponent>(GetOwningAbilitySystemComponent());
	if (ShooterASC == nullptr)
	{
		return;
	}

	ShooterASC->AbilityInputTagReleased(TAG_Input_Fire);
}

void UShooterCombatComponent::SetAimInputPressed(bool bNewIsAiming)
{
	if (bNewIsAiming && (IsWeaponInteractionBlocked() || IsReloading()))
	{
		return;
	}
	if (bIsAiming == bNewIsAiming)
	{
		return;
	}

	bIsAiming = bNewIsAiming;

	if (IShooterCombatInterface* CombatOwner = Cast<IShooterCombatInterface>(GetOwner()))
	{
		CombatOwner->HandleShooterAimStateChanged(bIsAiming);
	}
}

bool UShooterCombatComponent::IsWeaponInteractionBlocked() const
{
	if (bOwnerDeathHandled)
	{
		return true;
	}

	const UAbilitySystemComponent* AbilitySystemComponent = GetOwningAbilitySystemComponent();
	return AbilitySystemComponent != nullptr && AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Dead);
}

void UShooterCombatComponent::OnRep_IsAiming()
{
	if (IShooterCombatInterface* CombatOwner = Cast<IShooterCombatInterface>(GetOwner()))
	{
		CombatOwner->HandleShooterAimStateChanged(bIsAiming);
	}
}

void UShooterCombatComponent::ServerStartFire_Implementation()
{
	HandleFireInputPressed();
}

void UShooterCombatComponent::ServerStopFire_Implementation()
{
	HandleFireInputReleased();
}

void UShooterCombatComponent::ServerStartAim_Implementation()
{
	SetAimInputPressed(true);
}

void UShooterCombatComponent::ServerStopAim_Implementation()
{
	SetAimInputPressed(false);
}

void UShooterCombatComponent::StartReloadInput()
{
	if (IsWeaponInteractionBlocked() || IsReloading())
	{
		return;
	}
	StopFireInput();
	if (GetOwner()->HasAuthority())
	{
		BeginReload();
	}
	else
	{
		ServerReload();
	}
}

void UShooterCombatComponent::ServerReload_Implementation()
{
	BeginReload();
}

void UShooterCombatComponent::BeginReload()
{
	APlayerCharacter* Pawn = Cast<APlayerCharacter>(GetOwner());
	UShooterInventoryComponent* Inventory = Pawn != nullptr ? Pawn->GetInventoryComponent() : nullptr;
	UShooterWeaponEquipmentComponent* Equipment = GetOwningWeaponEquipmentComponent();
	if (IsWeaponInteractionBlocked() || IsReloading() || Inventory == nullptr || Equipment == nullptr
		|| !Inventory->CanReload(Equipment->GetEquippedItemId()) || Pawn->GetReloadDuration() <= 0.f)
	{
		return;
	}

	ReloadItemId = Equipment->GetEquippedItemId();
	ReloadStartServerTime = GetWorld()->GetTimeSeconds();
	HandleFireInputReleased();
	// Ability cancellation can synchronously uninitialize or kill this Pawn.
	if (!IsReloading() || IsWeaponInteractionBlocked())
	{
		return;
	}
	SetAimInputPressed(false);
	GetWorld()->GetTimerManager().SetTimer(ReloadTimer, this, &UShooterCombatComponent::FinishReload, Pawn->GetReloadDuration(), false);
	OnRep_ReloadStartServerTime();
	GetOwner()->ForceNetUpdate();
}

void UShooterCombatComponent::FinishReload()
{
	APlayerCharacter* Pawn = Cast<APlayerCharacter>(GetOwner());
	UShooterInventoryComponent* Inventory = Pawn != nullptr ? Pawn->GetInventoryComponent() : nullptr;
	const UShooterWeaponEquipmentComponent* Equipment = GetOwningWeaponEquipmentComponent();
	if (!IsWeaponInteractionBlocked() && Inventory != nullptr && Equipment != nullptr && Equipment->GetEquippedItemId() == ReloadItemId)
	{
		Inventory->ReloadMagazine(ReloadItemId);
	}
	CancelReload();
}

void UShooterCombatComponent::CancelReload()
{
	if (GetWorld() != nullptr)
	{
		GetWorld()->GetTimerManager().ClearTimer(ReloadTimer);
	}
	ReloadItemId = INDEX_NONE;
	if (GetOwner()->HasAuthority())
	{
		ReloadStartServerTime = -1.f;
		GetOwner()->ForceNetUpdate();
	}
	if (APlayerCharacter* Pawn = Cast<APlayerCharacter>(GetOwner()))
	{
		Pawn->StopFirstPersonReload();
	}
}

void UShooterCombatComponent::OnRep_ReloadStartServerTime()
{
	APlayerCharacter* Pawn = Cast<APlayerCharacter>(GetOwner());
	if (Pawn == nullptr)
	{
		return;
	}
	if (!IsReloading())
	{
		Pawn->StopFirstPersonReload();
		return;
	}
	const AGameStateBase* GameState = GetWorld()->GetGameState();
	const float ServerTime = GameState != nullptr ? GameState->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
	const float Elapsed = FMath::Max(0.f, ServerTime - ReloadStartServerTime);
	if (Elapsed < Pawn->GetReloadDuration())
	{
		Pawn->PlayFirstPersonReload(Elapsed);
	}
}

void UShooterCombatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CancelReload();
	Super::EndPlay(EndPlayReason);
}
