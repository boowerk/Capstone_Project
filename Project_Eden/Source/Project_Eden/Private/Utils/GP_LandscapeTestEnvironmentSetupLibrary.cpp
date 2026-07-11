#include "Utils/GP_LandscapeTestEnvironmentSetupLibrary.h"

#if WITH_EDITOR

#include "ActorFactories/ActorFactory.h"
#include "AI/NavigationSystemBase.h"
#include "Builders/CubeBuilder.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "FileHelpers.h"
#include "Game/Corruption/GP_CorruptionTestZoneActor.h"
#include "Game/RegionEvents/GP_RegionEventTestTriggerActor.h"
#include "Game/Testing/GP_LandscapeTestFloorActor.h"
#include "Game/Testing/GP_LandscapeTestInstructionActor.h"
#include "GameFramework/PlayerStart.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "NavMesh/NavMeshBoundsVolume.h"

namespace GPLandscapeTestEnvironment
{
	constexpr TCHAR LandscapeMapPath[] = TEXT("/Game/Maps/MainMap/L_LandscapeMap");
	constexpr float TestDeckElevation = 5000.0f;
	const FName EnvironmentTag(TEXT("GP.TestEnvironment"));

	struct FCorruptionStationConfig
	{
		FName IdentityTag;
		FName StableName;
		FVector2D Offset;
		float Corruption = 0.0f;
		FLinearColor Color = FLinearColor::White;
	};

	struct FEventStationConfig
	{
		FName IdentityTag;
		FName StableName;
		FVector2D Offset;
		const TCHAR* ClassPath = nullptr;
		int32 RegionId = 0;
	};

	struct FTestFloorConfig
	{
		FName IdentityTag;
		FName StableName;
		FVector2D Offset;
		FVector2D Size;
	};

	FRotator MakeStationRotation(const FVector& StationLocation, const FVector& PlayerStartLocation)
	{
		// TextRender faces local -X, so point local -X back toward the central PlayerStart.
		const FVector AwayFromPlayer = (StationLocation - PlayerStartLocation).GetSafeNormal2D();
		return AwayFromPlayer.IsNearlyZero() ? FRotator::ZeroRotator : AwayFromPlayer.Rotation();
	}

	AActor* FindOrSpawnTaggedActor(UWorld* World, UClass* RequiredClass, const FName IdentityTag, const FName StableName)
	{
		if (!IsValid(World) || !IsValid(RequiredClass))
		{
			return nullptr;
		}

		AActor* Result = nullptr;
		TArray<AActor*> ActorsToRemove;
		for (AActor* Actor : World->PersistentLevel->Actors)
		{
			if (!IsValid(Actor) || !Actor->Tags.Contains(EnvironmentTag) || !Actor->Tags.Contains(IdentityTag))
			{
				continue;
			}

			if (!Result && Actor->IsA(RequiredClass))
			{
				Result = Actor;
			}
			else
			{
				ActorsToRemove.Add(Actor);
			}
		}

		for (AActor* Duplicate : ActorsToRemove)
		{
			// Only actors carrying both managed tags are removed, preserving all designer-owned map actors.
			World->EditorDestroyActor(Duplicate, true);
		}

		if (!Result)
		{
			FActorSpawnParameters SpawnParameters;
			SpawnParameters.OverrideLevel = World->PersistentLevel;
			SpawnParameters.Name = StableName;
			SpawnParameters.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;
			SpawnParameters.InitialActorLabel = StableName.ToString();
			SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Result = World->SpawnActor<AActor>(RequiredClass, FTransform::Identity, SpawnParameters);
		}

		if (Result)
		{
			Result->Modify();
			Result->Tags.AddUnique(EnvironmentTag);
			Result->Tags.AddUnique(IdentityTag);
			Result->SetActorLabel(StableName.ToString(), false);
			Result->MarkPackageDirty();
		}

		return Result;
	}

	bool ConfigureTestFloors(UWorld* World, const FVector& PlayerStartLocation, float TestFloorZ)
	{
		const FTestFloorConfig Configs[] =
		{
			{ TEXT("GP.Test.Floor.Corruption"), TEXT("TEST_Floor_Corruption"), FVector2D(0.0f, -1500.0f), FVector2D(5200.0f, 1800.0f) },
			{ TEXT("GP.Test.Floor.Connector"), TEXT("TEST_Floor_Connector"), FVector2D(0.0f, 450.0f), FVector2D(800.0f, 2100.0f) },
			{ TEXT("GP.Test.Floor.Events"), TEXT("TEST_Floor_Events"), FVector2D(0.0f, 3500.0f), FVector2D(20000.0f, 4000.0f) },
		};

		constexpr float FloorThickness = 20.0f;
		for (const FTestFloorConfig& Config : Configs)
		{
			AGP_LandscapeTestFloorActor* Floor = Cast<AGP_LandscapeTestFloorActor>(FindOrSpawnTaggedActor(
				World,
				AGP_LandscapeTestFloorActor::StaticClass(),
				Config.IdentityTag,
				Config.StableName));
			if (!Floor || !Floor->GetNavigationFloor())
			{
				return false;
			}

			Floor->SetActorLocation(PlayerStartLocation + FVector(Config.Offset.X, Config.Offset.Y, TestFloorZ - PlayerStartLocation.Z - FloorThickness * 0.5f));
			Floor->SetActorRotation(FRotator::ZeroRotator);
			Floor->SetActorScale3D(FVector::OneVector);
			Floor->ConfigureFloor(Config.Size, FloorThickness);
			Floor->MarkPackageDirty();
		}

		return true;
	}

	bool ConfigureCorruptionStations(
		UWorld* World,
		const FVector& PlayerStartLocation,
		float TestFloorZ,
		TArray<FVector>& OutStationLocations)
	{
		const FCorruptionStationConfig Configs[] =
		{
			{ TEXT("GP.Test.Corruption.Clean"), TEXT("TEST_Corruption_Clean"), FVector2D(-2000.0f, -1500.0f), 0.0f, FLinearColor(0.03f, 1.0f, 0.22f, 1.0f) },
			{ TEXT("GP.Test.Corruption.Mid"), TEXT("TEST_Corruption_Mid"), FVector2D(0.0f, -1500.0f), 50.0f, FLinearColor(1.0f, 0.42f, 0.03f, 1.0f) },
			{ TEXT("GP.Test.Corruption.High"), TEXT("TEST_Corruption_High"), FVector2D(2000.0f, -1500.0f), 100.0f, FLinearColor(0.78f, 0.03f, 1.0f, 1.0f) },
		};

		for (const FCorruptionStationConfig& Config : Configs)
		{
			AGP_CorruptionTestZoneActor* Station = Cast<AGP_CorruptionTestZoneActor>(FindOrSpawnTaggedActor(
				World,
				AGP_CorruptionTestZoneActor::StaticClass(),
				Config.IdentityTag,
				Config.StableName));
			if (!Station)
			{
				return false;
			}

			const FVector GroundLocation(
				PlayerStartLocation.X + Config.Offset.X,
				PlayerStartLocation.Y + Config.Offset.Y,
				TestFloorZ + 1.0f);
			Station->SetActorLocationAndRotation(
				GroundLocation,
				MakeStationRotation(GroundLocation, PlayerStartLocation),
				false,
				nullptr,
				ETeleportType::TeleportPhysics);
			Station->ConfigureWorldTestStation(Config.Corruption, Config.Color);
			Station->MarkPackageDirty();
			OutStationLocations.Add(GroundLocation);
		}

		return true;
	}

	bool ConfigureEventStations(
		UWorld* World,
		const FVector& PlayerStartLocation,
		float TestFloorZ,
		TArray<FVector>& OutStationLocations)
	{
		const FEventStationConfig Configs[] =
		{
			{ TEXT("GP.Test.Event.RedRift"), TEXT("TEST_Event_RedRift"), FVector2D(-7500.0f, 3500.0f), TEXT("/Game/RegionEvents/Examples/BP_RE_TestTrigger_RedRift.BP_RE_TestTrigger_RedRift_C"), 0 },
			{ TEXT("GP.Test.Event.Crystal"), TEXT("TEST_Event_CrystalCorruption"), FVector2D(-2500.0f, 3500.0f), TEXT("/Game/RegionEvents/Examples/BP_RE_TestTrigger_CrystalCorruption.BP_RE_TestTrigger_CrystalCorruption_C"), 1 },
			{ TEXT("GP.Test.Event.Shrine"), TEXT("TEST_Event_ShrineRuins"), FVector2D(2500.0f, 3500.0f), TEXT("/Game/RegionEvents/Examples/BP_RE_TestTrigger_ShrineRuins.BP_RE_TestTrigger_ShrineRuins_C"), 2 },
			{ TEXT("GP.Test.Event.Defense"), TEXT("TEST_Event_StructureDefense"), FVector2D(7500.0f, 3500.0f), TEXT("/Game/RegionEvents/Examples/BP_RE_TestTrigger_StructureDefense.BP_RE_TestTrigger_StructureDefense_C"), 3 },
		};

		for (const FEventStationConfig& Config : Configs)
		{
			UClass* TriggerClass = LoadClass<AGP_RegionEventTestTriggerActor>(nullptr, Config.ClassPath);
			if (!TriggerClass)
			{
				UE_LOG(LogTemp, Error, TEXT("[LandscapeTestEnvironment] Missing trigger Blueprint class: %s"), Config.ClassPath);
				return false;
			}

			AGP_RegionEventTestTriggerActor* Station = Cast<AGP_RegionEventTestTriggerActor>(FindOrSpawnTaggedActor(
				World,
				TriggerClass,
				Config.IdentityTag,
				Config.StableName));
			if (!Station)
			{
				return false;
			}

			const FVector GroundLocation(
				PlayerStartLocation.X + Config.Offset.X,
				PlayerStartLocation.Y + Config.Offset.Y,
				TestFloorZ + 1.0f);
			Station->SetActorLocationAndRotation(
				GroundLocation,
				MakeStationRotation(GroundLocation, PlayerStartLocation),
				false,
				nullptr,
				ETeleportType::TeleportPhysics);
			Station->ConfigureAsOverlapTestStation(Config.RegionId);
			Station->MarkPackageDirty();
			OutStationLocations.Add(GroundLocation);
		}

		return true;
	}

	bool ConfigureInstructionSign(UWorld* World, const FVector& PlayerStartLocation, float TestFloorZ)
	{
		const FName IdentityTag(TEXT("GP.Test.Instructions"));
		const FName StableName(TEXT("TEST_Instructions"));
		AGP_LandscapeTestInstructionActor* Sign = Cast<AGP_LandscapeTestInstructionActor>(FindOrSpawnTaggedActor(
			World,
			AGP_LandscapeTestInstructionActor::StaticClass(),
			IdentityTag,
			StableName));
		if (!Sign)
		{
			return false;
		}

		const FVector SignLocation(
			PlayerStartLocation.X,
			PlayerStartLocation.Y - 650.0f,
			TestFloorZ + 220.0f);
		Sign->SetActorLocationAndRotation(
			SignLocation,
			MakeStationRotation(SignLocation, PlayerStartLocation),
			false,
			nullptr,
			ETeleportType::TeleportPhysics);
		Sign->SetInstructionText(FText::FromString(TEXT("CORRUPTION: 0 / 50 / 100\nREGION EVENTS: WALK INTO ONE STATION")));
		Sign->MarkPackageDirty();
		return true;
	}

	ANavMeshBoundsVolume* ConfigureNavigationBounds(
		UWorld* World,
		const FVector& PlayerStartLocation,
		const TArray<FVector>& TestLocations)
	{
		const FName IdentityTag(TEXT("GP.Test.Navigation"));
		const FName StableName(TEXT("TEST_NavMeshBounds"));
		ANavMeshBoundsVolume* NavBounds = Cast<ANavMeshBoundsVolume>(FindOrSpawnTaggedActor(
			World,
			ANavMeshBoundsVolume::StaticClass(),
			IdentityTag,
			StableName));
		if (!NavBounds)
		{
			return nullptr;
		}

		FBox LayoutBounds(PlayerStartLocation, PlayerStartLocation);
		for (const FVector& Location : TestLocations)
		{
			LayoutBounds += Location;
		}

		const FVector LayoutCenter = LayoutBounds.GetCenter();
		const FVector LayoutSize = LayoutBounds.GetSize();
		const float NavSizeX = FMath::Max(20000.0f, LayoutSize.X + 5000.0f);
		const float NavSizeY = FMath::Max(12000.0f, LayoutSize.Y + 5000.0f);
		constexpr float NavSizeZ = 10000.0f;

		NavBounds->SetActorLocation(FVector(LayoutCenter.X, LayoutCenter.Y, PlayerStartLocation.Z));
		NavBounds->SetActorRotation(FRotator::ZeroRotator);
		NavBounds->SetActorScale3D(FVector::OneVector);

		UCubeBuilder* CubeBuilder = NewObject<UCubeBuilder>(GetTransientPackage(), NAME_None, RF_Transactional);
		CubeBuilder->X = NavSizeX;
		CubeBuilder->Y = NavSizeY;
		CubeBuilder->Z = NavSizeZ;
		CubeBuilder->Hollow = false;
		CubeBuilder->Tessellated = false;
		// Rebuilding the brush avoids relying on engine-default volume dimensions or accumulated actor scale.
		UActorFactory::CreateBrushForVolumeActor(NavBounds, CubeBuilder);
		NavBounds->MarkPackageDirty();
		return NavBounds;
	}

	bool BuildAndValidateNavigation(
		UWorld* World,
		ANavMeshBoundsVolume* NavBounds,
		const FVector& PlayerStartLocation,
		TArray<FVector>& EventLocations)
	{
		FNavigationSystem::AddNavigationSystemToWorld(*World, FNavigationSystemRunMode::EditorMode);
		UNavigationSystemV1* NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
		if (!NavigationSystem)
		{
			UE_LOG(LogTemp, Error, TEXT("[LandscapeTestEnvironment] Could not create the editor navigation system."));
			return false;
		}

		NavigationSystem->OnNavigationBoundsUpdated(NavBounds);
		// Bounds/component updates are queued; commandlets have no editor frame tick to flush them automatically.
		NavigationSystem->Tick(0.0f);
		for (AActor* Actor : World->PersistentLevel->Actors)
		{
			AGP_LandscapeTestFloorActor* TestFloor = Cast<AGP_LandscapeTestFloorActor>(Actor);
			if (IsValid(TestFloor)
				&& TestFloor->Tags.Contains(EnvironmentTag)
				&& TestFloor->GetNavigationFloor())
			{
				UBoxComponent* FloorComponent = TestFloor->GetNavigationFloor();
				if (FNavigationSystem::HasComponentData(*FloorComponent))
				{
					FNavigationSystem::UpdateComponentData(*FloorComponent);
				}
				else
				{
					// Components spawned before the commandlet creates its nav system need an explicit first registration.
					FNavigationSystem::RegisterComponent(*FloorComponent);
				}

				UE_LOG(LogTemp, Display, TEXT("[LandscapeTestEnvironment] Floor=%s Registered=%d NavRelevant=%d Bounds=%s"),
					*TestFloor->GetActorLabel(),
					FloorComponent->IsRegistered() ? 1 : 0,
					FloorComponent->IsNavigationRelevant() ? 1 : 0,
					*FloorComponent->Bounds.GetBox().ToString());
			}
		}
		NavigationSystem->Tick(0.0f);

		const FBox NavActorBounds = NavBounds->GetComponentsBoundingBox(true);
		const FBox RegisteredBounds = NavigationSystem->GetNavigableWorldBounds();
		UE_LOG(LogTemp, Display, TEXT("[LandscapeTestEnvironment] Nav actor bounds=%s registered bounds=%s"),
			*NavActorBounds.ToString(),
			*RegisteredBounds.ToString());
		// Building navigation does not require waiting for every unrelated project mesh and distance field to compile.
		FNavigationSystem::Build(*World);

		FNavLocation ProjectedPlayerStart;
		if (!NavigationSystem->ProjectPointToNavigation(
			PlayerStartLocation,
			ProjectedPlayerStart,
			FVector(800.0f, 800.0f, 1800.0f)))
		{
			UE_LOG(LogTemp, Error, TEXT("[LandscapeTestEnvironment] No walkable navigation below PlayerStart %s."),
				*PlayerStartLocation.ToCompactString());
			return false;
		}

		bool bAllLocationsReachNav = true;
		for (FVector& EventLocation : EventLocations)
		{
			FNavLocation ProjectedLocation;
			if (NavigationSystem->ProjectPointToNavigation(
				EventLocation,
				ProjectedLocation,
				FVector(800.0f, 800.0f, 1800.0f)))
			{
				EventLocation = ProjectedLocation.Location;
				const UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(
					World,
					ProjectedPlayerStart.Location,
					ProjectedLocation.Location);
				if (!IsValid(Path) || !Path->IsValid() || Path->IsPartial())
				{
					bAllLocationsReachNav = false;
					UE_LOG(LogTemp, Error, TEXT("[LandscapeTestEnvironment] PlayerStart has no complete path to event station %s."),
						*EventLocation.ToCompactString());
				}
			}
			else
			{
				bAllLocationsReachNav = false;
				UE_LOG(LogTemp, Error, TEXT("[LandscapeTestEnvironment] No walkable navigation near event station %s."),
					*EventLocation.ToCompactString());
			}
		}

		return bAllLocationsReachNav;
	}
}

#endif

bool UGP_LandscapeTestEnvironmentSetupLibrary::CreateOrUpdateLandscapeTestEnvironment()
{
#if WITH_EDITOR
	using namespace GPLandscapeTestEnvironment;

	UWorld* World = UEditorLoadingAndSavingUtils::LoadMap(LandscapeMapPath);
	if (!IsValid(World) || !IsValid(World->PersistentLevel))
	{
		UE_LOG(LogTemp, Error, TEXT("[LandscapeTestEnvironment] Failed to load %s."), LandscapeMapPath);
		return false;
	}

	APlayerStart* PlayerStart = nullptr;
	for (AActor* Actor : World->PersistentLevel->Actors)
	{
		if (APlayerStart* Candidate = Cast<APlayerStart>(Actor))
		{
			PlayerStart = Candidate;
			break;
		}
	}

	if (!PlayerStart)
	{
		UE_LOG(LogTemp, Error, TEXT("[LandscapeTestEnvironment] L_LandscapeMap requires its existing PlayerStart."));
		return false;
	}

	World->Modify();
	World->PersistentLevel->Modify();
	const float PlayerCapsuleHalfHeight = PlayerStart->GetCapsuleComponent()
		? PlayerStart->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()
		: 92.0f;
	const FVector OriginalPlayerStartLocation = PlayerStart->GetActorLocation();
	const FVector PlayerStartLocation(
		OriginalPlayerStartLocation.X,
		OriginalPlayerStartLocation.Y,
		TestDeckElevation + PlayerCapsuleHalfHeight);
	PlayerStart->Modify();
	PlayerStart->SetActorLocation(PlayerStartLocation);
	PlayerStart->MarkPackageDirty();
	const float TestFloorZ = TestDeckElevation;
	TArray<FVector> CorruptionLocations;
	TArray<FVector> EventLocations;
	// Report the exact failed stage so unattended map generation never exits without an actionable reason.
	if (!ConfigureTestFloors(World, PlayerStartLocation, TestFloorZ))
	{
		UE_LOG(LogTemp, Error, TEXT("[LandscapeTestEnvironment] Failed while configuring navigation floors."));
		return false;
	}
	if (!ConfigureCorruptionStations(World, PlayerStartLocation, TestFloorZ, CorruptionLocations))
	{
		UE_LOG(LogTemp, Error, TEXT("[LandscapeTestEnvironment] Failed while configuring corruption stations."));
		return false;
	}
	if (!ConfigureEventStations(World, PlayerStartLocation, TestFloorZ, EventLocations))
	{
		UE_LOG(LogTemp, Error, TEXT("[LandscapeTestEnvironment] Failed while configuring Region Event stations."));
		return false;
	}
	if (!ConfigureInstructionSign(World, PlayerStartLocation, TestFloorZ))
	{
		UE_LOG(LogTemp, Error, TEXT("[LandscapeTestEnvironment] Failed while configuring the instruction sign."));
		return false;
	}

	TArray<FVector> AllTestLocations = CorruptionLocations;
	AllTestLocations.Append(EventLocations);
	ANavMeshBoundsVolume* NavBounds = ConfigureNavigationBounds(World, PlayerStartLocation, AllTestLocations);
	if (!NavBounds || !BuildAndValidateNavigation(World, NavBounds, PlayerStartLocation, EventLocations))
	{
		return false;
	}

	World->PersistentLevel->MarkPackageDirty();
	if (!UEditorLoadingAndSavingUtils::SaveMap(World, LandscapeMapPath))
	{
		UE_LOG(LogTemp, Error, TEXT("[LandscapeTestEnvironment] Failed to save %s."), LandscapeMapPath);
		return false;
	}

	UE_LOG(LogTemp, Display, TEXT("[LandscapeTestEnvironment] Prepared 3 corruption stations, 4 Region Event stations, and navigation in %s."),
		LandscapeMapPath);
	return true;
#else
	return false;
#endif
}
