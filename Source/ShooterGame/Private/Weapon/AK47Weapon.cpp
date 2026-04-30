#include "Weapon/AK47Weapon.h"

#include "AbilitySystem/Effects/GE_DamageInstant.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Projectile/ShooterProjectileBase.h"

AAK47Weapon::AAK47Weapon()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> AK47Mesh(
		TEXT("/Game/FPS_Weapon_Bundle/Weapons/Meshes/Ka47/SM_KA47.SM_KA47"));

	if (AK47Mesh.Succeeded())
	{
		GetWeaponMesh()->SetStaticMesh(AK47Mesh.Object);
	}

	CharacterAttachSocketName = TEXT("hand_r");
	WeaponMeshRelativeTransform = FTransform(
		FRotator(0.f, 90.f, 0.f),
		FVector(4.f, 2.f, -2.f),
		FVector(1.f, 1.f, 1.f));

	FallbackFireConfig.BaseDamage = 25.f;
	FallbackFireConfig.FireInterval = 0.1f;
	FallbackFireConfig.MuzzleSocketName = TEXT("Muzzle");
	FallbackFireConfig.MuzzleFallbackTransform = FTransform(
		FRotator::ZeroRotator,
		FVector(60.f, 0.f, 8.f));
	FallbackFireConfig.ProjectileClass = AShooterProjectileBase::StaticClass();
	FallbackFireConfig.DamageEffectClass = UGE_DamageInstant::StaticClass();
	FallbackAmmoConfig.MagazineSize = 30;
	FallbackAmmoConfig.InitialReserveAmmo = 90;
}
