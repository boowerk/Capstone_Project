#if WITH_DEV_AUTOMATION_TESTS

#include "Characters/GP_DarkArmorKnightBossCharacter.h"
#include "Characters/GP_EnemyCharacter.h"
#include "Misc/AutomationTest.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "UObject/UnrealType.h"
#include "VFX/GP_BossTargetMarkerVFXComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBossTargetMarkerVFXConfigurationTest,
	"ProjectEden.Combat.Boss.TargetMarkerVFXConfiguration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBossTargetMarkerVFXConfigurationTest::RunTest(const FString& Parameters)
{
	// The marker is an editor-facing toggle so designers can disable the cue per inherited boss BP.
	const FBoolProperty* ToggleProperty = FindFProperty<FBoolProperty>(
		UGP_BossTargetMarkerVFXComponent::StaticClass(),
		TEXT("bTargetMarkerVFXEnabled"));
	TestNotNull(TEXT("Target marker VFX toggle is a bool property"), ToggleProperty);
	TestTrue(TEXT("Target marker VFX toggle is editable in Blueprint Details"),
		ToggleProperty && ToggleProperty->HasAnyPropertyFlags(CPF_Edit));

	UGP_BossTargetMarkerVFXComponent* ComponentFixture = NewObject<UGP_BossTargetMarkerVFXComponent>();
	TestTrue(TEXT("Target marker is enabled by default"), ComponentFixture->IsTargetMarkerVFXEnabled());
	TestNotNull(TEXT("Target marker uses the render-on-top stroke Niagara by default"), ComponentFixture->GetTargetMarkerSystem());
	TestFalse(TEXT("No marker can play without a valid target actor"), ComponentFixture->ShouldPlayTargetMarker(nullptr));

	ComponentFixture->ActiveTargetMarkerComponents.AddDefaulted();
	ComponentFixture->SetTargetMarkerVFXEnabled(false);
	TestFalse(TEXT("Disabling the marker toggle also disables playback"), ComponentFixture->IsTargetMarkerVFXEnabled());
	TestEqual(TEXT("Disabling the marker toggle clears tracked marker handles"),
		ComponentFixture->ActiveTargetMarkerComponents.Num(),
		0);

	ComponentFixture->SetTargetMarkerVFXEnabled(true);
	ComponentFixture->bTargetMarkerPlaybackStoppedForOwnerDeath = false;
	ComponentFixture->ActiveTargetMarkerComponents.AddDefaulted();
	ComponentFixture->HandleOwnerDeath();
	TestTrue(TEXT("Boss death suppresses any later target marker playback"),
		ComponentFixture->bTargetMarkerPlaybackStoppedForOwnerDeath);
	TestEqual(TEXT("Boss death clears tracked marker handles"),
		ComponentFixture->ActiveTargetMarkerComponents.Num(),
		0);

	const AGP_EnemyCharacter* EnemyDefaults = GetDefault<AGP_EnemyCharacter>();
	const AGP_DarkArmorKnightBossCharacter* DarkKnightDefaults = GetDefault<AGP_DarkArmorKnightBossCharacter>();
	TestNotNull(TEXT("Enemy base owns the reusable boss target marker component"),
		EnemyDefaults->GetBossTargetMarkerVFXComponent());
	TestNotNull(TEXT("Boss classes inherit the target marker component"),
		DarkKnightDefaults->GetBossTargetMarkerVFXComponent());
	TestFalse(TEXT("Generic enemy defaults remain non-boss despite owning the dormant component"),
		EnemyDefaults->IsBossEnemy());
	TestTrue(TEXT("Dark Knight defaults can request target marker playback as a boss"),
		DarkKnightDefaults->IsBossEnemy());

	return true;
}

#endif
