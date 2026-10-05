#include "MultiplayerSessionRootWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/PanelWidget.h"
#include "Components/Widget.h"

TSharedRef<SWidget> UMultiplayerSessionRootWidget::RebuildWidget()
{
	BuildRootWidgetTree();
	return Super::RebuildWidget();
}

void UMultiplayerSessionRootWidget::AddWidgetToLayer(UWidget* Widget, int32 ZOrder)
{
	if (!Widget)
	{
		return;
	}

	BuildRootWidgetTree();
	if (!RootCanvas || Widget->GetParent() == RootCanvas)
	{
		return;
	}

	if (UPanelWidget* ExistingParent = Widget->GetParent())
	{
		ExistingParent->RemoveChild(Widget);
	}

	UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(Widget);
	CanvasSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	CanvasSlot->SetOffsets(FMargin(0.f));
	CanvasSlot->SetZOrder(ZOrder);
}

void UMultiplayerSessionRootWidget::RemoveWidgetFromLayer(UWidget* Widget)
{
	if (RootCanvas && Widget && Widget->GetParent() == RootCanvas)
	{
		RootCanvas->RemoveChild(Widget);
	}
}

void UMultiplayerSessionRootWidget::BuildRootWidgetTree()
{
	if (!WidgetTree || RootCanvas)
	{
		return;
	}

	RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("MultiplayerSessionRootCanvas"));
	WidgetTree->RootWidget = RootCanvas;
}
