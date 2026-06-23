#include "VFX/GP_BossTargetMarkerVFXComponent.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"

UGP_BossTargetMarkerVFXComponent::UGP_BossTargetMarkerVFXComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> DefaultTargetMarkerFinder(
		TEXT("/Game/Niagara/Vefects/Render_Particles_On_Top/VFX/Particles/NS_Render_Particles_On_Top_Stroke_03.NS_Render_Particles_On_Top_Stroke_03"));
	if (DefaultTargetMarkerFinder.Succeeded())
	{
		TargetMarkerSystem = DefaultTargetMarkerFinder.Object;
	}
}

void UGP_BossTargetMarkerVFXComponent::PlayTargetMarker(AActor* TargetActor)
{
	if (!ShouldPlayTargetMarker(TargetActor))
	{
		return;
	}

	AActor* OwnerActor = GetOwner();
	if (IsValid(OwnerActor) && OwnerActor->HasAuthority())
	{
		MulticastPlayTargetMarker(TargetActor);
		return;
	}

	// Client-side preview calls are still useful in editor utility tests or non-replicated prototypes.
	PlayTargetMarkerLocal(TargetActor);
}

bool UGP_BossTargetMarkerVFXComponent::ShouldPlayTargetMarker(AActor* TargetActor) const
{
	return bTargetMarkerVFXEnabled && IsValid(TargetActor) && IsValid(TargetMarkerSystem);
}

void UGP_BossTargetMarkerVFXComponent::MulticastPlayTargetMarker_Implementation(AActor* TargetActor)
{
	PlayTargetMarkerLocal(TargetActor);
}

void UGP_BossTargetMarkerVFXComponent::PlayTargetMarkerLocal(AActor* TargetActor) const
{
	UWorld* World = GetWorld();
	if (!IsValid(World) || World->GetNetMode() == NM_DedicatedServer || !ShouldPlayTargetMarker(TargetActor))
	{
		return;
	}

	USceneComponent* AttachComponent = ResolveTargetAttachComponent(TargetActor);
	if (HasTargetBodySocket(TargetActor))
	{
		if (UNiagaraComponent* MarkerComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			TargetMarkerSystem,
			AttachComponent,
			TargetBodySocketName,
			TargetBodyOffset,
			FRotator::ZeroRotator,
			EAttachLocation::KeepRelativeOffset,
			true,
			true,
			ENCPoolMethod::AutoRelease,
			true))
		{
			MarkerComponent->SetWorldScale3D(TargetMarkerScale);
		}
		return;
	}

	const FVector BodyLocation = ResolveTargetBodyLocation(TargetActor);
	if (IsValid(AttachComponent))
	{
		// KeepWorldPosition anchors the one-shot to the torso position while still following the target actor.
		if (UNiagaraComponent* MarkerComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			TargetMarkerSystem,
			AttachComponent,
			NAME_None,
			BodyLocation,
			FRotator::ZeroRotator,
			EAttachLocation::KeepWorldPosition,
			true,
			true,
			ENCPoolMethod::AutoRelease,
			true))
		{
			MarkerComponent->SetWorldScale3D(TargetMarkerScale);
		}
		return;
	}

	if (UNiagaraComponent* MarkerComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		World,
		TargetMarkerSystem,
		BodyLocation,
		FRotator::ZeroRotator,
		TargetMarkerScale,
		true,
		true,
		ENCPoolMethod::AutoRelease,
		true))
	{
		MarkerComponent->SetWorldScale3D(TargetMarkerScale);
	}
}

USceneComponent* UGP_BossTargetMarkerVFXComponent::ResolveTargetAttachComponent(AActor* TargetActor) const
{
	if (!IsValid(TargetActor))
	{
		return nullptr;
	}

	const ACharacter* TargetCharacter = Cast<ACharacter>(TargetActor);
	if (IsValid(TargetCharacter) && IsValid(TargetCharacter->GetMesh()))
	{
		return TargetCharacter->GetMesh();
	}

	return TargetActor->GetRootComponent();
}

bool UGP_BossTargetMarkerVFXComponent::HasTargetBodySocket(AActor* TargetActor) const
{
	if (TargetBodySocketName == NAME_None)
	{
		return false;
	}

	const ACharacter* TargetCharacter = Cast<ACharacter>(TargetActor);
	USkeletalMeshComponent* TargetMesh = IsValid(TargetCharacter) ? TargetCharacter->GetMesh() : nullptr;
	if (!IsValid(TargetMesh) || !TargetMesh->DoesSocketExist(TargetBodySocketName))
	{
		return false;
	}

	return true;
}

FVector UGP_BossTargetMarkerVFXComponent::ResolveTargetBodyLocation(AActor* TargetActor) const
{
	if (!IsValid(TargetActor))
	{
		return FVector::ZeroVector;
	}

	FVector Origin = TargetActor->GetActorLocation();
	FVector BoxExtent = FVector::ZeroVector;
	TargetActor->GetActorBounds(false, Origin, BoxExtent);
	if (!BoxExtent.IsNearlyZero())
	{
		return Origin + TargetBodyOffset;
	}

	return TargetActor->GetActorLocation() + TargetBodyOffset;
}
