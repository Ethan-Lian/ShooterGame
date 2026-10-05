#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MultiplayerInviteConfirmationWidget.generated.h"

class UButton;
class UTextBlock;

UCLASS(Abstract)
class MULTIPLAYERSESSION_API UMultiplayerInviteConfirmationWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetInvitingHost(const FString& HostName);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> MessageText;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ConfirmButton;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CancelButton;

private:
	UFUNCTION()
	void HandleConfirm();
	UFUNCTION()
	void HandleCancel();
};
