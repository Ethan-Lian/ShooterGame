#include "LobbyFriendRowWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"

namespace
{
	constexpr float FriendRowHeight = 48.f;
	constexpr float InviteButtonWidth = 72.f;
	constexpr float InviteButtonHeight = 32.f;

	FSlateChildSize RowAutoSize()
	{
		return FSlateChildSize(ESlateSizeRule::Automatic);
	}

	FSlateChildSize RowFillSize(float Value = 1.f)
	{
		FSlateChildSize Size(ESlateSizeRule::Fill);
		Size.Value = Value;
		return Size;
	}

	UTextBlock* BuildRowTextBlock(UWidgetTree* WidgetTree, const FName Name, const FText& Text, const int32 FontSize, const FLinearColor Color)
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
}

TSharedRef<SWidget> ULobbyFriendRowWidget::RebuildWidget()
{
	BuildNativeWidgetTree();
	return Super::RebuildWidget();
}

void ULobbyFriendRowWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (NativeInviteButton)
	{
		NativeInviteButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleInviteClicked);
	}

	RefreshDisplayedFriend();
}

void ULobbyFriendRowWidget::NativeDestruct()
{
	if (NativeInviteButton)
	{
		NativeInviteButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleInviteClicked);
	}

	Super::NativeDestruct();
}

void ULobbyFriendRowWidget::SetupFriendRow(const FSteamFriendInviteEntry& InFriendEntry)
{
	FriendEntry = InFriendEntry;
	RefreshDisplayedFriend();
}

void ULobbyFriendRowWidget::HandleInviteClicked()
{
	ULobbyInviteSubsystem* InviteSubsystem = GetLobbyInviteSubsystem();
	if (!InviteSubsystem || FriendEntry.FriendIdString.IsEmpty())
	{
		return;
	}

	InviteSubsystem->SendSteamInviteToFriendByIdString(FriendEntry.FriendIdString);
}

ULobbyInviteSubsystem* ULobbyFriendRowWidget::GetLobbyInviteSubsystem() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<ULobbyInviteSubsystem>() : nullptr;
}

void ULobbyFriendRowWidget::BuildNativeWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	USizeBox* RowSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("LobbyFriendRowSizeBox"));
	RowSizeBox->SetHeightOverride(FriendRowHeight);
	WidgetTree->RootWidget = RowSizeBox;

	UBorder* RowBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LobbyFriendRowBorder"));
	RowBorder->SetPadding(FMargin(12.f, 6.f));
	RowBorder->SetBrushColor(FLinearColor(0.08f, 0.10f, 0.13f, 0.78f));
	RowSizeBox->SetContent(RowBorder);

	UHorizontalBox* RowContent = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("LobbyFriendRowContent"));
	RowBorder->SetContent(RowContent);

	NativeNameText = BuildRowTextBlock(
		WidgetTree,
		TEXT("NativeNameText"),
		FText::FromString(TEXT("Friend")),
		15,
		FLinearColor(0.92f, 0.95f, 1.f, 1.f));
	UHorizontalBoxSlot* NameSlot = RowContent->AddChildToHorizontalBox(NativeNameText);
	NameSlot->SetSize(RowFillSize());
	NameSlot->SetVerticalAlignment(VAlign_Center);

	NativeStatusText = BuildRowTextBlock(
		WidgetTree,
		TEXT("NativeStatusText"),
		FText::FromString(TEXT("Offline")),
		14,
		FLinearColor(0.55f, 0.55f, 0.55f, 1.f));
	UHorizontalBoxSlot* StatusSlot = RowContent->AddChildToHorizontalBox(NativeStatusText);
	StatusSlot->SetSize(RowAutoSize());
	StatusSlot->SetPadding(FMargin(12.f, 0.f));
	StatusSlot->SetVerticalAlignment(VAlign_Center);

	USizeBox* InviteButtonSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("InviteButtonSizeBox"));
	InviteButtonSizeBox->SetWidthOverride(InviteButtonWidth);
	InviteButtonSizeBox->SetHeightOverride(InviteButtonHeight);

	NativeInviteButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("NativeInviteButton"));
	NativeInviteButton->SetBackgroundColor(FLinearColor(0.13f, 0.22f, 0.30f, 1.f));

	UTextBlock* InviteText = BuildRowTextBlock(
		WidgetTree,
		TEXT("InviteButtonText"),
		FText::FromString(TEXT("Invite")),
		14,
		FLinearColor(0.92f, 0.96f, 1.f, 1.f));
	NativeInviteButton->SetContent(InviteText);
	InviteButtonSizeBox->SetContent(NativeInviteButton);

	UHorizontalBoxSlot* InviteSlot = RowContent->AddChildToHorizontalBox(InviteButtonSizeBox);
	InviteSlot->SetSize(RowAutoSize());
	InviteSlot->SetVerticalAlignment(VAlign_Center);
}

void ULobbyFriendRowWidget::RefreshDisplayedFriend()
{
	if (NativeNameText)
	{
		NativeNameText->SetText(FText::FromString(FriendEntry.DisplayName));
	}

	if (NativeStatusText)
	{
		const FText StatusLabel = FriendEntry.bIsOnline
			? FText::FromString(TEXT("Online"))
			: FText::FromString(TEXT("Offline"));
		const FLinearColor StatusColor = FriendEntry.bIsOnline
			? FLinearColor(0.25f, 0.9f, 0.45f, 1.0f)
			: FLinearColor(0.55f, 0.55f, 0.55f, 1.0f);

		NativeStatusText->SetText(StatusLabel);
		NativeStatusText->SetColorAndOpacity(FSlateColor(StatusColor));
	}
}
