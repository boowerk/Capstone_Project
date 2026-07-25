#if WITH_DEV_AUTOMATION_TESTS

#include "AI/Combat/EnemyAttackTransitionPolicy.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEnemyAttackFacingPolicyTest,
	"ProjectEden.AI.Enemy.AttackFacing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnemyAttackFacingPolicyTest::RunTest(const FString& Parameters)
{
	// 60fps 기준 540deg/s는 한 프레임에 최대 9도만 회전해야 한다.
	const float OneFrameYaw = EnemyAttackTransitionPolicy::StepFacingYaw(0.0f, 180.0f, 540.0f, 1.0f / 60.0f);
	const float OneFrameDelta = FMath::Abs(FMath::FindDeltaAngleDegrees(0.0f, OneFrameYaw));
	TestTrue(TEXT("Facing does not snap farther than the per-frame turn budget"), OneFrameDelta <= 9.01f);

	const float WrappedYaw = EnemyAttackTransitionPolicy::StepFacingYaw(170.0f, -170.0f, 10.0f, 1.0f);
	const float WrappedRemainingAngle = FMath::Abs(FMath::FindDeltaAngleDegrees(WrappedYaw, -170.0f));
	TestTrue(TEXT("Facing crosses the yaw boundary along the shortest path"), WrappedRemainingAngle <= 10.01f);

	const float NoTimeYaw = EnemyAttackTransitionPolicy::StepFacingYaw(35.0f, 120.0f, 540.0f, 0.0f);
	TestTrue(TEXT("Facing remains unchanged when no frame time elapsed"), FMath::IsNearlyEqual(NoTimeYaw, 35.0f));

	return true;
}

#endif
