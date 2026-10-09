#pragma once

#include "Animation/ShooterAnimInstance.h"
#include "ShooterFirstPersonAnimInstance.generated.h"

// The AnimGraph consumes these values; it does not calculate gameplay state.
UCLASS()
class SHOOTERGAME_API UShooterFirstPersonAnimInstance : public UShooterAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|FirstPerson")
	bool bIsMoving = false;
};
