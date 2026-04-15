#include "Commandlets/GP_ApplyFemaleAnimationSetupCommandlet.h"

#include "Utils/GP_AnimationSetupLibrary.h"

UGP_ApplyFemaleAnimationSetupCommandlet::UGP_ApplyFemaleAnimationSetupCommandlet()
{
	IsClient = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UGP_ApplyFemaleAnimationSetupCommandlet::Main(const FString& Params)
{
#if WITH_EDITOR
	if (UGP_AnimationSetupLibrary::CreateFemalePlayerAnimationSetup())
	{
		return 0;
	}

	return 1;
#else
	return 1;
#endif
}
