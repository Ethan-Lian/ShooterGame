#pragma once

#include "GameFramework/Actor.h"
#include "ShooterWeaponEquipmentActor.generated.h"

class ACharacter;
class USceneComponent;
class UStaticMeshComponent;
class UWeaponDataAsset;

UCLASS()
class SHOOTERGAME_API AShooterWeaponEquipmentActor : public AActor
{
	GENERATED_BODY()

public:
	AShooterWeaponEquipmentActor();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Equipment supplies the Inventory definition before FinishSpawning.
	void InitializeFromDefinition(UWeaponDataAsset* InWeaponDefinition);
	const UWeaponDataAsset* GetWeaponDefinition() const { return WeaponDefinition; }
	UStaticMeshComponent* GetWeaponMesh() const { return WeaponMeshComponent; }
	FTransform GetMuzzleTransform() const;

	// Switches the actor into its character-equipped runtime state.
	void EnterEquippedState(ACharacter* NewOwnerCharacter);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<USceneComponent> SceneRootComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UStaticMeshComponent> WeaponMeshComponent;

private:
	void ApplyDefinitionPresentation();

	UFUNCTION()
	void OnRep_WeaponDefinition();

	// Remote observers need the definition for mesh, attachment and fire feedback.
	// Runtime ammo exists only in the PlayerState's Inventory entry.
	UPROPERTY(ReplicatedUsing = OnRep_WeaponDefinition, VisibleInstanceOnly, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWeaponDataAsset> WeaponDefinition;
};
