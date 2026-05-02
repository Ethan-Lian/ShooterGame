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
#include "Components/ShooterWeaponInteractionComponent.h"
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
	WeaponInteractionComponent = CreateDefaultSubobject<UShooterWeaponInteractionComponent>(TEXT("WeaponInteractionComponent"));
	
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
	InitializeAbilitySystemActorInfo();
}

void APlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	InitializeAbilitySystemActorInfo();
}

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (HealthComponent != nullptr)
	{
		HealthComponent->OnHealthChanged.AddUniqueDynamic(this, &APlayerCharacter::HandleHealthChanged);
	}
}

AShooterPlayerState* APlayerCharacter::GetShooterPlayerState() const
{
	return GetPlayerState<AShooterPlayerState>();
}

void APlayerCharacter::InitializeAbilitySystemActorInfo()
{
	AShooterPlayerState* ShooterPlayerState = GetShooterPlayerState();
	if (ShooterPlayerState == nullptr)
	{
		return;
	}

	ShooterPlayerState->InitializeAbilitySystem(this);

	if (HealthComponent != nullptr)
	{
		HealthComponent->InitializeWithAbilitySystem(ShooterPlayerState->GetAbilitySystemComponent());
	}

	if (MovementStateComponent != nullptr)
	{
		MovementStateComponent->InitializeWithAbilitySystem(ShooterPlayerState->GetAbilitySystemComponent());
	}

	BindDeathStateTagListener(ShooterPlayerState->GetAbilitySystemComponent());

	if (IsLocallyControlled())
	{
		if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
		{
			if (AShooterHUD* ShooterHUD = PlayerController->GetHUD<AShooterHUD>())
			{
				ShooterHUD->SetObservedPawn(this);
			}
		}
	}
}

void APlayerCharacter::BindDeathStateTagListener(UAbilitySystemComponent* AbilitySystemComponent)
{
	if (BoundDeathStateAbilitySystemComponent.Get() == AbilitySystemComponent && DeathStateTagChangedDelegateHandle.IsValid())
	{
		return;
	}

	if (UAbilitySystemComponent* BoundAbilitySystemComponent = BoundDeathStateAbilitySystemComponent.Get())
	{
		if (DeathStateTagChangedDelegateHandle.IsValid())
		{
			BoundAbilitySystemComponent->RegisterGameplayTagEvent(TAG_State_Dead).Remove(DeathStateTagChangedDelegateHandle);
		}
	}

	BoundDeathStateAbilitySystemComponent = AbilitySystemComponent;
	DeathStateTagChangedDelegateHandle.Reset();

	if (AbilitySystemComponent == nullptr)
	{
		return;
	}

	DeathStateTagChangedDelegateHandle = AbilitySystemComponent->RegisterGameplayTagEvent(
		TAG_State_Dead,
		EGameplayTagEventType::NewOrRemoved).AddUObject(
			this,
			&APlayerCharacter::HandleDeathStateTagChanged);

	if (AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Dead))
	{
		ApplyDeathPresentation();
	}
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

void APlayerCharacter::HandleDeathStateTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (Tag == TAG_State_Dead && NewCount > 0)
	{
		ApplyDeathPresentation();
	}
}
