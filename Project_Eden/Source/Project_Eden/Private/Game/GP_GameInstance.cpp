#include "Game/GP_GameInstance.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

void UGP_GameInstance::Init()
{
	Super::Init();

	if (GEngine)
	{
		GEngine->OnNetworkFailure().AddUObject(this, &UGP_GameInstance::HandleNetworkFailure);
		GEngine->OnTravelFailure().AddUObject(this, &UGP_GameInstance::HandleTravelFailure);
	}
}

void UGP_GameInstance::Shutdown()
{
	if (GEngine)
	{
		GEngine->OnNetworkFailure().RemoveAll(this);
		GEngine->OnTravelFailure().RemoveAll(this);
	}

	Super::Shutdown();
}

bool UGP_GameInstance::IsAuthority(const UWorld* World) const
{
	if (!World)
	{
		return false;
	}
	const ENetMode NetMode = World->GetNetMode();
	return NetMode == NM_DedicatedServer || NetMode == NM_ListenServer;
}

void UGP_GameInstance::HandleNetworkFailure(UWorld* World, UNetDriver* /*NetDriver*/,
	ENetworkFailure::Type /*FailureType*/, const FString& ErrorString)
{
	// A client dropping off the host fires here on the host too; don't yank the
	// host to the menu — Logout already handles a departed client there.
	if (IsAuthority(World))
	{
		return;
	}

	LastConnectionError = ErrorString.IsEmpty()
		? TEXT("Connection lost.")
		: ErrorString;

	ReturnToMainMenu();
}

void UGP_GameInstance::HandleTravelFailure(UWorld* World, ETravelFailure::Type /*FailureType*/,
	const FString& ErrorString)
{
	if (IsAuthority(World))
	{
		return;
	}

	LastConnectionError = ErrorString.IsEmpty()
		? TEXT("Travel failed.")
		: ErrorString;

	ReturnToMainMenu();
}

void UGP_GameInstance::ReturnToMainMenu()
{
	// Absolute travel disconnects from any server and loads the menu locally,
	// which also tears down a listen server when the host leaves.
	UGameplayStatics::OpenLevel(this, FName(*MainMenuMapName), /*bAbsolute=*/true);
}
