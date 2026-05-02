#include "Messages/ShooterGameplayMessageSubsystem.h"

#include "AbilitySystem/ShooterGameplayTags.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

UShooterGameplayMessageSubsystem* UShooterGameplayMessageSubsystem::Get(const UObject* WorldContextObject)
{
	if (WorldContextObject == nullptr || GEngine == nullptr)
	{
		return nullptr;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	return World != nullptr ? World->GetSubsystem<UShooterGameplayMessageSubsystem>() : nullptr;
}

FDelegateHandle UShooterGameplayMessageSubsystem::RegisterPlayerDeathListener(const FShooterPlayerDeathMessageDelegate::FDelegate& Listener)
{
	return PlayerDeathMessageDelegate.Add(Listener);
}

void UShooterGameplayMessageSubsystem::UnregisterPlayerDeathListener(FDelegateHandle ListenerHandle)
{
	if (ListenerHandle.IsValid())
	{
		PlayerDeathMessageDelegate.Remove(ListenerHandle);
	}
}

void UShooterGameplayMessageSubsystem::BroadcastPlayerDeath(const FShooterPlayerDeathMessage& Message)
{
	if (Message.Channel != TAG_Message_Player_Death)
	{
		return;
	}

	PlayerDeathMessageDelegate.Broadcast(Message);
}
