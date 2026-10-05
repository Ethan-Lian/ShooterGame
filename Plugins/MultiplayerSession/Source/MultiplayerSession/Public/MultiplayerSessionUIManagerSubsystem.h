#pragma once

#include "CoreMinimal.h"
#include "MultiplayerSessionFlowSubsystem.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MultiplayerSessionUIManagerSubsystem.generated.h"

class APlayerController;
class ULobbyInvitePanelWidget;
class UMenu;
class UMultiplayerSessionRootWidget;
class UMultiplayerInviteConfirmationWidget;
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
	void HandleFlowStateChanged(EMultiplayerSessionFlowState State, FText Error);

	void HandlePostLoadMap(UWorld* LoadedWorld);

	UFUNCTION()
	void HandleInviteConfirmationRequested(FString HostName);
	void ClearInviteConfirmation();
	void RestoreVisibleWidgetInputMode();

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

	UPROPERTY(Transient)
	TObjectPtr<UMultiplayerSessionFlowSubsystem> SessionFlow;

	FDelegateHandle PostLoadMapHandle;

	UPROPERTY(Transient)
	TObjectPtr<UMultiplayerSessionRootWidget> RootWidget;
	TWeakObjectPtr<UWorld> RootOwnerWorld;
	TWeakObjectPtr<APlayerController> RootOwnerController;

	UPROPERTY(Transient)
	TObjectPtr<ULobbyInvitePanelWidget> LobbyInvitePanelWidget;

	UPROPERTY(Transient)
	TObjectPtr<UMenu> MenuWidget;

	UPROPERTY(Transient)
	TObjectPtr<UMultiplayerInviteConfirmationWidget> InviteConfirmationWidget;
};
