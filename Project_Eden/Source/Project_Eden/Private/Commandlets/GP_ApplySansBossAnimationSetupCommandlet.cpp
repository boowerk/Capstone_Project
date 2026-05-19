#include "Commandlets/GP_ApplySansBossAnimationSetupCommandlet.h"

#include "Utils/GP_AnimationSetupLibrary.h"

UGP_ApplySansBossAnimationSetupCommandlet::UGP_ApplySansBossAnimationSetupCommandlet()
{
	IsClient = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UGP_ApplySansBossAnimationSetupCommandlet::Main(const FString& Params)
{
#if WITH_EDITOR
	// Reuse the editor utility path so repeated commandlet runs keep all boss animation assets in sync.
	return UGP_AnimationSetupLibrary::CreateSansBossAnimationSetup() ? 0 : 1;
#else
	return 1;
#endif
}
