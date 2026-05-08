#include "AI/Contexts/EnvQueryContext_EnemyTargetActor.h"

#include "AI/Controllers/EnemyAIController.h"
#include "AI/Data/EnemyBlackboardKeys.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Actor.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

void UEnvQueryContext_EnemyTargetActor::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
	AActor* QueryOwnerActor = Cast<AActor>(QueryInstance.Owner.Get());
	const APawn* QueryOwnerPawn = Cast<APawn>(QueryOwnerActor);

	AEnemyAIController* EnemyAIController = nullptr;
	if (IsValid(QueryOwnerPawn))
	{
		EnemyAIController = Cast<AEnemyAIController>(QueryOwnerPawn->GetController());
	}
	else if (AController* OwnerController = Cast<AController>(QueryOwnerActor))
	{
		EnemyAIController = Cast<AEnemyAIController>(OwnerController);
	}

	if (!IsValid(EnemyAIController))
	{
		return;
	}

	const UBlackboardComponent* BlackboardComponent = EnemyAIController->GetBlackboardComponent();
	if (!IsValid(BlackboardComponent))
	{
		return;
	}

	AActor* TargetActor = Cast<AActor>(BlackboardComponent->GetValueAsObject(EnemyBlackboardKeys::TargetActor));
	if (IsValid(TargetActor))
	{
		UEnvQueryItemType_Actor::SetContextHelper(ContextData, TargetActor);
	}
}
