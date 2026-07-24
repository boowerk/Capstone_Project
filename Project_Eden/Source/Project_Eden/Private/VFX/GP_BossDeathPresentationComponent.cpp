#include "VFX/GP_BossDeathPresentationComponent.h"

#include "Characters/GP_EnemyCharacter.h"
#include "Engine/World.h"

UGP_BossDeathPresentationComponent::UGP_BossDeathPresentationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
	PresentationActorClass = AGP_BossDeathPresentationActor::StaticClass();
	// The shared absorption component owns the source-mesh material transition
	// and hide timing. This component owns only boss-specific shards/particles.
	SpawnSettings.bHideSourceMesh = false;
}

void UGP_BossDeathPresentationComponent::ConfigureFragmentMaterial(
	UMaterialInterface* InFragmentMaterial,
	bool bInHideSourceMesh)
{
	SpawnSettings.FragmentMaterial = InFragmentMaterial;
	SpawnSettings.bHideSourceMesh = bInHideSourceMesh;
}

bool UGP_BossDeathPresentationComponent::PlayDeathPresentation(AActor* InstigatorActor)
{
	if (bPresentationPlayed || !bEnableDeathPresentation || GetNetMode() == NM_DedicatedServer)
	{
		return false;
	}

	AGP_EnemyCharacter* EnemyOwner = Cast<AGP_EnemyCharacter>(GetOwner());
	if (!CanPlayForOwner(EnemyOwner))
	{
		return false;
	}

	const EGPBossDeathPresentationStyle ResolvedStyle = ResolvePresentationStyle();
	if (ResolvedStyle == EGPBossDeathPresentationStyle::None || ResolvedStyle == EGPBossDeathPresentationStyle::Auto)
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	TSubclassOf<AGP_BossDeathPresentationActor> ActorClass = PresentationActorClass;
	if (!*ActorClass)
	{
		ActorClass = AGP_BossDeathPresentationActor::StaticClass();
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = EnemyOwner;
	SpawnParameters.Instigator = EnemyOwner;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AGP_BossDeathPresentationActor* PresentationActor = World->SpawnActor<AGP_BossDeathPresentationActor>(
		ActorClass,
		EnemyOwner->GetActorLocation(),
		EnemyOwner->GetActorRotation(),
		SpawnParameters);
	if (!IsValid(PresentationActor))
	{
		return false;
	}

	// Mark before initialization so Blueprint callbacks cannot recursively request another clear effect.
	bPresentationPlayed = true;
	PresentationActor->InitializePresentation(ResolvedStyle, EnemyOwner, InstigatorActor, SpawnSettings);
	return true;
}

EGPBossDeathPresentationStyle UGP_BossDeathPresentationComponent::ResolvePresentationStyle() const
{
	if (PresentationStyle != EGPBossDeathPresentationStyle::Auto)
	{
		return PresentationStyle;
	}

	const AGP_EnemyCharacter* EnemyOwner = Cast<AGP_EnemyCharacter>(GetOwner());
	if (!IsValid(EnemyOwner))
	{
		return EGPBossDeathPresentationStyle::None;
	}

	return ResolveAutoPresentationStyleFromName(EnemyOwner->GetName(), EnemyOwner->GetBossDisplayName());
}

EGPBossDeathPresentationStyle UGP_BossDeathPresentationComponent::ResolveAutoPresentationStyleFromName(
	const FString& OwnerName,
	const FText& BossDisplayName)
{
	const FString CombinedName = FString::Printf(TEXT("%s %s"), *OwnerName, *BossDisplayName.ToString()).ToLower();
	if (CombinedName.Contains(TEXT("crystal")) || CombinedName.Contains(TEXT("seraph")))
	{
		return EGPBossDeathPresentationStyle::CrystalSeraph;
	}

	if (CombinedName.Contains(TEXT("sans")))
	{
		return EGPBossDeathPresentationStyle::Sans;
	}

	if (CombinedName.Contains(TEXT("dark")) || CombinedName.Contains(TEXT("knight")) || CombinedName.Contains(TEXT("armor")))
	{
		return EGPBossDeathPresentationStyle::DarkArmorKnight;
	}

	if (CombinedName.Contains(TEXT("matador")))
	{
		return EGPBossDeathPresentationStyle::Matador;
	}

	return EGPBossDeathPresentationStyle::None;
}

bool UGP_BossDeathPresentationComponent::CanPlayForOwner(const AGP_EnemyCharacter* EnemyOwner) const
{
	// Regular enemies keep their lightweight death flow; only boss pawns receive the expensive clear presentation.
	return IsValid(EnemyOwner) && EnemyOwner->IsBossEnemy();
}
