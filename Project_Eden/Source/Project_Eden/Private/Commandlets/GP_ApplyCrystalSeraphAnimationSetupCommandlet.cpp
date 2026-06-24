#include "Commandlets/GP_ApplyCrystalSeraphAnimationSetupCommandlet.h"

#include "Utils/GP_AnimationSetupLibrary.h"

UGP_ApplyCrystalSeraphAnimationSetupCommandlet::UGP_ApplyCrystalSeraphAnimationSetupCommandlet()
{
	IsClient = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UGP_ApplyCrystalSeraphAnimationSetupCommandlet::Main(const FString& Params)
{
#if WITH_EDITOR
	// Keep the commandlet thin so the editor utility remains the single source of truth for generated assets.
	return UGP_AnimationSetupLibrary::CreateCrystalSeraphAnimationSetup() ? 0 : 1;
#else
	return 1;
#endif
}
