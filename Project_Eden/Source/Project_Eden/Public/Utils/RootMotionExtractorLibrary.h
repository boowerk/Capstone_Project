#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RootMotionExtractorLibrary.generated.h"

UENUM(BlueprintType)
enum class ERootMotionPlanarAxis : uint8
{
	PosX UMETA(DisplayName = "+X"),
	NegX UMETA(DisplayName = "-X"),
	PosY UMETA(DisplayName = "+Y"),
	NegY UMETA(DisplayName = "-Y")
};

UCLASS()
class PROJECT_EDEN_API URootMotionExtractorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Editor Utilities | Animation")
	static void BatchNormalizeRootPose(FString Suffix = TEXT("_RootNormalized"));

	UFUNCTION(BlueprintCallable, Category = "Editor Utilities | Animation")
	static void BatchExtractRootMotionFromFeet(
		FString Suffix = TEXT("_RM"),
		FName LeftFootBone = TEXT("foot_l"),
		FName RightFootBone = TEXT("foot_r"),
		bool bInvertForward = false,
		bool bInvertLateral = false,
		bool bIncludeZ = false,
		ERootMotionPlanarAxis SourceForwardAxis = ERootMotionPlanarAxis::PosY,
		ERootMotionPlanarAxis ForwardAxis = ERootMotionPlanarAxis::PosY,
		ERootMotionPlanarAxis LateralAxis = ERootMotionPlanarAxis::NegX
	);

private:
	static void ProcessRootPoseNormalization(class UAnimSequence* AnimSeq);
	static void ProcessRelayAnchorInference(
		class UAnimSequence* AnimSeq,
		FName LeftFoot,
		FName RightFoot,
		bool bInvertForward,
		bool bInvertLateral,
		bool bIncludeZ,
		ERootMotionPlanarAxis SourceForwardAxis,
		ERootMotionPlanarAxis ForwardAxis,
		ERootMotionPlanarAxis LateralAxis
	);
};
