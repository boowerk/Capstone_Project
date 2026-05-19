#include "AI/Tasks/BTT_ExecuteBossAttack.h"

#include "AbilitySystem/GP_AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AI/Controllers/EnemyAIController.h"
#include "AI/Data/EnemyBlackboardKeys.h"
#include "AI/Data/EnemyLLMEvaluation.h"
#include "AI/Debug/EnemyAIDebugUtils.h"
#include "AI/Tasks/EnemyBTTaskCommon.h"
#include "Abilities/GameplayAbility.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameplayTags/GP_Tags.h"

namespace BossAttackSelector
{
	struct FPatternCandidate
	{
		FGameplayTag AbilityTag;
		float Score = 0.0f;
		const TCHAR* DebugName = TEXT("Unknown");
	};

	bool HasBlackboardKey(const UBlackboardComponent* BlackboardComponent, const FName& KeyName)
	{
		return IsValid(BlackboardComponent) && BlackboardComponent->GetKeyID(KeyName) != FBlackboard::InvalidKey;
	}

	bool GetBool(const UBlackboardComponent* BlackboardComponent, const FName& KeyName)
	{
		return HasBlackboardKey(BlackboardComponent, KeyName) && BlackboardComponent->GetValueAsBool(KeyName);
	}

	float GetFloat(const UBlackboardComponent* BlackboardComponent, const FName& KeyName, float DefaultValue)
	{
		return HasBlackboardKey(BlackboardComponent, KeyName) ? BlackboardComponent->GetValueAsFloat(KeyName) : DefaultValue;
	}

	FName GetName(const UBlackboardComponent* BlackboardComponent, const FName& KeyName, FName DefaultValue)
	{
		return HasBlackboardKey(BlackboardComponent, KeyName) ? BlackboardComponent->GetValueAsName(KeyName) : DefaultValue;
	}

	void AddCandidate(TArray<FPatternCandidate>& Candidates, const FGameplayTag& AbilityTag, float Score, const TCHAR* DebugName)
	{
		if (!AbilityTag.IsValid() || Score <= KINDA_SMALL_NUMBER)
		{
			return;
		}

		for (FPatternCandidate& Candidate : Candidates)
		{
			if (Candidate.AbilityTag == AbilityTag)
			{
				Candidate.Score = FMath::Max(Candidate.Score, Score);
				Candidate.DebugName = DebugName;
				return;
			}
		}

		FPatternCandidate Candidate;
		Candidate.AbilityTag = AbilityTag;
		Candidate.Score = Score;
		Candidate.DebugName = DebugName;
		Candidates.Add(Candidate);
	}

	void MoveWeightedChoiceToFront(TArray<FPatternCandidate>& Candidates)
	{
		if (Candidates.Num() <= 1)
		{
			return;
		}

		float TotalScore = 0.0f;
		for (const FPatternCandidate& Candidate : Candidates)
		{
			TotalScore += FMath::Max(0.0f, Candidate.Score);
		}

		if (TotalScore <= KINDA_SMALL_NUMBER)
		{
			return;
		}

		float Roll = FMath::FRandRange(0.0f, TotalScore);
		int32 SelectedIndex = 0;
		for (int32 Index = 0; Index < Candidates.Num(); ++Index)
		{
			Roll -= FMath::Max(0.0f, Candidates[Index].Score);
			if (Roll <= 0.0f)
			{
				SelectedIndex = Index;
				break;
			}
		}

		FPatternCandidate SelectedCandidate = Candidates[SelectedIndex];
		Candidates.RemoveAt(SelectedIndex);
		Candidates.Sort([](const FPatternCandidate& Left, const FPatternCandidate& Right)
		{
			return Left.Score > Right.Score;
		});
		Candidates.Insert(SelectedCandidate, 0);
	}

	FString DescribeCandidates(const TArray<FPatternCandidate>& Candidates)
	{
		TArray<FString> Parts;
		for (const FPatternCandidate& Candidate : Candidates)
		{
			Parts.Add(FString::Printf(TEXT("%s:%.2f:%s"), Candidate.DebugName, Candidate.Score, *Candidate.AbilityTag.ToString()));
		}

		return FString::Join(Parts, TEXT(", "));
	}

	bool HasGrantedAbilityForTag(const UAbilitySystemComponent* ASC, const FGameplayTag& AbilityTag)
	{
		if (!IsValid(ASC) || !AbilityTag.IsValid())
		{
			return false;
		}

		for (const FGameplayAbilitySpec& AbilitySpec : ASC->GetActivatableAbilities())
		{
			const UGameplayAbility* Ability = AbilitySpec.Ability;
			if (IsValid(Ability) && Ability->GetAssetTags().HasTagExact(AbilityTag))
			{
				return true;
			}
		}

		return false;
	}

	TArray<FPatternCandidate> BuildPatternCandidates(const APawn* ControlledPawn, const UBlackboardComponent* BlackboardComponent, const FGameplayTag& DefaultAttackAbilityTag)
	{
		TArray<FPatternCandidate> Candidates;
		if (!IsValid(ControlledPawn) || !IsValid(BlackboardComponent))
		{
			AddCandidate(Candidates, DefaultAttackAbilityTag, 1.0f, TEXT("BasicFallback"));
			return Candidates;
		}

		const AActor* TargetActor = EnemyBTTaskCommon::GetTargetActor(BlackboardComponent);
		const float BlackboardDistance = GetFloat(BlackboardComponent, EnemyBlackboardKeys::DistanceToTarget, 0.0f);
		const float DistanceToTarget = IsValid(TargetActor)
			? FVector::Dist2D(ControlledPawn->GetActorLocation(), TargetActor->GetActorLocation())
			: BlackboardDistance;

		const float Aggression = FMath::Clamp(GetFloat(BlackboardComponent, EnemyBlackboardKeys::Aggression, 0.5f), 0.0f, 1.0f);
		const float PreferredRange = FMath::Max(100.0f, GetFloat(BlackboardComponent, EnemyBlackboardKeys::PreferredRange, 600.0f));
		const float HealthRatio = FMath::Clamp(GetFloat(BlackboardComponent, EnemyBlackboardKeys::HealthRatio, 1.0f), 0.0f, 1.0f);
		const int32 BossPhase = HasBlackboardKey(BlackboardComponent, EnemyBlackboardKeys::BossPhase)
			? FMath::Max(1, BlackboardComponent->GetValueAsInt(EnemyBlackboardKeys::BossPhase))
			: 1;
		const FName EnemyMode = GetName(BlackboardComponent, EnemyBlackboardKeys::EnemyMode, FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyMode::Patrol));
		const FName FocusRule = GetName(BlackboardComponent, EnemyBlackboardKeys::FocusTargetRule, FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyFocusTargetRule::CurrentThreat));

		const bool bPressureMode = EnemyMode == FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyMode::Pressure);
		const bool bHoldMode = EnemyMode == FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyMode::Hold);
		const bool bRetreatMode = EnemyMode == FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyMode::Retreat);
		const bool bWeakestFocus = FocusRule == FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyFocusTargetRule::Weakest);
		const bool bPlayerFirstFocus = FocusRule == FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyFocusTargetRule::PlayerFirst);

		const float PreferredRangeFit = 1.0f - FMath::Clamp(FMath::Abs(DistanceToTarget - PreferredRange) / PreferredRange, 0.0f, 1.0f);
		const float ClosePressure = 1.0f - FMath::Clamp(DistanceToTarget / PreferredRange, 0.0f, 1.0f);
		const float FarPressure = FMath::Clamp((DistanceToTarget - PreferredRange) / PreferredRange, 0.0f, 1.0f);
		const float PhaseBonus = FMath::Clamp(static_cast<float>(BossPhase - 1) * 0.15f, 0.0f, 0.35f);

		const bool bClosePunishWindow = GetBool(BlackboardComponent, EnemyBlackboardKeys::bCanUseBossHeavyAttack);

		const float BasicScore = 0.35f
			+ (1.0f - Aggression) * 0.25f
			+ ClosePressure * 0.2f
			+ (bHoldMode ? 0.2f : 0.0f)
			+ (bWeakestFocus ? 0.1f : 0.0f)
			+ (bClosePunishWindow ? 0.25f : 0.0f)
			+ (HealthRatio <= 0.45f ? 0.1f : 0.0f);
		// Basic attacks stay in the selector so a valid sweep window no longer collapses every boss attack into sweep.
		// The heavy blackboard flag currently boosts this scratch attack instead of using the legacy missing heavy montage.
		AddCandidate(Candidates, DefaultAttackAbilityTag, BasicScore, TEXT("Basic"));

		if (GetBool(BlackboardComponent, EnemyBlackboardKeys::bCanUseBossSweepAttack))
		{
			const float SweepScore = 0.2f
				+ Aggression * 0.25f
				+ PreferredRangeFit * 0.35f
				+ (bPressureMode ? 0.15f : 0.0f)
				+ (bPlayerFirstFocus ? 0.1f : 0.0f);
			AddCandidate(Candidates, GPTags::Ability::Enemy::Attack_BossSweep, SweepScore, TEXT("Sweep"));
		}

		if (GetBool(BlackboardComponent, EnemyBlackboardKeys::bCanUseBossAreaAttack))
		{
			const float AreaScore = 0.18f
				+ PhaseBonus
				+ FarPressure * 0.25f
				+ (1.0f - Aggression) * 0.12f
				+ (bHoldMode || bRetreatMode ? 0.2f : 0.0f);
			AddCandidate(Candidates, GPTags::Ability::Enemy::Attack_BossArea, AreaScore, TEXT("Area"));
		}

		MoveWeightedChoiceToFront(Candidates);
		return Candidates;
	}

	bool TryActivateAbilityByTag(UAbilitySystemComponent* ASC, const FGameplayTag& AbilityTag)
	{
		if (!IsValid(ASC) || !AbilityTag.IsValid())
		{
			return false;
		}

		if (UGP_AbilitySystemComponent* GPASC = Cast<UGP_AbilitySystemComponent>(ASC))
		{
			return GPASC->TryActivateAbilityByTag(AbilityTag);
		}

		return ASC->TryActivateAbilitiesByTag(AbilityTag.GetSingleTagContainer());
	}
}

UBTT_ExecuteBossAttack::UBTT_ExecuteBossAttack()
{
	// Boss BT attack entry now runs a pattern selector instead of letting the sweep flag dominate every attack.
	NodeName = TEXT("Select And Execute Boss Attack");
	BossSweepAttackChance = 0.0f;
}

EBTNodeResult::Type UBTT_ExecuteBossAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = nullptr;
	APawn* ControlledPawn = nullptr;
	UBlackboardComponent* BlackboardComponent = nullptr;
	if (!EnemyBTTaskCommon::TryGetTaskContext(OwnerComp, AIController, ControlledPawn, BlackboardComponent))
	{
		return EBTNodeResult::Failed;
	}

	if (AEnemyAIController* EnemyAIController = Cast<AEnemyAIController>(AIController))
	{
		// Re-score the player target from the latest AI evaluation before choosing a boss pattern.
		EnemyAIController->RequestTargetActorReevaluation();
	}

	AActor* TargetActor = EnemyBTTaskCommon::GetTargetActor(BlackboardComponent);
	if (IsValid(TargetActor) && BossAttackSelector::HasBlackboardKey(BlackboardComponent, EnemyBlackboardKeys::DistanceToTarget))
	{
		BlackboardComponent->SetValueAsFloat(EnemyBlackboardKeys::DistanceToTarget, FVector::Dist2D(ControlledPawn->GetActorLocation(), TargetActor->GetActorLocation()));
	}

	if (bStopMovementBeforeAttack)
	{
		AIController->StopMovement();
	}

	if (bFaceTargetBeforeAttack && IsValid(TargetActor))
	{
		FVector LookDirection = TargetActor->GetActorLocation() - ControlledPawn->GetActorLocation();
		LookDirection.Z = 0.0f;
		if (!LookDirection.IsNearlyZero())
		{
			ControlledPawn->SetActorRotation(LookDirection.Rotation());
		}
	}

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(ControlledPawn);
	if (!IsValid(ASC))
	{
		UE_LOG(LogEnemyAI, Warning, TEXT("[BossAI] Cannot execute boss pattern - missing ASC: %s"), *GetNameSafe(ControlledPawn));
		return EBTNodeResult::Failed;
	}

	const TArray<BossAttackSelector::FPatternCandidate> Candidates = BossAttackSelector::BuildPatternCandidates(ControlledPawn, BlackboardComponent, AttackAbilityTag);
	bool bFoundGrantedCandidate = false;
	for (const BossAttackSelector::FPatternCandidate& Candidate : Candidates)
	{
		if (!BossAttackSelector::HasGrantedAbilityForTag(ASC, Candidate.AbilityTag))
		{
			UE_LOG(
				LogEnemyAI,
				Log,
				TEXT("[BossAI] Pattern skipped because ability is not granted: %s Tag=%s"),
				Candidate.DebugName,
				*Candidate.AbilityTag.ToString());
			continue;
		}

		bFoundGrantedCandidate = true;
		if (BossAttackSelector::TryActivateAbilityByTag(ASC, Candidate.AbilityTag))
		{
			UE_LOG(
				LogEnemyAI,
				Log,
				TEXT("[BossAI] Pattern selected: %s Score=%.2f Tag=%s Target=%s"),
				Candidate.DebugName,
				Candidate.Score,
				*Candidate.AbilityTag.ToString(),
				*EnemyAIDebugUtils::DescribeActor(TargetActor));
			return EBTNodeResult::Succeeded;
		}
	}

	if (bFoundGrantedCandidate)
	{
		// Treat a temporary block as handled so the attack sequence wait can run instead of instantly reselecting sweep.
		UE_LOG(
			LogEnemyAI,
			Log,
			TEXT("[BossAI] Pattern candidates were blocked this tick: %s Candidates=%s"),
			*GetNameSafe(ControlledPawn),
			*BossAttackSelector::DescribeCandidates(Candidates));
		return EBTNodeResult::Succeeded;
	}

	UE_LOG(
		LogEnemyAI,
		Warning,
		TEXT("[BossAI] No granted ability matched selected boss patterns: %s Candidates=%s"),
		*GetNameSafe(ControlledPawn),
		*BossAttackSelector::DescribeCandidates(Candidates));
	return EBTNodeResult::Failed;
}
