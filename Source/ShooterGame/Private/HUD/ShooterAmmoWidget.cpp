#include "HUD/ShooterAmmoWidget.h"

#include "Character/PlayerCharacter.h"
#include "Components/Border.h"
#include "Components/ShooterCombatComponent.h"
#include "Components/ShooterInventoryComponent.h"
#include "Components/ShooterPawnExtensionComponent.h"
#include "Components/ShooterWeaponEquipmentComponent.h"
#include "Components/TextBlock.h"
#include "ShooterGame.h"

void UShooterAmmoWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (AmmoPanel == nullptr || AmmoCountText == nullptr || ReloadHintText == nullptr)
	{
		UE_LOG(LogShooterGame, Warning,
			TEXT("%s requires Blueprint widgets named AmmoPanel, AmmoCountText and ReloadHintText."), *GetName());
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	RefreshAmmoDisplay();
}

void UShooterAmmoWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RefreshAmmoDisplay();
}

void UShooterAmmoWidget::RefreshAmmoDisplay()
{
	if (AmmoPanel == nullptr || AmmoCountText == nullptr || ReloadHintText == nullptr)
	{
		return;
	}

	const APlayerCharacter* Pawn = GetOwningPlayerPawn<APlayerCharacter>();
	const UShooterPawnExtensionComponent* Extension = Pawn != nullptr ? Pawn->GetPawnExtensionComponent() : nullptr;
	const UShooterCombatComponent* Combat = Pawn != nullptr ? Pawn->GetCombatComponent() : nullptr;
	const UShooterWeaponEquipmentComponent* Equipment = Pawn != nullptr ? Pawn->GetWeaponEquipmentComponent() : nullptr;
	const FWeaponInventoryEntry* Entry = Equipment != nullptr ? Equipment->GetEquippedInventoryEntry() : nullptr;
	const bool bShowAmmo = Extension != nullptr && Extension->IsGameplayReady()
		&& Combat != nullptr && !Combat->IsWeaponInteractionBlocked()
		&& Entry != nullptr && Entry->IsValid();

	// Hide only the panel so polling continues while possession/replication catches up.
	AmmoPanel->SetVisibility(bShowAmmo ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	if (!bShowAmmo)
	{
		return;
	}

	const bool bReloading = Combat->IsReloading();
	const bool bEmpty = Entry->CurrentMagazineAmmo == 0;
	const FLinearColor AmmoColor = bEmpty ? FLinearColor(1.f, 0.3f, 0.25f) : FLinearColor::White;
	AmmoCountText->SetText(FText::Format(
		NSLOCTEXT("ShooterHUD", "InfiniteReserveAmmo", "{0} / \u221e"), FText::AsNumber(Entry->CurrentMagazineAmmo)));
	AmmoCountText->SetColorAndOpacity(FSlateColor(AmmoColor));

	ReloadHintText->SetVisibility(bReloading || bEmpty ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Hidden);
	ReloadHintText->SetText(bReloading
		? NSLOCTEXT("ShooterHUD", "Reloading", "RELOADING...")
		: NSLOCTEXT("ShooterHUD", "ReloadHint", "R  RELOAD"));
	ReloadHintText->SetColorAndOpacity(FSlateColor(bReloading ? FLinearColor(0.5f, 0.9f, 1.f) : AmmoColor));
}
