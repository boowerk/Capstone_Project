#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GP_Summonable.generated.h"

UINTERFACE(MinimalAPI, NotBlueprintable)
class UGP_Summonable : public UInterface
{
	GENERATED_BODY()
};

/**
 * IGP_Summonable
 * 소환수 즉 플레이어가 소환한 모든 객체가 공통으로 가질 규약
 * 일부 장판스킬이나 지속형 투사체도 묶을 예정 - 슝민
 */
class PROJECT_EDEN_API IGP_Summonable
{
	GENERATED_BODY()

public:
	// 소환물의 주인 플레이어 리턴
	virtual AActor* GetSummonOwner() const = 0;

	// 물속성 서포터 E 스킬 패시브 발동 시 호출될 장판 당겨오기 명령
	UFUNCTION(BlueprintCallable, Category = "Eden|Summon")
	virtual void CommandPullTowards(AActor* TargetActor, float PullSpeed) = 0;

	// 공격 명령
	UFUNCTION(BlueprintCallable, Category = "Eden|Summon")
	virtual void CommandAttackTarget() = 0;
};
