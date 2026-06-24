#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GP_ChainEffectActor.generated.h"

class UGP_MatadorBossStateComponent;
class USceneComponent;

UCLASS(Blueprintable)
class PROJECT_EDEN_API AGP_ChainEffectActor : public AActor
{
	GENERATED_BODY()

public:
	AGP_ChainEffectActor();

	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "Boss|Matador")
	void InitializeChain(AActor* InMainBossActor, AActor* InDecoyActor, UGP_MatadorBossStateComponent* InStateComponent);

	UFUNCTION(BlueprintCallable, Category = "Boss|Matador")
	void SetChainStage(int32 NewStage);

	UFUNCTION(BlueprintPure, Category = "Boss|Matador")
	int32 GetChainStage() const { return ChainStage; }

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Matador")
	void BP_OnChainStageChanged(int32 NewStage);

private:
	void DrawChainPreview() const;
	FLinearColor ResolveStageColor() const;
	float ResolveStageThickness() const;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Matador", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Boss|Matador", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AActor> MainBossActor;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Boss|Matador", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AActor> DecoyActor;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Boss|Matador", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGP_MatadorBossStateComponent> MatadorStateComponent;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Boss|Matador", meta = (AllowPrivateAccess = "true"))
	int32 ChainStage = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Matador")
	TArray<FLinearColor> StageColors;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Matador")
	float BaseLineThickness = 5.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Matador")
	float BrokenLineThicknessBonus = 4.0f;

	/** Prototype link preview. Disabled by default; real chain presentation belongs in Blueprint/VFX. */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Matador|Debug")
	bool bShowDebugVisuals = false;
};
