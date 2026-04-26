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

namespace
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

	const float HealthRatio = GetControlledPawnHealthRatio(ControlledPawn);
	BlackboardComponent->SetValueAsFloat(EnemyBlackboardKeys::HealthRatio, HealthRatio);

	AEnemyAIController* EnemyAIController = Cast<AEnemyAIController>(AIController);
	const FVector HomeLocation = EnemyBTTaskCommon::GetBehaviorAnchorLocation(ControlledPawn);
	const float DistanceFromHome = FVector::Dist2D(ControlledPawn->GetActorLocation(), HomeLocation);
	SetOptionalBlackboardVector(BlackboardComponent, EnemyBlackboardKeys::HomeLocation, HomeLocation);
	SetOptionalBlackboardFloat(BlackboardComponent, EnemyBlackboardKeys::DistanceFromHome, DistanceFromHome);

	AActor* TargetActor = Cast<AActor>(BlackboardComponent->GetValueAsObject(EnemyBlackboardKeys::TargetActor));
	bool bShouldReturnHome = bEnableLeashReturnHome
		&& IsValid(EnemyAIController)
		&& EnemyAIController->IsLeashReturnHomeActive()
		&& DistanceFromHome > ReturnHomeAcceptanceRadius;

	if (bEnableLeashReturnHome && IsValid(EnemyAIController) && IsValid(TargetActor) && DistanceFromHome > MaxChaseDistanceFromHome)
	{
		// The leash is an AI rule, but movement remains a Behavior Tree MoveTo driven by MoveToLocation.
		bShouldReturnHome = true;
	}

	if (IsValid(EnemyAIController))
	{
		EnemyAIController->SetLeashReturnHomeActive(bShouldReturnHome);
	}

	SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bShouldReturnHome, bShouldReturnHome);

	if (bShouldReturnHome)
	{
		BlackboardComponent->SetValueAsVector(EnemyBlackboardKeys::MoveToLocation, HomeLocation);
		BlackboardComponent->SetValueAsFloat(EnemyBlackboardKeys::DistanceToTarget, 0.0f);
		BlackboardComponent->SetValueAsBool(EnemyBlackboardKeys::bHasLineOfSight, false);
		BlackboardComponent->SetValueAsBool(EnemyBlackboardKeys::bShouldRetreat, false);
		BlackboardComponent->SetValueAsBool(EnemyBlackboardKeys::bCanAttack, false);
		BlackboardComponent->SetValueAsBool(EnemyBlackboardKeys::bShouldReposition, false);
		BlackboardComponent->SetValueAsBool(EnemyBlackboardKeys::bShouldChase, false);

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

		return;
	}

	if (!IsValid(TargetActor))
	{
		// No target means all combat branch keys collapse to false; decorators can abort immediately.
		BlackboardComponent->SetValueAsFloat(EnemyBlackboardKeys::DistanceToTarget, 0.0f);
		BlackboardComponent->SetValueAsBool(EnemyBlackboardKeys::bHasLineOfSight, false);
		BlackboardComponent->SetValueAsBool(EnemyBlackboardKeys::bShouldRetreat, false);
		BlackboardComponent->SetValueAsBool(EnemyBlackboardKeys::bCanAttack, false);
		BlackboardComponent->SetValueAsBool(EnemyBlackboardKeys::bShouldReposition, false);
		BlackboardComponent->SetValueAsBool(EnemyBlackboardKeys::bShouldChase, false);
		SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bShouldReturnHome, false);

		if (bPreviousShouldRetreat || bPreviousCanAttack || bPreviousShouldReposition || bPreviousShouldChase || bPreviousShouldReturnHome)
		{
			UE_LOG(LogEnemyAI, Verbose, TEXT("[State] No target -> combat branches off (%s)"), *EnemyAIDebugUtils::DescribeActor(ControlledPawn));
		}

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

	BlackboardComponent->SetValueAsFloat(EnemyBlackboardKeys::DistanceToTarget, DistanceToTarget);
	BlackboardComponent->SetValueAsBool(EnemyBlackboardKeys::bHasLineOfSight, bHasLineOfSight);
	BlackboardComponent->SetValueAsBool(EnemyBlackboardKeys::bShouldRetreat, bShouldRetreat);
	BlackboardComponent->SetValueAsBool(EnemyBlackboardKeys::bCanAttack, bCanAttack);
	BlackboardComponent->SetValueAsBool(EnemyBlackboardKeys::bShouldReposition, bShouldReposition);
	BlackboardComponent->SetValueAsBool(EnemyBlackboardKeys::bShouldChase, bShouldChase);
	SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bShouldReturnHome, false);

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
}
