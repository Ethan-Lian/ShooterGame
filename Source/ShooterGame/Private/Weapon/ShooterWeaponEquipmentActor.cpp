#include "Weapon/ShooterWeaponEquipmentActor.h"

#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"

AShooterWeaponEquipmentActor::AShooterWeaponEquipmentActor()
{
	SetReplicateMovement(false);
}

void AShooterWeaponEquipmentActor::EnterEquippedState(ACharacter* NewOwnerCharacter)
{
	if (NewOwnerCharacter == nullptr || NewOwnerCharacter->GetMesh() == nullptr || WeaponMeshComponent == nullptr)
	{
		return;
	}

	if (HasAuthority())
	{
		bOnlyRelevantToOwner = false;
		bNetUseOwnerRelevancy = false;
		SetNetDormancy(DORM_Awake);
		SetOwner(NewOwnerCharacter);
		SetInstigator(NewOwnerCharacter);
	}

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetActorTickEnabled(false);
	ApplyEquippedRuntimeState();
	AttachToComponent(
		NewOwnerCharacter->GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		GetAttachSocketName());
	SetActorRelativeTransform(GetEquippedRelativeTransform());
}

void AShooterWeaponEquipmentActor::BeginPlay()
{
	Super::BeginPlay();
	ApplyEquippedRuntimeState();
}

void AShooterWeaponEquipmentActor::ApplyEquippedRuntimeState()
{
	if (WeaponMeshComponent == nullptr)
	{
		return;
	}

	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	SetReplicateMovement(false);
	WeaponMeshComponent->SetRelativeTransform(FTransform::Identity);
	WeaponMeshComponent->SetSimulatePhysics(false);
	WeaponMeshComponent->SetEnableGravity(false);
	WeaponMeshComponent->SetPhysicsLinearVelocity(FVector::ZeroVector);
	WeaponMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMeshComponent->SetGenerateOverlapEvents(false);
	WeaponMeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
}
