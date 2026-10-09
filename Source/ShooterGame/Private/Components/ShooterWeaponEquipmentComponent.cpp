#include "Components/ShooterWeaponEquipmentComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/ShooterGameplayTags.h"
#include "Components/ShooterCombatComponent.h"
#include "Components/ShooterInventoryComponent.h"
#include "Components/ShooterPawnExtensionComponent.h"
#include "Character/PlayerCharacter.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Interfaces/ShooterCombatInterface.h"
#include "Interfaces/ShooterEquipmentInterface.h"
#include "Net/UnrealNetwork.h"
#include "Weapon/ShooterWeaponEquipmentActor.h"
#include "Weapon/ShooterWeaponInstance.h"
#include "Weapon/WeaponDataAsset.h"
#include "UObject/ConstructorHelpers.h"
#include "ShooterGame.h"

UShooterWeaponEquipmentComponent::UShooterWeaponEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	static ConstructorHelpers::FObjectFinder<UWeaponDataAsset> DefaultWeapon(
		TEXT("/Game/ShooterGameContent/DataConfig/DA_Weapon_AK47.DA_Weapon_AK47"));
	if (DefaultWeapon.Succeeded())
	{
		DefaultWeaponDefinition = DefaultWeapon.Object;
	}
}

void UShooterWeaponEquipmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UShooterWeaponEquipmentComponent, EquippedWeapon);
	DOREPLIFETIME_CONDITION(UShooterWeaponEquipmentComponent, EquippedItemId, COND_OwnerOnly);
}

bool UShooterWeaponEquipmentComponent::HandleOwnerDeath()
{
	AActor* OwnerActor = GetOwner();
	if (OwnerActor == nullptr)
	{
		return false;
	}

	if (!OwnerActor->HasAuthority())
	{
		ClearLocalEquippedWeaponPresentation();
		return false;
	}

	const bool bHadWeapon = EquippedWeapon != nullptr || EquippedItemId != INDEX_NONE;
	DestroyEquippedWeaponActor();
	if (UShooterInventoryComponent* InventoryComponent = GetOwningInventoryComponent())
	{
		FWeaponInventoryEntry RemovedEntry;
		InventoryComponent->RemoveWeaponByItemId(EquippedItemId, RemovedEntry);
	}
	EquippedItemId = INDEX_NONE;
	ClearEquippedWeaponInstance();
	return bHadWeapon;
}

void UShooterWeaponEquipmentComponent::HandleInventoryReplicated()
{
	RefreshEquippedWeaponInstance();
}

void UShooterWeaponEquipmentComponent::RefreshEquipmentForPawnReady()
{
	AActor* OwnerActor = GetOwner();
	UShooterInventoryComponent* InventoryComponent = GetOwningInventoryComponent();

	if (IsEquipmentInteractionBlocked())
	{
		return;
	}

	if (OwnerActor != nullptr && OwnerActor->HasAuthority() && EquippedWeapon == nullptr && InventoryComponent != nullptr)
	{
		if (DefaultWeaponDefinition == nullptr)
		{
			UE_LOG(LogShooterGame, Error, TEXT("%s requires a default Hitscan weapon definition."), *GetNameSafe(OwnerActor));
			return;
		}

		const FWeaponInventoryEntry* ExistingEntry = InventoryComponent->GetDefaultWeaponEntry();
		int32 ItemId = ExistingEntry != nullptr ? ExistingEntry->ItemId : INDEX_NONE;
		const bool bNeedsGrant = ItemId == INDEX_NONE;
		if (bNeedsGrant && !InventoryComponent->AddWeaponFromDefinition(DefaultWeaponDefinition, ItemId))
		{
			UE_LOG(LogShooterGame, Error, TEXT("Failed to grant default weapon to %s."), *GetNameSafe(OwnerActor));
			return;
		}

		if (!EquipInventoryItemById(ItemId) && bNeedsGrant)
		{
			FWeaponInventoryEntry RemovedEntry;
			InventoryComponent->RemoveWeaponByItemId(ItemId, RemovedEntry);
			UE_LOG(LogShooterGame, Error, TEXT("Failed to equip default weapon for %s."), *GetNameSafe(OwnerActor));
		}
	}

	if (EquippedWeapon != nullptr)
	{
		if (EquippedWeapon->IsActorBeingDestroyed())
		{
			ClearLocalEquippedWeaponPresentation();
			return;
		}

		if (ACharacter* OwnerCharacter = GetOwningCharacter())
		{
			if (EquippedWeapon != nullptr)
			{
				EquippedWeapon->EnterEquippedState(OwnerCharacter);
			}
		}
	}

	RefreshEquippedWeaponInstance();
}

void UShooterWeaponEquipmentComponent::UninitializeForPawn()
{
	AShooterWeaponEquipmentActor* WeaponToClear = EquippedWeapon;
	ClearLocalEquippedWeaponPresentation(WeaponToClear);

	if (AActor* OwnerActor = GetOwner(); OwnerActor != nullptr && OwnerActor->HasAuthority())
	{
		if (WeaponToClear != nullptr)
		{
			WeaponToClear->Destroy();
		}
		EquippedWeapon = nullptr;
		EquippedItemId = INDEX_NONE;
	}

	ClearEquippedWeaponInstance();
}

ACharacter* UShooterWeaponEquipmentComponent::GetOwningCharacter() const
{
	return Cast<ACharacter>(GetOwner());
}

UShooterCombatComponent* UShooterWeaponEquipmentComponent::GetOwningCombatComponent() const
{
	const IShooterCombatInterface* CombatOwner = Cast<IShooterCombatInterface>(GetOwner());
	return CombatOwner != nullptr ? CombatOwner->GetShooterCombatComponent() : nullptr;
}

UShooterInventoryComponent* UShooterWeaponEquipmentComponent::GetOwningInventoryComponent() const
{
	const IShooterEquipmentInterface* EquipmentOwner = Cast<IShooterEquipmentInterface>(GetOwner());
	return EquipmentOwner != nullptr ? EquipmentOwner->GetShooterInventoryComponent() : nullptr;
}

bool UShooterWeaponEquipmentComponent::IsEquipmentInteractionBlocked() const
{
	if (const UShooterCombatComponent* CombatComponent = GetOwningCombatComponent())
	{
		return CombatComponent->IsWeaponInteractionBlocked();
	}

	const IShooterCombatInterface* CombatOwner = Cast<IShooterCombatInterface>(GetOwner());
	if (CombatOwner == nullptr)
	{
		return true;
	}

	if (const UAbilitySystemComponent* AbilitySystemComponent = CombatOwner->GetShooterAbilitySystemComponent())
	{
		return AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Dead);
	}

	return false;
}

bool UShooterWeaponEquipmentComponent::EquipInventoryItemById(int32 ItemId)
{
	AActor* OwnerActor = GetOwner();
	ACharacter* OwnerCharacter = GetOwningCharacter();
	UShooterInventoryComponent* InventoryComponent = GetOwningInventoryComponent();
	if (OwnerActor == nullptr || !OwnerActor->HasAuthority() || OwnerCharacter == nullptr || InventoryComponent == nullptr)
	{
		return false;
	}

	const FWeaponInventoryEntry* InventoryEntry = InventoryComponent->GetInventoryEntryByItemId(ItemId);
	if (InventoryEntry == nullptr || !InventoryEntry->IsValid())
	{
		return false;
	}

	if (EquippedWeapon != nullptr)
	{
		DestroyEquippedWeaponActor();
	}

	EquippedItemId = InventoryEntry->ItemId;
	EquippedWeapon = SpawnEquippedWeaponActor(*InventoryEntry);
	if (EquippedWeapon == nullptr)
	{
		EquippedItemId = INDEX_NONE;
		ClearEquippedWeaponInstance();
		return false;
	}

	EquippedWeapon->SetPickupData(InventoryEntry->ToPickupData());
	EquippedWeapon->EnterEquippedState(OwnerCharacter);
	RefreshEquippedWeaponInstance();
	return true;
}

AShooterWeaponEquipmentActor* UShooterWeaponEquipmentComponent::SpawnEquippedWeaponActor(const FWeaponInventoryEntry& Entry)
{
	ACharacter* OwnerCharacter = GetOwningCharacter();
	UWorld* World = GetWorld();
	if (OwnerCharacter == nullptr || World == nullptr || !Entry.IsValid())
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = OwnerCharacter;
	SpawnParameters.Instigator = OwnerCharacter;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AShooterWeaponEquipmentActor* SpawnedWeapon = World->SpawnActor<AShooterWeaponEquipmentActor>(
		Entry.EquipmentActorClass,
		OwnerCharacter->GetActorTransform(),
		SpawnParameters);
	if (SpawnedWeapon != nullptr)
	{
		SpawnedWeapon->SetPickupData(Entry.ToPickupData());
	}

	return SpawnedWeapon;
}

void UShooterWeaponEquipmentComponent::DestroyEquippedWeaponActor()
{
	if (EquippedWeapon != nullptr)
	{
		EquippedWeapon->Destroy();
		EquippedWeapon = nullptr;
	}
}

void UShooterWeaponEquipmentComponent::ClearLocalEquippedWeaponPresentation(AShooterWeaponEquipmentActor* WeaponToClear)
{
	AShooterWeaponEquipmentActor* LocalWeaponToClear = WeaponToClear != nullptr ? WeaponToClear : EquippedWeapon.Get();
	if (LocalWeaponToClear != nullptr)
	{
		LocalWeaponToClear->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		LocalWeaponToClear->SetActorHiddenInGame(true);
		LocalWeaponToClear->SetActorEnableCollision(false);
	}

	const bool bClearedCurrentWeapon = WeaponToClear == nullptr || WeaponToClear == EquippedWeapon;
	if (bClearedCurrentWeapon)
	{
		// Replicated fields belong to the server. A client may temporarily unbind
		// after receiving them and must retain them for the next ready callback.
		ClearEquippedWeaponInstance();
	}
}

void UShooterWeaponEquipmentComponent::RefreshEquippedWeaponInstance()
{
	if (APlayerCharacter* Pawn = Cast<APlayerCharacter>(GetOwner()))
	{
		Pawn->RefreshFirstPersonPresentation();
	}
	if (IsEquipmentInteractionBlocked())
	{
		ClearEquippedWeaponInstance();
		return;
	}

	const UShooterInventoryComponent* InventoryComponent = GetOwningInventoryComponent();
	const FWeaponInventoryEntry* InventoryEntry = InventoryComponent != nullptr
		? InventoryComponent->GetInventoryEntryByItemId(EquippedItemId)
		: nullptr;
	if (InventoryEntry == nullptr || !InventoryEntry->IsValid())
	{
		ClearEquippedWeaponInstance();
		return;
	}

	if (EquippedWeaponInstance == nullptr)
	{
		EquippedWeaponInstance = NewObject<UShooterWeaponInstance>(this);
	}

	EquippedWeaponInstance->InitializeFromInventoryEntry(*InventoryEntry, EquippedWeapon);
}

void UShooterWeaponEquipmentComponent::ClearEquippedWeaponInstance()
{
	if (EquippedWeaponInstance != nullptr)
	{
		EquippedWeaponInstance->ClearEquippedWeaponActor();
	}

	EquippedWeaponInstance = nullptr;
}

void UShooterWeaponEquipmentComponent::OnRep_EquippedWeapon(AShooterWeaponEquipmentActor* OldEquippedWeapon)
{
	const APlayerCharacter* OwnerPawn = Cast<APlayerCharacter>(GetOwner());
	if (OldEquippedWeapon != nullptr && OldEquippedWeapon != EquippedWeapon)
	{
		ClearLocalEquippedWeaponPresentation(OldEquippedWeapon);
	}

	if (OwnerPawn == nullptr || OwnerPawn->GetPawnExtensionComponent() == nullptr || !OwnerPawn->GetPawnExtensionComponent()->IsGameplayReady())
	{
		return;
	}

	if (EquippedWeapon == nullptr)
	{
		if (EquippedItemId == INDEX_NONE)
		{
			ClearEquippedWeaponInstance();
		}

		return;
	}

	ACharacter* OwnerCharacter = GetOwningCharacter();
	const IShooterCombatInterface* CombatOwner = Cast<IShooterCombatInterface>(GetOwner());
	const UAbilitySystemComponent* AbilitySystemComponent = CombatOwner != nullptr ? CombatOwner->GetShooterAbilitySystemComponent() : nullptr;
	if (OwnerCharacter == nullptr || (AbilitySystemComponent != nullptr && AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Dead)))
	{
		ClearLocalEquippedWeaponPresentation();
		return;
	}

	EquippedWeapon->EnterEquippedState(OwnerCharacter);
	RefreshEquippedWeaponInstance();
}

void UShooterWeaponEquipmentComponent::OnRep_EquippedItemId()
{
	RefreshEquippedWeaponInstance();
}
