#include "Utils/GP_LandscapeTestEnvironmentSetupLibrary.h"

#if WITH_EDITOR

#include "ActorFactories/ActorFactory.h"
#include "AI/NavigationSystemBase.h"
#include "Builders/CubeBuilder.h"
#include "Components/TextRenderComponent.h"
#include "Engine/Level.h"
#include "Engine/TextRenderActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "Game/Corruption/GP_CorruptionTestZoneActor.h"
#include "Game/RegionEvents/GP_RegionEventTestTriggerActor.h"
#include "GameFramework/PlayerStart.h"
#include "LandscapeHeightfieldCollisionComponent.h"
#include "LandscapeProxy.h"
#include "NavigationSystem.h"
#include "NavMesh/NavMeshBoundsVolume.h"

namespace GPLandscapeTestEnvironment
{
	constexpr TCHAR LandscapeMapPath[] = TEXT("/Game/Maps/MainMap/L_LandscapeMap");
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

	TOptional<float> FindLandscapeHeight(UWorld* World, const FVector2D& XY)
	{
		TOptional<float> HighestHeight;
		for (TActorIterator<ALandscapeProxy> It(World); It; ++It)
		{
			const FVector QueryLocation(XY.X, XY.Y, It->GetActorLocation().Z);
			TOptional<float> Height = It->GetHeightAtLocation(QueryLocation, EHeightfieldSource::Editor);
			if (!Height.IsSet())
			{
				Height = It->GetHeightAtLocation(QueryLocation, EHeightfieldSource::Complex);
			}

			if (Height.IsSet() && (!HighestHeight.IsSet() || Height.GetValue() > HighestHeight.GetValue()))
			{
				HighestHeight = Height;
			}
		}

		if (!HighestHeight.IsSet())
		{
			TArray<FHitResult> SurfaceHits;
			FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(LandscapeTestGround), true);
			const FVector TraceStart(XY.X, XY.Y, 100000.0f);
			const FVector TraceEnd(XY.X, XY.Y, -100000.0f);
			const FCollisionObjectQueryParams ObjectQuery(ECC_WorldStatic);
			if (World->LineTraceMultiByObjectType(SurfaceHits, TraceStart, TraceEnd, ObjectQuery, QueryParams))
			{
				for (const FHitResult& Hit : SurfaceHits)
				{
					if (Hit.ImpactNormal.Z > 0.55f
						&& (!HighestHeight.IsSet() || Hit.ImpactPoint.Z > HighestHeight.GetValue()))
					{
						// Roads and other walkable static surfaces are valid station floors when Landscape editor data is unavailable.
						HighestHeight = Hit.ImpactPoint.Z;
					}
				}
			}
		}

		return HighestHeight;
	}

	FVector FindBestGroundLocation(UWorld* World, const FVector& DesiredLocation)
	{
		constexpr float SearchStep = 200.0f;
		constexpr int32 SearchRadiusInSteps = 4;
		constexpr float FootprintRadius = 260.0f;
		float BestScore = TNumericLimits<float>::Max();
		TOptional<FVector> BestLocation;

		for (int32 XStep = -SearchRadiusInSteps; XStep <= SearchRadiusInSteps; ++XStep)
		{
			for (int32 YStep = -SearchRadiusInSteps; YStep <= SearchRadiusInSteps; ++YStep)
			{
				const FVector2D CandidateXY(
					DesiredLocation.X + static_cast<float>(XStep) * SearchStep,
					DesiredLocation.Y + static_cast<float>(YStep) * SearchStep);
				const FVector2D SampleOffsets[] =
				{
					FVector2D::ZeroVector,
					FVector2D(FootprintRadius, 0.0f),
					FVector2D(-FootprintRadius, 0.0f),
					FVector2D(0.0f, FootprintRadius),
					FVector2D(0.0f, -FootprintRadius),
				};

				float MinimumHeight = TNumericLimits<float>::Max();
				float MaximumHeight = TNumericLimits<float>::Lowest();
				bool bAllSamplesValid = true;
				for (const FVector2D& SampleOffset : SampleOffsets)
				{
					const TOptional<float> Height = FindLandscapeHeight(World, CandidateXY + SampleOffset);
					if (!Height.IsSet())
					{
						bAllSamplesValid = false;
						break;
					}

					MinimumHeight = FMath::Min(MinimumHeight, Height.GetValue());
					MaximumHeight = FMath::Max(MaximumHeight, Height.GetValue());
				}

				if (!bAllSamplesValid)
				{
					continue;
				}

				// Prefer nearby, flat terrain so wide trigger platforms do not visibly clip into sculpted slopes.
				const float DistanceScore = FVector2D::Distance(CandidateXY, FVector2D(DesiredLocation));
				const float RoughnessScore = (MaximumHeight - MinimumHeight) * 20.0f;
				const float CandidateScore = DistanceScore + RoughnessScore;
				if (CandidateScore < BestScore)
				{
					BestScore = CandidateScore;
					BestLocation = FVector(CandidateXY.X, CandidateXY.Y, (MinimumHeight + MaximumHeight) * 0.5f);
				}
			}
		}

		if (BestLocation.IsSet())
		{
			return BestLocation.GetValue();
		}

		UE_LOG(LogTemp, Warning, TEXT("[LandscapeTestEnvironment] No Landscape height near %s; using PlayerStart Z fallback."),
			*DesiredLocation.ToCompactString());
		return DesiredLocation;
	}

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

	bool ConfigureCorruptionStations(UWorld* World, const FVector& PlayerStartLocation, TArray<FVector>& OutStationLocations)
	{
		const FCorruptionStationConfig Configs[] =
		{
			{ TEXT("GP.Test.Corruption.Clean"), TEXT("TEST_Corruption_Clean"), FVector2D(-2000.0f, -1800.0f), 0.0f, FLinearColor(0.03f, 1.0f, 0.22f, 1.0f) },
			{ TEXT("GP.Test.Corruption.Mid"), TEXT("TEST_Corruption_Mid"), FVector2D(0.0f, -1800.0f), 50.0f, FLinearColor(1.0f, 0.42f, 0.03f, 1.0f) },
			{ TEXT("GP.Test.Corruption.High"), TEXT("TEST_Corruption_High"), FVector2D(2000.0f, -1800.0f), 100.0f, FLinearColor(0.78f, 0.03f, 1.0f, 1.0f) },
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

			const FVector DesiredLocation = PlayerStartLocation + FVector(Config.Offset.X, Config.Offset.Y, 0.0f);
			const FVector GroundLocation = FindBestGroundLocation(World, DesiredLocation);
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

	bool ConfigureEventStations(UWorld* World, const FVector& PlayerStartLocation, TArray<FVector>& OutStationLocations)
	{
		const FEventStationConfig Configs[] =
		{
			{ TEXT("GP.Test.Event.RedRift"), TEXT("TEST_Event_RedRift"), FVector2D(-7500.0f, 3000.0f), TEXT("/Game/RegionEvents/Examples/BP_RE_TestTrigger_RedRift.BP_RE_TestTrigger_RedRift_C"), 0 },
			{ TEXT("GP.Test.Event.Crystal"), TEXT("TEST_Event_CrystalCorruption"), FVector2D(-2500.0f, 3000.0f), TEXT("/Game/RegionEvents/Examples/BP_RE_TestTrigger_CrystalCorruption.BP_RE_TestTrigger_CrystalCorruption_C"), 1 },
			{ TEXT("GP.Test.Event.Shrine"), TEXT("TEST_Event_ShrineRuins"), FVector2D(2500.0f, 3000.0f), TEXT("/Game/RegionEvents/Examples/BP_RE_TestTrigger_ShrineRuins.BP_RE_TestTrigger_ShrineRuins_C"), 2 },
			{ TEXT("GP.Test.Event.Defense"), TEXT("TEST_Event_StructureDefense"), FVector2D(7500.0f, 3000.0f), TEXT("/Game/RegionEvents/Examples/BP_RE_TestTrigger_StructureDefense.BP_RE_TestTrigger_StructureDefense_C"), 3 },
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

			const FVector DesiredLocation = PlayerStartLocation + FVector(Config.Offset.X, Config.Offset.Y, 0.0f);
			const FVector GroundLocation = FindBestGroundLocation(World, DesiredLocation);
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

	bool ConfigureInstructionSign(UWorld* World, const FVector& PlayerStartLocation)
	{
		const FName IdentityTag(TEXT("GP.Test.Instructions"));
		const FName StableName(TEXT("TEST_Instructions"));
		ATextRenderActor* Sign = Cast<ATextRenderActor>(FindOrSpawnTaggedActor(
			World,
			ATextRenderActor::StaticClass(),
			IdentityTag,
			StableName));
		if (!Sign || !Sign->GetTextRender())
		{
			return false;
		}

		FVector SignLocation = FindBestGroundLocation(World, PlayerStartLocation + FVector(0.0f, -700.0f, 0.0f));
		SignLocation.Z += 220.0f;
		Sign->SetActorLocationAndRotation(
			SignLocation,
			MakeStationRotation(SignLocation, PlayerStartLocation),
			false,
			nullptr,
			ETeleportType::TeleportPhysics);
		Sign->GetTextRender()->SetText(FText::FromString(TEXT("CORRUPTION: 0 / 50 / 100\nREGION EVENTS: WALK INTO ONE STATION")));
		Sign->GetTextRender()->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
		Sign->GetTextRender()->SetWorldSize(55.0f);
		Sign->GetTextRender()->SetTextRenderColor(FColor::White);
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

	bool BuildAndValidateNavigation(UWorld* World, ANavMeshBoundsVolume* NavBounds, TArray<FVector>& EventLocations)
	{
		FNavigationSystem::AddNavigationSystemToWorld(*World, FNavigationSystemRunMode::EditorMode);
		UNavigationSystemV1* NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
		if (!NavigationSystem)
		{
			UE_LOG(LogTemp, Error, TEXT("[LandscapeTestEnvironment] Could not create the editor navigation system."));
			return false;
		}

		NavigationSystem->OnNavigationBoundsUpdated(NavBounds);
		// Building navigation does not require waiting for every unrelated project mesh and distance field to compile.
		FNavigationSystem::Build(*World);

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
	const FVector PlayerStartLocation = PlayerStart->GetActorLocation();
	TArray<FVector> CorruptionLocations;
	TArray<FVector> EventLocations;
	if (!ConfigureCorruptionStations(World, PlayerStartLocation, CorruptionLocations)
		|| !ConfigureEventStations(World, PlayerStartLocation, EventLocations)
		|| !ConfigureInstructionSign(World, PlayerStartLocation))
	{
		return false;
	}

	TArray<FVector> AllTestLocations = CorruptionLocations;
	AllTestLocations.Append(EventLocations);
	ANavMeshBoundsVolume* NavBounds = ConfigureNavigationBounds(World, PlayerStartLocation, AllTestLocations);
	if (!NavBounds || !BuildAndValidateNavigation(World, NavBounds, EventLocations))
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
