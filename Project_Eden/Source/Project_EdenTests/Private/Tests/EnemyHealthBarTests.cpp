#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/GP_AttributeSet.h"
#include "Characters/GP_EnemyCharacter.h"
#include "Components/WidgetComponent.h"
#include "Misc/AutomationTest.h"
#include "UI/GP_WidgetComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEnemyHealthBarDefaultsTest,
	"ProjectEden.UI.EnemyHealthBar.Defaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnemyHealthBarDefaultsTest::RunTest(const FString& Parameters)
{
	const AGP_EnemyCharacter* EnemyDefaults = GetDefault<AGP_EnemyCharacter>();
	UGP_WidgetComponent* HealthBarComponent = EnemyDefaults->GetWorldHealthBarComponent();

	if (!TestNotNull(TEXT("Enemy owns a native world health bar component"), HealthBarComponent))
	{
		return false;
	}

	TestTrue(TEXT("Enemy health bar widget asset is assigned"), HealthBarComponent->GetWidgetClass() != nullptr);
	TestEqual(TEXT("Enemy health bar renders in screen space"), HealthBarComponent->GetWidgetSpace(), EWidgetSpace::Screen);
	TestTrue(TEXT("Enemy health bar sizes itself from the widget asset"), HealthBarComponent->GetDrawAtDesiredSize());
	TestEqual(TEXT("Enemy health bar never blocks gameplay collision"), HealthBarComponent->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
	TestTrue(
		TEXT("Enemy health bar tracks the shared GAS health pair"),
		HealthBarComponent->TracksAttributePair(
			UGP_AttributeSet::GetHealthAttribute(),
			UGP_AttributeSet::GetMaxHealthAttribute()));

	return true;
}

#endif
