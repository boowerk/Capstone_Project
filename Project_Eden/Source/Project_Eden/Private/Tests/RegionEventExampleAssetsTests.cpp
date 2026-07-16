#if WITH_DEV_AUTOMATION_TESTS

#include "Game/RegionEvents/GP_CrystalCorruptionRegionEventActor.h"
#include "Game/RegionEvents/GP_RedRiftRegionEventActor.h"
#include "Game/RegionEvents/GP_RegionEventData.h"
#include "Game/RegionEvents/GP_RegionEventDirector.h"
#include "Game/RegionEvents/GP_RegionEventTestTriggerActor.h"
#include "Game/RegionEvents/GP_ShrineRuinsRegionEventActor.h"
#include "Game/RegionEvents/GP_StructureDefenseRegionEventActor.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

namespace GPRegionEventExampleAssetTests
{
	template <typename TClass>
	UClass* LoadClassChecked(FAutomationTestBase& Test, const TCHAR* Path)
	{
		UClass* LoadedClass = LoadClass<TClass>(nullptr, Path);
		Test.TestNotNull(FString::Printf(TEXT("Loaded class %s"), Path), LoadedClass);
		return LoadedClass;
	}

	UGP_RegionEventData* LoadEventDataChecked(FAutomationTestBase& Test, const TCHAR* Path)
	{
		UGP_RegionEventData* LoadedData = LoadObject<UGP_RegionEventData>(nullptr, Path);
		Test.TestNotNull(FString::Printf(TEXT("Loaded event data %s"), Path), LoadedData);
		return LoadedData;
	}

	float GetFloatProperty(UObject* Object, const FName PropertyName)
	{
		const FFloatProperty* FloatProperty = IsValid(Object) ? FindFProperty<FFloatProperty>(Object->GetClass(), PropertyName) : nullptr;
		return FloatProperty ? FloatProperty->GetPropertyValue_InContainer(Object) : -BIG_NUMBER;
	}

	int32 GetIntProperty(UObject* Object, const FName PropertyName)
	{
		const FIntProperty* IntProperty = IsValid(Object) ? FindFProperty<FIntProperty>(Object->GetClass(), PropertyName) : nullptr;
		return IntProperty ? IntProperty->GetPropertyValue_InContainer(Object) : MIN_int32;
	}

	bool GetBoolProperty(UObject* Object, const FName PropertyName)
	{
		const FBoolProperty* BoolProperty = IsValid(Object) ? FindFProperty<FBoolProperty>(Object->GetClass(), PropertyName) : nullptr;
		return BoolProperty ? BoolProperty->GetPropertyValue_InContainer(Object) : false;
	}

	UObject* GetObjectProperty(UObject* Object, const FName PropertyName)
	{
		const FProperty* Property = IsValid(Object) ? FindFProperty<FProperty>(Object->GetClass(), PropertyName) : nullptr;
		const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property);
		return ObjectProperty ? ObjectProperty->GetObjectPropertyValue_InContainer(Object) : nullptr;
	}

	int32 GetEventPoolCount(UObject* Object)
	{
		const FArrayProperty* ArrayProperty = IsValid(Object) ? FindFProperty<FArrayProperty>(Object->GetClass(), TEXT("EventPool")) : nullptr;
		if (!ArrayProperty)
		{
			return INDEX_NONE;
		}

		// The director keeps EventPool protected, so tests inspect the serialized Blueprint defaults through reflection.
		FScriptArrayHelper ArrayHelper(ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<void>(Object));
		return ArrayHelper.Num();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRegionEventExampleAssetsTest,
	"ProjectEden.Game.RegionEvents.ExampleAssets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRegionEventExampleAssetsTest::RunTest(const FString& Parameters)
{
	using namespace GPRegionEventExampleAssetTests;

	UClass* RedRiftClass = LoadClassChecked<AGP_RedRiftRegionEventActor>(
		*this,
		TEXT("/Game/RegionEvents/Examples/BP_RE_Test_RedRift.BP_RE_Test_RedRift_C"));
	UClass* CrystalClass = LoadClassChecked<AGP_CrystalCorruptionRegionEventActor>(
		*this,
		TEXT("/Game/RegionEvents/Examples/BP_RE_Test_CrystalCorruption.BP_RE_Test_CrystalCorruption_C"));
	UClass* ShrineClass = LoadClassChecked<AGP_ShrineRuinsRegionEventActor>(
		*this,
		TEXT("/Game/RegionEvents/Examples/BP_RE_Test_ShrineRuins.BP_RE_Test_ShrineRuins_C"));
	UClass* DefenseClass = LoadClassChecked<AGP_StructureDefenseRegionEventActor>(
		*this,
		TEXT("/Game/RegionEvents/Examples/BP_RE_Test_StructureDefense.BP_RE_Test_StructureDefense_C"));
	UClass* DirectorClass = LoadClassChecked<AGP_RegionEventDirector>(
		*this,
		TEXT("/Game/RegionEvents/Examples/BP_RE_Test_Director_AllExamples.BP_RE_Test_Director_AllExamples_C"));
	UClass* RedRiftTriggerClass = LoadClassChecked<AGP_RegionEventTestTriggerActor>(
		*this,
		TEXT("/Game/RegionEvents/Examples/BP_RE_TestTrigger_RedRift.BP_RE_TestTrigger_RedRift_C"));
	UClass* CrystalTriggerClass = LoadClassChecked<AGP_RegionEventTestTriggerActor>(
		*this,
		TEXT("/Game/RegionEvents/Examples/BP_RE_TestTrigger_CrystalCorruption.BP_RE_TestTrigger_CrystalCorruption_C"));
	UClass* ShrineTriggerClass = LoadClassChecked<AGP_RegionEventTestTriggerActor>(
		*this,
		TEXT("/Game/RegionEvents/Examples/BP_RE_TestTrigger_ShrineRuins.BP_RE_TestTrigger_ShrineRuins_C"));
	UClass* DefenseTriggerClass = LoadClassChecked<AGP_RegionEventTestTriggerActor>(
		*this,
		TEXT("/Game/RegionEvents/Examples/BP_RE_TestTrigger_StructureDefense.BP_RE_TestTrigger_StructureDefense_C"));

	UGP_RegionEventData* RedRiftData = LoadEventDataChecked(*this, TEXT("/Game/RegionEvents/Examples/DA_RE_Test_RedRift.DA_RE_Test_RedRift"));
	UGP_RegionEventData* CrystalData = LoadEventDataChecked(*this, TEXT("/Game/RegionEvents/Examples/DA_RE_Test_CrystalCorruption.DA_RE_Test_CrystalCorruption"));
	UGP_RegionEventData* ShrineData = LoadEventDataChecked(*this, TEXT("/Game/RegionEvents/Examples/DA_RE_Test_ShrineRuins.DA_RE_Test_ShrineRuins"));
	UGP_RegionEventData* DefenseData = LoadEventDataChecked(*this, TEXT("/Game/RegionEvents/Examples/DA_RE_Test_StructureDefense.DA_RE_Test_StructureDefense"));

	TestTrue(TEXT("Red Rift data points at its test Blueprint"), IsValid(RedRiftData) && RedRiftData->EventActorClass == RedRiftClass);
	TestTrue(TEXT("Crystal Corruption data points at its test Blueprint"), IsValid(CrystalData) && CrystalData->EventActorClass == CrystalClass);
	TestTrue(TEXT("Shrine Ruins data points at its test Blueprint"), IsValid(ShrineData) && ShrineData->EventActorClass == ShrineClass);
	TestTrue(TEXT("Structure Defense data points at its test Blueprint"), IsValid(DefenseData) && DefenseData->EventActorClass == DefenseClass);

	TestEqual(TEXT("All example events are ZoneStarted for quick smoke tests"), RedRiftData->Trigger, EGPRegionEventTrigger::ZoneStarted);
	TestEqual(TEXT("Crystal example completes through crystal destruction"), CrystalData->DurationSeconds, -1.0f);
	TestTrue(TEXT("Red Rift example spawns a wave composition"), RedRiftData->EnemySpawns.Num() > 0);
	TestTrue(TEXT("Structure Defense example spawns defense waves"), DefenseData->EnemySpawns.Num() > 0);
	TestEqual(TEXT("Shrine example opens the reward UI without extra enemy composition"), ShrineData->EnemySpawns.Num(), 0);

	TestEqual(TEXT("Red Rift test BP starts its first wave quickly"),
		GetFloatProperty(IsValid(RedRiftClass) ? RedRiftClass->GetDefaultObject() : nullptr, TEXT("InitialWaveDelaySeconds")),
		2.0f);
	TestEqual(TEXT("Red Rift test BP caps wave count"),
		GetIntProperty(IsValid(RedRiftClass) ? RedRiftClass->GetDefaultObject() : nullptr, TEXT("MaxWaveCount")),
		3);
	TestEqual(TEXT("Structure Defense test BP has a short defense timer"),
		GetFloatProperty(IsValid(DefenseClass) ? DefenseClass->GetDefaultObject() : nullptr, TEXT("DefenseDurationSeconds")),
		25.0f);
	TestEqual(TEXT("Test director pools all four examples"),
		GetEventPoolCount(IsValid(DirectorClass) ? DirectorClass->GetDefaultObject() : nullptr),
		4);

	UObject* RedRiftTriggerDefaults = IsValid(RedRiftTriggerClass) ? RedRiftTriggerClass->GetDefaultObject() : nullptr;
	UObject* CrystalTriggerDefaults = IsValid(CrystalTriggerClass) ? CrystalTriggerClass->GetDefaultObject() : nullptr;
	UObject* ShrineTriggerDefaults = IsValid(ShrineTriggerClass) ? ShrineTriggerClass->GetDefaultObject() : nullptr;
	UObject* DefenseTriggerDefaults = IsValid(DefenseTriggerClass) ? DefenseTriggerClass->GetDefaultObject() : nullptr;
	const AGP_RegionEventTestTriggerActor* RedRiftTriggerActorDefaults = Cast<AGP_RegionEventTestTriggerActor>(RedRiftTriggerDefaults);
	const AGP_RegionEventTestTriggerActor* CrystalTriggerActorDefaults = Cast<AGP_RegionEventTestTriggerActor>(CrystalTriggerDefaults);
	const AGP_RegionEventTestTriggerActor* ShrineTriggerActorDefaults = Cast<AGP_RegionEventTestTriggerActor>(ShrineTriggerDefaults);
	const AGP_RegionEventTestTriggerActor* DefenseTriggerActorDefaults = Cast<AGP_RegionEventTestTriggerActor>(DefenseTriggerDefaults);

	// Direct test triggers are intended for designers: drop one BP in any PIE map and that single event starts immediately.
	TestTrue(TEXT("Red Rift direct trigger starts on BeginPlay"), GetBoolProperty(RedRiftTriggerDefaults, TEXT("bTriggerOnBeginPlay")));
	TestTrue(TEXT("Crystal direct trigger starts on BeginPlay"), GetBoolProperty(CrystalTriggerDefaults, TEXT("bTriggerOnBeginPlay")));
	TestTrue(TEXT("Shrine direct trigger starts on BeginPlay"), GetBoolProperty(ShrineTriggerDefaults, TEXT("bTriggerOnBeginPlay")));
	TestTrue(TEXT("Defense direct trigger starts on BeginPlay"), GetBoolProperty(DefenseTriggerDefaults, TEXT("bTriggerOnBeginPlay")));
	TestTrue(TEXT("Red Rift direct trigger uses Red Rift data"), GetObjectProperty(RedRiftTriggerDefaults, TEXT("EventData")) == RedRiftData);
	TestTrue(TEXT("Crystal direct trigger uses Crystal data"), GetObjectProperty(CrystalTriggerDefaults, TEXT("EventData")) == CrystalData);
	TestTrue(TEXT("Shrine direct trigger uses Shrine data"), GetObjectProperty(ShrineTriggerDefaults, TEXT("EventData")) == ShrineData);
	TestTrue(TEXT("Defense direct trigger uses Defense data"), GetObjectProperty(DefenseTriggerDefaults, TEXT("EventData")) == DefenseData);
	TestTrue(TEXT("Red Rift trigger is server-authoritative in multiplayer PIE"),
		IsValid(RedRiftTriggerActorDefaults) && RedRiftTriggerActorDefaults->GetIsReplicated());
	TestTrue(TEXT("Crystal trigger is server-authoritative in multiplayer PIE"),
		IsValid(CrystalTriggerActorDefaults) && CrystalTriggerActorDefaults->GetIsReplicated());
	TestTrue(TEXT("Shrine trigger is server-authoritative in multiplayer PIE"),
		IsValid(ShrineTriggerActorDefaults) && ShrineTriggerActorDefaults->GetIsReplicated());
	TestTrue(TEXT("Defense trigger is server-authoritative in multiplayer PIE"),
		IsValid(DefenseTriggerActorDefaults) && DefenseTriggerActorDefaults->GetIsReplicated());

	return true;
}

#endif
