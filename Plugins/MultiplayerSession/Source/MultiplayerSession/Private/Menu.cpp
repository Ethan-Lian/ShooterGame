#include "Menu.h"
#include "MultiplayerSession/Public/MultiplayerSessionsSubsystem.h"
#include "Components/Button.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"

namespace
{
	FString GetCurrentSubsystemLabel(const UWorld* World)
	{
		if (const IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(World))
		{
			return OnlineSubsystem->GetSubsystemName().ToString();
		}

		return TEXT("None");
	}
}

void UMenu::MenuSetup(int32 NumberOfPublicConnections, FString TypeOfMatch, FString Path)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			10,
			FColor::Yellow,
			FString::Printf(TEXT("UMenu::MenuSetup called, OSS=%s"), *GetCurrentSubsystemLabel(GetWorld())));
	}
	
	NumPublicConnections = NumberOfPublicConnections;
	MatchType = TypeOfMatch;
	LobbyPath = FString::Printf(TEXT("%s?listen"), *Path);
	
	AddToViewport();
	SetVisibility(ESlateVisibility::Visible);
	SetIsFocusable(true);
	
	UWorld* World = GetWorld();
	if (World)
	{
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (PlayerController)
		{
			FInputModeUIOnly InputModeData;
			InputModeData.SetWidgetToFocus(TakeWidget());
			InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(true);
		}
	}

	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance)
	{
		//Subsystem created when GameInstance created
		MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
	}
	
	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->MultiplayerOnCreateSessionCompleteDelegate.AddDynamic(this,&ThisClass::UMenu::OnCreateSessionComplete);
		MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionCompleteDelegate.AddDynamic(this, &ThisClass::OnDestroySessionComplete);
		MultiplayerSessionsSubsystem->MultiplayerOnStartSessionCompleteDelegate.AddDynamic(this, &ThisClass::OnStartSessionComplete);
	}
}

void UMenu::MenuTearDown()
{
	RemoveFromParent();
	
	UWorld* World = GetWorld();
	if (World)
	{
		APlayerController* PlayerController  =World->GetFirstPlayerController();
		if (PlayerController)
		{
			FInputModeGameOnly InputModeData;
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(false);
		}
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

	if (JoinButton)
	{
		JoinButton->SetIsEnabled(false);
		JoinButton->SetVisibility(ESlateVisibility::Collapsed);
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
	
	// if (GEngine)
	// {
	// 	GEngine->AddOnScreenDebugMessage(
	// 		-1,
	// 		10,
	// 		FColor::Red,
	// 		FString::Printf(TEXT("UMenu::HostButtonClicked fired, OSS=%s"), *GetCurrentSubsystemLabel()));
	// }

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

	// if (GEngine)
	// {
	// 	GEngine->AddOnScreenDebugMessage(
	// 		-1,
	// 		10.f,
	// 		bWasSuccessful ? FColor::Green : FColor::Red,
	// 		FString::Printf(TEXT("OnCreateSessionComplete: %s, OSS=%s, LobbyPath=%s"),
	// 			bWasSuccessful ? TEXT("Success") : TEXT("Fail"),
	// 			*GetCurrentSubsystemLabel(),
	// 			*LobbyPath));
	// }
	if (bWasSuccessful)
	{
		if (MultiplayerSessionsSubsystem)
		{
			MultiplayerSessionsSubsystem->RequestShowLobbyInvitePanelAfterTravel();
		}
		
		UWorld* World = GetWorld();
		if (World)
		{
			World->ServerTravel(LobbyPath);
		}
	}
}

void UMenu::OnDestroySessionComplete(bool bWasSuccessful)
{
}

void UMenu::OnStartSessionComplete(bool bWasSuccessful)
{
}
