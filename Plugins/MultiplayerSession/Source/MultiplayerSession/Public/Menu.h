#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Menu.generated.h"

class UButton;
class UMultiplayerSessionUIManagerSubsystem;

UCLASS()
class MULTIPLAYERSESSION_API UMenu : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable)
	void MenuSetup(int32 NumberOfPublicConnections = 4, FString TypeOfMatch = FString(TEXT("FreeForAll")), FString Path = FString(TEXT("/Game/ShooterGameContent/Maps/Lobby")));
	
	virtual bool Initialize() override;
	virtual void NativeDestruct() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<class UMultiplayerSessionsSubsystem> MultiplayerSessionsSubsystem;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> HostButton;

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	int32 NumPublicConnections{4};

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	FString MatchType{TEXT("FreeForAll")};
	
	FString LobbyPath;

	bool bIsTearingDown = false;
	
	UFUNCTION()
	void HostButtonClicked();

	void MenuTearDown();
	UMultiplayerSessionUIManagerSubsystem* GetUIManagerSubsystem() const;

	UFUNCTION()
	void OnCreateSessionComplete(bool bWasSuccessful);
};
