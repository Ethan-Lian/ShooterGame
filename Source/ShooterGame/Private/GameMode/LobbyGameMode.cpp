#include "GameMode/LobbyGameMode.h"

#include "MultiplayerGameSession.h"

TSubclassOf<AGameSession> ALobbyGameMode::GetGameSessionClass() const
{
	return AMultiplayerGameSession::StaticClass();
}
