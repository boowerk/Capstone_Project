#include "Game/Testing/GP_LandscapeTestInstructionActor.h"

#include "Components/TextRenderComponent.h"

AGP_LandscapeTestInstructionActor::AGP_LandscapeTestInstructionActor()
{
	PrimaryActorTick.bCanEverTick = false;

	InstructionText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("InstructionText"));
	SetRootComponent(InstructionText);
	InstructionText->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	InstructionText->SetGenerateOverlapEvents(false);
	InstructionText->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	InstructionText->SetWorldSize(55.0f);
	InstructionText->SetTextRenderColor(FColor::White);
}

void AGP_LandscapeTestInstructionActor::SetInstructionText(const FText& InText)
{
	if (InstructionText)
	{
		InstructionText->SetText(InText);
	}
}
