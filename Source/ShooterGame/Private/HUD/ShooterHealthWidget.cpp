#include "HUD/ShooterHealthWidget.h"

#include "Components/ProgressBar.h"
#include "Components/ShooterHealthComponent.h"
#include "Components/TextBlock.h"
#include "ShooterGame.h"

void UShooterHealthWidget::InitializeFromHealthComponent(UShooterHealthComponent* InHealthComponent)
{
	if (ObservedHealthComponent.Get() == InHealthComponent)
	{
		RefreshHealthDisplay();
		return;
	}

	ClearHealthBinding();
	ObservedHealthComponent = InHealthComponent;

	if (InHealthComponent != nullptr)
	{
		InHealthComponent->OnHealthChanged.AddUniqueDynamic(this, &UShooterHealthWidget::HandleHealthChanged);
	}

	RefreshHealthDisplay();
}

void UShooterHealthWidget::ClearHealthBinding()
{
	if (UShooterHealthComponent* HealthComponent = ObservedHealthComponent.Get())
	{
		HealthComponent->OnHealthChanged.RemoveDynamic(this, &UShooterHealthWidget::HandleHealthChanged);
	}

	ObservedHealthComponent.Reset();
}

void UShooterHealthWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!HasRequiredBindings())
	{
		UE_LOG(LogShooterGame, Warning,
			TEXT("%s requires Blueprint widgets named HealthProgressBar and HealthPercentText."), *GetName());
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	RefreshHealthDisplay();
}

void UShooterHealthWidget::NativeDestruct()
{
	ClearHealthBinding();
	Super::NativeDestruct();
}

void UShooterHealthWidget::RefreshHealthDisplay()
{
	if (!HasRequiredBindings())
	{
		return;
	}

	const UShooterHealthComponent* HealthComponent = ObservedHealthComponent.Get();
	const float NormalizedHealth = HealthComponent != nullptr
		? FMath::Clamp(HealthComponent->GetHealthNormalized(), 0.f, 1.f)
		: 0.f;
	const int32 HealthPercent = FMath::RoundToInt(NormalizedHealth * 100.f);

	HealthProgressBar->SetPercent(NormalizedHealth);
	HealthPercentText->SetText(FText::Format(NSLOCTEXT("ShooterHUD", "HealthPercentFormat", "{0}%"), HealthPercent));
}

void UShooterHealthWidget::HandleHealthChanged(float OldValue, float NewValue)
{
	RefreshHealthDisplay();
}

bool UShooterHealthWidget::HasRequiredBindings() const
{
	return HealthProgressBar != nullptr && HealthPercentText != nullptr;
}
