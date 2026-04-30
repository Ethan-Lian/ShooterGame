#include "Weapon/ShooterWeaponBase.h"

#include "Character/PlayerCharacter.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"

AShooterWeaponBase::AShooterWeaponBase()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = true;
	bOnlyRelevantToOwner = false;
	bNetUseOwnerRelevancy = false;
	NetDormancy = DORM_Awake;
	SetReplicateMovement(true);

	SceneRootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRootComponent);

	WeaponMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMeshComponent->SetupAttachment(SceneRootComponent);
	WeaponMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMeshComponent->SetGenerateOverlapEvents(false);
	WeaponMeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	WeaponMeshComponent->SetSimulatePhysics(false);
	WeaponMeshComponent->SetEnableGravity(false);

	PickupTrigger = CreateDefaultSubobject<USphereComponent>(TEXT("PickupTrigger"));
	PickupTrigger->SetupAttachment(SceneRootComponent);
	PickupTrigger->InitSphereRadius(PickupTriggerRadius);
	PickupTrigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PickupTrigger->SetCollisionObjectType(ECC_WorldDynamic);
	PickupTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	PickupTrigger->SetGenerateOverlapEvents(false);

	PickupWidget = CreateDefaultSubobject<UWidgetComponent>(FName("PickUpWidgetComponent"));
	PickupWidget->SetupAttachment(SceneRootComponent);
	PickupWidget->SetVisibility(false);
}

void AShooterWeaponBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AShooterWeaponBase, bIsWorldPickup);
	DOREPLIFETIME(AShooterWeaponBase, PickupData);
	DOREPLIFETIME(AShooterWeaponBase, DropPresentationData);
	DOREPLIFETIME(AShooterWeaponBase, bPickupInteractionEnabled);
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

void AShooterWeaponBase::EnterEquippedState(APlayerCharacter* NewOwnerCharacter)
{
	if (NewOwnerCharacter == nullptr || NewOwnerCharacter->GetMesh() == nullptr || WeaponMeshComponent == nullptr)
	{
		return;
	}

	if (HasAuthority())
	{
		bIsWorldPickup = false;
		bPickupInteractionEnabled = false;
		bOnlyRelevantToOwner = false;
		bNetUseOwnerRelevancy = false;
		SetNetDormancy(DORM_Awake);
		SetOwner(NewOwnerCharacter);
		SetInstigator(NewOwnerCharacter);
	}

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	bDropPresentationActive = false;
	SetActorTickEnabled(false);
	ApplyEquippedRuntimeState();
	AttachToComponent(
		NewOwnerCharacter->GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		GetAttachSocketName());
	SetActorRelativeTransform(GetEquippedRelativeTransform());
}

void AShooterWeaponBase::EnterWorldPickupState(const FTransform& WorldTransform)
{
	if (WeaponMeshComponent == nullptr)
	{
		return;
	}

	if (HasAuthority())
	{
		bIsWorldPickup = true;
		bPickupInteractionEnabled = true;
		bOnlyRelevantToOwner = false;
		bNetUseOwnerRelevancy = false;
		SetNetDormancy(DORM_Awake);
		SetOwner(nullptr);
		SetInstigator(nullptr);
	}

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	ApplyWorldPickupRuntimeState();
	SetActorTransform(WorldTransform, false, nullptr, ETeleportType::TeleportPhysics);

	if (HasAuthority())
	{
		ForceNetUpdate();
	}
}

void AShooterWeaponBase::SetPickupWidgetVisible(bool bVisible)
{
	if (PickupWidget == nullptr)
	{
		return;
	}

	PickupWidget->SetVisibility(bVisible && bIsWorldPickup);
}

void AShooterWeaponBase::SetDropPresentationData(const FWeaponDropPresentationData& NewDropPresentationData)
{
	DropPresentationData = NewDropPresentationData;
	BeginDropPresentation();
}

void AShooterWeaponBase::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		SeedPickupDataFromDefaults();
	}

	ApplyDataAssetPresentation();

	SetPickupWidgetVisible(false);

	if (bIsWorldPickup)
	{
		ApplyWorldPickupRuntimeState();
	}
	else
	{
		ApplyEquippedRuntimeState();
	}
}

void AShooterWeaponBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bDropPresentationActive)
	{
		SetActorTickEnabled(false);
		return;
	}

	DropPresentationElapsedTime += DeltaSeconds;
	const float Alpha = DropPresentationData.Duration > 0.f
		? FMath::Clamp(DropPresentationElapsedTime / DropPresentationData.Duration, 0.f, 1.f)
		: 1.f;

	UpdateDropPresentationVisual(Alpha);

	if (Alpha >= 1.f)
	{
		FinishDropPresentation();
	}
}

void AShooterWeaponBase::ApplyDataAssetPresentation()
{
	const UWeaponDataAsset* WeaponDefinition = GetPickupData().WeaponDefinition;
	if (WeaponDefinition == nullptr)
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

void AShooterWeaponBase::ApplyEquippedRuntimeState()
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

	if (PickupTrigger != nullptr)
	{
		PickupTrigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PickupTrigger->SetGenerateOverlapEvents(false);
	}

	SetPickupWidgetVisible(false);
}

void AShooterWeaponBase::ApplyWorldPickupRuntimeState()
{
	if (WeaponMeshComponent == nullptr)
	{
		return;
	}

	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	SetReplicateMovement(true);
	if (!bDropPresentationActive)
	{
		WeaponMeshComponent->SetRelativeTransform(GetDroppedMeshRelativeTransform());
	}
	WeaponMeshComponent->SetSimulatePhysics(false);
	WeaponMeshComponent->SetEnableGravity(false);
	WeaponMeshComponent->SetPhysicsLinearVelocity(FVector::ZeroVector);
	WeaponMeshComponent->SetCollisionEnabled(bPickupInteractionEnabled ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	WeaponMeshComponent->SetGenerateOverlapEvents(false);
	WeaponMeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
	WeaponMeshComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	WeaponMeshComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	if (PickupTrigger != nullptr)
	{
		PickupTrigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PickupTrigger->SetGenerateOverlapEvents(false);
	}

	SetPickupWidgetVisible(false);
}

void AShooterWeaponBase::BeginDropPresentation()
{
	if (!bIsWorldPickup || !DropPresentationData.IsValid())
	{
		return;
	}

	bPickupInteractionEnabled = false;
	bDropPresentationActive = true;
	DropPresentationElapsedTime = 0.f;
	SetActorTickEnabled(true);
	ApplyWorldPickupRuntimeState();
	UpdateDropPresentationVisual(0.f);

	if (HasAuthority())
	{
		ForceNetUpdate();
	}
}

void AShooterWeaponBase::FinishDropPresentation()
{
	bDropPresentationActive = false;
	bPickupInteractionEnabled = true;
	SetActorTickEnabled(false);

	if (WeaponMeshComponent != nullptr)
	{
		WeaponMeshComponent->SetRelativeTransform(DropPresentationData.FinalMeshRelativeTransform);
	}

	ApplyWorldPickupRuntimeState();

	if (HasAuthority())
	{
		ForceNetUpdate();
	}
}

void AShooterWeaponBase::UpdateDropPresentationVisual(float Alpha)
{
	if (WeaponMeshComponent == nullptr)
	{
		return;
	}

	const FVector EndLocation = DropPresentationData.EndLocation;
	const FVector LinearWorldLocation = FMath::Lerp(DropPresentationData.StartLocation, EndLocation, Alpha);
	const float ArcOffset = FMath::Sin(Alpha * PI) * DropPresentationData.ArcHeight;
	const FVector VisualWorldLocation = LinearWorldLocation + FVector(0.f, 0.f, ArcOffset);
	const FVector WorldOffsetFromRoot = VisualWorldLocation - EndLocation;
	const FVector LocalOffsetFromRoot = GetActorTransform().InverseTransformVectorNoScale(WorldOffsetFromRoot);

	FTransform VisualMeshTransform = DropPresentationData.FinalMeshRelativeTransform;
	VisualMeshTransform.AddToTranslation(LocalOffsetFromRoot);
	WeaponMeshComponent->SetRelativeTransform(VisualMeshTransform);
}

void AShooterWeaponBase::OnRep_IsWorldPickup()
{
	if (bIsWorldPickup)
	{
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		ApplyWorldPickupRuntimeState();
		return;
	}

	ApplyEquippedRuntimeState();
}

void AShooterWeaponBase::OnRep_PickupData()
{
	ApplyDataAssetPresentation();
}

void AShooterWeaponBase::OnRep_DropPresentationData()
{
	BeginDropPresentation();
}

void AShooterWeaponBase::OnRep_PickupInteractionEnabled()
{
	if (bIsWorldPickup && !bDropPresentationActive)
	{
		ApplyWorldPickupRuntimeState();
	}
}
