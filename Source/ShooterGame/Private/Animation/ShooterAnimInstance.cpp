#include "Animation/ShooterAnimInstance.h"
#include "Character/PlayerCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Weapon/ShooterWeaponEquipmentActor.h"

void UShooterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	RefreshOwningPlayerCharacter();
}

void UShooterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	RefreshOwningPlayerCharacter();

	const APlayerCharacter* PlayerCharacter = OwningPlayerCharacter.Get();
	UpdateLeftHandIK(PlayerCharacter);
	if (PlayerCharacter == nullptr)
	{
		GroundSpeed = 0.f;
		bIsInAir = false;
		bIsAiming = false;
		bIsEquipped = false;
		bIsCrouching = false;
		bIsSprinting = false;
		return;
	}

	const UCharacterMovementComponent* MovementComponent = PlayerCharacter->GetCharacterMovement();
	if (MovementComponent == nullptr)
	{
		GroundSpeed = 0.f;
		bIsInAir = false;
		bIsAiming = false;
		bIsEquipped = false;
		bIsCrouching = false;
		bIsSprinting = false;
		return;
	}

	const FVector Velocity = PlayerCharacter->GetVelocity();
	GroundSpeed = FVector(Velocity.X, Velocity.Y, 0.f).Size();
	bIsInAir = MovementComponent->IsFalling();
	bIsAiming = PlayerCharacter->IsAiming();
	bIsEquipped = PlayerCharacter->IsEquipped();
	bIsCrouching = MovementComponent->IsCrouching();
	bIsSprinting = PlayerCharacter->IsSprinting();
}

APlayerCharacter* UShooterAnimInstance::GetOwningPlayerCharacter() const
{
	return OwningPlayerCharacter.Get();
}

void UShooterAnimInstance::UpdateLeftHandIK(const APlayerCharacter* PlayerCharacter)
{
	LeftHandIKAlpha = 0.f;
	LeftHandIKTransform = FTransform::Identity;
	if (PlayerCharacter == nullptr || GetSkelMeshComponent() != PlayerCharacter->GetMesh() || PlayerCharacter->IsDead())
	{
		return;
	}

	const AShooterWeaponEquipmentActor* Weapon = PlayerCharacter->GetEquippedWeapon();
	if (Weapon == nullptr)
	{
		return;
	}

	const UStaticMeshComponent* WeaponMesh = Weapon->GetWeaponMesh();
	static const FName GripSocketName(TEXT("LeftHandGrip"));
	static const FName RightHandBoneName(TEXT("hand_r"));
	if (!WeaponMesh->DoesSocketExist(GripSocketName))
	{
		return;
	}

	// Both transforms use the same rendered pose. Converting to hand_r space removes
	// that pose; FABRIK applies the offset to the current frame's right hand.
	LeftHandIKTransform = WeaponMesh->GetSocketTransform(GripSocketName).GetRelativeTransform(
		PlayerCharacter->GetMesh()->GetSocketTransform(RightHandBoneName));
	LeftHandIKAlpha = 1.f;
}

void UShooterAnimInstance::RefreshOwningPlayerCharacter()
{
	if (OwningPlayerCharacter.IsValid()) return;
	OwningPlayerCharacter = Cast<APlayerCharacter>(TryGetPawnOwner());
}
