#if WITH_DEV_AUTOMATION_TESTS

#include "Animation/AnimBlueprint.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Animation/PDA_EnemyAnimationSet.h"
#include "Characters/GP_CrystalSeraphBossCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Blueprint.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCrystalSeraphAnimationSetupTest,
	"ProjectEden.Combat.CrystalSeraph.AnimationSetup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCrystalSeraphAnimationSetupTest::RunTest(const FString& Parameters)
{
	UBlueprint* BossBlueprint = LoadObject<UBlueprint>(
		nullptr,
		TEXT("/Game/Characters/EnemyCharacter/Boss/BP_Boss_CrystalSeraph/BP_Crystal_Seraph.BP_Crystal_Seraph"));
	TestNotNull(TEXT("Crystal Seraph boss Blueprint exists"), BossBlueprint);
	if (!BossBlueprint || !BossBlueprint->GeneratedClass)
	{
		return false;
	}

	const AGP_CrystalSeraphBossCharacter* BossDefaults = Cast<AGP_CrystalSeraphBossCharacter>(BossBlueprint->GeneratedClass->GetDefaultObject());
	TestNotNull(TEXT("Crystal Seraph Blueprint uses the native Crystal Seraph parent"), BossDefaults);
	if (!BossDefaults)
	{
		return false;
	}

	const UAnimBlueprint* AnimBlueprint = LoadObject<UAnimBlueprint>(
		nullptr,
		TEXT("/Game/Characters/EnemyCharacter/Boss/BP_Boss_CrystalSeraph/Animation/ABP_CrystalSeraph.ABP_CrystalSeraph"));
	const UAnimMontage* BasicMontage = LoadObject<UAnimMontage>(
		nullptr,
		TEXT("/Game/Characters/EnemyCharacter/Boss/BP_Boss_CrystalSeraph/Animation/AM_CrystalSeraph_Basic_Simple.AM_CrystalSeraph_Basic_Simple"));
	const UAnimMontage* LaserMontage = LoadObject<UAnimMontage>(
		nullptr,
		TEXT("/Game/Characters/EnemyCharacter/Boss/BP_Boss_CrystalSeraph/Animation/AM_CrystalSeraph_Laser_Double.AM_CrystalSeraph_Laser_Double"));
	const UAnimSequence* HoverIdle = LoadObject<UAnimSequence>(
		nullptr,
		TEXT("/Game/Characters/EnemyCharacter/Boss/BP_Boss_CrystalSeraph/Animation/TravelMode_Hover_Idle.TravelMode_Hover_Idle"));
	const UPDA_EnemyAnimationSet* AnimationSet = LoadObject<UPDA_EnemyAnimationSet>(
		nullptr,
		TEXT("/Game/Characters/EnemyCharacter/Boss/BP_Boss_CrystalSeraph/Animation/PDA_CrystalSeraphAnimationSet.PDA_CrystalSeraphAnimationSet"));

	TestNotNull(TEXT("Crystal Seraph ABP exists"), AnimBlueprint);
	TestNotNull(TEXT("Simple spell montage is available for the basic attack"), BasicMontage);
	TestNotNull(TEXT("Double spell montage is available for the laser attack"), LaserMontage);
	TestNotNull(TEXT("Hover idle animation is available"), HoverIdle);
	TestNotNull(TEXT("Crystal Seraph enemy animation set exists"), AnimationSet);

	TestTrue(TEXT("Basic pattern montage points at the Simple spell montage"),
		BossDefaults->GetCrystalSeraphBasicAttackMontage() == BasicMontage);
	TestTrue(TEXT("Laser pattern montage points at the Double spell montage"),
		BossDefaults->GetCrystalSeraphLaserAttackMontage() == LaserMontage);

	USkeletalMeshComponent* MeshComponent = BossDefaults->GetMesh();
	TestNotNull(TEXT("Crystal Seraph defaults keep a skeletal mesh component"), MeshComponent);
	TestTrue(TEXT("Crystal Seraph mesh uses the generated ABP class"),
		MeshComponent && AnimBlueprint && MeshComponent->GetAnimClass() == AnimBlueprint->GeneratedClass);

	TestTrue(TEXT("Enemy animation set is assigned on the boss defaults"),
		BossDefaults->GetEnemyAnimationSet() == AnimationSet);
	TestTrue(TEXT("Enemy animation set uses TravelMode_Hover_Idle as idle"),
		(AnimationSet ? AnimationSet->IdleAnimation.Get() : nullptr) == HoverIdle);
	TestTrue(TEXT("Enemy animation set exposes both Simple and Double spell montages"),
		AnimationSet
		&& AnimationSet->LightAttackMontages.Contains(BasicMontage)
		&& AnimationSet->LightAttackMontages.Contains(LaserMontage));

	return true;
}

#endif
