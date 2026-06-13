#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GP_FabFenceActor.generated.h"

class UInstancedStaticMeshComponent;

UCLASS()
class PROJECT_EDEN_API AGP_FabFenceActor : public AActor
{
	GENERATED_BODY()

public:
	AGP_FabFenceActor();

	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Fence")
	void RebuildFence();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fence")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fence")
	TObjectPtr<UInstancedStaticMeshComponent> NewelPostISM;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fence")
	TObjectPtr<UInstancedStaticMeshComponent> BalusterISM;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fence")
	TObjectPtr<UInstancedStaticMeshComponent> HandrailISM;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fence", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "cm"))
	float Length = 460.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fence", meta = (ClampMin = "1.0", UIMin = "1.0", Units = "cm"))
	float BalusterSpacing = 40.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fence", meta = (ClampMin = "1.0", UIMin = "1.0", Units = "cm"))
	float HandrailSpacing = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fence", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "cm"))
	float BalusterStartOffset = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fence", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "cm"))
	float BalusterEndOffset = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fence", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "cm"))
	float HandrailStartOffset = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fence", meta = (Units = "cm"))
	float HandrailEndOverhang = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fence")
	bool bUseScaleYAsLength = true;

private:
	void AddInstancesAlongY(UInstancedStaticMeshComponent* Component, float StartY, float EndY, float Spacing) const;
};
