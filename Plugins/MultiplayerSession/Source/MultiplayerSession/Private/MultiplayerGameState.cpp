#include "MultiplayerGameState.h"
#include "Engine/Engine.h"

void AMultiplayerGameState::MulticastBroadcastConnectionMessage_Implementation(const FString& Message, bool bIsJoinMessage)
{
	ShowConnectionMessageLocally(Message, bIsJoinMessage);
}

void AMultiplayerGameState::ShowConnectionMessageLocally(const FString& Message, bool bIsJoinMessage) const
{
	const FColor MessageColor = bIsJoinMessage ? FColor::Green : FColor::Yellow;

	UE_LOG(LogTemp, Log, TEXT("%s"), *Message);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 8.f, MessageColor, Message);
	}
}
