#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GP_MatadorBossStateComponent.generated.h"

class UAbilitySystemComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGPMatadorChainStageChangedSignature, int32, ChainBreakCount, int32, ChainBreakTarget);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGPMatadorGroggyChangedSignature, bool, bIsGroggy);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECT_EDEN_API UGP_MatadorBossStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGP_MatadorBossStateComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "Boss|Matador")
	void InitializeMatadorState(AActor* InMainBossActor);

	UFUNCTION(BlueprintCallable, Category = "Boss|Matador")
	void RegisterDecoyActor(AActor* InDecoyActor);

	UFUNCTION(BlueprintCallable, Category = "Boss|Matador")
	void RegisterChainEffectActor(AActor* InChainEffectActor);

	UFUNCTION(BlueprintCallable, Category = "Boss|Matador")
	void RegisterActiveBullActor(AActor* InBullActor);

	UFUNCTION(BlueprintCallable, Category = "Boss|Matador")
	bool TryRedirectActiveBullTowardDecoy(AActor* RedirectingActor);

	UFUNCTION(BlueprintCallable, Category = "Boss|Matador")
	void RecordBullHitDecoy();

	UFUNCTION(BlueprintCallable, Category = "Boss|Matador")
	void SetChainBreakCount(int32 NewCount);

	UFUNCTION(BlueprintCallable, Category = "Boss|Matador")
	void ResetChainBreakCount();

	UFUNCTION(BlueprintCallable, Category = "Boss|Matador")
	void EnterGroggy();

	UFUNCTION(BlueprintCallable, Category = "Boss|Matador")
	void RecoverFromGroggy();

	UFUNCTION(BlueprintPure, Category = "Boss|Matador")
	int32 GetChainBreakCount() const { return ChainBreakCount; }

	UFUNCTION(BlueprintPure, Category = "Boss|Matador")
	int32 GetChainBreakTarget() const { return FMath::Max(1, ChainBreakTarget); }

	UFUNCTION(BlueprintPure, Category = "Boss|Matador")
	bool IsGroggy() const { return bIsGroggy; }

	UFUNCTION(BlueprintPure, Category = "Boss|Matador")
	bool IsGuarded() const { return bIsGuarded; }

	UFUNCTION(BlueprintPure, Category = "Boss|Matador")
	AActor* GetMainBossActor() const { return MainBossActor.Get(); }

	UFUNCTION(BlueprintPure, Category = "Boss|Matador")
	AActor* GetDecoyActor() const { return DecoyActor.Get(); }

	UFUNCTION(BlueprintPure, Category = "Boss|Matador")
	AActor* GetActiveDecoyActor() const { return DecoyActor.Get(); }

	UFUNCTION(BlueprintPure, Category = "Boss|Matador")
	AActor* GetChainEffectActor() const { return ChainEffectActor.Get(); }

	UFUNCTION(BlueprintPure, Category = "Boss|Matador")
	AActor* GetActiveBullActor() const { return ActiveBullActor.Get(); }

	UPROPERTY(BlueprintAssignable, Category = "Boss|Matador")
	FGPMatadorChainStageChangedSignature OnChainStageChanged;

	UPROPERTY(BlueprintAssignable, Category = "Boss|Matador")
	FGPMatadorGroggyChangedSignature OnGroggyChanged;

private:
	UFUNCTION()
	void OnRep_ChainBreakCount();

	UFUNCTION()
	void OnRep_IsGroggy();

	UFUNCTION()
	void OnRep_IsGuarded();

	void SetGroggyInternal(bool bNewGroggy);
	void SetGuardedInternal(bool bNewGuarded);
	void ApplyStateTags();
	UAbilitySystemComponent* ResolveOwnerASC() const;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Matador", meta = (AllowPrivateAccess = "true", ClampMin = "1"))
	int32 ChainBreakTarget = 3;

	UPROPERTY(ReplicatedUsing = OnRep_ChainBreakCount, BlueprintReadOnly, Category = "Boss|Matador", meta = (AllowPrivateAccess = "true"))
	int32 ChainBreakCount = 0;

	UPROPERTY(ReplicatedUsing = OnRep_IsGroggy, BlueprintReadOnly, Category = "Boss|Matador", meta = (AllowPrivateAccess = "true"))
	bool bIsGroggy = false;

	UPROPERTY(ReplicatedUsing = OnRep_IsGuarded, BlueprintReadOnly, Category = "Boss|Matador", meta = (AllowPrivateAccess = "true"))
	bool bIsGuarded = true;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Boss|Matador", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AActor> MainBossActor;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Boss|Matador", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AActor> DecoyActor;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Boss|Matador", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AActor> ChainEffectActor;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Boss|Matador", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AActor> ActiveBullActor;

	bool bAppliedGuardedTag = false;
	bool bAppliedGroggyTag = false;
};
