#include "LobbyInvitePanelWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "LobbyFriendRowWidget.h"

namespace
{
	constexpr float LobbyInvitePanelWidth = 520.f;
	constexpr float LobbyInvitePanelHeight = 620.f;
	constexpr float LobbyInvitePanelRightOffset = 40.f;
	constexpr float LobbyInviteButtonHeight = 44.f;
	constexpr float LobbyInviteCloseButtonWidth = 40.f;
	constexpr float LobbyInviteCloseButtonHeight = 32.f;

	FSlateChildSize AutoSize()
	{
		return FSlateChildSize(ESlateSizeRule::Automatic);
	}

	FSlateChildSize FillSize(float Value = 1.f)
	{
		FSlateChildSize Size(ESlateSizeRule::Fill);
		Size.Value = Value;
		return Size;
	}

	UTextBlock* BuildTextBlock(UWidgetTree* WidgetTree, const FName Name, const FText& Text, const int32 FontSize, const FLinearColor Color)
	{
		UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		TextBlock->SetText(Text);
		TextBlock->SetColorAndOpacity(FSlateColor(Color));

		FSlateFontInfo FontInfo = TextBlock->GetFont();
		FontInfo.Size = FontSize;
		TextBlock->SetFont(FontInfo);
		TextBlock->SetAutoWrapText(false);
		TextBlock->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
		return TextBlock;
	}

	UButton* BuildTextButton(UWidgetTree* WidgetTree, const FName Name, const FText& Text)
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		Button->SetBackgroundColor(FLinearColor(0.12f, 0.18f, 0.24f, 1.f));

		UTextBlock* ButtonText = BuildTextBlock(
			WidgetTree,
			NAME_None,
			Text,
			16,
			FLinearColor(0.92f, 0.96f, 1.f, 1.f));
		Button->SetContent(ButtonText);
		return Button;
	}
}

void ULobbyInvitePanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BindButtonDelegates();

	if (!ResolveLobbyInviteSubsystem())
	{
		SetStatusMessage(FText::FromString(TEXT("Lobby invite subsystem is unavailable.")));
		return;
	}

	BindSubsystemDelegates();
	SetStatusMessage(LobbyInviteSubsystem->GetLobbyInviteStatus());
	RebuildFriendRowsFromList(LobbyInviteSubsystem->GetCachedSteamFriends());
	LobbyInviteSubsystem->RefreshSteamFriendsList();
}

TSharedRef<SWidget> ULobbyInvitePanelWidget::RebuildWidget()
{
	BuildNativeWidgetTree();
	return Super::RebuildWidget();
}

void ULobbyInvitePanelWidget::NativeDestruct()
{
	UnbindSubsystemDelegates();
	UnbindButtonDelegates();

	Super::NativeDestruct();
}

void ULobbyInvitePanelWidget::RefreshFriends()
{
	if (!ResolveLobbyInviteSubsystem())
	{
		SetStatusMessage(FText::FromString(TEXT("Lobby invite subsystem is unavailable.")));
		return;
	}

	LobbyInviteSubsystem->RefreshSteamFriendsList();
}

void ULobbyInvitePanelWidget::RebuildFriendRows()
{
	if (!ResolveLobbyInviteSubsystem())
	{
		RebuildFriendRowsFromList(TArray<FSteamFriendInviteEntry>());
		return;
	}

	RebuildFriendRowsFromList(LobbyInviteSubsystem->GetCachedSteamFriends());
}

void ULobbyInvitePanelWidget::HandleRefreshClicked()
{
	RefreshFriends();
}

void ULobbyInvitePanelWidget::HandleStartClicked()
{
	OnStartRequested.Broadcast();
}

void ULobbyInvitePanelWidget::HandleCloseClicked()
{
	OnCloseRequested.Broadcast();
}

void ULobbyInvitePanelWidget::HandleFriendsListUpdated(const TArray<FSteamFriendInviteEntry>& Friends)
{
	RebuildFriendRowsFromList(Friends);
}

void ULobbyInvitePanelWidget::HandleInviteStatusChanged(FText StatusMessage)
{
	SetStatusMessage(StatusMessage);
}

bool ULobbyInvitePanelWidget::ResolveLobbyInviteSubsystem()
{
	if (LobbyInviteSubsystem)
	{
		return true;
	}

	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return false;
	}

	LobbyInviteSubsystem = GameInstance->GetSubsystem<ULobbyInviteSubsystem>();
	return LobbyInviteSubsystem != nullptr;
}

void ULobbyInvitePanelWidget::BuildNativeWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("LobbyInviteRootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	USizeBox* PanelSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("LobbyInvitePanelSizeBox"));
	PanelSizeBox->SetWidthOverride(LobbyInvitePanelWidth);
	PanelSizeBox->SetHeightOverride(LobbyInvitePanelHeight);

	UCanvasPanelSlot* PanelCanvasSlot = RootCanvas->AddChildToCanvas(PanelSizeBox);
	PanelCanvasSlot->SetAnchors(FAnchors(1.f, 0.5f, 1.f, 0.5f));
	PanelCanvasSlot->SetAlignment(FVector2D(1.f, 0.5f));
	PanelCanvasSlot->SetPosition(FVector2D(-LobbyInvitePanelRightOffset, 0.f));
	PanelCanvasSlot->SetSize(FVector2D(LobbyInvitePanelWidth, LobbyInvitePanelHeight));

	UBorder* PanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LobbyInvitePanelBorder"));
	PanelBorder->SetPadding(FMargin(24.f));
	PanelBorder->SetBrushColor(FLinearColor(0.025f, 0.033f, 0.045f, 0.92f));
	PanelSizeBox->SetContent(PanelBorder);

	UVerticalBox* PanelStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LobbyInvitePanelStack"));
	PanelBorder->SetContent(PanelStack);

	UHorizontalBox* HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("LobbyInviteHeaderRow"));
	UVerticalBoxSlot* HeaderSlot = PanelStack->AddChildToVerticalBox(HeaderRow);
	HeaderSlot->SetSize(AutoSize());
	HeaderSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 16.f));
	HeaderSlot->SetHorizontalAlignment(HAlign_Fill);

	UTextBlock* TitleText = BuildTextBlock(
		WidgetTree,
		TEXT("LobbyInviteTitleText"),
		FText::FromString(TEXT("Invite Friends")),
		24,
		FLinearColor(0.95f, 0.97f, 1.f, 1.f));
	UHorizontalBoxSlot* TitleSlot = HeaderRow->AddChildToHorizontalBox(TitleText);
	TitleSlot->SetSize(FillSize());
	TitleSlot->SetVerticalAlignment(VAlign_Center);

	USizeBox* CloseButtonSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("LobbyInviteCloseButtonSizeBox"));
	CloseButtonSizeBox->SetWidthOverride(LobbyInviteCloseButtonWidth);
	CloseButtonSizeBox->SetHeightOverride(LobbyInviteCloseButtonHeight);
	NativeCloseButton = BuildTextButton(WidgetTree, TEXT("NativeCloseButton"), FText::FromString(TEXT("X")));
	CloseButtonSizeBox->SetContent(NativeCloseButton);
	UHorizontalBoxSlot* CloseSlot = HeaderRow->AddChildToHorizontalBox(CloseButtonSizeBox);
	CloseSlot->SetSize(AutoSize());
	CloseSlot->SetVerticalAlignment(VAlign_Center);

	NativeStatusText = BuildTextBlock(
		WidgetTree,
		TEXT("NativeStatusText"),
		FText::FromString(TEXT("Steam friends not loaded.")),
		15,
		FLinearColor(0.72f, 0.78f, 0.86f, 1.f));
	UVerticalBoxSlot* StatusSlot = PanelStack->AddChildToVerticalBox(NativeStatusText);
	StatusSlot->SetSize(AutoSize());
	StatusSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
	StatusSlot->SetHorizontalAlignment(HAlign_Fill);

	UScrollBox* FriendScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("NativeFriendListBox"));
	FriendScrollBox->SetOrientation(Orient_Vertical);
	FriendScrollBox->SetScrollBarVisibility(ESlateVisibility::Visible);
	FriendScrollBox->SetConsumeMouseWheel(EConsumeMouseWheel::Always);
	NativeFriendListBox = FriendScrollBox;
	UVerticalBoxSlot* FriendsSlot = PanelStack->AddChildToVerticalBox(FriendScrollBox);
	FriendsSlot->SetSize(FillSize());
	FriendsSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 16.f));
	FriendsSlot->SetHorizontalAlignment(HAlign_Fill);

	UHorizontalBox* BottomRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("LobbyInviteBottomRow"));
	UVerticalBoxSlot* BottomSlot = PanelStack->AddChildToVerticalBox(BottomRow);
	BottomSlot->SetSize(AutoSize());
	BottomSlot->SetHorizontalAlignment(HAlign_Fill);

	USizeBox* RefreshButtonSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RefreshButtonSizeBox"));
	RefreshButtonSizeBox->SetHeightOverride(LobbyInviteButtonHeight);
	NativeRefreshButton = BuildTextButton(WidgetTree, TEXT("NativeRefreshButton"), FText::FromString(TEXT("Refresh Friends")));
	RefreshButtonSizeBox->SetContent(NativeRefreshButton);
	UHorizontalBoxSlot* RefreshSlot = BottomRow->AddChildToHorizontalBox(RefreshButtonSizeBox);
	RefreshSlot->SetSize(FillSize());
	RefreshSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));

	USizeBox* StartButtonSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("StartButtonSizeBox"));
	StartButtonSizeBox->SetHeightOverride(LobbyInviteButtonHeight);
	NativeStartButton = BuildTextButton(WidgetTree, TEXT("NativeStartButton"), FText::FromString(TEXT("Start Game")));
	StartButtonSizeBox->SetContent(NativeStartButton);
	UHorizontalBoxSlot* StartSlot = BottomRow->AddChildToHorizontalBox(StartButtonSizeBox);
	StartSlot->SetSize(FillSize());
	StartSlot->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
}

void ULobbyInvitePanelWidget::BindButtonDelegates()
{
	if (NativeRefreshButton)
	{
		NativeRefreshButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleRefreshClicked);
	}

	if (NativeStartButton)
	{
		NativeStartButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleStartClicked);
	}

	if (NativeCloseButton)
	{
		NativeCloseButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleCloseClicked);
	}
}

void ULobbyInvitePanelWidget::UnbindButtonDelegates()
{
	if (NativeRefreshButton)
	{
		NativeRefreshButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleRefreshClicked);
	}

	if (NativeStartButton)
	{
		NativeStartButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleStartClicked);
	}

	if (NativeCloseButton)
	{
		NativeCloseButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleCloseClicked);
	}
}

void ULobbyInvitePanelWidget::BindSubsystemDelegates()
{
	if (!LobbyInviteSubsystem)
	{
		return;
	}

	LobbyInviteSubsystem->OnSteamFriendsListUpdated.AddUniqueDynamic(
		this,
		&ThisClass::HandleFriendsListUpdated);
	LobbyInviteSubsystem->OnLobbyInviteStatusChanged.AddUniqueDynamic(
		this,
		&ThisClass::HandleInviteStatusChanged);
}

void ULobbyInvitePanelWidget::UnbindSubsystemDelegates()
{
	if (!LobbyInviteSubsystem)
	{
		return;
	}

	LobbyInviteSubsystem->OnSteamFriendsListUpdated.RemoveDynamic(
		this,
		&ThisClass::HandleFriendsListUpdated);
	LobbyInviteSubsystem->OnLobbyInviteStatusChanged.RemoveDynamic(
		this,
		&ThisClass::HandleInviteStatusChanged);
}

void ULobbyInvitePanelWidget::RebuildFriendRowsFromList(const TArray<FSteamFriendInviteEntry>& Friends)
{
	if (!NativeFriendListBox)
	{
		return;
	}

	NativeFriendListBox->ClearChildren();

	if (Friends.Num() == 0)
	{
		return;
	}

	for (const FSteamFriendInviteEntry& FriendEntry : Friends)
	{
		TSubclassOf<ULobbyFriendRowWidget> RowClass = FriendRowWidgetClass;
		if (!RowClass)
		{
			RowClass = ULobbyFriendRowWidget::StaticClass();
		}
		ULobbyFriendRowWidget* FriendRowWidget = CreateWidget<ULobbyFriendRowWidget>(this, RowClass);
		if (!FriendRowWidget)
		{
			continue;
		}

		FriendRowWidget->SetupFriendRow(FriendEntry);
		NativeFriendListBox->AddChild(FriendRowWidget);
	}
}

void ULobbyInvitePanelWidget::SetStatusMessage(const FText& StatusMessage)
{
	if (NativeStatusText)
	{
		NativeStatusText->SetText(StatusMessage);
	}
}
