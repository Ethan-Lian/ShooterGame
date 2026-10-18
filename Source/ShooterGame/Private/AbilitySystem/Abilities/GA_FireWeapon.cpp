#include "AbilitySystem/Abilities/GA_FireWeapon.h"
#include "AbilitySystem/ShooterGameplayTags.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/ShooterWeaponEquipmentComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Interfaces/ShooterEquipmentInterface.h"
#include "Weapon/ShooterWeaponEquipmentActor.h"
#include "Weapon/WeaponDataAsset.h"
#include "ShooterGame.h"
#include "Engine/World.h"
#include "Interfaces/ShooterCombatInterface.h"
#include "Components/ShooterCombatComponent.h"
#include "Components/ShooterInventoryComponent.h"
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

	float FireInterval = 0.f;
	if (FireSingleShot(FireInterval) && IsActive())
	{
		QueueNextShot(FireInterval);
	}
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

bool UGA_FireWeapon::FireSingleShot(float& OutFireInterval)
{
	APawn* ShooterPawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	const IShooterEquipmentInterface* EquipmentOwner = Cast<IShooterEquipmentInterface>(ShooterPawn);
	const UShooterWeaponEquipmentComponent* Equipment = EquipmentOwner != nullptr ? EquipmentOwner->GetShooterWeaponEquipmentComponent() : nullptr;
	UShooterInventoryComponent* Inventory = EquipmentOwner != nullptr ? EquipmentOwner->GetShooterInventoryComponent() : nullptr;
	const FWeaponInventoryEntry* Entry = Equipment != nullptr ? Equipment->GetEquippedInventoryEntry() : nullptr;
	AShooterWeaponEquipmentActor* EquippedWeaponActor = Equipment != nullptr ? Equipment->GetEquippedWeapon() : nullptr;
	UAbilitySystemComponent* SourceAbilitySystem = GetAbilitySystemComponentFromActorInfo();
	if (ShooterPawn == nullptr
		|| Entry == nullptr || !Entry->IsValid()
		|| EquippedWeaponActor == nullptr
		|| EquippedWeaponActor->GetWeaponDefinition() != Entry->WeaponDefinition
		|| SourceAbilitySystem == nullptr)
	{
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
		return false;
	}

	const int32 ItemId = Entry->ItemId;
	const FWeaponFireConfig& FireConfig = Entry->WeaponDefinition->FireConfig;
	if (FireConfig.DamageEffectClass == nullptr)
	{
		UE_LOG(LogShooterGame, Warning, TEXT("%s cannot fire because its weapon config is incomplete."), *ShooterPawn->GetName());
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
		return false;
	}

	const IShooterCombatInterface* CombatOwner = Cast<IShooterCombatInterface>(ShooterPawn);
	const UShooterCombatComponent* Combat = CombatOwner != nullptr ? CombatOwner->GetShooterCombatComponent() : nullptr;
	if (Entry->CurrentMagazineAmmo <= 0 || (Combat != nullptr && (Combat->IsWeaponInteractionBlocked() || Combat->IsReloading())))
	{
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
		return false;
	}
	if (!CommitFireShot())
	{
		return false;
	}
	// Commit can invoke GAS callbacks. Do not retain the borrowed entry across it.
	if (!IsActive() || GetAvatarActorFromActorInfo() != ShooterPawn
		|| Equipment->GetEquippedItemId() != ItemId || Equipment->GetEquippedWeapon() != EquippedWeaponActor
		|| (Combat != nullptr && (Combat->IsWeaponInteractionBlocked() || Combat->IsReloading()))
		|| !Inventory->ConsumeMagazineRound(ItemId))
	{
		if (IsActive())
		{
			CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
		}
		return false;
	}

	const FTransform MuzzleTransform = EquippedWeaponActor->GetMuzzleTransform();
	OutFireInterval = FireConfig.FireInterval;

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
	const AShooterWeaponEquipmentActor* EquippedWeaponActor,
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
	AShooterWeaponEquipmentActor* EquippedWeaponActor,
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
	AShooterWeaponEquipmentActor* EquippedWeaponActor,
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

	float FireInterval = 0.f;
	if (FireSingleShot(FireInterval) && IsActive())
	{
		QueueNextShot(FireInterval);
	}
}
