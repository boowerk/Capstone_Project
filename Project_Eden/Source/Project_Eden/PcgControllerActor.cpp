#include "PcgControllerActor.h"
#include "OpenAIRequester.h"
#include "Components/SceneComponent.h"


// Sets default values
APcgControllerActor::APcgControllerActor()
{
    PrimaryActorTick.bCanEverTick = false;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));

	OpenAIRequester = CreateDefaultSubobject<UOpenAIRequester>(TEXT("OpenAIRequester"));
}

// Called when the game starts or when spawned
void APcgControllerActor::BeginPlay()
{
	Super::BeginPlay();
	
    if (OpenAIRequester)
    {
        OpenAIRequester->OnScatterParams.AddDynamic(this, &APcgControllerActor::HandleScatterParams);

        OpenAIRequester->SendOpenAIRequest();
    }
}

void APcgControllerActor::HandleScatterParams(FPCGScatterParams Params)
{
    ApplyPcgParameters(Params);
}
