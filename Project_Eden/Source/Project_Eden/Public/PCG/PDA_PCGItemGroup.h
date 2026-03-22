#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PCGStructs.h" 
#include "PDA_PCGItemGroup.generated.h"


 // 아이템들의 그룹을 정의하는 PDA
UCLASS(BlueprintType)
class PROJECT_EDEN_API UPDA_PCGItemGroup : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG Item Group")
    TArray<FPCGItemDetails> Items;
};