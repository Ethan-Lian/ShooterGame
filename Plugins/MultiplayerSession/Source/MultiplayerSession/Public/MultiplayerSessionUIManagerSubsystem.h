#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MultiplayerSessionUIManagerSubsystem.generated.h"

class APlayerController;
class ULobbyInvitePanelWidget;
class UMenu;
class UMultiplayerSessionRootWidget;
class UMultiplayerSessionsSubsystem;
class UUserWidget;

UCLASS()
class MULTIPLAYERSESSION_API UMultiplayerSessionUIManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Multiplayer Sessions|UI")
	void ShowLobbyInvitePanel();

	UFUNCTION(BlueprintCallable, Category = "Multiplayer Sessions|UI")
	void HideLobbyInvitePanel();

	UFUNCTION(BlueprintCallable, Category = "Multiplayer Sessions|UI")
	void ShowMenu(UMenu* InMenuWidget);

	UFUNCTION(BlueprintCallable, Category = "Multiplayer Sessions|UI")
	void HideMenu(UMenu* InMenuWidget);

	UFUNCTION(BlueprintCallable, Category = "Multiplayer Sessions|Travel")
	bool StartHostedGame();

private:
	UFUNCTION()
	void HandleLobbyInvitePanelRequested();

	UFUNCTION()
	void HandleLobbyInvitePanelCloseRequested();

	UFUNCTION()
	void HandleLobbyInvitePanelStartRequested();

	bool ResolveMultiplayerSessionsSubsystem();
	bool IsUsableLocalPlayerController(const APlayerController* PlayerController) const;
	bool DoesWidgetBelongToPlayer(const UUserWidget* Widget, const APlayerController* PlayerController) const;
	APlayerController* GetLocalPlayerController() const;
	UMultiplayerSessionRootWidget* GetOrCreateRootWidget(APlayerController* PlayerController);
	void ClearLobbyInvitePanelWidget();
	void ClearMenuWidget();
	void ClearRootWidget();
	void ApplyMenuInputMode(UMenu* InMenuWidget) const;
	void ApplyLobbyInviteInputMode();
	void RestoreGameInputMode() const;

	UPROPERTY(EditDefaultsOnly, Category = "Multiplayer Sessions|Travel")
	FString GameplayMapPath = TEXT("/Game/ShooterGameContent/Maps/GameLevel?listen");

	UPROPERTY(Transient)
	TObjectPtr<UMultiplayerSessionsSubsystem> MultiplayerSessionsSubsystem;

	UPROPERTY(Transient)
	TObjectPtr<UMultiplayerSessionRootWidget> RootWidget;

	UPROPERTY(Transient)
	TObjectPtr<ULobbyInvitePanelWidget> LobbyInvitePanelWidget;

	UPROPERTY(Transient)
	TObjectPtr<UMenu> MenuWidget;
};
