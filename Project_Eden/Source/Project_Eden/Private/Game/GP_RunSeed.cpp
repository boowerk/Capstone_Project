#include "Game/GP_RunSeed.h"

#include "Kismet/GameplayStatics.h"

namespace
{
	constexpr TCHAR RunSeedOptionName[] = TEXT("RunSeed");
}

int32 GPRunSeed::Generate()
{
	return static_cast<int32>(FGuid::NewGuid().A & static_cast<uint32>(MAX_int32));
}

bool GPRunSeed::TryParse(const FString& Options, int32& OutRunSeed)
{
	if (!UGameplayStatics::HasOption(Options, RunSeedOptionName))
	{
		return false;
	}

	const FString SeedText = UGameplayStatics::ParseOption(Options, RunSeedOptionName);
	if (SeedText.IsEmpty())
	{
		return false;
	}

	for (const TCHAR Character : SeedText)
	{
		if (!FChar::IsDigit(Character))
		{
			return false;
		}
	}

	const uint64 ParsedSeed = FCString::Strtoui64(*SeedText, nullptr, 10);
	if (ParsedSeed > static_cast<uint64>(MAX_int32))
	{
		return false;
	}

	OutRunSeed = static_cast<int32>(ParsedSeed);
	return true;
}

FString GPRunSeed::BuildTravelURL(const FString& GameMapName, int32 RunSeed)
{
	check(RunSeed >= 0);
	return FString::Printf(TEXT("/Game/Maps/%s?listen?RunSeed=%d"), *GameMapName, RunSeed);
}
