#include "Game/Regions/GP_RegionSpatialSubsystem.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogRegionSpatial, Log, All);

namespace
{
	const TCHAR* RegionSeedGeneratedClassPath =
		TEXT("/Game/RegionSystem/Blueprints/BP_RegionSeed.BP_RegionSeed_C");

	struct FRegionSeedCandidate
	{
		int32 RegionId = INDEX_NONE;
		FVector WorldLocation = FVector::ZeroVector;
		uint8 AuthoredState = 0;
		FString StableActorIdentifier;
		bool bReadFromProperty = false;
		bool bHasAuthoredState = false;
	};
}

bool UGP_RegionSpatialSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
	// Support runtime, PIE, and editor preview worlds while avoiding inactive thumbnail worlds.
	return WorldType == EWorldType::Game
		|| WorldType == EWorldType::PIE
		|| WorldType == EWorldType::GamePreview
		|| WorldType == EWorldType::Editor
		|| WorldType == EWorldType::EditorPreview;
}

void UGP_RegionSpatialSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	// Level actors are registered by this point, so one scan is enough for the normal non-streaming map path.
	RefreshRegionSeeds();
}

void UGP_RegionSpatialSubsystem::Deinitialize()
{
	CachedSeeds.Reset();
	CachedSeedLocations.Reset();
	CachedRegionCount = 0;
	bHasScannedWorld = false;
	Super::Deinitialize();
}

void UGP_RegionSpatialSubsystem::RefreshRegionSeeds()
{
	CachedSeeds.Reset();
	CachedSeedLocations.Reset();
	CachedRegionCount = 0;
	bHasScannedWorld = true;

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	TArray<FRegionSeedCandidate> Candidates;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* SeedActor = *It;
		if (!IsValid(SeedActor) || !IsRegionSeedActor(*SeedActor))
		{
			continue;
		}

		int32 RegionId = INDEX_NONE;
		bool bHadSeedIndexProperty = false;
		if (!TryResolveSeedIndex(*SeedActor, RegionId, bHadSeedIndexProperty))
		{
			UE_LOG(
				LogRegionSpatial,
				Warning,
				TEXT("Ignored region seed '%s': %s."),
				*SeedActor->GetPathName(),
				bHadSeedIndexProperty
					? TEXT("SeedIndex is negative or unsupported")
					: TEXT("SeedIndex and numeric label/name are unavailable"));
			continue;
		}

		FRegionSeedCandidate& Candidate = Candidates.AddDefaulted_GetRef();
		Candidate.RegionId = RegionId;
		Candidate.WorldLocation = SeedActor->GetActorLocation();
		Candidate.StableActorIdentifier = SeedActor->GetPathName();
		Candidate.bReadFromProperty = bHadSeedIndexProperty;

		bool bHadStateProperty = false;
		Candidate.bHasAuthoredState = TryResolveAuthoredState(*SeedActor, Candidate.AuthoredState, bHadStateProperty);
		if (bHadStateProperty && !Candidate.bHasAuthoredState)
		{
			// A schema change must fail closed instead of reinterpreting arbitrary Blueprint memory as a biome id.
			UE_LOG(
				LogRegionSpatial,
				Warning,
				TEXT("Region seed '%s' has an unsupported or out-of-range State property."),
				*SeedActor->GetPathName());
		}
	}

	// Property values beat legacy name fallbacks; actor paths then make duplicate handling deterministic.
	Candidates.Sort([](const FRegionSeedCandidate& Left, const FRegionSeedCandidate& Right)
	{
		if (Left.RegionId != Right.RegionId)
		{
			return Left.RegionId < Right.RegionId;
		}
		if (Left.bReadFromProperty != Right.bReadFromProperty)
		{
			return Left.bReadFromProperty;
		}
		return Left.StableActorIdentifier < Right.StableActorIdentifier;
	});

	for (const FRegionSeedCandidate& Candidate : Candidates)
	{
		if (CachedSeedLocations.Contains(Candidate.RegionId))
		{
			UE_LOG(
				LogRegionSpatial,
				Warning,
				TEXT("Ignored duplicate RegionId %d from '%s'."),
				Candidate.RegionId,
				*Candidate.StableActorIdentifier);
			continue;
		}

		FCachedRegionSeed& CachedSeed = CachedSeeds.AddDefaulted_GetRef();
		CachedSeed.RegionId = Candidate.RegionId;
		CachedSeed.WorldLocation = Candidate.WorldLocation;
		CachedSeed.AuthoredState = Candidate.AuthoredState;
		CachedSeed.bHasAuthoredState = Candidate.bHasAuthoredState;
		CachedSeedLocations.Add(Candidate.RegionId, Candidate.WorldLocation);
		CachedRegionCount = FMath::Max(CachedRegionCount, Candidate.RegionId + 1);
	}

	UE_LOG(
		LogRegionSpatial,
		Log,
		TEXT("Cached %d unique region seeds (addressable RegionCount=%d)."),
		CachedSeeds.Num(),
		CachedRegionCount);
}

int32 UGP_RegionSpatialSubsystem::ResolveRegionIdAtLocation(const FVector& WorldLocation)
{
	EnsureRegionSeedsCached();

	int32 NearestRegionId = INDEX_NONE;
	double NearestDistanceSquared = TNumericLimits<double>::Max();
	for (const FCachedRegionSeed& Seed : CachedSeeds)
	{
		const double DeltaX = static_cast<double>(WorldLocation.X) - static_cast<double>(Seed.WorldLocation.X);
		const double DeltaY = static_cast<double>(WorldLocation.Y) - static_cast<double>(Seed.WorldLocation.Y);
		const double DistanceSquared = DeltaX * DeltaX + DeltaY * DeltaY;
		if (DistanceSquared < NearestDistanceSquared)
		{
			NearestDistanceSquared = DistanceSquared;
			NearestRegionId = Seed.RegionId;
		}
	}

	// CachedSeeds is sorted by RegionId, so exact distance ties consistently keep the smaller id.
	return NearestRegionId;
}

int32 UGP_RegionSpatialSubsystem::GetRegionCount()
{
	EnsureRegionSeedsCached();
	return CachedRegionCount;
}

int32 UGP_RegionSpatialSubsystem::GetSeedCount()
{
	EnsureRegionSeedsCached();
	return CachedSeeds.Num();
}

bool UGP_RegionSpatialSubsystem::GetSeedLocation(int32 RegionId, FVector& OutWorldLocation)
{
	EnsureRegionSeedsCached();
	if (const FVector* Location = CachedSeedLocations.Find(RegionId))
	{
		OutWorldLocation = *Location;
		return true;
	}

	OutWorldLocation = FVector::ZeroVector;
	return false;
}

bool UGP_RegionSpatialSubsystem::GetAuthoredRegionState(int32 RegionId, uint8& OutState)
{
	EnsureRegionSeedsCached();
	for (const FCachedRegionSeed& Seed : CachedSeeds)
	{
		if (Seed.RegionId == RegionId && Seed.bHasAuthoredState)
		{
			OutState = Seed.AuthoredState;
			return true;
		}
	}

	OutState = 0;
	return false;
}

bool UGP_RegionSpatialSubsystem::BuildAuthoredRegionStates(uint8 FallbackState, TArray<uint8>& OutStates)
{
	EnsureRegionSeedsCached();
	OutStates.Init(FallbackState, CachedRegionCount);

	bool bFoundAuthoredState = false;
	for (const FCachedRegionSeed& Seed : CachedSeeds)
	{
		if (Seed.bHasAuthoredState && OutStates.IsValidIndex(Seed.RegionId))
		{
			// Sparse ids retain the legacy fallback while every valid seed keeps its serialized biome state.
			OutStates[Seed.RegionId] = Seed.AuthoredState;
			bFoundAuthoredState = true;
		}
	}

	if (!bFoundAuthoredState)
	{
		OutStates.Reset();
	}
	return bFoundAuthoredState;
}

void UGP_RegionSpatialSubsystem::EnsureRegionSeedsCached()
{
	if (!bHasScannedWorld)
	{
		// Lazy initialization also supports callers that query a transient world before BeginPlay.
		RefreshRegionSeeds();
	}
}

bool UGP_RegionSpatialSubsystem::IsRegionSeedActor(const AActor& Actor)
{
	// Walk the hierarchy so designer-authored children of BP_RegionSeed remain discoverable.
	for (const UClass* ActorClass = Actor.GetClass(); ActorClass != nullptr; ActorClass = ActorClass->GetSuperClass())
	{
		if (ActorClass->GetPathName().Equals(RegionSeedGeneratedClassPath, ESearchCase::CaseSensitive))
		{
			return true;
		}
	}

	return false;
}

bool UGP_RegionSpatialSubsystem::TryResolveSeedIndex(
	const AActor& Actor,
	int32& OutSeedIndex,
	bool& bOutHadSeedIndexProperty)
{
	OutSeedIndex = INDEX_NONE;
	bOutHadSeedIndexProperty = false;

	const FProperty* SeedIndexProperty = Actor.GetClass()->FindPropertyByName(TEXT("SeedIndex"));
	if (const FIntProperty* IntProperty = CastField<FIntProperty>(SeedIndexProperty))
	{
		bOutHadSeedIndexProperty = true;
		const int32 ReflectedSeedIndex = IntProperty->GetPropertyValue_InContainer(&Actor);
		if (ReflectedSeedIndex < 0 || ReflectedSeedIndex == MAX_int32)
		{
			return false;
		}

		OutSeedIndex = ReflectedSeedIndex;
		return true;
	}

	// A same-named property with a changed type is treated as invalid rather than reading unsafe memory.
	if (SeedIndexProperty != nullptr)
	{
		bOutHadSeedIndexProperty = true;
		return false;
	}

#if WITH_EDITOR
	// Actor labels preserve RegionSeed_14-style authoring names in editor and PIE worlds.
	if (TryParseTrailingRegionId(Actor.GetActorLabel(), OutSeedIndex))
	{
		return true;
	}
#endif

	// Packaged maps have no editor labels, so use the serialized actor object name as the final fallback.
	return TryParseTrailingRegionId(Actor.GetName(), OutSeedIndex);
}

bool UGP_RegionSpatialSubsystem::TryResolveAuthoredState(
	const AActor& Actor,
	uint8& OutState,
	bool& bOutHadStateProperty)
{
	OutState = 0;
	bOutHadStateProperty = false;

	const FProperty* StateProperty = Actor.GetClass()->FindPropertyByName(TEXT("State"));
	if (StateProperty == nullptr)
	{
		return false;
	}
	bOutHadStateProperty = true;

	// State is currently a Blueprint byte, so read that common contract directly before handling migrated numeric types.
	if (const FByteProperty* ByteProperty = CastField<FByteProperty>(StateProperty))
	{
		OutState = ByteProperty->GetPropertyValue_InContainer(&Actor);
		return true;
	}

	// Enum properties wrap an underlying integer property rather than deriving from FNumericProperty themselves.
	const FNumericProperty* NumericProperty = CastField<FNumericProperty>(StateProperty);
	if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(StateProperty))
	{
		NumericProperty = EnumProperty->GetUnderlyingProperty();
	}
	if (NumericProperty == nullptr || !NumericProperty->IsInteger())
	{
		return false;
	}

	// Use the outer property's address because an enum's underlying field has no actor-container offset of its own.
	const void* ValueAddress = StateProperty->ContainerPtrToValuePtr<void>(&Actor);
	const int64 Value = NumericProperty->GetSignedIntPropertyValue(ValueAddress);
	if (Value < 0 || Value > MAX_uint8)
	{
		return false;
	}
	OutState = static_cast<uint8>(Value);
	return true;
}

bool UGP_RegionSpatialSubsystem::TryParseTrailingRegionId(const FString& Identifier, int32& OutRegionId)
{
	OutRegionId = INDEX_NONE;
	if (Identifier.IsEmpty())
	{
		return false;
	}

	int32 DigitStart = Identifier.Len();
	while (DigitStart > 0 && FChar::IsDigit(Identifier[DigitStart - 1]))
	{
		--DigitStart;
	}

	if (DigitStart == Identifier.Len())
	{
		return false;
	}

	const FString NumericSuffix = Identifier.Mid(DigitStart);
	return LexTryParseString(OutRegionId, *NumericSuffix)
		&& OutRegionId >= 0
		&& OutRegionId < MAX_int32;
}
