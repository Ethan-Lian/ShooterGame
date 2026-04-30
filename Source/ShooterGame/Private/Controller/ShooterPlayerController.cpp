// Copyright Epic Games, Inc. All Rights Reserved.

#include "Controller/ShooterPlayerController.h"

#include "Character/PlayerCharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "HUD/ShooterHUD.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "ShooterGame.h"
#include "UObject/ConstructorHelpers.h"

AShooterPlayerController::AShooterPlayerController()
{
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> MappingContextRef(
		TEXT("/Game/ShooterGameContent/Input/IMC_Shootergame.IMC_Shootergame"));
	static ConstructorHelpers::FObjectFinder<UInputAction> MoveActionRef(
		TEXT("/Game/ShooterGameContent/Input/Actions/IA_Move.IA_Move"));
	static ConstructorHelpers::FObjectFinder<UInputAction> LookActionRef(
		TEXT("/Game/ShooterGameContent/Input/Actions/IA_Look.IA_Look"));
	static ConstructorHelpers::FObjectFinder<UInputAction> FireActionRef(
		TEXT("/Game/ShooterGameContent/Input/Actions/IA_Fire.IA_Fire"));
	static ConstructorHelpers::FObjectFinder<UInputAction> JumpActionRef(
		TEXT("/Game/ShooterGameContent/Input/Actions/IA_Jump.IA_Jump"));

	if (MappingContextRef.Succeeded())
	{
		DefaultMappingContext = MappingContextRef.Object;
	}

	if (MoveActionRef.Succeeded())
	{
		MoveAction = MoveActionRef.Object;
	}

	if (LookActionRef.Succeeded())
	{
		LookAction = LookActionRef.Object;
	}

	if (FireActionRef.Succeeded())
	{
		FireAction = FireActionRef.Object;
	}

	if (JumpActionRef.Succeeded())
	{
		JumpAction = JumpActionRef.Object;
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

	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (InputSubsystem == nullptr)
	{
		UE_LOG(LogShooterGame, Warning, TEXT("%s could not resolve Enhanced Input subsystem."), *GetName());
		return;
	}

	EnsureRuntimeInputBindings();

	if (DefaultMappingContext != nullptr)
	{
		InputSubsystem->AddMappingContext(DefaultMappingContext, 0);
	}
	else
	{
		UE_LOG(LogShooterGame, Warning, TEXT("%s has no DefaultMappingContext assigned."), *GetName());
	}

	if (RuntimeInputMappingContext != nullptr)
	{
		InputSubsystem->AddMappingContext(RuntimeInputMappingContext, 1);
	}
}

void AShooterPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (EnhancedInputComponent == nullptr)
	{
		UE_LOG(LogShooterGame, Error, TEXT("%s requires an EnhancedInputComponent."), *GetName());
		return;
	}

	if (MoveAction != nullptr)
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AShooterPlayerController::HandleMove);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &AShooterPlayerController::HandleMoveCompleted);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Canceled, this, &AShooterPlayerController::HandleMoveCompleted);
	}

	if (LookAction != nullptr)
	{
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AShooterPlayerController::HandleLook);
	}

	if (FireAction != nullptr)
	{
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &AShooterPlayerController::HandleFireStarted);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &AShooterPlayerController::HandleFireCompleted);
	}

	if (JumpAction != nullptr)
	{
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AShooterPlayerController::HandleJumpStarted);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AShooterPlayerController::HandleJumpCompleted);
	}

	if (AimAction != nullptr)
	{
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &AShooterPlayerController::HandleAimStarted);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &AShooterPlayerController::HandleAimCompleted);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Canceled, this, &AShooterPlayerController::HandleAimCompleted);
	}

	if (CrouchAction != nullptr)
	{
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &AShooterPlayerController::HandleCrouchStarted);
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Completed, this, &AShooterPlayerController::HandleCrouchCompleted);
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Canceled, this, &AShooterPlayerController::HandleCrouchCompleted);
	}

	if (SprintAction != nullptr)
	{
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &AShooterPlayerController::HandleSprintStarted);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &AShooterPlayerController::HandleSprintCompleted);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Canceled, this, &AShooterPlayerController::HandleSprintCompleted);
	}

	if (PickupWeaponAction != nullptr)
	{
		EnhancedInputComponent->BindAction(PickupWeaponAction, ETriggerEvent::Started, this, &AShooterPlayerController::HandlePickupStarted);
	}

	if (DropWeaponAction != nullptr)
	{
		EnhancedInputComponent->BindAction(DropWeaponAction, ETriggerEvent::Started, this, &AShooterPlayerController::HandleDropStarted);
	}
}

void AShooterPlayerController::SetPawn(APawn* InPawn)
{
	Super::SetPawn(InPawn);
	NotifyHUDObservedPawnChanged();
}

void AShooterPlayerController::EnsureRuntimeInputBindings()
{
	if (RuntimeInputMappingContext == nullptr)
	{
		RuntimeInputMappingContext = NewObject<UInputMappingContext>(this, TEXT("RuntimeInputMappingContext"));
	}

	if (PickupWeaponAction == nullptr)
	{
		PickupWeaponAction = NewObject<UInputAction>(this, TEXT("RuntimePickupWeaponAction"));
		PickupWeaponAction->ValueType = EInputActionValueType::Boolean;
	}

	if (DropWeaponAction == nullptr)
	{
		DropWeaponAction = NewObject<UInputAction>(this, TEXT("RuntimeDropWeaponAction"));
		DropWeaponAction->ValueType = EInputActionValueType::Boolean;
	}

	if (AimAction == nullptr)
	{
		AimAction = NewObject<UInputAction>(this, TEXT("RuntimeAimAction"));
		AimAction->ValueType = EInputActionValueType::Boolean;
	}

	if (CrouchAction == nullptr)
	{
		CrouchAction = NewObject<UInputAction>(this, TEXT("RuntimeCrouchAction"));
		CrouchAction->ValueType = EInputActionValueType::Boolean;
	}

	EnsureActionMapped(RuntimeInputMappingContext, PickupWeaponAction, EKeys::F);
	EnsureActionMapped(RuntimeInputMappingContext, DropWeaponAction, EKeys::Q);
	EnsureActionMapped(RuntimeInputMappingContext, AimAction, EKeys::RightMouseButton);
	EnsureActionMapped(RuntimeInputMappingContext, CrouchAction, EKeys::C);
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

bool AShooterPlayerController::HasActionMapped(const UInputMappingContext* MappingContext, UInputAction* Action, const FKey& Key) const
{
	if (MappingContext == nullptr || Action == nullptr)
	{
		return false;
	}

	for (const FEnhancedActionKeyMapping& Mapping : MappingContext->GetMappings())
	{
		if (Mapping.Action == Action && Mapping.Key == Key)
		{
			return true;
		}
	}

	return false;
}

void AShooterPlayerController::EnsureActionMapped(UInputMappingContext* MappingContext, UInputAction* Action, const FKey& Key) const
{
	if (MappingContext == nullptr || Action == nullptr)
	{
		return;
	}

	if (HasActionMapped(DefaultMappingContext, Action, Key) || HasActionMapped(MappingContext, Action, Key))
	{
		return;
	}

	MappingContext->MapKey(Action, Key);
}

APlayerCharacter* AShooterPlayerController::GetPlayerCharacter() const
{
	return Cast<APlayerCharacter>(GetPawn());
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

void AShooterPlayerController::HandleFireStarted()
{
	if (APlayerCharacter* PlayerCharacter = GetPlayerCharacter())
	{
		PlayerCharacter->StartFireInput();
	}
}

void AShooterPlayerController::HandleFireCompleted()
{
	if (APlayerCharacter* PlayerCharacter = GetPlayerCharacter())
	{
		PlayerCharacter->StopFireInput();
	}
}

void AShooterPlayerController::HandleJumpStarted()
{
	if (APlayerCharacter* PlayerCharacter = GetPlayerCharacter())
	{
		PlayerCharacter->StartJumpInput();
	}
}

void AShooterPlayerController::HandleJumpCompleted()
{
	if (APlayerCharacter* PlayerCharacter = GetPlayerCharacter())
	{
		PlayerCharacter->StopJumpInput();
	}
}

void AShooterPlayerController::HandleAimStarted()
{
	if (APlayerCharacter* PlayerCharacter = GetPlayerCharacter())
	{
		PlayerCharacter->StartAimInput();
	}
}

void AShooterPlayerController::HandleAimCompleted()
{
	if (APlayerCharacter* PlayerCharacter = GetPlayerCharacter())
	{
		PlayerCharacter->StopAimInput();
	}
}

void AShooterPlayerController::HandleCrouchStarted()
{
	if (APlayerCharacter* PlayerCharacter = GetPlayerCharacter())
	{
		PlayerCharacter->StartCrouchInput();
	}
}

void AShooterPlayerController::HandleCrouchCompleted()
{
	if (APlayerCharacter* PlayerCharacter = GetPlayerCharacter())
	{
		PlayerCharacter->StopCrouchInput();
	}
}

void AShooterPlayerController::HandleSprintStarted()
{
	if (APlayerCharacter* PlayerCharacter = GetPlayerCharacter())
	{
		PlayerCharacter->StartSprintInput();
	}
}

void AShooterPlayerController::HandleSprintCompleted()
{
	if (APlayerCharacter* PlayerCharacter = GetPlayerCharacter())
	{
		PlayerCharacter->StopSprintInput();
	}
}

void AShooterPlayerController::HandlePickupStarted()
{
	if (APlayerCharacter* PlayerCharacter = GetPlayerCharacter())
	{
		PlayerCharacter->StartPickupInput();
	}
}

void AShooterPlayerController::HandleDropStarted()
{
	if (APlayerCharacter* PlayerCharacter = GetPlayerCharacter())
	{
		PlayerCharacter->StartDropInput();
	}
}
