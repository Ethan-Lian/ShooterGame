#include "Weapon/ShooterWeaponBase.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "Weapon/ShooterWeaponEquipmentActor.h"
#include "Weapon/ShooterWeaponPickupActor.h"

AShooterWeaponBase::AShooterWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bOnlyRelevantToOwner = false;
	bNetUseOwnerRelevancy = false;
	NetDormancy = DORM_Awake;

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

void AShooterWeaponBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AShooterWeaponBase, PickupData);
}

FWeaponPickupData AShooterWeaponBase::GetPickupData() const
{
	FWeaponPickupData ResolvedPickupData = PickupData;
	if (!ResolvedPickupData.HasValidDefinition())
	{
		ResolvedPickupData.WeaponDefinition = WeaponDataAsset;
	}

	return ResolvedPickupData;
}

void AShooterWeaponBase::SetPickupData(const FWeaponPickupData& NewPickupData)
{
	PickupData = NewPickupData;
	ApplyDataAssetPresentation();
}

TSubclassOf<AShooterWeaponPickupActor> AShooterWeaponBase::GetPickupActorClass() const
{
	if (const UWeaponDataAsset* WeaponDefinition = GetPickupData().WeaponDefinition)
	{
		if (WeaponDefinition->PickupActorClass != nullptr)
		{
			return WeaponDefinition->PickupActorClass;
		}
	}

	if (PickupActorClass != nullptr)
	{
		return PickupActorClass;
	}

	if (GetClass()->IsChildOf(AShooterWeaponPickupActor::StaticClass()))
	{
		return TSubclassOf<AShooterWeaponPickupActor>(GetClass());
	}

	return AShooterWeaponPickupActor::StaticClass();
}

TSubclassOf<AShooterWeaponEquipmentActor> AShooterWeaponBase::GetEquipmentActorClass() const
{
	if (const UWeaponDataAsset* WeaponDefinition = GetPickupData().WeaponDefinition)
	{
		if (WeaponDefinition->EquipmentActorClass != nullptr)
		{
			return WeaponDefinition->EquipmentActorClass;
		}
	}

	if (EquipmentActorClass != nullptr)
	{
		return EquipmentActorClass;
	}

	if (GetClass()->IsChildOf(AShooterWeaponEquipmentActor::StaticClass()))
	{
		return TSubclassOf<AShooterWeaponEquipmentActor>(GetClass());
	}

	return AShooterWeaponEquipmentActor::StaticClass();
}

const FWeaponFireConfig& AShooterWeaponBase::GetFireConfig() const
{
	const UWeaponDataAsset* WeaponDefinition = GetPickupData().WeaponDefinition;
	return WeaponDefinition != nullptr ? WeaponDefinition->FireConfig : FallbackFireConfig;
}

const FWeaponAmmoConfig& AShooterWeaponBase::GetAmmoConfig() const
{
	const UWeaponDataAsset* WeaponDefinition = GetPickupData().WeaponDefinition;
	return WeaponDefinition != nullptr ? WeaponDefinition->AmmoConfig : FallbackAmmoConfig;
}

FTransform AShooterWeaponBase::GetMuzzleTransform() const
{
	if (WeaponMeshComponent == nullptr)
	{
		return GetActorTransform();
	}

	const FWeaponFireConfig& FireConfig = GetFireConfig();
	if (!FireConfig.MuzzleSocketName.IsNone() && WeaponMeshComponent->DoesSocketExist(FireConfig.MuzzleSocketName))
	{
		return WeaponMeshComponent->GetSocketTransform(FireConfig.MuzzleSocketName);
	}

	return FireConfig.MuzzleFallbackTransform * WeaponMeshComponent->GetComponentTransform();
}

FName AShooterWeaponBase::GetAttachSocketName() const
{
	const UWeaponDataAsset* WeaponDefinition = GetPickupData().WeaponDefinition;
	return WeaponDefinition != nullptr ? WeaponDefinition->CharacterAttachSocketName : CharacterAttachSocketName;
}

FTransform AShooterWeaponBase::GetEquippedRelativeTransform() const
{
	const UWeaponDataAsset* WeaponDefinition = GetPickupData().WeaponDefinition;
	return WeaponDefinition != nullptr ? WeaponDefinition->WeaponMeshRelativeTransform : WeaponMeshRelativeTransform;
}

FTransform AShooterWeaponBase::GetDroppedMeshRelativeTransform() const
{
	const UWeaponDataAsset* WeaponDefinition = GetPickupData().WeaponDefinition;
	return WeaponDefinition != nullptr ? WeaponDefinition->DroppedMeshRelativeTransform : DroppedMeshRelativeTransform;
}

UStaticMeshComponent* AShooterWeaponBase::GetWeaponMesh() const
{
	return WeaponMeshComponent;
}

void AShooterWeaponBase::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		SeedPickupDataFromDefaults();
	}

	ApplyDataAssetPresentation();
}

void AShooterWeaponBase::ApplyDataAssetPresentation()
{
	const UWeaponDataAsset* WeaponDefinition = GetPickupData().WeaponDefinition;
	if (WeaponDefinition == nullptr || WeaponMeshComponent == nullptr)
	{
		return;
	}

	if (!WeaponDefinition->WeaponMesh.IsNull())
	{
		if (UStaticMesh* WeaponMesh = WeaponDefinition->WeaponMesh.LoadSynchronous())
		{
			WeaponMeshComponent->SetStaticMesh(WeaponMesh);
		}
	}
}

void AShooterWeaponBase::SeedPickupDataFromDefaults()
{
	if (PickupData.HasValidDefinition())
	{
		return;
	}

	PickupData.WeaponDefinition = WeaponDataAsset;
	if (PickupData.WeaponDefinition == nullptr)
	{
		return;
	}

	PickupData.CurrentMagazineAmmo = PickupData.WeaponDefinition->AmmoConfig.MagazineSize;
	PickupData.CurrentReserveAmmo = PickupData.WeaponDefinition->AmmoConfig.InitialReserveAmmo;
}

void AShooterWeaponBase::OnRep_PickupData()
{
	ApplyDataAssetPresentation();
}
