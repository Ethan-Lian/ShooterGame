// Fill out your copyright notice in the Description page of Project Settings.


#include "GameState/ShooterGameState.h"

#include "Net/UnrealNetwork.h"

void AShooterGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AShooterGameState, MatchEndServerTime);
	DOREPLIFETIME(AShooterGameState, ControlPointState);
	DOREPLIFETIME(AShooterGameState, MatchResults);
}

void AShooterGameState::SetMatchEndServerTime(double NewEndTime)
{
	check(HasAuthority());

	MatchEndServerTime = NewEndTime;
	ForceNetUpdate();
}

double AShooterGameState::GetRemainingMatchTime() const
{
	return FMath::Max(0.0, MatchEndServerTime - GetServerWorldTimeSeconds());
}

FName AShooterGameState::GetMatchPhase() const
{
	return GetMatchState();
}

bool AShooterGameState::HasMatchEndTime() const
{
	return MatchEndServerTime > 0.0;
}

bool AShooterGameState::IsMatchGameplayAllowed() const
{
	return IsMatchInProgress() && HasMatchEndTime() && GetRemainingMatchTime() > 0.0;
}

void AShooterGameState::ResetMatchResults()
{
	check(HasAuthority());
	MatchResults = {};
	ForceNetUpdate();
}

void AShooterGameState::SetMatchResults(const FShooterMatchResults& NewResults)
{
	check(HasAuthority());
	check(NewResults.bIsFinalized && !MatchResults.bIsFinalized);
	MatchResults = NewResults;
	ForceNetUpdate();
}

void AShooterGameState::SetControlPointState(const FShooterControlPointState& NewState)
{
	check(HasAuthority());
	ControlPointState = NewState;
	ForceNetUpdate();
}
