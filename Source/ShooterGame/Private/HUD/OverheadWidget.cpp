#include "HUD/OverheadWidget.h"
#include "Components/TextBlock.h"

void UOverheadWidget::SetDisplayText(const FString& TextToDisplay) 
{
	if (DisplayText)
	{
		DisplayText->SetText(FText::FromString(TextToDisplay));
	}
}

void UOverheadWidget::ShowPlayerNetRole(APawn* InPawn)
{
	if (!InPawn) return;
	
	ENetRole LocalRole = InPawn->GetLocalRole();
	FString Role;
	
	switch (LocalRole)
	{
	case ROLE_Authority:
		Role = FString("Authority");
		break;
	case ROLE_AutonomousProxy:
		Role = FString("Autonomous Proxy");
		break;
	case ROLE_SimulatedProxy:
		Role = FString("Simulated Proxy");
		break;
	case ROLE_None:
		Role = FString("None");
		break;
	case ROLE_MAX:
		Role = FString("Max");
		break;
	}
	
	FString PlayerRole = FString::Printf(TEXT("Player Role is %s"),*Role);
	
	SetDisplayText(PlayerRole);
}

void UOverheadWidget::NativeDestruct()
{
	
	RemoveFromParent();
	Super::NativeDestruct();
}
