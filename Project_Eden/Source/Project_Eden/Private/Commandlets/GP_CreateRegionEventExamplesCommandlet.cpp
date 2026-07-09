#include "Commandlets/GP_CreateRegionEventExamplesCommandlet.h"

#include "Utils/GP_RegionEventExampleSetupLibrary.h"

UGP_CreateRegionEventExamplesCommandlet::UGP_CreateRegionEventExamplesCommandlet()
{
	IsClient = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UGP_CreateRegionEventExamplesCommandlet::Main(const FString& Params)
{
#if WITH_EDITOR
	// Keep commandlet logic tiny so the editor callable utility remains the single source of generated asset rules.
	return UGP_RegionEventExampleSetupLibrary::CreateOrUpdateRegionEventExampleAssets() ? 0 : 1;
#else
	return 1;
#endif
}
