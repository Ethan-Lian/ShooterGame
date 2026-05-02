#include "Components/ShooterWeaponEquipmentComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/ShooterAbilitySystemComponent.h"
#include "AbilitySystem/ShooterGameplayTags.h"
#include "Components/ShooterCombatComponent.h"
#include "Components/ShooterInventoryComponent.h"
#include "Components/ShooterWeaponInteractionComponent.h"
#include "CollisionShape.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "Interfaces/ShooterCombatInterface.h"
#include "Interfaces/ShooterEquipmentInterface.h"
#include "Net/UnrealNetwork.h"
#include "Weapon/ShooterWeaponEquipmentActor.h"
#include "Weapon/ShooterWeaponPickupActor.h"
#include "Weapon/ShooterWeaponInstance.h"

namespace
{
	constexpr float DropWallSweepRadius = 18.f;
	constexpr float DropWallClearance = 12.f;
	constexpr float DropGravityZ = -980.f;
	constexpr float DropMaxSimulationTime = 1.5f;
	constexpr float DropSimulationStep = 0.04f;
	constexpr float DropGroundTraceUpOffset = 80.f;
	constexpr float DropGroundTraceDistance = 1200.f;
	constexpr float DropGroundOffset = 8.f;
	constexpr float PickupFallbackValidationRadius = 300.f;
}

UShooterWeaponEquipmentComponent::UShooterWeaponEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UShooterWeaponEquipmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UShooterWeaponEquipmentComponent, EquippedWeapon);
	DOREPLIFETIME_CONDITION(UShooterWeaponEquipmentComponent, EquippedItemId, COND_OwnerOnly);
}

bool UShooterWeaponEquipmentComponent::StartPickupInput()
{
	if (IsEquipmentInteractionBlocked())
	{
		return false;
	}

	UShooterAbilitySystemComponent* ShooterASC = GetOwningShooterAbilitySystemComponent();
	if (ShooterASC != nullptr)
	{
		ShooterASC->AbilityInputTagPressed(TAG_Input_Interact);
	}

	return true;
}

bool UShooterWeaponEquipmentComponent::StartDropInput()
{
	if (EquippedItemId == INDEX_NONE || IsEquipmentInteractionBlocked())
	{
		return false;
	}

	if (UShooterCombatComponent* CombatComponent = GetOwningCombatComponent())
	{
		CombatComponent->StopFireInput();
	}

	UShooterAbilitySystemComponent* ShooterASC = GetOwningShooterAbilitySystemComponent();
	if (ShooterASC != nullptr)
	{
		ShooterASC->AbilityInputTagPressed(TAG_Input_Drop);
	}

	return true;
}

bool UShooterWeaponEquipmentComponent::EquipInventorySlot(int32 SlotIndex)
{
	if (SlotIndex == INDEX_NONE || IsEquipmentInteractionBlocked())
	{
		return false;
	}

	const UShooterInventoryComponent* InventoryComponent = GetOwningInventoryComponent();
	const FWeaponInventoryEntry* TargetEntry = InventoryComponent != nullptr
		? InventoryComponent->GetInventoryEntryBySlotIndex(SlotIndex)
		: nullptr;
	if (TargetEntry == nullptr)
	{
		return false;
	}

	AActor* OwnerActor = GetOwner();
	if (OwnerActor != nullptr && OwnerActor->HasAuthority())
	{
		return EquipInventoryItemById(TargetEntry->ItemId);
	}

	ServerEquipInventorySlot(SlotIndex);
	return true;
}

bool UShooterWeaponEquipmentComponent::EquipNextInventorySlot()
{
	const UShooterInventoryComponent* InventoryComponent = GetOwningInventoryComponent();
	if (InventoryComponent == nullptr)
	{
		return false;
	}

	int32 CurrentSlotIndex = INDEX_NONE;
	if (const FWeaponInventoryEntry* EquippedEntry = InventoryComponent->GetInventoryEntryByItemId(EquippedItemId))
	{
		CurrentSlotIndex = EquippedEntry->SlotIndex;
	}

	const int32 NextSlotIndex = CurrentSlotIndex == INDEX_NONE
		? InventoryComponent->FindFirstOccupiedSlotIndex()
		: InventoryComponent->FindNextOccupiedSlotIndex(CurrentSlotIndex, true);
	return NextSlotIndex != INDEX_NONE ? EquipInventorySlot(NextSlotIndex) : false;
}

bool UShooterWeaponEquipmentComponent::EquipPreviousInventorySlot()
{
	const UShooterInventoryComponent* InventoryComponent = GetOwningInventoryComponent();
	if (InventoryComponent == nullptr)
	{
		return false;
	}

	int32 CurrentSlotIndex = INDEX_NONE;
	if (const FWeaponInventoryEntry* EquippedEntry = InventoryComponent->GetInventoryEntryByItemId(EquippedItemId))
	{
		CurrentSlotIndex = EquippedEntry->SlotIndex;
	}

	const int32 PreviousSlotIndex = CurrentSlotIndex == INDEX_NONE
		? InventoryComponent->FindFirstOccupiedSlotIndex()
		: InventoryComponent->FindNextOccupiedSlotIndex(CurrentSlotIndex, false);
	return PreviousSlotIndex != INDEX_NONE ? EquipInventorySlot(PreviousSlotIndex) : false;
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

	UShooterInventoryComponent* InventoryComponent = GetOwningInventoryComponent();
	if (InventoryComponent == nullptr)
	{
		return false;
	}

	const TArray<FWeaponInventoryEntry> InventoryEntriesToDrop = InventoryComponent->GetInventoryEntries();
	if (InventoryEntriesToDrop.IsEmpty())
	{
		DestroyEquippedWeaponActor();
		ClearEquippedWeaponInstance();
		EquippedItemId = INDEX_NONE;
		return false;
	}

	AShooterWeaponEquipmentActor* EquippedPresentationActor = EquippedWeapon;
	EquippedWeapon = nullptr;
	EquippedItemId = INDEX_NONE;
	ClearEquippedWeaponInstance();

	const ACharacter* OwnerCharacter = GetOwningCharacter();
	const FVector RightDirection = OwnerCharacter != nullptr ? OwnerCharacter->GetActorRightVector() : FVector::RightVector;

	bool bDroppedAnyWeapon = false;
	for (const FWeaponInventoryEntry& Entry : InventoryEntriesToDrop)
	{
		if (!Entry.IsValid())
		{
			continue;
		}

		FTransform DropTransform = GetWeaponDeathDropTransform();
		const FVector DropLocation = DropTransform.GetLocation() + (RightDirection * 35.f * static_cast<float>(Entry.SlotIndex));
		DropTransform.SetLocation(DropLocation);

		if (SpawnWorldPickupFromEntry(Entry, DropTransform, EShooterWeaponDropMode::DeathInPlace))
		{
			FWeaponInventoryEntry RemovedEntry;
			InventoryComponent->RemoveWeaponByItemId(Entry.ItemId, RemovedEntry);
			bDroppedAnyWeapon = true;
		}
	}

	if (EquippedPresentationActor != nullptr)
	{
		EquippedPresentationActor->Destroy();
	}

	return bDroppedAnyWeapon;
}

void UShooterWeaponEquipmentComponent::HandleInventoryReplicated()
{
	RefreshEquippedWeaponInstance();
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

UShooterWeaponInteractionComponent* UShooterWeaponEquipmentComponent::GetOwningWeaponInteractionComponent() const
{
	const IShooterEquipmentInterface* EquipmentOwner = Cast<IShooterEquipmentInterface>(GetOwner());
	return EquipmentOwner != nullptr ? EquipmentOwner->GetShooterWeaponInteractionComponent() : nullptr;
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

UShooterAbilitySystemComponent* UShooterWeaponEquipmentComponent::GetOwningShooterAbilitySystemComponent() const
{
	const IShooterCombatInterface* CombatOwner = Cast<IShooterCombatInterface>(GetOwner());
	return CombatOwner != nullptr
		? Cast<UShooterAbilitySystemComponent>(CombatOwner->GetShooterAbilitySystemComponent())
		: nullptr;
}

bool UShooterWeaponEquipmentComponent::TryPickupTargetWeapon(AShooterWeaponPickupActor* TargetWeapon)
{
	AActor* OwnerActor = GetOwner();
	UShooterInventoryComponent* InventoryComponent = GetOwningInventoryComponent();
	if (OwnerActor == nullptr || !OwnerActor->HasAuthority() || IsEquipmentInteractionBlocked() || InventoryComponent == nullptr)
	{
		return false;
	}

	if (TargetWeapon == nullptr)
	{
		return false;
	}

	// Revalidate that the target weapon is a valid pickup within acceptable range.
	// We prefer the view-trace match but allow a distance-based fallback because the
	// server's physics-driven weapon position and the client's view rotation can
	// diverge under network latency, causing the view trace to miss the weapon that
	// the client is legitimately looking at.
	UShooterWeaponInteractionComponent* InteractionComponent = GetOwningWeaponInteractionComponent();
	AShooterWeaponPickupActor* ViewTraceWeapon = InteractionComponent != nullptr
		? InteractionComponent->FindPickupWeaponFromView()
		: nullptr;

	const bool bViewTraceMatches = ViewTraceWeapon != nullptr && ViewTraceWeapon == TargetWeapon;

	if (!bViewTraceMatches && !IsValidWorldPickupForPickup(TargetWeapon))
	{
		return false;
	}

	// Prefer the view-trace result when it matches; otherwise use the client-supplied target.
	AShooterWeaponPickupActor* WeaponToPickup = bViewTraceMatches ? ViewTraceWeapon : TargetWeapon;

	int32 NewItemId = INDEX_NONE;
	int32 NewSlotIndex = INDEX_NONE;
	if (!InventoryComponent->AddWeaponFromPickup(WeaponToPickup, NewItemId, NewSlotIndex))
	{
		if (EquippedItemId == INDEX_NONE || !DropEquippedWeapon() || !InventoryComponent->AddWeaponFromPickup(WeaponToPickup, NewItemId, NewSlotIndex))
		{
			return false;
		}
	}

	return EquipInventoryItemById(NewItemId, WeaponToPickup);
}

bool UShooterWeaponEquipmentComponent::EquipInventoryItemById(int32 ItemId, AShooterWeaponPickupActor* ExistingPickupActor)
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
	if (ExistingPickupActor != nullptr)
	{
		ExistingPickupActor->Destroy();
	}
	RefreshEquippedWeaponInstance();
	return true;
}

bool UShooterWeaponEquipmentComponent::DropEquippedWeapon()
{
	AActor* OwnerActor = GetOwner();
	UShooterInventoryComponent* InventoryComponent = GetOwningInventoryComponent();
	if (OwnerActor == nullptr || !OwnerActor->HasAuthority() || EquippedItemId == INDEX_NONE || InventoryComponent == nullptr)
	{
		return false;
	}

	if (UShooterCombatComponent* CombatComponent = GetOwningCombatComponent())
	{
		CombatComponent->StopFireInput();
	}

	const FWeaponInventoryEntry* EquippedEntry = InventoryComponent->GetInventoryEntryByItemId(EquippedItemId);
	if (EquippedEntry == nullptr || !EquippedEntry->IsValid())
	{
		DestroyEquippedWeaponActor();
		EquippedItemId = INDEX_NONE;
		ClearEquippedWeaponInstance();
		return false;
	}

	const int32 DroppedSlotIndex = EquippedEntry->SlotIndex;
	const FTransform DropTransform = GetWeaponDropTransform();
	AShooterWeaponEquipmentActor* EquippedPresentationActor = EquippedWeapon;
	const int32 DroppedItemId = EquippedItemId;

	EquippedWeapon = nullptr;
	EquippedItemId = INDEX_NONE;
	ClearEquippedWeaponInstance();

	FWeaponInventoryEntry RemovedEntry;
	if (!InventoryComponent->RemoveWeaponByItemId(DroppedItemId, RemovedEntry))
	{
		EquippedWeapon = EquippedPresentationActor;
		EquippedItemId = DroppedItemId;
		RefreshEquippedWeaponInstance();
		return false;
	}

	const bool bSpawnedWorldPickup = SpawnWorldPickupFromEntry(RemovedEntry, DropTransform, EShooterWeaponDropMode::ManualThrow);
	if (EquippedPresentationActor != nullptr)
	{
		EquippedPresentationActor->Destroy();
	}

	if (!bSpawnedWorldPickup)
	{
		return false;
	}

	const int32 NextSlotIndex = InventoryComponent->FindNextOccupiedSlotIndex(DroppedSlotIndex, true);
	const FWeaponInventoryEntry* NextEntry = NextSlotIndex != INDEX_NONE
		? InventoryComponent->GetInventoryEntryBySlotIndex(NextSlotIndex)
		: nullptr;
	if (NextEntry != nullptr)
	{
		EquipInventoryItemById(NextEntry->ItemId);
	}

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

bool UShooterWeaponEquipmentComponent::SpawnWorldPickupFromEntry(
	const FWeaponInventoryEntry& Entry,
	const FTransform& DropTransform,
	EShooterWeaponDropMode DropMode)
{
	if (!Entry.IsValid())
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return false;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FTransform StartDropTransform = DropTransform;
	const FTransform GroundedDropTransform = DropMode == EShooterWeaponDropMode::ManualThrow
		? ResolveBallisticDropTransform(StartDropTransform)
		: ResolveGroundedDropTransform(ResolveReachableDropTransform(StartDropTransform));
	AShooterWeaponPickupActor* WorldWeapon = World->SpawnActor<AShooterWeaponPickupActor>(
		Entry.PickupActorClass,
		GroundedDropTransform,
		SpawnParameters);
	if (WorldWeapon == nullptr)
	{
		return false;
	}

	WorldWeapon->SetPickupData(Entry.ToPickupData());
	WorldWeapon->EnterWorldPickupState(GroundedDropTransform);

	FWeaponDropPresentationData DropPresentationData;
	DropPresentationData.StartLocation = StartDropTransform.GetLocation();
	DropPresentationData.EndLocation = GroundedDropTransform.GetLocation();
	DropPresentationData.Duration = WeaponDropPresentationDuration;
	DropPresentationData.ArcHeight = DropMode == EShooterWeaponDropMode::ManualThrow ? WeaponDropPresentationArcHeight : 0.f;
	DropPresentationData.FinalMeshRelativeTransform = WorldWeapon->GetDroppedMeshRelativeTransform();
	WorldWeapon->SetDropPresentationData(DropPresentationData);
	return true;
}

FTransform UShooterWeaponEquipmentComponent::ResolveGroundedDropTransform(const FTransform& DropTransform) const
{
	FTransform GroundedTransform = DropTransform;
	UWorld* World = GetWorld();
	if (World == nullptr || DropGroundTraceDistance <= 0.f)
	{
		return GroundedTransform;
	}

	const FVector DropLocation = DropTransform.GetLocation();
	const FVector TraceStart = DropLocation + FVector(0.f, 0.f, DropGroundTraceUpOffset);
	const FVector TraceEnd = DropLocation - FVector(0.f, 0.f, DropGroundTraceDistance);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PlayerWeaponDropGroundTrace), false, GetOwner());
	if (AActor* OwnerActor = GetOwner())
	{
		QueryParams.AddIgnoredActor(OwnerActor);
	}

	if (EquippedWeapon != nullptr)
	{
		QueryParams.AddIgnoredActor(EquippedWeapon);
	}

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);

	FHitResult GroundHit;
	if (World->LineTraceSingleByObjectType(GroundHit, TraceStart, TraceEnd, ObjectQueryParams, QueryParams))
	{
		GroundedTransform.SetLocation(GroundHit.ImpactPoint + (GroundHit.ImpactNormal * DropGroundOffset));
	}

	return GroundedTransform;
}

FTransform UShooterWeaponEquipmentComponent::ResolveBallisticDropTransform(const FTransform& StartTransform) const
{
	UWorld* World = GetWorld();
	const ACharacter* OwnerCharacter = GetOwningCharacter();
	if (World == nullptr || OwnerCharacter == nullptr)
	{
		return ResolveGroundedDropTransform(StartTransform);
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PlayerWeaponDropBallisticSweep), false, OwnerCharacter);
	QueryParams.AddIgnoredActor(OwnerCharacter);

	if (EquippedWeapon != nullptr)
	{
		QueryParams.AddIgnoredActor(EquippedWeapon);
	}

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);

	const FVector StartLocation = StartTransform.GetLocation();
	const FVector ForwardDirection = StartTransform.GetRotation().GetForwardVector().GetSafeNormal();
	const FVector InitialVelocity = (ForwardDirection * WeaponDropForwardSpeed) + (FVector::UpVector * WeaponDropUpSpeed);
	const FVector GravityAcceleration(0.f, 0.f, DropGravityZ);
	const float SimulationStep = FMath::Max(DropSimulationStep, 0.005f);
	const float MaxSimulationTime = FMath::Max(DropMaxSimulationTime, SimulationStep);
	const FCollisionShape SweepShape = FCollisionShape::MakeSphere(FMath::Max(DropWallSweepRadius, 1.f));

	FVector PreviousLocation = StartLocation;
	FVector LastPredictedLocation = StartLocation;

	for (float Time = SimulationStep; Time <= MaxSimulationTime + KINDA_SMALL_NUMBER; Time += SimulationStep)
	{
		const FVector CurrentLocation = StartLocation
			+ (InitialVelocity * Time)
			+ (GravityAcceleration * (0.5f * Time * Time));
		LastPredictedLocation = CurrentLocation;

		FHitResult HitResult;
		if (World->SweepSingleByObjectType(
			HitResult,
			PreviousLocation,
			CurrentLocation,
			FQuat::Identity,
			ObjectQueryParams,
			SweepShape,
			QueryParams))
		{
			FTransform HitTransform = StartTransform;
			if (HitResult.ImpactNormal.Z >= 0.45f)
			{
				HitTransform.SetLocation(HitResult.ImpactPoint + (HitResult.ImpactNormal * DropGroundOffset));
				return HitTransform;
			}

			const FVector SweepDirection = (CurrentLocation - PreviousLocation).GetSafeNormal();
			const FVector WallSafeLocation = HitResult.Location - (SweepDirection * DropWallClearance);
			HitTransform.SetLocation(WallSafeLocation);
			return ResolveGroundedDropTransform(HitTransform);
		}

		PreviousLocation = CurrentLocation;
	}

	FTransform FallbackTransform = StartTransform;
	FallbackTransform.SetLocation(LastPredictedLocation);
	return ResolveGroundedDropTransform(ResolveReachableDropTransform(FallbackTransform));
}

FTransform UShooterWeaponEquipmentComponent::ResolveReachableDropTransform(const FTransform& DropTransform) const
{
	FTransform ReachableTransform = DropTransform;
	UWorld* World = GetWorld();
	const ACharacter* OwnerCharacter = GetOwningCharacter();
	if (World == nullptr || OwnerCharacter == nullptr || DropWallSweepRadius <= 0.f)
	{
		return ReachableTransform;
	}

	const FVector TargetLocation = DropTransform.GetLocation();
	const FVector SweepStart = OwnerCharacter->GetActorLocation() + FVector(0.f, 0.f, WeaponDropUpOffset);
	const FVector SweepDelta = TargetLocation - SweepStart;
	if (SweepDelta.SizeSquared() <= KINDA_SMALL_NUMBER)
	{
		return ReachableTransform;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PlayerWeaponDropWallSweep), false, OwnerCharacter);
	QueryParams.AddIgnoredActor(OwnerCharacter);

	if (EquippedWeapon != nullptr)
	{
		QueryParams.AddIgnoredActor(EquippedWeapon);
	}

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);

	FHitResult WallHit;
	const FCollisionShape SweepShape = FCollisionShape::MakeSphere(DropWallSweepRadius);
	if (World->SweepSingleByObjectType(
		WallHit,
		SweepStart,
		TargetLocation,
		FQuat::Identity,
		ObjectQueryParams,
		SweepShape,
		QueryParams))
	{
		const FVector SweepDirection = SweepDelta.GetSafeNormal();
		const FVector ClampedLocation = WallHit.Location - (SweepDirection * DropWallClearance);
		ReachableTransform.SetLocation(ClampedLocation);
	}

	return ReachableTransform;
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
		EquippedWeapon = nullptr;
		EquippedItemId = INDEX_NONE;
		ClearEquippedWeaponInstance();
	}
}

void UShooterWeaponEquipmentComponent::RefreshEquippedWeaponInstance()
{
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

FTransform UShooterWeaponEquipmentComponent::GetWeaponDropTransform() const
{
	const ACharacter* OwnerCharacter = GetOwningCharacter();
	if (OwnerCharacter == nullptr)
	{
		return FTransform::Identity;
	}

	FRotator ControlYawRotation = OwnerCharacter->GetActorRotation();
	if (OwnerCharacter->GetController() != nullptr)
	{
		ControlYawRotation = FRotator(0.f, OwnerCharacter->GetController()->GetControlRotation().Yaw, 0.f);
	}

	FVector DropForwardDirection = ControlYawRotation.Vector();
	if (DropForwardDirection.IsNearlyZero())
	{
		DropForwardDirection = OwnerCharacter->GetActorForwardVector();
	}

	const FVector DropLocation = OwnerCharacter->GetActorLocation()
		+ (DropForwardDirection * WeaponDropForwardOffset)
		+ FVector(0.f, 0.f, WeaponDropUpOffset);

	return FTransform(ControlYawRotation, DropLocation);
}

FTransform UShooterWeaponEquipmentComponent::GetWeaponDeathDropTransform() const
{
	const ACharacter* OwnerCharacter = GetOwningCharacter();
	if (OwnerCharacter == nullptr)
	{
		return FTransform::Identity;
	}

	const FRotator OwnerYawRotation(0.f, OwnerCharacter->GetActorRotation().Yaw, 0.f);
	const FVector DropLocation = OwnerCharacter->GetActorLocation() + FVector(0.f, 0.f, WeaponDropUpOffset);
	return FTransform(OwnerYawRotation, DropLocation);
}

bool UShooterWeaponEquipmentComponent::IsValidWorldPickupForPickup(const AShooterWeaponPickupActor* Weapon) const
{
	if (Weapon == nullptr)
	{
		return false;
	}

	if (!Weapon->IsPickupInteractionEnabled())
	{
		return false;
	}

	const ACharacter* OwnerCharacter = GetOwningCharacter();
	if (OwnerCharacter == nullptr)
	{
		return false;
	}

	const float DistSq = FVector::DistSquared(Weapon->GetActorLocation(), OwnerCharacter->GetActorLocation());
	return DistSq <= FMath::Square(PickupFallbackValidationRadius);
}

void UShooterWeaponEquipmentComponent::OnRep_EquippedWeapon(AShooterWeaponEquipmentActor* OldEquippedWeapon)
{
	if (OldEquippedWeapon != nullptr && OldEquippedWeapon != EquippedWeapon)
	{
		ClearLocalEquippedWeaponPresentation(OldEquippedWeapon);
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

void UShooterWeaponEquipmentComponent::ServerEquipInventorySlot_Implementation(int32 SlotIndex)
{
	if (IsEquipmentInteractionBlocked())
	{
		return;
	}

	EquipInventorySlot(SlotIndex);
}
