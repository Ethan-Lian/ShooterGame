#pragma once

#include "Blueprint/UserWidget.h"
#include "ShooterCrosshairWidget.generated.h"

UCLASS()
class SHOOTERGAME_API UShooterCrosshairWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Initializes Blueprint-authored crosshair widgets without generating fallback UI.
	virtual bool Initialize() override;
};
