#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MultiplayerSessionFlowSubsystem.h"
#include "Menu.generated.h"

class UButton;
class UTextBlock;
class UMultiplayerSessionUIManagerSubsystem;

UCLASS()
class MULTIPLAYERSESSION_API UMenu : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable)
	void MenuSetup(int32 NumberOfPublicConnections = 4, FString TypeOfMatch = FString(TEXT("FreeForAll")), FString Path = FString());
	
	virtual bool Initialize() override;
	virtual void NativeDestruct() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UMultiplayerSessionFlowSubsystem> SessionFlow;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> HostButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> RetryButton;

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	int32 NumPublicConnections{4};

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	FString MatchType{TEXT("FreeForAll")};
	
	FString LobbyPath;

	bool bIsTearingDown = false;
	
	UFUNCTION()
	void HostButtonClicked();
	UFUNCTION()
	void RetryButtonClicked();

	void MenuTearDown();
	UMultiplayerSessionUIManagerSubsystem* GetUIManagerSubsystem() const;

	UFUNCTION()
	void HandleFlowStateChanged(EMultiplayerSessionFlowState State, FText Error);
};
