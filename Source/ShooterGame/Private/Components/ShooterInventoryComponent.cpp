#include "Components/ShooterInventoryComponent.h"

#include "Components/ShooterWeaponEquipmentComponent.h"
#include "GameFramework/Pawn.h"
#include "Interfaces/ShooterEquipmentInterface.h"
#include "Net/UnrealNetwork.h"
#include "PlayerState/ShooterPlayerState.h"
#include "Weapon/ShooterWeaponEquipmentActor.h"
#include "Weapon/ShooterWeaponPickupActor.h"

UShooterInventoryComponent::UShooterInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

bool FWeaponInventoryEntry::IsValid() const
{
	return ItemId != INDEX_NONE
		&& SlotIndex != INDEX_NONE
		&& WeaponDefinition != nullptr
		&& PickupActorClass != nullptr
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

const FWeaponInventoryEntry* UShooterInventoryComponent::GetInventoryEntryBySlotIndex(int32 SlotIndex) const
{
	const int32 EntryIndex = FindEntryArrayIndexBySlotIndex(SlotIndex);
	return InventoryEntries.IsValidIndex(EntryIndex) ? &InventoryEntries[EntryIndex] : nullptr;
}

int32 UShooterInventoryComponent::FindFirstOccupiedSlotIndex() const
{
	int32 BestSlotIndex = INDEX_NONE;
	for (const FWeaponInventoryEntry& Entry : InventoryEntries)
	{
		if (!Entry.IsValid())
		{
			continue;
		}

		if (BestSlotIndex == INDEX_NONE || Entry.SlotIndex < BestSlotIndex)
		{
			BestSlotIndex = Entry.SlotIndex;
		}
	}

	return BestSlotIndex;
}

int32 UShooterInventoryComponent::FindNextOccupiedSlotIndex(int32 CurrentSlotIndex, bool bForward) const
{
	if (InventoryEntries.IsEmpty() || MaxWeaponSlots <= 0)
	{
		return INDEX_NONE;
	}

	for (int32 Step = 1; Step <= MaxWeaponSlots; ++Step)
	{
		const int32 CandidateSlot = bForward
			? (CurrentSlotIndex + Step) % MaxWeaponSlots
			: (CurrentSlotIndex - Step + (MaxWeaponSlots * 2)) % MaxWeaponSlots;

		if (GetInventoryEntryBySlotIndex(CandidateSlot) != nullptr)
		{
			return CandidateSlot;
		}
	}

	return INDEX_NONE;
}

bool UShooterInventoryComponent::AddWeaponFromPickup(AShooterWeaponPickupActor* PickupWeapon, int32& OutItemId, int32& OutSlotIndex)
{
	OutItemId = INDEX_NONE;
	OutSlotIndex = INDEX_NONE;

	if (PickupWeapon == nullptr || GetOwner() == nullptr || !GetOwner()->HasAuthority())
	{
		return false;
	}

	const int32 FreeSlotIndex = FindFirstFreeSlotIndex();
	if (FreeSlotIndex == INDEX_NONE)
	{
		return false;
	}

	FWeaponInventoryEntry NewEntry;
	if (!BuildInventoryEntryFromPickup(PickupWeapon, FreeSlotIndex, NewEntry))
	{
		return false;
	}

	NewEntry.ItemId = NextItemId++;
	InventoryEntries.Add(NewEntry);

	OutItemId = NewEntry.ItemId;
	OutSlotIndex = NewEntry.SlotIndex;
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

FWeaponInventoryEntry* UShooterInventoryComponent::GetMutableInventoryEntryByItemId(int32 ItemId)
{
	const int32 EntryIndex = FindEntryArrayIndexByItemId(ItemId);
	return InventoryEntries.IsValidIndex(EntryIndex) ? &InventoryEntries[EntryIndex] : nullptr;
}

int32 UShooterInventoryComponent::FindEntryArrayIndexByItemId(int32 ItemId) const
{
	return InventoryEntries.IndexOfByPredicate([ItemId](const FWeaponInventoryEntry& Entry)
	{
		return Entry.ItemId == ItemId;
	});
}

int32 UShooterInventoryComponent::FindEntryArrayIndexBySlotIndex(int32 SlotIndex) const
{
	return InventoryEntries.IndexOfByPredicate([SlotIndex](const FWeaponInventoryEntry& Entry)
	{
		return Entry.SlotIndex == SlotIndex;
	});
}

int32 UShooterInventoryComponent::FindFirstFreeSlotIndex() const
{
	for (int32 SlotIndex = 0; SlotIndex < MaxWeaponSlots; ++SlotIndex)
	{
		if (GetInventoryEntryBySlotIndex(SlotIndex) == nullptr)
		{
			return SlotIndex;
		}
	}

	return INDEX_NONE;
}

bool UShooterInventoryComponent::BuildInventoryEntryFromPickup(AShooterWeaponPickupActor* PickupWeapon, int32 SlotIndex, FWeaponInventoryEntry& OutEntry) const
{
	if (PickupWeapon == nullptr || SlotIndex == INDEX_NONE)
	{
		return false;
	}

	const FWeaponPickupData PickupData = PickupWeapon->GetPickupData();
	if (!PickupData.HasValidDefinition())
	{
		return false;
	}

	OutEntry = FWeaponInventoryEntry();
	OutEntry.SlotIndex = SlotIndex;
	OutEntry.WeaponDefinition = PickupData.WeaponDefinition;
	OutEntry.PickupActorClass = PickupWeapon->GetPickupActorClass();
	OutEntry.EquipmentActorClass = PickupWeapon->GetEquipmentActorClass();
	OutEntry.CurrentMagazineAmmo = PickupData.CurrentMagazineAmmo;
	OutEntry.CurrentReserveAmmo = PickupData.CurrentReserveAmmo;

	// ItemId is assigned by AddWeaponFromPickup after the entry skeleton has been built.
	return OutEntry.SlotIndex != INDEX_NONE
		&& OutEntry.WeaponDefinition != nullptr
		&& OutEntry.PickupActorClass != nullptr
		&& OutEntry.EquipmentActorClass != nullptr;
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
