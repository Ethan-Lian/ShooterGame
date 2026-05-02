#include "Components/ShooterWeaponInteractionComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/ShooterGameplayTags.h"
#include "CollisionShape.h"
#include "Components/ShooterCombatComponent.h"
#include "Components/ShooterWeaponEquipmentComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Interfaces/ShooterCombatInterface.h"
#include "Interfaces/ShooterEquipmentInterface.h"
#include "Weapon/ShooterWeaponEquipmentActor.h"
#include "Weapon/ShooterWeaponPickupActor.h"
#include "WorldCollision.h"

UShooterWeaponInteractionComponent::UShooterWeaponInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UShooterWeaponInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr || !OwnerPawn->IsLocallyControlled() || IsInteractionBlocked())
	{
		ClearTargetedPickupWeapon();
		return;
	}

	RefreshTargetedPickupWeapon();
}

AShooterWeaponPickupActor* UShooterWeaponInteractionComponent::FindPickupWeaponFromView() const
{
	FHitResult HitResult;
	if (!TracePickupView(HitResult))
	{
		return nullptr;
	}

	AShooterWeaponPickupActor* HitWeapon = Cast<AShooterWeaponPickupActor>(HitResult.GetActor());
	return CanPickupWeapon(HitWeapon) ? HitWeapon : nullptr;
}

bool UShooterWeaponInteractionComponent::CanPickupWeapon(const AShooterWeaponPickupActor* WeaponToPickup) const
{
	const AActor* OwnerActor = GetOwner();
	const UShooterWeaponEquipmentComponent* EquipmentComponent = GetOwningWeaponEquipmentComponent();
	if (WeaponToPickup == nullptr || OwnerActor == nullptr || EquipmentComponent == nullptr)
	{
		return false;
	}

	if (!WeaponToPickup->IsPickupInteractionEnabled())
	{
		return false;
	}

	if (PickupSearchRadius <= 0.f)
	{
		return false;
	}

	return FVector::DistSquared(WeaponToPickup->GetActorLocation(), OwnerActor->GetActorLocation()) <= FMath::Square(PickupSearchRadius);
}

bool UShooterWeaponInteractionComponent::GetViewTracePoints(float TraceDistance, FVector& OutTraceStart, FVector& OutTraceEnd) const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr || TraceDistance <= 0.f)
	{
		return false;
	}

	FVector ViewLocation = OwnerPawn->GetPawnViewLocation();
	FRotator ViewRotation = OwnerPawn->GetBaseAimRotation();

	if (const AController* OwnerController = OwnerPawn->GetController())
	{
		OwnerController->GetPlayerViewPoint(ViewLocation, ViewRotation);
	}

	OutTraceStart = ViewLocation;
	OutTraceEnd = ViewLocation + (ViewRotation.Vector() * TraceDistance);
	return true;
}

void UShooterWeaponInteractionComponent::ClearTargetedPickupWeapon()
{
	SetTargetedPickupWeapon(nullptr);
}

UShooterCombatComponent* UShooterWeaponInteractionComponent::GetOwningCombatComponent() const
{
	const IShooterCombatInterface* CombatOwner = Cast<IShooterCombatInterface>(GetOwner());
	return CombatOwner != nullptr ? CombatOwner->GetShooterCombatComponent() : nullptr;
}

UShooterWeaponEquipmentComponent* UShooterWeaponInteractionComponent::GetOwningWeaponEquipmentComponent() const
{
	const IShooterEquipmentInterface* EquipmentOwner = Cast<IShooterEquipmentInterface>(GetOwner());
	return EquipmentOwner != nullptr ? EquipmentOwner->GetShooterWeaponEquipmentComponent() : nullptr;
}

bool UShooterWeaponInteractionComponent::TracePickupView(FHitResult& OutHitResult) const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	const UShooterWeaponEquipmentComponent* EquipmentComponent = GetOwningWeaponEquipmentComponent();
	UWorld* World = GetWorld();
	if (OwnerPawn == nullptr || EquipmentComponent == nullptr || World == nullptr)
	{
		return false;
	}

	if (PickupTraceMaxDistance <= 0.f)
	{
		return false;
	}

	FVector TraceStart = FVector::ZeroVector;
	FVector TraceEnd = FVector::ZeroVector;
	if (!GetViewTracePoints(PickupTraceMaxDistance, TraceStart, TraceEnd))
	{
		return false;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PlayerWeaponPickupViewTrace), false, OwnerPawn);
	QueryParams.AddIgnoredActor(OwnerPawn);

	if (AShooterWeaponEquipmentActor* EquippedWeapon = EquipmentComponent->GetEquippedWeapon())
	{
		QueryParams.AddIgnoredActor(EquippedWeapon);
	}

	if (PickupTraceRadius <= KINDA_SMALL_NUMBER)
	{
		return World->LineTraceSingleByChannel(OutHitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams);
	}

	return World->SweepSingleByChannel(
		OutHitResult,
		TraceStart,
		TraceEnd,
		FQuat::Identity,
		ECC_Visibility,
		FCollisionShape::MakeSphere(PickupTraceRadius),
		QueryParams);
}

bool UShooterWeaponInteractionComponent::IsInteractionBlocked() const
{
	const IShooterCombatInterface* CombatOwner = Cast<IShooterCombatInterface>(GetOwner());
	if (CombatOwner == nullptr)
	{
		return true;
	}

	if (const UShooterCombatComponent* CombatComponent = GetOwningCombatComponent())
	{
		if (CombatComponent->IsWeaponInteractionBlocked())
		{
			return true;
		}
	}

	if (const UAbilitySystemComponent* AbilitySystemComponent = CombatOwner->GetShooterAbilitySystemComponent())
	{
		return AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Dead);
	}

	return false;
}

void UShooterWeaponInteractionComponent::RefreshTargetedPickupWeapon()
{
	SetTargetedPickupWeapon(FindPickupWeaponFromView());
}

void UShooterWeaponInteractionComponent::SetTargetedPickupWeapon(AShooterWeaponPickupActor* NewTargetWeapon)
{
	AShooterWeaponPickupActor* PreviousWeapon = CurrentTargetedPickupWeapon.Get();
	if (PreviousWeapon == NewTargetWeapon)
	{
		if (NewTargetWeapon != nullptr)
		{
			NewTargetWeapon->SetPickupWidgetVisible(true);
		}

		return;
	}

	if (PreviousWeapon != nullptr)
	{
		PreviousWeapon->SetPickupWidgetVisible(false);
	}

	CurrentTargetedPickupWeapon = NewTargetWeapon;

	if (NewTargetWeapon != nullptr)
	{
		NewTargetWeapon->SetPickupWidgetVisible(true);
	}
}
