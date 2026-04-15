#include "AI/Controllers/EnemyAIController.h"

#include "AI/Data/EnemyBlackboardKeys.h"
#include "AI/Debug/EnemyAIDebugUtils.h"
#include "AbilitySystem/GP_AttributeSet.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "Characters/GP_EnemyCharacter.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISenseConfig_Sight.h"
#include "TimerManager.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	float SanitizeFiniteFloat(float InValue, float FallbackValue, float MinValue, float MaxValue)
	{
		return FMath::IsFinite(InValue)
			? FMath::Clamp(InValue, MinValue, MaxValue)
			: FMath::Clamp(FallbackValue, MinValue, MaxValue);
	}

	float GetClampedBlackboardFloat(const UBlackboardComponent* BlackboardComponent, const FName& KeyName, float DefaultValue, float MinValue, float MaxValue)
	{
		if (!IsValid(BlackboardComponent))
		{
			return DefaultValue;
		}

		return FMath::Clamp(BlackboardComponent->GetValueAsFloat(KeyName), MinValue, MaxValue);
	}

	FEnemyLLMEvaluation SanitizeEnemyEvaluationForRuntime(
		const FEnemyLLMEvaluation& InEvaluation,
		const FEnemyLLMEvaluation& FallbackEvaluation)
	{
		FEnemyLLMEvaluation SafeEvaluation = InEvaluation;
		SafeEvaluation.Aggression = SanitizeFiniteFloat(SafeEvaluation.Aggression, FallbackEvaluation.Aggression, 0.0f, 1.0f);
		SafeEvaluation.PreferredRange = SanitizeFiniteFloat(SafeEvaluation.PreferredRange, FallbackEvaluation.PreferredRange, 0.0f, 3000.0f);
		SafeEvaluation.RetreatThreshold = SanitizeFiniteFloat(SafeEvaluation.RetreatThreshold, FallbackEvaluation.RetreatThreshold, 0.0f, 1.0f);
		SafeEvaluation.ChasePersistence = SanitizeFiniteFloat(SafeEvaluation.ChasePersistence, FallbackEvaluation.ChasePersistence, 0.0f, 1.0f);
		SafeEvaluation.CoverPreference = SanitizeFiniteFloat(SafeEvaluation.CoverPreference, FallbackEvaluation.CoverPreference, 0.0f, 1.0f);
		SafeEvaluation.ValidateAndClamp();
		return SafeEvaluation;
	}

	bool AreEnemyEvaluationsEffectivelyEqual(const FEnemyLLMEvaluation& Left, const FEnemyLLMEvaluation& Right)
	{
		return Left.EnemyMode == Right.EnemyMode
			&& Left.FocusTargetRule == Right.FocusTargetRule
			&& FMath::IsNearlyEqual(Left.Aggression, Right.Aggression, KINDA_SMALL_NUMBER)
			&& FMath::IsNearlyEqual(Left.PreferredRange, Right.PreferredRange, KINDA_SMALL_NUMBER)
			&& FMath::IsNearlyEqual(Left.RetreatThreshold, Right.RetreatThreshold, KINDA_SMALL_NUMBER)
			&& FMath::IsNearlyEqual(Left.ChasePersistence, Right.ChasePersistence, KINDA_SMALL_NUMBER)
			&& FMath::IsNearlyEqual(Left.CoverPreference, Right.CoverPreference, KINDA_SMALL_NUMBER);
	}

	const TCHAR* GetSharedBehaviorTreeFallbackPath()
	{
		return TEXT("/Game/Characters/EnemyCharacter/BT/BT_EnemyCommon.BT_EnemyCommon");
	}

	const TCHAR* GetSharedBlackboardFallbackPath()
	{
		return TEXT("/Game/Characters/EnemyCharacter/BT/BB_EnemyCommon.BB_EnemyCommon");
	}
}

AEnemyAIController::AEnemyAIController()
{
	EnemyPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("EnemyPerceptionComponent"));
	SightSenseConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightSenseConfig"));

	// 공용 컨트롤러가 같은 감지 컴포넌트를 쓰도록 AAIController의 기본 참조도 연결한다.
	SetPerceptionComponent(*EnemyPerceptionComponent);

	// 기본값은 폰 감지로 두고, 필요하면 에디터에서 더 좁은 클래스로 제한할 수 있다.
	TargetActorClass = APawn::StaticClass();

	if (IsValid(EnemyPerceptionComponent))
	{
		EnemyPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &ThisClass::HandleTargetPerceptionUpdated);
	}

	ConfigureSightSense();
}

void AEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	UE_LOG(LogEnemyAI, Log, TEXT("[AI] Possess: Controller=%s Pawn=%s"), *GetName(), *EnemyAIDebugUtils::DescribeActor(InPawn));

	// 에디터에서 바꾼 감지 파라미터를 실제 런타임 설정에 다시 반영한다.
	ConfigureSightSense();
	InitializeBehaviorTree(InPawn);
	StartCombatStateUpdateLoop();
	StartEvaluationRefreshLoop();

	// possess 이전에 들어온 평가값이 있다면 Blackboard가 준비된 직후 안전하게 반영한다.
	if (bHasPendingEnemyEvaluation)
	{
		HandlePendingEnemyEvaluationTimerElapsed();
	}

	RefreshTargetActorFromPerception();
	HandleCombatStateUpdateTimerElapsed();
}

void AEnemyAIController::OnUnPossess()
{
	UE_LOG(LogEnemyAI, Log, TEXT("[AI] UnPossess: Controller=%s Pawn=%s"), *GetName(), *EnemyAIDebugUtils::DescribeActor(GetPawn()));

	StopCombatStateUpdateLoop();
	StopEvaluationRefreshLoop();
	GetWorldTimerManager().ClearTimer(PendingEnemyEvaluationTimerHandle);

	// 폰을 잃으면 이전 타겟이 남아 있지 않도록 Blackboard를 정리한다.
	SetBlackboardTargetActor(nullptr);

	Super::OnUnPossess();
}

bool AEnemyAIController::SubmitEnemyEvaluation(const FEnemyLLMEvaluation& InEvaluation, bool bForceImmediate)
{
	const FEnemyLLMEvaluation FallbackEvaluation = bHasLastAppliedEnemyEvaluation
		? LastAppliedEnemyEvaluation
		: FEnemyLLMEvaluation::MakeSafeDefault();

	const FEnemyLLMEvaluation SafeEvaluation = SanitizeEnemyEvaluationForRuntime(InEvaluation, FallbackEvaluation);
	if (bHasLastAppliedEnemyEvaluation && AreEnemyEvaluationsEffectivelyEqual(LastAppliedEnemyEvaluation, SafeEvaluation))
	{
		// 실질적으로 달라진 값이 없으면 Blackboard를 다시 쓰지 않아 분기 흔들림을 줄인다.
		UE_LOG(LogEnemyAI, Verbose, TEXT("[Eval] 중복 평가 무시: %s"), *EnemyAIDebugUtils::DescribeEvaluation(SafeEvaluation));
		return true;
	}

	const UWorld* World = GetWorld();
	const float CurrentTime = World != nullptr ? World->GetTimeSeconds() : 0.0f;
	const float TimeSinceLastApply = CurrentTime - LastEnemyEvaluationApplyTime;
	const bool bCanApplyNow = bForceImmediate
		|| !bHasLastAppliedEnemyEvaluation
		|| TimeSinceLastApply >= MinEnemyEvaluationApplyInterval;

	if (bCanApplyNow)
	{
		GetWorldTimerManager().ClearTimer(PendingEnemyEvaluationTimerHandle);
		bHasPendingEnemyEvaluation = false;
		return ApplyEnemyEvaluationInternal(SafeEvaluation);
	}

	// 너무 빠르게 들어온 최신 평가값은 마지막 것만 보관했다가 최소 간격 후 반영한다.
	PendingEnemyEvaluation = SafeEvaluation;
	bHasPendingEnemyEvaluation = true;

	const float RemainingDelay = FMath::Max(0.0f, MinEnemyEvaluationApplyInterval - TimeSinceLastApply);
	UE_LOG(
		LogEnemyAI,
		Verbose,
		TEXT("[Eval] 적용 지연 %.2fs: %s"),
		RemainingDelay,
		*EnemyAIDebugUtils::DescribeEvaluation(SafeEvaluation));

	GetWorldTimerManager().SetTimer(
		PendingEnemyEvaluationTimerHandle,
		this,
		&ThisClass::HandlePendingEnemyEvaluationTimerElapsed,
		RemainingDelay,
		false);

	return true;
}

bool AEnemyAIController::SubmitEnemyEvaluationFromJson(const FString& JsonPayload, bool bForceImmediate)
{
	FEnemyLLMEvaluation ParsedEvaluation;
	const bool bParsed = FEnemyLLMEvaluationParser::ParseFromJson(JsonPayload, ParsedEvaluation);
	if (!bParsed)
	{
		// 파싱 실패 응답은 현재 Blackboard 값을 덮어쓰지 않고 무시해 행동 불안정을 막는다.
		UE_LOG(LogEnemyAI, Warning, TEXT("[Eval] JSON 기반 평가 적용 실패: Controller=%s"), *GetName());
		return false;
	}

	return SubmitEnemyEvaluation(ParsedEvaluation, bForceImmediate);
}

bool AEnemyAIController::ApplyEnemyEvaluationToBlackboard(const FEnemyLLMEvaluation& InEvaluation)
{
	UBlackboardComponent* BlackboardComponent = GetEnemyBlackboardComponent();
	if (!IsValid(BlackboardComponent))
	{
		UE_LOG(LogEnemyAI, Warning, TEXT("[Blackboard] 평가 적용 실패 - Blackboard 없음: %s"), *GetName());
		return false;
	}

	FEnemyLLMEvaluation SafeEvaluation = InEvaluation;
	SafeEvaluation.ValidateAndClamp();

	const bool bModeChanged = !bHasLastAppliedEnemyEvaluation || LastAppliedEnemyEvaluation.EnemyMode != SafeEvaluation.EnemyMode;
	if (bModeChanged)
	{
		const FString PreviousMode = bHasLastAppliedEnemyEvaluation
			? LastAppliedEnemyEvaluation.GetEnemyModeBlackboardValue().ToString()
			: TEXT("None");

		UE_LOG(
			LogEnemyAI,
			Log,
			TEXT("[Mode] %s -> %s (%s)"),
			*PreviousMode,
			*SafeEvaluation.GetEnemyModeBlackboardValue().ToString(),
			*EnemyAIDebugUtils::DescribeActor(GetPawn()));
	}

	// BT를 재구성하지 않고 Blackboard 값만 갱신해 기존 분기들이 자동으로 반응하게 만든다.
	BlackboardComponent->SetValueAsName(EnemyBlackboardKeys::EnemyMode, SafeEvaluation.GetEnemyModeBlackboardValue());
	BlackboardComponent->SetValueAsFloat(EnemyBlackboardKeys::Aggression, SafeEvaluation.Aggression);
	BlackboardComponent->SetValueAsFloat(EnemyBlackboardKeys::PreferredRange, SafeEvaluation.PreferredRange);
	BlackboardComponent->SetValueAsFloat(EnemyBlackboardKeys::RetreatThreshold, SafeEvaluation.RetreatThreshold);
	BlackboardComponent->SetValueAsFloat(EnemyBlackboardKeys::ChasePersistence, SafeEvaluation.ChasePersistence);
	BlackboardComponent->SetValueAsFloat(EnemyBlackboardKeys::CoverPreference, SafeEvaluation.CoverPreference);
	BlackboardComponent->SetValueAsName(EnemyBlackboardKeys::FocusTargetRule, SafeEvaluation.GetFocusTargetRuleBlackboardValue());

	UE_LOG(
		LogEnemyAI,
		Log,
		TEXT("[Blackboard] 적용 완료: Pawn=%s %s"),
		*EnemyAIDebugUtils::DescribeActor(GetPawn()),
		*EnemyAIDebugUtils::DescribeEvaluation(SafeEvaluation));

	// 평가값이 바뀌면 테스트용 전투 상태 키도 즉시 다시 계산해 BT가 바로 반응하게 만든다.
	UpdateCombatStateBlackboard();
	return true;
}

bool AEnemyAIController::InitializeBehaviorTree(APawn* InPawn)
{
	if (!IsValid(InPawn))
	{
		UE_LOG(LogEnemyAI, Warning, TEXT("[AI] Behavior Tree 초기화 실패 - Pawn이 유효하지 않습니다."));
		return false;
	}

	UBehaviorTree* BehaviorTreeAssetToUse = nullptr;
	UBlackboardData* BlackboardAssetToUse = nullptr;
	ResolveBehaviorAssets(InPawn, BehaviorTreeAssetToUse, BlackboardAssetToUse);

	if (!IsValid(BehaviorTreeAssetToUse))
	{
		UE_LOG(LogEnemyAI, Warning, TEXT("[AI] 사용할 Behavior Tree를 찾지 못했습니다: %s"), *GetNameSafe(InPawn));
		return false;
	}

	if (!IsValid(BlackboardAssetToUse))
	{
		BlackboardAssetToUse = BehaviorTreeAssetToUse->BlackboardAsset;
	}

	if (!IsValid(BlackboardAssetToUse))
	{
		UE_LOG(LogEnemyAI, Warning, TEXT("[AI] Blackboard 에셋이 없습니다: %s"), *GetNameSafe(BehaviorTreeAssetToUse));
		return false;
	}

	UBlackboardComponent* BlackboardComponent = nullptr;

	// 공용 Blackboard 에셋이 준비되면 여기서 UseBlackboard를 호출해 키 기본값을 초기화한다.
	if (!UseBlackboard(BlackboardAssetToUse, BlackboardComponent) || !IsValid(BlackboardComponent))
	{
		UE_LOG(LogEnemyAI, Warning, TEXT("[AI] Blackboard 초기화에 실패했습니다: %s"), *GetNameSafe(InPawn));
		return false;
	}

	ValidateSharedBlackboardSchema(BlackboardComponent);
	InitializeBlackboardValues(InPawn);

	// 공용 Behavior Tree 에셋이 준비되면 여기서 RunBehaviorTree를 호출해 적 AI 루프를 시작한다.
	if (!RunBehaviorTree(BehaviorTreeAssetToUse))
	{
		UE_LOG(LogEnemyAI, Warning, TEXT("[AI] Behavior Tree 실행에 실패했습니다: %s"), *GetNameSafe(InPawn));
		return false;
	}

	UE_LOG(LogEnemyAI, Log, TEXT("[AI] Behavior Tree 연결 완료: BT=%s BB=%s"), *GetNameSafe(BehaviorTreeAssetToUse), *GetNameSafe(BlackboardAssetToUse));
	return true;
}

void AEnemyAIController::InitializeBlackboardValues(APawn* InPawn)
{
	UBlackboardComponent* BlackboardComponent = GetEnemyBlackboardComponent();
	if (!IsValid(BlackboardComponent) || !IsValid(InPawn))
	{
		UE_LOG(LogEnemyAI, Warning, TEXT("[Blackboard] 초기값 설정을 건너뜁니다. Controller=%s Pawn=%s"), *GetName(), *GetNameSafe(InPawn));
		return;
	}

	// 런타임 상태 키는 컨트롤러가 초기화하고, 이후 서비스와 태스크가 같은 키를 공유해서 갱신한다.
	BlackboardComponent->ClearValue(EnemyBlackboardKeys::TargetActor);
	BlackboardComponent->SetValueAsVector(EnemyBlackboardKeys::MoveToLocation, InPawn->GetActorLocation());
	BlackboardComponent->SetValueAsBool(EnemyBlackboardKeys::bShouldRetreat, false);
	BlackboardComponent->SetValueAsBool(EnemyBlackboardKeys::bCanAttack, false);
	BlackboardComponent->SetValueAsBool(EnemyBlackboardKeys::bShouldReposition, false);
	BlackboardComponent->SetValueAsBool(EnemyBlackboardKeys::bShouldChase, false);
	BlackboardComponent->SetValueAsBool(EnemyBlackboardKeys::bHasLineOfSight, false);
	BlackboardComponent->SetValueAsFloat(EnemyBlackboardKeys::DistanceToTarget, 0.0f);
	BlackboardComponent->SetValueAsFloat(EnemyBlackboardKeys::HealthRatio, GetCurrentHealthRatio());

	FEnemyLLMEvaluation InitialEvaluation = FEnemyLLMEvaluation::MakeSafeDefault();
	if (const AGP_EnemyCharacter* EnemyCharacter = Cast<AGP_EnemyCharacter>(InPawn))
	{
		// 적 캐릭터가 가진 설정 에셋/테이블을 읽어 개체별 시작 성향값을 주입한다.
		EnemyCharacter->BuildInitialEnemyEvaluation(InitialEvaluation);
	}

	ApplyEnemyEvaluationToBlackboard(InitialEvaluation);
	LastAppliedEnemyEvaluation = InitialEvaluation;
	bHasLastAppliedEnemyEvaluation = true;
	LastEnemyEvaluationApplyTime = GetWorld() != nullptr ? GetWorld()->GetTimeSeconds() : 0.0f;
}

UBlackboardComponent* AEnemyAIController::GetEnemyBlackboardComponent()
{
	return GetBlackboardComponent();
}

bool AEnemyAIController::ApplyEnemyEvaluationInternal(const FEnemyLLMEvaluation& InEvaluation)
{
	if (!ApplyEnemyEvaluationToBlackboard(InEvaluation))
	{
		// Blackboard가 아직 없으면 최신 평가값을 보관했다가 possess 이후 다시 시도한다.
		PendingEnemyEvaluation = InEvaluation;
		bHasPendingEnemyEvaluation = true;
		UE_LOG(LogEnemyAI, Verbose, TEXT("[Eval] Blackboard 준비 전 평가값을 보관합니다: %s"), *EnemyAIDebugUtils::DescribeEvaluation(InEvaluation));
		return false;
	}

	LastAppliedEnemyEvaluation = InEvaluation;
	bHasLastAppliedEnemyEvaluation = true;
	bHasPendingEnemyEvaluation = false;
	LastEnemyEvaluationApplyTime = GetWorld() != nullptr ? GetWorld()->GetTimeSeconds() : 0.0f;
	return true;
}

void AEnemyAIController::HandlePendingEnemyEvaluationTimerElapsed()
{
	if (!bHasPendingEnemyEvaluation)
	{
		return;
	}

	const FEnemyLLMEvaluation QueuedEvaluation = PendingEnemyEvaluation;
	bHasPendingEnemyEvaluation = false;

	UE_LOG(LogEnemyAI, Verbose, TEXT("[Eval] 지연된 평가 적용: %s"), *EnemyAIDebugUtils::DescribeEvaluation(QueuedEvaluation));
	ApplyEnemyEvaluationInternal(QueuedEvaluation);
}

void AEnemyAIController::HandleEvaluationRefreshTimerElapsed()
{
	// Step 8/현재 테스트 단계에서는 실제 네트워크/LLM 요청을 보내지 않는다.
	// 이후에는 여기서 플레이어 상태 요약 생성 -> 비동기 서비스 요청 -> 응답 시 SubmitEnemyEvaluation 호출 흐름을 붙인다.
}

void AEnemyAIController::StartEvaluationRefreshLoop()
{
	if (!bEnableEvaluationRefreshLoop || EvaluationRefreshInterval <= 0.0f)
	{
		return;
	}

	// 매 프레임이 아니라 고정 간격으로만 평가 요청 루프를 돌려 비용과 분기 흔들림을 줄인다.
	GetWorldTimerManager().SetTimer(
		EvaluationRefreshTimerHandle,
		this,
		&ThisClass::HandleEvaluationRefreshTimerElapsed,
		EvaluationRefreshInterval,
		true);
}

void AEnemyAIController::StopEvaluationRefreshLoop()
{
	if (GetWorld() != nullptr)
	{
		GetWorldTimerManager().ClearTimer(EvaluationRefreshTimerHandle);
	}
}

void AEnemyAIController::HandleCombatStateUpdateTimerElapsed()
{
	UpdateCombatStateBlackboard();
}

void AEnemyAIController::StartCombatStateUpdateLoop()
{
	if (!bEnableBuiltInCombatStateUpdater || CombatStateUpdateInterval <= 0.0f)
	{
		return;
	}

	// 평가 함수나 BT 서비스가 비어 있어도 테스트 가능한 기본 전투 상태 갱신 루프를 유지한다.
	GetWorldTimerManager().SetTimer(
		CombatStateUpdateTimerHandle,
		this,
		&ThisClass::HandleCombatStateUpdateTimerElapsed,
		CombatStateUpdateInterval,
		true);
}

void AEnemyAIController::StopCombatStateUpdateLoop()
{
	if (GetWorld() != nullptr)
	{
		GetWorldTimerManager().ClearTimer(CombatStateUpdateTimerHandle);
	}
}

void AEnemyAIController::UpdateCombatStateBlackboard()
{
	if (!bEnableBuiltInCombatStateUpdater)
	{
		return;
	}

	UBlackboardComponent* BlackboardComponent = GetEnemyBlackboardComponent();
	APawn* ControlledPawn = GetPawn();
	if (!IsValid(BlackboardComponent) || !IsValid(ControlledPawn))
	{
		return;
	}

	ValidateSharedBlackboardSchema(BlackboardComponent);

	const bool bPreviousShouldRetreat = BlackboardComponent->GetValueAsBool(EnemyBlackboardKeys::bShouldRetreat);
	const bool bPreviousCanAttack = BlackboardComponent->GetValueAsBool(EnemyBlackboardKeys::bCanAttack);
	const bool bPreviousShouldReposition = BlackboardComponent->GetValueAsBool(EnemyBlackboardKeys::bShouldReposition);
	const bool bPreviousShouldChase = BlackboardComponent->GetValueAsBool(EnemyBlackboardKeys::bShouldChase);

	const float HealthRatio = GetCurrentHealthRatio();
	BlackboardComponent->SetValueAsFloat(EnemyBlackboardKeys::HealthRatio, HealthRatio);

	AActor* TargetActor = Cast<AActor>(BlackboardComponent->GetValueAsObject(EnemyBlackboardKeys::TargetActor));
	if (!IsValid(TargetActor))
	{
		BlackboardComponent->SetValueAsFloat(EnemyBlackboardKeys::DistanceToTarget, 0.0f);
		BlackboardComponent->SetValueAsBool(EnemyBlackboardKeys::bHasLineOfSight, false);
		BlackboardComponent->SetValueAsBool(EnemyBlackboardKeys::bShouldRetreat, false);
		BlackboardComponent->SetValueAsBool(EnemyBlackboardKeys::bCanAttack, false);
		BlackboardComponent->SetValueAsBool(EnemyBlackboardKeys::bShouldReposition, false);
		BlackboardComponent->SetValueAsBool(EnemyBlackboardKeys::bShouldChase, false);

		if (bPreviousShouldRetreat || bPreviousCanAttack || bPreviousShouldReposition || bPreviousShouldChase)
		{
			UE_LOG(LogEnemyAI, Verbose, TEXT("[State] 타겟 없음 -> 모든 전투 분기 종료 (%s)"), *EnemyAIDebugUtils::DescribeActor(ControlledPawn));
		}

		return;
	}

	const float DistanceToTarget = FVector::Distance(ControlledPawn->GetActorLocation(), TargetActor->GetActorLocation());
	const bool bHasLineOfSight = LineOfSightTo(TargetActor);
	const float PreferredRange = GetClampedBlackboardFloat(BlackboardComponent, EnemyBlackboardKeys::PreferredRange, 600.0f, 0.0f, 3000.0f);
	const float Aggression = GetClampedBlackboardFloat(BlackboardComponent, EnemyBlackboardKeys::Aggression, 0.5f, 0.0f, 1.0f);
	const float ChasePersistence = GetClampedBlackboardFloat(BlackboardComponent, EnemyBlackboardKeys::ChasePersistence, 0.5f, 0.0f, 1.0f);
	const float RetreatThreshold = GetClampedBlackboardFloat(BlackboardComponent, EnemyBlackboardKeys::RetreatThreshold, 0.35f, 0.0f, 1.0f);
	const float CoverPreference = GetClampedBlackboardFloat(BlackboardComponent, EnemyBlackboardKeys::CoverPreference, 0.35f, 0.0f, 1.0f);
	const FName EnemyMode = BlackboardComponent->GetValueAsName(EnemyBlackboardKeys::EnemyMode);

	const bool bModeForcesRetreat = EnemyMode == FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyMode::Retreat);
	const bool bModePrefersHold = EnemyMode == FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyMode::Hold);
	const bool bModePrefersPressure = EnemyMode == FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyMode::Pressure);

	// PreferredRange를 기준으로 공격 허용 구간을 만들고, 모드/성향에 따라 추적과 재배치를 나눈다.
	const float AttackWindow = FMath::Max(125.0f, FMath::Lerp(175.0f, 325.0f, Aggression));
	const float MinAttackRange = FMath::Max(0.0f, PreferredRange - AttackWindow);
	const float MaxAttackRange = PreferredRange + AttackWindow;
	const bool bInsideAttackBand = DistanceToTarget >= MinAttackRange && DistanceToTarget <= MaxAttackRange;
	const bool bTooClose = DistanceToTarget < MinAttackRange;
	const bool bTooFar = DistanceToTarget > MaxAttackRange;

	bool bShouldRetreat = bModeForcesRetreat || (HealthRatio <= RetreatThreshold);
	bool bCanAttack = !bShouldRetreat && bHasLineOfSight && bInsideAttackBand;
	bool bShouldReposition = false;
	bool bShouldChase = false;

	if (!bShouldRetreat && !bCanAttack)
	{
		const bool bCoverDrivenReposition = CoverPreference >= 0.55f && !bHasLineOfSight;
		const bool bRangeDrivenReposition = bTooClose || (bModePrefersHold && !bTooFar);

		bShouldReposition = bCoverDrivenReposition || bRangeDrivenReposition;
		bShouldChase = bTooFar || (!bHasLineOfSight && ChasePersistence >= 0.35f) || (bModePrefersPressure && !bHasLineOfSight);

		// 재배치와 추적이 동시에 참이면 방어적 성향은 재배치, 공격적 성향은 추적을 우선한다.
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

	BlackboardComponent->SetValueAsFloat(EnemyBlackboardKeys::DistanceToTarget, DistanceToTarget);
	BlackboardComponent->SetValueAsBool(EnemyBlackboardKeys::bHasLineOfSight, bHasLineOfSight);
	BlackboardComponent->SetValueAsBool(EnemyBlackboardKeys::bShouldRetreat, bShouldRetreat);
	BlackboardComponent->SetValueAsBool(EnemyBlackboardKeys::bCanAttack, bCanAttack);
	BlackboardComponent->SetValueAsBool(EnemyBlackboardKeys::bShouldReposition, bShouldReposition);
	BlackboardComponent->SetValueAsBool(EnemyBlackboardKeys::bShouldChase, bShouldChase);

	if (bPreviousShouldRetreat != bShouldRetreat
		|| bPreviousCanAttack != bCanAttack
		|| bPreviousShouldReposition != bShouldReposition
		|| bPreviousShouldChase != bShouldChase)
	{
		UE_LOG(
			LogEnemyAI,
			Verbose,
			TEXT("[State] Retreat=%d Attack=%d Reposition=%d Chase=%d Dist=%.0f Health=%.2f LOS=%d Pawn=%s"),
			bShouldRetreat ? 1 : 0,
			bCanAttack ? 1 : 0,
			bShouldReposition ? 1 : 0,
			bShouldChase ? 1 : 0,
			DistanceToTarget,
			HealthRatio,
			bHasLineOfSight ? 1 : 0,
			*EnemyAIDebugUtils::DescribeActor(ControlledPawn));
	}
}

float AEnemyAIController::GetCurrentHealthRatio() const
{
	const AGP_EnemyCharacter* EnemyCharacter = Cast<AGP_EnemyCharacter>(GetPawn());
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

void AEnemyAIController::ResolveBehaviorAssets(APawn* InPawn, UBehaviorTree*& OutBehaviorTreeAsset, UBlackboardData*& OutBlackboardAsset) const
{
	OutBehaviorTreeAsset = DefaultBehaviorTreeAsset;
	OutBlackboardAsset = DefaultBlackboardAsset;

	if (const AGP_EnemyCharacter* EnemyCharacter = Cast<AGP_EnemyCharacter>(InPawn))
	{
		// 적 액터에 직접 지정한 자산이 있으면 테스트 중 별도 AIController BP 없이도 바로 사용한다.
		if (!IsValid(OutBehaviorTreeAsset))
		{
			OutBehaviorTreeAsset = EnemyCharacter->GetBehaviorTreeAssetOverride();
		}

		if (!IsValid(OutBlackboardAsset))
		{
			OutBlackboardAsset = EnemyCharacter->GetBlackboardAssetOverride();
		}
	}

	if (!IsValid(OutBehaviorTreeAsset) && bUseSharedBehaviorAssetFallback)
	{
		// 현재 프로젝트의 공용 BT를 자동 연결해 테스트 진입 장벽을 낮춘다.
		OutBehaviorTreeAsset = LoadObject<UBehaviorTree>(nullptr, GetSharedBehaviorTreeFallbackPath());
		if (IsValid(OutBehaviorTreeAsset))
		{
			UE_LOG(LogEnemyAI, Log, TEXT("[AI] 공용 BT 자동 연결: %s"), *GetNameSafe(OutBehaviorTreeAsset));
		}
	}

	if (!IsValid(OutBlackboardAsset) && IsValid(OutBehaviorTreeAsset) && IsValid(OutBehaviorTreeAsset->BlackboardAsset))
	{
		OutBlackboardAsset = OutBehaviorTreeAsset->BlackboardAsset;
	}

	if (!IsValid(OutBlackboardAsset) && bUseSharedBehaviorAssetFallback)
	{
		// BT 에셋에 Blackboard가 연결되지 않았더라도 공용 Blackboard를 자동 연결한다.
		OutBlackboardAsset = LoadObject<UBlackboardData>(nullptr, GetSharedBlackboardFallbackPath());
		if (IsValid(OutBlackboardAsset))
		{
			UE_LOG(LogEnemyAI, Log, TEXT("[AI] 공용 Blackboard 자동 연결: %s"), *GetNameSafe(OutBlackboardAsset));
		}
	}
}

bool AEnemyAIController::ValidateSharedBlackboardSchema(UBlackboardComponent* BlackboardComponent)
{
	if (!IsValid(BlackboardComponent))
	{
		return false;
	}

	static const FName RequiredKeys[] =
	{
		EnemyBlackboardKeys::TargetActor,
		EnemyBlackboardKeys::EnemyMode,
		EnemyBlackboardKeys::Aggression,
		EnemyBlackboardKeys::PreferredRange,
		EnemyBlackboardKeys::RetreatThreshold,
		EnemyBlackboardKeys::ChasePersistence,
		EnemyBlackboardKeys::CoverPreference,
		EnemyBlackboardKeys::MoveToLocation,
		EnemyBlackboardKeys::FocusTargetRule,
		EnemyBlackboardKeys::bShouldRetreat,
		EnemyBlackboardKeys::bCanAttack,
		EnemyBlackboardKeys::bShouldReposition,
		EnemyBlackboardKeys::bShouldChase,
		EnemyBlackboardKeys::bHasLineOfSight,
		EnemyBlackboardKeys::DistanceToTarget,
		EnemyBlackboardKeys::HealthRatio,
	};

	TArray<FString> MissingKeys;
	for (const FName& KeyName : RequiredKeys)
	{
		if (BlackboardComponent->GetKeyID(KeyName) == FBlackboard::InvalidKey)
		{
			MissingKeys.Add(KeyName.ToString());
		}
	}

	if (MissingKeys.IsEmpty())
	{
		return true;
	}

	if (!bHasWarnedAboutMissingBlackboardKeys)
	{
		UE_LOG(
			LogEnemyAI,
			Warning,
			TEXT("[Blackboard] 공유 스키마 키가 부족합니다. BB_EnemyCommon에 다음 키를 추가하세요: %s"),
			*FString::Join(MissingKeys, TEXT(", ")));

		bHasWarnedAboutMissingBlackboardKeys = true;
	}

	return false;
}

void AEnemyAIController::ConfigureSightSense()
{
	if (!IsValid(EnemyPerceptionComponent) || !IsValid(SightSenseConfig))
	{
		return;
	}

	SightSenseConfig->SightRadius = SightRadius;
	SightSenseConfig->LoseSightRadius = FMath::Max(LoseSightRadius, SightRadius);
	SightSenseConfig->PeripheralVisionAngleDegrees = PeripheralVisionAngleDegrees;
	SightSenseConfig->SetMaxAge(SightMaxAge);
	SightSenseConfig->AutoSuccessRangeFromLastSeenLocation = AutoSuccessRangeFromLastSeenLocation;
	SightSenseConfig->DetectionByAffiliation.bDetectEnemies = bDetectEnemies;
	SightSenseConfig->DetectionByAffiliation.bDetectFriendlies = bDetectFriendlies;
	SightSenseConfig->DetectionByAffiliation.bDetectNeutrals = bDetectNeutrals;

	// 시야 센스를 공용 기본 센스로 등록해 여러 적이 같은 방식으로 대상을 감지하게 만든다.
	EnemyPerceptionComponent->ConfigureSense(*SightSenseConfig);
	EnemyPerceptionComponent->SetDominantSense(SightSenseConfig->GetSenseImplementation());
	EnemyPerceptionComponent->RequestStimuliListenerUpdate();
}

void AEnemyAIController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	UE_LOG(
		LogEnemyAI,
		Verbose,
		TEXT("[Perception] %s Target=%s %s"),
		*GetName(),
		*EnemyAIDebugUtils::DescribeActor(Actor),
		*EnemyAIDebugUtils::DescribeStimulus(Stimulus));

	// 개별 자극 결과에 직접 분기하기보다 현재 시점의 유효 타겟 전체를 다시 평가해 일관성을 유지한다.
	RefreshTargetActorFromPerception();
}

void AEnemyAIController::RefreshTargetActorFromPerception()
{
	SetBlackboardTargetActor(SelectBestTargetActorFromPerception());
	UpdateCombatStateBlackboard();
}

AActor* AEnemyAIController::SelectBestTargetActorFromPerception() const
{
	if (!IsValid(EnemyPerceptionComponent) || !IsValid(GetPawn()))
	{
		return nullptr;
	}

	TArray<AActor*> PerceivedActors;
	EnemyPerceptionComponent->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), PerceivedActors);

	AActor* BestTargetActor = nullptr;
	float BestDistanceSquared = TNumericLimits<float>::Max();

	for (AActor* CandidateActor : PerceivedActors)
	{
		if (!IsValidPerceptionTarget(CandidateActor))
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared(GetPawn()->GetActorLocation(), CandidateActor->GetActorLocation());
		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestTargetActor = CandidateActor;
		}
	}

	return BestTargetActor;
}

bool AEnemyAIController::IsValidPerceptionTarget(AActor* CandidateActor) const
{
	if (!IsValid(CandidateActor))
	{
		return false;
	}

	if (CandidateActor == GetPawn())
	{
		return false;
	}

	if (TargetActorClass && !CandidateActor->IsA(TargetActorClass))
	{
		return false;
	}

	if (bTargetOnlyPlayerControlledPawns)
	{
		const APawn* CandidatePawn = Cast<APawn>(CandidateActor);
		if (!IsValid(CandidatePawn) || !CandidatePawn->IsPlayerControlled())
		{
			return false;
		}
	}

	return true;
}

void AEnemyAIController::SetBlackboardTargetActor(AActor* NewTargetActor)
{
	UBlackboardComponent* BlackboardComponent = GetEnemyBlackboardComponent();
	if (!IsValid(BlackboardComponent))
	{
		return;
	}

	AActor* CurrentTargetActor = Cast<AActor>(BlackboardComponent->GetValueAsObject(EnemyBlackboardKeys::TargetActor));
	if (CurrentTargetActor == NewTargetActor)
	{
		return;
	}

	if (!IsValid(CurrentTargetActor) && IsValid(NewTargetActor))
	{
		UE_LOG(LogEnemyAI, Log, TEXT("[Perception] 타겟 획득: %s"), *EnemyAIDebugUtils::DescribeActor(NewTargetActor));
	}
	else if (IsValid(CurrentTargetActor) && !IsValid(NewTargetActor))
	{
		UE_LOG(LogEnemyAI, Log, TEXT("[Perception] 타겟 상실: %s"), *EnemyAIDebugUtils::DescribeActor(CurrentTargetActor));
	}
	else
	{
		UE_LOG(
			LogEnemyAI,
			Log,
			TEXT("[Perception] 타겟 변경: %s -> %s"),
			*EnemyAIDebugUtils::DescribeActor(CurrentTargetActor),
			*EnemyAIDebugUtils::DescribeActor(NewTargetActor));
	}

	if (IsValid(NewTargetActor))
	{
		// 감지에 성공한 가장 적합한 대상을 공유 Blackboard의 TargetActor 키에 기록한다.
		BlackboardComponent->SetValueAsObject(EnemyBlackboardKeys::TargetActor, NewTargetActor);
	}
	else
	{
		// 현재 감지 중인 유효 타겟이 없으면 Blackboard에서 안전하게 타겟을 비운다.
		BlackboardComponent->ClearValue(EnemyBlackboardKeys::TargetActor);
	}
}
