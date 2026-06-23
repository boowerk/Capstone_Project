#include "Characters/GP_DarkArmorKnightBossCharacter.h"

#include "AbilitySystem/GP_AbilitySystemComponent.h"
#include "AbilitySystem/GP_AttributeSet.h"
#include "AbilitySystem/Abilities/Enemy/GP_DarkKnightPatternAbility.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Actors/GP_BossCombatUtils.h"
#include "Actors/GP_DarkKnightChargeActor.h"
#include "Actors/GP_DarkKnightGroundCrackActor.h"
#include "Actors/GP_DarkWaveProjectile.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardData.h"
#include "BrainComponent.h"
#include "Characters/GP_DarkArmorKnightStateComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameplayEffect.h"
#include "GameplayTags/GP_Tags.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "VFX/GP_BossTelegraphVFXComponent.h"

AGP_DarkArmorKnightBossCharacter::AGP_DarkArmorKnightBossCharacter()
{
	bIsBossEnemy = true;
	bShowWorldHealthBar = false;
	bGrantDefaultBossPatternAbilities = false;
	bGrantDefaultEnemyAttackAbility = false;
	BossDisplayName = NSLOCTEXT("GPDarkArmorKnightBoss", "BossDisplayName", "Dark Armor Knight");

	DarkKnightStateComponent = CreateDefaultSubobject<UGP_DarkArmorKnightStateComponent>(TEXT("DarkKnightStateComponent"));
	// Prefill every authored Dark Knight attack so Blueprint defaults expose one checkbox per pattern.
	TelegraphVFXPatterns.Add(GPTags::Ability::Boss::DarkKnight::Basic, false);
	TelegraphVFXPatterns.Add(GPTags::Ability::Boss::DarkKnight::Heavy, false);
	TelegraphVFXPatterns.Add(GPTags::Ability::Boss::DarkKnight::Sweep, false);
	TelegraphVFXPatterns.Add(GPTags::Ability::Boss::DarkKnight::Guard, false);
	TelegraphVFXPatterns.Add(GPTags::Ability::Boss::DarkKnight::Counter, false);
	TelegraphVFXPatterns.Add(GPTags::Ability::Boss::DarkKnight::Charge, false);
	TelegraphVFXPatterns.Add(GPTags::Ability::Boss::DarkKnight::DarkWave, false);
	TelegraphVFXPatterns.Add(GPTags::Ability::Boss::DarkKnight::GroundCrack, false);
	DarkWaveProjectileClass = AGP_DarkWaveProjectile::StaticClass();
	GroundCrackActorClass = AGP_DarkKnightGroundCrackActor::StaticClass();
	ChargeActorClass = AGP_DarkKnightChargeActor::StaticClass();
	static ConstructorHelpers::FObjectFinder<UAnimMontage> PreAttackMontageFinder(TEXT("/Game/Characters/EnemyCharacter/Boss/BP_Boss_DarkArmorKnight/Animations/AM_DK_Windup.AM_DK_Windup"));
	if (PreAttackMontageFinder.Succeeded())
	{
		PreAttackMontage = PreAttackMontageFinder.Object;
	}

	DarkKnightAbilityClasses =
	{
		UGP_DarkKnightBasicAbility::StaticClass(),
		UGP_DarkKnightHeavyAbility::StaticClass(),
		UGP_DarkKnightChargeAbility::StaticClass(),
		UGP_DarkKnightDarkWaveAbility::StaticClass(),
		UGP_DarkKnightGroundCrackAbility::StaticClass(),
		UGP_DarkKnightGroggyAbility::StaticClass(),
	};

	static ConstructorHelpers::FObjectFinder<UBehaviorTree> BehaviorTreeFinder(TEXT("/Game/Characters/EnemyCharacter/BT/Boss/BT_BossCommon.BT_BossCommon"));
	if (BehaviorTreeFinder.Succeeded())
	{
		BehaviorTreeAssetOverride = BehaviorTreeFinder.Object;
	}
	static ConstructorHelpers::FObjectFinder<UBlackboardData> BlackboardFinder(TEXT("/Game/Characters/EnemyCharacter/BT/Boss/BB_BossCommon.BB_BossCommon"));
	if (BlackboardFinder.Succeeded())
	{
		BlackboardAssetOverride = BlackboardFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> KnightMeshFinder(TEXT("/Game/Meshes/Monsters/Knight/SK_KnightBoss.SK_KnightBoss"));
	if (KnightMeshFinder.Succeeded())
	{
		// The supplied knightBoss asset is skeletal, so the character mesh owns it directly.
		GetMesh()->SetSkeletalMesh(KnightMeshFinder.Object);
	}

	static ConstructorHelpers::FClassFinder<UGameplayEffect> DamageEffectFinder(TEXT("/Game/GAS_Pattern/AbilitySystem/GameplayEffects/Damage/GE_PrimaryDamage"));
	if (DamageEffectFinder.Succeeded())
	{
		DamageEffectClass = DamageEffectFinder.Class;
	}

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = 360.0f;
		Movement->RotationRate = FRotator(0.0f, 240.0f, 0.0f);
	}
}

void AGP_DarkArmorKnightBossCharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// Reapply common assets after serialized Blueprint defaults so patrol/chase/leash behavior remains inherited.
	BehaviorTreeAssetOverride = LoadObject<UBehaviorTree>(nullptr, TEXT("/Game/Characters/EnemyCharacter/BT/Boss/BT_BossCommon.BT_BossCommon"));
	BlackboardAssetOverride = LoadObject<UBlackboardData>(nullptr, TEXT("/Game/Characters/EnemyCharacter/BT/Boss/BB_BossCommon.BB_BossCommon"));
}

void AGP_DarkArmorKnightBossCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (IsValid(DarkKnightStateComponent))
	{
		DarkKnightStateComponent->OnGroggyChanged.AddUniqueDynamic(this, &ThisClass::HandleGroggyChanged);
	}
	if (!HasAuthority())
	{
		return;
	}
	if (IsValid(DarkKnightStateComponent))
	{
		DarkKnightStateComponent->InitializeDarkKnightState(this);
	}
	GrantDarkKnightAbilities();
}

void AGP_DarkArmorKnightBossCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		for (FTimerHandle& TimerHandle : PatternTimerHandles)
		{
			World->GetTimerManager().ClearTimer(TimerHandle);
		}
	}
	PatternTimerHandles.Reset();
	Super::EndPlay(EndPlayReason);
}

UGP_BossTelegraphVFXComponent* AGP_DarkArmorKnightBossCharacter::GetBossTelegraphVFXComponent() const
{
	// BP_DarkArmorKnight already owns the designer-added component, so reuse it instead of creating a duplicate native node.
	return FindComponentByClass<UGP_BossTelegraphVFXComponent>();
}

bool AGP_DarkArmorKnightBossCharacter::IsBossTelegraphEnabledForPattern(FGameplayTag PatternTag) const
{
	const UGP_BossTelegraphVFXComponent* TelegraphComponent = GetBossTelegraphVFXComponent();
	return IsValid(TelegraphComponent)
		&& TelegraphComponent->IsPatternTelegraphEnabled(PatternTag, TelegraphVFXPatterns);
}

float AGP_DarkArmorKnightBossCharacter::PlayBossTelegraphForPattern(FGameplayTag PatternTag)
{
	UGP_BossTelegraphVFXComponent* TelegraphComponent = GetBossTelegraphVFXComponent();
	return IsValid(TelegraphComponent)
		? TelegraphComponent->PlayPatternTelegraph(PatternTag, TelegraphVFXPatterns)
		: 0.0f;
}

int32 AGP_DarkArmorKnightBossCharacter::GetDarkKnightPhase() const
{
	const UGP_AttributeSet* Attributes = Cast<UGP_AttributeSet>(GetAttributeSet());
	const float HealthRatio = IsValid(Attributes) && Attributes->GetMaxHealth() > KINDA_SMALL_NUMBER
		? Attributes->GetHealth() / Attributes->GetMaxHealth()
		: 1.0f;
	return HealthRatio <= 0.25f ? 3 : (HealthRatio <= 0.6f ? 2 : 1);
}

bool AGP_DarkArmorKnightBossCharacter::CanStartDarkKnightPattern() const
{
	return GetWorld() == nullptr || GetWorld()->GetTimeSeconds() - LastPatternStartTime >= FMath::Max(0.0f, MinimumPatternInterval);
}

bool AGP_DarkArmorKnightBossCharacter::TryStartDarkKnightPattern()
{
	if (!HasAuthority() || !CanStartDarkKnightPattern())
	{
		return false;
	}
	LastPatternStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : LastPatternStartTime;
	return true;
}

bool AGP_DarkArmorKnightBossCharacter::IsPatternCooldownReady(FGameplayTag PatternTag) const
{
	const float* LastUseTime = LastPatternUseTimes.Find(PatternTag);
	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	return !LastUseTime || CurrentTime - *LastUseTime >= ResolvePatternCooldown(PatternTag);
}

void AGP_DarkArmorKnightBossCharacter::RequestCounterAttackAbility(AActor* CounterTarget)
{
	if (!HasAuthority())
	{
		return;
	}
	PendingCounterTarget = CounterTarget;
	if (UGP_AbilitySystemComponent* ASC = Cast<UGP_AbilitySystemComponent>(GetAbilitySystemComponent()))
	{
		ASC->TryActivateAbilityByTag(GPTags::Ability::Boss::DarkKnight::Counter);
	}
}

void AGP_DarkArmorKnightBossCharacter::RequestEnterGroggyAbility()
{
	if (!HasAuthority())
	{
		return;
	}
	bool bActivated = false;
	if (UGP_AbilitySystemComponent* ASC = Cast<UGP_AbilitySystemComponent>(GetAbilitySystemComponent()))
	{
		bActivated = ASC->TryActivateAbilityByTag(GPTags::Ability::Boss::DarkKnight::Groggy);
	}
	if (!bActivated)
	{
		// Native fallback preserves the mechanic if a designer removes the groggy ability from a child Blueprint.
		ExecuteEnterGroggy();
	}
}

bool AGP_DarkArmorKnightBossCharacter::ExecuteBasicAttack(AActor* TargetActor)
{
	TargetActor = ResolvePatternTarget(TargetActor);
	if (!HasAuthority() || !IsValid(TargetActor))
	{
		return false;
	}
	SetActorRotation((TargetActor->GetActorLocation() - GetActorLocation()).Rotation());
	RecordPatternUse(GPTags::Ability::Boss::DarkKnight::Basic);
	// One authored slash needs one readable wind-up. Phase two adds only one
	// follow-up instead of dealing three invisible hits during the same montage.
	const int32 HitCount = GetDarkKnightPhase() >= 2 ? 2 : 1;
	for (int32 Index = 0; Index < HitCount; ++Index)
	{
		FTimerHandle& Handle = PatternTimerHandles.AddDefaulted_GetRef();
		const float Coefficient = Index == 0 ? 0.7f : (Index == 1 ? 0.9f : 1.0f);
		GetWorldTimerManager().SetTimer(Handle, [WeakThis = TWeakObjectPtr<AGP_DarkArmorKnightBossCharacter>(this), Coefficient]()
		{
			if (WeakThis.IsValid() && !WeakThis->IsDead())
			{
				WeakThis->ApplyConeDamage(WeakThis->BasicAttackRange, 40.0f, Coefficient);
			}
		}, 0.55f + 0.32f * Index, false);
	}
	return true;
}

bool AGP_DarkArmorKnightBossCharacter::ExecuteHeavyAttack(AActor* TargetActor)
{
	TargetActor = ResolvePatternTarget(TargetActor);
	if (!HasAuthority() || !IsValid(TargetActor))
	{
		return false;
	}
	SetActorRotation((TargetActor->GetActorLocation() - GetActorLocation()).Rotation());
	RecordPatternUse(GPTags::Ability::Boss::DarkKnight::Heavy);
	FTimerHandle& Handle = PatternTimerHandles.AddDefaulted_GetRef();
	GetWorldTimerManager().SetTimer(Handle, [WeakThis = TWeakObjectPtr<AGP_DarkArmorKnightBossCharacter>(this)]()
	{
		if (WeakThis.IsValid() && !WeakThis->IsDead())
		{
			WeakThis->ApplyConeDamage(420.0f, 55.0f, 1.6f, 350.0f);
		}
	}, 1.1f, false);
	return true;
}

bool AGP_DarkArmorKnightBossCharacter::ExecuteSweepAttack(AActor* TargetActor)
{
	TargetActor = ResolvePatternTarget(TargetActor);
	if (!HasAuthority() || !IsValid(TargetActor))
	{
		return false;
	}
	SetActorRotation((TargetActor->GetActorLocation() - GetActorLocation()).Rotation());
	RecordPatternUse(GPTags::Ability::Boss::DarkKnight::Sweep);
	FTimerHandle& Handle = PatternTimerHandles.AddDefaulted_GetRef();
	GetWorldTimerManager().SetTimer(Handle, [WeakThis = TWeakObjectPtr<AGP_DarkArmorKnightBossCharacter>(this)]()
	{
		if (WeakThis.IsValid() && !WeakThis->IsDead())
		{
			// A 180-degree half-angle produces the full circular sweep described by the melee pressure pattern.
			WeakThis->ApplyConeDamage(420.0f, 180.0f, 1.1f, 250.0f);
		}
	}, 1.05f, false);
	return true;
}

bool AGP_DarkArmorKnightBossCharacter::ExecuteGuardStance()
{
	if (!HasAuthority() || !IsValid(DarkKnightStateComponent))
	{
		return false;
	}
	const float EffectiveDuration = GetDarkKnightPhase() >= 2 ? GuardDuration + 0.5f : GuardDuration;
	if (!DarkKnightStateComponent->StartGuardStance(EffectiveDuration))
	{
		return false;
	}
	RecordPatternUse(GPTags::Ability::Boss::DarkKnight::Guard);
	// The prototype opens its readable parry window at guard entry; animation/VFX can offset this in a Blueprint later.
	DarkKnightStateComponent->StartParryWindow(ParryWindowDuration);
	return true;
}

bool AGP_DarkArmorKnightBossCharacter::ExecuteCounterAttack(AActor* TargetActor)
{
	if (!HasAuthority() || !IsValid(DarkKnightStateComponent) || DarkKnightStateComponent->IsGroggy())
	{
		return false;
	}
	TargetActor = IsValid(TargetActor) ? TargetActor : PendingCounterTarget.Get();
	TargetActor = ResolvePatternTarget(TargetActor);
	if (IsValid(TargetActor))
	{
		SetActorRotation((TargetActor->GetActorLocation() - GetActorLocation()).Rotation());
	}
	FTimerHandle& Handle = PatternTimerHandles.AddDefaulted_GetRef();
	GetWorldTimerManager().SetTimer(Handle, [WeakThis = TWeakObjectPtr<AGP_DarkArmorKnightBossCharacter>(this), WeakTarget = TWeakObjectPtr<AActor>(TargetActor)]()
	{
		if (!WeakThis.IsValid() || WeakThis->IsDead())
		{
			return;
		}
		WeakThis->ApplyConeDamage(500.0f, 50.0f, 1.8f, 500.0f);
		if (WeakThis->GetDarkKnightPhase() >= 2)
		{
			WeakThis->SpawnDarkWaveVolley(WeakTarget.Get(), 1);
		}
	}, 0.35f, false);
	PendingCounterTarget.Reset();
	return true;
}

bool AGP_DarkArmorKnightBossCharacter::ExecuteChargeAttack(AActor* TargetActor)
{
	TargetActor = ResolvePatternTarget(TargetActor);
	if (!HasAuthority() || !IsValid(TargetActor) || !*ChargeActorClass)
	{
		return false;
	}
	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.Instigator = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AGP_DarkKnightChargeActor* Charge = GetWorld()->SpawnActor<AGP_DarkKnightChargeActor>(ChargeActorClass, GetActorLocation(), GetActorRotation(), Params);
	if (!IsValid(Charge))
	{
		return false;
	}
	RecordPatternUse(GPTags::Ability::Boss::DarkKnight::Charge);
	Charge->InitializeCharge(this, TargetActor);
	return true;
}

bool AGP_DarkArmorKnightBossCharacter::ExecuteDarkWave(AActor* TargetActor)
{
	TargetActor = ResolvePatternTarget(TargetActor);
	if (!HasAuthority() || !IsValid(TargetActor))
	{
		return false;
	}
	SetActorRotation((TargetActor->GetActorLocation() - GetActorLocation()).Rotation());
	RecordPatternUse(GPTags::Ability::Boss::DarkKnight::DarkWave);
	const int32 SlashCount = GetDarkKnightPhase() >= 3 ? 2 : 1;
	for (int32 Index = 0; Index < SlashCount; ++Index)
	{
		FTimerHandle& Handle = PatternTimerHandles.AddDefaulted_GetRef();
		GetWorldTimerManager().SetTimer(Handle, [WeakThis = TWeakObjectPtr<AGP_DarkArmorKnightBossCharacter>(this)]()
		{
			if (WeakThis.IsValid() && !WeakThis->IsDead())
			{
				WeakThis->ApplyConeDamage(520.0f, 60.0f, 1.35f, 325.0f);
			}
		}, 0.78f + Index * 0.30f, false);
	}
	return true;
}

bool AGP_DarkArmorKnightBossCharacter::ExecuteGroundCrack(AActor* TargetActor)
{
	TargetActor = ResolvePatternTarget(TargetActor);
	if (!HasAuthority() || !IsValid(TargetActor) || !*GroundCrackActorClass)
	{
		return false;
	}
	RecordPatternUse(GPTags::Ability::Boss::DarkKnight::GroundCrack);
	FTimerHandle& Handle = PatternTimerHandles.AddDefaulted_GetRef();
	GetWorldTimerManager().SetTimer(Handle, [WeakThis = TWeakObjectPtr<AGP_DarkArmorKnightBossCharacter>(this), WeakTarget = TWeakObjectPtr<AActor>(TargetActor)]()
	{
		if (WeakThis.IsValid() && !WeakThis->IsDead() && WeakTarget.IsValid())
		{
			WeakThis->SpawnGroundCracks(WeakTarget.Get());
		}
	}, 1.05f, false);
	return true;
}

bool AGP_DarkArmorKnightBossCharacter::ExecuteEnterGroggy()
{
	if (!HasAuthority() || !IsValid(DarkKnightStateComponent) || DarkKnightStateComponent->IsGroggy())
	{
		return false;
	}
	DarkKnightStateComponent->EnterGroggy(GetDarkKnightPhase() >= 3 ? FinalPhaseGroggyDuration : GroggyDuration);
	return true;
}

bool AGP_DarkArmorKnightBossCharacter::StartPatternWithWindup(FGameplayTag PatternTag, AActor* TargetActor)
{
	TargetActor = ResolvePatternTarget(TargetActor);
	const bool bUsesWindup = PatternTag != GPTags::Ability::Boss::DarkKnight::Charge
		&& PatternTag != GPTags::Ability::Boss::DarkKnight::Groggy;
	bool bWindupStarted = false;
	if (bUsesWindup && IsValid(PreAttackMontage))
	{
		UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
		if (!IsValid(AnimInstance) || AnimInstance->Montage_Play(PreAttackMontage, 1.0f) <= 0.0f)
		{
			return false;
		}
		bWindupStarted = true;
	}

	// Groggy is a state reaction, not an authored attack, so it deliberately bypasses the optional VFX lead-in.
	const float TelegraphDelay = PlayBossTelegraphForPattern(PatternTag);
	const float WindupDelay = bWindupStarted ? FMath::Max(0.0f, PreAttackDuration) : 0.0f;
	const float ExecutionDelay = FMath::Max(WindupDelay, TelegraphDelay);
	if (ExecutionDelay <= KINDA_SMALL_NUMBER)
	{
		const bool bExecuted = ExecutePatternNow(PatternTag, TargetActor);
		if (bExecuted && PatternTag != GPTags::Ability::Boss::DarkKnight::Charge)
		{
			PlayPatternMontage(PatternTag);
		}
		return bExecuted;
	}

	FTimerHandle& Handle = PatternTimerHandles.AddDefaulted_GetRef();
	GetWorldTimerManager().SetTimer(Handle, [WeakThis = TWeakObjectPtr<AGP_DarkArmorKnightBossCharacter>(this), WeakTarget = TWeakObjectPtr<AActor>(TargetActor), PatternTag]()
	{
		if (!WeakThis.IsValid() || WeakThis->IsDead())
		{
			return;
		}
		if (WeakThis->ExecutePatternNow(PatternTag, WeakTarget.Get())
			&& PatternTag != GPTags::Ability::Boss::DarkKnight::Charge)
		{
			WeakThis->PlayPatternMontage(PatternTag);
		}
	}, ExecutionDelay, false);
	return true;
}

bool AGP_DarkArmorKnightBossCharacter::ExecutePatternNow(FGameplayTag PatternTag, AActor* TargetActor)
{
	if (PatternTag == GPTags::Ability::Boss::DarkKnight::Basic) return ExecuteBasicAttack(TargetActor);
	if (PatternTag == GPTags::Ability::Boss::DarkKnight::Heavy) return ExecuteHeavyAttack(TargetActor);
	if (PatternTag == GPTags::Ability::Boss::DarkKnight::Charge) return ExecuteChargeAttack(TargetActor);
	if (PatternTag == GPTags::Ability::Boss::DarkKnight::DarkWave) return ExecuteDarkWave(TargetActor);
	if (PatternTag == GPTags::Ability::Boss::DarkKnight::GroundCrack) return ExecuteGroundCrack(TargetActor);
	if (PatternTag == GPTags::Ability::Boss::DarkKnight::Groggy) return ExecuteEnterGroggy();
	return false;
}

bool AGP_DarkArmorKnightBossCharacter::PlayPatternMontage(FGameplayTag PatternTag)
{
	const TObjectPtr<UAnimMontage>* Montage = PatternMontages.Find(PatternTag);
	UAnimMontage* MontageToPlay = Montage ? Montage->Get() : nullptr;
	// Dark Wave is delivered with the authored sword slash, not the old cast pose.
	if (PatternTag == GPTags::Ability::Boss::DarkKnight::DarkWave)
	{
		MontageToPlay = LoadObject<UAnimMontage>(nullptr, TEXT("/Game/Characters/EnemyCharacter/Boss/BP_Boss_DarkArmorKnight/Animations/AM_DK_DarkSlash.AM_DK_DarkSlash"));
	}
	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!IsValid(MontageToPlay) || !IsValid(AnimInstance))
	{
		return false;
	}

	return AnimInstance->Montage_Play(MontageToPlay, 1.0f) > 0.0f;
}

void AGP_DarkArmorKnightBossCharacter::HandleChargeFinished(bool bHitTarget, AActor* TargetActor)
{
	(void)bHitTarget;

	if (!HasAuthority() || !IsValid(TargetActor))
	{
		return;
	}

	const FGameplayTag FollowUpTag = GetDarkKnightPhase() >= 3
		? GPTags::Ability::Boss::DarkKnight::GroundCrack
		: FGameplayTag();
	if (!FollowUpTag.IsValid())
	{
		return;
	}

	FTimerHandle& Handle = PatternTimerHandles.AddDefaulted_GetRef();
	GetWorldTimerManager().SetTimer(Handle, [WeakThis = TWeakObjectPtr<AGP_DarkArmorKnightBossCharacter>(this), FollowUpTag]()
	{
		if (WeakThis.IsValid() && !WeakThis->IsDead())
		{
			// Follow-ups re-enter through GAS so grants, ability rules, and cooldowns remain the single execution path.
			if (UGP_AbilitySystemComponent* ASC = Cast<UGP_AbilitySystemComponent>(WeakThis->GetAbilitySystemComponent()))
			{
				ASC->TryActivateAbilityByTag(FollowUpTag);
			}
		}
	}, 0.25f, false);
}

void AGP_DarkArmorKnightBossCharacter::HandlePostDamageTaken(AActor* InstigatorActor, float DamageAmount, FGameplayTag ElementTag)
{
	Super::HandlePostDamageTaken(InstigatorActor, DamageAmount, ElementTag);
	if (HasAuthority() && IsValid(DarkKnightStateComponent))
	{
		DarkKnightStateComponent->SetCombatPhase(GetDarkKnightPhase());
	}
}

void AGP_DarkArmorKnightBossCharacter::GrantDarkKnightAbilities()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!HasAuthority() || !IsValid(ASC))
	{
		return;
	}
	for (const TSubclassOf<UGameplayAbility>& AbilityClass : DarkKnightAbilityClasses)
	{
		if (*AbilityClass && ASC->FindAbilitySpecFromClass(AbilityClass) == nullptr)
		{
			// Native grants make the C++ boss playable before polished GameplayAbility Blueprint children exist.
			ASC->GiveAbility(FGameplayAbilitySpec(AbilityClass));
		}
	}
}

AActor* AGP_DarkArmorKnightBossCharacter::ResolvePatternTarget(AActor* ExplicitTarget) const
{
	if (IsValid(ExplicitTarget))
	{
		return ExplicitTarget;
	}
	if (const AAIController* AIController = Cast<AAIController>(GetController()))
	{
		if (IsValid(AIController->GetFocusActor()))
		{
			return AIController->GetFocusActor();
		}
	}
	return UGameplayStatics::GetPlayerPawn(this, 0);
}

TArray<AActor*> AGP_DarkArmorKnightBossCharacter::GatherTargetsInCone(float Range, float HalfAngleDegrees) const
{
	TArray<AActor*> Targets;
	const float MinimumDot = FMath::Cos(FMath::DegreesToRadians(FMath::Clamp(HalfAngleDegrees, 0.0f, 180.0f)));
	for (TActorIterator<APawn> It(GetWorld()); It; ++It)
	{
		APawn* Pawn = *It;
		if (!IsValid(Pawn) || Pawn == this)
		{
			continue;
		}
		const FVector Delta = Pawn->GetActorLocation() - GetActorLocation();
		if (Delta.Size2D() <= Range && FVector::DotProduct(GetActorForwardVector().GetSafeNormal2D(), Delta.GetSafeNormal2D()) >= MinimumDot)
		{
			Targets.Add(Pawn);
		}
	}
	return Targets;
}

void AGP_DarkArmorKnightBossCharacter::ApplyConeDamage(float Range, float HalfAngleDegrees, float DamageCoefficient, float KnockbackStrength)
{
	TArray<AActor*> Targets = GatherTargetsInCone(Range, HalfAngleDegrees);
	GPBossCombatUtils::ApplyDamage(this, Targets, DamageEffectClass, DamageCoefficient, 0.0f, false, GPTags::Damage::Element::Brute);
	if (KnockbackStrength > 0.0f)
	{
		for (AActor* Target : Targets)
		{
			if (ACharacter* Character = Cast<ACharacter>(Target))
			{
				Character->LaunchCharacter((Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D() * KnockbackStrength + FVector(0.0f, 0.0f, 180.0f), true, true);
			}
		}
	}
}

void AGP_DarkArmorKnightBossCharacter::RecordPatternUse(const FGameplayTag& PatternTag)
{
	LastPatternUseTimes.FindOrAdd(PatternTag) = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
}

float AGP_DarkArmorKnightBossCharacter::ResolvePatternCooldown(const FGameplayTag& PatternTag) const
{
	if (PatternTag == GPTags::Ability::Boss::DarkKnight::Basic) return BasicCooldown;
	if (PatternTag == GPTags::Ability::Boss::DarkKnight::Heavy) return HeavyCooldown;
	if (PatternTag == GPTags::Ability::Boss::DarkKnight::Sweep) return SweepCooldown;
	if (PatternTag == GPTags::Ability::Boss::DarkKnight::Guard) return GuardCooldown;
	if (PatternTag == GPTags::Ability::Boss::DarkKnight::Charge) return ChargeCooldown;
	if (PatternTag == GPTags::Ability::Boss::DarkKnight::DarkWave) return DarkWaveCooldown;
	if (PatternTag == GPTags::Ability::Boss::DarkKnight::GroundCrack) return GroundCrackCooldown;
	return 0.0f;
}

void AGP_DarkArmorKnightBossCharacter::SpawnDarkWaveVolley(AActor* TargetActor, int32 ProjectileCount)
{
	if (!HasAuthority() || !IsValid(TargetActor) || !*DarkWaveProjectileClass)
	{
		return;
	}
	const FVector SpawnLocation = GetActorLocation() + FVector(0.0f, 0.0f, 120.0f) + GetActorForwardVector() * 100.0f;
	const FRotator BaseRotation = (TargetActor->GetActorLocation() + FVector(0.0f, 0.0f, 80.0f) - SpawnLocation).Rotation();
	for (int32 Index = 0; Index < FMath::Max(1, ProjectileCount); ++Index)
	{
		const float CenteredIndex = static_cast<float>(Index) - static_cast<float>(ProjectileCount - 1) * 0.5f;
		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.Instigator = this;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		GetWorld()->SpawnActor<AGP_DarkWaveProjectile>(DarkWaveProjectileClass, SpawnLocation, BaseRotation + FRotator(0.0f, CenteredIndex * 12.0f, 0.0f), Params);
	}
}

void AGP_DarkArmorKnightBossCharacter::SpawnGroundCracks(AActor* TargetActor)
{
	const int32 MarkerCount = GetDarkKnightPhase() >= 2 ? 7 : 4;
	const FVector PredictedCenter = TargetActor->GetActorLocation() + TargetActor->GetVelocity() * 0.45f;
	for (int32 Index = 0; Index < MarkerCount; ++Index)
	{
		const float Angle = (360.0f / MarkerCount) * Index + (Index % 2 == 0 ? 12.0f : -8.0f);
		const float Radius = Index == 0 ? 0.0f : 180.0f + 90.0f * (Index % 3);
		FVector Candidate = PredictedCenter + FRotator(0.0f, Angle, 0.0f).Vector() * Radius;
		FHitResult GroundHit;
		if (!GetWorld()->LineTraceSingleByChannel(GroundHit, Candidate + FVector(0.0f, 0.0f, 500.0f), Candidate - FVector(0.0f, 0.0f, 1000.0f), ECC_Visibility))
		{
			// Skip invalid samples instead of leaving a temporary marker floating beside a cliff or above empty space.
			continue;
		}
		Candidate = GroundHit.ImpactPoint + FVector(0.0f, 0.0f, 5.0f);

		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.Instigator = this;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (AGP_DarkKnightGroundCrackActor* Crack = GetWorld()->SpawnActor<AGP_DarkKnightGroundCrackActor>(GroundCrackActorClass, Candidate, FRotator::ZeroRotator, Params))
		{
			Crack->InitializeGroundCrack(this);
		}
	}
}

void AGP_DarkArmorKnightBossCharacter::HandleGroggyChanged(bool bNewGroggy)
{
	if (!HasAuthority())
	{
		return;
	}
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		if (bNewGroggy)
		{
			Movement->StopMovementImmediately();
			Movement->DisableMovement();
		}
		else
		{
			Movement->SetMovementMode(MOVE_Walking);
		}
	}
	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		AIController->StopMovement();
		if (UBrainComponent* Brain = AIController->GetBrainComponent())
		{
			if (bNewGroggy)
			{
				Brain->PauseLogic(TEXT("Dark Knight guard broken"));
			}
			else
			{
				Brain->ResumeLogic(TEXT("Dark Knight recovered"));
			}
		}
	}
}
