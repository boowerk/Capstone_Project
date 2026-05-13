#include "Utils/RootMotionExtractorLibrary.h"

#if WITH_EDITOR
#include "Animation/AnimData/IAnimationDataController.h"
#include "Animation/AnimData/IAnimationDataModel.h"
#include "Animation/AnimEnums.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimTypes.h"
#include "Animation/AnimationAsset.h"
#include "EditorAssetLibrary.h"
#include "EditorUtilityLibrary.h"
#include "Misc/Paths.h"
#include "ReferenceSkeleton.h"

namespace
{
	constexpr TCHAR RootBoneNameLiteral[] = TEXT("root");
	constexpr float GroundedHeightTolerance = 6.0f;
	constexpr float AnchorSwitchBlendHeight = 8.0f;
	constexpr float MotionBlendTolerance = 6.0f;
	constexpr float AirborneHeightTolerance = 12.0f;

	enum class ESupportLimb : uint8
	{
		Left,
		Right
	};

	struct FContactSample
	{
		FVector Left = FVector::ZeroVector;
		FVector Right = FVector::ZeroVector;
	};

	struct FFramePoseSample
	{
		FContactSample Contact;
		FTransform RootTransform = FTransform::Identity;
	};

	struct FRootSetup
	{
		FName RootBoneName = TEXT("root");
		int32 RootBoneIndex = INDEX_NONE;
		FTransform InitialRootTransform = FTransform::Identity;
		FQuat InitialRootRotation = FQuat::Identity;
		FVector RootScale = FVector::OneVector;
	};

	struct FSemanticDelta
	{
		float Forward = 0.0f;
		float Lateral = 0.0f;
		float Vertical = 0.0f;
	};

	static FTransform GetBoneLocalTransform(const UAnimSequence* AnimSeq, int32 BoneIndex, float Time)
	{
		FTransform LocalTransform = FTransform::Identity;
		if (!AnimSeq || BoneIndex == INDEX_NONE)
		{
			return LocalTransform;
		}

		FAnimExtractContext Context(static_cast<double>(Time));
		AnimSeq->GetBoneTransform(LocalTransform, FSkeletonPoseBoneIndex(BoneIndex), Context, false);
		return LocalTransform;
	}

	static FVector GetBoneComponentLocation(const UAnimSequence* AnimSeq, int32 BoneIndex, float Time)
	{
		if (!AnimSeq || BoneIndex == INDEX_NONE || !AnimSeq->GetSkeleton())
		{
			return FVector::ZeroVector;
		}

		const FReferenceSkeleton& RefSkeleton = AnimSeq->GetSkeleton()->GetReferenceSkeleton();
		TArray<int32> BoneChain;
		for (int32 CurrentBoneIndex = BoneIndex; CurrentBoneIndex != INDEX_NONE; CurrentBoneIndex = RefSkeleton.GetParentIndex(CurrentBoneIndex))
		{
			BoneChain.Add(CurrentBoneIndex);
		}

		FTransform ComponentTransform = FTransform::Identity;
		for (int32 ChainIndex = BoneChain.Num() - 1; ChainIndex >= 0; --ChainIndex)
		{
			ComponentTransform = GetBoneLocalTransform(AnimSeq, BoneChain[ChainIndex], Time) * ComponentTransform;
		}

		return ComponentTransform.GetLocation();
	}

	static float GetSampleTime(const IAnimationDataModel* Model, int32 NumFrames, int32 FrameIndex)
	{
		if (!Model || NumFrames <= 0)
		{
			return 0.0f;
		}

		return (static_cast<float>(FrameIndex) / static_cast<float>(NumFrames)) * Model->GetPlayLength();
	}

	static bool BuildRootSetup(UAnimSequence* AnimSeq, FRootSetup& OutSetup)
	{
		if (!AnimSeq || !AnimSeq->GetSkeleton())
		{
			return false;
		}

		const FReferenceSkeleton& RefSkeleton = AnimSeq->GetSkeleton()->GetReferenceSkeleton();
		OutSetup.RootBoneName = RootBoneNameLiteral;
		OutSetup.RootBoneIndex = RefSkeleton.FindBoneIndex(OutSetup.RootBoneName);
		if (OutSetup.RootBoneIndex == INDEX_NONE)
		{
			OutSetup.RootBoneIndex = 0;
			OutSetup.RootBoneName = RefSkeleton.GetBoneName(0);
		}

		OutSetup.InitialRootTransform = GetBoneLocalTransform(AnimSeq, OutSetup.RootBoneIndex, 0.0f);
		OutSetup.InitialRootRotation = OutSetup.InitialRootTransform.GetRotation();
		OutSetup.RootScale = RefSkeleton.GetRefBonePose()[OutSetup.RootBoneIndex].GetScale3D();
		return true;
	}

	static FContactSample SampleContactBones(const UAnimSequence* AnimSeq, int32 LeftIndex, int32 RightIndex, float Time)
	{
		FContactSample Sample;
		Sample.Left = GetBoneComponentLocation(AnimSeq, LeftIndex, Time);
		Sample.Right = GetBoneComponentLocation(AnimSeq, RightIndex, Time);
		return Sample;
	}

	static TArray<FFramePoseSample> CaptureSourceSamples(UAnimSequence* AnimSeq, const FRootSetup& Setup, int32 LeftIndex, int32 RightIndex)
	{
		TArray<FFramePoseSample> Samples;
		const IAnimationDataModel* Model = AnimSeq ? AnimSeq->GetDataModel() : nullptr;
		if (!AnimSeq || !Model)
		{
			return Samples;
		}

		const int32 NumFrames = Model->GetNumberOfFrames();
		Samples.Reserve(NumFrames + 1);

		for (int32 FrameIndex = 0; FrameIndex <= NumFrames; ++FrameIndex)
		{
			const float Time = GetSampleTime(Model, NumFrames, FrameIndex);
			FFramePoseSample Sample;
			Sample.Contact = SampleContactBones(AnimSeq, LeftIndex, RightIndex, Time);
			Sample.RootTransform = GetBoneLocalTransform(AnimSeq, Setup.RootBoneIndex, Time);
			Samples.Add(Sample);
		}

		return Samples;
	}

	static FVector AxisToUnitVector(ERootMotionPlanarAxis Axis)
	{
		switch (Axis)
		{
		case ERootMotionPlanarAxis::PosX:
			return FVector::ForwardVector;
		case ERootMotionPlanarAxis::NegX:
			return -FVector::ForwardVector;
		case ERootMotionPlanarAxis::PosY:
			return FVector::RightVector;
		case ERootMotionPlanarAxis::NegY:
			return -FVector::RightVector;
		default:
			return FVector::ForwardVector;
		}
	}

	static FSemanticDelta ConvertComponentDeltaToSemanticDelta(const FVector& ComponentDelta, const FQuat& SourceRotation, bool bInvertForward, bool bInvertLateral, ERootMotionPlanarAxis SourceForwardAxis)
	{
		const FRotator SourceYawRotation(0.0f, SourceRotation.Rotator().Yaw, 0.0f);
		FVector VisualForwardAxis = SourceYawRotation.RotateVector(AxisToUnitVector(SourceForwardAxis)).GetSafeNormal2D();
		FVector VisualRightAxis(-VisualForwardAxis.Y, VisualForwardAxis.X, 0.0f);

		if (bInvertForward)
		{
			VisualForwardAxis *= -1.0f;
		}
		if (bInvertLateral)
		{
			VisualRightAxis *= -1.0f;
		}

		FSemanticDelta SemanticDelta;
		SemanticDelta.Forward = FVector::DotProduct(ComponentDelta, VisualForwardAxis);
		SemanticDelta.Lateral = FVector::DotProduct(ComponentDelta, VisualRightAxis);
		SemanticDelta.Vertical = ComponentDelta.Z;
		return SemanticDelta;
	}

	static void AddAlongPlanarAxis(FVector& InOutDelta, ERootMotionPlanarAxis Axis, float Amount)
	{
		switch (Axis)
		{
		case ERootMotionPlanarAxis::PosX:
			InOutDelta.X += Amount;
			break;
		case ERootMotionPlanarAxis::NegX:
			InOutDelta.X -= Amount;
			break;
		case ERootMotionPlanarAxis::PosY:
			InOutDelta.Y += Amount;
			break;
		case ERootMotionPlanarAxis::NegY:
			InOutDelta.Y -= Amount;
			break;
		default:
			break;
		}
	}

	static FVector ComposeRootDelta(const FSemanticDelta& SemanticDelta, bool bIncludeZ, ERootMotionPlanarAxis ForwardAxis, ERootMotionPlanarAxis LateralAxis)
	{
		FVector RootDelta = FVector::ZeroVector;
		AddAlongPlanarAxis(RootDelta, ForwardAxis, SemanticDelta.Forward);
		AddAlongPlanarAxis(RootDelta, LateralAxis, SemanticDelta.Lateral);
		RootDelta.Z = bIncludeZ ? SemanticDelta.Vertical : 0.0f;
		return RootDelta;
	}

	static FVector ComputeAnchorDelta(const FContactSample& PreviousSample, const FContactSample& CurrentSample, float ReferenceGroundZ, ESupportLimb& InOutSupportLimb)
	{
		const FVector LeftDelta = CurrentSample.Left - PreviousSample.Left;
		const FVector RightDelta = CurrentSample.Right - PreviousSample.Right;
		const float LeftPlanarSpeed = FVector2D(LeftDelta.X, LeftDelta.Y).Length();
		const float RightPlanarSpeed = FVector2D(RightDelta.X, RightDelta.Y).Length();

		const float GroundZ = FMath::Min(CurrentSample.Left.Z, CurrentSample.Right.Z);
		const float LeftHeightAboveGround = CurrentSample.Left.Z - GroundZ;
		const float RightHeightAboveGround = CurrentSample.Right.Z - GroundZ;
		const bool bLeftAirborne = (CurrentSample.Left.Z - ReferenceGroundZ) > AirborneHeightTolerance;
		const bool bRightAirborne = (CurrentSample.Right.Z - ReferenceGroundZ) > AirborneHeightTolerance;

		if (bLeftAirborne && bRightAirborne)
		{
			return (InOutSupportLimb == ESupportLimb::Left) ? LeftDelta : RightDelta;
		}

		if (LeftHeightAboveGround + GroundedHeightTolerance < RightHeightAboveGround)
		{
			InOutSupportLimb = ESupportLimb::Left;
			return LeftDelta;
		}

		if (RightHeightAboveGround + GroundedHeightTolerance < LeftHeightAboveGround)
		{
			InOutSupportLimb = ESupportLimb::Right;
			return RightDelta;
		}

		const float HeightDifference = CurrentSample.Right.Z - CurrentSample.Left.Z;
		const float LeftHeightWeight = FMath::GetMappedRangeValueClamped(
			FVector2D(-AnchorSwitchBlendHeight, AnchorSwitchBlendHeight),
			FVector2D(0.0f, 1.0f),
			HeightDifference);
		const float RightHeightWeight = 1.0f - LeftHeightWeight;

		const float LeftMotionWeight = FMath::Clamp(LeftPlanarSpeed / MotionBlendTolerance, 0.0f, 1.0f);
		const float RightMotionWeight = FMath::Clamp(RightPlanarSpeed / MotionBlendTolerance, 0.0f, 1.0f);
		const float LeftScore = (LeftHeightWeight * 0.8f) + (LeftMotionWeight * 0.2f);
		const float RightScore = (RightHeightWeight * 0.8f) + (RightMotionWeight * 0.2f);
		const float ScoreSum = FMath::Max(LeftScore + RightScore, UE_SMALL_NUMBER);

		const float LeftWeight = LeftScore / ScoreSum;
		const float RightWeight = RightScore / ScoreSum;
		InOutSupportLimb = (LeftWeight >= RightWeight) ? ESupportLimb::Left : ESupportLimb::Right;
		return (LeftDelta * LeftWeight) + (RightDelta * RightWeight);
	}

	static void WriteCentralizedRootPose(UAnimSequence* AnimSeq, const FRootSetup& Setup)
	{
		IAnimationDataController& Controller = AnimSeq->GetController();
		const IAnimationDataModel* Model = AnimSeq->GetDataModel();
		if (!Model)
		{
			return;
		}

		const int32 NumFrames = Model->GetNumberOfFrames();
		TArray<FVector> RootPosKeys;
		TArray<FQuat> RootRotKeys;
		TArray<FVector> RootScaleKeys;
		RootPosKeys.Init(FVector::ZeroVector, NumFrames + 1);
		RootRotKeys.Reserve(NumFrames + 1);
		RootScaleKeys.Init(Setup.RootScale, NumFrames + 1);

		for (int32 FrameIndex = 0; FrameIndex <= NumFrames; ++FrameIndex)
		{
			const float Time = GetSampleTime(Model, NumFrames, FrameIndex);
			RootRotKeys.Add(GetBoneLocalTransform(AnimSeq, Setup.RootBoneIndex, Time).GetRotation());
		}

		Controller.OpenBracket(FText::FromString(TEXT("Normalize Root Pose")));
		Controller.RemoveBoneTrack(Setup.RootBoneName);
		Controller.AddBoneTrack(Setup.RootBoneName);
		Controller.SetBoneTrackKeys(Setup.RootBoneName, RootPosKeys, RootRotKeys, RootScaleKeys);
		Controller.CloseBracket();
	}
}
#endif

void URootMotionExtractorLibrary::BatchNormalizeRootPose(FString Suffix)
{
#if WITH_EDITOR
	const TArray<UObject*> SelectedAssets = UEditorUtilityLibrary::GetSelectedAssets();
	for (UObject* Asset : SelectedAssets)
	{
		UAnimSequence* AnimSeq = Cast<UAnimSequence>(Asset);
		if (!AnimSeq)
		{
			continue;
		}

		const FString OriginalPath = AnimSeq->GetPathName();
		const FString NewPath = FPaths::Combine(FPaths::GetPath(OriginalPath), FPaths::GetBaseFilename(OriginalPath) + Suffix);
		if (UEditorAssetLibrary::DoesAssetExist(NewPath))
		{
			UEditorAssetLibrary::DeleteAsset(NewPath);
		}

		UAnimSequence* NewAnimSeq = Cast<UAnimSequence>(UEditorAssetLibrary::DuplicateAsset(OriginalPath, NewPath));
		if (!NewAnimSeq)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to duplicate animation asset for root normalization: %s"), *OriginalPath);
			continue;
		}

		NewAnimSeq->bEnableRootMotion = true;
		NewAnimSeq->bForceRootLock = true;
		NewAnimSeq->RootMotionRootLock = ERootMotionRootLock::RefPose;
		ProcessRootPoseNormalization(NewAnimSeq);
		UEditorAssetLibrary::SaveAsset(NewPath, false);
	}
#else
	UE_LOG(LogTemp, Warning, TEXT("BatchNormalizeRootPose is only available in the editor."));
#endif
}

void URootMotionExtractorLibrary::BatchExtractRootMotionFromFeet(FString Suffix, FName LeftFootBone, FName RightFootBone, bool bInvertForward, bool bInvertLateral, bool bIncludeZ, ERootMotionPlanarAxis SourceForwardAxis, ERootMotionPlanarAxis ForwardAxis, ERootMotionPlanarAxis LateralAxis)
{
#if WITH_EDITOR
	const TArray<UObject*> SelectedAssets = UEditorUtilityLibrary::GetSelectedAssets();
	for (UObject* Asset : SelectedAssets)
	{
		UAnimSequence* AnimSeq = Cast<UAnimSequence>(Asset);
		if (!AnimSeq)
		{
			continue;
		}

		const FString OriginalPath = AnimSeq->GetPathName();
		const FString NewPath = FPaths::Combine(FPaths::GetPath(OriginalPath), FPaths::GetBaseFilename(OriginalPath) + Suffix);
		if (UEditorAssetLibrary::DoesAssetExist(NewPath))
		{
			UEditorAssetLibrary::DeleteAsset(NewPath);
		}

		UAnimSequence* NewAnimSeq = Cast<UAnimSequence>(UEditorAssetLibrary::DuplicateAsset(OriginalPath, NewPath));
		if (!NewAnimSeq)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to duplicate animation asset for root motion extraction: %s"), *OriginalPath);
			continue;
		}

		NewAnimSeq->bEnableRootMotion = true;
		NewAnimSeq->bForceRootLock = false;
		NewAnimSeq->RootMotionRootLock = ERootMotionRootLock::AnimFirstFrame;
		ProcessRelayAnchorInference(NewAnimSeq, LeftFootBone, RightFootBone, bInvertForward, bInvertLateral, bIncludeZ, SourceForwardAxis, ForwardAxis, LateralAxis);
		UEditorAssetLibrary::SaveAsset(NewPath, false);
	}
#else
	UE_LOG(LogTemp, Warning, TEXT("BatchExtractRootMotionFromFeet is only available in the editor."));
#endif
}

void URootMotionExtractorLibrary::ProcessRootPoseNormalization(UAnimSequence* AnimSeq)
{
#if WITH_EDITOR
	FRootSetup Setup;
	if (!BuildRootSetup(AnimSeq, Setup))
	{
		return;
	}

	WriteCentralizedRootPose(AnimSeq, Setup);
	AnimSeq->MarkPackageDirty();
	AnimSeq->PostEditChange();

	UE_LOG(LogTemp, Warning, TEXT("Root pose normalized for %s. RootBone=%s | RootLock=RefPose"),
		*AnimSeq->GetName(),
		*Setup.RootBoneName.ToString());
#endif
}

void URootMotionExtractorLibrary::ProcessRelayAnchorInference(UAnimSequence* AnimSeq, FName LeftFoot, FName RightFoot, bool bInvertForward, bool bInvertLateral, bool bIncludeZ, ERootMotionPlanarAxis SourceForwardAxis, ERootMotionPlanarAxis ForwardAxis, ERootMotionPlanarAxis LateralAxis)
{
#if WITH_EDITOR
	FRootSetup Setup;
	if (!BuildRootSetup(AnimSeq, Setup))
	{
		return;
	}

	const IAnimationDataModel* Model = AnimSeq->GetDataModel();
	if (!Model)
	{
		return;
	}

	const FReferenceSkeleton& RefSkeleton = AnimSeq->GetSkeleton()->GetReferenceSkeleton();
	const int32 LeftFootIndex = RefSkeleton.FindBoneIndex(LeftFoot);
	const int32 RightFootIndex = RefSkeleton.FindBoneIndex(RightFoot);
	if (LeftFootIndex == INDEX_NONE || RightFootIndex == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("Root motion extraction skipped for %s. Missing bones: %s / %s"),
			*AnimSeq->GetName(),
			*LeftFoot.ToString(),
			*RightFoot.ToString());
		return;
	}

	const int32 NumFrames = Model->GetNumberOfFrames();
	const TArray<FFramePoseSample> SourceSamples = CaptureSourceSamples(AnimSeq, Setup, LeftFootIndex, RightFootIndex);
	if (SourceSamples.Num() != NumFrames + 1)
	{
		return;
	}

	WriteCentralizedRootPose(AnimSeq, Setup);

	IAnimationDataController& Controller = AnimSeq->GetController();
	TArray<FVector> RootPosKeys;
	TArray<FQuat> RootRotKeys;
	TArray<FVector> RootScaleKeys;
	RootPosKeys.Reserve(NumFrames + 1);
	RootRotKeys.Reserve(NumFrames + 1);
	RootScaleKeys.Init(Setup.RootScale, NumFrames + 1);

	const FContactSample InitialSample = SourceSamples[0].Contact;
	FContactSample PreviousSample = InitialSample;
	const float ReferenceGroundZ = FMath::Min(InitialSample.Left.Z, InitialSample.Right.Z);
	ESupportLimb SupportLimb = (InitialSample.Left.Z <= InitialSample.Right.Z) ? ESupportLimb::Left : ESupportLimb::Right;

	FVector AccumulatedRootTranslation = FVector::ZeroVector;
	for (int32 FrameIndex = 0; FrameIndex <= NumFrames; ++FrameIndex)
	{
		if (FrameIndex > 0)
		{
			const FContactSample& CurrentSample = SourceSamples[FrameIndex].Contact;
			const FVector AnchorDelta = ComputeAnchorDelta(PreviousSample, CurrentSample, ReferenceGroundZ, SupportLimb);
			const FSemanticDelta SemanticDelta = ConvertComponentDeltaToSemanticDelta(AnchorDelta, Setup.InitialRootRotation, bInvertForward, bInvertLateral, SourceForwardAxis);
			const FVector RootDelta = ComposeRootDelta(SemanticDelta, bIncludeZ, ForwardAxis, LateralAxis);

			AccumulatedRootTranslation += RootDelta;
			PreviousSample = CurrentSample;
		}

		RootPosKeys.Add(AccumulatedRootTranslation);
		RootRotKeys.Add(Setup.InitialRootRotation);
	}

	Controller.OpenBracket(FText::FromString(TEXT("Apply Inferred Root Motion")));
	Controller.RemoveBoneTrack(Setup.RootBoneName);
	Controller.AddBoneTrack(Setup.RootBoneName);
	Controller.SetBoneTrackKeys(Setup.RootBoneName, RootPosKeys, RootRotKeys, RootScaleKeys);
	Controller.CloseBracket();
	AnimSeq->MarkPackageDirty();
	AnimSeq->PostEditChange();

	UE_LOG(LogTemp, Warning, TEXT("Root motion extracted for %s. Final root translation: %s"),
		*AnimSeq->GetName(),
		*AccumulatedRootTranslation.ToString());
	const int32 MidFrameIndex = NumFrames > 0 ? (NumFrames / 2) : 0;
	UE_LOG(LogTemp, Warning, TEXT("Root motion extractor root keys -> First: %s | Mid: %s | Last: %s"),
		*RootPosKeys[0].ToString(),
		*RootPosKeys[MidFrameIndex].ToString(),
		*RootPosKeys.Last().ToString());
	UE_LOG(LogTemp, Warning, TEXT("Root motion extractor settings -> IncludeZ: %s | InvertForward: %s | InvertLateral: %s | SourceForwardAxis=%d | ForwardAxis=%d | LateralAxis=%d | RootBone=%s | RootLock=AnimFirstFrame | ForceRootLock=False | EnableRootMotion=True"),
		bIncludeZ ? TEXT("True") : TEXT("False"),
		bInvertForward ? TEXT("True") : TEXT("False"),
		bInvertLateral ? TEXT("True") : TEXT("False"),
		static_cast<int32>(SourceForwardAxis),
		static_cast<int32>(ForwardAxis),
		static_cast<int32>(LateralAxis),
		*Setup.RootBoneName.ToString());
	UE_LOG(LogTemp, Warning, TEXT("Root motion extractor source basis -> RawSourcePose | RootRotationMode=InitialConstant"));
#endif
}
