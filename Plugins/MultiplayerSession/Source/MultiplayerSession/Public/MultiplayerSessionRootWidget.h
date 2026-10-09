#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MultiplayerSessionRootWidget.generated.h"

class UCanvasPanel;
class UWidget;

UCLASS()
class MULTIPLAYERSESSION_API UMultiplayerSessionRootWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void AddWidgetToLayer(UWidget* Widget, int32 ZOrder);
	void RemoveWidgetFromLayer(UWidget* Widget);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void BuildRootWidgetTree();

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> RootCanvas;
};
