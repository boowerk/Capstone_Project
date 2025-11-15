#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PDA_PCGItemGroup.h"
#include "PDA_Biome.generated.h"


UCLASS()
class PROJECT_EDEN_API UPDA_Biome : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Biome")
    FString BiomeName = "DefaultBiome";

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Biome|Item Groups")
    TSoftObjectPtr<UPDA_PCGItemGroup> TreeGroup;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Biome|Item Groups")
    TSoftObjectPtr<UPDA_PCGItemGroup> RockGroup;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Biome|Item Groups")
    TSoftObjectPtr<UPDA_PCGItemGroup> BushGroup;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Biome|Visuals")
    FLinearColor FogColor = FLinearColor::White;

};