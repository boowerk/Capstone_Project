#include "Characters/GP_EnemyCharacter.h"

#include "AI/Controllers/EnemyAIController.h"
#include "AIController.h"
#include "AI/Data/EnemyArchetypeData.h"
#include "AI/Data/EnemyLLMEvaluation.h"
#include "AI/Debug/EnemyAIRangeVisualizationComponent.h"
#include "BrainComponent.h"
#include "AbilitySystem/Abilities/Enemy/GP_BossAreaAttack.h"
#include "AbilitySystem/Abilities/Enemy/GP_BossBasicAttack.h"
#include "AbilitySystem/Abilities/Enemy/GP_BossGroundHandsAttack.h"
#include "AbilitySystem/Abilities/Enemy/GP_BossHeavyAttack.h"
#include "AbilitySystem/Abilities/Enemy/GP_BossSummonAdds.h"
#include "AbilitySystem/Abilities/Enemy/GP_BossSweepAttack.h"
#include "AbilitySystem/Abilities/Enemy/GP_EnemyAttack.h"
#include "AbilitySystem/Abilities/Enemy/GP_EnemyDeathAbility.h"
#include "AbilitySystem/GP_AbilitySystemComponent.h"
#include "AbilitySystem/GP_AttributeSet.h"
#include "Animation/AnimInstance.h"
#include "Animation/PDA_EnemyAnimationSet.h"
#include "Components/CapsuleComponent.h"
#include "Engine/DataTable.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameplayTags/GP_Tags.h"
#include "Net/UnrealNetwork.h"
#include "Player/GP_PlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "Game/Corruption/GP_EnemyCorruptionComponent.h"
#include "UI/GP_AttributeWidget.h"
#include "UI/GP_WidgetComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "VFX/GP_BossDeathPresentationComponent.h"
#include "VFX/GP_BossTargetMarkerVFXComponent.h"

AGP_EnemyCharacter::AGP_EnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	// Blueprint-only regular enemies (including FurnaceWalker) otherwise inherit
	// CharacterMovement's unspecified turn defaults.  Keep their body aligned
	// with the chase path at a responsive, but not snapping, rate.  Enemy
	// subclasses can still override this in their own constructors.
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->bOrientRotationToMovement = true;
		MovementComponent->bUseControllerDesiredRotation = false;
		MovementComponent->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	}

	AbilitySystemComponent = CreateDefaultSubobject<UGP_AbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	AttributeSet = CreateDefaultSubobject<UGP_AttributeSet>(TEXT("AttributeSet"));

	DefaultEnemyAttackAbilityClass = UGP_EnemyAttack::StaticClass();
	DefaultAttackAbilityTag = GPTags::Ability::Enemy::Attack_Melee;
	DefaultEnemyDeathAbilityClass = UGP_EnemyDeathAbility::StaticClass();

	BossTargetMarkerVFXComponent = CreateDefaultSubobject<UGP_BossTargetMarkerVFXComponent>(TEXT("BossTargetMarkerVFXComponent"));

	// Bosses use this dormant component to convert the shared GAS death state into a boss-specific clear effect.
	BossDeathPresentationComponent = CreateDefaultSubobject<UGP_BossDeathPresentationComponent>(TEXT("BossDeathPresentationComponent"));
	BossDeathPresentationComponent->SetupAttachment(GetRootComponent());

	WorldHealthBarComponent = CreateDefaultSubobject<UGP_WidgetComponent>(TEXT("WorldHealthBarComponent"));
	WorldHealthBarComponent->SetupAttachment(GetRootComponent());
	WorldHealthBarComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 135.0f));
	WorldHealthBarComponent->SetWidgetSpace(EWidgetSpace::Screen);
	WorldHealthBarComponent->SetDrawAtDesiredSize(true);
	WorldHealthBarComponent->SetPivot(FVector2D(0.5f, 1.0f));
	WorldHealthBarComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WorldHealthBarComponent->SetGenerateOverlapEvents(false);
	WorldHealthBarComponent->SetCastShadow(false);

	// Every native and Blueprint enemy inherits the same GAS-backed corruption scaling adapter.
	EnemyCorruptionComponent = CreateDefaultSubobject<UGP_EnemyCorruptionComponent>(TEXT("EnemyCorruptionComponent"));

	// Existing Blueprint enemies receive the shared health bar asset through their native parent automatically.
	static ConstructorHelpers::FClassFinder<UGP_AttributeWidget> EnemyHealthBarFinder(TEXT("/Game/UI/WBP_EnemyHealthBar"));
	if (EnemyHealthBarFinder.Succeeded())
	{
		WorldHealthBarComponent->SetWidgetClass(EnemyHealthBarFinder.Class);
	}

	// 적은 배치/스폰 시 공용 AIController를 자동 점유해 BT/Blackboard 초기화를 컨트롤러에 위임한다.
	AIControllerClass = AEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

#if WITH_EDITORONLY_DATA
	AIRangeVisualizer = CreateEditorOnlyDefaultSubobject<UEnemyAIRangeVisualizationComponent>(TEXT("AIRangeVisualizer"));
	if (AIRangeVisualizer != nullptr)
	{
		AIRangeVisualizer->SetupAttachment(GetRootComponent());
	}

	// Editor-only shapes make the gameplay ranges visible without adding runtime collision.
	RefreshAIRangeVisualizers();
#endif
}

UAttributeSet* AGP_EnemyCharacter::GetAttributeSet() const
{
	return AttributeSet;
}

void AGP_EnemyCharacter::UpdateAnimationSet()
{
	if (!IsValid(EnemyAnimationSet))
	{
		// Existing enemies and bosses can continue using the legacy shared asset until migrated.
		Super::UpdateAnimationSet();
		return;
	}

	if (IsValid(EnemyAnimationSet->CharacterMesh))
	{
		GetMesh()->SetSkeletalMeshAsset(EnemyAnimationSet->CharacterMesh);
	}

	if (EnemyAnimationSet->AnimBlueprintClass)
	{
		GetMesh()->SetAnimInstanceClass(EnemyAnimationSet->AnimBlueprintClass);
	}
}

UAbilitySystemComponent* AGP_EnemyCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

FVector AGP_EnemyCharacter::GetBehaviorAnchorLocation() const
{
	// AI possession can ask for the anchor before BeginPlay, so compute the editor-authored point on demand.
	return bHasBehaviorAnchorLocation ? BehaviorAnchorLocation : GetActorTransform().TransformPosition(BehaviorAnchorOffset);
}

void AGP_EnemyCharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	RefreshWorldHealthBarVisibility();

	// Keep editor range rings in sync when designers move the anchor or change range values.
	RefreshAIRangeVisualizers();
}

bool AGP_EnemyCharacter::BuildInitialEnemyEvaluation(FEnemyLLMEvaluation& OutEvaluation) const
{
	if (const FEnemyArchetypeTuning* ArchetypeTuning = ResolveEnemyArchetypeTuning())
	{
		OutEvaluation = ArchetypeTuning->BuildEvaluation(ResolvePersonalitySeed());
		return true;
	}

	// 데이터가 없더라도 AI가 깨지지 않도록 안전한 기본값을 사용한다.
	OutEvaluation = FEnemyLLMEvaluation::MakeSafeDefault();
	return false;
}

FText AGP_EnemyCharacter::GetBossDisplayName() const
{
	if (!BossDisplayName.IsEmpty())
	{
		return BossDisplayName;
	}

	return FText::FromString(GetName());
}

void AGP_EnemyCharacter::NotifyBossTargetSelected(AActor* TargetActor)
{
	if (!bIsBossEnemy || bIsDead || !IsValid(BossTargetMarkerVFXComponent))
	{
		return;
	}

	// Keep the AIController free of Niagara details; boss pawns own how their selected target is presented.
	BossTargetMarkerVFXComponent->PlayTargetMarker(TargetActor);
}

void AGP_EnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
	RefreshWorldHealthBarVisibility();
	InitializeBasicEnemyAttackCadence();

	if (IsValid(EnemyAnimationSet))
	{
		if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
		{
			// Enemy attack movement is authored in lower-body montage root motion.
			AnimInstance->SetRootMotionMode(ERootMotionMode::RootMotionFromMontagesOnly);
		}
	}

	if (!IsValid(GetAbilitySystemComponent()))
	{
		return;
	}

	GetAbilitySystemComponent()->InitAbilityActorInfo(this, this);
	OnASCInitialized.Broadcast(GetAbilitySystemComponent(), GetAttributeSet());
	BindMoveSpeedAttribute();
	if (UGP_AttributeSet* EnemyAttributeSet = Cast<UGP_AttributeSet>(GetAttributeSet()))
	{
		// AttributeSet publishes the terminal health event; the enemy translates it into a GAS death request.
		EnemyAttributeSet->OnOutOfHealth.AddUniqueDynamic(this, &ThisClass::HandleOutOfHealth);
	}

	if (!HasAuthority())
	{
		return;
	}

	GiveStartupAbilities();
	GiveDefaultEnemyDeathAbility();
	GiveDefaultEnemyAttackAbility();
	if (bIsBossEnemy && bGrantDefaultBossPatternAbilities)
	{
		// Boss-specific defaults keep Sans prototypes playable even before designers add custom BP abilities.
		GiveDefaultBossPatternAbilities();
	}
	InitializeAttributes();
	if (IsValid(EnemyCorruptionComponent))
	{
		// Apply corruption after base attributes exist so the infinite effect remains an independent modifier.
		EnemyCorruptionComponent->InitializeFromOwner();
	}

	const float AttributeMoveSpeed = GetAbilitySystemComponent()->GetNumericAttribute(UGP_AttributeSet::GetMoveSpeedAttribute());
	if (AttributeMoveSpeed > KINDA_SMALL_NUMBER)
	{
		if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
		{
			MovementComponent->MaxWalkSpeed = AttributeMoveSpeed;
		}
	}
	else if (const UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		GetAbilitySystemComponent()->SetNumericAttributeBase(
			UGP_AttributeSet::GetMoveSpeedAttribute(),
			MovementComponent->MaxWalkSpeed);
	}

	// 기준 위치는 캐릭터가 저장하고, 실제 Blackboard/Behavior Tree 시작은 AEnemyAIController::OnPossess에서 담당한다.
	BehaviorAnchorLocation = GetActorTransform().TransformPosition(BehaviorAnchorOffset);
	bHasBehaviorAnchorLocation = true;
}

void AGP_EnemyCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindMoveSpeedAttribute();
	if (IsValid(BossTargetMarkerVFXComponent))
	{
		// Target marker VFX is attached to the player, so clear it explicitly when the boss leaves the world.
		BossTargetMarkerVFXComponent->HandleOwnerDeath();
	}

	Super::EndPlay(EndPlayReason);
}

bool AGP_EnemyCharacter::IsBasicEnemyAttackReady() const
{
	if (bIsBossEnemy)
	{
		// Boss pattern selectors own their cadence and must not inherit regular-enemy timing.
		return true;
	}

	const UWorld* World = GetWorld();
	return IsValid(World)
		&& !bBasicEnemyAttackInProgress
		&& EnemyAttackCadencePolicy::IsReady(World->GetTimeSeconds(), BasicEnemyAttackReadyTimeSeconds);
}

float AGP_EnemyCharacter::ScheduleNextBasicEnemyAttack()
{
	if (bIsBossEnemy || !HasAuthority())
	{
		return 0.0f;
	}

	const FVector2D DelayRange = EnemyAttackCadencePolicy::SanitizeDelayRange(
		AttackCadenceSettings.NextAttackDelayMinSeconds,
		AttackCadenceSettings.NextAttackDelayMaxSeconds);
	const float SelectedDelay = EnemyAttackCadencePolicy::RollDelay(DelayRange, AttackCadenceRandomStream);
	if (const UWorld* World = GetWorld())
	{
		// Only the authoritative enemy owns AI decisions, so this timestamp does not need replication.
		BasicEnemyAttackReadyTimeSeconds = World->GetTimeSeconds() + SelectedDelay;
	}
	return SelectedDelay;
}

void AGP_EnemyCharacter::InitializeBasicEnemyAttackCadence()
{
	if (bIsBossEnemy || !HasAuthority())
	{
		return;
	}

	// Actor names/IDs differ across spawned instances, preventing identical random streams in an encounter group.
	AttackCadenceRandomStream.Initialize(HashCombineFast(GetTypeHash(GetFName()), GetUniqueID()));
	const FVector2D InitialDelayRange = EnemyAttackCadencePolicy::SanitizeDelayRange(
		AttackCadenceSettings.InitialDelayMinSeconds,
		AttackCadenceSettings.InitialDelayMaxSeconds);
	const float InitialDelay = EnemyAttackCadencePolicy::RollDelay(InitialDelayRange, AttackCadenceRandomStream);
	if (const UWorld* World = GetWorld())
	{
		BasicEnemyAttackReadyTimeSeconds = World->GetTimeSeconds() + InitialDelay;
	}
}

void AGP_EnemyCharacter::BindMoveSpeedAttribute()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!IsValid(ASC) || MoveSpeedAttributeDelegateHandle.IsValid())
	{
		return;
	}

	MoveSpeedAttributeDelegateHandle = ASC
		->GetGameplayAttributeValueChangeDelegate(UGP_AttributeSet::GetMoveSpeedAttribute())
		.AddUObject(this, &ThisClass::HandleMoveSpeedAttributeChanged);
}

void AGP_EnemyCharacter::UnbindMoveSpeedAttribute()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!IsValid(ASC) || !MoveSpeedAttributeDelegateHandle.IsValid())
	{
		return;
	}

	ASC->GetGameplayAttributeValueChangeDelegate(UGP_AttributeSet::GetMoveSpeedAttribute())
		.Remove(MoveSpeedAttributeDelegateHandle);
	MoveSpeedAttributeDelegateHandle.Reset();
}

void AGP_EnemyCharacter::HandleMoveSpeedAttributeChanged(const FOnAttributeChangeData& ChangeData)
{
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->MaxWalkSpeed = FMath::Max(ChangeData.NewValue, 0.0f);
	}
}

const FEnemyArchetypeTuning* AGP_EnemyCharacter::ResolveEnemyArchetypeTuning() const
{
	if (IsValid(EnemyArchetypeData))
	{
		return &EnemyArchetypeData->Tuning;
	}

	if (EnemyArchetypeRow.DataTable != nullptr && EnemyArchetypeRow.RowName != NAME_None)
	{
		const FEnemyArchetypeTableRow* ArchetypeRow = EnemyArchetypeRow.GetRow<FEnemyArchetypeTableRow>(TEXT("AGP_EnemyCharacter::ResolveEnemyArchetypeTuning"));
		return ArchetypeRow != nullptr ? &ArchetypeRow->Tuning : nullptr;
	}

	if (bUseBuiltInArchetypeTuning)
	{
		return &BuiltInArchetypeTuning;
	}

	return nullptr;
}

int32 AGP_EnemyCharacter::ResolvePersonalitySeed() const
{
	if (bOverridePersonalitySeed)
	{
		return PersonalitySeedOverride;
	}

	// 같은 아키타입이라도 배치 위치와 이름이 다르면 미세한 개성 차이가 생기도록 시드를 만든다.
	return static_cast<int32>(
		HashCombineFast(
			GetTypeHash(GetFName()),
			HashCombineFast(GetTypeHash(GetActorLocation()), GetUniqueID())));
}

void AGP_EnemyCharacter::GiveDefaultBossPatternAbilities()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!IsValid(ASC))
	{
		return;
	}

	const TSubclassOf<UGameplayAbility> BossPatternAbilities[] =
	{
		UGP_BossBasicAttack::StaticClass(),
		UGP_BossHeavyAttack::StaticClass(),
		UGP_BossSweepAttack::StaticClass(),
		UGP_BossAreaAttack::StaticClass(),
		UGP_BossGroundHandsAttack::StaticClass(),
		UGP_BossSummonAdds::StaticClass(),
	};

	for (const TSubclassOf<UGameplayAbility>& AbilityClass : BossPatternAbilities)
	{
		if (!*AbilityClass || ASC->FindAbilitySpecFromClass(AbilityClass) != nullptr)
		{
			continue;
		}

		// Grant each default once; custom Blueprint abilities can still be added through StartupAbilities.
		ASC->GiveAbility(FGameplayAbilitySpec(AbilityClass));
	}
}

void AGP_EnemyCharacter::GiveDefaultEnemyAttackAbility()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!bGrantDefaultEnemyAttackAbility || !IsValid(ASC) || !*DefaultEnemyAttackAbilityClass)
	{
		return;
	}

	// Blueprint StartupAbilities can override this; the default grant only fills an otherwise missing basic attack.
	if (ASC->FindAbilitySpecFromClass(DefaultEnemyAttackAbilityClass) == nullptr)
	{
		ASC->GiveAbility(FGameplayAbilitySpec(DefaultEnemyAttackAbilityClass));
	}
}

void AGP_EnemyCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGP_EnemyCharacter, bIsDead);
	DOREPLIFETIME(AGP_EnemyCharacter, DeathInstigatorActor);
}

void AGP_EnemyCharacter::GiveDefaultEnemyDeathAbility()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!bGrantDefaultEnemyDeathAbility || !IsValid(ASC) || !DefaultEnemyDeathAbilityClass)
	{
		return;
	}

	for (const FGameplayAbilitySpec& AbilitySpec : ASC->GetActivatableAbilities())
	{
		if (IsValid(AbilitySpec.Ability)
			&& AbilitySpec.Ability->GetAssetTags().HasTagExact(GPTags::Ability::Enemy::Death))
		{
			// A tag-compatible StartupAbility is the extension point for a future animated death ability.
			return;
		}
	}

	ASC->GiveAbility(FGameplayAbilitySpec(DefaultEnemyDeathAbilityClass));
}

void AGP_EnemyCharacter::HandlePostDamageTaken(AActor* InstigatorActor, float DamageAmount, FGameplayTag ElementTag)
{
	Super::HandlePostDamageTaken(InstigatorActor, DamageAmount, ElementTag);

	const UGP_AttributeSet* GPAttributeSet = Cast<UGP_AttributeSet>(GetAttributeSet());
	if (!HasAuthority() || bXPRewardGranted || !IsValid(GPAttributeSet) || GPAttributeSet->GetHealth() > KINDA_SMALL_NUMBER)
	{
		return;
	}

	bXPRewardGranted = true;
	GrantXPRewardToInstigator(InstigatorActor);

	// Same first-death gate as XP: let the GameMode count this enemy toward the current zone clear.
	OnEnemyDied.Broadcast(this);
}

void AGP_EnemyCharacter::HandleOutOfHealth(AActor* InstigatorActor, AActor* TargetActor)
{
	if (TargetActor != this || !HasAuthority())
	{
		return;
	}

	RequestDeath(InstigatorActor);
}

void AGP_EnemyCharacter::RequestDeath(AActor* InstigatorActor)
{
	if (!HasAuthority() || bIsDead || bDeathRequested)
	{
		return;
	}

	bDeathRequested = true;
	DeathInstigatorActor = InstigatorActor;
	UGP_AbilitySystemComponent* ASC = Cast<UGP_AbilitySystemComponent>(GetAbilitySystemComponent());
	const bool bDeathAbilityStarted = IsValid(ASC)
		&& ASC->TryActivateAbilityByTag(GPTags::Ability::Enemy::Death);
	if (!bDeathAbilityStarted)
	{
		// Death is a mandatory invariant, so a missing/misconfigured custom ability cannot leave a zero-health enemy alive.
		UE_LOG(LogTemp, Warning, TEXT("[EnemyDeath] Death ability failed; applying native fallback. Enemy=%s"), *GetNameSafe(this));
		EnterDeathStateFromAbility();
	}
}

void AGP_EnemyCharacter::EnterDeathStateFromAbility()
{
	if (!HasAuthority() || bIsDead)
	{
		return;
	}

	bIsDead = true;
	bDeathRequested = true;
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		// The persistent loose tag remains after the short death ability ends and identifies the terminal state to GAS queries.
		ASC->AddLooseGameplayTag(GPTags::Ability::Enemy::Death);
	}

	ApplyDeathState();
	ForceNetUpdate();
}

void AGP_EnemyCharacter::OnRep_IsDead()
{
	if (bIsDead)
	{
		ApplyDeathState();
	}
}

void AGP_EnemyCharacter::ApplyDeathState()
{
	if (bDeathStateApplied)
	{
		return;
	}

	bDeathStateApplied = true;
	if (IsValid(EnemyCorruptionComponent))
	{
		// Only bosses mutate world corruption; the component guards authority and duplicate death presentation calls.
		EnemyCorruptionComponent->HandleOwnerDeath(bIsBossEnemy);
	}
	if (IsValid(BossTargetMarkerVFXComponent))
	{
		// Death must revoke selected-target presentation before delayed despawn can leave the boss corpse around.
		BossTargetMarkerVFXComponent->HandleOwnerDeath();
	}
	if (IsValid(BossDeathPresentationComponent))
	{
		// Presentation is local-only and no-ops for regular enemies, keeping the GAS death invariant centralised here.
		BossDeathPresentationComponent->PlayDeathPresentation(DeathInstigatorActor);
	}

	RefreshWorldHealthBarVisibility();
	SetCanBeDamaged(false);
	SetActorTickEnabled(false);
	SetActorEnableCollision(false);

	if (UCapsuleComponent* EnemyCapsule = GetCapsuleComponent())
	{
		EnemyCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->DisableMovement();
	}

	// Notify presentation before scheduling destruction so a zero-delay setup can still react safely.
	OnEnemyDeathStarted.Broadcast(this, DeathInstigatorActor);
	BP_OnDeathStarted(DeathInstigatorActor);

	if (HasAuthority())
	{
		if (AAIController* AIController = Cast<AAIController>(GetController()))
		{
			AIController->StopMovement();
			if (UBrainComponent* BrainComponent = AIController->GetBrainComponent())
			{
				// Stop the Behavior Tree before detaching so no task can enqueue another attack during the corpse delay.
				BrainComponent->StopLogic(TEXT("Enemy health reached zero"));
			}
		}

		DetachFromControllerPendingDestroy();
		if (DeathDespawnDelay <= KINDA_SMALL_NUMBER)
		{
			Destroy();
		}
		else
		{
			SetLifeSpan(DeathDespawnDelay);
		}
	}
}

void AGP_EnemyCharacter::SetCorruptionRegionId(int32 RegionId)
{
	if (IsValid(EnemyCorruptionComponent))
	{
		EnemyCorruptionComponent->SetCorruptionRegionId(RegionId);
	}
}

void AGP_EnemyCharacter::RefreshWorldHealthBarVisibility()
{
	if (!IsValid(WorldHealthBarComponent))
	{
		return;
	}

	// Bosses already have a dedicated HUD bar, while dead enemies must not leave an orphaned screen-space bar.
	WorldHealthBarComponent->SetVisibility(bShowWorldHealthBar && !bIsBossEnemy && !bIsDead, true);
}

void AGP_EnemyCharacter::GrantXPRewardToInstigator(AActor* InstigatorActor)
{
	if (XPReward <= 0.0f)
	{
		return;
	}

	// Shared-XP co-op: every player in the match gains the full reward, not just
	// the one who landed the killing blow. Server-authoritative (callers guard on
	// HasAuthority), so this loops the replicated PlayerArray once.
	const UWorld* World = GetWorld();
	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	if (!GameState)
	{
		// Fall back to the instigator alone if the game state isn't available yet.
		if (AGP_PlayerState* InstigatorPlayerState = ResolveInstigatorPlayerState(InstigatorActor))
		{
			InstigatorPlayerState->AddXP(XPReward);
		}
		return;
	}

	for (APlayerState* PartyPlayerState : GameState->PlayerArray)
	{
		if (AGP_PlayerState* GPPlayerState = Cast<AGP_PlayerState>(PartyPlayerState))
		{
			GPPlayerState->AddXP(XPReward);
		}
	}
}

AGP_PlayerState* AGP_EnemyCharacter::ResolveInstigatorPlayerState(AActor* InstigatorActor) const
{
	if (!IsValid(InstigatorActor))
	{
		return nullptr;
	}

	if (AGP_PlayerState* InstigatorPlayerState = Cast<AGP_PlayerState>(InstigatorActor))
	{
		return InstigatorPlayerState;
	}

	if (const APawn* InstigatorPawn = Cast<APawn>(InstigatorActor))
	{
		if (AController* InstigatorPawnController = InstigatorPawn->GetController())
		{
			return InstigatorPawnController->GetPlayerState<AGP_PlayerState>();
		}
	}

	if (const AController* InstigatorController = Cast<AController>(InstigatorActor))
	{
		return InstigatorController->GetPlayerState<AGP_PlayerState>();
	}

	if (AController* InstigatorController = InstigatorActor->GetInstigatorController())
	{
		return InstigatorController->GetPlayerState<AGP_PlayerState>();
	}

	return nullptr;
}

void AGP_EnemyCharacter::RefreshAIRangeVisualizers()
{
#if WITH_EDITORONLY_DATA
	if (AIRangeVisualizer != nullptr)
	{
		// The anchor offset is the same local point used by HomeLocation, so the rings preview runtime behavior.
		AIRangeVisualizer->SetRelativeLocation(BehaviorAnchorOffset);
		AIRangeVisualizer->ConfigureRanges(
			GetReturnHomeDistance(),
			GetPatrolRadius(),
			GetSightRadius(),
			GetLoseSightRadius(),
			GetPeripheralVisionAngleDegrees());
		AIRangeVisualizer->ConfigureVisibility(bShowAIRangesInEditor, bDrawAIRangesOnlyWhenSelected);
	}
#endif
}
