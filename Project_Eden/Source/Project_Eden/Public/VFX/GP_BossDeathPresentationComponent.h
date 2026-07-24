#pragma once

#include "CoreMinimal.h"
#include "Actors/GP_BossDeathPresentationActor.h"
#include "Components/SceneComponent.h"
#include "GP_BossDeathPresentationComponent.generated.h"

class AGP_EnemyCharacter;

/** Boss-owned component that maps a GAS death into a local cinematic clear presentation. */
UCLASS(ClassGroup = (Boss), meta = (BlueprintSpawnableComponent))
class PROJECT_EDEN_API UGP_BossDeathPresentationComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UGP_BossDeathPresentationComponent();

	UFUNCTION(BlueprintCallable, Category = "Boss|Death Presentation")
	bool PlayDeathPresentation(AActor* InstigatorActor);

	UFUNCTION(BlueprintPure, Category = "Boss|Death Presentation")
	EGPBossDeathPresentationStyle ResolvePresentationStyle() const;

	UFUNCTION(BlueprintPure, Category = "Boss|Death Presentation")
	EGPBossDeathPresentationStyle GetConfiguredPresentationStyle() const { return PresentationStyle; }

	void SetPresentationStyle(EGPBossDeathPresentationStyle NewStyle) { PresentationStyle = NewStyle; }

	UFUNCTION(BlueprintPure, Category = "Boss|Death Presentation")
	bool HasPlayedDeathPresentation() const { return bPresentationPlayed; }

	UFUNCTION(BlueprintCallable, Category = "Boss|Death Presentation|Materials")
	void ConfigureFragmentMaterial(UMaterialInterface* InFragmentMaterial, bool bInHideSourceMesh);

	UFUNCTION(BlueprintPure, Category = "Boss|Death Presentation|Materials")
	UMaterialInterface* GetFragmentMaterial() const { return SpawnSettings.FragmentMaterial; }

	UFUNCTION(BlueprintPure, Category = "Boss|Death Presentation")
	bool DoesPresentationHideSourceMesh() const { return SpawnSettings.bHideSourceMesh; }

	UFUNCTION(BlueprintPure, Category = "Boss|Death Presentation")
	static EGPBossDeathPresentationStyle ResolveAutoPresentationStyleFromName(const FString& OwnerName, const FText& BossDisplayName);

private:
	bool CanPlayForOwner(const AGP_EnemyCharacter* EnemyOwner) const;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Death Presentation", meta = (AllowPrivateAccess = "true"))
	bool bEnableDeathPresentation = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Death Presentation", meta = (AllowPrivateAccess = "true"))
	EGPBossDeathPresentationStyle PresentationStyle = EGPBossDeathPresentationStyle::Auto;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Death Presentation", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AGP_BossDeathPresentationActor> PresentationActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Death Presentation", meta = (AllowPrivateAccess = "true"))
	FGPBossDeathPresentationSpawnSettings SpawnSettings;

	bool bPresentationPlayed = false;
};
