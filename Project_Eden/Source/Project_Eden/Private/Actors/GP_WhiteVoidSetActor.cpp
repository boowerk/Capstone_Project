#include "Actors/GP_WhiteVoidSetActor.h"

#include "Actors/GP_WhiteVoidSetComponent.h"

AGP_WhiteVoidSetActor::AGP_WhiteVoidSetActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	WhiteVoidSetComponent = CreateDefaultSubobject<UGP_WhiteVoidSetComponent>(TEXT("WhiteVoidSet"));
	SetRootComponent(WhiteVoidSetComponent);
	Tags.AddUnique(TEXT("WhiteVoidSet"));
}

void AGP_WhiteVoidSetActor::RebuildWhiteVoidSet()
{
	if (WhiteVoidSetComponent)
	{
		WhiteVoidSetComponent->RebuildWhiteVoidSet();
	}
}
