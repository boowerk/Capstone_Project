// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GP_BlueprintLibrary.generated.h"

class UGP_SkillData;
class UGameplayEffect;

UENUM(BlueprintType)
enum class EHitDirection : uint8 {
	Left,
	Right,
	Forward,
	Back
};
/**
 * UGP_BlueprintLibrary
 * 블루프린트 범용 유틸리티 함수 모음
 * 이펙트 적용이나 어트리뷰트 변동을 BP로 임시로 BP로 구현하려고 만들었음
 * 코드를 조금이나마 깔끔하고 가독성을 높이게 만드는 다른 용도도 추가됨 - 슝민
 */
UCLASS()
class PROJECT_EDEN_API UGP_BlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	// 피격 방향 계산
	UFUNCTION(BlueprintPure, Category = "Eden|Combat")
	static EHitDirection GetHitDirection(const FVector& TargetForward, const FVector& ToInstigator); // 피격 방향을 계산

	// 방향이름 받아오기
	UFUNCTION(BlueprintPure, Category = "Eden|Combat")
	static FName GetHitDirectionName(const EHitDirection& HitDirection); 
	
	// 구체 피격 검사
	UFUNCTION(BlueprintCallable, Category = "Eden|Combat") 
	static TArray<AActor*> SphereMeleeHitBoxOverlap(AActor* AvatarActor, float Radius, float ForwardOffset, float ElevationOffset, bool bDrawDebug = false);

	// 지정 위치 기준 구체 피격 검사
	UFUNCTION(BlueprintCallable, Category = "Eden|Combat", meta = (WorldContext = "WorldContextObject"))
	static TArray<AActor*> SphereOverlapActorsAtLocation(UObject* WorldContextObject, const FVector& Location, float Radius, AActor* ActorToIgnore, bool bDrawDebug = false);

	// 지정 위치/회전 기준 박스 피격 검사
	UFUNCTION(BlueprintCallable, Category = "Eden|Combat", meta = (WorldContext = "WorldContextObject"))
	static TArray<AActor*> BoxOverlapActorsAtLocation(UObject* WorldContextObject, const FVector& Location, const FVector& BoxExtent, const FRotator& Rotation, AActor* ActorToIgnore, bool bDrawDebug = false);
	
	
	// GAS 유틸 
	
	// 액터배열에 게임플레이 이벤트 전송
	UFUNCTION(BlueprintCallable, Category = "Eden|Combat|Abilities")
	static void SendGameplayEventToActors(AActor* Instigator, const TArray<AActor*>& TargetActors, FGameplayTag EventTag);
	
	// 액터배열에 게임플레이 이펙트 일괄 적용
	UFUNCTION(BlueprintCallable, Category = "Eden|Combat|Abilities")
	static void ApplyGameplayEffectToActors(AActor* Instigator, const TArray<AActor*>& TargetActors, TSubclassOf<UGameplayEffect> EffectClass, float EffectLevel = 1.0f, UGP_SkillData* SkillData = nullptr);

	// 액터배열에 게임플레이 이펙트 적용 후 이벤트 전송
	UFUNCTION(BlueprintCallable, Category = "Eden|Combat|Abilities")
	static void ApplyGameplayEffectAndEventToActors(AActor* Instigator, const TArray<AActor*>& TargetActors, TSubclassOf<UGameplayEffect> EffectClass, FGameplayTag EventTag, float EffectLevel = 1.0f, UGP_SkillData* SkillData = nullptr);

	// 지정 위치 기준 범위 안의 액터들에게 게임플레이 이펙트 적용
	UFUNCTION(BlueprintCallable, Category = "Eden|Combat|Abilities")
	static void ApplyAreaGameplayEffectAtLocation(AActor* Instigator, const FVector& Location, float Radius, TSubclassOf<UGameplayEffect> EffectClass, float EffectLevel = 1.0f, UGP_SkillData* SkillData = nullptr, bool bDrawDebug = false);
};
