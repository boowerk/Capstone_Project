#include "PCG/PcgControllerActor.h"

#include "AI/OpenAIRequester.h"
#include "Components/SceneComponent.h"
#include "UI/GP_MinimapSubsystem.h"

APcgControllerActor::APcgControllerActor()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	OpenAIRequester = CreateDefaultSubobject<UOpenAIRequester>(TEXT("OpenAIRequester"));
}

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

	if (bAutoNotifyMinimapAfterApplyParameters)
	{
		// BP에서 적용한 PCG 결과가 월드에 반영될 시간을 주고 현재 배치를 미니맵에 다시 캡처합니다.
		NotifyPcgGenerationFinished(AutoMinimapCaptureDelay);
	}
}

void APcgControllerActor::NotifyPcgGenerationFinished(float MinimapCaptureDelay)
{
	OnPcgLayoutReady.Broadcast(this);

	if (UWorld* World = GetWorld())
	{
		if (UGP_MinimapSubsystem* MinimapSubsystem = World->GetSubsystem<UGP_MinimapSubsystem>())
		{
			MinimapSubsystem->NotifyPcgLayoutReady(MinimapCaptureDelay);
		}
	}
}
