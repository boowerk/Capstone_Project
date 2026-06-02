#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "GP_RootMotionExtractionEditorLibrary.generated.h"

class UAnimSequence;

UCLASS()
class PROJECT_EDEN_API UGP_RootMotionExtractionEditorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "ProjectEden|Animation|Editor", meta = (DevelopmentOnly))
	static UAnimSequence* ExtractRootMotionFromInPlaceAnimation(
		UAnimSequence* SourceSequence,
		FName MotionSourceBoneName = TEXT("pelvis"),
		bool bExtractHorizontalTranslation = true,
		bool bExtractYawRotation = false,
		bool bZeroMotionSourceHorizontalTranslation = true,
		bool bEnableRootMotionOnResult = true,
		const FString& AssetSuffix = TEXT("_RM"));

	UFUNCTION(BlueprintCallable, Category = "ProjectEden|Animation|Editor", meta = (DevelopmentOnly))
	static TArray<UAnimSequence*> ExtractRootMotionFromInPlaceAnimations(
		const TArray<UAnimSequence*>& SourceSequences,
		FName MotionSourceBoneName = TEXT("pelvis"),
		bool bExtractHorizontalTranslation = true,
		bool bExtractYawRotation = false,
		bool bZeroMotionSourceHorizontalTranslation = true,
		bool bEnableRootMotionOnResult = true,
		const FString& AssetSuffix = TEXT("_RM"));

	UFUNCTION(BlueprintCallable, Category = "ProjectEden|Animation|Editor", meta = (DevelopmentOnly))
	static UAnimSequence* ExtractRootMotionFromDirectionalDodgeAnimation(
		UAnimSequence* SourceSequence,
		FName MotionSourceBoneName = TEXT("pelvis"),
		bool bZeroMotionSourceHorizontalTranslation = true,
		bool bEnableRootMotionOnResult = true,
		const FString& AssetSuffix = TEXT("_DodgeRM"));

	UFUNCTION(BlueprintCallable, Category = "ProjectEden|Animation|Editor", meta = (DevelopmentOnly))
	static TArray<UAnimSequence*> ExtractRootMotionFromDirectionalDodgeAnimations(
		const TArray<UAnimSequence*>& SourceSequences,
		FName MotionSourceBoneName = TEXT("pelvis"),
		bool bZeroMotionSourceHorizontalTranslation = true,
		bool bEnableRootMotionOnResult = true,
		const FString& AssetSuffix = TEXT("_DodgeRM"));
};
