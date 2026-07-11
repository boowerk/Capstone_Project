#include "Game/Testing/GP_LandscapeTestInstructionActor.h"

#include "Components/SceneComponent.h"
#include "Components/TextRenderComponent.h"

AGP_LandscapeTestInstructionActor::AGP_LandscapeTestInstructionActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	InstructionText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("InstructionText"));
	InstructionText->SetupAttachment(SceneRoot);
	InstructionText->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	InstructionText->SetGenerateOverlapEvents(false);
	InstructionText->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	InstructionText->SetWorldSize(55.0f);
	InstructionText->SetTextRenderColor(FColor::White);
	DisplayText = FText::FromString(TEXT("CORRUPTION: 0 / 50 / 100\nREGION EVENTS: WALK INTO ONE STATION"));
	InstructionText->SetText(DisplayText);
}

void AGP_LandscapeTestInstructionActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	EnsureStableRoot();

	// The text component may be reconstructed only when the map is opened in a rendering editor process.
	if (InstructionText)
	{
		InstructionText->SetText(DisplayText);
	}
}

void AGP_LandscapeTestInstructionActor::SetInstructionText(const FText& InText)
{
	EnsureStableRoot();
	DisplayText = InText;
	if (InstructionText)
	{
		InstructionText->SetText(InText);
	}
}

void AGP_LandscapeTestInstructionActor::EnsureStableRoot()
{
	if (SceneRoot && GetRootComponent() != SceneRoot)
	{
		// Older saved instances can still point at the commandlet-stripped text component as their root.
		SetRootComponent(SceneRoot);
	}
}
