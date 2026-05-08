#include "AI/Services/BTS_UpdateEnemyTactics.h"

#include "AI/Controllers/EnemyAIController.h"
#include "AI/Data/EnemyBlackboardKeys.h"
#include "AI/Data/EnemyLLMEvaluation.h"
#include "AI/Debug/EnemyAIDebugUtils.h"
#include "AI/Tasks/EnemyBTTaskCommon.h"
#include "AIController.h"
#include "AbilitySystem/GP_AttributeSet.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/GP_EnemyCharacter.h"
#include "GameplayTags/GP_Tags.h"
#include "Navigation/PathFollowingComponent.h"

namespace BTS_UpdateEnemyTactics_Internal
{
	float GetClampedBlackboardFloat(const UBlackboardComponent* BlackboardComponent, const FName& KeyName, float DefaultValue, float MinValue, float MaxValue)
	{
		if (!IsValid(BlackboardComponent))
		{
			return DefaultValue;
		}

		return FMath::Clamp(BlackboardComponent->GetValueAsFloat(KeyName), MinValue, MaxValue);
	}

	bool HasBlackboardKey(const UBlackboardComponent* BlackboardComponent, const FName& KeyName)
	{
		return IsValid(BlackboardComponent) && BlackboardComponent->GetKeyID(KeyName) != FBlackboard::InvalidKey;
	}

	bool GetOptionalBlackboardBool(const UBlackboardComponent* BlackboardComponent, const FName& KeyName)
	{
		return HasBlackboardKey(BlackboardComponent, KeyName) && BlackboardComponent->GetValueAsBool(KeyName);
	}

	void SetOptionalBlackboardBool(UBlackboardComponent* BlackboardComponent, const FName& KeyName, bool bValue)
	{
		if (HasBlackboardKey(BlackboardComponent, KeyName))
		{
			BlackboardComponent->SetValueAsBool(KeyName, bValue);
		}
	}

	void SetOptionalBlackboardName(UBlackboardComponent* BlackboardComponent, const FName& KeyName, const FName& Value)
	{
		if (HasBlackboardKey(BlackboardComponent, KeyName))
		{
			BlackboardComponent->SetValueAsName(KeyName, Value);
		}
	}

	void SetOptionalBlackboardInt(UBlackboardComponent* BlackboardComponent, const FName& KeyName, int32 Value)
	{
		if (HasBlackboardKey(BlackboardComponent, KeyName))
		{
			BlackboardComponent->SetValueAsInt(KeyName, Value);
		}
	}

	void SetCombatStateTag(UBlackboardComponent* BlackboardComponent, const FGameplayTag& StateTag)
	{
		// Blackboard의 Name 키에는 GameplayTag 전체 경로를 넣어 BT 데코레이터가 새 태그 체계를 그대로 비교하게 한다.
		SetOptionalBlackboardName(BlackboardComponent, EnemyBlackboardKeys::CombatState, FName(*StateTag.ToString()));
	}

	void SetOptionalBlackboardFloat(UBlackboardComponent* BlackboardComponent, const FName& KeyName, float Value)
	{
		if (HasBlackboardKey(BlackboardComponent, KeyName))
		{
			BlackboardComponent->SetValueAsFloat(KeyName, Value);
		}
	}

	void SetOptionalBlackboardVector(UBlackboardComponent* BlackboardComponent, const FName& KeyName, const FVector& Value)
	{
		if (HasBlackboardKey(BlackboardComponent, KeyName))
		{
			BlackboardComponent->SetValueAsVector(KeyName, Value);
		}
	}

	bool IsPatternWindowOpen(float WorldTimeSeconds, float Interval, float Window)
	{
		if (Interval <= KINDA_SMALL_NUMBER)
		{
			return true;
		}

		return FMath::Fmod(FMath::Max(0.0f, WorldTimeSeconds), Interval) <= FMath::Max(0.0f, Window);
	}

	float GetControlledPawnHealthRatio(const APawn* ControlledPawn)
	{
		const AGP_EnemyCharacter* EnemyCharacter = Cast<AGP_EnemyCharacter>(ControlledPawn);
		if (!IsValid(EnemyCharacter))
		{
			return 1.0f;
		}

		const UGP_AttributeSet* AttributeSet = Cast<UGP_AttributeSet>(EnemyCharacter->GetAttributeSet());
		if (!IsValid(AttributeSet))
		{
			return 1.0f;
		}

		const float MaxHealth = AttributeSet->GetMaxHealth();
		if (MaxHealth <= KINDA_SMALL_NUMBER)
		{
			return 1.0f;
		}

		return FMath::Clamp(AttributeSet->GetHealth() / MaxHealth, 0.0f, 1.0f);
	}
}

using namespace BTS_UpdateEnemyTactics_Internal;

UBTS_UpdateEnemyTactics::UBTS_UpdateEnemyTactics()
{
	NodeName = TEXT("Update Enemy Tactics");

	// Start with responsive test values; increase Interval/RandomDeviation after profiling larger enemy counts.
	Interval = 0.2f;
	RandomDeviation = 0.05f;
	bCallTickOnSearchStart = true;
	bRestartTimerOnEachActivation = false;

	INIT_SERVICE_NODE_NOTIFY_FLAGS();
}

void UBTS_UpdateEnemyTactics::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
	UpdateTactics(OwnerComp);
}

void UBTS_UpdateEnemyTactics::OnSearchStart(FBehaviorTreeSearchData& SearchData)
{
	Super::OnSearchStart(SearchData);
	UpdateTactics(SearchData.OwnerComp);
}

FString UBTS_UpdateEnemyTactics::GetStaticServiceDescription() const
{
	return FString::Printf(
		TEXT("Writes tactical BB keys only. Interval %.2fs +/- %.2fs"),
		Interval,
		RandomDeviation);
}

void UBTS_UpdateEnemyTactics::UpdateTactics(UBehaviorTreeComponent& OwnerComp) const
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* ControlledPawn = IsValid(AIController) ? AIController->GetPawn() : nullptr;
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();

	if (!IsValid(AIController) || !IsValid(ControlledPawn) || !IsValid(BlackboardComponent))
	{
		return;
	}

	const bool bPreviousShouldRetreat = BlackboardComponent->GetValueAsBool(EnemyBlackboardKeys::bShouldRetreat);
	const bool bPreviousCanAttack = BlackboardComponent->GetValueAsBool(EnemyBlackboardKeys::bCanAttack);
	const bool bPreviousShouldReposition = BlackboardComponent->GetValueAsBool(EnemyBlackboardKeys::bShouldReposition);
	const bool bPreviousShouldChase = BlackboardComponent->GetValueAsBool(EnemyBlackboardKeys::bShouldChase);
	const bool bPreviousShouldReturnHome = GetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bShouldReturnHome);
	const int32 PreviousBossPhase = HasBlackboardKey(BlackboardComponent, EnemyBlackboardKeys::BossPhase)
		? BlackboardComponent->GetValueAsInt(EnemyBlackboardKeys::BossPhase)
		: 0;

	const float HealthRatio = GetControlledPawnHealthRatio(ControlledPawn);
	BlackboardComponent->SetValueAsFloat(EnemyBlackboardKeys::HealthRatio, HealthRatio);

	AEnemyAIController* EnemyAIController = Cast<AEnemyAIController>(AIController);
	const AGP_EnemyCharacter* EnemyCharacter = Cast<AGP_EnemyCharacter>(ControlledPawn);
	const float EffectiveMaxChaseDistanceFromHome = IsValid(EnemyCharacter) ? EnemyCharacter->GetReturnHomeDistance() : MaxChaseDistanceFromHome;
	const float EffectiveReturnHomeAcceptanceRadius = IsValid(EnemyCharacter) ? EnemyCharacter->GetReturnHomeAcceptanceRadius() : ReturnHomeAcceptanceRadius;
	const FVector HomeLocation = EnemyBTTaskCommon::GetBehaviorAnchorLocation(ControlledPawn);
	const float DistanceFromHome = FVector::Dist2D(ControlledPawn->GetActorLocation(), HomeLocation);
	SetOptionalBlackboardVector(BlackboardComponent, EnemyBlackboardKeys::HomeLocation, HomeLocation);
	SetOptionalBlackboardFloat(BlackboardComponent, EnemyBlackboardKeys::DistanceFromHome, DistanceFromHome);

	AActor* TargetActor = Cast<AActor>(BlackboardComponent->GetValueAsObject(EnemyBlackboardKeys::TargetActor));
	// If MoveTo stops slightly outside the authored radius, unlock near home instead of trapping the enemy in ReturnHome.
	const float FailSafeReturnHomeAcceptanceRadius = FMath::Max(
		EffectiveReturnHomeAcceptanceRadius,
		FMath::Min(EffectiveMaxChaseDistanceFromHome * 0.25f, 600.0f));
	const bool bReturnMoveStoppedNearHome =
		IsValid(EnemyAIController)
		&& EnemyAIController->IsLeashReturnHomeActive()
		&& AIController->GetMoveStatus() != EPathFollowingStatus::Moving
		&& DistanceFromHome <= FailSafeReturnHomeAcceptanceRadius;
	bool bShouldReturnHome = bEnableLeashReturnHome
		&& IsValid(EnemyAIController)
		&& EnemyAIController->IsLeashReturnHomeActive()
		&& DistanceFromHome > EffectiveReturnHomeAcceptanceRadius
		&& !bReturnMoveStoppedNearHome;

	if (bEnableLeashReturnHome && IsValid(EnemyAIController) && IsValid(TargetActor) && DistanceFromHome > EffectiveMaxChaseDistanceFromHome)
	{
		// Enemy Character range settings define the actual leash distance used by this service.
		// The leash is an AI rule, but movement remains a Behavior Tree MoveTo driven by MoveToLocation.
		bShouldReturnHome = true;
	}

	if (IsValid(EnemyAIController))
	{
		EnemyAIController->SetLeashReturnHomeActive(bShouldReturnHome);
	}

	SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bShouldReturnHome, bShouldReturnHome);
	if (!bShouldReturnHome && !IsValid(TargetActor) && IsValid(EnemyAIController))
	{
		// After returning home, perception may need an explicit rescore before the player can become TargetActor again.
		EnemyAIController->RequestTargetActorReevaluation();
		TargetActor = Cast<AActor>(BlackboardComponent->GetValueAsObject(EnemyBlackboardKeys::TargetActor));
	}

	if (bShouldReturnHome)
	{
		SetCombatStateTag(BlackboardComponent, GPTags::AI::State::Patrol);
		BlackboardComponent->SetValueAsVector(EnemyBlackboardKeys::MoveToLocation, HomeLocation);
		BlackboardComponent->SetValueAsFloat(EnemyBlackboardKeys::DistanceToTarget, 0.0f);
		BlackboardComponent->SetValueAsBool(EnemyBlackboardKeys::bHasLineOfSight, false);
		BlackboardComponent->SetValueAsBool(EnemyBlackboardKeys::bShouldRetreat, false);
		BlackboardComponent->SetValueAsBool(EnemyBlackboardKeys::bCanAttack, false);
		BlackboardComponent->SetValueAsBool(EnemyBlackboardKeys::bShouldReposition, false);
		BlackboardComponent->SetValueAsBool(EnemyBlackboardKeys::bShouldChase, false);
		// 귀환 중에는 보스 패턴 키를 내려서 공용 공격 태스크가 특수 패턴을 이어 실행하지 않게 한다.
		SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bShouldBossPhaseTransition, false);
		SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bCanUseBossHeavyAttack, false);
		SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bCanUseBossAreaAttack, false);
		SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bCanSummonAdds, false);

		if (!bPreviousShouldReturnHome)
		{
			UE_LOG(
				LogEnemyAI,
				Log,
				TEXT("[Leash] ReturnHome=1 DistFromHome=%.0f Home=%s Pawn=%s"),
				DistanceFromHome,
				*EnemyAIDebugUtils::DescribeLocation(HomeLocation),
				*EnemyAIDebugUtils::DescribeActor(ControlledPawn));
		}

		RestartTreeIfTacticalStateChanged(
			OwnerComp,
			bPreviousShouldRetreat,
			bPreviousCanAttack,
			bPreviousShouldReposition,
			bPreviousShouldChase,
			bPreviousShouldReturnHome,
			false,
			false,
			false,
			false,
			true);
		return;
	}

	if (!IsValid(TargetActor))
	{
		// No target means all combat branch keys collapse to false; decorators can abort immediately.
		// 이동 중이 아니면 Idle, 순찰 MoveTo가 진행 중이면 Patrol 태그로 BT 논리 상태를 나눈다.
		SetCombatStateTag(
			BlackboardComponent,
			AIController->GetMoveStatus() == EPathFollowingStatus::Moving ? GPTags::AI::State::Patrol : GPTags::AI::State::Idle);
		BlackboardComponent->SetValueAsFloat(EnemyBlackboardKeys::DistanceToTarget, 0.0f);
		BlackboardComponent->SetValueAsBool(EnemyBlackboardKeys::bHasLineOfSight, false);
		BlackboardComponent->SetValueAsBool(EnemyBlackboardKeys::bShouldRetreat, false);
		BlackboardComponent->SetValueAsBool(EnemyBlackboardKeys::bCanAttack, false);
		BlackboardComponent->SetValueAsBool(EnemyBlackboardKeys::bShouldReposition, false);
		BlackboardComponent->SetValueAsBool(EnemyBlackboardKeys::bShouldChase, false);
		SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bShouldReturnHome, false);
		// 타깃을 잃으면 보스 특수 패턴도 닫아 다음 감지 때 새 상태로 다시 평가하게 한다.
		SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bShouldBossPhaseTransition, false);
		SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bCanUseBossHeavyAttack, false);
		SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bCanUseBossAreaAttack, false);
		SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bCanSummonAdds, false);

		if (bPreviousShouldRetreat || bPreviousCanAttack || bPreviousShouldReposition || bPreviousShouldChase || bPreviousShouldReturnHome)
		{
			UE_LOG(LogEnemyAI, Verbose, TEXT("[State] No target -> combat branches off (%s)"), *EnemyAIDebugUtils::DescribeActor(ControlledPawn));
		}

		RestartTreeIfTacticalStateChanged(
			OwnerComp,
			bPreviousShouldRetreat,
			bPreviousCanAttack,
			bPreviousShouldReposition,
			bPreviousShouldChase,
			bPreviousShouldReturnHome,
			false,
			false,
			false,
			false,
			false);
		return;
	}

	const float DistanceToTarget = FVector::Distance(ControlledPawn->GetActorLocation(), TargetActor->GetActorLocation());
	const bool bHasLineOfSight = AIController->LineOfSightTo(TargetActor);
	const float PreferredRange = GetClampedBlackboardFloat(BlackboardComponent, EnemyBlackboardKeys::PreferredRange, 600.0f, 0.0f, 3000.0f);
	const float Aggression = GetClampedBlackboardFloat(BlackboardComponent, EnemyBlackboardKeys::Aggression, 0.5f, 0.0f, 1.0f);
	const float ChasePersistence = GetClampedBlackboardFloat(BlackboardComponent, EnemyBlackboardKeys::ChasePersistence, 0.5f, 0.0f, 1.0f);
	const float RetreatThreshold = GetClampedBlackboardFloat(BlackboardComponent, EnemyBlackboardKeys::RetreatThreshold, 0.35f, 0.0f, 1.0f);
	const float CoverPreference = GetClampedBlackboardFloat(BlackboardComponent, EnemyBlackboardKeys::CoverPreference, 0.35f, 0.0f, 1.0f);
	const FName EnemyMode = BlackboardComponent->GetValueAsName(EnemyBlackboardKeys::EnemyMode);

	const bool bModeForcesRetreat = EnemyMode == FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyMode::Retreat);
	const bool bModePrefersHold = EnemyMode == FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyMode::Hold);
	const bool bModePrefersPressure = EnemyMode == FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyMode::Pressure);

	// The attack band is intentionally tunable here, while branch selection remains in BT decorators.
	const float AttackWindow = FMath::Max(AttackWindowFloor, FMath::Lerp(MinAttackWindow, MaxAttackWindow, Aggression));
	const float MinAttackRange = FMath::Max(0.0f, PreferredRange - AttackWindow);
	const float MaxAttackRange = PreferredRange + AttackWindow;
	const bool bInsideAttackBand = DistanceToTarget <= MaxAttackRange && (bAllowAttacksInsidePreferredRange || DistanceToTarget >= MinAttackRange);
	const bool bTooClose = !bAllowAttacksInsidePreferredRange && DistanceToTarget < MinAttackRange;
	const bool bTooFar = DistanceToTarget > MaxAttackRange;

	bool bShouldRetreat = bModeForcesRetreat || (HealthRatio <= RetreatThreshold);
	bool bCanAttack = !bShouldRetreat && bHasLineOfSight && bInsideAttackBand;
	bool bShouldReposition = false;
	bool bShouldChase = false;

	if (!bShouldRetreat && !bCanAttack)
	{
		const bool bCoverDrivenReposition = CoverPreference >= CoverRepositionThreshold && !bHasLineOfSight;
		const bool bRangeDrivenReposition = bTooClose || (bModePrefersHold && !bTooFar);

		bShouldReposition = bCoverDrivenReposition || bRangeDrivenReposition;
		bShouldChase = bTooFar || (!bHasLineOfSight && ChasePersistence >= LostLineOfSightChaseThreshold) || (bModePrefersPressure && !bHasLineOfSight);

		// When reposition and chase both pass, prefer the behavior that matches the current tuning.
		if (bShouldReposition && bShouldChase)
		{
			if (bModePrefersHold || CoverPreference >= Aggression)
			{
				bShouldChase = false;
			}
			else
			{
				bShouldReposition = false;
			}
		}
	}

	// When the AI still has a live target, patrol should be the "no target" fallback, not the "I saw the player" fallback.
	if (bFallbackToChaseWhenTargetExists && !bShouldRetreat && !bCanAttack && !bShouldReposition)
	{
		bShouldChase = true;
	}

	// 추격만 별도 상태로 분리하고, 공격/후퇴/재배치는 모두 교전 상태로 BT에 전달한다.
	const FGameplayTag CombatStateTag = bShouldChase
		? GPTags::AI::State::Chasing
		: GPTags::AI::State::Combat;
	SetCombatStateTag(BlackboardComponent, CombatStateTag);

	BlackboardComponent->SetValueAsFloat(EnemyBlackboardKeys::DistanceToTarget, DistanceToTarget);
	BlackboardComponent->SetValueAsBool(EnemyBlackboardKeys::bHasLineOfSight, bHasLineOfSight);
	BlackboardComponent->SetValueAsBool(EnemyBlackboardKeys::bShouldRetreat, bShouldRetreat);
	BlackboardComponent->SetValueAsBool(EnemyBlackboardKeys::bCanAttack, bCanAttack);
	BlackboardComponent->SetValueAsBool(EnemyBlackboardKeys::bShouldReposition, bShouldReposition);
	BlackboardComponent->SetValueAsBool(EnemyBlackboardKeys::bShouldChase, bShouldChase);
	SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bShouldReturnHome, false);

	if (IsValid(EnemyCharacter) && EnemyCharacter->IsBossEnemy() && HasBlackboardKey(BlackboardComponent, EnemyBlackboardKeys::BossPhase))
	{
		constexpr float BossPhaseTwoHealthRatio = 0.66f;
		constexpr float BossPhaseThreeHealthRatio = 0.33f;
		constexpr float BossHeavyAttackRange = 650.0f;
		constexpr float BossAreaAttackRange = 1400.0f;
		constexpr float BossAreaAttackInterval = 8.0f;
		constexpr float BossAreaAttackWindow = 1.2f;
		constexpr float BossSummonInterval = 18.0f;
		constexpr float BossSummonWindow = 1.5f;

		const int32 BossPhase = HealthRatio <= BossPhaseThreeHealthRatio ? 3 : (HealthRatio <= BossPhaseTwoHealthRatio ? 2 : 1);
		SetOptionalBlackboardInt(BlackboardComponent, EnemyBlackboardKeys::BossPhase, BossPhase);

		const float WorldTimeSeconds = OwnerComp.GetWorld() != nullptr ? OwnerComp.GetWorld()->GetTimeSeconds() : 0.0f;
		const bool bShouldBossPhaseTransition = PreviousBossPhase > 0 && PreviousBossPhase != BossPhase;
		const bool bCanUseBossHeavyAttack = !bShouldBossPhaseTransition && bCanAttack && DistanceToTarget <= BossHeavyAttackRange;
		const bool bCanUseBossAreaAttack = !bShouldBossPhaseTransition
			&& BossPhase >= 2
			&& bHasLineOfSight
			&& DistanceToTarget <= BossAreaAttackRange
			&& IsPatternWindowOpen(WorldTimeSeconds, BossAreaAttackInterval, BossAreaAttackWindow);
		const bool bCanSummonAdds = !bShouldBossPhaseTransition
			&& BossPhase >= 3
			&& IsPatternWindowOpen(WorldTimeSeconds, BossSummonInterval, BossSummonWindow);
		const bool bBossPatternRequestsAttack = bShouldBossPhaseTransition || bCanSummonAdds || bCanUseBossAreaAttack || bCanUseBossHeavyAttack;

		SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bShouldBossPhaseTransition, bShouldBossPhaseTransition);
		SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bCanUseBossHeavyAttack, bCanUseBossHeavyAttack);
		SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bCanUseBossAreaAttack, bCanUseBossAreaAttack);
		SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bCanSummonAdds, bCanSummonAdds);

		if (bBossPatternRequestsAttack)
		{
			// 기존 공용 BT의 공격 분기만으로도 보스 전용 Ability 태그가 실행되도록 전술 분기 값을 정리한다.
			bCanAttack = true;
			bShouldRetreat = false;
			bShouldReposition = false;
			bShouldChase = false;
			BlackboardComponent->SetValueAsBool(EnemyBlackboardKeys::bCanAttack, true);
			BlackboardComponent->SetValueAsBool(EnemyBlackboardKeys::bShouldRetreat, false);
			BlackboardComponent->SetValueAsBool(EnemyBlackboardKeys::bShouldReposition, false);
			BlackboardComponent->SetValueAsBool(EnemyBlackboardKeys::bShouldChase, false);
		}
	}

	if (bPreviousShouldRetreat != bShouldRetreat
		|| bPreviousCanAttack != bCanAttack
		|| bPreviousShouldReposition != bShouldReposition
		|| bPreviousShouldChase != bShouldChase
		|| bPreviousShouldReturnHome)
	{
		UE_LOG(
			LogEnemyAI,
			Verbose,
			TEXT("[State] Retreat=%d Attack=%d Reposition=%d Chase=%d ReturnHome=0 Dist=%.0f HomeDist=%.0f Health=%.2f LOS=%d Pawn=%s"),
			bShouldRetreat ? 1 : 0,
			bCanAttack ? 1 : 0,
			bShouldReposition ? 1 : 0,
			bShouldChase ? 1 : 0,
			DistanceToTarget,
			DistanceFromHome,
			HealthRatio,
			bHasLineOfSight ? 1 : 0,
			*EnemyAIDebugUtils::DescribeActor(ControlledPawn));
	}

	RestartTreeIfTacticalStateChanged(
		OwnerComp,
		bPreviousShouldRetreat,
		bPreviousCanAttack,
		bPreviousShouldReposition,
		bPreviousShouldChase,
		bPreviousShouldReturnHome,
		bShouldRetreat,
		bCanAttack,
		bShouldReposition,
		bShouldChase,
		false);
}

void UBTS_UpdateEnemyTactics::RestartTreeIfTacticalStateChanged(
	UBehaviorTreeComponent& OwnerComp,
	bool bPreviousShouldRetreat,
	bool bPreviousCanAttack,
	bool bPreviousShouldReposition,
	bool bPreviousShouldChase,
	bool bPreviousShouldReturnHome,
	bool bShouldRetreat,
	bool bCanAttack,
	bool bShouldReposition,
	bool bShouldChase,
	bool bShouldReturnHome) const
{
	if (!bRestartTreeOnTacticalStateChange)
	{
		return;
	}

	const bool bTacticalStateChanged =
		bPreviousShouldRetreat != bShouldRetreat
		|| bPreviousCanAttack != bCanAttack
		|| bPreviousShouldReposition != bShouldReposition
		|| bPreviousShouldChase != bShouldChase
		|| bPreviousShouldReturnHome != bShouldReturnHome;

	if (!bTacticalStateChanged || !OwnerComp.IsRunning() || OwnerComp.IsRestartPending() || OwnerComp.IsAbortPending())
	{
		return;
	}

	AEnemyAIController* EnemyAIController = Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
	if (!IsValid(EnemyAIController))
	{
		return;
	}

	// Services run inside BehaviorTreeComponent tick, so defer the restart through the controller to avoid BT priority ensures.
	EnemyAIController->RequestBehaviorTreeRootReevaluation();
}
