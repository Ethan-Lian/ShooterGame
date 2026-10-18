#pragma once

#include "GameFramework/GameSession.h"
#include "MultiplayerGameSession.generated.h"

// Applies the persistent room admission policy at both PreLogin and final Login.
UCLASS()
class MULTIPLAYERSESSION_API AMultiplayerGameSession : public AGameSession
{
	GENERATED_BODY()

public:
	virtual FString ApproveLogin(const FString& Options) override;
};
