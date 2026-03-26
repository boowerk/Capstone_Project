#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GP_GameplayAbility.h"

#include "GP_HitReact.generated.h"

/**
 * UGP_HitReact
 * 적의 피격 반응 어빌리티를 처리
 * 피격이 발생한 방향을 계산하고 캐싱
 */
UCLASS()
class PROJECT_EDEN_API UGP_HitReact : public UGP_GameplayAbility
{
	GENERATED_BODY()
public:
	
	UFUNCTION(BlueprintCallable, Category = "GAS|Abilities") // Instigator 위치 기반 방향 벡터계산
	void CacheHitDirectionVectors(AActor* Instigator);

	UPROPERTY(BlueprintReadOnly, Category = "GAS|Abilities") // 피격당할때 어빌리티 아바타 전방 벡터
	FVector AvatarForward;

	UPROPERTY(BlueprintReadOnly, Category = "GAS|Abilities") // 아바타->공격자 정규 방향 벡터
	FVector ToInstigator;
};
