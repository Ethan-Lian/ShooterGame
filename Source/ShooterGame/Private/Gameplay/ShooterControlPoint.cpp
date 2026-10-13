#include "Gameplay/ShooterControlPoint.h"

#include "Character/PlayerCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/ShooterPawnExtensionComponent.h"
#include "Engine/World.h"
#include "GameMode/ShooterGameMode.h"
#include "PlayerState/ShooterPlayerState.h"

AShooterControlPoint::AShooterControlPoint()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(false);

	CaptureVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("CaptureVolume"));
	SetRootComponent(CaptureVolume);
	CaptureVolume->InitBoxExtent(FVector(400.f, 400.f, 150.f));
	// ControlPoint object channel in DefaultEngine.ini; weapon object queries exclude it.
	CaptureVolume->SetCollisionObjectType(ECC_GameTraceChannel1);
	CaptureVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CaptureVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	CaptureVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CaptureVolume->SetGenerateOverlapEvents(true);
	CaptureVolume->ShapeColor = FColor::Cyan;
	CaptureVolume->SetHiddenInGame(false);

	PointLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("PointLabel"));
	PointLabel->SetupAttachment(CaptureVolume);
	PointLabel->SetRelativeLocation(FVector(0.f, 0.f, 200.f));
	PointLabel->SetHorizontalAlignment(EHTA_Center);
	PointLabel->SetWorldSize(60.f);
	PointLabel->SetTextRenderColor(FColor::Cyan);
	PointLabel->SetText(NSLOCTEXT("ShooterControlPoint", "Label", "CONTROL POINT"));
}

void AShooterControlPoint::BeginPlay()
{
	Super::BeginPlay();
	if (!HasAuthority())
	{
		return;
	}

	CaptureVolume->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::HandleBeginOverlap);
	CaptureVolume->OnComponentEndOverlap.AddDynamic(this, &ThisClass::HandleEndOverlap);

	// Overlaps can already exist before delegates are bound.
	TArray<UPrimitiveComponent*> Components;
	CaptureVolume->GetOverlappingComponents(Components);
	for (UPrimitiveComponent* Component : Components)
	{
		APlayerCharacter* Character = Cast<APlayerCharacter>(Component->GetOwner());
		if (Character != nullptr && Component == Character->GetCapsuleComponent())
		{
			TrackCharacter(Character);
		}
	}

	if (AShooterGameMode* GameMode = GetWorld()->GetAuthGameMode<AShooterGameMode>())
	{
		GameMode->RegisterControlPoint(this);
	}
}

void AShooterControlPoint::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority())
	{
		CaptureVolume->OnComponentBeginOverlap.RemoveDynamic(this, &ThisClass::HandleBeginOverlap);
		CaptureVolume->OnComponentEndOverlap.RemoveDynamic(this, &ThisClass::HandleEndOverlap);
		for (const TWeakObjectPtr<APlayerCharacter>& Character : OverlappingCharacters)
		{
			if (Character.IsValid())
			{
				Character->OnEndPlay.RemoveDynamic(this, &ThisClass::HandleCharacterEndPlay);
			}
		}
		OverlappingCharacters.Reset();
		if (AShooterGameMode* GameMode = GetWorld()->GetAuthGameMode<AShooterGameMode>())
		{
			GameMode->UnregisterControlPoint(this);
		}
	}
	Super::EndPlay(EndPlayReason);
}

FShooterControlPointState AShooterControlPoint::EvaluateOccupancy() const
{
	check(HasAuthority());
	
	TArray<APlayerCharacter*> Characters;
	GetOverlappingCharacters(Characters);
	
	TSet<AShooterPlayerState*> LivingPlayers;
	
	for (APlayerCharacter* Character : Characters)
	{
		AShooterPlayerState* PlayerState =
			Character->GetPlayerState<AShooterPlayerState>();

		const UShooterPawnExtensionComponent* Extension =
			Character->GetPawnExtensionComponent();

		if (PlayerState == nullptr
			|| PlayerState->GetPawn() != Character
			|| Character->GetController() == nullptr
			|| Extension == nullptr
			|| !Extension->IsGameplayReady()
			|| Character->IsDead())
		{
			continue;
		}

		LivingPlayers.Add(PlayerState);
	}
	
	FShooterControlPointState ControlPointState;

	if (LivingPlayers.Num() == 1)
	{
		ControlPointState.Status = EShooterControlPointStatus::Held;
		ControlPointState.SolePlayer = *LivingPlayers.CreateConstIterator();
	}
	else if (LivingPlayers.Num() > 1)
	{
		ControlPointState.Status = EShooterControlPointStatus::Contested;
	}

	return ControlPointState;
}

void AShooterControlPoint::GetOverlappingCharacters(TArray<APlayerCharacter*>& OutCharacters) const
{
	OutCharacters.Reset();
	for (const TWeakObjectPtr<APlayerCharacter>& Character : OverlappingCharacters)
	{
		if (Character.IsValid())
		{
			OutCharacters.Add(Character.Get());
		}
	}
}

void AShooterControlPoint::TrackCharacter(APlayerCharacter* Character)
{
	if (!OverlappingCharacters.Contains(Character))
	{
		// Record membership before subscribing to another Actor's lifecycle.
		OverlappingCharacters.Add(Character);
		Character->OnEndPlay.AddUniqueDynamic(this, &ThisClass::HandleCharacterEndPlay);
	}
}

void AShooterControlPoint::RemoveCharacter(APlayerCharacter* Character)
{
	check(HasAuthority());
	if (Character != nullptr && OverlappingCharacters.Remove(Character) > 0)
	{
		Character->OnEndPlay.RemoveDynamic(this, &ThisClass::HandleCharacterEndPlay);
		NotifyOccupancyChanged();
	}
}

void AShooterControlPoint::NotifyOccupancyChanged()
{
	if (AShooterGameMode* GameMode = GetWorld()->GetAuthGameMode<AShooterGameMode>())
	{
		GameMode->NotifyControlPointOccupancyChanged(this);
	}
}

void AShooterControlPoint::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	APlayerCharacter* Character = Cast<APlayerCharacter>(OtherActor);
	if (Character != nullptr && OtherComponent == Character->GetCapsuleComponent())
	{
		TrackCharacter(Character);
		NotifyOccupancyChanged();
	}
}

void AShooterControlPoint::HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex)
{
	APlayerCharacter* Character = Cast<APlayerCharacter>(OtherActor);
	if (Character != nullptr && OtherComponent == Character->GetCapsuleComponent())
	{
		RemoveCharacter(Character);
	}
}

void AShooterControlPoint::HandleCharacterEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason)
{
	RemoveCharacter(Cast<APlayerCharacter>(Actor));
}
