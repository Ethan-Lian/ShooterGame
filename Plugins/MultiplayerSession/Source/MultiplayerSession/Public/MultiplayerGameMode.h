#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MultiplayerGameMode.generated.h"

class AController;
class APlayerController;

UCLASS()
class MULTIPLAYERSESSION_API AMultiplayerGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	// Configures the replicated GameState used by this multiplayer GameMode. */
	AMultiplayerGameMode();

	// Logs when a player has joined and reports the current player count. */
	virtual void PostLogin(APlayerController* NewPlayer) override;

	// Logs when a player has left and reports the current player count. */
	virtual void Logout(AController* Exiting) override;

private:
	// Resolves the best display name for a controller before broadcasting the message. */
	static FString ResolvePlayerDisplayName(const AController* Controller);

	// Builds the final on-screen message for a player join event. */
	static FString BuildJoinMessage(const FString& PlayerName, int32 PlayerCount);

	// Builds the final on-screen message for a player leave event. */
	static FString BuildLeaveMessage(const FString& PlayerName, int32 PlayerCount);
};
