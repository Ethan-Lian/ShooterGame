#include "Animation/ShooterFirstPersonAnimInstance.h"

void UShooterFirstPersonAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	bIsMoving = GroundSpeed > 10.f;
}
