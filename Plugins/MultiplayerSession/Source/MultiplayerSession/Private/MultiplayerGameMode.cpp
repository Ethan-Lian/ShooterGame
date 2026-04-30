#include "MultiplayerGameMode.h"
#include "MultiplayerGameState.h"
#include "GameFramework/PlayerState.h"

AMultiplayerGameMode::AMultiplayerGameMode()
{
	// Keep the replicated notification path inside the plugin so every map using this
	// GameMode automatically gets a GameState that can multicast connection messages.
	GameStateClass = AMultiplayerGameState::StaticClass();
}

void AMultiplayerGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	const FString PlayerName = ResolvePlayerDisplayName(NewPlayer);
	const FString Message = BuildJoinMessage(PlayerName, GetNumPlayers());

	// GameMode only exists on the server, so it detects the event and asks GameState
	// to multicast the final message to every connected machine.
	if (AMultiplayerGameState* MultiplayerGameState = GetGameState<AMultiplayerGameState>())
	{
		MultiplayerGameState->MulticastBroadcastConnectionMessage(Message, true);
	}
}

void AMultiplayerGameMode::Logout(AController* Exiting)
{
	const FString PlayerName = ResolvePlayerDisplayName(Exiting);

	Super::Logout(Exiting);

	const FString Message = BuildLeaveMessage(PlayerName, GetNumPlayers());

	if (AMultiplayerGameState* MultiplayerGameState = GetGameState<AMultiplayerGameState>())
	{
		MultiplayerGameState->MulticastBroadcastConnectionMessage(Message, false);
	}
}

FString AMultiplayerGameMode::ResolvePlayerDisplayName(const AController* Controller)
{
	if (Controller == nullptr)
	{
		return TEXT("UnknownPlayer");
	}

	if (const APlayerState* PlayerState = Controller->GetPlayerState<APlayerState>())
	{
		const FString PlayerName = PlayerState->GetPlayerName();
		if (!PlayerName.IsEmpty())
		{
			return PlayerName;
		}
	}

	return Controller->GetName();
}

FString AMultiplayerGameMode::BuildJoinMessage(const FString& PlayerName, int32 PlayerCount)
{
	return FString::Printf(TEXT("用户 %s 加入游戏，当前人数: %d"), *PlayerName, PlayerCount);
}

FString AMultiplayerGameMode::BuildLeaveMessage(const FString& PlayerName, int32 PlayerCount)
{
	return FString::Printf(TEXT("用户 %s 离开游戏，当前人数: %d"), *PlayerName, PlayerCount);
}
