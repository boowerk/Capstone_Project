#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "AnimGraphNode_Root.h"
#include "AnimGraphNode_SequencePlayer.h"
#include "AnimGraphNode_Slot.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "AnimationGraphSchema.h"
#include "Characters/GP_EnemyCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "Engine/Blueprint.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSansBossAnimationSetupTest,
	"ProjectEden.Combat.Sans.AnimationSetup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSansBossAnimationSetupTest::RunTest(const FString& Parameters)
{
	const UAnimSequence* RailIdle = LoadObject<UAnimSequence>(
		nullptr,
		TEXT("/Game/Asset/BossAction/Sans/Animations/Sans_Idle_Rail_Loop.Sans_Idle_Rail_Loop"));
	const UAnimBlueprint* AnimBlueprint = LoadObject<UAnimBlueprint>(
		nullptr,
		TEXT("/Game/Asset/BossAction/Sans/AnimBlueprints/ABP_Sans_Boss.ABP_Sans_Boss"));
	UBlueprint* BossBlueprint = LoadObject<UBlueprint>(
		nullptr,
		TEXT("/Game/Characters/EnemyCharacter/Boss/BP_Boss_Sans/BP_Boss_Sans.BP_Boss_Sans"));

	TestNotNull(TEXT("Sans rail idle animation exists"), RailIdle);
	TestNotNull(TEXT("Sans animation Blueprint exists"), AnimBlueprint);
	TestNotNull(TEXT("Sans boss Blueprint exists"), BossBlueprint);
	if (!RailIdle || !AnimBlueprint || !BossBlueprint || !BossBlueprint->GeneratedClass)
	{
		return false;
	}

	const AGP_EnemyCharacter* BossDefaults =
		Cast<AGP_EnemyCharacter>(BossBlueprint->GeneratedClass->GetDefaultObject());
	TestNotNull(TEXT("Sans boss Blueprint uses an enemy character parent"), BossDefaults);
	TestTrue(
		TEXT("Sans boss mesh uses the generated Sans animation Blueprint class"),
		BossDefaults
		&& BossDefaults->GetMesh()
		&& BossDefaults->GetMesh()->GetAnimClass() == AnimBlueprint->GeneratedClass);

	const UAnimGraphNode_SequencePlayer* IdlePlayer = nullptr;
	const UAnimGraphNode_Slot* DefaultSlot = nullptr;
	const UAnimGraphNode_Root* RootNode = nullptr;
	int32 SequencePlayerCount = 0;
	int32 SlotCount = 0;

	TArray<UEdGraph*> AllGraphs;
	AnimBlueprint->GetAllGraphs(AllGraphs);
	for (const UEdGraph* Graph : AllGraphs)
	{
		if (!Graph || !Graph->GetSchema() || !Graph->GetSchema()->IsA<UAnimationGraphSchema>())
		{
			continue;
		}

		for (const UEdGraphNode* GraphNode : Graph->Nodes)
		{
			if (const UAnimGraphNode_SequencePlayer* SequencePlayer =
				Cast<UAnimGraphNode_SequencePlayer>(GraphNode))
			{
				++SequencePlayerCount;
				if (SequencePlayer->GetAnimationAsset() == RailIdle)
				{
					IdlePlayer = SequencePlayer;
				}
			}
			else if (const UAnimGraphNode_Slot* Slot = Cast<UAnimGraphNode_Slot>(GraphNode))
			{
				++SlotCount;
				if (Slot->Node.SlotName == TEXT("DefaultSlot"))
				{
					DefaultSlot = Slot;
				}
			}
			else if (const UAnimGraphNode_Root* Root = Cast<UAnimGraphNode_Root>(GraphNode))
			{
				RootNode = Root;
			}
		}
	}

	TestEqual(TEXT("Sans graph has one fixed base sequence player"), SequencePlayerCount, 1);
	TestEqual(TEXT("Sans graph has one attack slot"), SlotCount, 1);
	TestNotNull(TEXT("The fixed base sequence is Sans_Idle_Rail_Loop"), IdlePlayer);
	TestNotNull(TEXT("The attack layer remains DefaultSlot"), DefaultSlot);
	TestNotNull(TEXT("The animation graph has an output root"), RootNode);
	TestTrue(TEXT("Sans rail idle is configured to loop"), IdlePlayer && IdlePlayer->Node.IsLooping());

	const UEdGraphPin* SlotSourcePin = DefaultSlot
		? DefaultSlot->FindPin(TEXT("Source"), EGPD_Input)
		: nullptr;
	const UEdGraphPin* RootResultPin = RootNode
		? RootNode->FindPin(TEXT("Result"), EGPD_Input)
		: nullptr;
	TestTrue(
		TEXT("Sans rail idle feeds DefaultSlot directly"),
		SlotSourcePin
		&& SlotSourcePin->LinkedTo.Num() == 1
		&& SlotSourcePin->LinkedTo[0]
		&& SlotSourcePin->LinkedTo[0]->GetOwningNode() == IdlePlayer);
	TestTrue(
		TEXT("DefaultSlot feeds the final pose directly"),
		RootResultPin
		&& RootResultPin->LinkedTo.Num() == 1
		&& RootResultPin->LinkedTo[0]
		&& RootResultPin->LinkedTo[0]->GetOwningNode() == DefaultSlot);

	bool bHasLinkedRuntimeSequenceOverride = false;
	if (IdlePlayer)
	{
		for (const UEdGraphPin* Pin : IdlePlayer->Pins)
		{
			if (Pin
				&& Pin->Direction == EGPD_Input
				&& Pin->PinName == TEXT("Sequence")
				&& !Pin->LinkedTo.IsEmpty())
			{
				bHasLinkedRuntimeSequenceOverride = true;
				break;
			}
		}
	}
	TestFalse(
		TEXT("No nullable runtime sequence input can override the authored rail idle"),
		bHasLinkedRuntimeSequenceOverride);

	const TArray<FString> AttackMontagePaths =
	{
		TEXT("/Game/Asset/BossAction/Sans/Montages/AM_Sans_Zombie_Scratch.AM_Sans_Zombie_Scratch"),
		TEXT("/Game/Asset/BossAction/Sans/Montages/AM_Sans_BossSweep.AM_Sans_BossSweep"),
		TEXT("/Game/Asset/BossAction/Sans/Montages/AM_Sans_BossHeavy.AM_Sans_BossHeavy"),
		TEXT("/Game/Asset/BossAction/Sans/Montages/AM_Sans_BossArea.AM_Sans_BossArea")
	};
	for (const FString& MontagePath : AttackMontagePaths)
	{
		const UAnimMontage* Montage = LoadObject<UAnimMontage>(nullptr, *MontagePath);
		TestNotNull(*FString::Printf(TEXT("%s exists"), *MontagePath), Montage);
		TestTrue(
			*FString::Printf(TEXT("%s plays through DefaultSlot"), *MontagePath),
			Montage
			&& !Montage->SlotAnimTracks.IsEmpty()
			&& Montage->SlotAnimTracks[0].SlotName == TEXT("DefaultSlot"));
	}

	return true;
}

#endif
