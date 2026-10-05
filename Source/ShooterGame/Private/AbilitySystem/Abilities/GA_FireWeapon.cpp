#include "AbilitySystem/Abilities/GA_FireWeapon.h"
#include "AbilitySystem/ShooterGameplayTags.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/ShooterWeaponEquipmentComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Interfaces/ShooterEquipmentInterface.h"
#include "Weapon/ShooterWeaponBase.h"
#include "Weapon/ShooterWeaponEquipmentActor.h"
#include "Weapon/WeaponDataAsset.h"
#include "Weapon/ShooterWeaponInstance.h"
#include "ShooterGame.h"
#include "Engine/World.h"
#include "GameplayCueManager.h"

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

	APawn* ShooterPawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	UShooterWeaponInstance* EquippedWeaponInstance = GetEquippedWeaponInstance();
	if (ShooterPawn == nullptr || EquippedWeaponInstance == nullptr)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!FireSingleShot())
	{
		return;
	}

	const float FireInterval = FMath::Max(0.01f, EquippedWeaponInstance->GetFireConfig().FireInterval);
	QueueNextShot(FireInterval);
}

void UGA_FireWeapon::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (FireDelayTask != nullptr)
	{
		FireDelayTask->EndTask();
		FireDelayTask = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

UShooterWeaponInstance* UGA_FireWeapon::GetEquippedWeaponInstance() const
{
	const APawn* ShooterPawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	const IShooterEquipmentInterface* EquipmentOwner = Cast<IShooterEquipmentInterface>(ShooterPawn);
	const UShooterWeaponEquipmentComponent* EquipmentComponent = EquipmentOwner != nullptr
		? EquipmentOwner->GetShooterWeaponEquipmentComponent()
		: nullptr;
	return EquipmentComponent != nullptr ? EquipmentComponent->GetEquippedWeaponInstance() : nullptr;
}

bool UGA_FireWeapon::CommitFireShot()
{
	CostGameplayEffectClass = FireCostGameplayEffectClass;
	CooldownGameplayEffectClass = FireCooldownGameplayEffectClass;

	FGameplayTagContainer FailureTags;
	if (!CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, &FailureTags))
	{
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
		return false;
	}

	return true;
}

bool UGA_FireWeapon::FireSingleShot()
{
	APawn* ShooterPawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	UShooterWeaponInstance* EquippedWeaponInstance = GetEquippedWeaponInstance();
	AShooterWeaponBase* EquippedWeaponActor = EquippedWeaponInstance != nullptr
		? EquippedWeaponInstance->GetEquippedWeaponActor()
		: nullptr;
	UAbilitySystemComponent* SourceAbilitySystem = GetAbilitySystemComponentFromActorInfo();
	if (ShooterPawn == nullptr
		|| EquippedWeaponInstance == nullptr
		|| EquippedWeaponActor == nullptr
		|| SourceAbilitySystem == nullptr)
	{
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
		return false;
	}

	const FWeaponFireConfig& FireConfig = EquippedWeaponInstance->GetFireConfig();
	if (FireConfig.DamageEffectClass == nullptr)
	{
		UE_LOG(LogShooterGame, Warning, TEXT("%s cannot fire because its weapon config is incomplete."), *ShooterPawn->GetName());
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
		return false;
	}

	if (!CommitFireShot())
	{
		return false;
	}

	const FTransform MuzzleTransform = EquippedWeaponInstance->GetMuzzleTransform();

	FireHitscanShot(ShooterPawn, EquippedWeaponActor, SourceAbilitySystem, FireConfig, MuzzleTransform);
	return true;
}

void UGA_FireWeapon::QueueNextShot(float FireInterval)
{
	if (FireDelayTask != nullptr)
	{
		FireDelayTask->EndTask();
		FireDelayTask = nullptr;
	}

	FireDelayTask = UAbilityTask_WaitDelay::WaitDelay(this, FMath::Max(0.01f, FireInterval));
	if (FireDelayTask == nullptr)
	{
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
		return;
	}

	FireDelayTask->OnFinish.AddDynamic(this, &UGA_FireWeapon::HandleRepeatedFire);
	FireDelayTask->ReadyForActivation();
}

bool UGA_FireWeapon::ResolveAimPoint(
	const APawn* ShooterPawn,
	const AShooterWeaponBase* EquippedWeaponActor,
	const FWeaponFireConfig& FireConfig,
	FVector& OutAimPoint) const
{
	UWorld* World = GetWorld();
	if (ShooterPawn == nullptr || World == nullptr || FireConfig.AimTraceDistance <= 0.f)
	{
		return false;
	}

	FVector ViewLocation = ShooterPawn->GetPawnViewLocation();
	FRotator ViewRotation = ShooterPawn->GetBaseAimRotation();

	if (const AController* OwnerController = ShooterPawn->GetController())
	{
		OwnerController->GetPlayerViewPoint(ViewLocation, ViewRotation);
	}

	const FVector TraceStart = ViewLocation;
	const FVector TraceEnd = TraceStart + (ViewRotation.Vector() * FireConfig.AimTraceDistance);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WeaponCameraAimTrace), false, ShooterPawn);
	QueryParams.AddIgnoredActor(ShooterPawn);
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
	APawn* ShooterPawn,
	AShooterWeaponBase* EquippedWeaponActor,
	UAbilitySystemComponent* SourceAbilitySystem,
	const FWeaponFireConfig& FireConfig,
	const FTransform& MuzzleTransform)
{
	UWorld* World = GetWorld();
	if (ShooterPawn == nullptr || SourceAbilitySystem == nullptr || World == nullptr)
	{
		return;
	}

	const FVector MuzzleLocation = MuzzleTransform.GetLocation();
	FVector AimPoint = FVector::ZeroVector;
	if (!ResolveAimPoint(ShooterPawn, EquippedWeaponActor, FireConfig, AimPoint))
	{
		const float FallbackTraceDistance = FireConfig.AimTraceDistance > 0.f ? FireConfig.AimTraceDistance : 100000.f;
		AimPoint = MuzzleLocation + (ShooterPawn->GetBaseAimRotation().Vector() * FallbackTraceDistance);
	}

	FVector ShotDirection = (AimPoint - MuzzleLocation).GetSafeNormal();
	if (ShotDirection.IsNearlyZero())
	{
		ShotDirection = ShooterPawn->GetBaseAimRotation().Vector();
	}

	ExecuteFireCue(ShooterPawn, EquippedWeaponActor, SourceAbilitySystem, MuzzleLocation, ShotDirection);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WeaponMuzzleDamageTrace), false, ShooterPawn);
	QueryParams.AddIgnoredActor(ShooterPawn);
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
	if (HitActor == nullptr || HitActor == ShooterPawn || HitActor == EquippedWeaponActor)
	{
		return;
	}

	UAbilitySystemComponent* TargetAbilitySystem = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);
	if (TargetAbilitySystem == nullptr || FireConfig.BaseDamage <= 0.f)
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = SourceAbilitySystem->MakeEffectContext();
	EffectContext.AddInstigator(ShooterPawn, EquippedWeaponActor);
	EffectContext.AddSourceObject(EquippedWeaponActor);
	EffectContext.AddHitResult(HitResult, true);

	FGameplayEffectSpecHandle DamageSpecHandle = SourceAbilitySystem->MakeOutgoingSpec(FireConfig.DamageEffectClass, 1.f, EffectContext);
	if (!DamageSpecHandle.IsValid())
	{
		UE_LOG(LogShooterGame, Warning, TEXT("%s failed to create hitscan damage spec."), *ShooterPawn->GetName());
		return;
	}

	DamageSpecHandle.Data->SetSetByCallerMagnitude(TAG_Data_Damage, FireConfig.BaseDamage);
	SourceAbilitySystem->ApplyGameplayEffectSpecToTarget(*DamageSpecHandle.Data.Get(), TargetAbilitySystem);
}

void UGA_FireWeapon::ExecuteFireCue(
	APawn* ShooterPawn,
	AShooterWeaponBase* EquippedWeaponActor,
	UAbilitySystemComponent* SourceAbilitySystem,
	const FVector& MuzzleLocation,
	const FVector& ShotDirection) const
{
	if (ShooterPawn == nullptr || SourceAbilitySystem == nullptr)
	{
		return;
	}

	FGameplayCueParameters FireCueParameters;
	FireCueParameters.Instigator = ShooterPawn;
	FireCueParameters.EffectCauser = EquippedWeaponActor;
	FireCueParameters.SourceObject = EquippedWeaponActor;
	FireCueParameters.Location = MuzzleLocation;
	FireCueParameters.Normal = ShotDirection;
	SourceAbilitySystem->ExecuteGameplayCue(TAG_GameplayCue_Weapon_Fire, FireCueParameters);
}

void UGA_FireWeapon::HandleRepeatedFire()
{
	FireDelayTask = nullptr;

	UShooterWeaponInstance* EquippedWeaponInstance = GetEquippedWeaponInstance();
	if (EquippedWeaponInstance == nullptr || !FireSingleShot())
	{
		return;
	}

	const float FireInterval = FMath::Max(0.01f, EquippedWeaponInstance->GetFireConfig().FireInterval);
	QueueNextShot(FireInterval);
}
