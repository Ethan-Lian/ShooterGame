// Copyright Epic Games, Inc. All Rights Reserved.

#include "Controller/ShooterPlayerController.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/ShooterAbilitySystemComponent.h"
#include "AbilitySystem/ShooterGameplayTags.h"
#include "Character/PlayerCharacter.h"
#include "EnhancedInputSubsystems.h"
#include "Components/ShooterCombatComponent.h"
#include "Components/ShooterMovementStateComponent.h"
#include "Components/ShooterWeaponEquipmentComponent.h"
#include "GameFramework/Character.h"
#include "HUD/ShooterHUD.h"
#include "Input/ShooterInputConfig.h"
#include "Input/ShooterInputComponent.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Interfaces/ShooterCombatInterface.h"
#include "Interfaces/ShooterEquipmentInterface.h"
#include "ShooterGame.h"
#include "UObject/ConstructorHelpers.h"

AShooterPlayerController::AShooterPlayerController()
{
	static ConstructorHelpers::FObjectFinder<UShooterInputConfig> DefaultInputConfig(
		TEXT("/Game/ShooterGameContent/DataConfig/DA_ShooterInputConfig.DA_ShooterInputConfig"));
	if (DefaultInputConfig.Succeeded())
	{
		InputConfig = DefaultInputConfig.Object;
	}
}

void AShooterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	NotifyHUDObservedPawnChanged();

	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (LocalPlayer == nullptr)
	{
		UE_LOG(LogShooterGame, Warning, TEXT("%s could not resolve LocalPlayer during BeginPlay."), *GetName());
		return;
	}

	if (InputConfig == nullptr)
	{
		UE_LOG(LogShooterGame, Warning, TEXT("%s has no InputConfig assigned."), *GetName());
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (InputSubsystem == nullptr)
	{
		UE_LOG(LogShooterGame, Warning, TEXT("%s could not resolve Enhanced Input subsystem."), *GetName());
		return;
	}

	for (const UInputMappingContext* MappingContext : InputConfig->MappingContexts)
	{
		if (MappingContext != nullptr)
		{
			InputSubsystem->AddMappingContext(MappingContext, InputConfig->MappingPriority);
		}
	}
}

void AShooterPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UShooterInputComponent* ShooterInputComponent = Cast<UShooterInputComponent>(InputComponent);
	if (ShooterInputComponent == nullptr)
	{
		UE_LOG(LogShooterGame, Error, TEXT("%s requires UShooterInputComponent."), *GetName());
		return;
	}

	ShooterInputComponent->RemoveBinds(InputBindHandles);

	if (InputConfig == nullptr)
	{
		UE_LOG(LogShooterGame, Warning, TEXT("%s cannot bind input because no InputConfig is assigned."), *GetName());
		return;
	}

	ShooterInputComponent->BindNativeAction(InputConfig, TAG_Input_Move, ETriggerEvent::Triggered, this, &AShooterPlayerController::HandleMove, InputBindHandles);
	ShooterInputComponent->BindNativeAction(InputConfig, TAG_Input_Move, ETriggerEvent::Completed, this, &AShooterPlayerController::HandleMoveCompleted, InputBindHandles);
	ShooterInputComponent->BindNativeAction(InputConfig, TAG_Input_Move, ETriggerEvent::Canceled, this, &AShooterPlayerController::HandleMoveCompleted, InputBindHandles);
	ShooterInputComponent->BindNativeAction(InputConfig, TAG_Input_Look, ETriggerEvent::Triggered, this, &AShooterPlayerController::HandleLook, InputBindHandles);
	ShooterInputComponent->BindNativeAction(InputConfig, TAG_Input_Jump, ETriggerEvent::Started, this, &AShooterPlayerController::HandleJumpStarted, InputBindHandles);
	ShooterInputComponent->BindNativeAction(InputConfig, TAG_Input_Jump, ETriggerEvent::Completed, this, &AShooterPlayerController::HandleJumpCompleted, InputBindHandles);
	ShooterInputComponent->BindNativeAction(InputConfig, TAG_Input_Jump, ETriggerEvent::Canceled, this, &AShooterPlayerController::HandleJumpCompleted, InputBindHandles);
	ShooterInputComponent->BindNativeAction(InputConfig, TAG_Input_Aim, ETriggerEvent::Started, this, &AShooterPlayerController::HandleAimStarted, InputBindHandles);
	ShooterInputComponent->BindNativeAction(InputConfig, TAG_Input_Aim, ETriggerEvent::Completed, this, &AShooterPlayerController::HandleAimCompleted, InputBindHandles);
	ShooterInputComponent->BindNativeAction(InputConfig, TAG_Input_Aim, ETriggerEvent::Canceled, this, &AShooterPlayerController::HandleAimCompleted, InputBindHandles);
	ShooterInputComponent->BindNativeAction(InputConfig, TAG_Input_Crouch, ETriggerEvent::Started, this, &AShooterPlayerController::HandleCrouchStarted, InputBindHandles);
	ShooterInputComponent->BindNativeAction(InputConfig, TAG_Input_Crouch, ETriggerEvent::Completed, this, &AShooterPlayerController::HandleCrouchCompleted, InputBindHandles);
	ShooterInputComponent->BindNativeAction(InputConfig, TAG_Input_Crouch, ETriggerEvent::Canceled, this, &AShooterPlayerController::HandleCrouchCompleted, InputBindHandles);
	ShooterInputComponent->BindAbilityActions(
		InputConfig,
		this,
		&AShooterPlayerController::HandleAbilityInputPressed,
		&AShooterPlayerController::HandleAbilityInputReleased,
		InputBindHandles);
}

void AShooterPlayerController::SetPawn(APawn* InPawn)
{
	Super::SetPawn(InPawn);
	NotifyHUDObservedPawnChanged();
}

void AShooterPlayerController::NotifyHUDObservedPawnChanged()
{
	if (!IsLocalController())
	{
		return;
	}

	if (AShooterHUD* ShooterHUD = GetHUD<AShooterHUD>())
	{
		ShooterHUD->SetObservedPawn(GetPawn());
	}
}

APlayerCharacter* AShooterPlayerController::GetPlayerCharacter() const
{
	return Cast<APlayerCharacter>(GetPawn());
}

ACharacter* AShooterPlayerController::GetControlledCharacter() const
{
	return Cast<ACharacter>(GetPawn());
}

UAbilitySystemComponent* AShooterPlayerController::GetControlledAbilitySystemComponent() const
{
	const IShooterCombatInterface* CombatOwner = Cast<IShooterCombatInterface>(GetPawn());
	return CombatOwner != nullptr ? CombatOwner->GetShooterAbilitySystemComponent() : nullptr;
}

UShooterAbilitySystemComponent* AShooterPlayerController::GetControlledShooterAbilitySystemComponent() const
{
	return Cast<UShooterAbilitySystemComponent>(GetControlledAbilitySystemComponent());
}

UShooterCombatComponent* AShooterPlayerController::GetControlledCombatComponent() const
{
	const IShooterCombatInterface* CombatOwner = Cast<IShooterCombatInterface>(GetPawn());
	return CombatOwner != nullptr ? CombatOwner->GetShooterCombatComponent() : nullptr;
}

UShooterMovementStateComponent* AShooterPlayerController::GetControlledMovementStateComponent() const
{
	const IShooterCombatInterface* CombatOwner = Cast<IShooterCombatInterface>(GetPawn());
	return CombatOwner != nullptr ? CombatOwner->GetShooterMovementStateComponent() : nullptr;
}

UShooterWeaponEquipmentComponent* AShooterPlayerController::GetControlledWeaponEquipmentComponent() const
{
	const IShooterEquipmentInterface* EquipmentOwner = Cast<IShooterEquipmentInterface>(GetPawn());
	return EquipmentOwner != nullptr ? EquipmentOwner->GetShooterWeaponEquipmentComponent() : nullptr;
}

bool AShooterPlayerController::IsControlledPawnDead() const
{
	const UAbilitySystemComponent* AbilitySystemComponent = GetControlledAbilitySystemComponent();
	return AbilitySystemComponent != nullptr && AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Dead);
}

void AShooterPlayerController::HandleMove(const FInputActionValue& InputValue)
{
	if (APlayerCharacter* PlayerCharacter = GetPlayerCharacter())
	{
		PlayerCharacter->Move(InputValue.Get<FVector2D>());
	}
}

void AShooterPlayerController::HandleMoveCompleted()
{
	if (APlayerCharacter* PlayerCharacter = GetPlayerCharacter())
	{
		PlayerCharacter->Move(FVector2D::ZeroVector);
	}
}

void AShooterPlayerController::HandleLook(const FInputActionValue& InputValue)
{
	if (APlayerCharacter* PlayerCharacter = GetPlayerCharacter())
	{
		PlayerCharacter->Look(InputValue.Get<FVector2D>());
	}
}

void AShooterPlayerController::HandleAbilityInputPressed(FGameplayTag InputTag)
{
	if (InputTag == TAG_Input_Fire)
	{
		if (UShooterCombatComponent* CombatComponent = GetControlledCombatComponent())
		{
			CombatComponent->StartFireInput();
		}
		return;
	}

	if (InputTag == TAG_Input_Sprint)
	{
		if (UShooterMovementStateComponent* MovementStateComponent = GetControlledMovementStateComponent())
		{
			MovementStateComponent->StartSprintInput();
		}
		return;
	}

	if (InputTag == TAG_Input_Interact)
	{
		if (UShooterWeaponEquipmentComponent* EquipmentComponent = GetControlledWeaponEquipmentComponent())
		{
			EquipmentComponent->StartPickupInput();
		}
		return;
	}

	if (InputTag == TAG_Input_Drop)
	{
		if (UShooterWeaponEquipmentComponent* EquipmentComponent = GetControlledWeaponEquipmentComponent())
		{
			EquipmentComponent->StartDropInput();
		}
		return;
	}

	if (UShooterAbilitySystemComponent* ShooterASC = GetControlledShooterAbilitySystemComponent())
	{
		ShooterASC->AbilityInputTagPressed(InputTag);
	}
}

void AShooterPlayerController::HandleAbilityInputReleased(FGameplayTag InputTag)
{
	if (InputTag == TAG_Input_Fire)
	{
		if (UShooterCombatComponent* CombatComponent = GetControlledCombatComponent())
		{
			CombatComponent->StopFireInput();
		}
		return;
	}

	if (InputTag == TAG_Input_Sprint)
	{
		if (UShooterMovementStateComponent* MovementStateComponent = GetControlledMovementStateComponent())
		{
			MovementStateComponent->StopSprintInput();
		}
		return;
	}

	if (InputTag == TAG_Input_Interact || InputTag == TAG_Input_Drop)
	{
		return;
	}

	if (UShooterAbilitySystemComponent* ShooterASC = GetControlledShooterAbilitySystemComponent())
	{
		ShooterASC->AbilityInputTagReleased(InputTag);
	}
}

void AShooterPlayerController::HandleJumpStarted()
{
	if (!IsControlledPawnDead())
	{
		if (ACharacter* ControlledCharacter = GetControlledCharacter())
		{
			ControlledCharacter->Jump();
		}
	}
}

void AShooterPlayerController::HandleJumpCompleted()
{
	if (!IsControlledPawnDead())
	{
		if (ACharacter* ControlledCharacter = GetControlledCharacter())
		{
			ControlledCharacter->StopJumping();
		}
	}
}

void AShooterPlayerController::HandleAimStarted()
{
	if (IsControlledPawnDead())
	{
		return;
	}

	if (UShooterMovementStateComponent* MovementStateComponent = GetControlledMovementStateComponent())
	{
		if (MovementStateComponent->IsSprinting())
		{
			MovementStateComponent->StopSprintInput();
		}
	}

	if (UShooterCombatComponent* CombatComponent = GetControlledCombatComponent())
	{
		CombatComponent->StartAimInput();
	}
}

void AShooterPlayerController::HandleAimCompleted()
{
	if (!IsControlledPawnDead())
	{
		if (UShooterCombatComponent* CombatComponent = GetControlledCombatComponent())
		{
			CombatComponent->StopAimInput();
		}
	}
}

void AShooterPlayerController::HandleCrouchStarted()
{
	if (IsControlledPawnDead())
	{
		return;
	}

	if (UShooterMovementStateComponent* MovementStateComponent = GetControlledMovementStateComponent())
	{
		if (MovementStateComponent->IsSprinting())
		{
			MovementStateComponent->StopSprintInput();
		}
	}

	if (ACharacter* ControlledCharacter = GetControlledCharacter())
	{
		ControlledCharacter->Crouch();
	}
}

void AShooterPlayerController::HandleCrouchCompleted()
{
	if (!IsControlledPawnDead())
	{
		if (ACharacter* ControlledCharacter = GetControlledCharacter())
		{
			ControlledCharacter->UnCrouch();
		}
	}
}
