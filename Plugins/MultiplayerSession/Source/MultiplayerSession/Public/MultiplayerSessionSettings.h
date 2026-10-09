#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MultiplayerSessionSettings.generated.h"

class UWorld;
class ULobbyInvitePanelWidget;
class UMultiplayerInviteConfirmationWidget;

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Multiplayer Session"))
class MULTIPLAYERSESSION_API UMultiplayerSessionSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return FName(TEXT("Plugins")); }

	UPROPERTY(Config, EditAnywhere, Category = "Travel")
	TSoftObjectPtr<UWorld> MenuMap;

	UPROPERTY(Config, EditAnywhere, Category = "Travel")
	TSoftObjectPtr<UWorld> LobbyMap;

	UPROPERTY(Config, EditAnywhere, Category = "Travel")
	TSoftObjectPtr<UWorld> GameplayMap;

	UPROPERTY(Config, EditAnywhere, Category = "UI")
	TSoftClassPtr<ULobbyInvitePanelWidget> LobbyPanelClass;

	UPROPERTY(Config, EditAnywhere, Category = "UI")
	TSoftClassPtr<UMultiplayerInviteConfirmationWidget> InviteConfirmationClass;
};
