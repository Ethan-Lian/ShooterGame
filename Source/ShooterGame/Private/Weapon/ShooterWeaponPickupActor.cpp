#include "Weapon/ShooterWeaponPickupActor.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"

AShooterWeaponPickupActor::AShooterWeaponPickupActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	SetReplicateMovement(true);

	PickupTrigger = CreateDefaultSubobject<USphereComponent>(TEXT("PickupTrigger"));
	PickupTrigger->SetupAttachment(SceneRootComponent);
	PickupTrigger->InitSphereRadius(PickupTriggerRadius);
	PickupTrigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PickupTrigger->SetCollisionObjectType(ECC_WorldDynamic);
	PickupTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	PickupTrigger->SetGenerateOverlapEvents(false);

	PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickUpWidgetComponent"));
	PickupWidget->SetupAttachment(SceneRootComponent);
	PickupWidget->SetVisibility(false);
}

void AShooterWeaponPickupActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AShooterWeaponPickupActor, DropPresentationData);
	DOREPLIFETIME(AShooterWeaponPickupActor, bPickupInteractionEnabled);
}

void AShooterWeaponPickupActor::EnterWorldPickupState(const FTransform& WorldTransform)
{
	if (WeaponMeshComponent == nullptr)
	{
		return;
	}

	if (HasAuthority())
	{
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

void AShooterWeaponPickupActor::SetDropPresentationData(const FWeaponDropPresentationData& NewDropPresentationData)
{
	DropPresentationData = NewDropPresentationData;
	BeginDropPresentation();
}

void AShooterWeaponPickupActor::SetPickupWidgetVisible(bool bVisible)
{
	if (PickupWidget == nullptr)
	{
		return;
	}

	PickupWidget->SetVisibility(bVisible);
}

void AShooterWeaponPickupActor::BeginPlay()
{
	Super::BeginPlay();

	SetPickupWidgetVisible(false);
	ApplyWorldPickupRuntimeState();
}

void AShooterWeaponPickupActor::Tick(float DeltaSeconds)
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

void AShooterWeaponPickupActor::ApplyWorldPickupRuntimeState()
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

void AShooterWeaponPickupActor::BeginDropPresentation()
{
	if (!DropPresentationData.IsValid())
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

void AShooterWeaponPickupActor::FinishDropPresentation()
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

void AShooterWeaponPickupActor::UpdateDropPresentationVisual(float Alpha)
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

void AShooterWeaponPickupActor::OnRep_DropPresentationData()
{
	BeginDropPresentation();
}

void AShooterWeaponPickupActor::OnRep_PickupInteractionEnabled()
{
	if (!bDropPresentationActive)
	{
		ApplyWorldPickupRuntimeState();
	}
}
