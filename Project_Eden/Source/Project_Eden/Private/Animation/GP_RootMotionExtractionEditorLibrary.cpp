#include "Animation/GP_RootMotionExtractionEditorLibrary.h"

#if WITH_EDITOR

#include "Animation/AnimData/IAnimationDataController.h"
#include "Animation/AnimData/IAnimationDataModel.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimTypes.h"
#include "Animation/Skeleton.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"

namespace GPRootMotionExtraction
{
	constexpr TCHAR DefaultRootBoneName[] = TEXT("root");

	FTransform GetTrackTransformOrRefPose(
		const IAnimationDataModel* DataModel,
		const FReferenceSkeleton& ReferenceSkeleton,
		FName BoneName,
		int32 KeyIndex)
	{
		if (DataModel)
		{
			TArray<FName> TrackNames;
			DataModel->GetBoneTrackNames(TrackNames);
			if (TrackNames.Contains(BoneName))
			{
				return DataModel->GetBoneTrackTransform(BoneName, FFrameNumber(KeyIndex));
			}
		}

		const int32 BoneIndex = ReferenceSkeleton.FindBoneIndex(BoneName);
		return BoneIndex != INDEX_NONE ? ReferenceSkeleton.GetRefBonePose()[BoneIndex] : FTransform::Identity;
	}

	bool GetTrackTransforms(
		const IAnimationDataModel* DataModel,
		const FReferenceSkeleton& ReferenceSkeleton,
		FName BoneName,
		int32 NumKeys,
		TArray<FTransform>& OutTransforms)
	{
		OutTransforms.Reset(NumKeys);
		if (!DataModel || NumKeys <= 0)
		{
			return false;
		}

		TArray<FName> TrackNames;
		DataModel->GetBoneTrackNames(TrackNames);
		if (TrackNames.Contains(BoneName))
		{
			DataModel->GetBoneTrackTransforms(BoneName, OutTransforms);
		}
		else
		{
			const int32 BoneIndex = ReferenceSkeleton.FindBoneIndex(BoneName);
			if (BoneIndex == INDEX_NONE)
			{
				return false;
			}

			const FTransform RefTransform = ReferenceSkeleton.GetRefBonePose()[BoneIndex];
			for (int32 KeyIndex = 0; KeyIndex < NumKeys; ++KeyIndex)
			{
				OutTransforms.Add(RefTransform);
			}
		}

		return OutTransforms.Num() == NumKeys;
	}

	void SplitTransformKeys(
		const TArray<FTransform>& Transforms,
		TArray<FVector3f>& OutPositions,
		TArray<FQuat4f>& OutRotations,
		TArray<FVector3f>& OutScales)
	{
		OutPositions.Reset(Transforms.Num());
		OutRotations.Reset(Transforms.Num());
		OutScales.Reset(Transforms.Num());

		for (const FTransform& Transform : Transforms)
		{
			OutPositions.Add(FVector3f(Transform.GetTranslation()));
			OutRotations.Add(FQuat4f(Transform.GetRotation()));
			OutScales.Add(FVector3f(Transform.GetScale3D()));
		}
	}

	UAnimSequence* DuplicateSequence(UAnimSequence* SourceSequence, const FString& AssetSuffix)
	{
		if (!SourceSequence)
		{
			return nullptr;
		}

		const FString SourcePackagePath = FPackageName::GetLongPackagePath(SourceSequence->GetOutermost()->GetName());
		const FString CleanSuffix = AssetSuffix.IsEmpty() ? TEXT("_RM") : AssetSuffix;
		const FString BaseAssetName = SourceSequence->GetName() + CleanSuffix;

		FAssetToolsModule& AssetToolsModule = FAssetToolsModule::GetModule();
		FString UniquePackageName;
		FString UniqueAssetName;
		AssetToolsModule.Get().CreateUniqueAssetName(SourcePackagePath / BaseAssetName, TEXT(""), UniquePackageName, UniqueAssetName);

		return Cast<UAnimSequence>(AssetToolsModule.Get().DuplicateAsset(UniqueAssetName, SourcePackagePath, SourceSequence));
	}

	bool ReadEditableSequenceTracks(
		UAnimSequence* ResultSequence,
		FName MotionSourceBoneName,
		const IAnimationDataModel*& OutDataModel,
		FReferenceSkeleton& OutReferenceSkeleton,
		FName& OutRootBoneName,
		FName& OutSourceBoneName,
		TArray<FTransform>& OutRootTransforms,
		TArray<FTransform>& OutSourceBoneTransforms)
	{
		if (!ResultSequence)
		{
			return false;
		}

		USkeleton* Skeleton = ResultSequence->GetSkeleton();
		if (!Skeleton)
		{
			UE_LOG(LogTemp, Warning, TEXT("Root motion extraction failed for %s: missing skeleton."), *ResultSequence->GetName());
			return false;
		}

		OutReferenceSkeleton = Skeleton->GetReferenceSkeleton();
		OutRootBoneName = FName(DefaultRootBoneName);
		if (OutReferenceSkeleton.FindBoneIndex(OutRootBoneName) == INDEX_NONE)
		{
			UE_LOG(LogTemp, Warning, TEXT("Root motion extraction failed for %s: skeleton has no '%s' bone."), *ResultSequence->GetName(), *OutRootBoneName.ToString());
			return false;
		}

		OutSourceBoneName = MotionSourceBoneName.IsNone() ? FName(TEXT("pelvis")) : MotionSourceBoneName;
		if (OutReferenceSkeleton.FindBoneIndex(OutSourceBoneName) == INDEX_NONE)
		{
			UE_LOG(LogTemp, Warning, TEXT("Root motion extraction failed for %s: skeleton has no '%s' bone."), *ResultSequence->GetName(), *OutSourceBoneName.ToString());
			return false;
		}

		OutDataModel = ResultSequence->GetDataModel();
		if (!OutDataModel)
		{
			UE_LOG(LogTemp, Warning, TEXT("Root motion extraction failed for %s: sequence has no data model."), *ResultSequence->GetName());
			return false;
		}

		const int32 NumKeys = OutDataModel->GetNumberOfKeys();
		if (NumKeys <= 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("Root motion extraction failed for %s: sequence has no keys."), *ResultSequence->GetName());
			return false;
		}

		if (!GetTrackTransforms(OutDataModel, OutReferenceSkeleton, OutSourceBoneName, NumKeys, OutSourceBoneTransforms))
		{
			UE_LOG(LogTemp, Warning, TEXT("Root motion extraction failed for %s: cannot read '%s' track."), *ResultSequence->GetName(), *OutSourceBoneName.ToString());
			return false;
		}

		if (!GetTrackTransforms(OutDataModel, OutReferenceSkeleton, OutRootBoneName, NumKeys, OutRootTransforms))
		{
			UE_LOG(LogTemp, Warning, TEXT("Root motion extraction failed for %s: cannot read root track."), *ResultSequence->GetName());
			return false;
		}

		return true;
	}

	void CommitExtractedTracks(
		UAnimSequence* ResultSequence,
		FName RootBoneName,
		FName SourceBoneName,
		const TArray<FTransform>& RootTransforms,
		const TArray<FTransform>& SourceBoneTransforms,
		bool bEnableRootMotionOnResult,
		const FText& TransactionText)
	{
		TArray<FVector3f> RootPositions;
		TArray<FQuat4f> RootRotations;
		TArray<FVector3f> RootScales;
		SplitTransformKeys(RootTransforms, RootPositions, RootRotations, RootScales);

		TArray<FVector3f> SourcePositions;
		TArray<FQuat4f> SourceRotations;
		TArray<FVector3f> SourceScales;
		SplitTransformKeys(SourceBoneTransforms, SourcePositions, SourceRotations, SourceScales);

		ResultSequence->Modify();
		IAnimationDataController& Controller = ResultSequence->GetController();
		{
			IAnimationDataController::FScopedBracket ScopedBracket(Controller, TransactionText);
			Controller.AddBoneCurve(RootBoneName);
			Controller.SetBoneTrackKeys(RootBoneName, RootPositions, RootRotations, RootScales);
			Controller.SetBoneTrackKeys(SourceBoneName, SourcePositions, SourceRotations, SourceScales);
		}

		ResultSequence->bEnableRootMotion = bEnableRootMotionOnResult;
		ResultSequence->RootMotionRootLock = ERootMotionRootLock::RefPose;
		ResultSequence->MarkPackageDirty();
		ResultSequence->PostEditChange();
		FAssetRegistryModule::AssetCreated(ResultSequence);
	}
}

#endif

UAnimSequence* UGP_RootMotionExtractionEditorLibrary::ExtractRootMotionFromInPlaceAnimation(
	UAnimSequence* SourceSequence,
	FName MotionSourceBoneName,
	bool bExtractHorizontalTranslation,
	bool bExtractYawRotation,
	bool bZeroMotionSourceHorizontalTranslation,
	bool bEnableRootMotionOnResult,
	const FString& AssetSuffix)
{
#if WITH_EDITOR
	if (!SourceSequence)
	{
		UE_LOG(LogTemp, Warning, TEXT("ExtractRootMotionFromInPlaceAnimation failed: SourceSequence is null."));
		return nullptr;
	}

	if (!bExtractHorizontalTranslation && !bExtractYawRotation)
	{
		UE_LOG(LogTemp, Warning, TEXT("ExtractRootMotionFromInPlaceAnimation skipped for %s: no extraction mode is enabled."), *SourceSequence->GetName());
		return nullptr;
	}

	UAnimSequence* ResultSequence = GPRootMotionExtraction::DuplicateSequence(SourceSequence, AssetSuffix);
	if (!ResultSequence)
	{
		UE_LOG(LogTemp, Warning, TEXT("ExtractRootMotionFromInPlaceAnimation failed for %s: duplicate failed."), *SourceSequence->GetName());
		return nullptr;
	}

	const IAnimationDataModel* DataModel = nullptr;
	FReferenceSkeleton ReferenceSkeleton;
	FName RootBoneName;
	FName SourceBoneName;
	TArray<FTransform> SourceBoneTransforms;
	TArray<FTransform> RootTransforms;
	if (!GPRootMotionExtraction::ReadEditableSequenceTracks(ResultSequence, MotionSourceBoneName, DataModel, ReferenceSkeleton, RootBoneName, SourceBoneName, RootTransforms, SourceBoneTransforms))
	{
		return nullptr;
	}

	const FTransform FirstSourceTransform = SourceBoneTransforms[0];
	const FVector FirstSourceLocation = FirstSourceTransform.GetTranslation();
	const FRotator FirstSourceRotation = FirstSourceTransform.GetRotation().Rotator();
	const float FirstSourceYaw = FirstSourceRotation.Yaw;

	for (int32 KeyIndex = 0; KeyIndex < SourceBoneTransforms.Num(); ++KeyIndex)
	{
		FTransform& RootTransform = RootTransforms[KeyIndex];
		FTransform& SourceTransform = SourceBoneTransforms[KeyIndex];

		if (bExtractHorizontalTranslation)
		{
			const FVector SourceLocation = SourceTransform.GetTranslation();
			FVector RootLocation = RootTransform.GetTranslation();
			RootLocation.X += SourceLocation.X - FirstSourceLocation.X;
			RootLocation.Y += SourceLocation.Y - FirstSourceLocation.Y;
			RootTransform.SetTranslation(RootLocation);

			if (bZeroMotionSourceHorizontalTranslation)
			{
				FVector NewSourceLocation = SourceLocation;
				NewSourceLocation.X = FirstSourceLocation.X;
				NewSourceLocation.Y = FirstSourceLocation.Y;
				SourceTransform.SetTranslation(NewSourceLocation);
			}
		}

		if (bExtractYawRotation)
		{
			const FRotator SourceRotator = SourceTransform.GetRotation().Rotator();
			const float DeltaYaw = FRotator::NormalizeAxis(SourceRotator.Yaw - FirstSourceYaw);
			const FQuat RootYawDelta = FRotator(0.0f, DeltaYaw, 0.0f).Quaternion();
			RootTransform.SetRotation(RootYawDelta * RootTransform.GetRotation());

			FRotator NewSourceRotator = SourceRotator;
			NewSourceRotator.Yaw = FirstSourceYaw;
			SourceTransform.SetRotation(NewSourceRotator.Quaternion());
		}
	}

	GPRootMotionExtraction::CommitExtractedTracks(
		ResultSequence,
		RootBoneName,
		SourceBoneName,
		RootTransforms,
		SourceBoneTransforms,
		bEnableRootMotionOnResult,
		NSLOCTEXT("ProjectEden", "ExtractRootMotionFromInPlace", "Extract root motion from in-place animation"));

	UE_LOG(LogTemp, Display, TEXT("Extracted root motion: %s -> %s using source bone '%s'."), *SourceSequence->GetName(), *ResultSequence->GetName(), *SourceBoneName.ToString());
	return ResultSequence;
#else
	return nullptr;
#endif
}

TArray<UAnimSequence*> UGP_RootMotionExtractionEditorLibrary::ExtractRootMotionFromInPlaceAnimations(
	const TArray<UAnimSequence*>& SourceSequences,
	FName MotionSourceBoneName,
	bool bExtractHorizontalTranslation,
	bool bExtractYawRotation,
	bool bZeroMotionSourceHorizontalTranslation,
	bool bEnableRootMotionOnResult,
	const FString& AssetSuffix)
{
	TArray<UAnimSequence*> Results;
	for (UAnimSequence* SourceSequence : SourceSequences)
	{
		if (UAnimSequence* Result = ExtractRootMotionFromInPlaceAnimation(
			SourceSequence,
			MotionSourceBoneName,
			bExtractHorizontalTranslation,
			bExtractYawRotation,
			bZeroMotionSourceHorizontalTranslation,
			bEnableRootMotionOnResult,
			AssetSuffix))
		{
			Results.Add(Result);
		}
	}

	return Results;
}

UAnimSequence* UGP_RootMotionExtractionEditorLibrary::ExtractRootMotionFromDirectionalDodgeAnimation(
	UAnimSequence* SourceSequence,
	FName MotionSourceBoneName,
	bool bZeroMotionSourceHorizontalTranslation,
	bool bEnableRootMotionOnResult,
	const FString& AssetSuffix)
{
#if WITH_EDITOR
	if (!SourceSequence)
	{
		UE_LOG(LogTemp, Warning, TEXT("ExtractRootMotionFromDirectionalDodgeAnimation failed: SourceSequence is null."));
		return nullptr;
	}

	UAnimSequence* ResultSequence = GPRootMotionExtraction::DuplicateSequence(SourceSequence, AssetSuffix);
	if (!ResultSequence)
	{
		UE_LOG(LogTemp, Warning, TEXT("ExtractRootMotionFromDirectionalDodgeAnimation failed for %s: duplicate failed."), *SourceSequence->GetName());
		return nullptr;
	}

	const IAnimationDataModel* DataModel = nullptr;
	FReferenceSkeleton ReferenceSkeleton;
	FName RootBoneName;
	FName SourceBoneName;
	TArray<FTransform> SourceBoneTransforms;
	TArray<FTransform> RootTransforms;
	if (!GPRootMotionExtraction::ReadEditableSequenceTracks(ResultSequence, MotionSourceBoneName, DataModel, ReferenceSkeleton, RootBoneName, SourceBoneName, RootTransforms, SourceBoneTransforms))
	{
		return nullptr;
	}

	FVector InitialDirection = FVector::ZeroVector;
	for (int32 KeyIndex = 1; KeyIndex < SourceBoneTransforms.Num(); ++KeyIndex)
	{
		const FVector Delta = SourceBoneTransforms[KeyIndex].GetTranslation() - SourceBoneTransforms[KeyIndex - 1].GetTranslation();
		const FVector Delta2D(Delta.X, Delta.Y, 0.0);
		if (!Delta2D.IsNearlyZero(0.01))
		{
			InitialDirection = Delta2D.GetSafeNormal();
			break;
		}
	}

	if (InitialDirection.IsNearlyZero())
	{
		UE_LOG(LogTemp, Warning, TEXT("ExtractRootMotionFromDirectionalDodgeAnimation failed for %s: could not detect initial movement direction from '%s'."), *SourceSequence->GetName(), *SourceBoneName.ToString());
		return nullptr;
	}

	const FVector FirstSourceLocation = SourceBoneTransforms[0].GetTranslation();
	const FVector FirstRootLocation = RootTransforms[0].GetTranslation();
	float AccumulatedDistance = 0.0f;

	for (int32 KeyIndex = 0; KeyIndex < SourceBoneTransforms.Num(); ++KeyIndex)
	{
		if (KeyIndex > 0)
		{
			const FVector Delta = SourceBoneTransforms[KeyIndex].GetTranslation() - SourceBoneTransforms[KeyIndex - 1].GetTranslation();
			AccumulatedDistance += FMath::Abs(FVector::DotProduct(FVector(Delta.X, Delta.Y, 0.0), InitialDirection));
		}

		FVector RootLocation = RootTransforms[KeyIndex].GetTranslation();
		const FVector DirectedOffset = InitialDirection * AccumulatedDistance;
		RootLocation.X = FirstRootLocation.X + DirectedOffset.X;
		RootLocation.Y = FirstRootLocation.Y + DirectedOffset.Y;
		RootTransforms[KeyIndex].SetTranslation(RootLocation);

		if (bZeroMotionSourceHorizontalTranslation)
		{
			FVector SourceLocation = SourceBoneTransforms[KeyIndex].GetTranslation();
			SourceLocation.X = FirstSourceLocation.X;
			SourceLocation.Y = FirstSourceLocation.Y;
			SourceBoneTransforms[KeyIndex].SetTranslation(SourceLocation);
		}
	}

	GPRootMotionExtraction::CommitExtractedTracks(
		ResultSequence,
		RootBoneName,
		SourceBoneName,
		RootTransforms,
		SourceBoneTransforms,
		bEnableRootMotionOnResult,
		NSLOCTEXT("ProjectEden", "ExtractRootMotionFromDirectionalDodge", "Extract root motion from directional dodge animation"));

	UE_LOG(LogTemp, Display, TEXT("Extracted directional dodge root motion: %s -> %s using source bone '%s' direction=%s."),
		*SourceSequence->GetName(),
		*ResultSequence->GetName(),
		*SourceBoneName.ToString(),
		*InitialDirection.ToCompactString());
	return ResultSequence;
#else
	return nullptr;
#endif
}

TArray<UAnimSequence*> UGP_RootMotionExtractionEditorLibrary::ExtractRootMotionFromDirectionalDodgeAnimations(
	const TArray<UAnimSequence*>& SourceSequences,
	FName MotionSourceBoneName,
	bool bZeroMotionSourceHorizontalTranslation,
	bool bEnableRootMotionOnResult,
	const FString& AssetSuffix)
{
	TArray<UAnimSequence*> Results;
	for (UAnimSequence* SourceSequence : SourceSequences)
	{
		if (UAnimSequence* Result = ExtractRootMotionFromDirectionalDodgeAnimation(
			SourceSequence,
			MotionSourceBoneName,
			bZeroMotionSourceHorizontalTranslation,
			bEnableRootMotionOnResult,
			AssetSuffix))
		{
			Results.Add(Result);
		}
	}

	return Results;
}
