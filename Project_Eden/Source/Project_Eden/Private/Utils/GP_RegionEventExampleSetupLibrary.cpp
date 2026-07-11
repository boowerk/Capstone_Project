#include "Utils/GP_RegionEventExampleSetupLibrary.h"

#if WITH_EDITOR

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Characters/GP_EnemyCharacter.h"
#include "Engine/Blueprint.h"
#include "Factories/BlueprintFactory.h"
#include "Factories/DataAssetFactory.h"
#include "Game/RegionEvents/GP_CrystalCorruptionRegionEventActor.h"
#include "Game/RegionEvents/GP_RedRiftRegionEventActor.h"
#include "Game/RegionEvents/GP_RegionEventData.h"
#include "Game/RegionEvents/GP_RegionEventDirector.h"
#include "Game/RegionEvents/GP_RegionEventTestTriggerActor.h"
#include "Game/RegionEvents/GP_ShrineRuinsRegionEventActor.h"
#include "Game/RegionEvents/GP_StructureDefenseRegionEventActor.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"

namespace GPRegionEventExamples
{
	const FString ExamplePackagePath = TEXT("/Game/RegionEvents/Examples");
	const FString BasicEnemyMeleeClassPath = TEXT("/Game/Characters/EnemyCharacter/Basic/BP_BasicEnemy_Melee.BP_BasicEnemy_Melee_C");
	const FString BasicEnemyRangedClassPath = TEXT("/Game/Characters/EnemyCharacter/Basic/BP_BasicEnemy_Ranged.BP_BasicEnemy_Ranged_C");

	bool SaveAsset(UObject* Asset)
	{
		if (!IsValid(Asset))
		{
			return false;
		}

		UPackage* Package = Asset->GetOutermost();
		if (!IsValid(Package))
		{
			return false;
		}

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;

		return UPackage::SavePackage(
			Package,
			Asset,
			*FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension()),
			SaveArgs);
	}

	template <typename TObjectType>
	TObjectType* LoadAsset(const FString& PackagePath, const FString& AssetName)
	{
		return LoadObject<TObjectType>(nullptr, *FString::Printf(TEXT("%s/%s.%s"), *PackagePath, *AssetName, *AssetName));
	}

	UBlueprint* CreateOrLoadActorBlueprint(const FString& AssetName, UClass* ParentClass)
	{
		if (!IsValid(ParentClass))
		{
			return nullptr;
		}

		if (UBlueprint* ExistingBlueprint = LoadAsset<UBlueprint>(ExamplePackagePath, AssetName))
		{
			if (ExistingBlueprint->ParentClass != ParentClass)
			{
				UE_LOG(LogTemp, Warning, TEXT("[RegionEventExamples] %s already exists with parent %s instead of %s."),
					*AssetName,
					*GetNameSafe(ExistingBlueprint->ParentClass),
					*GetNameSafe(ParentClass));
			}

			FKismetEditorUtilities::CompileBlueprint(ExistingBlueprint);
			return ExistingBlueprint;
		}

		UBlueprintFactory* Factory = NewObject<UBlueprintFactory>();
		Factory->ParentClass = ParentClass;

		UBlueprint* NewBlueprint = Cast<UBlueprint>(FAssetToolsModule::GetModule().Get().CreateAsset(
			AssetName,
			ExamplePackagePath,
			UBlueprint::StaticClass(),
			Factory));
		if (IsValid(NewBlueprint))
		{
			// Compile immediately so DataAssets can reference the generated class instead of the native parent.
			FKismetEditorUtilities::CompileBlueprint(NewBlueprint);
			FAssetRegistryModule::AssetCreated(NewBlueprint);
			NewBlueprint->MarkPackageDirty();
		}

		return NewBlueprint;
	}

	UGP_RegionEventData* CreateOrLoadEventData(const FString& AssetName)
	{
		if (UGP_RegionEventData* ExistingData = LoadAsset<UGP_RegionEventData>(ExamplePackagePath, AssetName))
		{
			return ExistingData;
		}

		UDataAssetFactory* Factory = NewObject<UDataAssetFactory>();
		Factory->DataAssetClass = UGP_RegionEventData::StaticClass();

		UGP_RegionEventData* NewData = Cast<UGP_RegionEventData>(FAssetToolsModule::GetModule().Get().CreateAsset(
			AssetName,
			ExamplePackagePath,
			UGP_RegionEventData::StaticClass(),
			Factory));
		if (IsValid(NewData))
		{
			FAssetRegistryModule::AssetCreated(NewData);
			NewData->MarkPackageDirty();
		}

		return NewData;
	}

	FGP_EnemySpawnEntry MakeEnemyEntry(TSubclassOf<AGP_EnemyCharacter> EnemyClass, int32 Count)
	{
		FGP_EnemySpawnEntry Entry;
		Entry.EnemyClass = EnemyClass;
		Entry.Count = FMath::Max(1, Count);
		return Entry;
	}

	bool SetFloatProperty(UObject* Object, const FName PropertyName, float Value)
	{
		if (FFloatProperty* FloatProperty = IsValid(Object) ? FindFProperty<FFloatProperty>(Object->GetClass(), PropertyName) : nullptr)
		{
			FloatProperty->SetPropertyValue_InContainer(Object, Value);
			return true;
		}

		return false;
	}

	bool SetIntProperty(UObject* Object, const FName PropertyName, int32 Value)
	{
		if (FIntProperty* IntProperty = IsValid(Object) ? FindFProperty<FIntProperty>(Object->GetClass(), PropertyName) : nullptr)
		{
			IntProperty->SetPropertyValue_InContainer(Object, Value);
			return true;
		}

		return false;
	}

	bool SetBoolProperty(UObject* Object, const FName PropertyName, bool bValue)
	{
		if (FBoolProperty* BoolProperty = IsValid(Object) ? FindFProperty<FBoolProperty>(Object->GetClass(), PropertyName) : nullptr)
		{
			BoolProperty->SetPropertyValue_InContainer(Object, bValue);
			return true;
		}

		return false;
	}

	bool SetObjectProperty(UObject* Object, const FName PropertyName, UObject* Value)
	{
		FProperty* Property = IsValid(Object) ? FindFProperty<FProperty>(Object->GetClass(), PropertyName) : nullptr;
		if (FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property))
		{
			ObjectProperty->SetObjectPropertyValue_InContainer(Object, Value);
			return true;
		}

		return false;
	}

	bool SetTextProperty(UObject* Object, const FName PropertyName, const FText& Value)
	{
		if (FTextProperty* TextProperty = IsValid(Object) ? FindFProperty<FTextProperty>(Object->GetClass(), PropertyName) : nullptr)
		{
			TextProperty->SetPropertyValue_InContainer(Object, Value);
			return true;
		}

		return false;
	}

	bool SetLinearColorProperty(UObject* Object, const FName PropertyName, const FLinearColor& Value)
	{
		FStructProperty* StructProperty = IsValid(Object) ? FindFProperty<FStructProperty>(Object->GetClass(), PropertyName) : nullptr;
		if (StructProperty && StructProperty->Struct == TBaseStructure<FLinearColor>::Get())
		{
			// Keep protected Blueprint presentation settings editable while assigning deterministic generated defaults.
			*StructProperty->ContainerPtrToValuePtr<FLinearColor>(Object) = Value;
			return true;
		}

		return false;
	}

	bool SetEventPool(UObject* Object, const TArray<UGP_RegionEventData*>& EventDataAssets)
	{
		FArrayProperty* ArrayProperty = IsValid(Object) ? FindFProperty<FArrayProperty>(Object->GetClass(), TEXT("EventPool")) : nullptr;
		FObjectPropertyBase* InnerObjectProperty = ArrayProperty ? CastField<FObjectPropertyBase>(ArrayProperty->Inner) : nullptr;
		if (!ArrayProperty || !InnerObjectProperty)
		{
			return false;
		}

		// Protected Blueprint defaults are adjusted through reflection so designers still see normal Details-panel values.
		FScriptArrayHelper ArrayHelper(ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<void>(Object));
		ArrayHelper.EmptyAndAddValues(EventDataAssets.Num());
		for (int32 Index = 0; Index < EventDataAssets.Num(); ++Index)
		{
			InnerObjectProperty->SetObjectPropertyValue(ArrayHelper.GetRawPtr(Index), EventDataAssets[Index]);
		}

		return true;
	}

	UClass* ResolveBlueprintGeneratedClass(UBlueprint* Blueprint)
	{
		if (!IsValid(Blueprint))
		{
			return nullptr;
		}

		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		return Blueprint->GeneratedClass;
	}

	void MarkBlueprintDefaultsChanged(UBlueprint* Blueprint)
	{
		if (!IsValid(Blueprint))
		{
			return;
		}

		FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
		Blueprint->MarkPackageDirty();
	}

	bool ConfigureRedRiftData(UGP_RegionEventData* Data, UClass* ActorClass, TSubclassOf<AGP_EnemyCharacter> MeleeClass, TSubclassOf<AGP_EnemyCharacter> RangedClass)
	{
		if (!IsValid(Data) || !IsValid(ActorClass))
		{
			return false;
		}

		Data->EventId = TEXT("test_red_rift_wave");
		Data->DisplayName = FText::FromString(TEXT("테스트 - 붉은 균열 지역"));
		Data->Description = FText::FromString(TEXT("일정 시간마다 기본 적 웨이브를 소환하는 테스트용 지역 이벤트입니다."));
		Data->EventType = EGPRegionEventType::CorruptionRift;
		Data->Trigger = EGPRegionEventTrigger::ZoneStarted;
		Data->EventActorClass = ActorClass;
		Data->SelectionWeight = 1.0f;
		Data->DurationSeconds = 40.0f;
		Data->bApplyActiveRegionState = true;
		Data->ActiveRegionState = 2;
		Data->bApplyCompletedRegionState = false;
		Data->EnemySpawnScatterRadius = 650.0f;
		Data->EnemySpawns.Reset();
		if (*MeleeClass)
		{
			Data->EnemySpawns.Add(MakeEnemyEntry(MeleeClass, 2));
		}
		if (*RangedClass)
		{
			Data->EnemySpawns.Add(MakeEnemyEntry(RangedClass, 1));
		}
		Data->MarkPackageDirty();
		return true;
	}

	bool ConfigureCrystalCorruptionData(UGP_RegionEventData* Data, UClass* ActorClass)
	{
		if (!IsValid(Data) || !IsValid(ActorClass))
		{
			return false;
		}

		Data->EventId = TEXT("test_crystal_corruption");
		Data->DisplayName = FText::FromString(TEXT("테스트 - 수정 오염 지역"));
		Data->Description = FText::FromString(TEXT("플레이어 이동 속도를 낮추고, 생성된 수정을 파괴하면 해제되는 테스트용 지역 이벤트입니다."));
		Data->EventType = EGPRegionEventType::EnvironmentalHazard;
		Data->Trigger = EGPRegionEventTrigger::ZoneStarted;
		Data->EventActorClass = ActorClass;
		Data->SelectionWeight = 1.0f;
		Data->DurationSeconds = -1.0f;
		Data->bApplyActiveRegionState = true;
		Data->ActiveRegionState = 2;
		Data->bApplyCompletedRegionState = false;
		Data->EnemySpawns.Reset();
		Data->EnemySpawnScatterRadius = 450.0f;
		Data->MarkPackageDirty();
		return true;
	}

	bool ConfigureShrineRuinsData(UGP_RegionEventData* Data, UClass* ActorClass)
	{
		if (!IsValid(Data) || !IsValid(ActorClass))
		{
			return false;
		}

		Data->EventId = TEXT("test_shrine_ruins");
		Data->DisplayName = FText::FromString(TEXT("테스트 - 신전 잔해"));
		Data->Description = FText::FromString(TEXT("플레이어가 신전 반경에 들어오면 강화 선택지를 여는 테스트용 지역 이벤트입니다."));
		Data->EventType = EGPRegionEventType::ShrineCache;
		Data->Trigger = EGPRegionEventTrigger::ZoneStarted;
		Data->EventActorClass = ActorClass;
		Data->SelectionWeight = 1.0f;
		Data->DurationSeconds = 60.0f;
		Data->bApplyActiveRegionState = false;
		Data->bApplyCompletedRegionState = false;
		Data->EnemySpawns.Reset();
		Data->MarkPackageDirty();
		return true;
	}

	bool ConfigureStructureDefenseData(UGP_RegionEventData* Data, UClass* ActorClass, TSubclassOf<AGP_EnemyCharacter> MeleeClass, TSubclassOf<AGP_EnemyCharacter> RangedClass)
	{
		if (!IsValid(Data) || !IsValid(ActorClass))
		{
			return false;
		}

		Data->EventId = TEXT("test_structure_defense");
		Data->DisplayName = FText::FromString(TEXT("테스트 - 구조물 방어"));
		Data->Description = FText::FromString(TEXT("구조물 주변에서 짧은 시간 버티며 웨이브를 막는 테스트용 지역 이벤트입니다."));
		Data->EventType = EGPRegionEventType::EnemyAmbush;
		Data->Trigger = EGPRegionEventTrigger::ZoneStarted;
		Data->EventActorClass = ActorClass;
		Data->SelectionWeight = 1.0f;
		Data->DurationSeconds = 30.0f;
		Data->bApplyActiveRegionState = false;
		Data->bApplyCompletedRegionState = false;
		Data->EnemySpawnScatterRadius = 700.0f;
		Data->EnemySpawns.Reset();
		if (*MeleeClass)
		{
			Data->EnemySpawns.Add(MakeEnemyEntry(MeleeClass, 2));
		}
		if (*RangedClass)
		{
			Data->EnemySpawns.Add(MakeEnemyEntry(RangedClass, 1));
		}
		Data->MarkPackageDirty();
		return true;
	}
}

#endif

bool UGP_RegionEventExampleSetupLibrary::CreateOrUpdateRegionEventExampleAssets()
{
#if WITH_EDITOR
	using namespace GPRegionEventExamples;

	UBlueprint* RedRiftBlueprint = CreateOrLoadActorBlueprint(TEXT("BP_RE_Test_RedRift"), AGP_RedRiftRegionEventActor::StaticClass());
	UBlueprint* CrystalBlueprint = CreateOrLoadActorBlueprint(TEXT("BP_RE_Test_CrystalCorruption"), AGP_CrystalCorruptionRegionEventActor::StaticClass());
	UBlueprint* ShrineBlueprint = CreateOrLoadActorBlueprint(TEXT("BP_RE_Test_ShrineRuins"), AGP_ShrineRuinsRegionEventActor::StaticClass());
	UBlueprint* DefenseBlueprint = CreateOrLoadActorBlueprint(TEXT("BP_RE_Test_StructureDefense"), AGP_StructureDefenseRegionEventActor::StaticClass());
	UBlueprint* DirectorBlueprint = CreateOrLoadActorBlueprint(TEXT("BP_RE_Test_Director_AllExamples"), AGP_RegionEventDirector::StaticClass());
	UBlueprint* RedRiftTriggerBlueprint = CreateOrLoadActorBlueprint(TEXT("BP_RE_TestTrigger_RedRift"), AGP_RegionEventTestTriggerActor::StaticClass());
	UBlueprint* CrystalTriggerBlueprint = CreateOrLoadActorBlueprint(TEXT("BP_RE_TestTrigger_CrystalCorruption"), AGP_RegionEventTestTriggerActor::StaticClass());
	UBlueprint* ShrineTriggerBlueprint = CreateOrLoadActorBlueprint(TEXT("BP_RE_TestTrigger_ShrineRuins"), AGP_RegionEventTestTriggerActor::StaticClass());
	UBlueprint* DefenseTriggerBlueprint = CreateOrLoadActorBlueprint(TEXT("BP_RE_TestTrigger_StructureDefense"), AGP_RegionEventTestTriggerActor::StaticClass());

	if (!IsValid(RedRiftBlueprint) || !IsValid(CrystalBlueprint) || !IsValid(ShrineBlueprint)
		|| !IsValid(DefenseBlueprint) || !IsValid(DirectorBlueprint)
		|| !IsValid(RedRiftTriggerBlueprint) || !IsValid(CrystalTriggerBlueprint)
		|| !IsValid(ShrineTriggerBlueprint) || !IsValid(DefenseTriggerBlueprint))
	{
		return false;
	}

	UClass* RedRiftClass = ResolveBlueprintGeneratedClass(RedRiftBlueprint);
	UClass* CrystalClass = ResolveBlueprintGeneratedClass(CrystalBlueprint);
	UClass* ShrineClass = ResolveBlueprintGeneratedClass(ShrineBlueprint);
	UClass* DefenseClass = ResolveBlueprintGeneratedClass(DefenseBlueprint);

	// Actor BP defaults are short and noisy on purpose so PIE smoke tests provide feedback quickly.
	if (UObject* RedRiftDefaults = IsValid(RedRiftClass) ? RedRiftClass->GetDefaultObject() : nullptr)
	{
		SetFloatProperty(RedRiftDefaults, TEXT("InitialWaveDelaySeconds"), 2.0f);
		SetFloatProperty(RedRiftDefaults, TEXT("WaveIntervalSeconds"), 6.0f);
		SetIntProperty(RedRiftDefaults, TEXT("MaxWaveCount"), 3);
		MarkBlueprintDefaultsChanged(RedRiftBlueprint);
	}

	if (UObject* CrystalDefaults = IsValid(CrystalClass) ? CrystalClass->GetDefaultObject() : nullptr)
	{
		SetIntProperty(CrystalDefaults, TEXT("CrystalCount"), 3);
		SetFloatProperty(CrystalDefaults, TEXT("CrystalRingRadius"), 520.0f);
		SetFloatProperty(CrystalDefaults, TEXT("SlowRadius"), 1400.0f);
		SetFloatProperty(CrystalDefaults, TEXT("SlowMultiplier"), 0.55f);
		MarkBlueprintDefaultsChanged(CrystalBlueprint);
	}

	if (UObject* ShrineDefaults = IsValid(ShrineClass) ? ShrineClass->GetDefaultObject() : nullptr)
	{
		SetBoolProperty(ShrineDefaults, TEXT("bCompleteAfterFirstClaim"), true);
		SetFloatProperty(ShrineDefaults, TEXT("ActivationRadius"), 850.0f);
		MarkBlueprintDefaultsChanged(ShrineBlueprint);
	}

	if (UObject* DefenseDefaults = IsValid(DefenseClass) ? DefenseClass->GetDefaultObject() : nullptr)
	{
		SetFloatProperty(DefenseDefaults, TEXT("DefenseDurationSeconds"), 25.0f);
		SetFloatProperty(DefenseDefaults, TEXT("DefenseWaveIntervalSeconds"), 8.0f);
		SetBoolProperty(DefenseDefaults, TEXT("bSpawnInitialDefenseWave"), true);
		MarkBlueprintDefaultsChanged(DefenseBlueprint);
	}

	TSubclassOf<AGP_EnemyCharacter> MeleeClass = LoadClass<AGP_EnemyCharacter>(nullptr, *BasicEnemyMeleeClassPath);
	TSubclassOf<AGP_EnemyCharacter> RangedClass = LoadClass<AGP_EnemyCharacter>(nullptr, *BasicEnemyRangedClassPath);
	if (!*MeleeClass || !*RangedClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RegionEventExamples] Basic enemy classes were not fully loaded. Created event data may spawn fewer enemies."));
	}

	UGP_RegionEventData* RedRiftData = CreateOrLoadEventData(TEXT("DA_RE_Test_RedRift"));
	UGP_RegionEventData* CrystalData = CreateOrLoadEventData(TEXT("DA_RE_Test_CrystalCorruption"));
	UGP_RegionEventData* ShrineData = CreateOrLoadEventData(TEXT("DA_RE_Test_ShrineRuins"));
	UGP_RegionEventData* DefenseData = CreateOrLoadEventData(TEXT("DA_RE_Test_StructureDefense"));

	const bool bConfiguredData =
		ConfigureRedRiftData(RedRiftData, RedRiftClass, MeleeClass, RangedClass)
		& ConfigureCrystalCorruptionData(CrystalData, CrystalClass)
		& ConfigureShrineRuinsData(ShrineData, ShrineClass)
		& ConfigureStructureDefenseData(DefenseData, DefenseClass, MeleeClass, RangedClass);

	UClass* DirectorClass = ResolveBlueprintGeneratedClass(DirectorBlueprint);
	if (UObject* DirectorDefaults = IsValid(DirectorClass) ? DirectorClass->GetDefaultObject() : nullptr)
	{
		SetBoolProperty(DirectorDefaults, TEXT("bEnableRegionEvents"), true);
		SetBoolProperty(DirectorDefaults, TEXT("bAutoActivateSpawnedEvents"), true);
		SetIntProperty(DirectorDefaults, TEXT("MaxActiveEvents"), 4);
		SetIntProperty(DirectorDefaults, TEXT("RandomSeed"), 4207);
		TArray<UGP_RegionEventData*> EventPool = { RedRiftData, CrystalData, ShrineData, DefenseData };
		SetEventPool(DirectorDefaults, EventPool);
		MarkBlueprintDefaultsChanged(DirectorBlueprint);
	}

	struct FTriggerBlueprintConfig
	{
		UBlueprint* Blueprint = nullptr;
		UGP_RegionEventData* Data = nullptr;
		FString StationTitle;
		FLinearColor StationColor = FLinearColor::White;
		int32 RegionId = 0;
	};

	const FTriggerBlueprintConfig TriggerConfigs[] =
	{
		{ RedRiftTriggerBlueprint, RedRiftData, TEXT("RED RIFT WAVE"), FLinearColor(1.0f, 0.03f, 0.01f, 1.0f), 0 },
		{ CrystalTriggerBlueprint, CrystalData, TEXT("CRYSTAL CORRUPTION"), FLinearColor(0.28f, 0.65f, 1.0f, 1.0f), 1 },
		{ ShrineTriggerBlueprint, ShrineData, TEXT("SHRINE RUINS"), FLinearColor(1.0f, 0.63f, 0.08f, 1.0f), 2 },
		{ DefenseTriggerBlueprint, DefenseData, TEXT("STRUCTURE DEFENSE"), FLinearColor(0.08f, 1.0f, 0.38f, 1.0f), 3 },
	};

	for (const FTriggerBlueprintConfig& TriggerConfig : TriggerConfigs)
	{
		UClass* TriggerClass = ResolveBlueprintGeneratedClass(TriggerConfig.Blueprint);
		UObject* TriggerDefaults = IsValid(TriggerClass) ? TriggerClass->GetDefaultObject() : nullptr;
		if (!IsValid(TriggerDefaults))
		{
			continue;
		}

		// These trigger BPs are intentionally standalone: drag one into a level and PIE starts that single event.
		SetObjectProperty(TriggerDefaults, TEXT("EventData"), TriggerConfig.Data);
		SetBoolProperty(TriggerDefaults, TEXT("bTriggerOnBeginPlay"), true);
		SetBoolProperty(TriggerDefaults, TEXT("bTriggerOnPlayerOverlap"), false);
		SetBoolProperty(TriggerDefaults, TEXT("bAutoActivateSpawnedEvent"), true);
		SetFloatProperty(TriggerDefaults, TEXT("TriggerRadius"), 600.0f);
		SetIntProperty(TriggerDefaults, TEXT("TestRegionId"), TriggerConfig.RegionId);
		SetTextProperty(TriggerDefaults, TEXT("StationTitle"), FText::FromString(TriggerConfig.StationTitle));
		SetLinearColorProperty(TriggerDefaults, TEXT("StationColor"), TriggerConfig.StationColor);
		MarkBlueprintDefaultsChanged(TriggerConfig.Blueprint);
	}

	TArray<UObject*> AssetsToSave =
	{
		RedRiftBlueprint,
		CrystalBlueprint,
		ShrineBlueprint,
		DefenseBlueprint,
		DirectorBlueprint,
		RedRiftTriggerBlueprint,
		CrystalTriggerBlueprint,
		ShrineTriggerBlueprint,
		DefenseTriggerBlueprint,
		RedRiftData,
		CrystalData,
		ShrineData,
		DefenseData,
	};

	bool bSavedAll = true;
	for (UObject* Asset : AssetsToSave)
	{
		bSavedAll &= SaveAsset(Asset);
	}

	return bConfiguredData && bSavedAll;
#else
	return false;
#endif
}
