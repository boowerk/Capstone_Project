#include "Commandlets/GP_CreateLandscapeTestEnvironmentCommandlet.h"

#include "Utils/GP_LandscapeTestEnvironmentSetupLibrary.h"

UGP_CreateLandscapeTestEnvironmentCommandlet::UGP_CreateLandscapeTestEnvironmentCommandlet()
{
	IsClient = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UGP_CreateLandscapeTestEnvironmentCommandlet::Main(const FString& Params)
{
#if WITH_EDITOR
	// The shared setup library is also callable from editor utilities, keeping map generation rules in one place.
	return UGP_LandscapeTestEnvironmentSetupLibrary::CreateOrUpdateLandscapeTestEnvironment() ? 0 : 1;
#else
	return 1;
#endif
}
