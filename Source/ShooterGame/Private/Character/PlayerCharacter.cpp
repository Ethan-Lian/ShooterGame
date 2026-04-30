#include "Character/PlayerCharacter.h"
#include "AbilitySystem/ShooterGameplayTags.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/ShooterCombatComponent.h"
#include "Components/ShooterWeaponEquipmentComponent.h"
#include "Components/ShooterHealthComponent.h"
#include "Components/ShooterInventoryComponent.h"
#include "Components/ShooterMovementStateComponent.h"
#include "Components/ShooterWeaponInteractionComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "HUD/ShooterHUD.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraFunctionLibrary.h"
#include "Particles/ParticleSystem.h"
#include "PlayerState/ShooterPlayerState.h"
#include "ShooterGame.h"
#include "Sound/SoundBase.h"
#include "Weapon/ShooterWeaponBase.h"
#include "Weapon/ShooterWeaponInstance.h"

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
#include "DrawDebugHelpers.h"
#endif

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
	if (bIsDead) return;
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
	if (bIsDead) return;
	if (Controller == nullptr || InputValue.IsNearlyZero()) return;

	const float YawInput = InputValue.X * LookYawSensitivity;
	const float PitchInput = InputValue.Y * LookPitchSensitivity;

	AddControllerYawInput(YawInput);
	AddControllerPitchInput(PitchInput);
}

void APlayerCharacter::StartFireInput()
{
	if (bIsDead) return;
	if (CombatComponent != nullptr && CombatComponent->StartFireInput())
	{
		OnFireInputStarted();
	}
}

void APlayerCharacter::StopFireInput()
{
	if (bIsDead) return;
	if (CombatComponent != nullptr && CombatComponent->StopFireInput())
	{
		OnFireInputStopped();
	}
}

void APlayerCharacter::StartJumpInput()
{
	if (bIsDead) return;
	Jump();
}

void APlayerCharacter::StopJumpInput()
{
	if (bIsDead) return;
	StopJumping();
}

void APlayerCharacter::StartAimInput()
{
	if (bIsDead) return;
	if (MovementStateComponent != nullptr && MovementStateComponent->IsSprinting())
	{
		MovementStateComponent->StopSprintInput();
	}

	if (CombatComponent != nullptr)
	{
		CombatComponent->StartAimInput();
	}
}

void APlayerCharacter::StopAimInput()
{
	if (bIsDead) return;
	if (CombatComponent != nullptr)
	{
		CombatComponent->StopAimInput();
	}
}

void APlayerCharacter::StartCrouchInput()
{
	if (bIsDead) return;
	if (MovementStateComponent != nullptr && MovementStateComponent->IsSprinting())
	{
		MovementStateComponent->StopSprintInput();
	}

	Crouch();
}

void APlayerCharacter::StopCrouchInput()
{
	if (bIsDead) return;
	UnCrouch();
}

void APlayerCharacter::StartSprintInput()
{
	if (bIsDead) return;
	if (MovementStateComponent != nullptr)
	{
		MovementStateComponent->StartSprintInput();
	}
}

void APlayerCharacter::StopSprintInput()
{
	if (MovementStateComponent != nullptr)
	{
		MovementStateComponent->StopSprintInput();
	}
}

void APlayerCharacter::StartPickupInput()
{
	if (bIsDead) return;
	if (WeaponEquipmentComponent != nullptr)
	{
		WeaponEquipmentComponent->StartPickupInput();
	}
}

void APlayerCharacter::StartDropInput()
{
	if (bIsDead) return;
	if (WeaponEquipmentComponent != nullptr)
	{
		WeaponEquipmentComponent->StartDropInput();
	}
}

UAbilitySystemComponent* APlayerCharacter::GetAbilitySystemComponent() const
{
	const AShooterPlayerState* ShooterPlayerState = GetShooterPlayerState();
	return ShooterPlayerState != nullptr ? ShooterPlayerState->GetAbilitySystemComponent() : nullptr;
}

AShooterWeaponBase* APlayerCharacter::GetEquippedWeapon() const
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

void APlayerCharacter::GameplayCueDefaultHandler(EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters)
{
	if (Parameters.OriginalTag == TAG_GameplayCue_Damage_Hit || Parameters.MatchedTagName == TAG_GameplayCue_Damage_Hit)
	{
		HandleDamageHitGameplayCue(Parameters);
		return;
	}

	if (Parameters.OriginalTag == TAG_GameplayCue_Weapon_Fire || Parameters.MatchedTagName == TAG_GameplayCue_Weapon_Fire)
	{
		HandleWeaponFireGameplayCue(Parameters);
		return;
	}
	
	IGameplayCueInterface::GameplayCueDefaultHandler(EventType, Parameters);
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

void APlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APlayerCharacter, bIsDead);
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

void APlayerCharacter::HandleDamageHitGameplayCue(const FGameplayCueParameters& Parameters)
{
	const FHitResult* HitResult = Parameters.EffectContext.GetHitResult();
	const FVector ImpactLocation = HitResult != nullptr ? FVector(HitResult->ImpactPoint) : GetActorLocation();

	if (DamageHitEffect != nullptr)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), DamageHitEffect, ImpactLocation);
	}

	if (DamageHitSound != nullptr)
	{
		UGameplayStatics::PlaySoundAtLocation(this, DamageHitSound, ImpactLocation);
	}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	DrawDebugSphere(GetWorld(), ImpactLocation, 12.f, 12, FColor::Red, false, 1.0f);
#endif
}

void APlayerCharacter::HandleWeaponFireGameplayCue(const FGameplayCueParameters& Parameters)
{
	const UShooterWeaponInstance* EquippedWeaponInstance = GetEquippedWeaponInstance();
	AShooterWeaponBase* EquippedWeapon = GetEquippedWeapon();
	if (EquippedWeaponInstance == nullptr && EquippedWeapon == nullptr)
	{
		return;
	}

	const FWeaponFireConfig& FireConfig = EquippedWeaponInstance != nullptr
		? EquippedWeaponInstance->GetFireConfig()
		: EquippedWeapon->GetFireConfig();
	const FTransform MuzzleTransform = EquippedWeapon != nullptr
		? EquippedWeapon->GetMuzzleTransform()
		: FTransform(Parameters.Normal.Rotation(), Parameters.Location);
	UStaticMeshComponent* WeaponMesh = EquippedWeapon != nullptr ? EquippedWeapon->GetWeaponMesh() : nullptr;
	const bool bHasMuzzleSocket = WeaponMesh != nullptr
		&& !FireConfig.MuzzleSocketName.IsNone()
		&& WeaponMesh->DoesSocketExist(FireConfig.MuzzleSocketName);

	if (FireConfig.MuzzleFlashEffect != nullptr)
	{
		if (bHasMuzzleSocket)
		{
			UNiagaraFunctionLibrary::SpawnSystemAttached(
				FireConfig.MuzzleFlashEffect,
				WeaponMesh,
				FireConfig.MuzzleSocketName,
				FVector::ZeroVector,
				FRotator::ZeroRotator,
				EAttachLocation::SnapToTarget,
				true);
		}
		else
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(),
				FireConfig.MuzzleFlashEffect,
				MuzzleTransform.GetLocation(),
				MuzzleTransform.GetRotation().Rotator());
		}
	}

	if (FireConfig.FireSound != nullptr)
	{
		if (bHasMuzzleSocket)
		{
			UGameplayStatics::SpawnSoundAttached(
				FireConfig.FireSound,
				WeaponMesh,
				FireConfig.MuzzleSocketName,
				FVector::ZeroVector,
				FRotator::ZeroRotator,
				EAttachLocation::SnapToTarget);
		}
		else
		{
			UGameplayStatics::PlaySoundAtLocation(this, FireConfig.FireSound, MuzzleTransform.GetLocation());
		}
	}
}

void APlayerCharacter::HandleHealthChanged(float OldValue, float NewValue)
{
	UE_LOG(LogShooterGame, Verbose, TEXT("%s health changed %.1f -> %.1f"), *GetName(), OldValue, NewValue);
}

void APlayerCharacter::BeginDeathPresentation(const FGameplayEventData* DeathEventData)
{
	(void)DeathEventData;

	if (!HasAuthority() || bIsDead)
	{
		return;
	}

	bIsDead = true;
	ApplyDeathPresentation();
}

void APlayerCharacter::ApplyDeathPresentation()
{
	if (bDeathHandled)
	{
		return;
	}

	bDeathHandled = true;

	const bool bFireInputStopped = CombatComponent != nullptr ? CombatComponent->HandleOwnerDeath() : false;
	if (bFireInputStopped)
	{
		OnFireInputStopped();
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

void APlayerCharacter::OnRep_IsDead()
{
	if (bIsDead)
	{
		ApplyDeathPresentation();
	}
}
