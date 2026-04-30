#include "Projectile/ShooterProjectileBase.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/ShooterGameplayTags.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "ShooterGame.h"

AShooterProjectileBase::AShooterProjectileBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	CollisionComponent->InitSphereRadius(5.f);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComponent->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	SetRootComponent(CollisionComponent);

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->InitialSpeed = InitialSpeed;
	ProjectileMovementComponent->MaxSpeed = InitialSpeed;
	ProjectileMovementComponent->ProjectileGravityScale = 0.f;
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->bShouldBounce = false;
	ProjectileMovementComponent->SetUpdatedComponent(CollisionComponent);
}

void AShooterProjectileBase::InitializeProjectile(
	UAbilitySystemComponent* InSourceAbilitySystem,
	TSubclassOf<UGameplayEffect> InDamageEffectClass,
	float InBaseDamage)
{
	SourceAbilitySystemComponent = InSourceAbilitySystem;
	DamageEffectClass = InDamageEffectClass;
	BaseDamage = InBaseDamage;

	if (AActor* OwningActor = GetOwner())
	{
		CollisionComponent->IgnoreActorWhenMoving(OwningActor, true);
	}

	if (APawn* InstigatorPawn = GetInstigator())
	{
		CollisionComponent->IgnoreActorWhenMoving(InstigatorPawn, true);
	}

	ProjectileMovementComponent->Velocity = GetActorForwardVector() * InitialSpeed;
}

void AShooterProjectileBase::BeginPlay()
{
	Super::BeginPlay();

	ProjectileMovementComponent->OnProjectileStop.AddDynamic(this, &AShooterProjectileBase::HandleProjectileStop);
}

void AShooterProjectileBase::HandleProjectileStop(const FHitResult& ImpactResult)
{
	if (bHasProcessedImpact)
	{
		return;
	}

	bHasProcessedImpact = true;

	if (HasAuthority())
	{
		ApplyImpactDamage(ImpactResult);
	}

	Destroy();
}

void AShooterProjectileBase::ApplyImpactDamage(const FHitResult& ImpactResult)
{
	AActor* HitActor = ImpactResult.GetActor();
	if (HitActor == nullptr || HitActor == GetOwner() || HitActor == GetInstigator())
	{
		return;
	}

	UAbilitySystemComponent* SourceAbilitySystem = SourceAbilitySystemComponent.Get();
	if (SourceAbilitySystem == nullptr || DamageEffectClass == nullptr || BaseDamage <= 0.f)
	{
		return;
	}

	UAbilitySystemComponent* TargetAbilitySystem = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);
	if (TargetAbilitySystem == nullptr)
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = SourceAbilitySystem->MakeEffectContext();
	EffectContext.AddInstigator(GetInstigator(), GetOwner());
	EffectContext.AddSourceObject(this);
	EffectContext.AddHitResult(ImpactResult, true);

	FGameplayEffectSpecHandle DamageSpecHandle = SourceAbilitySystem->MakeOutgoingSpec(DamageEffectClass, 1.f, EffectContext);
	if (!DamageSpecHandle.IsValid())
	{
		UE_LOG(LogShooterGame, Warning, TEXT("%s failed to create damage spec."), *GetName());
		return;
	}

	DamageSpecHandle.Data->SetSetByCallerMagnitude(TAG_Data_Damage, BaseDamage);
	SourceAbilitySystem->ApplyGameplayEffectSpecToTarget(*DamageSpecHandle.Data.Get(), TargetAbilitySystem);
}
