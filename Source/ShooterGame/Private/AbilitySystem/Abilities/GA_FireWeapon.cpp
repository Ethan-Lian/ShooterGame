#include "AbilitySystem/Abilities/GA_FireWeapon.h"
#include "AbilitySystem/ShooterGameplayTags.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Character/PlayerCharacter.h"
#include "Components/ShooterWeaponEquipmentComponent.h"
#include "GameFramework/Controller.h"
#include "Projectile/ShooterProjectileBase.h"
#include "Weapon/ShooterWeaponBase.h"
#include "Weapon/WeaponDataAsset.h"
#include "Weapon/ShooterWeaponInstance.h"
#include "ShooterGame.h"
#include "Engine/World.h"
#include "GameplayCueManager.h"
#include "TimerManager.h"

UGA_FireWeapon::UGA_FireWeapon()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	FGameplayTagContainer AbilityTagContainer;
	AbilityTagContainer.AddTag(TAG_Ability_Weapon_Fire);
	SetAssetTags(AbilityTagContainer);
	ActivationBlockedTags.AddTag(TAG_State_Dead);
}

void UGA_FireWeapon::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	APlayerCharacter* ShooterCharacter = Cast<APlayerCharacter>(GetAvatarActorFromActorInfo());
	UShooterWeaponEquipmentComponent* EquipmentComponent = ShooterCharacter != nullptr
		? ShooterCharacter->GetWeaponEquipmentComponent()
		: nullptr;
	UShooterWeaponInstance* EquippedWeaponInstance = EquipmentComponent != nullptr
		? EquipmentComponent->GetEquippedWeaponInstance()
		: nullptr;
	if (ShooterCharacter == nullptr || EquippedWeaponInstance == nullptr)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FireSingleShot();

	const float FireInterval = FMath::Max(0.01f, EquippedWeaponInstance->GetFireConfig().FireInterval);
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			RepeatingFireTimerHandle,
			this,
			&UGA_FireWeapon::HandleRepeatedFire,
			FireInterval,
			true,
			FireInterval);
	}
}

void UGA_FireWeapon::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RepeatingFireTimerHandle);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_FireWeapon::FireSingleShot()
{
	APlayerCharacter* ShooterCharacter = Cast<APlayerCharacter>(GetAvatarActorFromActorInfo());
	UShooterWeaponEquipmentComponent* EquipmentComponent = ShooterCharacter != nullptr
		? ShooterCharacter->GetWeaponEquipmentComponent()
		: nullptr;
	UShooterWeaponInstance* EquippedWeaponInstance = EquipmentComponent != nullptr
		? EquipmentComponent->GetEquippedWeaponInstance()
		: nullptr;
	AShooterWeaponBase* EquippedWeaponActor = EquippedWeaponInstance != nullptr
		? EquippedWeaponInstance->GetEquippedWeaponActor()
		: nullptr;
	UAbilitySystemComponent* SourceAbilitySystem = GetAbilitySystemComponentFromActorInfo();
	if (ShooterCharacter == nullptr
		|| EquippedWeaponInstance == nullptr
		|| EquippedWeaponActor == nullptr
		|| SourceAbilitySystem == nullptr)
	{
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
		return;
	}

	const FWeaponFireConfig& FireConfig = EquippedWeaponInstance->GetFireConfig();
	if (FireConfig.DamageEffectClass == nullptr
		|| (FireConfig.FireMode == EWeaponFireMode::Projectile && FireConfig.ProjectileClass == nullptr))
	{
		UE_LOG(LogShooterGame, Warning, TEXT("%s cannot fire because its weapon config is incomplete."), *ShooterCharacter->GetName());
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
		return;
	}

	const FTransform MuzzleTransform = EquippedWeaponInstance->GetMuzzleTransform();

	if (FireConfig.FireMode == EWeaponFireMode::Projectile)
	{
		FireProjectileShot(ShooterCharacter, EquippedWeaponActor, SourceAbilitySystem, FireConfig, MuzzleTransform);
		return;
	}

	FireHitscanShot(ShooterCharacter, EquippedWeaponActor, SourceAbilitySystem, FireConfig, MuzzleTransform);
}

bool UGA_FireWeapon::ResolveAimPoint(
	const APlayerCharacter* ShooterCharacter,
	const AShooterWeaponBase* EquippedWeaponActor,
	const FWeaponFireConfig& FireConfig,
	FVector& OutAimPoint) const
{
	UWorld* World = GetWorld();
	if (ShooterCharacter == nullptr || World == nullptr || FireConfig.AimTraceDistance <= 0.f)
	{
		return false;
	}

	FVector ViewLocation = ShooterCharacter->GetPawnViewLocation();
	FRotator ViewRotation = ShooterCharacter->GetBaseAimRotation();

	if (const AController* OwnerController = ShooterCharacter->GetController())
	{
		OwnerController->GetPlayerViewPoint(ViewLocation, ViewRotation);
	}

	const FVector TraceStart = ViewLocation;
	const FVector TraceEnd = TraceStart + (ViewRotation.Vector() * FireConfig.AimTraceDistance);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WeaponCameraAimTrace), false, ShooterCharacter);
	QueryParams.AddIgnoredActor(ShooterCharacter);
	if (EquippedWeaponActor != nullptr)
	{
		QueryParams.AddIgnoredActor(EquippedWeaponActor);
	}

	FHitResult AimHitResult;
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	const bool bHitBlockingObject = World->LineTraceSingleByObjectType(
		AimHitResult,
		TraceStart,
		TraceEnd,
		ObjectQueryParams,
		QueryParams);

	OutAimPoint = bHitBlockingObject ? AimHitResult.ImpactPoint : TraceEnd;
	return true;
}

void UGA_FireWeapon::FireHitscanShot(
	APlayerCharacter* ShooterCharacter,
	AShooterWeaponBase* EquippedWeaponActor,
	UAbilitySystemComponent* SourceAbilitySystem,
	const FWeaponFireConfig& FireConfig,
	const FTransform& MuzzleTransform)
{
	UWorld* World = GetWorld();
	if (ShooterCharacter == nullptr || SourceAbilitySystem == nullptr || World == nullptr)
	{
		return;
	}

	const FVector MuzzleLocation = MuzzleTransform.GetLocation();
	FVector AimPoint = FVector::ZeroVector;
	if (!ResolveAimPoint(ShooterCharacter, EquippedWeaponActor, FireConfig, AimPoint))
	{
		const float FallbackTraceDistance = FireConfig.AimTraceDistance > 0.f ? FireConfig.AimTraceDistance : 100000.f;
		AimPoint = MuzzleLocation + (ShooterCharacter->GetBaseAimRotation().Vector() * FallbackTraceDistance);
	}

	FVector ShotDirection = (AimPoint - MuzzleLocation).GetSafeNormal();
	if (ShotDirection.IsNearlyZero())
	{
		ShotDirection = ShooterCharacter->GetBaseAimRotation().Vector();
	}

	ExecuteFireCue(ShooterCharacter, EquippedWeaponActor, SourceAbilitySystem, MuzzleLocation, ShotDirection);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WeaponMuzzleDamageTrace), false, ShooterCharacter);
	QueryParams.AddIgnoredActor(ShooterCharacter);
	if (EquippedWeaponActor != nullptr)
	{
		QueryParams.AddIgnoredActor(EquippedWeaponActor);
	}

	FHitResult HitResult;
	const FVector TraceEnd = AimPoint;
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	const bool bHitBlockingObject = World->LineTraceSingleByObjectType(
		HitResult,
		MuzzleLocation,
		TraceEnd,
		ObjectQueryParams,
		QueryParams);

	if (!bHitBlockingObject)
	{
		return;
	}

	AActor* HitActor = HitResult.GetActor();
	if (HitActor == nullptr || HitActor == ShooterCharacter || HitActor == EquippedWeaponActor)
	{
		return;
	}

	UAbilitySystemComponent* TargetAbilitySystem = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);
	if (TargetAbilitySystem == nullptr || FireConfig.BaseDamage <= 0.f)
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = SourceAbilitySystem->MakeEffectContext();
	EffectContext.AddInstigator(ShooterCharacter, EquippedWeaponActor);
	EffectContext.AddSourceObject(EquippedWeaponActor);
	EffectContext.AddHitResult(HitResult, true);

	FGameplayEffectSpecHandle DamageSpecHandle = SourceAbilitySystem->MakeOutgoingSpec(FireConfig.DamageEffectClass, 1.f, EffectContext);
	if (!DamageSpecHandle.IsValid())
	{
		UE_LOG(LogShooterGame, Warning, TEXT("%s failed to create hitscan damage spec."), *ShooterCharacter->GetName());
		return;
	}

	DamageSpecHandle.Data->SetSetByCallerMagnitude(TAG_Data_Damage, FireConfig.BaseDamage);
	SourceAbilitySystem->ApplyGameplayEffectSpecToTarget(*DamageSpecHandle.Data.Get(), TargetAbilitySystem);
}

void UGA_FireWeapon::FireProjectileShot(
	APlayerCharacter* ShooterCharacter,
	AShooterWeaponBase* EquippedWeaponActor,
	UAbilitySystemComponent* SourceAbilitySystem,
	const FWeaponFireConfig& FireConfig,
	const FTransform& MuzzleTransform)
{
	if (ShooterCharacter == nullptr || SourceAbilitySystem == nullptr || FireConfig.ProjectileClass == nullptr)
	{
		return;
	}

	const FRotator SpawnRotation = ShooterCharacter->GetBaseAimRotation();
	const FTransform SpawnTransform(SpawnRotation, MuzzleTransform.GetLocation());

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = ShooterCharacter;
	SpawnParameters.Instigator = ShooterCharacter;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AShooterProjectileBase* Projectile = GetWorld()->SpawnActor<AShooterProjectileBase>(
		FireConfig.ProjectileClass,
		SpawnTransform,
		SpawnParameters);

	if (Projectile == nullptr)
	{
		UE_LOG(LogShooterGame, Warning, TEXT("%s failed to spawn a projectile."), *ShooterCharacter->GetName());
		return;
	}

	Projectile->InitializeProjectile(SourceAbilitySystem, FireConfig.DamageEffectClass, FireConfig.BaseDamage);

	ExecuteFireCue(ShooterCharacter, EquippedWeaponActor, SourceAbilitySystem, MuzzleTransform.GetLocation(), SpawnRotation.Vector());
}

void UGA_FireWeapon::ExecuteFireCue(
	APlayerCharacter* ShooterCharacter,
	AShooterWeaponBase* EquippedWeaponActor,
	UAbilitySystemComponent* SourceAbilitySystem,
	const FVector& MuzzleLocation,
	const FVector& ShotDirection) const
{
	if (ShooterCharacter == nullptr || SourceAbilitySystem == nullptr)
	{
		return;
	}

	FGameplayCueParameters FireCueParameters;
	FireCueParameters.Instigator = ShooterCharacter;
	FireCueParameters.EffectCauser = EquippedWeaponActor;
	FireCueParameters.SourceObject = EquippedWeaponActor;
	FireCueParameters.Location = MuzzleLocation;
	FireCueParameters.Normal = ShotDirection;
	SourceAbilitySystem->ExecuteGameplayCue(TAG_GameplayCue_Weapon_Fire, FireCueParameters);
}

void UGA_FireWeapon::HandleRepeatedFire()
{
	FireSingleShot();
}
