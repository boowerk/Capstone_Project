#include "VFX/GP_EnemyDeathAbsorptionComponent.h"

#include "AbilitySystem/GP_AttributeSet.h"
#include "Characters/GP_EnemyCharacter.h"
#include "Characters/GP_PlayerCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

UGP_EnemyDeathAbsorptionComponent::UGP_EnemyDeathAbsorptionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(true);

	// A soft constructor default lets a missing optional cosmetic remain a quiet no-op instead of a CDO load error.
	DeathAbsorptionSystem = TSoftObjectPtr<UNiagaraSystem>(
		FSoftObjectPath(TEXT("/Game/Niagara/Dissolve_SK/NS_EnemyDeath_Absorb.NS_EnemyDeath_Absorb")));
}

UNiagaraSystem* UGP_EnemyDeathAbsorptionComponent::GetDeathAbsorptionSystem() const
{
	return DeathAbsorptionSystem.Get();
}

void UGP_EnemyDeathAbsorptionComponent::BeginPlay()
{
	Super::BeginPlay();

	const UWorld* World = GetWorld();
	if (IsValid(World) && World->GetNetMode() != NM_DedicatedServer)
	{
		// Warm the soft reference before combat so the first defeated enemy does not hitch while loading Niagara.
		DeathAbsorptionSystem.LoadSynchronous();
	}
}

void UGP_EnemyDeathAbsorptionComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bLocalPlaybackActive || !IsValid(ActiveNiagaraComponent))
	{
		SetComponentTickEnabled(false);
		return;
	}

	PlaybackElapsedSeconds += FMath::Max(0.0f, DeltaTime);
	++PlaybackTickCount;
	UpdateTargetAndBounds();
	UpdateAbsorbStrength();
	UpdateFallGravity();
	HideSourceMeshWhenReady();

	if (!bEmissionStopped
		&& PlaybackElapsedSeconds >= FMath::Max(0.01f, EmissionStopTimeSeconds)
		&& PlaybackTickCount >= FMath::Max(1, MinimumEmissionFrames))
	{
		// The frame gate survives a first-frame hitch; Deactivate then stops spawning while existing particles finish.
		ActiveNiagaraComponent->Deactivate();
		bEmissionStopped = true;
	}

	if (PlaybackElapsedSeconds >= FMath::Max(0.1f, EffectDeactivateTimeSeconds))
	{
		// Hard-stop any stragglers before the owning enemy reaches its two-second despawn boundary.
		StopLocalPlayback();
	}
}

void UGP_EnemyDeathAbsorptionComponent::PlayDeathAbsorption(AActor* DeathInstigatorActor)
{
	AGP_EnemyCharacter* EnemyOwner = Cast<AGP_EnemyCharacter>(GetOwner());
	if (!IsValid(EnemyOwner)
		|| !EnemyOwner->HasAuthority()
		|| EnemyOwner->IsBossEnemy()
		|| bAuthorityPlaybackRequested)
	{
		return;
	}

	bAuthorityPlaybackRequested = true;

	// Resolve once on authority; client-side nearest-player queries could otherwise disagree in a three-player session.
	AActor* TargetPlayerActor = ResolveAuthorityTarget(DeathInstigatorActor);
	MulticastPlayDeathAbsorption(TargetPlayerActor);
}

void UGP_EnemyDeathAbsorptionComponent::MulticastPlayDeathAbsorption_Implementation(AActor* TargetPlayerActor)
{
	PlayLocal(TargetPlayerActor);
}

void UGP_EnemyDeathAbsorptionComponent::PlayLocal(AActor* TargetPlayerActor)
{
	AGP_EnemyCharacter* EnemyOwner = Cast<AGP_EnemyCharacter>(GetOwner());
	UWorld* World = GetWorld();
	if (!IsValid(EnemyOwner)
		|| EnemyOwner->IsBossEnemy()
		|| !IsValid(World)
		|| World->GetNetMode() == NM_DedicatedServer
		|| bLocalPlaybackActive)
	{
		return;
	}

	UNiagaraSystem* ResolvedAbsorptionSystem = DeathAbsorptionSystem.LoadSynchronous();
	if (!IsValid(ResolvedAbsorptionSystem))
	{
		// Do not change visibility when a designer removes or renames the optional effect.
		return;
	}

	USkeletalMeshComponent* EnemyMesh = EnemyOwner->GetMesh();
	USceneComponent* AttachRoot = EnemyOwner->GetRootComponent();
	if (!IsValid(EnemyMesh)
		|| !IsValid(EnemyMesh->GetSkeletalMeshAsset())
		|| !IsValid(AttachRoot))
	{
		// The mesh must remain visible if the source DI cannot be initialized.
		return;
	}

	UNiagaraComponent* SpawnedComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
		ResolvedAbsorptionSystem,
		AttachRoot,
		NAME_None,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		EAttachLocation::KeepRelativeOffset,
		false,
		false,
		ENCPoolMethod::None,
		false);
	if (!IsValid(SpawnedComponent))
	{
		// Spawn failure is cosmetic only and must never erase the source mesh.
		return;
	}

	ActiveNiagaraComponent = SpawnedComponent;
	SourceMeshComponent = EnemyMesh;
	ActiveTargetActor = TargetPlayerActor;
	CachedSourceWorldBounds = EnemyMesh->Bounds.GetBox();
	LastTargetPosition = IsValid(TargetPlayerActor)
		? ResolveTargetPosition(TargetPlayerActor)
		: CachedSourceWorldBounds.GetCenter();
	PlaybackElapsedSeconds = 0.0f;
	PlaybackTickCount = 0;
	bEmissionStopped = false;
	bSourceMeshHiddenByComponent = false;

	// Bind the actual dead enemy rather than the sample Manny mesh stored in the original dissolve system.
	UNiagaraFunctionLibrary::OverrideSystemUserVariableSkeletalMeshComponent(
		ActiveNiagaraComponent,
		SourceMeshParameterName.ToString(),
		SourceMeshComponent);
	ActiveNiagaraComponent->SetVariablePosition(AbsorbTargetPositionParameterName, LastTargetPosition);
	ActiveNiagaraComponent->SetVariableFloat(AbsorbStrengthParameterName, 0.0f);
	ActiveNiagaraComponent->SetVariableFloat(AbsorbKillRadiusParameterName, FMath::Max(0.0f, AbsorbKillRadius));
	ActiveNiagaraComponent->SetVariableVec3(FallGravityParameterName, FallGravity);
	ActiveNiagaraComponent->SetVariableFloat(AbsorbDragParameterName, FMath::Max(0.0f, AbsorbDrag));
	ActiveNiagaraComponent->SetCustomTimeDilation(FMath::Max(0.1f, NiagaraPlaybackRate));
	UpdateCorridorFixedBounds();
	ActiveNiagaraComponent->Activate(true);

	bLocalPlaybackActive = true;
	SetComponentTickEnabled(true);
}

void UGP_EnemyDeathAbsorptionComponent::StopLocalPlayback()
{
	bLocalPlaybackActive = false;
	SetComponentTickEnabled(false);

	if (IsValid(ActiveNiagaraComponent))
	{
		// The Niagara component is transient and independent of gameplay state, so remove it explicitly on teardown.
		ActiveNiagaraComponent->DeactivateImmediate();
		ActiveNiagaraComponent->DestroyComponent();
	}

	ActiveNiagaraComponent = nullptr;
	SourceMeshComponent = nullptr;
	ActiveTargetActor.Reset();
	PlaybackTickCount = 0;
	bEmissionStopped = false;
}

void UGP_EnemyDeathAbsorptionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopLocalPlayback();
	Super::EndPlay(EndPlayReason);
}

AActor* UGP_EnemyDeathAbsorptionComponent::ResolveAuthorityTarget(AActor* DeathInstigatorActor) const
{
	AGP_PlayerCharacter* PreferredPlayer = ResolvePlayerCharacter(DeathInstigatorActor);
	TArray<AActor*> FallbackPlayers;

	const UWorld* World = GetWorld();
	const AGameStateBase* GameState = IsValid(World) ? World->GetGameState() : nullptr;
	if (IsValid(GameState))
	{
		FallbackPlayers.Reserve(GameState->PlayerArray.Num());
		for (APlayerState* PlayerState : GameState->PlayerArray)
		{
			AGP_PlayerCharacter* PlayerCharacter = IsValid(PlayerState)
				? Cast<AGP_PlayerCharacter>(PlayerState->GetPawn())
				: nullptr;
			if (IsUsablePlayerCharacter(PlayerCharacter))
			{
				FallbackPlayers.AddUnique(PlayerCharacter);
			}
		}
	}

	const AActor* OwnerActor = GetOwner();
	const FVector SourceLocation = IsValid(OwnerActor)
		? OwnerActor->GetActorLocation()
		: FVector::ZeroVector;
	return SelectPreferredOrNearestTarget(PreferredPlayer, SourceLocation, FallbackPlayers);
}

AGP_PlayerCharacter* UGP_EnemyDeathAbsorptionComponent::ResolvePlayerCharacter(AActor* CandidateActor) const
{
	if (!IsValid(CandidateActor))
	{
		return nullptr;
	}

	// Damage can report the player, their controller/state, a projectile instigator, or a player-owned weapon.
	TArray<AActor*, TInlineAllocator<6>> CandidateChain;
	CandidateChain.Add(CandidateActor);
	CandidateChain.Add(CandidateActor->GetInstigator());
	CandidateChain.Add(CandidateActor->GetInstigatorController());
	CandidateChain.Add(CandidateActor->GetOwner());

	if (AController* CandidateController = Cast<AController>(CandidateActor))
	{
		CandidateChain.Add(CandidateController->GetPawn());
	}
	if (APlayerState* CandidatePlayerState = Cast<APlayerState>(CandidateActor))
	{
		CandidateChain.Add(CandidatePlayerState->GetPawn());
	}

	for (AActor* ChainActor : CandidateChain)
	{
		if (AGP_PlayerCharacter* PlayerCharacter = Cast<AGP_PlayerCharacter>(ChainActor);
			IsUsablePlayerCharacter(PlayerCharacter))
		{
			return PlayerCharacter;
		}
	}

	return nullptr;
}

bool UGP_EnemyDeathAbsorptionComponent::IsUsablePlayerCharacter(
	const AGP_PlayerCharacter* PlayerCharacter) const
{
	if (!IsValid(PlayerCharacter) || PlayerCharacter->IsActorBeingDestroyed())
	{
		return false;
	}

	const UGP_AttributeSet* PlayerAttributes =
		Cast<UGP_AttributeSet>(PlayerCharacter->GetAttributeSet());
	if (IsValid(PlayerAttributes) && PlayerAttributes->GetHealth() <= KINDA_SMALL_NUMBER)
	{
		// A defeated teammate must not pull the corpse away from the living three-player squad.
		return false;
	}

	// A disconnected pawn is not a safe visual destination; fall back to another member of PlayerArray.
	const AController* PlayerController = PlayerCharacter->GetController();
	return IsValid(PlayerController) && PlayerController->IsPlayerController();
}

AActor* UGP_EnemyDeathAbsorptionComponent::SelectPreferredOrNearestTarget(
	AActor* PreferredTarget,
	const FVector& SourceLocation,
	const TArray<AActor*>& ValidFallbackTargets)
{
	if (IsValid(PreferredTarget))
	{
		return PreferredTarget;
	}

	AActor* BestTarget = nullptr;
	double BestDistanceSquared = TNumericLimits<double>::Max();
	FString BestStableName;
	for (AActor* Candidate : ValidFallbackTargets)
	{
		if (!IsValid(Candidate))
		{
			continue;
		}

		const double CandidateDistanceSquared = FVector::DistSquared(SourceLocation, Candidate->GetActorLocation());
		const FString CandidateStableName = Candidate->GetPathName();
		const bool bCloser = CandidateDistanceSquared < BestDistanceSquared - UE_DOUBLE_SMALL_NUMBER;
		const bool bStableTieBreak = FMath::IsNearlyEqual(CandidateDistanceSquared, BestDistanceSquared)
			&& (BestTarget == nullptr || CandidateStableName < BestStableName);
		if (bCloser || bStableTieBreak)
		{
			BestTarget = Candidate;
			BestDistanceSquared = CandidateDistanceSquared;
			BestStableName = CandidateStableName;
		}
	}

	return BestTarget;
}

float UGP_EnemyDeathAbsorptionComponent::CalculateAbsorbStrength(
	float ElapsedSeconds,
	float InScatterDelaySeconds,
	float RampDurationSeconds,
	float MaximumStrength)
{
	const float SafeMaximumStrength = FMath::Max(0.0f, MaximumStrength);
	const float SafeDelaySeconds = FMath::Max(0.0f, InScatterDelaySeconds);
	if (ElapsedSeconds <= SafeDelaySeconds || SafeMaximumStrength <= KINDA_SMALL_NUMBER)
	{
		return 0.0f;
	}

	const float SafeRampSeconds = FMath::Max(KINDA_SMALL_NUMBER, RampDurationSeconds);
	const float LinearAlpha = FMath::Clamp(
		(ElapsedSeconds - SafeDelaySeconds) / SafeRampSeconds,
		0.0f,
		1.0f);
	const float SmoothAlpha = LinearAlpha * LinearAlpha * (3.0f - (2.0f * LinearAlpha));
	return SafeMaximumStrength * SmoothAlpha;
}

float UGP_EnemyDeathAbsorptionComponent::CalculateFallGravityScale(
	float ElapsedSeconds,
	float InFullGravityDurationSeconds,
	float InGravityFadeEndSeconds)
{
	const float SafeFullDuration = FMath::Max(0.0f, InFullGravityDurationSeconds);
	const float SafeFadeEnd = FMath::Max(SafeFullDuration, InGravityFadeEndSeconds);
	if (ElapsedSeconds <= SafeFullDuration)
	{
		return 1.0f;
	}
	if (ElapsedSeconds >= SafeFadeEnd || SafeFadeEnd - SafeFullDuration <= KINDA_SMALL_NUMBER)
	{
		return 0.0f;
	}

	const float LinearAlpha = FMath::Clamp(
		(ElapsedSeconds - SafeFullDuration) / (SafeFadeEnd - SafeFullDuration),
		0.0f,
		1.0f);
	const float SmoothAlpha = LinearAlpha * LinearAlpha * (3.0f - (2.0f * LinearAlpha));
	return 1.0f - SmoothAlpha;
}

FVector UGP_EnemyDeathAbsorptionComponent::ResolveTargetPosition(const AActor* TargetActor) const
{
	if (!IsValid(TargetActor))
	{
		return LastTargetPosition;
	}

	if (const AGP_PlayerCharacter* PlayerCharacter = Cast<AGP_PlayerCharacter>(TargetActor))
	{
		const USkeletalMeshComponent* PlayerMesh = PlayerCharacter->GetMesh();
		if (IsValid(PlayerMesh) && PlayerMesh->DoesSocketExist(TargetBodySocketName))
		{
			// spine_03 follows the chest during locomotion and gives the particles an intentional absorption point.
			return PlayerMesh->GetSocketLocation(TargetBodySocketName) + TargetBodyOffset;
		}
	}

	const FBox TargetBounds = TargetActor->GetComponentsBoundingBox(true);
	return (TargetBounds.IsValid ? TargetBounds.GetCenter() : TargetActor->GetActorLocation())
		+ TargetBodyOffset;
}

void UGP_EnemyDeathAbsorptionComponent::UpdateTargetAndBounds()
{
	if (ActiveTargetActor.IsValid())
	{
		// Keep following the server-selected player only; never retarget locally if that actor disappears.
		LastTargetPosition = ResolveTargetPosition(ActiveTargetActor.Get());
	}

	ActiveNiagaraComponent->SetVariablePosition(AbsorbTargetPositionParameterName, LastTargetPosition);
	UpdateCorridorFixedBounds();
}

void UGP_EnemyDeathAbsorptionComponent::UpdateAbsorbStrength()
{
	const float CurrentStrength = CalculateAbsorbStrength(
		PlaybackElapsedSeconds,
		ScatterDelaySeconds,
		AbsorbStrengthRampSeconds,
		MaximumAbsorbStrength);
	ActiveNiagaraComponent->SetVariableFloat(AbsorbStrengthParameterName, CurrentStrength);
}

void UGP_EnemyDeathAbsorptionComponent::UpdateFallGravity()
{
	const float GravityScale = CalculateFallGravityScale(
		PlaybackElapsedSeconds,
		FullGravityDurationSeconds,
		GravityFadeEndSeconds);

	// Fade gravity while attraction ramps so the grains arc upward instead of snapping directly to the chest.
	ActiveNiagaraComponent->SetVariableVec3(FallGravityParameterName, FallGravity * GravityScale);
}

void UGP_EnemyDeathAbsorptionComponent::HideSourceMeshWhenReady()
{
	if (bSourceMeshHiddenByComponent
		|| PlaybackElapsedSeconds < FMath::Max(0.0f, SourceMeshHideDelaySeconds)
		|| !IsValid(SourceMeshComponent)
		|| !IsValid(ActiveNiagaraComponent))
	{
		return;
	}

	// Hide only after Niagara exists and sampled the source; asset/spawn failures therefore preserve the ordinary corpse.
	SourceMeshComponent->SetHiddenInGame(true, true);
	bSourceMeshHiddenByComponent = true;
}

void UGP_EnemyDeathAbsorptionComponent::UpdateCorridorFixedBounds()
{
	if (!IsValid(ActiveNiagaraComponent) || !CachedSourceWorldBounds.IsValid)
	{
		return;
	}

	FBox WorldCorridorBounds = CachedSourceWorldBounds;
	WorldCorridorBounds += LastTargetPosition;
	WorldCorridorBounds = WorldCorridorBounds.ExpandBy(FMath::Max(0.0f, BoundsMargin));

	// Niagara expects the fixed override in component-local space even though the attraction Position is world-space.
	const FTransform WorldToNiagara = ActiveNiagaraComponent->GetComponentTransform().Inverse();
	ActiveNiagaraComponent->SetSystemFixedBounds(WorldCorridorBounds.TransformBy(WorldToNiagara));

	const float CorridorLength = FVector::Distance(CachedSourceWorldBounds.GetCenter(), LastTargetPosition);
	ActiveNiagaraComponent->SetVariableFloat(
		AbsorbRadiusParameterName,
		FMath::Max(MinimumAbsorbRadius, CorridorLength + FMath::Max(0.0f, BoundsMargin)));
}
