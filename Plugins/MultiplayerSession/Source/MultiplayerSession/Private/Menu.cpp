#include "Menu.h"
#include "MultiplayerSessionUIManagerSubsystem.h"
#include "MultiplayerSession/Public/MultiplayerSessionsSubsystem.h"
#include "Components/Button.h"

void UMenu::MenuSetup(int32 NumberOfPublicConnections, FString TypeOfMatch, FString Path)
{
	bIsTearingDown = false;
	NumPublicConnections = NumberOfPublicConnections;
	MatchType = TypeOfMatch;
	LobbyPath = FString::Printf(TEXT("%s?listen"), *Path);
	
	if (UMultiplayerSessionUIManagerSubsystem* UIManagerSubsystem = GetUIManagerSubsystem())
	{
		UIManagerSubsystem->ShowMenu(this);
	}

	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance)
	{
		//Subsystem created when GameInstance created
		MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
	}
	
	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->MultiplayerOnCreateSessionCompleteDelegate.AddUniqueDynamic(this, &ThisClass::OnCreateSessionComplete);
	}
}

void UMenu::MenuTearDown()
{
	if (bIsTearingDown)
	{
		return;
	}

	bIsTearingDown = true;

	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->MultiplayerOnCreateSessionCompleteDelegate.RemoveDynamic(this, &ThisClass::OnCreateSessionComplete);
	}

	if (UMultiplayerSessionUIManagerSubsystem* UIManagerSubsystem = GetUIManagerSubsystem())
	{
		UIManagerSubsystem->HideMenu(this);
	}
}

bool UMenu::Initialize()
{
	const bool bSuccess = Super::Initialize();
	if (!bSuccess) return false;

	if (HostButton)
	{
		HostButton->OnClicked.AddDynamic(this, &ThisClass::HostButtonClicked);
	}

	return true;
}

void UMenu::NativeDestruct()
{
	Super::NativeDestruct();
	
	MenuTearDown();
}

void UMenu::HostButtonClicked()
{
	HostButton->SetIsEnabled(false);

	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->CreateSession(NumPublicConnections, MatchType);
	}
	else
	{
		HostButton->SetIsEnabled(true);
	}
}

void UMenu::OnCreateSessionComplete(bool bWasSuccessful)
{
	if (!bWasSuccessful && HostButton)
	{
		HostButton->SetIsEnabled(true);
	}

	if (bWasSuccessful)
	{
		UWorld* World = GetWorld();

		if (MultiplayerSessionsSubsystem)
		{
			MultiplayerSessionsSubsystem->RequestShowLobbyInvitePanelAfterTravel();
		}

		MenuTearDown();
		
		if (World)
		{
			World->ServerTravel(LobbyPath);
		}
	}
}

UMultiplayerSessionUIManagerSubsystem* UMenu::GetUIManagerSubsystem() const
{
	UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UMultiplayerSessionUIManagerSubsystem>() : nullptr;
}
