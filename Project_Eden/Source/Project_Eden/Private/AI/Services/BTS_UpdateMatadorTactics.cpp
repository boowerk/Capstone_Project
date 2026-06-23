#include "AI/Services/BTS_UpdateMatadorTactics.h"

#include "AI/Controllers/EnemyAIController.h"
#include "AI/Data/EnemyBlackboardKeys.h"
#include "AI/Debug/EnemyAIDebugUtils.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/GP_MatadorBossStateComponent.h"
#include "Characters/GP_MatadorMageBossCharacter.h"
#include "GameFramework/Pawn.h"
#include "GameplayTags/GP_Tags.h"
#include "Kismet/GameplayStatics.h"

namespace MatadorTactics
{
	bool HasBlackboardKey(const UBlackboardComponent* BlackboardComponent, const FName& KeyName)
	{
		return IsValid(BlackboardComponent) && BlackboardComponent->GetKeyID(KeyName) != FBlackboard::InvalidKey;
	}

	void SetBool(UBlackboardComponent* BlackboardComponent, const FName& KeyName, bool bValue)
	{
		if (HasBlackboardKey(BlackboardComponent, KeyName))
		{
			BlackboardComponent->SetValueAsBool(KeyName, bValue);
		}
	}

	void SetFloat(UBlackboardComponent* BlackboardComponent, const FName& KeyName, float Value)
	{
		if (HasBlackboardKey(BlackboardComponent, KeyName))
		{
			BlackboardComponent->SetValueAsFloat(KeyName, Value);
		}
	}

	void SetInt(UBlackboardComponent* BlackboardComponent, const FName& KeyName, int32 Value)
	{
		if (HasBlackboardKey(BlackboardComponent, KeyName))
		{
			BlackboardComponent->SetValueAsInt(KeyName, Value);
		}
	}

	void SetName(UBlackboardComponent* BlackboardComponent, const FName& KeyName, const FName& Value)
	{
		if (HasBlackboardKey(BlackboardComponent, KeyName))
		{
			BlackboardComponent->SetValueAsName(KeyName, Value);
		}
	}

	void SetObject(UBlackboardComponent* BlackboardComponent, const FName& KeyName, UObject* Value)
	{
		if (HasBlackboardKey(BlackboardComponent, KeyName))
		{
			BlackboardComponent->SetValueAsObject(KeyName, Value);
		}
	}

	bool GetBool(const UBlackboardComponent* BlackboardComponent, const FName& KeyName)
	{
		return HasBlackboardKey(BlackboardComponent, KeyName) && BlackboardComponent->GetValueAsBool(KeyName);
	}

	bool IsPatternWindowOpen(float WorldTimeSeconds, float Interval, float Window)
	{
		return Interval <= KINDA_SMALL_NUMBER
			|| FMath::Fmod(FMath::Max(0.0f, WorldTimeSeconds), Interval) <= FMath::Max(0.0f, Window);
	}
}

UBTS_UpdateMatadorTactics::UBTS_UpdateMatadorTactics()
{
	NodeName = TEXT("Update Matador Tactics");
	Interval = 0.2f;
	RandomDeviation = 0.05f;
	bCallTickOnSearchStart = true;
	bRestartTimerOnEachActivation = false;

	INIT_SERVICE_NODE_NOTIFY_FLAGS();
}

void UBTS_UpdateMatadorTactics::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
	UpdateMatadorTactics(OwnerComp);
}

void UBTS_UpdateMatadorTactics::OnSearchStart(FBehaviorTreeSearchData& SearchData)
{
	Super::OnSearchStart(SearchData);
	UpdateMatadorTactics(SearchData.OwnerComp);
}

FString UBTS_UpdateMatadorTactics::GetStaticServiceDescription() const
{
	return FString::Printf(
		TEXT("Matador bull %.1fs/%.1fs, range %.0f-%.0f"),
		BullPatternInterval,
		BullPatternWindow,
		BullPatternMinRange,
		BullPatternMaxRange);
}

void UBTS_UpdateMatadorTactics::UpdateMatadorTactics(UBehaviorTreeComponent& OwnerComp) const
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* ControlledPawn = IsValid(AIController) ? AIController->GetPawn() : nullptr;
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	AGP_MatadorMageBossCharacter* MatadorBoss = Cast<AGP_MatadorMageBossCharacter>(ControlledPawn);
	UGP_MatadorBossStateComponent* MatadorStateComponent = IsValid(MatadorBoss) ? MatadorBoss->GetMatadorStateComponent() : nullptr;

	if (!IsValid(AIController) || !IsValid(BlackboardComponent) || !IsValid(MatadorBoss) || !IsValid(MatadorStateComponent))
	{
		return;
	}

	AActor* TargetActor = Cast<AActor>(BlackboardComponent->GetValueAsObject(EnemyBlackboardKeys::TargetActor));
	if (!IsValid(TargetActor) && bFallbackToPlayerPawn)
	{
		TargetActor = UGameplayStatics::GetPlayerPawn(MatadorBoss, 0);
		MatadorTactics::SetObject(BlackboardComponent, EnemyBlackboardKeys::TargetActor, TargetActor);
	}

	const bool bHasTarget = IsValid(TargetActor);
	const bool bReturningHome = MatadorTactics::GetBool(BlackboardComponent, EnemyBlackboardKeys::bShouldReturnHome);
	const bool bGroggy = MatadorStateComponent->IsGroggy();
	// Include the telegraph reservation so tactics do not reopen the bull branch before its delayed spawn.
	const bool bBullActive = MatadorBoss->IsBullPatternActive();
	const int32 ChainBreakCount = MatadorStateComponent->GetChainBreakCount();
	const int32 ChainBreakTarget = MatadorStateComponent->GetChainBreakTarget();
	const float DistanceToTarget = bHasTarget
		? FVector::Dist2D(MatadorBoss->GetActorLocation(), TargetActor->GetActorLocation())
		: 0.0f;
	const bool bHasLineOfSight = bHasTarget && AIController->LineOfSightTo(TargetActor);
	const bool bInBullRange = DistanceToTarget >= BullPatternMinRange && DistanceToTarget <= BullPatternMaxRange;
	const float WorldTimeSeconds = OwnerComp.GetWorld() != nullptr ? OwnerComp.GetWorld()->GetTimeSeconds() : 0.0f;
	const bool bBullWindowOpen = MatadorTactics::IsPatternWindowOpen(WorldTimeSeconds, BullPatternInterval, BullPatternWindow);
	const bool bCanUseBullPattern = bHasTarget
		&& !bReturningHome
		&& !bGroggy
		&& !bBullActive
		&& bInBullRange
		&& (bHasLineOfSight || bIgnoreLineOfSightForBullPattern)
		&& bBullWindowOpen;
	const bool bTooClose = bHasTarget && DistanceToTarget < PreferredAirRange * TooCloseRangeRatio;

	MatadorTactics::SetName(BlackboardComponent, EnemyBlackboardKeys::CombatState, FName(*GPTags::AI::State::Combat.GetTag().ToString()));
	MatadorTactics::SetFloat(BlackboardComponent, EnemyBlackboardKeys::DistanceToTarget, DistanceToTarget);
	MatadorTactics::SetBool(BlackboardComponent, EnemyBlackboardKeys::bHasLineOfSight, bHasLineOfSight);
	MatadorTactics::SetInt(BlackboardComponent, EnemyBlackboardKeys::ChainBreakCount, ChainBreakCount);
	MatadorTactics::SetBool(BlackboardComponent, EnemyBlackboardKeys::bIsGroggy, bGroggy);
	MatadorTactics::SetBool(BlackboardComponent, EnemyBlackboardKeys::bCanUseBullPattern, bCanUseBullPattern);
	MatadorTactics::SetBool(BlackboardComponent, EnemyBlackboardKeys::bBullPatternActive, bBullActive);
	MatadorTactics::SetObject(BlackboardComponent, EnemyBlackboardKeys::DecoyActor, MatadorStateComponent->GetDecoyActor());
	MatadorTactics::SetObject(BlackboardComponent, EnemyBlackboardKeys::MainBossActor, MatadorStateComponent->GetMainBossActor());
	MatadorTactics::SetFloat(BlackboardComponent, EnemyBlackboardKeys::PreferredHoverHeight, MatadorBoss->GetPreferredHoverHeight());
	MatadorTactics::SetFloat(BlackboardComponent, EnemyBlackboardKeys::PreferredAirRange, MatadorBoss->GetPreferredAirRange());
	MatadorTactics::SetBool(BlackboardComponent, EnemyBlackboardKeys::bShouldTeleport, MatadorBoss->ShouldTeleportForMatador(DistanceToTarget));

	if (bCanUseBullPattern || (bEnterGroggyWhenChainComplete && ChainBreakCount >= ChainBreakTarget && !bGroggy))
	{
		MatadorTactics::SetBool(BlackboardComponent, EnemyBlackboardKeys::bCanAttack, true);
		MatadorTactics::SetBool(BlackboardComponent, EnemyBlackboardKeys::bShouldRetreat, false);
		MatadorTactics::SetBool(BlackboardComponent, EnemyBlackboardKeys::bShouldReposition, false);
		MatadorTactics::SetBool(BlackboardComponent, EnemyBlackboardKeys::bShouldChase, false);
	}
	else if (bForceRangeRepositionWhenTooClose && bTooClose && !bGroggy && !bBullActive)
	{
		MatadorTactics::SetBool(BlackboardComponent, EnemyBlackboardKeys::bCanAttack, false);
		MatadorTactics::SetBool(BlackboardComponent, EnemyBlackboardKeys::bShouldRetreat, true);
		MatadorTactics::SetBool(BlackboardComponent, EnemyBlackboardKeys::bShouldReposition, true);
		MatadorTactics::SetBool(BlackboardComponent, EnemyBlackboardKeys::bShouldChase, false);
	}

	UE_LOG(
		LogEnemyAI,
		Verbose,
		TEXT("[MatadorAI] Tactics Target=%s Dist=%.0f BullCan=%d BullActive=%d Groggy=%d Chain=%d/%d"),
		*EnemyAIDebugUtils::DescribeActor(TargetActor),
		DistanceToTarget,
		bCanUseBullPattern ? 1 : 0,
		bBullActive ? 1 : 0,
		bGroggy ? 1 : 0,
		ChainBreakCount,
		ChainBreakTarget);
}
