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
	constexpr float FootPlantHeightTolerance = 6.0f;
	constexpr float FootPlantMinDelta = 0.01f;
	constexpr int32 FootPlantSmoothingPasses = 2;

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

	bool BuildBoneChain(
		const FReferenceSkeleton& ReferenceSkeleton,
		FName BoneName,
		TArray<FName>& OutBoneChain)
	{
		OutBoneChain.Reset();

		int32 BoneIndex = ReferenceSkeleton.FindBoneIndex(BoneName);
		if (BoneIndex == INDEX_NONE)
		{
			return false;
		}

		while (BoneIndex != INDEX_NONE)
		{
			OutBoneChain.Insert(ReferenceSkeleton.GetBoneName(BoneIndex), 0);
			BoneIndex = ReferenceSkeleton.GetParentIndex(BoneIndex);
		}

		return OutBoneChain.Num() > 0;
	}

	FTransform GetLocalTransformForExtraction(
		const IAnimationDataModel* DataModel,
		const FReferenceSkeleton& ReferenceSkeleton,
		FName BoneName,
		FName RootBoneName,
		const TArray<FTransform>& RootTransforms,
		int32 KeyIndex)
	{
		if (BoneName == RootBoneName && RootTransforms.IsValidIndex(KeyIndex))
		{
			return RootTransforms[KeyIndex];
		}

		return GetTrackTransformOrRefPose(DataModel, ReferenceSkeleton, BoneName, KeyIndex);
	}

	FTransform BuildComponentSpaceTransformAtKey(
		const IAnimationDataModel* DataModel,
		const FReferenceSkeleton& ReferenceSkeleton,
		const TArray<FName>& BoneChain,
		FName RootBoneName,
		const TArray<FTransform>& RootTransforms,
		int32 KeyIndex,
		int32 LastChainIndex = INDEX_NONE)
	{
		FTransform ComponentTransform = FTransform::Identity;
		const int32 ChainEnd = LastChainIndex == INDEX_NONE ? BoneChain.Num() - 1 : LastChainIndex;
		for (int32 ChainIndex = 0; ChainIndex <= ChainEnd; ++ChainIndex)
		{
			const FTransform LocalTransform = GetLocalTransformForExtraction(
				DataModel,
				ReferenceSkeleton,
				BoneChain[ChainIndex],
				RootBoneName,
				RootTransforms,
				KeyIndex);
			ComponentTransform = LocalTransform * ComponentTransform;
		}

		return ComponentTransform;
	}

	bool GetComponentSpaceTransforms(
		const IAnimationDataModel* DataModel,
		const FReferenceSkeleton& ReferenceSkeleton,
		FName BoneName,
		FName RootBoneName,
		const TArray<FTransform>& RootTransforms,
		int32 NumKeys,
		TArray<FTransform>& OutComponentTransforms)
	{
		TArray<FName> BoneChain;
		if (!BuildBoneChain(ReferenceSkeleton, BoneName, BoneChain))
		{
			return false;
		}

		OutComponentTransforms.Reset(NumKeys);
		for (int32 KeyIndex = 0; KeyIndex < NumKeys; ++KeyIndex)
		{
			OutComponentTransforms.Add(BuildComponentSpaceTransformAtKey(
				DataModel,
				ReferenceSkeleton,
				BoneChain,
				RootBoneName,
				RootTransforms,
				KeyIndex));
		}

		return OutComponentTransforms.Num() == NumKeys;
	}

	bool IsLikelyFootBone(FName BoneName)
	{
		const FString LowerName = BoneName.ToString().ToLower();
		return LowerName.Contains(TEXT("foot"))
			|| LowerName.Contains(TEXT("ball"))
			|| LowerName.Contains(TEXT("toe"));
	}

	bool IsIgnoredFootHelperBone(FName BoneName)
	{
		const FString LowerName = BoneName.ToString().ToLower();
		return LowerName.Contains(TEXT("ik"))
			|| LowerName.Contains(TEXT("marker"))
			|| LowerName.Contains(TEXT("socket"))
			|| LowerName.Contains(TEXT("twist"))
			|| LowerName.Contains(TEXT("corrective"));
	}

	bool HasMeaningfulNetHorizontalMotion(const TArray<FTransform>& ComponentTransforms, float MinDistance = 5.0f)
	{
		if (ComponentTransforms.Num() < 2)
		{
			return false;
		}

		const FVector StartLocation = ComponentTransforms[0].GetTranslation();
		const FVector EndLocation = ComponentTransforms.Last().GetTranslation();
		return FVector(EndLocation.X - StartLocation.X, EndLocation.Y - StartLocation.Y, 0.0).Size() >= MinDistance;
	}

	float GetNetHorizontalDistance(const TArray<FTransform>& Transforms)
	{
		if (Transforms.Num() < 2)
		{
			return 0.0f;
		}

		const FVector StartLocation = Transforms[0].GetTranslation();
		const FVector EndLocation = Transforms.Last().GetTranslation();
		return FVector(EndLocation.X - StartLocation.X, EndLocation.Y - StartLocation.Y, 0.0).Size();
	}

	float GetPathHorizontalDistance(const TArray<FTransform>& Transforms)
	{
		float Distance = 0.0f;
		for (int32 KeyIndex = 1; KeyIndex < Transforms.Num(); ++KeyIndex)
		{
			const FVector PreviousLocation = Transforms[KeyIndex - 1].GetTranslation();
			const FVector Location = Transforms[KeyIndex].GetTranslation();
			Distance += FVector(Location.X - PreviousLocation.X, Location.Y - PreviousLocation.Y, 0.0).Size();
		}

		return Distance;
	}

	float GetNetHorizontalDistance(const TArray<FVector>& Locations)
	{
		if (Locations.Num() < 2)
		{
			return 0.0f;
		}

		return FVector(Locations.Last().X - Locations[0].X, Locations.Last().Y - Locations[0].Y, 0.0).Size();
	}

	void SmoothHorizontalRootLocations(TArray<FVector>& Locations, int32 Passes = FootPlantSmoothingPasses)
	{
		if (Locations.Num() < 3 || Passes <= 0)
		{
			return;
		}

		const FVector StartLocation = Locations[0];
		const FVector EndLocation = Locations.Last();
		for (int32 PassIndex = 0; PassIndex < Passes; ++PassIndex)
		{
			TArray<FVector> SmoothedLocations = Locations;
			for (int32 KeyIndex = 1; KeyIndex < Locations.Num() - 1; ++KeyIndex)
			{
				const FVector PreviousLocation = Locations[KeyIndex - 1];
				const FVector Location = Locations[KeyIndex];
				const FVector NextLocation = Locations[KeyIndex + 1];

				SmoothedLocations[KeyIndex].X = PreviousLocation.X * 0.25 + Location.X * 0.5 + NextLocation.X * 0.25;
				SmoothedLocations[KeyIndex].Y = PreviousLocation.Y * 0.25 + Location.Y * 0.5 + NextLocation.Y * 0.25;
			}
			Locations = MoveTemp(SmoothedLocations);
			Locations[0] = StartLocation;
			Locations.Last() = EndLocation;
		}
	}

	void LinearizeHorizontalRootLocations(TArray<FVector>& Locations)
	{
		if (Locations.Num() < 3)
		{
			return;
		}

		const FVector StartLocation = Locations[0];
		const FVector EndLocation = Locations.Last();
		const FVector HorizontalDelta(EndLocation.X - StartLocation.X, EndLocation.Y - StartLocation.Y, 0.0);
		if (HorizontalDelta.IsNearlyZero(1.0))
		{
			return;
		}

		const int32 LastIndex = Locations.Num() - 1;
		for (int32 KeyIndex = 1; KeyIndex < LastIndex; ++KeyIndex)
		{
			const double Alpha = static_cast<double>(KeyIndex) / static_cast<double>(LastIndex);
			Locations[KeyIndex].X = FMath::Lerp(StartLocation.X, EndLocation.X, Alpha);
			Locations[KeyIndex].Y = FMath::Lerp(StartLocation.Y, EndLocation.Y, Alpha);
		}
	}

	float SnapAngleDegrees(float AngleDegrees, float SnapDegrees)
	{
		if (SnapDegrees <= 0.0f)
		{
			return AngleDegrees;
		}

		return FMath::RoundToFloat(AngleDegrees / SnapDegrees) * SnapDegrees;
	}

	void SnapHorizontalRootDirection(TArray<FVector>& Locations, float SnapDegrees)
	{
		if (Locations.Num() < 2 || SnapDegrees <= 0.0f)
		{
			return;
		}

		const FVector StartLocation = Locations[0];
		const FVector EndLocation = Locations.Last();
		const FVector ExistingDelta(EndLocation.X - StartLocation.X, EndLocation.Y - StartLocation.Y, 0.0);
		if (ExistingDelta.IsNearlyZero(1.0))
		{
			return;
		}

		const float CurrentYaw = FMath::RadiansToDegrees(FMath::Atan2(ExistingDelta.Y, ExistingDelta.X));
		const float SnappedYaw = SnapAngleDegrees(CurrentYaw, SnapDegrees);
		const FQuat SnapDeltaRotation = FRotator(0.0f, FRotator::NormalizeAxis(SnappedYaw - CurrentYaw), 0.0f).Quaternion();
		for (FVector& Location : Locations)
		{
			const FVector Offset(Location.X - StartLocation.X, Location.Y - StartLocation.Y, 0.0);
			const FVector RotatedOffset = SnapDeltaRotation.RotateVector(Offset);
			Location.X = StartLocation.X + RotatedOffset.X;
			Location.Y = StartLocation.Y + RotatedOffset.Y;
		}
	}

	float GetMaxHorizontalSpeed(const TArray<FVector>& Locations, float Duration)
	{
		if (Locations.Num() < 2 || Duration <= 0.0f)
		{
			return 0.0f;
		}

		const float FrameTime = Duration / static_cast<float>(Locations.Num() - 1);
		if (FrameTime <= 0.0f)
		{
			return 0.0f;
		}

		float MaxSpeed = 0.0f;
		for (int32 KeyIndex = 1; KeyIndex < Locations.Num(); ++KeyIndex)
		{
			const FVector PreviousLocation = Locations[KeyIndex - 1];
			const FVector Location = Locations[KeyIndex];
			const float FrameDistance = FVector(Location.X - PreviousLocation.X, Location.Y - PreviousLocation.Y, 0.0).Size();
			MaxSpeed = FMath::Max(MaxSpeed, FrameDistance / FrameTime);
		}

		return MaxSpeed;
	}

	void ApplyMaxSpeedHorizontalDistance(TArray<FVector>& Locations, float Duration, float SpeedScale)
	{
		if (Locations.Num() < 2 || Duration <= 0.0f)
		{
			return;
		}

		const FVector StartLocation = Locations[0];
		const FVector EndLocation = Locations.Last();
		const FVector ExistingDelta(EndLocation.X - StartLocation.X, EndLocation.Y - StartLocation.Y, 0.0);
		const FVector Direction = ExistingDelta.GetSafeNormal();
		const float MaxSpeed = GetMaxHorizontalSpeed(Locations, Duration);
		if (Direction.IsNearlyZero() || MaxSpeed <= 0.0f)
		{
			return;
		}

		const FVector NewEndLocation = StartLocation + Direction * MaxSpeed * FMath::Max(SpeedScale, 0.0f) * Duration;
		Locations.Last().X = NewEndLocation.X;
		Locations.Last().Y = NewEndLocation.Y;
	}

	bool BuildFootPlantRootLocations(
		const TArray<FTransform>& SourceComponentTransforms,
		const TArray<FTransform>& RootTransforms,
		bool bUseConstantSpeed,
		float Duration,
		float SpeedScale,
		float DirectionSnapDegrees,
		TArray<FVector>& OutRootLocations)
	{
		OutRootLocations.Reset(SourceComponentTransforms.Num());
		if (SourceComponentTransforms.Num() != RootTransforms.Num() || SourceComponentTransforms.Num() <= 0)
		{
			return false;
		}

		float LowestFootZ = TNumericLimits<float>::Max();
		for (const FTransform& SourceTransform : SourceComponentTransforms)
		{
			LowestFootZ = FMath::Min(LowestFootZ, static_cast<float>(SourceTransform.GetTranslation().Z));
		}

		FVector AccumulatedRootOffset = FVector::ZeroVector;
		OutRootLocations.Add(RootTransforms[0].GetTranslation());
		for (int32 KeyIndex = 1; KeyIndex < SourceComponentTransforms.Num(); ++KeyIndex)
		{
			const FVector PreviousSourceLocation = SourceComponentTransforms[KeyIndex - 1].GetTranslation();
			const FVector SourceLocation = SourceComponentTransforms[KeyIndex].GetTranslation();
			const float LowerFootZ = FMath::Min(PreviousSourceLocation.Z, SourceLocation.Z);

			if (LowerFootZ <= LowestFootZ + FootPlantHeightTolerance)
			{
				FVector PlantDelta = SourceLocation - PreviousSourceLocation;
				PlantDelta.Z = 0.0;
				if (!PlantDelta.IsNearlyZero(FootPlantMinDelta))
				{
					AccumulatedRootOffset -= PlantDelta;
				}
			}

			FVector RootLocation = RootTransforms[KeyIndex].GetTranslation();
			RootLocation.X = RootTransforms[0].GetTranslation().X + AccumulatedRootOffset.X;
			RootLocation.Y = RootTransforms[0].GetTranslation().Y + AccumulatedRootOffset.Y;
			OutRootLocations.Add(RootLocation);
		}

		if (bUseConstantSpeed)
		{
			ApplyMaxSpeedHorizontalDistance(OutRootLocations, Duration, SpeedScale);
			SnapHorizontalRootDirection(OutRootLocations, DirectionSnapDegrees);
			LinearizeHorizontalRootLocations(OutRootLocations);
		}
		else
		{
			SnapHorizontalRootDirection(OutRootLocations, DirectionSnapDegrees);
			SmoothHorizontalRootLocations(OutRootLocations);
		}
		return OutRootLocations.Num() == RootTransforms.Num();
	}

	bool BuildAutoFootPlantRootLocations(
		const IAnimationDataModel* DataModel,
		const FReferenceSkeleton& ReferenceSkeleton,
		FName RootBoneName,
		const TArray<FTransform>& RootTransforms,
		int32 NumKeys,
		bool bUseConstantSpeed,
		float Duration,
		float SpeedScale,
		float DirectionSnapDegrees,
		TArray<FVector>& OutRootLocations)
	{
		TArray<FName> FootBoneNames;
		TArray<FName> FallbackFootBoneNames;
		for (int32 BoneIndex = 0; BoneIndex < ReferenceSkeleton.GetNum(); ++BoneIndex)
		{
			const FName BoneName = ReferenceSkeleton.GetBoneName(BoneIndex);
			if (!IsLikelyFootBone(BoneName) || IsIgnoredFootHelperBone(BoneName))
			{
				continue;
			}

			if (BoneName.ToString().ToLower().Contains(TEXT("foot")))
			{
				FootBoneNames.Add(BoneName);
			}
			else
			{
				FallbackFootBoneNames.Add(BoneName);
			}
		}

		if (FootBoneNames.Num() == 0)
		{
			FootBoneNames = MoveTemp(FallbackFootBoneNames);
		}

		TArray<TArray<FTransform>> FootTransformSets;
		for (const FName BoneName : FootBoneNames)
		{
			TArray<FTransform>& FootTransforms = FootTransformSets.AddDefaulted_GetRef();
			if (!GetComponentSpaceTransforms(DataModel, ReferenceSkeleton, BoneName, RootBoneName, RootTransforms, NumKeys, FootTransforms))
			{
				FootTransformSets.Pop(EAllowShrinking::No);
			}
		}

		OutRootLocations.Reset(NumKeys);
		if (FootTransformSets.Num() == 0 || RootTransforms.Num() != NumKeys || NumKeys <= 0)
		{
			return false;
		}

		float LowestFootZ = TNumericLimits<float>::Max();
		for (const TArray<FTransform>& FootTransforms : FootTransformSets)
		{
			for (const FTransform& FootTransform : FootTransforms)
			{
				LowestFootZ = FMath::Min(LowestFootZ, static_cast<float>(FootTransform.GetTranslation().Z));
			}
		}

		FVector AccumulatedRootOffset = FVector::ZeroVector;
		int32 PreviousPlantedFootIndex = INDEX_NONE;
		OutRootLocations.Add(RootTransforms[0].GetTranslation());
		for (int32 KeyIndex = 1; KeyIndex < NumKeys; ++KeyIndex)
		{
			int32 PlantedFootIndex = INDEX_NONE;
			float BestFootZ = TNumericLimits<float>::Max();
			for (int32 FootIndex = 0; FootIndex < FootTransformSets.Num(); ++FootIndex)
			{
				const float FootZ = static_cast<float>(FootTransformSets[FootIndex][KeyIndex].GetTranslation().Z);
				if (FootZ < BestFootZ)
				{
					BestFootZ = FootZ;
					PlantedFootIndex = FootIndex;
				}
			}

			if (PlantedFootIndex != INDEX_NONE && BestFootZ <= LowestFootZ + FootPlantHeightTolerance)
			{
				if (PreviousPlantedFootIndex == PlantedFootIndex)
				{
					FVector PlantDelta = FootTransformSets[PlantedFootIndex][KeyIndex].GetTranslation()
						- FootTransformSets[PlantedFootIndex][KeyIndex - 1].GetTranslation();
					PlantDelta.Z = 0.0;
					if (!PlantDelta.IsNearlyZero(FootPlantMinDelta))
					{
						AccumulatedRootOffset -= PlantDelta;
					}
				}

				PreviousPlantedFootIndex = PlantedFootIndex;
			}
			else
			{
				PreviousPlantedFootIndex = INDEX_NONE;
			}

			FVector RootLocation = RootTransforms[KeyIndex].GetTranslation();
			RootLocation.X = RootTransforms[0].GetTranslation().X + AccumulatedRootOffset.X;
			RootLocation.Y = RootTransforms[0].GetTranslation().Y + AccumulatedRootOffset.Y;
			OutRootLocations.Add(RootLocation);
		}

		if (bUseConstantSpeed)
		{
			ApplyMaxSpeedHorizontalDistance(OutRootLocations, Duration, SpeedScale);
			SnapHorizontalRootDirection(OutRootLocations, DirectionSnapDegrees);
			LinearizeHorizontalRootLocations(OutRootLocations);
		}
		else
		{
			SnapHorizontalRootDirection(OutRootLocations, DirectionSnapDegrees);
			SmoothHorizontalRootLocations(OutRootLocations);
		}
		return OutRootLocations.Num() == NumKeys
			&& !FVector(OutRootLocations.Last().X - OutRootLocations[0].X, OutRootLocations.Last().Y - OutRootLocations[0].Y, 0.0).IsNearlyZero(1.0);
	}

	bool ShouldUseFootPlantExtraction(FName SourceBoneName, const TArray<FTransform>& SourceComponentTransforms)
	{
		return IsLikelyFootBone(SourceBoneName) || !HasMeaningfulNetHorizontalMotion(SourceComponentTransforms);
	}

	void SetSourceLocalTransformFromComponentSpace(
		const IAnimationDataModel* DataModel,
		const FReferenceSkeleton& ReferenceSkeleton,
		FName SourceBoneName,
		FName RootBoneName,
		const TArray<FTransform>& RootTransforms,
		int32 KeyIndex,
		const FTransform& SourceComponentTransform,
		FTransform& OutSourceLocalTransform)
	{
		TArray<FName> BoneChain;
		if (!BuildBoneChain(ReferenceSkeleton, SourceBoneName, BoneChain) || BoneChain.Num() <= 1)
		{
			OutSourceLocalTransform = SourceComponentTransform;
			return;
		}

		const FTransform ParentComponentTransform = BuildComponentSpaceTransformAtKey(
			DataModel,
			ReferenceSkeleton,
			BoneChain,
			RootBoneName,
			RootTransforms,
			KeyIndex,
			BoneChain.Num() - 2);
		OutSourceLocalTransform = SourceComponentTransform.GetRelativeTransform(ParentComponentTransform);
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
		ResultSequence->bForceRootLock = false;
		ResultSequence->RootMotionRootLock = ERootMotionRootLock::AnimFirstFrame;
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
	bool bUseConstantSpeedFootPlantRootMotion,
	float FootPlantMaxSpeedScale,
	float FootPlantDirectionSnapDegrees,
	bool bEnableRootMotionOnResult,
	const FString& AssetSuffix){
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

	TArray<FTransform> SourceComponentTransforms;
	if (!GPRootMotionExtraction::GetComponentSpaceTransforms(
		DataModel,
		ReferenceSkeleton,
		SourceBoneName,
		RootBoneName,
		RootTransforms,
		SourceBoneTransforms.Num(),
		SourceComponentTransforms))
	{
		UE_LOG(LogTemp, Warning, TEXT("ExtractRootMotionFromInPlaceAnimation failed for %s: cannot build component-space transforms for '%s'."), *SourceSequence->GetName(), *SourceBoneName.ToString());
		return nullptr;
	}

	const FTransform FirstSourceTransform = SourceComponentTransforms[0];
	const FVector FirstSourceLocation = FirstSourceTransform.GetTranslation();
	const FRotator FirstSourceRotation = FirstSourceTransform.GetRotation().Rotator();
	const float FirstSourceYaw = FirstSourceRotation.Yaw;
	TArray<FVector> FootPlantRootLocations;
	const bool bWantsFootPlantExtraction = bExtractHorizontalTranslation
		&& GPRootMotionExtraction::ShouldUseFootPlantExtraction(SourceBoneName, SourceComponentTransforms);
	bool bUseFootPlantExtraction = false;
	bool bUsedAutoFootPlantExtraction = false;
	if (bWantsFootPlantExtraction && GPRootMotionExtraction::IsLikelyFootBone(SourceBoneName))
	{
		bUseFootPlantExtraction = GPRootMotionExtraction::BuildFootPlantRootLocations(
			SourceComponentTransforms,
			RootTransforms,
			bUseConstantSpeedFootPlantRootMotion,
			ResultSequence->GetPlayLength(),
			FootPlantMaxSpeedScale,
			FootPlantDirectionSnapDegrees,
			FootPlantRootLocations);
	}
	if (bWantsFootPlantExtraction && !bUseFootPlantExtraction)
	{
		bUseFootPlantExtraction = GPRootMotionExtraction::BuildAutoFootPlantRootLocations(
			DataModel,
			ReferenceSkeleton,
			RootBoneName,
			RootTransforms,
			SourceBoneTransforms.Num(),
			bUseConstantSpeedFootPlantRootMotion,
			ResultSequence->GetPlayLength(),
			FootPlantMaxSpeedScale,
			FootPlantDirectionSnapDegrees,
			FootPlantRootLocations);
		bUsedAutoFootPlantExtraction = bUseFootPlantExtraction;
	}
	if (bWantsFootPlantExtraction && !bUseFootPlantExtraction)
	{
		UE_LOG(LogTemp, Warning, TEXT("Root motion extraction for %s wanted foot-plant fallback, but no usable foot root path was inferred. Falling back to '%s' source motion."),
			*SourceSequence->GetName(),
			*SourceBoneName.ToString());
	}

	for (int32 KeyIndex = 0; KeyIndex < SourceBoneTransforms.Num(); ++KeyIndex)
	{
		FTransform& RootTransform = RootTransforms[KeyIndex];
		FTransform& SourceTransform = SourceBoneTransforms[KeyIndex];
		FTransform SourceComponentTransform = SourceComponentTransforms[KeyIndex];

		if (bUseFootPlantExtraction)
		{
			FVector RootLocation = RootTransform.GetTranslation();
			RootLocation.X = FootPlantRootLocations[KeyIndex].X;
			RootLocation.Y = FootPlantRootLocations[KeyIndex].Y;
			RootTransform.SetTranslation(RootLocation);

			if (bZeroMotionSourceHorizontalTranslation)
			{
				FVector NewSourceLocation = SourceComponentTransform.GetTranslation();
				NewSourceLocation.X = FirstSourceLocation.X;
				NewSourceLocation.Y = FirstSourceLocation.Y;
				SourceComponentTransform.SetTranslation(NewSourceLocation);
			}
		}
		else if (bExtractHorizontalTranslation)
		{
			const FVector SourceLocation = SourceComponentTransform.GetTranslation();
			FVector RootLocation = RootTransform.GetTranslation();
			RootLocation.X += SourceLocation.X - FirstSourceLocation.X;
			RootLocation.Y += SourceLocation.Y - FirstSourceLocation.Y;
			RootTransform.SetTranslation(RootLocation);

			if (bZeroMotionSourceHorizontalTranslation)
			{
				FVector NewSourceLocation = SourceLocation;
				NewSourceLocation.X = FirstSourceLocation.X;
				NewSourceLocation.Y = FirstSourceLocation.Y;
				SourceComponentTransform.SetTranslation(NewSourceLocation);
			}
		}

		if (bExtractYawRotation)
		{
			const FRotator SourceRotator = SourceComponentTransform.GetRotation().Rotator();
			const float DeltaYaw = FRotator::NormalizeAxis(SourceRotator.Yaw - FirstSourceYaw);
			const FQuat RootYawDelta = FRotator(0.0f, DeltaYaw, 0.0f).Quaternion();
			RootTransform.SetRotation(RootYawDelta * RootTransform.GetRotation());

			FRotator NewSourceRotator = SourceRotator;
			NewSourceRotator.Yaw = FirstSourceYaw;
			SourceComponentTransform.SetRotation(NewSourceRotator.Quaternion());
		}

		if (bZeroMotionSourceHorizontalTranslation || bExtractYawRotation)
		{
			GPRootMotionExtraction::SetSourceLocalTransformFromComponentSpace(
				DataModel,
				ReferenceSkeleton,
				SourceBoneName,
				RootBoneName,
				RootTransforms,
				KeyIndex,
				SourceComponentTransform,
				SourceTransform);
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

	UE_LOG(LogTemp, Display, TEXT("Extracted root motion: %s -> %s using source bone '%s'. rootNet2D=%.2f rootPath2D=%.2f sourceNet2D=%.2f zeroSourceXY=%d"),
		*SourceSequence->GetName(),
		*ResultSequence->GetName(),
		*SourceBoneName.ToString(),
		GPRootMotionExtraction::GetNetHorizontalDistance(RootTransforms),
		GPRootMotionExtraction::GetPathHorizontalDistance(RootTransforms),
		GPRootMotionExtraction::GetNetHorizontalDistance(SourceComponentTransforms),
		bZeroMotionSourceHorizontalTranslation ? 1 : 0);
	if (bUseFootPlantExtraction)
	{
		const float InferredSpeed = ResultSequence->GetPlayLength() > 0.0f
			? GPRootMotionExtraction::GetNetHorizontalDistance(FootPlantRootLocations) / ResultSequence->GetPlayLength()
			: 0.0f;
		const float MaxSampledSpeed = GPRootMotionExtraction::GetMaxHorizontalSpeed(FootPlantRootLocations, ResultSequence->GetPlayLength());
		UE_LOG(LogTemp, Display, TEXT("Root motion extraction used %s foot-plant inference for source bone '%s'. inferredNet2D=%.2f inferredSpeed=%.2f maxSampledSpeed=%.2f constantSpeed=%d speedScale=%.2f directionSnap=%.2f smoothingPasses=%d"),
			bUsedAutoFootPlantExtraction ? TEXT("auto") : TEXT("source-bone"),
			*SourceBoneName.ToString(),
			GPRootMotionExtraction::GetNetHorizontalDistance(FootPlantRootLocations),
			InferredSpeed,
			MaxSampledSpeed,
			bUseConstantSpeedFootPlantRootMotion ? 1 : 0,
			FootPlantMaxSpeedScale,
			FootPlantDirectionSnapDegrees,
			bUseConstantSpeedFootPlantRootMotion ? 0 : GPRootMotionExtraction::FootPlantSmoothingPasses);
	}
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
	bool bUseConstantSpeedFootPlantRootMotion,
	float FootPlantMaxSpeedScale,
	float FootPlantDirectionSnapDegrees,
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
			bUseConstantSpeedFootPlantRootMotion,
			FootPlantMaxSpeedScale,
			FootPlantDirectionSnapDegrees,
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
	bool bUseConstantSpeedFootPlantRootMotion,
	float FootPlantMaxSpeedScale,
	float FootPlantDirectionSnapDegrees,
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

	TArray<FTransform> SourceComponentTransforms;
	if (!GPRootMotionExtraction::GetComponentSpaceTransforms(
		DataModel,
		ReferenceSkeleton,
		SourceBoneName,
		RootBoneName,
		RootTransforms,
		SourceBoneTransforms.Num(),
		SourceComponentTransforms))
	{
		UE_LOG(LogTemp, Warning, TEXT("ExtractRootMotionFromDirectionalDodgeAnimation failed for %s: cannot build component-space transforms for '%s'."), *SourceSequence->GetName(), *SourceBoneName.ToString());
		return nullptr;
	}

	TArray<FVector> FootPlantRootLocations;
	const bool bWantsFootPlantExtraction = GPRootMotionExtraction::ShouldUseFootPlantExtraction(SourceBoneName, SourceComponentTransforms);
	bool bUseFootPlantExtraction = false;
	bool bUsedAutoFootPlantExtraction = false;
	if (bWantsFootPlantExtraction && GPRootMotionExtraction::IsLikelyFootBone(SourceBoneName))
	{
		bUseFootPlantExtraction = GPRootMotionExtraction::BuildFootPlantRootLocations(
			SourceComponentTransforms,
			RootTransforms,
			bUseConstantSpeedFootPlantRootMotion,
			ResultSequence->GetPlayLength(),
			FootPlantMaxSpeedScale,
			FootPlantDirectionSnapDegrees,
			FootPlantRootLocations);
	}
	if (bWantsFootPlantExtraction && !bUseFootPlantExtraction)
	{
		bUseFootPlantExtraction = GPRootMotionExtraction::BuildAutoFootPlantRootLocations(
			DataModel,
			ReferenceSkeleton,
			RootBoneName,
			RootTransforms,
			SourceBoneTransforms.Num(),
			bUseConstantSpeedFootPlantRootMotion,
			ResultSequence->GetPlayLength(),
			FootPlantMaxSpeedScale,
			FootPlantDirectionSnapDegrees,
			FootPlantRootLocations);
		bUsedAutoFootPlantExtraction = bUseFootPlantExtraction;
	}
	if (bWantsFootPlantExtraction && !bUseFootPlantExtraction)
	{
		UE_LOG(LogTemp, Warning, TEXT("Directional dodge extraction for %s wanted foot-plant fallback, but no usable foot root path was inferred. Falling back to '%s' source motion."),
			*SourceSequence->GetName(),
			*SourceBoneName.ToString());
	}

	FVector InitialDirection = FVector::ZeroVector;
	if (bUseFootPlantExtraction)
	{
		const FVector FootPlantOffset = FootPlantRootLocations.Last() - FootPlantRootLocations[0];
		InitialDirection = FVector(FootPlantOffset.X, FootPlantOffset.Y, 0.0).GetSafeNormal();
	}
	else
	{
		for (int32 KeyIndex = 1; KeyIndex < SourceComponentTransforms.Num(); ++KeyIndex)
		{
			const FVector Delta = SourceComponentTransforms[KeyIndex].GetTranslation() - SourceComponentTransforms[KeyIndex - 1].GetTranslation();
			const FVector Delta2D(Delta.X, Delta.Y, 0.0);
			if (!Delta2D.IsNearlyZero(0.01))
			{
				InitialDirection = Delta2D.GetSafeNormal();
				break;
			}
		}
	}

	if (InitialDirection.IsNearlyZero())
	{
		UE_LOG(LogTemp, Warning, TEXT("ExtractRootMotionFromDirectionalDodgeAnimation failed for %s: could not detect initial movement direction from '%s'."), *SourceSequence->GetName(), *SourceBoneName.ToString());
		return nullptr;
	}

	const FVector FirstSourceLocation = SourceComponentTransforms[0].GetTranslation();
	const FVector FirstRootLocation = RootTransforms[0].GetTranslation();
	float AccumulatedDistance = 0.0f;

	for (int32 KeyIndex = 0; KeyIndex < SourceComponentTransforms.Num(); ++KeyIndex)
	{
		if (bUseFootPlantExtraction)
		{
			FVector RootLocation = RootTransforms[KeyIndex].GetTranslation();
			RootLocation.X = FootPlantRootLocations[KeyIndex].X;
			RootLocation.Y = FootPlantRootLocations[KeyIndex].Y;
			RootTransforms[KeyIndex].SetTranslation(RootLocation);
		}
		else
		{
			if (KeyIndex > 0)
			{
				const FVector Delta = SourceComponentTransforms[KeyIndex].GetTranslation() - SourceComponentTransforms[KeyIndex - 1].GetTranslation();
				AccumulatedDistance += FMath::Abs(FVector::DotProduct(FVector(Delta.X, Delta.Y, 0.0), InitialDirection));
			}

			FVector RootLocation = RootTransforms[KeyIndex].GetTranslation();
			const FVector DirectedOffset = InitialDirection * AccumulatedDistance;
			RootLocation.X = FirstRootLocation.X + DirectedOffset.X;
			RootLocation.Y = FirstRootLocation.Y + DirectedOffset.Y;
			RootTransforms[KeyIndex].SetTranslation(RootLocation);
		}

		if (bZeroMotionSourceHorizontalTranslation)
		{
			FTransform SourceComponentTransform = SourceComponentTransforms[KeyIndex];
			FVector SourceLocation = SourceComponentTransform.GetTranslation();
			SourceLocation.X = FirstSourceLocation.X;
			SourceLocation.Y = FirstSourceLocation.Y;
			SourceComponentTransform.SetTranslation(SourceLocation);
			GPRootMotionExtraction::SetSourceLocalTransformFromComponentSpace(
				DataModel,
				ReferenceSkeleton,
				SourceBoneName,
				RootBoneName,
				RootTransforms,
				KeyIndex,
				SourceComponentTransform,
				SourceBoneTransforms[KeyIndex]);
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
	UE_LOG(LogTemp, Display, TEXT("Directional dodge root distances: rootNet2D=%.2f rootPath2D=%.2f sourceNet2D=%.2f zeroSourceXY=%d"),
		GPRootMotionExtraction::GetNetHorizontalDistance(RootTransforms),
		GPRootMotionExtraction::GetPathHorizontalDistance(RootTransforms),
		GPRootMotionExtraction::GetNetHorizontalDistance(SourceComponentTransforms),
		bZeroMotionSourceHorizontalTranslation ? 1 : 0);
	if (bUseFootPlantExtraction)
	{
		const float InferredSpeed = ResultSequence->GetPlayLength() > 0.0f
			? GPRootMotionExtraction::GetNetHorizontalDistance(FootPlantRootLocations) / ResultSequence->GetPlayLength()
			: 0.0f;
		const float MaxSampledSpeed = GPRootMotionExtraction::GetMaxHorizontalSpeed(FootPlantRootLocations, ResultSequence->GetPlayLength());
		UE_LOG(LogTemp, Display, TEXT("Directional dodge extraction used %s foot-plant inference for source bone '%s'. inferredNet2D=%.2f inferredSpeed=%.2f maxSampledSpeed=%.2f constantSpeed=%d speedScale=%.2f directionSnap=%.2f smoothingPasses=%d"),
			bUsedAutoFootPlantExtraction ? TEXT("auto") : TEXT("source-bone"),
			*SourceBoneName.ToString(),
			GPRootMotionExtraction::GetNetHorizontalDistance(FootPlantRootLocations),
			InferredSpeed,
			MaxSampledSpeed,
			bUseConstantSpeedFootPlantRootMotion ? 1 : 0,
			FootPlantMaxSpeedScale,
			FootPlantDirectionSnapDegrees,
			bUseConstantSpeedFootPlantRootMotion ? 0 : GPRootMotionExtraction::FootPlantSmoothingPasses);
	}
	return ResultSequence;
#else
	return nullptr;
#endif
}

TArray<UAnimSequence*> UGP_RootMotionExtractionEditorLibrary::ExtractRootMotionFromDirectionalDodgeAnimations(
	const TArray<UAnimSequence*>& SourceSequences,
	FName MotionSourceBoneName,
	bool bZeroMotionSourceHorizontalTranslation,
	bool bUseConstantSpeedFootPlantRootMotion,
	float FootPlantMaxSpeedScale,
	float FootPlantDirectionSnapDegrees,
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
			bUseConstantSpeedFootPlantRootMotion,
			FootPlantMaxSpeedScale,
			FootPlantDirectionSnapDegrees,
			bEnableRootMotionOnResult,
			AssetSuffix))
		{
			Results.Add(Result);
		}
	}

	return Results;
}
