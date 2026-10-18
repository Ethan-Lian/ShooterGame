#include "MultiplayerGameSession.h"

#include "Engine/GameInstance.h"
#include "MultiplayerSessionFlowSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogMultiplayerGameSession, Log, All);

FString AMultiplayerGameSession::ApproveLogin(const FString& Options)
{
	const FString Error = Super::ApproveLogin(Options);
	if (!Error.IsEmpty())
	{
		return Error;
	}
	// Returning to the standalone menu creates a local player while the old room remains closed.
	if (GetNetMode() == NM_Standalone)
	{
		return FString();
	}

	const UMultiplayerSessionFlowSubsystem* Flow = GetGameInstance()->GetSubsystem<UMultiplayerSessionFlowSubsystem>();
	check(Flow);
	if (!Flow->IsAcceptingNewPlayers())
	{
		UE_LOG(LogMultiplayerGameSession, Log, TEXT("Rejected a new login because the room is closed."));
		return TEXT("This room is closed to new players.");
	}
	return FString();
}
