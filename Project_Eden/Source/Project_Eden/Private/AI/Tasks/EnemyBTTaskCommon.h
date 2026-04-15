#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AI/Data/EnemyBlackboardKeys.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/GP_EnemyCharacter.h"
#include "NavigationSystem.h"

class APawn;

namespace EnemyBTTaskCommon
{
	inline bool TryGetTaskContext(UBehaviorTreeComponent& OwnerComp, AAIController*& OutController, APawn*& OutPawn, UBlackboardComponent*& OutBlackboard)
	{
		OutController = OwnerComp.GetAIOwner();
		OutPawn = OutController != nullptr ? OutController->GetPawn() : nullptr;
		OutBlackboard = OwnerComp.GetBlackboardComponent();

		return OutController != nullptr && IsValid(OutPawn) && IsValid(OutBlackboard);
	}

	inline FVector GetBehaviorAnchorLocation(const APawn* InPawn)
	{
		if (const AGP_EnemyCharacter* EnemyCharacter = Cast<AGP_EnemyCharacter>(InPawn))
		{
			return EnemyCharacter->GetBehaviorAnchorLocation();
		}

		return IsValid(InPawn) ? InPawn->GetActorLocation() : FVector::ZeroVector;
	}

	inline float GetClampedBlackboardFloat(const UBlackboardComponent* BlackboardComponent, const FName& KeyName, float DefaultValue, float MinValue, float MaxValue)
	{
		if (!IsValid(BlackboardComponent))
		{
			return DefaultValue;
		}

		const float Value = BlackboardComponent->GetValueAsFloat(KeyName);
		return FMath::Clamp(Value, MinValue, MaxValue);
	}

	inline AActor* GetTargetActor(const UBlackboardComponent* BlackboardComponent)
	{
		return IsValid(BlackboardComponent)
			? Cast<AActor>(BlackboardComponent->GetValueAsObject(EnemyBlackboardKeys::TargetActor))
			: nullptr;
	}

	inline bool TryResolveReachableLocation(UWorld* World, const FVector& DesiredLocation, float SearchRadius, FVector& OutLocation)
	{
		UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
		if (!IsValid(NavSystem))
		{
			return false;
		}

		FNavLocation NavLocation;
		if (NavSystem->ProjectPointToNavigation(DesiredLocation, NavLocation))
		{
			OutLocation = NavLocation.Location;
			return true;
		}

		if (NavSystem->GetRandomReachablePointInRadius(DesiredLocation, SearchRadius, NavLocation))
		{
			OutLocation = NavLocation.Location;
			return true;
		}

		return false;
	}
}
