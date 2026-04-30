#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Menu.generated.h"

class UButton;
UCLASS()
class MULTIPLAYERSESSION_API UMenu : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable)
	void MenuSetup(int32 NumberOfPublicConnections = 4, FString TypeOfMatch = FString(TEXT("FreeForAll")), FString Path = FString(TEXT("/Game/ThirdPerson/Lobby")));
	
	virtual bool Initialize() override;
	
	//这个函数是?
	virtual void NativeDestruct() override;
private:
	
	// The Subsystem designed to handle all online session functionality.
	class UMultiplayerSessionsSubsystem* MultiplayerSessionsSubsystem;
	
	UPROPERTY(meta=(BindWidget))
	UButton* HostButton;
	
	UPROPERTY(meta=(BindWidgetOptional))
	UButton* JoinButton;
	
	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	int32 NumPublicConnections{4};

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	FString MatchType{TEXT("FreeForAll")};
	
	FString LobbyPath;
	
	UFUNCTION()
	void HostButtonClicked();

	void MenuTearDown();
	
	/*
	 * custom callbacks for the multiplayerSessionSubsystem's custom delegates
	 */
	UFUNCTION()
	void OnCreateSessionComplete(bool bWasSuccessful);

	UFUNCTION()
	void OnDestroySessionComplete(bool bWasSuccessful);

	UFUNCTION()
	void OnStartSessionComplete(bool bWasSuccessful);
};
