#include "Actors/GP_TestEnemyResistanceSetter.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

AGP_TestEnemyResistanceSetter::AGP_TestEnemyResistanceSetter()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AGP_TestEnemyResistanceSetter::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		return;
	}

	if (InitialDelay > 0.0f)
	{
		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(
			TimerHandle,
			this,
			&AGP_TestEnemyResistanceSetter::ApplyResistanceEffects,
			InitialDelay,
			false);
		return;
	}

	GetWorldTimerManager().SetTimerForNextTick(this, &AGP_TestEnemyResistanceSetter::ApplyResistanceEffects);
}

void AGP_TestEnemyResistanceSetter::ApplyResistanceEffects()
{
	if (!ResistanceEffect)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ResistanceTest] ResistanceEffect is not set on %s"), *GetNameSafe(this));
		return;
	}

	TArray<AActor*> Targets;
	UGameplayStatics::GetAllActorsWithTag(this, TargetTag, Targets);

	if (Targets.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[ResistanceTest] No actors found with tag '%s'"), *TargetTag.ToString());
		return;
	}

	for (AActor* Target : Targets)
	{
		if (!IsValid(Target))
		{
			continue;
		}

		UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
		if (!IsValid(ASC))
		{
			UE_LOG(LogTemp, Warning, TEXT("[ResistanceTest] ASC missing: %s"), *GetNameSafe(Target));
			continue;
		}

		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		Context.AddSourceObject(this);

		const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(ResistanceEffect, EffectLevel, Context);
		if (!SpecHandle.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("[ResistanceTest] Failed to make spec for %s"), *GetNameSafe(Target));
			continue;
		}

		ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		UE_LOG(LogTemp, Log, TEXT("[ResistanceTest] Applied %s to %s"), *GetNameSafe(ResistanceEffect), *GetNameSafe(Target));
	}
}
