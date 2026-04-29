#include "AI/Debug/EnemyAIRangeVisualizationComponent.h"

#include "PrimitiveDrawInterface.h"
#include "PrimitiveDrawingUtils.h"
#include "PrimitiveSceneProxy.h"

namespace
{
	constexpr int32 RangeCircleSegments = 96;
	constexpr float RingHeightStep = 8.0f;

	FLinearColor WithAlpha(const FColor& Color, float Alpha)
	{
		FLinearColor LinearColor(Color);
		LinearColor.A = Alpha;
		return LinearColor;
	}

	void DrawRangeRing(FPrimitiveDrawInterface* PDI, const FVector& Origin, float Radius, const FLinearColor& Color, float HeightOffset, float Thickness)
	{
		if (Radius <= 0.0f)
		{
			return;
		}

		// Draw flat world-space rings so selected actors do not inherit the editor's orange selection color.
		DrawCircle(
			PDI,
			Origin + FVector(0.0f, 0.0f, HeightOffset),
			FVector::XAxisVector,
			FVector::YAxisVector,
			Color,
			Radius,
			RangeCircleSegments,
			SDPG_Foreground,
			Thickness);
	}

	void DrawForwardSightGuide(
		FPrimitiveDrawInterface* PDI,
		const FVector& Origin,
		const FVector& Forward,
		float Radius,
		float HalfAngleDegrees,
		const FLinearColor& Color)
	{
		if (Radius <= 0.0f)
		{
			return;
		}

		const FVector Forward2D = FVector(Forward.X, Forward.Y, 0.0f).GetSafeNormal(UE_SMALL_NUMBER, FVector::XAxisVector);
		const FVector LeftEdge = Forward2D.RotateAngleAxis(-HalfAngleDegrees, FVector::ZAxisVector);
		const FVector RightEdge = Forward2D.RotateAngleAxis(HalfAngleDegrees, FVector::ZAxisVector);
		const FVector RaisedOrigin = Origin + FVector(0.0f, 0.0f, RingHeightStep * 3.0f);

		PDI->DrawLine(RaisedOrigin, RaisedOrigin + LeftEdge * Radius, Color, SDPG_Foreground, 2.0f);
		PDI->DrawLine(RaisedOrigin, RaisedOrigin + RightEdge * Radius, Color, SDPG_Foreground, 2.0f);
		PDI->DrawLine(RaisedOrigin, RaisedOrigin + Forward2D * Radius, Color, SDPG_Foreground, 1.0f);
	}
}

class FEnemyAIRangeVisualizationSceneProxy final : public FPrimitiveSceneProxy
{
public:
	explicit FEnemyAIRangeVisualizationSceneProxy(const UEnemyAIRangeVisualizationComponent* InComponent)
		: FPrimitiveSceneProxy(InComponent)
		, ReturnHomeRadius(InComponent->ReturnHomeRadius)
		, PatrolRadius(InComponent->PatrolRadius)
		, SightRadius(InComponent->SightRadius)
		, LoseSightRadius(InComponent->LoseSightRadius)
		, PeripheralVisionAngleDegrees(InComponent->PeripheralVisionAngleDegrees)
		, bDrawOnlyIfSelected(InComponent->bDrawOnlyIfSelected)
	{
		bWillEverBeLit = false;
	}

	virtual SIZE_T GetTypeHash() const override
	{
		static size_t UniquePointer;
		return reinterpret_cast<size_t>(&UniquePointer);
	}

	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
	{
		const FVector Origin = GetLocalToWorld().GetOrigin();
		const FVector Forward = GetLocalToWorld().GetScaledAxis(EAxis::X);

		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ++ViewIndex)
		{
			if ((VisibilityMap & (1 << ViewIndex)) == 0)
			{
				continue;
			}

			FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);

			DrawRangeRing(PDI, Origin, ReturnHomeRadius, WithAlpha(FColor(255, 72, 72), 1.0f), RingHeightStep * 0.0f, 3.0f);
			DrawRangeRing(PDI, Origin, PatrolRadius, WithAlpha(FColor(64, 160, 255), 1.0f), RingHeightStep * 1.0f, 3.0f);
			DrawRangeRing(PDI, Origin, SightRadius, WithAlpha(FColor(50, 255, 120), 1.0f), RingHeightStep * 2.0f, 3.0f);
			DrawRangeRing(PDI, Origin, LoseSightRadius, WithAlpha(FColor(180, 255, 120), 0.85f), RingHeightStep * 3.0f, 1.5f);
			DrawForwardSightGuide(PDI, Origin, Forward, SightRadius, PeripheralVisionAngleDegrees, WithAlpha(FColor(50, 255, 120), 1.0f));
		}
	}

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override
	{
		FPrimitiveViewRelevance Result;
		const bool bProxyVisible = !bDrawOnlyIfSelected || IsSelected() || IsIndividuallySelected();
		Result.bDrawRelevance = IsShown(View) && bProxyVisible;
		Result.bDynamicRelevance = true;
		Result.bEditorPrimitiveRelevance = UseEditorCompositing(View);
		return Result;
	}

	virtual uint32 GetMemoryFootprint() const override
	{
		return sizeof(*this) + GetAllocatedSize();
	}

	uint32 GetAllocatedSize() const
	{
		return FPrimitiveSceneProxy::GetAllocatedSize();
	}

private:
	float ReturnHomeRadius = 0.0f;
	float PatrolRadius = 0.0f;
	float SightRadius = 0.0f;
	float LoseSightRadius = 0.0f;
	float PeripheralVisionAngleDegrees = 0.0f;
	bool bDrawOnlyIfSelected = true;
};

UEnemyAIRangeVisualizationComponent::UEnemyAIRangeVisualizationComponent()
{
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);
	SetCanEverAffectNavigation(false);
	SetHiddenInGame(true);
	bUseEditorCompositing = true;
}

void UEnemyAIRangeVisualizationComponent::ConfigureRanges(
	float InReturnHomeRadius,
	float InPatrolRadius,
	float InSightRadius,
	float InLoseSightRadius,
	float InPeripheralVisionAngleDegrees)
{
	ReturnHomeRadius = FMath::Max(0.0f, InReturnHomeRadius);
	PatrolRadius = FMath::Max(0.0f, InPatrolRadius);
	SightRadius = FMath::Max(0.0f, InSightRadius);
	LoseSightRadius = FMath::Max(SightRadius, InLoseSightRadius);
	PeripheralVisionAngleDegrees = FMath::Clamp(InPeripheralVisionAngleDegrees, 0.0f, 180.0f);
	MarkRenderStateDirty();
	UpdateBounds();
}

void UEnemyAIRangeVisualizationComponent::ConfigureVisibility(bool bInShowRanges, bool bInDrawOnlyIfSelected)
{
	bDrawOnlyIfSelected = bInDrawOnlyIfSelected;
	SetVisibility(bInShowRanges);
	MarkRenderStateDirty();
}

FPrimitiveSceneProxy* UEnemyAIRangeVisualizationComponent::CreateSceneProxy()
{
	return new FEnemyAIRangeVisualizationSceneProxy(this);
}

FBoxSphereBounds UEnemyAIRangeVisualizationComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	const float MaxRadius = FMath::Max3(ReturnHomeRadius, PatrolRadius, FMath::Max(SightRadius, LoseSightRadius));
	const FVector Extent(MaxRadius, MaxRadius, FMath::Max(64.0f, MaxRadius * 0.05f));
	return FBoxSphereBounds(FBox::BuildAABB(LocalToWorld.GetLocation(), Extent));
}
