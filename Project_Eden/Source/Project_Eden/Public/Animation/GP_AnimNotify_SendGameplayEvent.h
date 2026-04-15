#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "GP_AnimNotify_SendGameplayEvent.generated.h"

UCLASS()
class PROJECT_EDEN_API UGP_AnimNotify_SendGameplayEvent : public UAnimNotify
{
	GENERATED_BODY()

public:
	// 몽타주/애니메이션에서 소유 액터에게 보낼 Gameplay Event 태그다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay Event")
	FGameplayTag GameplayEventTag;

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
	virtual FString GetNotifyName_Implementation() const override;
};
