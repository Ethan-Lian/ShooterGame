#include "Components/ShooterWeaponInteractionComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/ShooterGameplayTags.h"
#include "Character/PlayerCharacter.h"
#include "CollisionShape.h"
#include "Components/ShooterCombatComponent.h"
#include "Components/ShooterWeaponEquipmentComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "Weapon/ShooterWeaponBase.h"
#include "WorldCollision.h"

UShooterWeaponInteractionComponent::UShooterWeaponInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UShooterWeaponInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	APlayerCharacter* OwnerCharacter = GetOwningPlayerCharacter();
	if (OwnerCharacter == nullptr || !OwnerCharacter->IsLocallyControlled() || IsInteractionBlocked())
	{
		ClearTargetedPickupWeapon();
		return;
	}

	RefreshTargetedPickupWeapon();
}

AShooterWeaponBase* UShooterWeaponInteractionComponent::FindPickupWeaponFromView() const
{
	FHitResult HitResult;
	if (!TracePickupView(HitResult))
	{
		return nullptr;
	}

	AShooterWeaponBase* HitWeapon = Cast<AShooterWeaponBase>(HitResult.GetActor());
	return CanPickupWeapon(HitWeapon) ? HitWeapon : nullptr;
}

bool UShooterWeaponInteractionComponent::CanPickupWeapon(const AShooterWeaponBase* WeaponToPickup) const
{
	const APlayerCharacter* OwnerCharacter = GetOwningPlayerCharacter();
	const UShooterWeaponEquipmentComponent* EquipmentComponent = GetOwningWeaponEquipmentComponent();
	if (WeaponToPickup == nullptr || OwnerCharacter == nullptr || EquipmentComponent == nullptr)
	{
		return false;
	}

	if (!WeaponToPickup->IsPickupInteractionEnabled() || WeaponToPickup == EquipmentComponent->GetEquippedWeapon())
	{
		return false;
	}

	if (PickupSearchRadius <= 0.f)
	{
		return false;
	}

	return FVector::DistSquared(WeaponToPickup->GetActorLocation(), OwnerCharacter->GetActorLocation()) <= FMath::Square(PickupSearchRadius);
}

bool UShooterWeaponInteractionComponent::GetViewTracePoints(float TraceDistance, FVector& OutTraceStart, FVector& OutTraceEnd) const
{
	const APlayerCharacter* OwnerCharacter = GetOwningPlayerCharacter();
	if (OwnerCharacter == nullptr || TraceDistance <= 0.f)
	{
		return false;
	}

	FVector ViewLocation = OwnerCharacter->GetPawnViewLocation();
	FRotator ViewRotation = OwnerCharacter->GetBaseAimRotation();

	if (const AController* OwnerController = OwnerCharacter->GetController())
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

APlayerCharacter* UShooterWeaponInteractionComponent::GetOwningPlayerCharacter() const
{
	return Cast<APlayerCharacter>(GetOwner());
}

UShooterCombatComponent* UShooterWeaponInteractionComponent::GetOwningCombatComponent() const
{
	const APlayerCharacter* OwnerCharacter = GetOwningPlayerCharacter();
	return OwnerCharacter != nullptr ? OwnerCharacter->GetCombatComponent() : nullptr;
}

UShooterWeaponEquipmentComponent* UShooterWeaponInteractionComponent::GetOwningWeaponEquipmentComponent() const
{
	const APlayerCharacter* OwnerCharacter = GetOwningPlayerCharacter();
	return OwnerCharacter != nullptr ? OwnerCharacter->GetWeaponEquipmentComponent() : nullptr;
}

bool UShooterWeaponInteractionComponent::TracePickupView(FHitResult& OutHitResult) const
{
	const APlayerCharacter* OwnerCharacter = GetOwningPlayerCharacter();
	const UShooterWeaponEquipmentComponent* EquipmentComponent = GetOwningWeaponEquipmentComponent();
	UWorld* World = GetWorld();
	if (OwnerCharacter == nullptr || EquipmentComponent == nullptr || World == nullptr)
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

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PlayerWeaponPickupViewTrace), false, OwnerCharacter);
	QueryParams.AddIgnoredActor(OwnerCharacter);

	if (AShooterWeaponBase* EquippedWeapon = EquipmentComponent->GetEquippedWeapon())
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
	const APlayerCharacter* OwnerCharacter = GetOwningPlayerCharacter();
	if (OwnerCharacter == nullptr)
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

	if (const UAbilitySystemComponent* AbilitySystemComponent = OwnerCharacter->GetAbilitySystemComponent())
	{
		return AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Dead);
	}

	return false;
}

void UShooterWeaponInteractionComponent::RefreshTargetedPickupWeapon()
{
	SetTargetedPickupWeapon(FindPickupWeaponFromView());
}

void UShooterWeaponInteractionComponent::SetTargetedPickupWeapon(AShooterWeaponBase* NewTargetWeapon)
{
	AShooterWeaponBase* PreviousWeapon = CurrentTargetedPickupWeapon.Get();
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
