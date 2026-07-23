#include "Game/RegionEvents/GP_RegionEventDirector.h"

AGP_RegionEventDirector::AGP_RegionEventDirector()
{
	// Preserve the serialized Blueprint parent without leaving an active runtime system behind.
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
}
