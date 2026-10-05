#include "Components/ShooterInventoryComponent.h"

#include "Components/ShooterWeaponEquipmentComponent.h"
#include "GameFramework/Pawn.h"
#include "Interfaces/ShooterEquipmentInterface.h"
#include "Net/UnrealNetwork.h"
#include "PlayerState/ShooterPlayerState.h"
#include "Weapon/ShooterWeaponEquipmentActor.h"

UShooterInventoryComponent::UShooterInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

bool FWeaponInventoryEntry::IsValid() const
{
	return ItemId != INDEX_NONE
		&& WeaponDefinition != nullptr
		&& EquipmentActorClass != nullptr;
}

FWeaponPickupData FWeaponInventoryEntry::ToPickupData() const
{
	FWeaponPickupData PickupData;
	PickupData.WeaponDefinition = WeaponDefinition;
	PickupData.CurrentMagazineAmmo = CurrentMagazineAmmo;
	PickupData.CurrentReserveAmmo = CurrentReserveAmmo;
	return PickupData;
}

void UShooterInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UShooterInventoryComponent, InventoryEntries, COND_OwnerOnly);
}

const FWeaponInventoryEntry* UShooterInventoryComponent::GetInventoryEntryByItemId(int32 ItemId) const
{
	const int32 EntryIndex = FindEntryArrayIndexByItemId(ItemId);
	return InventoryEntries.IsValidIndex(EntryIndex) ? &InventoryEntries[EntryIndex] : nullptr;
}

const FWeaponInventoryEntry* UShooterInventoryComponent::GetDefaultWeaponEntry() const
{
	return InventoryEntries.IsEmpty() ? nullptr : &InventoryEntries[0];
}

bool UShooterInventoryComponent::AddWeaponFromDefinition(UWeaponDataAsset* WeaponDefinition, int32& OutItemId)
{
	OutItemId = INDEX_NONE;
	if (GetOwner() == nullptr || !GetOwner()->HasAuthority() || WeaponDefinition == nullptr
		|| WeaponDefinition->EquipmentActorClass == nullptr || !InventoryEntries.IsEmpty())
	{
		return false;
	}

	FWeaponInventoryEntry NewEntry;
	NewEntry.ItemId = NextItemId++;
	NewEntry.WeaponDefinition = WeaponDefinition;
	NewEntry.EquipmentActorClass = WeaponDefinition->EquipmentActorClass;
	NewEntry.CurrentMagazineAmmo = WeaponDefinition->AmmoConfig.MagazineSize;
	NewEntry.CurrentReserveAmmo = WeaponDefinition->AmmoConfig.InitialReserveAmmo;
	InventoryEntries.Add(NewEntry);
	OutItemId = NewEntry.ItemId;
	return true;
}

bool UShooterInventoryComponent::RemoveWeaponByItemId(int32 ItemId, FWeaponInventoryEntry& OutRemovedEntry)
{
	if (GetOwner() == nullptr || !GetOwner()->HasAuthority())
	{
		return false;
	}

	const int32 EntryIndex = FindEntryArrayIndexByItemId(ItemId);
	if (!InventoryEntries.IsValidIndex(EntryIndex))
	{
		return false;
	}

	OutRemovedEntry = InventoryEntries[EntryIndex];
	InventoryEntries.RemoveAt(EntryIndex);
	return true;
}

int32 UShooterInventoryComponent::FindEntryArrayIndexByItemId(int32 ItemId) const
{
	return InventoryEntries.IndexOfByPredicate([ItemId](const FWeaponInventoryEntry& Entry)
	{
		return Entry.ItemId == ItemId;
	});
}

void UShooterInventoryComponent::OnRep_InventoryEntries()
{
	const AShooterPlayerState* OwnerPlayerState = Cast<AShooterPlayerState>(GetOwner());
	APawn* OwnerPawn = OwnerPlayerState != nullptr ? OwnerPlayerState->GetPawn() : nullptr;
	const IShooterEquipmentInterface* EquipmentOwner = Cast<IShooterEquipmentInterface>(OwnerPawn);
	if (EquipmentOwner == nullptr)
	{
		return;
	}

	if (UShooterWeaponEquipmentComponent* EquipmentComponent = EquipmentOwner->GetShooterWeaponEquipmentComponent())
	{
		EquipmentComponent->HandleInventoryReplicated();
	}
}
