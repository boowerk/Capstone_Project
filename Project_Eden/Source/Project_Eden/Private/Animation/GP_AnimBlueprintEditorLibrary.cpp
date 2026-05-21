#include "Animation/GP_AnimBlueprintEditorLibrary.h"

#if WITH_EDITOR
#include "AnimGraphNode_ChooserPlayer.h"
#include "AnimGraphNode_MotionMatching.h"
#include "Animation/GP_CharacterAnimInstance.h"
#include "ChooserFunctionLibrary.h"
#include "Engine/MemberReference.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Animation/AnimBlueprint.h"
#include "Chooser.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#endif

bool UGP_AnimBlueprintEditorLibrary::ConfigureChooserPlayer(UAnimBlueprint* AnimBlueprint, UChooserTable* ChooserTable, bool bCompileBlueprint)
{
#if WITH_EDITOR
	if (!AnimBlueprint)
	{
		return false;
	}

	if (!ChooserTable)
	{
		return RestoreMotionMatchingOutput(AnimBlueprint, bCompileBlueprint);
	}

	UAnimGraphNode_ChooserPlayer* ChooserPlayerNode = nullptr;
	UEdGraphNode* PoseHistoryNode = nullptr;

	TArray<UEdGraph*> AllGraphs;
	AnimBlueprint->GetAllGraphs(AllGraphs);

	for (UEdGraph* Graph : AllGraphs)
	{
		if (!Graph)
		{
			continue;
		}

		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (!Node)
			{
				continue;
			}

			if (!ChooserPlayerNode)
			{
				ChooserPlayerNode = Cast<UAnimGraphNode_ChooserPlayer>(Node);
			}

			if (!PoseHistoryNode && Node->GetClass()->GetName() == TEXT("AnimGraphNode_PoseSearchHistoryCollector"))
			{
				PoseHistoryNode = Node;
			}
		}
	}

	if (!ChooserPlayerNode || !PoseHistoryNode)
	{
		return false;
	}

	ChooserPlayerNode->Modify();
	ChooserPlayerNode->Node.Chooser = UChooserFunctionLibrary::MakeEvaluateChooser(ChooserTable);
	ChooserPlayerNode->ReconstructNode();

	UEdGraphPin* ChooserPosePin = nullptr;
	for (UEdGraphPin* Pin : ChooserPlayerNode->Pins)
	{
		if (Pin && Pin->PinName == TEXT("Pose") && Pin->Direction == EGPD_Output)
		{
			ChooserPosePin = Pin;
			break;
		}
	}

	UEdGraphPin* PoseHistorySourcePin = nullptr;
	for (UEdGraphPin* Pin : PoseHistoryNode->Pins)
	{
		if (Pin && Pin->PinName == TEXT("Source") && Pin->Direction == EGPD_Input)
		{
			PoseHistorySourcePin = Pin;
			break;
		}
	}

	if (!ChooserPosePin || !PoseHistorySourcePin)
	{
		return false;
	}

	PoseHistoryNode->Modify();
	PoseHistorySourcePin->BreakAllPinLinks();
	ChooserPosePin->BreakAllPinLinks();
	ChooserPosePin->MakeLinkTo(PoseHistorySourcePin);

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(AnimBlueprint);

	if (bCompileBlueprint)
	{
		FKismetEditorUtilities::CompileBlueprint(AnimBlueprint);
	}

	AnimBlueprint->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}

bool UGP_AnimBlueprintEditorLibrary::RestoreMotionMatchingOutput(UAnimBlueprint* AnimBlueprint, bool bCompileBlueprint)
{
#if WITH_EDITOR
	if (!AnimBlueprint)
	{
		return false;
	}

	UEdGraphNode* SelectedMotionMatchingNode = nullptr;
	UEdGraphNode* ChooserPlayerNode = nullptr;
	UEdGraphNode* PoseHistoryNode = nullptr;
	UEdGraphNode* RuntimeDatabaseGetterNode = nullptr;

	TArray<UEdGraph*> AllGraphs;
	AnimBlueprint->GetAllGraphs(AllGraphs);

	for (UEdGraph* Graph : AllGraphs)
	{
		if (!Graph)
		{
			continue;
		}

		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (!Node)
			{
				continue;
			}

			const FString NodeClassName = Node->GetClass()->GetName();
			const FString NodeName = Node->GetName();

			if (!ChooserPlayerNode &&
				(NodeName == TEXT("AnimGraphNode_ChooserPlayer_0") || NodeClassName == TEXT("AnimGraphNode_ChooserPlayer")))
			{
				ChooserPlayerNode = Node;
			}

			if (!PoseHistoryNode &&
				(NodeName == TEXT("AnimGraphNode_PoseSearchHistoryCollector_0") || NodeClassName == TEXT("AnimGraphNode_PoseSearchHistoryCollector")))
			{
				PoseHistoryNode = Node;
			}

			if (!RuntimeDatabaseGetterNode && Node->GetClass()->GetName() == TEXT("K2Node_VariableGet"))
			{
				if (const FName MemberName = Node->GetFName(); MemberName != NAME_None)
				{
					// no-op: keep compiler happy on older toolchains
				}

				for (UEdGraphPin* Pin : Node->Pins)
				{
					if (Pin && Pin->Direction == EGPD_Output && Pin->PinName == TEXT("RuntimePoseSearchDatabase"))
					{
						RuntimeDatabaseGetterNode = Node;
						break;
					}
				}
			}

			if (NodeName == TEXT("AnimGraphNode_MotionMatching_1") || NodeClassName == TEXT("AnimGraphNode_MotionMatching"))
			{
				bool bUsesRuntimeDatabase = false;
				for (UEdGraphPin* Pin : Node->Pins)
				{
					if (!Pin || Pin->Direction != EGPD_Input || Pin->PinName != TEXT("Database"))
					{
						continue;
					}

					for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
					{
						if (LinkedPin && LinkedPin->PinName == TEXT("RuntimePoseSearchDatabase"))
						{
							bUsesRuntimeDatabase = true;
							break;
						}
					}
				}

				if (NodeName == TEXT("AnimGraphNode_MotionMatching_1") || bUsesRuntimeDatabase || !SelectedMotionMatchingNode)
				{
					SelectedMotionMatchingNode = Node;
				}
			}
		}
	}

	if (!SelectedMotionMatchingNode || !PoseHistoryNode)
	{
		return false;
	}

	UEdGraphPin* MotionMatchingPosePin = nullptr;
	UEdGraphPin* MotionMatchingDatabasePin = nullptr;
	for (UEdGraphPin* Pin : SelectedMotionMatchingNode->Pins)
	{
		if (!Pin)
		{
			continue;
		}

		if (Pin->Direction == EGPD_Output && Pin->PinName == TEXT("Pose"))
		{
			MotionMatchingPosePin = Pin;
		}
		else if (Pin->Direction == EGPD_Input && Pin->PinName == TEXT("Database"))
		{
			MotionMatchingDatabasePin = Pin;
		}
	}

	UEdGraphPin* PoseHistorySourcePin = nullptr;
	for (UEdGraphPin* Pin : PoseHistoryNode->Pins)
	{
		if (Pin && Pin->Direction == EGPD_Input && Pin->PinName == TEXT("Source"))
		{
			PoseHistorySourcePin = Pin;
			break;
		}
	}

	UEdGraphPin* RuntimeDatabaseOutputPin = nullptr;
	if (RuntimeDatabaseGetterNode)
	{
		for (UEdGraphPin* Pin : RuntimeDatabaseGetterNode->Pins)
		{
			if (Pin && Pin->Direction == EGPD_Output && Pin->PinName == TEXT("RuntimePoseSearchDatabase"))
			{
				RuntimeDatabaseOutputPin = Pin;
				break;
			}
		}
	}

	if (!MotionMatchingPosePin || !PoseHistorySourcePin)
	{
		return false;
	}

	SelectedMotionMatchingNode->Modify();
	PoseHistoryNode->Modify();

	PoseHistorySourcePin->BreakAllPinLinks();
	MotionMatchingPosePin->BreakAllPinLinks();
	MotionMatchingPosePin->MakeLinkTo(PoseHistorySourcePin);

	if (MotionMatchingDatabasePin && RuntimeDatabaseOutputPin)
	{
		MotionMatchingDatabasePin->BreakAllPinLinks();
		RuntimeDatabaseOutputPin->BreakAllPinLinks();
		RuntimeDatabaseOutputPin->MakeLinkTo(MotionMatchingDatabasePin);
	}

	if (ChooserPlayerNode)
	{
		ChooserPlayerNode->Modify();
		for (UEdGraphPin* Pin : ChooserPlayerNode->Pins)
		{
			if (Pin && Pin->Direction == EGPD_Output && Pin->PinName == TEXT("Pose"))
			{
				Pin->BreakAllPinLinks();
			}
		}
	}

	if (UAnimGraphNode_MotionMatching* MotionMatchingNodeObject = Cast<UAnimGraphNode_MotionMatching>(SelectedMotionMatchingNode))
	{
		MotionMatchingNodeObject->Modify();
		if (FProperty* Prop = MotionMatchingNodeObject->GetClass()->FindPropertyByName(TEXT("OnMotionMatchingStateUpdatedFunction")))
		{
			if (FMemberReference* MemberRef = Prop->ContainerPtrToValuePtr<FMemberReference>(MotionMatchingNodeObject))
			{
				MemberRef->SetSelfMember(TEXT("ApplyRuntimeDatabaseToMotionMatchingNode"));
			}
		}
		MotionMatchingNodeObject->ReconstructNode();
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(AnimBlueprint);

	if (bCompileBlueprint)
	{
		FKismetEditorUtilities::CompileBlueprint(AnimBlueprint);
	}

	AnimBlueprint->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}

bool UGP_AnimBlueprintEditorLibrary::BindMotionMatchingUpdateFunction(UAnimBlueprint* AnimBlueprint, FName FunctionName, bool bCompileBlueprint)
{
#if WITH_EDITOR
	if (!AnimBlueprint || FunctionName.IsNone())
	{
		return false;
	}

	UAnimGraphNode_MotionMatching* MotionMatchingNode = nullptr;

	TArray<UEdGraph*> AllGraphs;
	AnimBlueprint->GetAllGraphs(AllGraphs);

	for (UEdGraph* Graph : AllGraphs)
	{
		if (!Graph)
		{
			continue;
		}

		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (UAnimGraphNode_MotionMatching* Candidate = Cast<UAnimGraphNode_MotionMatching>(Node))
			{
				if (Node->GetName() == TEXT("AnimGraphNode_MotionMatching_1"))
				{
					MotionMatchingNode = Candidate;
					break;
				}

				if (!MotionMatchingNode)
				{
					MotionMatchingNode = Candidate;
				}
			}
		}

		if (MotionMatchingNode)
		{
			break;
		}
	}

	if (!MotionMatchingNode)
	{
		return false;
	}

	MotionMatchingNode->Modify();
	if (FProperty* Prop = MotionMatchingNode->GetClass()->FindPropertyByName(TEXT("OnMotionMatchingStateUpdatedFunction")))
	{
		if (FMemberReference* MemberRef = Prop->ContainerPtrToValuePtr<FMemberReference>(MotionMatchingNode))
		{
			MemberRef->SetSelfMember(FunctionName);
		}
	}
	MotionMatchingNode->ReconstructNode();

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(AnimBlueprint);

	if (bCompileBlueprint)
	{
		FKismetEditorUtilities::CompileBlueprint(AnimBlueprint);
	}

	AnimBlueprint->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}

bool UGP_AnimBlueprintEditorLibrary::BindMotionMatchingFullUpdate(UAnimBlueprint* AnimBlueprint, FName FunctionName, bool bCompileBlueprint)
{
#if WITH_EDITOR
	if (!AnimBlueprint || FunctionName.IsNone())
	{
		return false;
	}

	UAnimGraphNode_MotionMatching* MotionMatchingNode = nullptr;

	TArray<UEdGraph*> AllGraphs;
	AnimBlueprint->GetAllGraphs(AllGraphs);

	for (UEdGraph* Graph : AllGraphs)
	{
		if (!Graph) continue;
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (UAnimGraphNode_MotionMatching* Candidate = Cast<UAnimGraphNode_MotionMatching>(Node))
			{
				MotionMatchingNode = Candidate;
				break;
			}
		}
		if (MotionMatchingNode) break;
	}

	if (!MotionMatchingNode) return false;

	MotionMatchingNode->Modify();
	
	if (FProperty* Prop = MotionMatchingNode->GetClass()->FindPropertyByName(TEXT("OnUpdateFunction")))
	{
		if (FMemberReference* MemberRef = Prop->ContainerPtrToValuePtr<FMemberReference>(MotionMatchingNode))
		{
			MemberRef->SetExternalMember(FunctionName, UGP_CharacterAnimInstance::StaticClass());
		}
	}
	
	if (FProperty* Prop = MotionMatchingNode->GetClass()->FindPropertyByName(TEXT("OnMotionMatchingStateUpdatedFunction")))
	{
		if (FMemberReference* MemberRef = Prop->ContainerPtrToValuePtr<FMemberReference>(MotionMatchingNode))
		{
			MemberRef->SetExternalMember(FunctionName, UGP_CharacterAnimInstance::StaticClass());
		}
	}
	
	MotionMatchingNode->ReconstructNode();

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(AnimBlueprint);
	if (bCompileBlueprint)
	{
		FKismetEditorUtilities::CompileBlueprint(AnimBlueprint);
	}

	AnimBlueprint->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}
