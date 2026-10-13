#include "Weapon/ShooterWeaponEquipmentActor.h"

#include "Components/SceneComponent.h"
#include "Components/ShooterWeaponEquipmentComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "Interfaces/ShooterEquipmentInterface.h"
#include "Net/UnrealNetwork.h"
#include "Weapon/WeaponDataAsset.h"

AShooterWeaponEquipmentActor::AShooterWeaponEquipmentActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(false);

	SceneRootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRootComponent);

	WeaponMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMeshComponent->SetupAttachment(SceneRootComponent);
	WeaponMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMeshComponent->SetGenerateOverlapEvents(false);
	WeaponMeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	WeaponMeshComponent->SetSimulatePhysics(false);
	WeaponMeshComponent->SetEnableGravity(false);
}

void AShooterWeaponEquipmentActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AShooterWeaponEquipmentActor, WeaponDefinition);
}

void AShooterWeaponEquipmentActor::InitializeFromDefinition(UWeaponDataAsset* InWeaponDefinition)
{
	WeaponDefinition = InWeaponDefinition;
	ApplyDefinitionPresentation();
}

FTransform AShooterWeaponEquipmentActor::GetMuzzleTransform() const
{
	return WeaponMeshComponent->GetSocketTransform(WeaponDefinition->FireConfig.MuzzleSocketName);
}

void AShooterWeaponEquipmentActor::EnterEquippedState(ACharacter* NewOwnerCharacter)
{
	// The actor pointer and its definition can arrive in either order on clients.
	if (WeaponDefinition == nullptr || NewOwnerCharacter == nullptr || NewOwnerCharacter->GetMesh() == nullptr)
	{
		return;
	}

	if (HasAuthority())
	{
		SetOwner(NewOwnerCharacter);
		SetInstigator(NewOwnerCharacter);
	}

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetActorHiddenInGame(false);
	AttachToComponent(
		NewOwnerCharacter->GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		WeaponDefinition->CharacterAttachSocketName);
	SetActorRelativeTransform(WeaponDefinition->WeaponMeshRelativeTransform);
}

void AShooterWeaponEquipmentActor::BeginPlay()
{
	Super::BeginPlay();
	ApplyDefinitionPresentation();
}

void AShooterWeaponEquipmentActor::ApplyDefinitionPresentation()
{
	if (WeaponDefinition == nullptr)
	{
		return;
	}

	WeaponMeshComponent->SetRelativeTransform(FTransform::Identity);
	WeaponMeshComponent->SetStaticMesh(WeaponDefinition->WeaponMesh.LoadSynchronous());
}

void AShooterWeaponEquipmentActor::OnRep_WeaponDefinition()
{
	ApplyDefinitionPresentation();
	// Equipment owns readiness/death checks and retries attachment after data arrives.
	if (const IShooterEquipmentInterface* EquipmentOwner = Cast<IShooterEquipmentInterface>(GetOwner()))
	{
		if (UShooterWeaponEquipmentComponent* Equipment = EquipmentOwner->GetShooterWeaponEquipmentComponent())
		{
			Equipment->RefreshEquipmentForPawnReady();
		}
	}
}
