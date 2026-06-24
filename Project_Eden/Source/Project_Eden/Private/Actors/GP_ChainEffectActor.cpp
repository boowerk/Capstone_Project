#include "Actors/GP_ChainEffectActor.h"

#include "Characters/GP_MatadorBossStateComponent.h"
#include "Components/SceneComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

AGP_ChainEffectActor::AGP_ChainEffectActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.05f;
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	StageColors =
	{
		FLinearColor(0.15f, 0.6f, 1.0f, 1.0f),
		FLinearColor(1.0f, 0.85f, 0.1f, 1.0f),
		FLinearColor(1.0f, 0.35f, 0.05f, 1.0f),
		FLinearColor(1.0f, 0.05f, 0.02f, 1.0f)
	};
}

void AGP_ChainEffectActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (HasAuthority() && IsValid(MatadorStateComponent.Get()))
	{
		SetChainStage(MatadorStateComponent->GetChainBreakCount());
	}

	if (bShowDebugVisuals)
	{
		DrawChainPreview();
	}
}

void AGP_ChainEffectActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGP_ChainEffectActor, MainBossActor);
	DOREPLIFETIME(AGP_ChainEffectActor, DecoyActor);
	DOREPLIFETIME(AGP_ChainEffectActor, MatadorStateComponent);
	DOREPLIFETIME(AGP_ChainEffectActor, ChainStage);
}

void AGP_ChainEffectActor::InitializeChain(AActor* InMainBossActor, AActor* InDecoyActor, UGP_MatadorBossStateComponent* InStateComponent)
{
	if (!HasAuthority())
	{
		return;
	}

	MainBossActor = InMainBossActor;
	DecoyActor = InDecoyActor;
	MatadorStateComponent = InStateComponent;
	if (IsValid(MatadorStateComponent.Get()))
	{
		MatadorStateComponent->RegisterChainEffectActor(this);
		SetChainStage(MatadorStateComponent->GetChainBreakCount());
	}
}

void AGP_ChainEffectActor::SetChainStage(int32 NewStage)
{
	const int32 ClampedStage = FMath::Clamp(NewStage, 0, 3);
	if (ChainStage == ClampedStage)
	{
		return;
	}

	// Stage mirrors ChainBreakCount: 0 normal, 1 cracked, 2 damaged, 3 broken/groggy.
	ChainStage = ClampedStage;
	BP_OnChainStageChanged(ChainStage);
}

void AGP_ChainEffectActor::DrawChainPreview() const
{
	UWorld* World = GetWorld();
	if (!World || !IsValid(MainBossActor) || !IsValid(DecoyActor))
	{
		return;
	}

	const FVector Start = MainBossActor->GetActorLocation() + FVector(0.0f, 0.0f, 110.0f);
	const FVector End = DecoyActor->GetActorLocation() + FVector(0.0f, 0.0f, 110.0f);
	const FColor ChainColor = ResolveStageColor().ToFColor(true);
	const float Thickness = ResolveStageThickness();
	constexpr float LifeSeconds = 0.08f;

	DrawDebugLine(World, Start, End, ChainColor, false, LifeSeconds, 0, Thickness);
	DrawDebugSphere(World, Start, 18.0f + ChainStage * 3.0f, 8, ChainColor, false, LifeSeconds, 0, Thickness * 0.5f);
	DrawDebugSphere(World, End, 18.0f + ChainStage * 3.0f, 8, ChainColor, false, LifeSeconds, 0, Thickness * 0.5f);
}

FLinearColor AGP_ChainEffectActor::ResolveStageColor() const
{
	return StageColors.IsValidIndex(ChainStage) ? StageColors[ChainStage] : FLinearColor::Red;
}

float AGP_ChainEffectActor::ResolveStageThickness() const
{
	return FMath::Max(1.0f, BaseLineThickness + static_cast<float>(ChainStage) * BrokenLineThicknessBonus);
}
