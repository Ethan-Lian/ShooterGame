#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "MultiplayerGameState.generated.h"

// Replicated match state that can broadcast connection messages to every client.
UCLASS()
class MULTIPLAYERSESSION_API AMultiplayerGameState : public AGameStateBase
{
	GENERATED_BODY()
	
public:
	// Broadcasts a connection message to every machine in the session.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastBroadcastConnectionMessage(const FString& Message, bool bIsJoinMessage);

private:
	// Shows the connection message locally on this machine.
	void ShowConnectionMessageLocally(const FString& Message, bool bIsJoinMessage) const;
};
