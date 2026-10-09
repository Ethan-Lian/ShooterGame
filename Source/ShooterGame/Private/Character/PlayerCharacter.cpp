#include "Character/PlayerCharacter.h"
#include "AbilitySystem/ShooterGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraTypes.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
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
	PrimaryActorTick.bCanEverTick = true;

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

	FirstPersonArms = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonArms"));
	FirstPersonArms->SetupAttachment(FollowCamera);
	FirstPersonArms->SetOnlyOwnerSee(true);
	FirstPersonArms->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FirstPersonArms->SetCastShadow(false);
	FirstPersonArms->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
	FirstPersonArms->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	FirstPersonWeapon = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonWeapon"));
	FirstPersonWeapon->SetupAttachment(FirstPersonArms, TEXT("ik_hand_gun"));
	FirstPersonWeapon->SetOnlyOwnerSee(true);
	FirstPersonWeapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FirstPersonWeapon->SetCastShadow(false);
	FirstPersonWeapon->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);

	FirstPersonMagazine = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FirstPersonMagazine"));
	FirstPersonMagazine->SetupAttachment(FirstPersonWeapon, TEXT("Magazine"));
	FirstPersonMagazine->SetOnlyOwnerSee(true);
	FirstPersonMagazine->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FirstPersonMagazine->SetCastShadow(false);
	FirstPersonMagazine->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);

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
	LookSway = InputValue;
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

	bUseControllerRotationYaw = true;
	MovementComponent->bOrientRotationToMovement = false;
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
	RefreshFirstPersonPresentation();

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

	// Override the serialized third-person spring-arm setup on the existing BP.
	FollowCamera->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	FollowCamera->SetRelativeLocation(FVector(0.f, 0.f, BaseEyeHeight));
	FollowCamera->SetRelativeRotation(FRotator::ZeroRotator);
	FollowCamera->bUsePawnControlRotation = true;
	FollowCamera->SetFieldOfView(100.f);
	FollowCamera->SetEnableFirstPersonFieldOfView(true);
	FollowCamera->SetFirstPersonFieldOfView(100.f);
	FollowCamera->SetEnableFirstPersonScale(true);
	FollowCamera->SetFirstPersonScale(0.25f);
	// Close view models must remain sharp regardless of the level's cinematic DOF.
	FollowCamera->PostProcessSettings.bOverride_DepthOfFieldEnabled = true;
	FollowCamera->PostProcessSettings.DepthOfFieldEnabled = false;
	CameraBoom->SetComponentTickEnabled(false);
	bUseControllerRotationYaw = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetMesh()->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::WorldSpaceRepresentation);
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	// The configured view mesh contains only our character's arms, rebound to the
	// template skeleton. Replacing it with the world body loses that binding.
	for (int32 Index = 0; Index < GetMesh()->GetNumMaterials(); ++Index)
	{
		FirstPersonArms->SetMaterial(Index, GetMesh()->GetMaterial(Index));
	}
	ViewMeshLocation = FirstPersonMeshOffset;
	FirstPersonArms->SetRelativeLocation(ViewMeshLocation);
	FirstPersonArms->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	FirstPersonWeapon->AttachToComponent(FirstPersonArms, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("ik_hand_gun"));
	FirstPersonWeapon->SetRelativeTransform(FirstPersonWeaponTransform);
	FirstPersonMagazine->AttachToComponent(FirstPersonWeapon, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("Magazine"));
	RefreshFirstPersonPresentation();

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
	StopFirstPersonReload();
	RefreshFirstPersonPresentation();

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
	RefreshFirstPersonPresentation();

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

FVector APlayerCharacter::GetPawnViewLocation() const
{
	return GetActorLocation() + FVector(0.f, 0.f, BaseEyeHeight);
}

void APlayerCharacter::CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult)
{
	Super::CalcCamera(DeltaTime, OutResult);
	// First-person rendering compresses mesh depth by 0.25. The default 10 cm
	// near plane clips away the hands and weapon; override it for this view only.
	OutResult.PerspectiveNearClipPlane = 0.25f;
}

void APlayerCharacter::RefreshFirstPersonPresentation()
{
	const bool bShowView = IsLocallyControlled() && !bDeathHandled && IsEquipped();
	FirstPersonArms->SetVisibility(bShowView);
	FirstPersonWeapon->SetVisibility(bShowView, true);
	FirstPersonArms->SetComponentTickEnabled(bShowView);
	FirstPersonWeapon->SetComponentTickEnabled(bShowView);
	if (AShooterWeaponEquipmentActor* Weapon = GetEquippedWeapon())
	{
		Weapon->GetWeaponMesh()->SetOwnerNoSee(true);
	}
}

void APlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// The server camera must also follow crouched eye height for aim traces.
	FollowCamera->SetRelativeLocation(FVector(0.f, 0.f, BaseEyeHeight));
	if (!IsLocallyControlled() || bDeathHandled)
	{
		return;
	}

	AimBlend = FMath::FInterpTo(AimBlend, IsAiming() ? 1.f : 0.f, DeltaSeconds, 12.f);
	FollowCamera->SetFieldOfView(FMath::Lerp(100.f, 70.f, AimBlend));
	FollowCamera->SetFirstPersonFieldOfView(FMath::Lerp(100.f, 70.f, AimBlend));
	ViewMeshLocation = FMath::VInterpTo(ViewMeshLocation, FMath::Lerp(FirstPersonMeshOffset, FirstPersonAimOffset, AimBlend), DeltaSeconds, 12.f);
	WeaponRecoil = FMath::FInterpTo(WeaponRecoil, 0.f, DeltaSeconds, 14.f);
	LookSway = FMath::Vector2DInterpTo(LookSway, FVector2D::ZeroVector, DeltaSeconds, 10.f);
	FirstPersonArms->SetRelativeLocation(ViewMeshLocation + FVector(-WeaponRecoil * 2.f, -LookSway.X * 0.15f, LookSway.Y * 0.15f));
	FirstPersonArms->SetRelativeRotation(FRotator(WeaponRecoil, -90.f - LookSway.X * 0.15f, -LookSway.Y * 0.15f));
	CameraRecoil = FMath::FInterpTo(CameraRecoil, 0.f, DeltaSeconds, 5.f);
	if (Controller != nullptr)
	{
		FRotator Rotation = Controller->GetControlRotation();
		Rotation.Pitch += CameraRecoil - AppliedCameraRecoil;
		Controller->SetControlRotation(Rotation);
		AppliedCameraRecoil = CameraRecoil;
	}
}

void APlayerCharacter::PlayFirstPersonFire()
{
	if (!IsLocallyControlled() || bDeathHandled)
	{
		return;
	}
	WeaponRecoil = FMath::Min(WeaponRecoil + (IsAiming() ? 0.7f : 1.2f), 4.f);
	CameraRecoil = FMath::Min(CameraRecoil + (IsAiming() ? 0.35f : 0.65f), 5.f);
	UAnimMontage* FireMontage = IsAiming() ? FirstPersonAimedFireMontage : FirstPersonFireMontage;
	if (UAnimInstance* Anim = FirstPersonArms->GetAnimInstance(); Anim != nullptr && FireMontage != nullptr)
	{
		Anim->Montage_Play(FireMontage);
	}
	if (FirstPersonWeaponFire != nullptr)
	{
		FirstPersonWeapon->PlayAnimation(FirstPersonWeaponFire, false);
	}
}

float APlayerCharacter::GetReloadDuration() const
{
	return FirstPersonReloadMontage != nullptr ? FirstPersonReloadMontage->GetPlayLength() : 0.f;
}

void APlayerCharacter::PlayFirstPersonReload(float ElapsedSeconds)
{
	if (!IsLocallyControlled() || bDeathHandled)
	{
		return;
	}
	if (UAnimInstance* Anim = FirstPersonArms->GetAnimInstance(); Anim != nullptr && FirstPersonReloadMontage != nullptr)
	{
		Anim->Montage_Play(FirstPersonReloadMontage, 1.f, EMontagePlayReturnType::MontageLength, ElapsedSeconds);
	}
	if (FirstPersonWeaponReload != nullptr)
	{
		FirstPersonWeapon->PlayAnimation(FirstPersonWeaponReload, false);
		FirstPersonWeapon->SetPosition(ElapsedSeconds);
	}
}

void APlayerCharacter::StopFirstPersonReload()
{
	if (UAnimInstance* Anim = FirstPersonArms->GetAnimInstance(); Anim != nullptr && FirstPersonReloadMontage != nullptr)
	{
		Anim->Montage_Stop(0.15f, FirstPersonReloadMontage);
	}
	FirstPersonWeapon->Stop();
	FirstPersonWeapon->SetPosition(0.f);
}
