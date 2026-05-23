#include "Animation/GP_BossAnimInstance.h"

void UGP_BossAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	// Boss animation setup currently uses the shared locomotion cache from UGP_CharacterAnimInstance.
}

void UGP_BossAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	// Keep this class as the boss-specific extension point for phase or pattern animation state.
}
