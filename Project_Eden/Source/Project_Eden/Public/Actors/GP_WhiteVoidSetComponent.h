#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "GP_WhiteVoidSetComponent.generated.h"

class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;

UCLASS(ClassGroup = (ProjectEden), meta = (BlueprintSpawnableComponent))
class PROJECT_EDEN_API UGP_WhiteVoidSetComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UGP_WhiteVoidSetComponent();

	UFUNCTION(BlueprintCallable, Category = "White Void")
	void RebuildWhiteVoidSet();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "White Void")
	FVector WhiteVoidOffset = FVector(0.0, 0.0, -10000.0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "White Void|Meshes")
	TObjectPtr<UStaticMesh> FloorMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "White Void|Meshes")
	TObjectPtr<UStaticMesh> SkySphereMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "White Void|Material")
	TObjectPtr<UMaterialInterface> WhiteMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "White Void|Material")
	TObjectPtr<UMaterialInterface> SkyMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "White Void|Layout")
	FVector FloorScale = FVector(200.0, 200.0, 1.0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "White Void|Layout")
	float FloorCenterZOffset = -50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "White Void|Layout")
	FVector SkySphereScale = FVector(100.0);

protected:
	virtual void OnRegister() override;
	virtual void OnComponentCreated() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> FloorComponent;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> SkySphereComponent;

	void EnsureChildComponents();
};
