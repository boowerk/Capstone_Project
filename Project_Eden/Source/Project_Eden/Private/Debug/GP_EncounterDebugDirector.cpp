#include "Debug/GP_EncounterDebugDirector.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "AI/Data/EnemyBlackboardKeys.h"
#include "AI/Tasks/BossAttackExecution.h"
#include "Actors/GP_MatadorBossDecoyActor.h"
#include "Actors/GP_MatadorDecoyPressureComponent.h"
#include "Animation/GP_MatadorDecoyAnimInstance.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BrainComponent.h"
#include "Characters/GP_EnemyCharacter.h"
#include "Characters/GP_MatadorBossStateComponent.h"
#include "Characters/GP_MatadorMageBossCharacter.h"
#include "Characters/GP_PlayerCharacter.h"
#include "AbilitySystem/GP_AttributeSet.h"
#include "Blueprint/UserWidget.h"
#include "Debug/GP_EncounterDebugRuntimeWidget.h"
#include "Engine/Engine.h"
#include "Engine/Blueprint.h"
#include "EngineUtils.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameplayAbilitySpec.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"

namespace EncounterDebugReport
{
	bool HasBlackboardKey(const UBlackboardComponent* BlackboardComponent, const FName& KeyName)
	{
		return IsValid(BlackboardComponent) && BlackboardComponent->GetKeyID(KeyName) != FBlackboard::InvalidKey;
	}

	FString BoolText(bool bValue)
	{
		return bValue ? TEXT("true") : TEXT("false");
	}

	FString ActorText(const UObject* Object)
	{
		return IsValid(Object) ? GetNameSafe(Object) : TEXT("None");
	}

	FString VectorText(const FVector& Value)
	{
		return Value.ToCompactString();
	}

	FString EnumText(const UEnum* Enum, int64 Value)
	{
		if (!Enum)
		{
			return FString::FromInt(static_cast<int32>(Value));
		}

		return Enum->GetNameStringByValue(Value);
	}

	void AddBlackboardObject(TArray<FString>& Lines, const UBlackboardComponent* BlackboardComponent, const FName& KeyName)
	{
		if (HasBlackboardKey(BlackboardComponent, KeyName))
		{
			Lines.Add(FString::Printf(TEXT("  %s: %s"), *KeyName.ToString(), *ActorText(BlackboardComponent->GetValueAsObject(KeyName))));
		}
	}

	void AddBlackboardBool(TArray<FString>& Lines, const UBlackboardComponent* BlackboardComponent, const FName& KeyName)
	{
		if (HasBlackboardKey(BlackboardComponent, KeyName))
		{
			Lines.Add(FString::Printf(TEXT("  %s: %s"), *KeyName.ToString(), *BoolText(BlackboardComponent->GetValueAsBool(KeyName))));
		}
	}

	void AddBlackboardFloat(TArray<FString>& Lines, const UBlackboardComponent* BlackboardComponent, const FName& KeyName)
	{
		if (HasBlackboardKey(BlackboardComponent, KeyName))
		{
			Lines.Add(FString::Printf(TEXT("  %s: %.1f"), *KeyName.ToString(), BlackboardComponent->GetValueAsFloat(KeyName)));
		}
	}

	void AddBlackboardInt(TArray<FString>& Lines, const UBlackboardComponent* BlackboardComponent, const FName& KeyName)
	{
		if (HasBlackboardKey(BlackboardComponent, KeyName))
		{
			Lines.Add(FString::Printf(TEXT("  %s: %d"), *KeyName.ToString(), BlackboardComponent->GetValueAsInt(KeyName)));
		}
	}

	void AddBlackboardName(TArray<FString>& Lines, const UBlackboardComponent* BlackboardComponent, const FName& KeyName)
	{
		if (HasBlackboardKey(BlackboardComponent, KeyName))
		{
			Lines.Add(FString::Printf(TEXT("  %s: %s"), *KeyName.ToString(), *BlackboardComponent->GetValueAsName(KeyName).ToString()));
		}
	}

	void AddBlackboardVector(TArray<FString>& Lines, const UBlackboardComponent* BlackboardComponent, const FName& KeyName)
	{
		if (HasBlackboardKey(BlackboardComponent, KeyName))
		{
			Lines.Add(FString::Printf(TEXT("  %s: %s"), *KeyName.ToString(), *VectorText(BlackboardComponent->GetValueAsVector(KeyName))));
		}
	}
}

AGP_EncounterDebugDirector::AGP_EncounterDebugDirector()
{
	PrimaryActorTick.bCanEverTick = false;

	StageSlots.SetNum(3);
	for (int32 Index = 0; Index < StageSlots.Num(); ++Index)
	{
		StageSlots[Index].StageName = *FString::Printf(TEXT("Stage %d"), Index + 1);
		StageSlots[Index].FallbackTransform.SetLocation(FVector(Index * 1500.0f, 0.0f, 100.0f));
	}
}

void AGP_EncounterDebugDirector::BeginPlay()
{
	Super::BeginPlay();

	if (bEnableRuntimeDebugHotkey)
	{
		if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
		{
			EnableInput(PlayerController);
			if (InputComponent)
			{
				InputComponent->BindKey(RuntimeDebugToggleKey, IE_Pressed, this, &AGP_EncounterDebugDirector::ToggleRuntimeDebugPanel);
			}
		}
	}

	Log(TEXT("[OK] EncounterDebugDirector ready."));
}

AGP_EncounterDebugDirector* AGP_EncounterDebugDirector::FindActiveEncounterDebugDirector()
{
	if (!GEngine)
	{
		return nullptr;
	}

	UWorld* FallbackEditorWorld = nullptr;
	for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
	{
		UWorld* World = WorldContext.World();
		if (!World)
		{
			continue;
		}

		if (WorldContext.WorldType == EWorldType::PIE || WorldContext.WorldType == EWorldType::Game)
		{
			for (TActorIterator<AGP_EncounterDebugDirector> It(World); It; ++It)
			{
				return *It;
			}
		}
		else if (WorldContext.WorldType == EWorldType::Editor && !FallbackEditorWorld)
		{
			FallbackEditorWorld = World;
		}
	}

	if (FallbackEditorWorld)
	{
		for (TActorIterator<AGP_EncounterDebugDirector> It(FallbackEditorWorld); It; ++It)
		{
			return *It;
		}
	}

	return nullptr;
}

bool AGP_EncounterDebugDirector::ValidateSetup()
{
	if (AGP_EncounterDebugDirector* RuntimeDirector = ResolveRuntimeDirectorForCall(); RuntimeDirector != this)
	{
		return RuntimeDirector->ValidateSetup();
	}

	bool bValid = true;

	Log(TEXT("[Validate] Encounter Debug setup check started."));

	if (!GetWorld() || !GetWorld()->IsGameWorld())
	{
		Log(TEXT("[Fail] Director is not running in a PIE/game world."));
		bValid = false;
	}
	else
	{
		Log(FString::Printf(TEXT("[OK] PIE World found: %s"), *GetWorld()->GetName()));
	}

	if (!FindPlayerCharacter())
	{
		Log(TEXT("[Fail] Player character not found."));
		bValid = false;
	}
	else
	{
		Log(TEXT("[OK] Player character found."));
	}

	if (StageSlots.Num() <= 0)
	{
		Log(TEXT("[Fail] No stage slots configured."));
		return false;
	}

	for (int32 Index = 0; Index < StageSlots.Num(); ++Index)
	{
		const FGPEncounterDebugStageSlot& Slot = StageSlots[Index];
		FTransform StageTransform;
		const bool bHasTransform = ResolveStageTransform(Index, StageTransform);
		if (!bHasTransform)
		{
			Log(FString::Printf(TEXT("[Fail] Stage %d has no valid point or fallback transform."), Index + 1));
			bValid = false;
		}
		else
		{
			Log(FString::Printf(TEXT("[OK] Stage %d location: %s"), Index + 1, *StageTransform.GetLocation().ToCompactString()));
		}

		if (!*Slot.MobClass)
		{
			Log(FString::Printf(TEXT("[Warning] Stage %d MobClass is empty."), Index + 1));
		}

		if (!*Slot.BossClass)
		{
			Log(FString::Printf(TEXT("[Warning] Stage %d BossClass is empty."), Index + 1));
		}
	}

	Log(bValid ? TEXT("[OK] Validate finished.") : TEXT("[Fail] Validate finished with errors."));
	return bValid;
}

void AGP_EncounterDebugDirector::ToggleRuntimeDebugPanel()
{
	if (RuntimeDebugWidget && RuntimeDebugWidget->IsInViewport())
	{
		HideRuntimeDebugPanel();
	}
	else
	{
		ShowRuntimeDebugPanel();
	}
}

void AGP_EncounterDebugDirector::ShowRuntimeDebugPanel()
{
	if (AGP_EncounterDebugDirector* RuntimeDirector = ResolveRuntimeDirectorForCall(); RuntimeDirector != this)
	{
		RuntimeDirector->ShowRuntimeDebugPanel();
		return;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!PlayerController)
	{
		Log(TEXT("[Fail] Cannot show runtime debug panel: player controller not found."));
		return;
	}

	if (!RuntimeDebugWidget)
	{
		RuntimeDebugWidget = CreateWidget<UGP_EncounterDebugRuntimeWidget>(PlayerController, UGP_EncounterDebugRuntimeWidget::StaticClass());
		if (RuntimeDebugWidget)
		{
			RuntimeDebugWidget->SetDirector(this);
		}
	}

	if (!RuntimeDebugWidget)
	{
		Log(TEXT("[Fail] Cannot show runtime debug panel: widget creation failed."));
		return;
	}

	if (!RuntimeDebugWidget->IsInViewport())
	{
		RuntimeDebugWidget->AddToViewport(1000);
	}

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(RuntimeDebugWidget->TakeWidget());
	InputMode.SetHideCursorDuringCapture(false);
	PlayerController->SetInputMode(InputMode);
	PlayerController->SetShowMouseCursor(true);
	Log(TEXT("[OK] Runtime debug panel shown."));
}

void AGP_EncounterDebugDirector::HideRuntimeDebugPanel()
{
	if (AGP_EncounterDebugDirector* RuntimeDirector = ResolveRuntimeDirectorForCall(); RuntimeDirector != this)
	{
		RuntimeDirector->HideRuntimeDebugPanel();
		return;
	}

	if (RuntimeDebugWidget)
	{
		RuntimeDebugWidget->RemoveFromParent();
	}

	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
	{
		PlayerController->SetInputMode(FInputModeGameOnly());
		PlayerController->SetShowMouseCursor(false);
	}

	Log(TEXT("[OK] Runtime debug panel hidden."));
}

bool AGP_EncounterDebugDirector::SelectStage(int32 StageIndex)
{
	if (AGP_EncounterDebugDirector* RuntimeDirector = ResolveRuntimeDirectorForCall(); RuntimeDirector != this)
	{
		return RuntimeDirector->SelectStage(StageIndex);
	}

	if (!StageSlots.IsValidIndex(StageIndex))
	{
		Log(FString::Printf(TEXT("[Fail] Invalid stage index: %d"), StageIndex));
		return false;
	}

	SelectedStageIndex = StageIndex;
	Log(FString::Printf(TEXT("[OK] Selected %s."), *StageSlots[StageIndex].StageName.ToString()));
	return true;
}

int32 AGP_EncounterDebugDirector::GetSelectedStageIndex() const
{
	if (AGP_EncounterDebugDirector* RuntimeDirector = ResolveRuntimeDirectorForCall(); RuntimeDirector != this)
	{
		return RuntimeDirector->GetSelectedStageIndex();
	}

	return SelectedStageIndex;
}

bool AGP_EncounterDebugDirector::TeleportPlayerToSelectedStage()
{
	if (AGP_EncounterDebugDirector* RuntimeDirector = ResolveRuntimeDirectorForCall(); RuntimeDirector != this)
	{
		return RuntimeDirector->TeleportPlayerToSelectedStage();
	}

	AGP_PlayerCharacter* Player = FindPlayerCharacter();
	FTransform StageTransform;
	if (!Player || !ResolveStageTransform(SelectedStageIndex, StageTransform))
	{
		Log(TEXT("[Fail] Cannot teleport player: missing player or stage transform."));
		return false;
	}

	Player->SetActorLocationAndRotation(StageTransform.GetLocation(), StageTransform.GetRotation().Rotator(), false, nullptr, ETeleportType::TeleportPhysics);
	Log(FString::Printf(TEXT("[OK] Player teleported to stage %d."), SelectedStageIndex + 1));
	return true;
}

AGP_EnemyCharacter* AGP_EncounterDebugDirector::SpawnSelectedStageMob()
{
	if (AGP_EncounterDebugDirector* RuntimeDirector = ResolveRuntimeDirectorForCall(); RuntimeDirector != this)
	{
		return RuntimeDirector->SpawnSelectedStageMob();
	}

	if (!StageSlots.IsValidIndex(SelectedStageIndex))
	{
		Log(TEXT("[Fail] Cannot spawn stage mob: invalid selected stage."));
		return nullptr;
	}

	return SpawnEnemyForSelectedStage(EGPEncounterDebugSpawnKind::Mob, StageSlots[SelectedStageIndex].MobClass);
}

AGP_EnemyCharacter* AGP_EncounterDebugDirector::SpawnSelectedStageBoss()
{
	if (AGP_EncounterDebugDirector* RuntimeDirector = ResolveRuntimeDirectorForCall(); RuntimeDirector != this)
	{
		return RuntimeDirector->SpawnSelectedStageBoss();
	}

	if (!StageSlots.IsValidIndex(SelectedStageIndex))
	{
		Log(TEXT("[Fail] Cannot spawn stage boss: invalid selected stage."));
		return nullptr;
	}

	return SpawnEnemyForSelectedStage(EGPEncounterDebugSpawnKind::Boss, StageSlots[SelectedStageIndex].BossClass);
}

AGP_EnemyCharacter* AGP_EncounterDebugDirector::SpawnSelectedClassMob()
{
	if (AGP_EncounterDebugDirector* RuntimeDirector = ResolveRuntimeDirectorForCall(); RuntimeDirector != this)
	{
		return RuntimeDirector->SpawnSelectedClassMob();
	}

	return SpawnSelectedClass();
}

AGP_EnemyCharacter* AGP_EncounterDebugDirector::SpawnSelectedClassBoss()
{
	if (AGP_EncounterDebugDirector* RuntimeDirector = ResolveRuntimeDirectorForCall(); RuntimeDirector != this)
	{
		return RuntimeDirector->SpawnSelectedClassBoss();
	}

	return SpawnSelectedClass();
}

AGP_EnemyCharacter* AGP_EncounterDebugDirector::SpawnSelectedClass()
{
	if (AGP_EncounterDebugDirector* RuntimeDirector = ResolveRuntimeDirectorForCall(); RuntimeDirector != this)
	{
		return RuntimeDirector->SpawnSelectedClass();
	}

	EnsureAvailableMobClassList();
	return SpawnEnemyForSelectedStage(EGPEncounterDebugSpawnKind::Mob, ResolveSelectedClassForSpawn());
}

TArray<FString> AGP_EncounterDebugDirector::GetAvailableMobClassNames() const
{
	if (AGP_EncounterDebugDirector* RuntimeDirector = ResolveRuntimeDirectorForCall(); RuntimeDirector != this)
	{
		return RuntimeDirector->GetAvailableMobClassNames();
	}

	TArray<FString> Names;
	AGP_EncounterDebugDirector* MutableThis = const_cast<AGP_EncounterDebugDirector*>(this);
	MutableThis->EnsureAvailableMobClassList();

	Names.Reserve(AvailableMobClasses.Num() + DiscoveredMobClassNames.Num());
	for (const TSubclassOf<AGP_EnemyCharacter>& MobClass : AvailableMobClasses)
	{
		Names.Add(*MobClass ? MobClass->GetName() : FString(TEXT("<Empty>")));
	}
	Names.Append(DiscoveredMobClassNames);

	return Names;
}

bool AGP_EncounterDebugDirector::SelectAvailableMobClass(int32 ClassIndex)
{
	if (AGP_EncounterDebugDirector* RuntimeDirector = ResolveRuntimeDirectorForCall(); RuntimeDirector != this)
	{
		return RuntimeDirector->SelectAvailableMobClass(ClassIndex);
	}

	EnsureAvailableMobClassList();

	const int32 TotalClassCount = AvailableMobClasses.Num() + DiscoveredMobClassPaths.Num();
	if (ClassIndex < 0 || ClassIndex >= TotalClassCount)
	{
		Log(FString::Printf(TEXT("[Fail] Invalid mob class index: %d"), ClassIndex));
		return false;
	}

	SelectedAvailableMobClassIndex = ClassIndex;
	Log(FString::Printf(TEXT("[OK] Selected mob class: %s."), *GetSelectedAvailableMobClassName()));
	return true;
}

int32 AGP_EncounterDebugDirector::GetSelectedAvailableMobClassIndex() const
{
	if (AGP_EncounterDebugDirector* RuntimeDirector = ResolveRuntimeDirectorForCall(); RuntimeDirector != this)
	{
		return RuntimeDirector->GetSelectedAvailableMobClassIndex();
	}

	return SelectedAvailableMobClassIndex;
}

FString AGP_EncounterDebugDirector::GetSelectedAvailableMobClassName() const
{
	if (AGP_EncounterDebugDirector* RuntimeDirector = ResolveRuntimeDirectorForCall(); RuntimeDirector != this)
	{
		return RuntimeDirector->GetSelectedAvailableMobClassName();
	}

	if (AvailableMobClasses.IsValidIndex(SelectedAvailableMobClassIndex) && *AvailableMobClasses[SelectedAvailableMobClassIndex])
	{
		return AvailableMobClasses[SelectedAvailableMobClassIndex]->GetName();
	}

	const int32 DiscoveredIndex = SelectedAvailableMobClassIndex - AvailableMobClasses.Num();
	if (DiscoveredMobClassNames.IsValidIndex(DiscoveredIndex))
	{
		return DiscoveredMobClassNames[DiscoveredIndex];
	}

	return FString(TEXT("<No Selected Class>"));
}

void AGP_EncounterDebugDirector::SpawnSelectedStageMobAndBoss()
{
	if (AGP_EncounterDebugDirector* RuntimeDirector = ResolveRuntimeDirectorForCall(); RuntimeDirector != this)
	{
		RuntimeDirector->SpawnSelectedStageMobAndBoss();
		return;
	}

	SpawnSelectedStageMob();
	SpawnSelectedStageBoss();
}

int32 AGP_EncounterDebugDirector::DespawnSelectedStageDebugEnemies()
{
	if (AGP_EncounterDebugDirector* RuntimeDirector = ResolveRuntimeDirectorForCall(); RuntimeDirector != this)
	{
		return RuntimeDirector->DespawnSelectedStageDebugEnemies();
	}

	int32 RemovedCount = 0;
	for (int32 Index = DebugSpawnedEnemies.Num() - 1; Index >= 0; --Index)
	{
		AGP_EnemyCharacter* Enemy = DebugSpawnedEnemies[Index];
		if (!IsValid(Enemy))
		{
			DebugSpawnedEnemies.RemoveAtSwap(Index);
			continue;
		}

		if (ResolveDebugStageIndex(Enemy) == SelectedStageIndex)
		{
			RemovedCount += DestroyMatadorDecoysForBoss(Enemy);
			Enemy->Destroy();
			DebugSpawnedEnemies.RemoveAtSwap(Index);
			++RemovedCount;
		}
	}

	if (SelectedEnemy.IsValid() && !IsValid(SelectedEnemy.Get()))
	{
		SelectedEnemy.Reset();
	}

	Log(FString::Printf(TEXT("[OK] Despawned %d debug enemies in stage %d."), RemovedCount, SelectedStageIndex + 1));
	return RemovedCount;
}

int32 AGP_EncounterDebugDirector::DespawnAllDebugEnemies()
{
	if (AGP_EncounterDebugDirector* RuntimeDirector = ResolveRuntimeDirectorForCall(); RuntimeDirector != this)
	{
		return RuntimeDirector->DespawnAllDebugEnemies();
	}

	int32 RemovedCount = 0;
	for (int32 Index = DebugSpawnedEnemies.Num() - 1; Index >= 0; --Index)
	{
		AGP_EnemyCharacter* Enemy = DebugSpawnedEnemies[Index];
		if (IsValid(Enemy))
		{
			Enemy->Destroy();
			++RemovedCount;
		}
		DebugSpawnedEnemies.RemoveAtSwap(Index);
	}

	TArray<AActor*> DecoyActors;
	UGameplayStatics::GetAllActorsOfClass(this, AGP_MatadorBossDecoyActor::StaticClass(), DecoyActors);
	for (AActor* DecoyActor : DecoyActors)
	{
		if (IsValid(DecoyActor))
		{
			DecoyActor->Destroy();
			++RemovedCount;
		}
	}

	SelectedEnemy.Reset();
	Log(FString::Printf(TEXT("[OK] Despawned %d debug enemies."), RemovedCount));
	return RemovedCount;
}

TArray<FGPEncounterDebugEnemySnapshot> AGP_EncounterDebugDirector::RefreshEnemyList()
{
	if (AGP_EncounterDebugDirector* RuntimeDirector = ResolveRuntimeDirectorForCall(); RuntimeDirector != this)
	{
		return RuntimeDirector->RefreshEnemyList();
	}

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(this, AGP_EnemyCharacter::StaticClass(), FoundActors);

	TArray<FGPEncounterDebugEnemySnapshot> Snapshots;
	Snapshots.Reserve(FoundActors.Num());
	for (AActor* Actor : FoundActors)
	{
		if (AGP_EnemyCharacter* Enemy = Cast<AGP_EnemyCharacter>(Actor))
		{
			Snapshots.Add(BuildEnemySnapshot(Enemy));
		}
	}

	Log(FString::Printf(TEXT("[OK] Enemy list refreshed: %d enemies."), Snapshots.Num()));
	return Snapshots;
}

bool AGP_EncounterDebugDirector::SelectEnemy(AGP_EnemyCharacter* Enemy)
{
	if (AGP_EncounterDebugDirector* RuntimeDirector = ResolveRuntimeDirectorForCall(); RuntimeDirector != this)
	{
		return RuntimeDirector->SelectEnemy(Enemy);
	}

	if (!IsValid(Enemy))
	{
		SelectedEnemy.Reset();
		Log(TEXT("[Fail] Selected enemy is invalid."));
		return false;
	}

	SelectedEnemy = Enemy;
	Log(FString::Printf(TEXT("[OK] Selected enemy: %s."), *GetNameSafe(Enemy)));
	return true;
}

AGP_EnemyCharacter* AGP_EncounterDebugDirector::GetSelectedEnemy() const
{
	if (AGP_EncounterDebugDirector* RuntimeDirector = ResolveRuntimeDirectorForCall(); RuntimeDirector != this)
	{
		return RuntimeDirector->GetSelectedEnemy();
	}

	return SelectedEnemy.Get();
}

FString AGP_EncounterDebugDirector::GetSelectedEnemyDebugReport(EGPEncounterDebugReportMode ReportMode) const
{
	if (AGP_EncounterDebugDirector* RuntimeDirector = ResolveRuntimeDirectorForCall(); RuntimeDirector != this)
	{
		return RuntimeDirector->GetSelectedEnemyDebugReport(ReportMode);
	}

	AGP_EnemyCharacter* Enemy = SelectedEnemy.Get();
	if (!IsValid(Enemy))
	{
		return TEXT("Selected Enemy\n<none>\n\nEnemy combo에서 몬스터를 선택하면 실시간 상태가 표시됩니다.");
	}

	return BuildEnemyDebugReport(Enemy, ReportMode);
}

FString AGP_EncounterDebugDirector::GetSelectedEnemyHeaderReport() const
{
	if (AGP_EncounterDebugDirector* RuntimeDirector = ResolveRuntimeDirectorForCall(); RuntimeDirector != this)
	{
		return RuntimeDirector->GetSelectedEnemyHeaderReport();
	}

	AGP_EnemyCharacter* Enemy = SelectedEnemy.Get();
	return IsValid(Enemy) ? BuildEnemyHeaderReport(Enemy) : TEXT("No selected enemy\nSelect an enemy from the combo.");
}

FString AGP_EncounterDebugDirector::GetSelectedEnemySkillReport() const
{
	if (AGP_EncounterDebugDirector* RuntimeDirector = ResolveRuntimeDirectorForCall(); RuntimeDirector != this)
	{
		return RuntimeDirector->GetSelectedEnemySkillReport();
	}

	AGP_EnemyCharacter* Enemy = SelectedEnemy.Get();
	return IsValid(Enemy) ? BuildEnemySkillReport(Enemy) : TEXT("No selected enemy");
}

FString AGP_EncounterDebugDirector::GetSelectedEnemyLinkedActorReport() const
{
	if (AGP_EncounterDebugDirector* RuntimeDirector = ResolveRuntimeDirectorForCall(); RuntimeDirector != this)
	{
		return RuntimeDirector->GetSelectedEnemyLinkedActorReport();
	}

	AGP_EnemyCharacter* Enemy = SelectedEnemy.Get();
	return IsValid(Enemy) ? BuildEnemyLinkedActorReport(Enemy) : TEXT("No selected enemy");
}

bool AGP_EncounterDebugDirector::KillSelectedEnemy()
{
	if (AGP_EncounterDebugDirector* RuntimeDirector = ResolveRuntimeDirectorForCall(); RuntimeDirector != this)
	{
		return RuntimeDirector->KillSelectedEnemy();
	}

	AGP_EnemyCharacter* Enemy = SelectedEnemy.Get();
	if (!IsValid(Enemy))
	{
		Log(TEXT("[Fail] Cannot kill: no selected enemy."));
		return false;
	}

	DestroyMatadorDecoysForBoss(Enemy);
	Enemy->RequestDeath(FindPlayerCharacter());
	Log(FString::Printf(TEXT("[OK] Death requested: %s."), *GetNameSafe(Enemy)));
	return true;
}

bool AGP_EncounterDebugDirector::DestroySelectedEnemy()
{
	if (AGP_EncounterDebugDirector* RuntimeDirector = ResolveRuntimeDirectorForCall(); RuntimeDirector != this)
	{
		return RuntimeDirector->DestroySelectedEnemy();
	}

	AGP_EnemyCharacter* Enemy = SelectedEnemy.Get();
	if (!IsValid(Enemy))
	{
		Log(TEXT("[Fail] Cannot destroy: no selected enemy."));
		return false;
	}

	DestroyMatadorDecoysForBoss(Enemy);
	Log(FString::Printf(TEXT("[OK] Destroyed enemy: %s."), *GetNameSafe(Enemy)));
	Enemy->Destroy();
	SelectedEnemy.Reset();
	return true;
}

bool AGP_EncounterDebugDirector::SetSelectedEnemyHealthPercent(float HealthPercent)
{
	if (AGP_EncounterDebugDirector* RuntimeDirector = ResolveRuntimeDirectorForCall(); RuntimeDirector != this)
	{
		return RuntimeDirector->SetSelectedEnemyHealthPercent(HealthPercent);
	}

	AGP_EnemyCharacter* Enemy = SelectedEnemy.Get();
	UAbilitySystemComponent* ASC = IsValid(Enemy) ? Enemy->GetAbilitySystemComponent() : nullptr;
	if (!IsValid(Enemy) || !IsValid(ASC))
	{
		Log(TEXT("[Fail] Cannot set HP: selected enemy has no ASC."));
		return false;
	}

	const float MaxHealth = ASC->GetNumericAttribute(UGP_AttributeSet::GetMaxHealthAttribute());
	if (MaxHealth <= KINDA_SMALL_NUMBER)
	{
		Log(TEXT("[Fail] Cannot set HP: MaxHealth is zero."));
		return false;
	}

	const float NewHealth = FMath::Clamp(HealthPercent, 0.0f, 1.0f) * MaxHealth;
	ASC->SetNumericAttributeBase(UGP_AttributeSet::GetHealthAttribute(), NewHealth);
	if (NewHealth <= KINDA_SMALL_NUMBER)
	{
		Enemy->RequestDeath(FindPlayerCharacter());
	}

	Log(FString::Printf(TEXT("[OK] %s HP set to %.0f%%."), *GetNameSafe(Enemy), HealthPercent * 100.0f));
	return true;
}

bool AGP_EncounterDebugDirector::SetSelectedEnemyAIEnabled(bool bEnabled)
{
	if (AGP_EncounterDebugDirector* RuntimeDirector = ResolveRuntimeDirectorForCall(); RuntimeDirector != this)
	{
		return RuntimeDirector->SetSelectedEnemyAIEnabled(bEnabled);
	}

	AGP_EnemyCharacter* Enemy = SelectedEnemy.Get();
	if (!IsValid(Enemy))
	{
		Log(TEXT("[Fail] Cannot toggle AI: no selected enemy."));
		return false;
	}

	if (bEnabled && !Enemy->GetController())
	{
		Enemy->SpawnDefaultController();
	}

	AAIController* AIController = Cast<AAIController>(Enemy->GetController());
	if (!IsValid(AIController))
	{
		Log(FString::Printf(TEXT("[Warning] %s has no AIController."), *GetNameSafe(Enemy)));
		return false;
	}

	if (bEnabled)
	{
		if (UCharacterMovementComponent* Movement = Enemy->GetCharacterMovement())
		{
			Movement->SetMovementMode(MOVE_Walking);
		}
		if (UBrainComponent* Brain = AIController->GetBrainComponent())
		{
			Brain->RestartLogic();
		}
		Log(FString::Printf(TEXT("[OK] AI enabled: %s."), *GetNameSafe(Enemy)));
	}
	else
	{
		AIController->StopMovement();
		if (UBrainComponent* Brain = AIController->GetBrainComponent())
		{
			Brain->StopLogic(TEXT("Encounter debug disabled AI"));
		}
		if (UCharacterMovementComponent* Movement = Enemy->GetCharacterMovement())
		{
			Movement->StopMovementImmediately();
			Movement->DisableMovement();
		}
		Log(FString::Printf(TEXT("[OK] AI disabled: %s."), *GetNameSafe(Enemy)));
	}

	return true;
}

bool AGP_EncounterDebugDirector::AddLooseTagToSelectedEnemy(FGameplayTag Tag)
{
	if (AGP_EncounterDebugDirector* RuntimeDirector = ResolveRuntimeDirectorForCall(); RuntimeDirector != this)
	{
		return RuntimeDirector->AddLooseTagToSelectedEnemy(Tag);
	}

	AGP_EnemyCharacter* Enemy = SelectedEnemy.Get();
	UAbilitySystemComponent* ASC = IsValid(Enemy) ? Enemy->GetAbilitySystemComponent() : nullptr;
	if (!IsValid(ASC) || !Tag.IsValid())
	{
		Log(TEXT("[Fail] Cannot add tag: missing ASC or invalid tag."));
		return false;
	}

	ASC->AddLooseGameplayTag(Tag);
	Log(FString::Printf(TEXT("[OK] Added tag %s to %s."), *Tag.ToString(), *GetNameSafe(Enemy)));
	return true;
}

bool AGP_EncounterDebugDirector::RemoveLooseTagFromSelectedEnemy(FGameplayTag Tag)
{
	if (AGP_EncounterDebugDirector* RuntimeDirector = ResolveRuntimeDirectorForCall(); RuntimeDirector != this)
	{
		return RuntimeDirector->RemoveLooseTagFromSelectedEnemy(Tag);
	}

	AGP_EnemyCharacter* Enemy = SelectedEnemy.Get();
	UAbilitySystemComponent* ASC = IsValid(Enemy) ? Enemy->GetAbilitySystemComponent() : nullptr;
	if (!IsValid(ASC) || !Tag.IsValid())
	{
		Log(TEXT("[Fail] Cannot remove tag: missing ASC or invalid tag."));
		return false;
	}

	ASC->RemoveLooseGameplayTag(Tag);
	Log(FString::Printf(TEXT("[OK] Removed tag %s from %s."), *Tag.ToString(), *GetNameSafe(Enemy)));
	return true;
}

bool AGP_EncounterDebugDirector::ToggleLooseTagOnSelectedEnemy(FGameplayTag Tag)
{
	if (AGP_EncounterDebugDirector* RuntimeDirector = ResolveRuntimeDirectorForCall(); RuntimeDirector != this)
	{
		return RuntimeDirector->ToggleLooseTagOnSelectedEnemy(Tag);
	}

	AGP_EnemyCharacter* Enemy = SelectedEnemy.Get();
	UAbilitySystemComponent* ASC = IsValid(Enemy) ? Enemy->GetAbilitySystemComponent() : nullptr;
	if (!IsValid(ASC) || !Tag.IsValid())
	{
		Log(TEXT("[Fail] Cannot toggle tag: missing ASC or invalid tag."));
		return false;
	}

	if (ASC->HasMatchingGameplayTag(Tag))
	{
		ASC->RemoveLooseGameplayTag(Tag);
		Log(FString::Printf(TEXT("[OK] Toggled tag off %s on %s."), *Tag.ToString(), *GetNameSafe(Enemy)));
	}
	else
	{
		ASC->AddLooseGameplayTag(Tag);
		Log(FString::Printf(TEXT("[OK] Toggled tag on %s on %s."), *Tag.ToString(), *GetNameSafe(Enemy)));
	}

	return true;
}

bool AGP_EncounterDebugDirector::ApplyGameplayEffectToSelectedEnemy(TSubclassOf<UGameplayEffect> EffectClass, float Level)
{
	if (AGP_EncounterDebugDirector* RuntimeDirector = ResolveRuntimeDirectorForCall(); RuntimeDirector != this)
	{
		return RuntimeDirector->ApplyGameplayEffectToSelectedEnemy(EffectClass, Level);
	}

	AGP_EnemyCharacter* Enemy = SelectedEnemy.Get();
	UAbilitySystemComponent* ASC = IsValid(Enemy) ? Enemy->GetAbilitySystemComponent() : nullptr;
	if (!IsValid(ASC) || !*EffectClass)
	{
		Log(TEXT("[Fail] Cannot apply GameplayEffect: missing ASC or EffectClass."));
		return false;
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(this);
	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(EffectClass, FMath::Max(1.0f, Level), Context);
	if (!SpecHandle.IsValid())
	{
		Log(TEXT("[Fail] Cannot apply GameplayEffect: spec creation failed."));
		return false;
	}

	ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	Log(FString::Printf(TEXT("[OK] Applied GameplayEffect to %s."), *GetNameSafe(Enemy)));
	return true;
}

AGP_EncounterDebugDirector* AGP_EncounterDebugDirector::ResolveRuntimeDirectorForCall() const
{
	if (const UWorld* World = GetWorld(); World && World->IsGameWorld())
	{
		return const_cast<AGP_EncounterDebugDirector*>(this);
	}

	AGP_EncounterDebugDirector* ActiveDirector = FindActiveEncounterDebugDirector();
	if (IsValid(ActiveDirector) && ActiveDirector != this)
	{
		if (const UWorld* ActiveWorld = ActiveDirector->GetWorld(); ActiveWorld && ActiveWorld->IsGameWorld())
		{
			return ActiveDirector;
		}
	}

	return const_cast<AGP_EncounterDebugDirector*>(this);
}

AGP_PlayerCharacter* AGP_EncounterDebugDirector::FindPlayerCharacter() const
{
	return Cast<AGP_PlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
}

bool AGP_EncounterDebugDirector::ResolveStageTransform(int32 StageIndex, FTransform& OutTransform) const
{
	if (!StageSlots.IsValidIndex(StageIndex))
	{
		return false;
	}

	const FGPEncounterDebugStageSlot& Slot = StageSlots[StageIndex];
	if (IsValid(Slot.StagePoint))
	{
		OutTransform = Slot.StagePoint->GetActorTransform();
		return true;
	}

	OutTransform = Slot.FallbackTransform;
	return true;
}

AGP_EnemyCharacter* AGP_EncounterDebugDirector::SpawnEnemyForSelectedStage(EGPEncounterDebugSpawnKind SpawnKind, TSubclassOf<AGP_EnemyCharacter> OverrideClass)
{
	if (!StageSlots.IsValidIndex(SelectedStageIndex))
	{
		Log(TEXT("[Fail] Cannot spawn: invalid selected stage."));
		return nullptr;
	}

	if (bDestroyExistingStageDebugEnemiesBeforeSpawn && SpawnKind == EGPEncounterDebugSpawnKind::Boss)
	{
		DespawnSelectedStageDebugEnemiesOfKind(SpawnKind);
	}

	TSubclassOf<AGP_EnemyCharacter> EnemyClass = OverrideClass;
	if (!*EnemyClass)
	{
		Log(FString::Printf(TEXT("[Fail] Cannot spawn %s: class is empty."), SpawnKind == EGPEncounterDebugSpawnKind::Boss ? TEXT("Boss") : TEXT("Mob")));
		return nullptr;
	}

	UWorld* World = GetWorld();
	FTransform SpawnTransform;
	if (!World || !ResolveStageTransform(SelectedStageIndex, SpawnTransform))
	{
		Log(TEXT("[Fail] Cannot spawn: missing world or stage transform."));
		return nullptr;
	}

	if (SpawnKind == EGPEncounterDebugSpawnKind::Boss)
	{
		SpawnTransform.AddToTranslation(SpawnTransform.GetRotation().GetForwardVector() * BossForwardOffset);
	}
	SpawnTransform.AddToTranslation(FVector::UpVector * SpawnVerticalOffset);

	if (bFaceSpawnedEnemyToPlayer)
	{
		if (AGP_PlayerCharacter* Player = FindPlayerCharacter())
		{
			FVector LookDirection = Player->GetActorLocation() - SpawnTransform.GetLocation();
			LookDirection.Z = 0.0f;
			if (!LookDirection.IsNearlyZero())
			{
				SpawnTransform.SetRotation(LookDirection.Rotation().Quaternion());
			}
		}
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AGP_EnemyCharacter* Enemy = World->SpawnActor<AGP_EnemyCharacter>(EnemyClass, SpawnTransform, Params);
	if (!IsValid(Enemy))
	{
		Log(TEXT("[Fail] SpawnActor failed."));
		return nullptr;
	}

	TrackDebugEnemy(Enemy, SelectedStageIndex, SpawnKind);
	if (bSpawnAIEnabled)
	{
		Enemy->SpawnDefaultController();
	}
	else
	{
		SelectedEnemy = Enemy;
		SetSelectedEnemyAIEnabled(false);
	}

	if (bSelectSpawnedEnemy)
	{
		SelectedEnemy = Enemy;
	}

	Log(FString::Printf(
		TEXT("[Spawn] Stage %d %s spawned: %s."),
		SelectedStageIndex + 1,
		SpawnKind == EGPEncounterDebugSpawnKind::Boss ? TEXT("Boss") : TEXT("Mob"),
		*GetNameSafe(Enemy)));
	return Enemy;
}

int32 AGP_EncounterDebugDirector::DespawnSelectedStageDebugEnemiesOfKind(EGPEncounterDebugSpawnKind SpawnKind)
{
	const FName KindTag = SpawnKind == EGPEncounterDebugSpawnKind::Boss
		? TEXT("EncounterDebugKind_Boss")
		: TEXT("EncounterDebugKind_Mob");

	int32 RemovedCount = 0;
	for (int32 Index = DebugSpawnedEnemies.Num() - 1; Index >= 0; --Index)
	{
		AGP_EnemyCharacter* Enemy = DebugSpawnedEnemies[Index];
		if (!IsValid(Enemy))
		{
			DebugSpawnedEnemies.RemoveAtSwap(Index);
			continue;
		}

		if (ResolveDebugStageIndex(Enemy) == SelectedStageIndex && Enemy->Tags.Contains(KindTag))
		{
			RemovedCount += DestroyMatadorDecoysForBoss(Enemy);
			if (SelectedEnemy.Get() == Enemy)
			{
				SelectedEnemy.Reset();
			}
			Enemy->Destroy();
			DebugSpawnedEnemies.RemoveAtSwap(Index);
			++RemovedCount;
		}
	}

	if (RemovedCount > 0)
	{
		Log(FString::Printf(
			TEXT("[OK] Despawned %d existing %s enemies in stage %d."),
			RemovedCount,
			SpawnKind == EGPEncounterDebugSpawnKind::Boss ? TEXT("Boss") : TEXT("Mob"),
			SelectedStageIndex + 1));
	}

	return RemovedCount;
}

TSubclassOf<AGP_EnemyCharacter> AGP_EncounterDebugDirector::ResolveSelectedClassForSpawn() const
{
	if (AvailableMobClasses.IsValidIndex(SelectedAvailableMobClassIndex) && *AvailableMobClasses[SelectedAvailableMobClassIndex])
	{
		return AvailableMobClasses[SelectedAvailableMobClassIndex];
	}

	const int32 DiscoveredIndex = SelectedAvailableMobClassIndex - AvailableMobClasses.Num();
	if (DiscoveredMobClassPaths.IsValidIndex(DiscoveredIndex))
	{
		return LoadClass<AGP_EnemyCharacter>(nullptr, *DiscoveredMobClassPaths[DiscoveredIndex].ToString());
	}

	return nullptr;
}

void AGP_EncounterDebugDirector::EnsureAvailableMobClassList()
{
	if (!bAutoDiscoverAvailableMobClasses || !DiscoveredMobClassPaths.IsEmpty() || MobClassAssetRootPath.IsNone())
	{
		return;
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FTopLevelAssetPath> EnemyBaseClassNames;
	EnemyBaseClassNames.Add(AGP_EnemyCharacter::StaticClass()->GetClassPathName());

	TSet<FTopLevelAssetPath> EnemyDerivedClassNames;
	AssetRegistry.GetDerivedClassNames(EnemyBaseClassNames, TSet<FTopLevelAssetPath>(), EnemyDerivedClassNames);

	TArray<FAssetData> BlueprintAssets;
	FARFilter Filter;
	Filter.PackagePaths.Add(MobClassAssetRootPath);
	Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;
	AssetRegistry.GetAssets(Filter, BlueprintAssets);

	for (const FAssetData& AssetData : BlueprintAssets)
	{
		FString GeneratedClassPath;
		if (!AssetData.GetTagValue(FName(TEXT("GeneratedClass")), GeneratedClassPath))
		{
			continue;
		}

		const FString ClassObjectPath = FPackageName::ExportTextPathToObjectPath(GeneratedClassPath);
		const FSoftClassPath ClassPath(ClassObjectPath);
		if (!ClassPath.IsValid())
		{
			continue;
		}

		const FTopLevelAssetPath GeneratedClassAssetPath(*ClassObjectPath);
		if (!EnemyDerivedClassNames.Contains(GeneratedClassAssetPath))
		{
			continue;
		}

		DiscoveredMobClassPaths.AddUnique(ClassPath);
	}

	DiscoveredMobClassPaths.Sort([](const FSoftClassPath& Left, const FSoftClassPath& Right)
	{
		return Left.GetAssetName() < Right.GetAssetName();
	});

	DiscoveredMobClassNames.Reset(DiscoveredMobClassPaths.Num());
	for (const FSoftClassPath& ClassPath : DiscoveredMobClassPaths)
	{
		DiscoveredMobClassNames.Add(ClassPath.GetAssetName());
	}

	Log(FString::Printf(TEXT("[OK] Discovered %d enemy class paths from %s."), DiscoveredMobClassPaths.Num(), *MobClassAssetRootPath.ToString()));
}

int32 AGP_EncounterDebugDirector::DestroyMatadorDecoysForBoss(const AActor* BossActor)
{
	if (!IsValid(BossActor) || !BossActor->IsA<AGP_MatadorMageBossCharacter>())
	{
		return 0;
	}

	int32 RemovedCount = 0;
	TArray<AActor*> DecoyActors;
	UGameplayStatics::GetAllActorsOfClass(this, AGP_MatadorBossDecoyActor::StaticClass(), DecoyActors);
	for (AActor* DecoyActor : DecoyActors)
	{
		if (AGP_MatadorBossDecoyActor* MatadorDecoy = Cast<AGP_MatadorBossDecoyActor>(DecoyActor))
		{
			if (MatadorDecoy->GetMainBossActor() == BossActor)
			{
				MatadorDecoy->Destroy();
				++RemovedCount;
			}
		}
	}

	return RemovedCount;
}

void AGP_EncounterDebugDirector::TrackDebugEnemy(AGP_EnemyCharacter* Enemy, int32 StageIndex, EGPEncounterDebugSpawnKind SpawnKind)
{
	if (!IsValid(Enemy))
	{
		return;
	}

	Enemy->Tags.AddUnique(DebugSpawnedActorTag);
	Enemy->Tags.AddUnique(*FString::Printf(TEXT("EncounterDebugStage_%d"), StageIndex));
	Enemy->Tags.AddUnique(SpawnKind == EGPEncounterDebugSpawnKind::Boss ? TEXT("EncounterDebugKind_Boss") : TEXT("EncounterDebugKind_Mob"));
	DebugSpawnedEnemies.AddUnique(Enemy);
}

bool AGP_EncounterDebugDirector::IsDebugSpawnedEnemy(const AGP_EnemyCharacter* Enemy) const
{
	return IsValid(Enemy) && Enemy->Tags.Contains(DebugSpawnedActorTag);
}

int32 AGP_EncounterDebugDirector::ResolveDebugStageIndex(const AGP_EnemyCharacter* Enemy) const
{
	if (!IsValid(Enemy))
	{
		return INDEX_NONE;
	}

	for (const FName& Tag : Enemy->Tags)
	{
		const FString TagText = Tag.ToString();
		int32 ParsedIndex = INDEX_NONE;
		if (TagText.StartsWith(TEXT("EncounterDebugStage_"))
			&& LexTryParseString(ParsedIndex, *TagText.RightChop(20)))
		{
			return ParsedIndex;
		}
	}

	return INDEX_NONE;
}

FGPEncounterDebugEnemySnapshot AGP_EncounterDebugDirector::BuildEnemySnapshot(AGP_EnemyCharacter* Enemy) const
{
	FGPEncounterDebugEnemySnapshot Snapshot;
	Snapshot.Enemy = Enemy;
	Snapshot.Name = GetNameSafe(Enemy);
	Snapshot.ClassName = IsValid(Enemy) ? GetNameSafe(Enemy->GetClass()) : FString();
	Snapshot.StageIndex = ResolveDebugStageIndex(Enemy);
	Snapshot.bBoss = IsValid(Enemy) && Enemy->IsBossEnemy();
	Snapshot.bDead = IsValid(Enemy) && Enemy->IsDead();
	Snapshot.bDebugSpawned = IsDebugSpawnedEnemy(Enemy);
	Snapshot.ControllerName = IsValid(Enemy) ? GetNameSafe(Enemy->GetController()) : FString();

	if (const UAbilitySystemComponent* ASC = IsValid(Enemy) ? Enemy->GetAbilitySystemComponent() : nullptr)
	{
		Snapshot.Health = ASC->GetNumericAttribute(UGP_AttributeSet::GetHealthAttribute());
		Snapshot.MaxHealth = ASC->GetNumericAttribute(UGP_AttributeSet::GetMaxHealthAttribute());
		Snapshot.HealthPercent = Snapshot.MaxHealth > KINDA_SMALL_NUMBER
			? Snapshot.Health / Snapshot.MaxHealth
			: 0.0f;
	}

	if (const AGP_PlayerCharacter* Player = FindPlayerCharacter())
	{
		Snapshot.DistanceToPlayer = IsValid(Enemy)
			? FVector::Dist(Player->GetActorLocation(), Enemy->GetActorLocation())
			: 0.0f;
	}

	return Snapshot;
}

FString AGP_EncounterDebugDirector::BuildEnemyHeaderReport(AGP_EnemyCharacter* Enemy) const
{
	using namespace EncounterDebugReport;

	const FGPEncounterDebugEnemySnapshot Snapshot = BuildEnemySnapshot(Enemy);
	AAIController* AIController = IsValid(Enemy) ? Cast<AAIController>(Enemy->GetController()) : nullptr;
	UBlackboardComponent* BlackboardComponent = IsValid(AIController) ? AIController->GetBlackboardComponent() : nullptr;
	const UObject* TargetActor = IsValid(BlackboardComponent) && HasBlackboardKey(BlackboardComponent, EnemyBlackboardKeys::TargetActor)
		? BlackboardComponent->GetValueAsObject(EnemyBlackboardKeys::TargetActor)
		: nullptr;

	const bool bCanAttack = IsValid(BlackboardComponent)
		&& HasBlackboardKey(BlackboardComponent, EnemyBlackboardKeys::bCanAttack)
		&& BlackboardComponent->GetValueAsBool(EnemyBlackboardKeys::bCanAttack);
	const bool bReturning = IsValid(BlackboardComponent)
		&& HasBlackboardKey(BlackboardComponent, EnemyBlackboardKeys::bShouldReturnHome)
		&& BlackboardComponent->GetValueAsBool(EnemyBlackboardKeys::bShouldReturnHome);

	TArray<FString> Lines;
	Lines.Add(FString::Printf(TEXT("%s  |  HP %.0f%%  |  %.0fcm"), *Snapshot.Name, Snapshot.HealthPercent * 100.0f, Snapshot.DistanceToPlayer));
	Lines.Add(FString::Printf(
		TEXT("%s / %s / Stage %s / Target %s"),
		Snapshot.bBoss ? TEXT("Boss") : TEXT("Mob"),
		Snapshot.bDead ? TEXT("Dead") : TEXT("Alive"),
		Snapshot.StageIndex >= 0 ? *FString::Printf(TEXT("%d"), Snapshot.StageIndex + 1) : TEXT("?"),
		*ActorText(TargetActor)));
	Lines.Add(FString::Printf(TEXT("AI Attack %s  Return %s  Ctrl %s"), *BoolText(bCanAttack), *BoolText(bReturning), *ActorText(AIController)));
	return FString::Join(Lines, TEXT("\n"));
}

FString AGP_EncounterDebugDirector::BuildEnemySkillReport(AGP_EnemyCharacter* Enemy) const
{
	using namespace EncounterDebugReport;

	AAIController* AIController = IsValid(Enemy) ? Cast<AAIController>(Enemy->GetController()) : nullptr;
	UBlackboardComponent* BlackboardComponent = IsValid(AIController) ? AIController->GetBlackboardComponent() : nullptr;
	TArray<FString> Lines;

	if (IsValid(Enemy) && IsValid(BlackboardComponent)
		&& (Enemy->IsBossEnemy() || IsValid(Enemy->FindComponentByClass<UGP_MatadorBossStateComponent>())))
	{
		const FGPBossAttackPatternContext PatternContext =
			BossAttackExecution::BuildPatternContext(Enemy, BlackboardComponent, Enemy->GetDefaultAttackAbilityTag());
		const TArray<FGPBossAttackPatternCandidate> Candidates = FGPBossAttackPatternSelector::BuildCandidates(PatternContext);
		const FGPBossAttackPatternCandidate* BestCandidate = FGPBossAttackPatternSelector::SelectBestCandidate(Candidates);

		Lines.Add(BestCandidate
			? FString::Printf(TEXT("Next  %s  %.2f"), *BestCandidate->DebugName.ToString(), BestCandidate->Score)
			: TEXT("Next  <none>"));

		TArray<FString> TopCandidates;
		for (int32 Index = 0; Index < FMath::Min(3, Candidates.Num()); ++Index)
		{
			TopCandidates.Add(FString::Printf(TEXT("%s %.2f"), *Candidates[Index].DebugName.ToString(), Candidates[Index].Score));
		}
		Lines.Add(FString::Printf(TEXT("Top   %s"), TopCandidates.Num() > 0 ? *FString::Join(TopCandidates, TEXT("  |  ")) : TEXT("<none>")));
		Lines.Add(FString::Printf(
			TEXT("Bull Ready %s  Active %s  Chain %d/%d"),
			*BoolText(PatternContext.bCanUseBullPattern),
			*BoolText(PatternContext.bBullPatternActive),
			PatternContext.ChainBreakCount,
			PatternContext.ChainBreakTarget));
	}
	else
	{
		Lines.Add(FString::Printf(TEXT("Default  %s"), IsValid(Enemy) ? *Enemy->GetDefaultAttackAbilityTag().ToString() : TEXT("<none>")));
	}

	if (UAbilitySystemComponent* ASC = IsValid(Enemy) ? Enemy->GetAbilitySystemComponent() : nullptr)
	{
		TArray<FString> ActiveAbilities;
		FScopedAbilityListLock AbilityScopeLock(*ASC);
		for (const FGameplayAbilitySpec& AbilitySpec : ASC->GetActivatableAbilities())
		{
			if (AbilitySpec.Ability && AbilitySpec.IsActive())
			{
				ActiveAbilities.Add(GetNameSafe(AbilitySpec.Ability));
			}
		}
		Lines.Add(FString::Printf(TEXT("Active %s"), ActiveAbilities.Num() > 0 ? *FString::Join(ActiveAbilities, TEXT(", ")) : TEXT("<none>")));
	}

	return FString::Join(Lines, TEXT("\n"));
}

FString AGP_EncounterDebugDirector::BuildEnemyLinkedActorReport(AGP_EnemyCharacter* Enemy) const
{
	using namespace EncounterDebugReport;

	TArray<FString> Lines;
	UGP_MatadorBossStateComponent* MatadorState = IsValid(Enemy) ? Enemy->FindComponentByClass<UGP_MatadorBossStateComponent>() : nullptr;
	AGP_MatadorBossDecoyActor* Decoy = Cast<AGP_MatadorBossDecoyActor>(Enemy);
	AGP_MatadorMageBossCharacter* MatadorBoss = Cast<AGP_MatadorMageBossCharacter>(Enemy);

	if (!MatadorState && IsValid(Decoy) && IsValid(Decoy->GetMainBossActor()))
	{
		MatadorState = Decoy->GetMainBossActor()->FindComponentByClass<UGP_MatadorBossStateComponent>();
		MatadorBoss = Cast<AGP_MatadorMageBossCharacter>(Decoy->GetMainBossActor());
	}

	if (MatadorState)
	{
		const AActor* BossActor = IsValid(MatadorState->GetMainBossActor()) ? MatadorState->GetMainBossActor() : Cast<AActor>(MatadorBoss);
		const AActor* DecoyActor = MatadorState->GetActiveDecoyActor();
		const AActor* BullActor = MatadorState->GetActiveBullActor();
		Lines.Add(FString::Printf(
			TEXT("Boss %s  Chain %d/%d  Groggy %s"),
			*ActorText(BossActor),
			MatadorState->GetChainBreakCount(),
			MatadorState->GetChainBreakTarget(),
			*BoolText(MatadorState->IsGroggy())));
		Lines.Add(FString::Printf(TEXT("Decoy %s"), *ActorText(DecoyActor)));
		Lines.Add(FString::Printf(TEXT("Bull  %s"), *ActorText(BullActor)));

		if (IsValid(MatadorBoss))
		{
			Lines.Add(FString::Printf(TEXT("Bull pending/active %s"), *BoolText(MatadorBoss->IsBullPatternActive())));
		}
	}

	if (IsValid(Decoy))
	{
		if (UGP_MatadorDecoyPressureComponent* Pressure = Decoy->GetPressureComponent())
		{
			Lines.Add(FString::Printf(
				TEXT("Decoy pressure %s  target %s"),
				*EnumText(StaticEnum<EGPMatadorDecoyPressureState>(), static_cast<int64>(Pressure->GetPressureState())),
				*ActorText(Pressure->GetCurrentTarget())));
			Lines.Add(FString::Printf(
				TEXT("Teleport %s  PostLock %s  Step %d"),
				*BoolText(Pressure->IsTeleportRequested()),
				*BoolText(Pressure->IsPostTeleportAttackLocked()),
				Pressure->GetStepThrustIndex()));
		}
	}

	if (Lines.IsEmpty())
	{
		Lines.Add(TEXT("No linked summon data for this enemy."));
		Lines.Add(TEXT("Matador boss/decoy will show Boss, Decoy, Bull, pressure state here."));
	}

	return FString::Join(Lines, TEXT("\n"));
}

FString AGP_EncounterDebugDirector::BuildEnemyDebugReport(AGP_EnemyCharacter* Enemy, EGPEncounterDebugReportMode ReportMode) const
{
	using namespace EncounterDebugReport;

	const bool bShowSummary = ReportMode == EGPEncounterDebugReportMode::Summary;
	const bool bShowAI = ReportMode == EGPEncounterDebugReportMode::AI;
	const bool bShowSkill = ReportMode == EGPEncounterDebugReportMode::Skill;
	const bool bShowGAS = ReportMode == EGPEncounterDebugReportMode::GAS;
	const bool bShowMatador = ReportMode == EGPEncounterDebugReportMode::Matador;
	const bool bShowAll = ReportMode == EGPEncounterDebugReportMode::All;

	TArray<FString> Lines;
	Lines.Reserve(96);

	const FGPEncounterDebugEnemySnapshot Snapshot = BuildEnemySnapshot(Enemy);
	AAIController* AIController = IsValid(Enemy) ? Cast<AAIController>(Enemy->GetController()) : nullptr;
	UBlackboardComponent* BlackboardComponent = IsValid(AIController) ? AIController->GetBlackboardComponent() : nullptr;

	Lines.Add(FString::Printf(TEXT("[%s] %s"), *EnumText(StaticEnum<EGPEncounterDebugReportMode>(), static_cast<int64>(ReportMode)), *Snapshot.Name));
	Lines.Add(FString::Printf(
		TEXT("HP %.0f%% | Dist %.0f | %s | %s"),
		Snapshot.HealthPercent * 100.0f,
		Snapshot.DistanceToPlayer,
		Snapshot.bBoss ? TEXT("Boss") : TEXT("Mob"),
		Snapshot.bDead ? TEXT("Dead") : TEXT("Alive")));

	if (bShowSummary || bShowAll)
	{
		Lines.Add(FString::Printf(TEXT("Class: %s"), *Snapshot.ClassName));
		Lines.Add(FString::Printf(TEXT("Stage: %s | DebugSpawned: %s"), Snapshot.StageIndex >= 0 ? *FString::Printf(TEXT("%d"), Snapshot.StageIndex + 1) : TEXT("?"), *BoolText(Snapshot.bDebugSpawned)));
		if (const UCharacterMovementComponent* Movement = IsValid(Enemy) ? Enemy->GetCharacterMovement() : nullptr)
		{
			Lines.Add(FString::Printf(TEXT("Move: Mode %d | Vel %.0f | Max %.0f"), static_cast<int32>(Movement->MovementMode), Movement->Velocity.Size(), Movement->MaxWalkSpeed));
		}
		Lines.Add(FString::Printf(TEXT("Target: %s"), *ActorText(IsValid(BlackboardComponent) && HasBlackboardKey(BlackboardComponent, EnemyBlackboardKeys::TargetActor)
			? BlackboardComponent->GetValueAsObject(EnemyBlackboardKeys::TargetActor)
			: nullptr)));
		if (IsValid(BlackboardComponent))
		{
			Lines.Add(FString::Printf(
				TEXT("AI: CanAttack=%s Chase=%s Return=%s LoS=%s"),
				*BoolText(HasBlackboardKey(BlackboardComponent, EnemyBlackboardKeys::bCanAttack) && BlackboardComponent->GetValueAsBool(EnemyBlackboardKeys::bCanAttack)),
				*BoolText(HasBlackboardKey(BlackboardComponent, EnemyBlackboardKeys::bShouldChase) && BlackboardComponent->GetValueAsBool(EnemyBlackboardKeys::bShouldChase)),
				*BoolText(HasBlackboardKey(BlackboardComponent, EnemyBlackboardKeys::bShouldReturnHome) && BlackboardComponent->GetValueAsBool(EnemyBlackboardKeys::bShouldReturnHome)),
				*BoolText(HasBlackboardKey(BlackboardComponent, EnemyBlackboardKeys::bHasLineOfSight) && BlackboardComponent->GetValueAsBool(EnemyBlackboardKeys::bHasLineOfSight))));
		}
	}

	const bool bCanShowPattern = IsValid(Enemy) && IsValid(BlackboardComponent)
		&& (Enemy->IsBossEnemy() || IsValid(Enemy->FindComponentByClass<UGP_MatadorBossStateComponent>()));
	if (bCanShowPattern && (bShowSummary || bShowSkill || bShowAll))
	{
		const FGPBossAttackPatternContext PatternContext =
			BossAttackExecution::BuildPatternContext(Enemy, BlackboardComponent, Enemy->GetDefaultAttackAbilityTag());
		const TArray<FGPBossAttackPatternCandidate> Candidates = FGPBossAttackPatternSelector::BuildCandidates(PatternContext);
		const FGPBossAttackPatternCandidate* BestCandidate = FGPBossAttackPatternSelector::SelectBestCandidate(Candidates);

		Lines.Add(TEXT(""));
		Lines.Add(TEXT("Skill"));
		if (BestCandidate)
		{
			Lines.Add(FString::Printf(TEXT("Next: %s | %.2f"), *BestCandidate->DebugName.ToString(), BestCandidate->Score));
			if (!bShowSummary)
			{
				Lines.Add(FString::Printf(TEXT("Tag: %s"), *BestCandidate->AbilityTag.ToString()));
			}
		}
		else
		{
			Lines.Add(TEXT("Next: <none>"));
		}
		if (!bShowSummary)
		{
			Lines.Add(FString::Printf(TEXT("Candidates: %s"), *FGPBossAttackPatternSelector::DescribeCandidates(Candidates)));
			Lines.Add(FString::Printf(
				TEXT("BullReady=%s BullActive=%s Teleport=%s Chain=%d/%d"),
				*BoolText(PatternContext.bCanUseBullPattern),
				*BoolText(PatternContext.bBullPatternActive),
				*BoolText(PatternContext.bShouldTeleport),
				PatternContext.ChainBreakCount,
				PatternContext.ChainBreakTarget));
		}
	}

	if (bShowAI || bShowAll)
	{
		Lines.Add(TEXT(""));
		Lines.Add(TEXT("AI / Blackboard"));
		Lines.Add(FString::Printf(TEXT("Controller: %s"), *ActorText(AIController)));
		Lines.Add(FString::Printf(TEXT("Brain: %s"), *ActorText(IsValid(AIController) ? AIController->GetBrainComponent() : nullptr)));
		AddBlackboardObject(Lines, BlackboardComponent, EnemyBlackboardKeys::TargetActor);
		AddBlackboardName(Lines, BlackboardComponent, EnemyBlackboardKeys::CombatState);
		AddBlackboardName(Lines, BlackboardComponent, EnemyBlackboardKeys::EnemyMode);
		AddBlackboardName(Lines, BlackboardComponent, EnemyBlackboardKeys::FocusTargetRule);
		AddBlackboardFloat(Lines, BlackboardComponent, EnemyBlackboardKeys::DistanceToTarget);
		AddBlackboardFloat(Lines, BlackboardComponent, EnemyBlackboardKeys::DistanceFromHome);
		AddBlackboardVector(Lines, BlackboardComponent, EnemyBlackboardKeys::HomeLocation);
		AddBlackboardVector(Lines, BlackboardComponent, EnemyBlackboardKeys::MoveToLocation);
		AddBlackboardBool(Lines, BlackboardComponent, EnemyBlackboardKeys::bCanAttack);
		AddBlackboardBool(Lines, BlackboardComponent, EnemyBlackboardKeys::bShouldChase);
		AddBlackboardBool(Lines, BlackboardComponent, EnemyBlackboardKeys::bShouldReturnHome);
		AddBlackboardBool(Lines, BlackboardComponent, EnemyBlackboardKeys::bShouldRetreat);
		AddBlackboardBool(Lines, BlackboardComponent, EnemyBlackboardKeys::bShouldReposition);
		AddBlackboardBool(Lines, BlackboardComponent, EnemyBlackboardKeys::bHasLineOfSight);
		AddBlackboardFloat(Lines, BlackboardComponent, EnemyBlackboardKeys::HealthRatio);
		AddBlackboardInt(Lines, BlackboardComponent, EnemyBlackboardKeys::BossPhase);
		AddBlackboardBool(Lines, BlackboardComponent, EnemyBlackboardKeys::bShouldTeleport);
		AddBlackboardBool(Lines, BlackboardComponent, EnemyBlackboardKeys::bCanUseBullPattern);
		AddBlackboardBool(Lines, BlackboardComponent, EnemyBlackboardKeys::bBullPatternActive);
		AddBlackboardInt(Lines, BlackboardComponent, EnemyBlackboardKeys::ChainBreakCount);
		AddBlackboardBool(Lines, BlackboardComponent, EnemyBlackboardKeys::bIsGroggy);
		AddBlackboardObject(Lines, BlackboardComponent, EnemyBlackboardKeys::DecoyActor);
		AddBlackboardObject(Lines, BlackboardComponent, EnemyBlackboardKeys::MainBossActor);
	}

	if (bShowGAS || bShowAll)
	{
		if (UAbilitySystemComponent* ASC = IsValid(Enemy) ? Enemy->GetAbilitySystemComponent() : nullptr)
		{
			FGameplayTagContainer OwnedTags;
			ASC->GetOwnedGameplayTags(OwnedTags);

			Lines.Add(TEXT(""));
			Lines.Add(TEXT("GAS"));
			Lines.Add(FString::Printf(TEXT("Tags: %s"), OwnedTags.IsEmpty() ? TEXT("<none>") : *OwnedTags.ToStringSimple()));

			TArray<FString> ActiveAbilities;
			TArray<FString> GrantedAbilities;
			FScopedAbilityListLock AbilityScopeLock(*ASC);
			for (const FGameplayAbilitySpec& AbilitySpec : ASC->GetActivatableAbilities())
			{
				if (!AbilitySpec.Ability)
				{
					continue;
				}

				FString AbilityText = GetNameSafe(AbilitySpec.Ability);
				const FGameplayTagContainer AssetTags = AbilitySpec.Ability->GetAssetTags();
				if (!AssetTags.IsEmpty())
				{
					AbilityText += FString::Printf(TEXT(" [%s]"), *AssetTags.ToStringSimple());
				}

				GrantedAbilities.Add(AbilityText);
				if (AbilitySpec.IsActive())
				{
					ActiveAbilities.Add(AbilityText);
				}
			}

			Lines.Add(FString::Printf(TEXT("Active: %s"), ActiveAbilities.Num() > 0 ? *FString::Join(ActiveAbilities, TEXT(", ")) : TEXT("<none>")));
			Lines.Add(FString::Printf(TEXT("Granted(%d): %s"), GrantedAbilities.Num(), GrantedAbilities.Num() > 0 ? *FString::Join(GrantedAbilities, TEXT(", ")) : TEXT("<none>")));
		}
	}

	if (bShowSummary || bShowMatador || bShowAll)
	{
		if (UGP_MatadorBossStateComponent* MatadorState = IsValid(Enemy) ? Enemy->FindComponentByClass<UGP_MatadorBossStateComponent>() : nullptr)
		{
			Lines.Add(TEXT(""));
			Lines.Add(TEXT("Matador Boss"));
			Lines.Add(FString::Printf(TEXT("Chain: %d / %d"), MatadorState->GetChainBreakCount(), MatadorState->GetChainBreakTarget()));
			Lines.Add(FString::Printf(TEXT("Guarded/Groggy: %s / %s"), *BoolText(MatadorState->IsGuarded()), *BoolText(MatadorState->IsGroggy())));
			Lines.Add(FString::Printf(TEXT("Decoy: %s"), *ActorText(MatadorState->GetActiveDecoyActor())));
			Lines.Add(FString::Printf(TEXT("Bull: %s"), *ActorText(MatadorState->GetActiveBullActor())));
		}

		if (AGP_MatadorMageBossCharacter* MatadorBoss = Cast<AGP_MatadorMageBossCharacter>(Enemy))
		{
			Lines.Add(FString::Printf(TEXT("BullActiveOrPending: %s"), *BoolText(MatadorBoss->IsBullPatternActive())));
		}

		if (AGP_MatadorBossDecoyActor* Decoy = Cast<AGP_MatadorBossDecoyActor>(Enemy))
		{
			Lines.Add(TEXT(""));
			Lines.Add(TEXT("Matador Decoy"));
			Lines.Add(FString::Printf(TEXT("MainBoss: %s"), *ActorText(Decoy->GetMainBossActor())));
			if (UGP_MatadorDecoyPressureComponent* Pressure = Decoy->GetPressureComponent())
			{
				Lines.Add(FString::Printf(
					TEXT("Pressure: %s | Active=%s"),
					*EnumText(StaticEnum<EGPMatadorDecoyPressureState>(), static_cast<int64>(Pressure->GetPressureState())),
					*BoolText(Pressure->IsPressureActive())));
				Lines.Add(FString::Printf(TEXT("Target: %s"), *ActorText(Pressure->GetCurrentTarget())));
				Lines.Add(FString::Printf(TEXT("Teleport/PostLock: %s / %s"), *BoolText(Pressure->IsTeleportRequested()), *BoolText(Pressure->IsPostTeleportAttackLocked())));
				Lines.Add(FString::Printf(TEXT("StepThrustIndex: %d"), Pressure->GetStepThrustIndex()));
			}

			if (USkeletalMeshComponent* DecoyMesh = Decoy->GetDecoyMesh())
			{
				Lines.Add(FString::Printf(TEXT("AnimClass: %s"), *GetNameSafe(DecoyMesh->GetAnimClass())));
				if (const UGP_MatadorDecoyAnimInstance* DecoyAnim = Cast<UGP_MatadorDecoyAnimInstance>(DecoyMesh->GetAnimInstance()))
				{
					Lines.Add(FString::Printf(
						TEXT("ABP: %s | Walk=%s"),
						*EnumText(StaticEnum<EGPMatadorDecoyPressureState>(), static_cast<int64>(DecoyAnim->GetPressureState())),
						*BoolText(DecoyAnim->IsWalkingPressure())));
					Lines.Add(FString::Printf(
						TEXT("Presentation: %s %.2f/%.2f"),
						*EnumText(StaticEnum<EGPMatadorDecoyPresentationState>(), static_cast<int64>(DecoyAnim->GetPresentationState())),
						DecoyAnim->GetPresentationStateTime(),
						DecoyAnim->GetPresentationStateDuration()));
				}
			}
		}
	}

	return FString::Join(Lines, TEXT("\n"));
}

void AGP_EncounterDebugDirector::Log(const FString& Message) const
{
	UE_LOG(LogTemp, Log, TEXT("[EncounterDebug] %s"), *Message);
	OnLogMessage.Broadcast(Message);
}
