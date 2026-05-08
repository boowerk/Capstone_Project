#include "AI/Contexts/EnvQueryContext_EnemyHomeLocation.h"

#include "AI/Controllers/EnemyAIController.h"
#include "AI/Data/EnemyBlackboardKeys.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/GP_EnemyCharacter.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Point.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

void UEnvQueryContext_EnemyHomeLocation::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
	AActor* QueryOwnerActor = Cast<AActor>(QueryInstance.Owner.Get());
	APawn* QueryOwnerPawn = Cast<APawn>(QueryOwnerActor);

	AEnemyAIController* EnemyAIController = nullptr;
	if (IsValid(QueryOwnerPawn))
	{
		EnemyAIController = Cast<AEnemyAIController>(QueryOwnerPawn->GetController());
	}
	else if (AController* OwnerController = Cast<AController>(QueryOwnerActor))
	{
		EnemyAIController = Cast<AEnemyAIController>(OwnerController);
		QueryOwnerPawn = IsValid(EnemyAIController) ? EnemyAIController->GetPawn() : nullptr;
	}

	FVector HomeLocation = FVector::ZeroVector;
	bool bHasHomeLocation = false;

	if (IsValid(EnemyAIController))
	{
		if (const UBlackboardComponent* BlackboardComponent = EnemyAIController->GetBlackboardComponent())
		{
			if (BlackboardComponent->GetKeyID(EnemyBlackboardKeys::HomeLocation) != FBlackboard::InvalidKey)
			{
				// Runtime EQS should use the same HomeLocation that ReturnHome and leash logic use.
				HomeLocation = BlackboardComponent->GetValueAsVector(EnemyBlackboardKeys::HomeLocation);
				bHasHomeLocation = true;
			}
		}
	}

	if (!bHasHomeLocation && IsValid(QueryOwnerPawn))
	{
		const AGP_EnemyCharacter* EnemyCharacter = Cast<AGP_EnemyCharacter>(QueryOwnerPawn);
		HomeLocation = IsValid(EnemyCharacter) ? EnemyCharacter->GetBehaviorAnchorLocation() : QueryOwnerPawn->GetActorLocation();
		bHasHomeLocation = true;
	}

	if (bHasHomeLocation)
	{
		UEnvQueryItemType_Point::SetContextHelper(ContextData, HomeLocation);
	}
}
