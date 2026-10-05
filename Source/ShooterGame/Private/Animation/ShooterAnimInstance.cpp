#include "Animation/ShooterAnimInstance.h"
#include "Character/PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

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

void UShooterAnimInstance::RefreshOwningPlayerCharacter()
{
	if (OwningPlayerCharacter.IsValid()) return;
	OwningPlayerCharacter = Cast<APlayerCharacter>(TryGetPawnOwner());
}
