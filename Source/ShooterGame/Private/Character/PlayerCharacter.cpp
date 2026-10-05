#include "Character/PlayerCharacter.h"
#include "AbilitySystem/ShooterGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/ShooterCombatComponent.h"
#include "Components/ShooterWeaponEquipmentComponent.h"
#include "Components/ShooterHealthComponent.h"
#include "Components/ShooterInventoryComponent.h"
#include "Components/ShooterMovementStateComponent.h"
#include "Components/ShooterPawnExtensionComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "HUD/ShooterHUD.h"
#include "PlayerState/ShooterPlayerState.h"
#include "ShooterGame.h"
#include "Weapon/ShooterWeaponEquipmentActor.h"
#include "Weapon/ShooterWeaponInstance.h"

APlayerCharacter::APlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	// Let the controller drive the camera while movement controls the actor facing.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 720.f, 0.f);

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->TargetArmLength = 400.f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	HealthComponent = CreateDefaultSubobject<UShooterHealthComponent>(TEXT("HealthComponent"));
	CombatComponent = CreateDefaultSubobject<UShooterCombatComponent>(TEXT("CombatComponent"));
	MovementStateComponent = CreateDefaultSubobject<UShooterMovementStateComponent>(TEXT("MovementStateComponent"));
	WeaponEquipmentComponent = CreateDefaultSubobject<UShooterWeaponEquipmentComponent>(TEXT("WeaponEquipmentComponent"));
	PawnExtensionComponent = CreateDefaultSubobject<UShooterPawnExtensionComponent>(TEXT("PawnExtensionComponent"));
	
	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(FName("OverheadWidget"));
	OverheadWidget->SetupAttachment(GetRootComponent());
}

void APlayerCharacter::Move(const FVector2D& InputValue)
{
	if (IsDead()) return;
	if (MovementStateComponent != nullptr)
	{
		MovementStateComponent->HandleMoveInput(InputValue);
	}

	if (Controller == nullptr || InputValue.IsNearlyZero()) return;

	const FRotator ControlRotation = Controller->GetControlRotation();
	const FRotator YawRotation(0.f, ControlRotation.Yaw, 0.f);

	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(ForwardDirection, InputValue.Y);
	AddMovementInput(RightDirection, InputValue.X);
}

void APlayerCharacter::Look(const FVector2D& InputValue)
{
	if (IsDead()) return;
	if (Controller == nullptr || InputValue.IsNearlyZero()) return;

	const float YawInput = InputValue.X * LookYawSensitivity;
	const float PitchInput = InputValue.Y * LookPitchSensitivity;

	AddControllerYawInput(YawInput);
	AddControllerPitchInput(PitchInput);
}

UAbilitySystemComponent* APlayerCharacter::GetAbilitySystemComponent() const
{
	const AShooterPlayerState* ShooterPlayerState = GetShooterPlayerState();
	return ShooterPlayerState != nullptr ? ShooterPlayerState->GetAbilitySystemComponent() : nullptr;
}

UAbilitySystemComponent* APlayerCharacter::GetShooterAbilitySystemComponent() const
{
	return GetAbilitySystemComponent();
}

AShooterWeaponEquipmentActor* APlayerCharacter::GetEquippedWeapon() const
{
	return WeaponEquipmentComponent != nullptr ? WeaponEquipmentComponent->GetEquippedWeapon() : nullptr;
}

UShooterWeaponInstance* APlayerCharacter::GetEquippedWeaponInstance() const
{
	return WeaponEquipmentComponent != nullptr ? WeaponEquipmentComponent->GetEquippedWeaponInstance() : nullptr;
}

UShooterInventoryComponent* APlayerCharacter::GetInventoryComponent() const
{
	const AShooterPlayerState* ShooterPlayerState = GetShooterPlayerState();
	return ShooterPlayerState != nullptr ? ShooterPlayerState->GetInventoryComponent() : nullptr;
}

UShooterInventoryComponent* APlayerCharacter::GetShooterInventoryComponent() const
{
	return GetInventoryComponent();
}

bool APlayerCharacter::IsAiming() const
{
	return CombatComponent != nullptr && CombatComponent->IsAiming();
}

bool APlayerCharacter::IsEquipped() const
{
	return WeaponEquipmentComponent != nullptr && WeaponEquipmentComponent->GetEquippedWeapon() != nullptr;
}

bool APlayerCharacter::IsSprinting() const
{
	return MovementStateComponent != nullptr && MovementStateComponent->IsSprinting();
}

bool APlayerCharacter::IsDead() const
{
	const UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponent();
	return AbilitySystemComponent != nullptr && AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Dead);
}

void APlayerCharacter::HandleAimStateChanged(bool bIsNowAiming)
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (MovementComponent == nullptr)
	{
		return;
	}

	bUseControllerRotationYaw = bIsNowAiming;
	MovementComponent->bOrientRotationToMovement = !bIsNowAiming;
}

void APlayerCharacter::HandleShooterAimStateChanged(bool bIsNowAiming)
{
	HandleAimStateChanged(bIsNowAiming);
}

void APlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
}

void APlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	if (PawnExtensionComponent != nullptr)
	{
		PawnExtensionComponent->CheckDefaultInitialization();
	}
}

void APlayerCharacter::UnPossessed()
{
	if (PawnExtensionComponent != nullptr)
	{
		PawnExtensionComponent->UninitializePawn();
	}
	Super::UnPossessed();
}

void APlayerCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();
}

void APlayerCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	if (PawnExtensionComponent != nullptr)
	{
		if (Controller == nullptr)
		{
			PawnExtensionComponent->UninitializePawn();
		}
		else
		{
			PawnExtensionComponent->HandleControllerChanged();
		}
	}
}

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (HealthComponent != nullptr)
	{
		HealthComponent->OnHealthChanged.AddUniqueDynamic(this, &APlayerCharacter::HandleHealthChanged);
	}

	if (PawnExtensionComponent != nullptr)
	{
		PawnExtensionComponent->CheckDefaultInitialization();
	}
}

void APlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HealthComponent != nullptr)
	{
		HealthComponent->OnHealthChanged.RemoveDynamic(this, &APlayerCharacter::HandleHealthChanged);
	}

	if (PawnExtensionComponent != nullptr)
	{
		PawnExtensionComponent->UninitializePawn();
	}

	Super::EndPlay(EndPlayReason);
}

AShooterPlayerState* APlayerCharacter::GetShooterPlayerState() const
{
	return GetPlayerState<AShooterPlayerState>();
}

void APlayerCharacter::HandleHealthChanged(float OldValue, float NewValue)
{
	UE_LOG(LogShooterGame, Verbose, TEXT("%s health changed %.1f -> %.1f"), *GetName(), OldValue, NewValue);
}

void APlayerCharacter::ApplyDeathPresentation()
{
	if (bDeathHandled)
	{
		return;
	}

	bDeathHandled = true;

	if (CombatComponent != nullptr)
	{
		CombatComponent->HandleOwnerDeath();
	}

	if (MovementStateComponent != nullptr)
	{
		MovementStateComponent->HandleOwnerDeath();
	}

	GetCharacterMovement()->DisableMovement();

	if (UCapsuleComponent* CharacterCapsule = GetCapsuleComponent())
	{
		CharacterCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		DisableInput(PlayerController);
	}

	PlayDeathMontage();
}

void APlayerCharacter::PlayDeathMontage()
{
	if (DeathMontage == nullptr || GetMesh() == nullptr)
	{
		return;
	}

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_Play(DeathMontage);
	}
}

void APlayerCharacter::RestoreAlivePresentation()
{
	bDeathHandled = false;

	if (DeathMontage != nullptr && GetMesh() != nullptr)
	{
		if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
		{
			AnimInstance->Montage_Stop(0.1f, DeathMontage);
		}
	}

	if (CombatComponent != nullptr)
	{
		CombatComponent->HandleOwnerRespawn();
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->SetMovementMode(MOVE_Walking);
	}

	if (UCapsuleComponent* CharacterCapsule = GetCapsuleComponent())
	{
		CharacterCapsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}

	if (IsLocallyControlled())
	{
		if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
		{
			EnableInput(PlayerController);
		}
	}

}
