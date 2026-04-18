#include "Utils/GP_AnimationSetupLibrary.h"

#if WITH_EDITOR

#include "Animation/AnimBlueprint.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimNodeBase.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/BlendSpace.h"
#include "Animation/BlendSpace1D.h"
#include "Animation/GP_FemaleAnimInstance.h"
#include "Animation/PDA_CharacterAnimationSet.h"
#include "AnimationGraphSchema.h"
#include "AnimGraphNode_Base.h"
#include "AnimGraphNode_BlendListByBool.h"
#include "AnimGraphNode_BlendSpacePlayer.h"
#include "AnimGraphNode_Root.h"
#include "AnimGraphNode_SequencePlayer.h"
#include "AnimGraphNode_Slot.h"
#include "AlphaBlend.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Components/SkeletalMeshComponent.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "Engine/SkeletalMesh.h"
#include "Factories/AnimBlueprintFactory.h"
#include "Factories/AnimMontageFactory.h"
#include "Factories/BlendSpaceFactory1D.h"
#include "Factories/DataAssetFactory.h"
#include "EnhancedActionKeyMapping.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputEditorModule.h"
#include "InputMappingContext.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "K2Node_VariableGet.h"
#include "UObject/SavePackage.h"

namespace GPFemaleAnimationSetup
{
	const FString FemaleMeshPath = TEXT("/Game/Asset/CharacterAction/female/female");
	const FString FemaleSkeletonPath = TEXT("/Game/Asset/CharacterAction/female/female_Skeleton");
	const FString FemaleIdlePath = TEXT("/Game/Asset/CharacterAction/female/Animations/femaleIdle_Loop");
	const FString FemaleWalkPath = TEXT("/Game/Asset/CharacterAction/female/Animations/femaleWalk_Loop");
	const FString FemaleJogPath = TEXT("/Game/Asset/CharacterAction/female/Animations/femaleJog_Fwd_Loop");
	const FString FemaleSprintPath = TEXT("/Game/Asset/CharacterAction/female/Animations/femaleSprint_Loop");
	const FString FemaleSprintEnterLeftPath = TEXT("/Game/Asset/CharacterAction/female/Animations/femaleSprint_Enter_L");
	const FString FemaleSprintEnterRightPath = TEXT("/Game/Asset/CharacterAction/female/Animations/femaleSprint_Enter_R");
	const FString FemaleSprintExitLeftPath = TEXT("/Game/Asset/CharacterAction/female/Animations/femaleSprint_Exit_L");
	const FString FemaleSprintExitRightPath = TEXT("/Game/Asset/CharacterAction/female/Animations/femaleSprint_Exit_R");
	const FString FemaleJumpLoopPath = TEXT("/Game/Asset/CharacterAction/female/Animations/femaleJump_Loop");
	// 기본 공격도 루트모션 몽타주를 사용한다.
	const FString FemaleSwordAttackPath = TEXT("/Game/Asset/CharacterAction/female/Animations/femaleSword_Attack_RM");
	const FString FemaleJumpLandMontagePath = TEXT("/Game/Asset/CharacterAction/female/Montages/AM_Female_Jump_Land");
	// 구르기는 루트모션 몽타주를 사용해서 애니메이션 전진량으로 이동한다.
	const FString FemaleRollMontagePath = TEXT("/Game/Asset/CharacterAction/female/Montages/AM_Female_Roll_RM");

	const FString BlendSpacePackagePath = TEXT("/Game/Asset/CharacterAction/female/BlendSpaces");
	const FString BlendSpaceName = TEXT("BS_Female_Locomotion");
	const FString AnimBlueprintPackagePath = TEXT("/Game/Asset/CharacterAction/female/AnimBlueprints");
	const FString AnimBlueprintName = TEXT("ABP_Female_Player");
	const FString AnimationSetPackagePath = TEXT("/Game/Asset/CharacterAction/female/DataAssets");
	const FString AnimationSetName = TEXT("PDA_FemaleAnimationSet");
	const FString MontagePackagePath = TEXT("/Game/Asset/CharacterAction/female/Montages");
	const FString PrimaryMontageName = TEXT("AM_Female_Primary_RM");
	const FString SprintEnterLeftMontageName = TEXT("AM_Female_Sprint_Enter_L");
	const FString SprintEnterRightMontageName = TEXT("AM_Female_Sprint_Enter_R");
	const FString SprintExitLeftMontageName = TEXT("AM_Female_Sprint_Exit_L");
	const FString SprintExitRightMontageName = TEXT("AM_Female_Sprint_Exit_R");
	const FName LocomotionSyncGroupName(TEXT("Locomotion"));
	const float LocomotionInputSmoothingTime = 0.12f;
	const float LocomotionSampleWeightSpeed = 8.0f;
	const float SprintEnterMontageBlendInTime = 0.18f;
	const float SprintEnterMontageBlendOutTime = 0.28f;
	const float SprintExitMontageBlendInTime = 0.14f;
	const float SprintExitMontageBlendOutTime = 0.22f;
	const FString PlayerBlueprintPath = TEXT("/Game/Characters/PlayerCharacter/BP_GP_PlayerCharacter");
	const FString PlayerControllerBlueprintPath = TEXT("/Game/GAS_Pattern/Player/BP_GP_PlayerController");
	const FString MovementMappingContextPath = TEXT("/Game/GAS_Pattern/Input/IMC_Movement");
	const FString DashActionPackagePath = TEXT("/Game/GAS_Pattern/Input/MovementActions");
	const FString DashActionName = TEXT("IA_Dash");

	template <typename TObjectType>
	TObjectType* LoadRequiredAsset(const FString& AssetPath)
	{
		TObjectType* Asset = LoadObject<TObjectType>(nullptr, *AssetPath);
		if (!Asset)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to load asset: %s"), *AssetPath);
		}

		return Asset;
	}

	bool SaveAsset(UObject* Asset)
	{
		if (!IsValid(Asset))
		{
			return false;
		}

		UPackage* Package = Asset->GetOutermost();
		if (!IsValid(Package))
		{
			return false;
		}

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;

		return UPackage::SavePackage(Package, Asset, *FPackageName::LongPackageNameToFilename(
			Package->GetName(),
			FPackageName::GetAssetPackageExtension()
		), SaveArgs);
	}

	FProperty* FindPropertyChecked(const UStruct* OwnerStruct, const FName PropertyName)
	{
		FProperty* Property = FindFProperty<FProperty>(OwnerStruct, PropertyName);
		if (!Property)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to find property '%s' on '%s'"), *PropertyName.ToString(), *GetNameSafe(OwnerStruct));
		}

		return Property;
	}

	UEdGraphPin* FindPinChecked(UEdGraphNode* Node, const FName PinName, EEdGraphPinDirection Direction)
	{
		if (!IsValid(Node))
		{
			return nullptr;
		}

		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin && Pin->PinName == PinName && Pin->Direction == Direction)
			{
				return Pin;
			}
		}

		UE_LOG(LogTemp, Error, TEXT("Failed to find pin '%s' on node '%s'"), *PinName.ToString(), *Node->GetName());
		return nullptr;
	}

	UEdGraphPin* FindPoseOutputPin(UEdGraphNode* Node)
	{
		if (!IsValid(Node))
		{
			return nullptr;
		}

		const UAnimationGraphSchema* AnimationSchema = GetDefault<UAnimationGraphSchema>();
		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin && Pin->Direction == EGPD_Output && AnimationSchema->IsPosePin(Pin->PinType))
			{
				return Pin;
			}
		}

		UE_LOG(LogTemp, Error, TEXT("Failed to find pose output pin on node '%s'"), *Node->GetName());
		return nullptr;
	}

	UEdGraphPin* FindFirstOutputPin(UEdGraphNode* Node)
	{
		if (!IsValid(Node))
		{
			return nullptr;
		}

		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin && Pin->Direction == EGPD_Output)
			{
				return Pin;
			}
		}

		UE_LOG(LogTemp, Error, TEXT("Failed to find output pin on node '%s'"), *Node->GetName());
		return nullptr;
	}

	bool ConnectPins(UEdGraphPin* FromPin, UEdGraphPin* ToPin)
	{
		if (!FromPin || !ToPin)
		{
			return false;
		}

		const UEdGraphSchema* Schema = FromPin->GetOwningNode()->GetGraph()->GetSchema();
		if (!Schema)
		{
			return false;
		}

		return Schema->TryCreateConnection(FromPin, ToPin);
	}

	bool ShowOptionalInputPin(UAnimGraphNode_Base* Node, const FName PropertyName)
	{
		if (!IsValid(Node))
		{
			return false;
		}

		const int32 OptionalPinIndex = Node->ShowPinForProperties.IndexOfByPredicate([&PropertyName](const FOptionalPinFromProperty& OptionalPin)
		{
			return OptionalPin.PropertyName == PropertyName;
		});

		if (OptionalPinIndex == INDEX_NONE)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to find optional pin '%s' on node '%s'"), *PropertyName.ToString(), *Node->GetName());
			return false;
		}

		Node->SetPinVisibility(true, OptionalPinIndex);
		return true;
	}

	UBlendSpace1D* CreateOrUpdateFemaleLocomotionBlendSpace(USkeleton* Skeleton, USkeletalMesh* SkeletalMesh)
	{
		if (!IsValid(Skeleton) || !IsValid(SkeletalMesh))
		{
			return nullptr;
		}

		const FString BlendSpaceObjectPath = FString::Printf(TEXT("%s/%s.%s"), *BlendSpacePackagePath, *BlendSpaceName, *BlendSpaceName);
		UBlendSpace1D* BlendSpace = LoadObject<UBlendSpace1D>(nullptr, *BlendSpaceObjectPath);

		if (!BlendSpace)
		{
			UBlendSpaceFactory1D* Factory = NewObject<UBlendSpaceFactory1D>();
			Factory->TargetSkeleton = Skeleton;
			Factory->PreviewSkeletalMesh = SkeletalMesh;

			BlendSpace = Cast<UBlendSpace1D>(FAssetToolsModule::GetModule().Get().CreateAsset(
				BlendSpaceName,
				BlendSpacePackagePath,
				UBlendSpace1D::StaticClass(),
				Factory
			));
		}

		if (!IsValid(BlendSpace))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create female locomotion blend space"));
			return nullptr;
		}

		BlendSpace->Modify();

		if (FStructProperty* BlendParametersProperty = CastField<FStructProperty>(FindPropertyChecked(UBlendSpace::StaticClass(), TEXT("BlendParameters"))))
		{
			FBlendParameter* BlendParameters = BlendParametersProperty->ContainerPtrToValuePtr<FBlendParameter>(BlendSpace);
			BlendParameters[0].DisplayName = TEXT("Speed");
			BlendParameters[0].Min = 0.0f;
			BlendParameters[0].Max = 500.0f;
			BlendParameters[0].GridNum = 5;
			BlendParameters[0].bSnapToGrid = false;
			BlendParameters[0].bWrapInput = false;
		}

		// Walk/Jog/SprintLoop 사이 샘플 전환을 부드럽게 해서 Enter/Exit 몽타지가 얹힐 때 하체가 덜 튄다.
		BlendSpace->InterpolationParam[0].InterpolationTime = LocomotionInputSmoothingTime;
		BlendSpace->TargetWeightInterpolationSpeedPerSec = LocomotionSampleWeightSpeed;
		BlendSpace->bTargetWeightInterpolationEaseInOut = true;

		for (int32 SampleIndex = BlendSpace->GetNumberOfBlendSamples() - 1; SampleIndex >= 0; --SampleIndex)
		{
			BlendSpace->DeleteSample(SampleIndex);
		}

		UAnimSequence* Idle = LoadRequiredAsset<UAnimSequence>(*FemaleIdlePath);
		UAnimSequence* Walk = LoadRequiredAsset<UAnimSequence>(*FemaleWalkPath);
		UAnimSequence* Jog = LoadRequiredAsset<UAnimSequence>(*FemaleJogPath);
		UAnimSequence* Sprint = LoadRequiredAsset<UAnimSequence>(*FemaleSprintPath);
		if (!Idle || !Walk || !Jog || !Sprint)
		{
			return nullptr;
		}

		BlendSpace->AddSample(Idle, FVector(0.0f, 0.0f, 0.0f));
		BlendSpace->AddSample(Walk, FVector(150.0f, 0.0f, 0.0f));
		BlendSpace->AddSample(Jog, FVector(300.0f, 0.0f, 0.0f));
		BlendSpace->AddSample(Sprint, FVector(500.0f, 0.0f, 0.0f));
		BlendSpace->ValidateSampleData();
		BlendSpace->ResampleData();
		BlendSpace->MarkPackageDirty();

		if (!SaveAsset(BlendSpace))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to save female locomotion blend space"));
			return nullptr;
		}

		return BlendSpace;
	}

	UAnimMontage* CreateOrUpdateMontageFromSequence(
		USkeleton* Skeleton,
		const FString& SourceAnimationPath,
		const FString& MontageName,
		const TCHAR* LogName)
	{
		if (!IsValid(Skeleton))
		{
			return nullptr;
		}

		const FString MontageObjectPath = FString::Printf(TEXT("%s/%s.%s"), *MontagePackagePath, *MontageName, *MontageName);
		UAnimMontage* Montage = LoadObject<UAnimMontage>(nullptr, *MontageObjectPath);
		if (Montage)
		{
			return Montage;
		}

		UAnimSequence* SourceAnimation = LoadRequiredAsset<UAnimSequence>(SourceAnimationPath);
		if (!SourceAnimation)
		{
			return nullptr;
		}

		UAnimMontageFactory* Factory = NewObject<UAnimMontageFactory>();
		Factory->TargetSkeleton = Skeleton;
		Factory->SourceAnimation = SourceAnimation;

		Montage = Cast<UAnimMontage>(FAssetToolsModule::GetModule().Get().CreateAsset(
			MontageName,
			MontagePackagePath,
			UAnimMontage::StaticClass(),
			Factory
		));

		if (!IsValid(Montage))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create %s montage"), LogName);
			return nullptr;
		}

		Montage->MarkPackageDirty();
		if (!SaveAsset(Montage))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to save %s montage"), LogName);
			return nullptr;
		}

		return Montage;
	}

	UAnimMontage* CreateOrUpdatePrimaryMontage(USkeleton* Skeleton)
	{
		return CreateOrUpdateMontageFromSequence(Skeleton, FemaleSwordAttackPath, PrimaryMontageName, TEXT("female primary"));
	}

	UAnimMontage* CreateOrUpdateSprintTransitionMontage(
		USkeleton* Skeleton,
		const FString& SourceAnimationPath,
		const FString& MontageName,
		const float BlendInTime,
		const float BlendOutTime,
		const TCHAR* LogName)
	{
		UAnimMontage* Montage = CreateOrUpdateMontageFromSequence(Skeleton, SourceAnimationPath, MontageName, LogName);
		if (!IsValid(Montage))
		{
			return nullptr;
		}

		Montage->Modify();
		// Sprint 전환 몽타지도 Locomotion Sync Group을 사용해야 LeftPlant/RightPlant 기준으로 자연스럽게 맞는다.
		Montage->BlendIn.SetBlendOption(EAlphaBlendOption::Cubic);
		Montage->BlendIn.SetBlendTime(BlendInTime);
		Montage->BlendOut.SetBlendOption(EAlphaBlendOption::Cubic);
		Montage->BlendOut.SetBlendTime(BlendOutTime);
		Montage->BlendOutTriggerTime = -1.0f;
		Montage->SyncGroup = LocomotionSyncGroupName;
		Montage->SyncSlotIndex = 0;
		Montage->MarkPackageDirty();

		if (!SaveAsset(Montage))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to save %s sprint transition montage"), LogName);
			return nullptr;
		}

		return Montage;
	}

	UPDA_CharacterAnimationSet* CreateOrUpdateFemaleAnimationSet(
		USkeletalMesh* SkeletalMesh,
		UBlendSpace* BlendSpace,
		UAnimSequenceBase* JumpLoop,
		UAnimMontage* PrimaryMontage,
		UAnimMontage* SprintEnterLeftMontage,
		UAnimMontage* SprintEnterRightMontage,
		UAnimMontage* SprintExitLeftMontage,
		UAnimMontage* SprintExitRightMontage)
	{
		if (!IsValid(SkeletalMesh) || !IsValid(BlendSpace) || !IsValid(JumpLoop) || !IsValid(PrimaryMontage) || !IsValid(SprintEnterLeftMontage) || !IsValid(SprintEnterRightMontage) || !IsValid(SprintExitLeftMontage) || !IsValid(SprintExitRightMontage))
		{
			return nullptr;
		}

		UAnimMontage* RollMontage = LoadRequiredAsset<UAnimMontage>(*FemaleRollMontagePath);
		UAnimMontage* LandingMontage = LoadRequiredAsset<UAnimMontage>(*FemaleJumpLandMontagePath);
		if (!IsValid(RollMontage) || !IsValid(LandingMontage))
		{
			return nullptr;
		}

		const FString AnimationSetObjectPath = FString::Printf(TEXT("%s/%s.%s"), *AnimationSetPackagePath, *AnimationSetName, *AnimationSetName);
		UPDA_CharacterAnimationSet* AnimationSet = LoadObject<UPDA_CharacterAnimationSet>(nullptr, *AnimationSetObjectPath);
		if (!AnimationSet)
		{
			UDataAssetFactory* Factory = NewObject<UDataAssetFactory>();
			Factory->DataAssetClass = UPDA_CharacterAnimationSet::StaticClass();

			AnimationSet = Cast<UPDA_CharacterAnimationSet>(FAssetToolsModule::GetModule().Get().CreateAsset(
				AnimationSetName,
				AnimationSetPackagePath,
				UPDA_CharacterAnimationSet::StaticClass(),
				Factory
			));
		}

		if (!IsValid(AnimationSet))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create female animation set data asset"));
			return nullptr;
		}

		AnimationSet->Modify();
		AnimationSet->CharacterMesh = SkeletalMesh;
		AnimationSet->LocomotionBlendSpace = BlendSpace;
		AnimationSet->JumpLoopAnimation = JumpLoop;
		AnimationSet->LandingMontage = LandingMontage;
		AnimationSet->RollMontage = RollMontage;
		AnimationSet->PrimaryAttackMontage = PrimaryMontage;
		AnimationSet->SprintEnterLeftMontage = SprintEnterLeftMontage;
		AnimationSet->SprintEnterRightMontage = SprintEnterRightMontage;
		AnimationSet->SprintExitLeftMontage = SprintExitLeftMontage;
		AnimationSet->SprintExitRightMontage = SprintExitRightMontage;
		AnimationSet->MarkPackageDirty();

		if (!SaveAsset(AnimationSet))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to save female animation set data asset"));
			return nullptr;
		}

		return AnimationSet;
	}

	UAnimBlueprint* CreateOrUpdateFemaleAnimBlueprint(USkeleton* Skeleton, USkeletalMesh* SkeletalMesh, UBlendSpace1D* BlendSpace, UAnimSequence* JumpLoop)
	{
		if (!IsValid(Skeleton) || !IsValid(SkeletalMesh) || !IsValid(BlendSpace) || !IsValid(JumpLoop))
		{
			return nullptr;
		}

		const FString BlueprintObjectPath = FString::Printf(TEXT("%s/%s.%s"), *AnimBlueprintPackagePath, *AnimBlueprintName, *AnimBlueprintName);
		UAnimBlueprint* AnimBlueprint = LoadObject<UAnimBlueprint>(nullptr, *BlueprintObjectPath);

		if (!AnimBlueprint)
		{
			UAnimBlueprintFactory* Factory = NewObject<UAnimBlueprintFactory>();
			Factory->ParentClass = UGP_FemaleAnimInstance::StaticClass();
			Factory->TargetSkeleton = Skeleton;
			Factory->PreviewSkeletalMesh = SkeletalMesh;

			AnimBlueprint = Cast<UAnimBlueprint>(FAssetToolsModule::GetModule().Get().CreateAsset(
				AnimBlueprintName,
				AnimBlueprintPackagePath,
				UAnimBlueprint::StaticClass(),
				Factory
			));
		}

		if (!IsValid(AnimBlueprint))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create female anim blueprint"));
			return nullptr;
		}

		AnimBlueprint->ParentClass = UGP_FemaleAnimInstance::StaticClass();
		AnimBlueprint->TargetSkeleton = Skeleton;

		UEdGraph* AnimationGraph = nullptr;
		auto TryFindAnimationGraph = [&AnimationGraph](const TArray<TObjectPtr<UEdGraph>>& Graphs)
		{
			for (UEdGraph* Graph : Graphs)
			{
				if (Graph && Graph->GetSchema()->IsA(UAnimationGraphSchema::StaticClass()))
				{
					AnimationGraph = Graph;
					return;
				}
			}
		};

		TryFindAnimationGraph(AnimBlueprint->FunctionGraphs);
		if (!AnimationGraph)
		{
			TryFindAnimationGraph(AnimBlueprint->UbergraphPages);
		}
		if (!AnimationGraph)
		{
			TryFindAnimationGraph(AnimBlueprint->IntermediateGeneratedGraphs);
		}
		if (!AnimationGraph)
		{
			TryFindAnimationGraph(AnimBlueprint->MacroGraphs);
		}

		if (!AnimationGraph)
		{
			UE_LOG(LogTemp, Error, TEXT("Female anim blueprint has no animation graph"));
			return nullptr;
		}
		AnimationGraph->Modify();

		UAnimGraphNode_Root* RootNode = nullptr;
		TArray<UEdGraphNode*> ExistingNodes = AnimationGraph->Nodes;
		for (UEdGraphNode* Node : ExistingNodes)
		{
			if (UAnimGraphNode_Root* CandidateRoot = Cast<UAnimGraphNode_Root>(Node))
			{
				RootNode = CandidateRoot;
				continue;
			}

			AnimationGraph->RemoveNode(Node);
		}

		if (!IsValid(RootNode))
		{
			UE_LOG(LogTemp, Error, TEXT("Female anim blueprint root node was not found"));
			return nullptr;
		}

		FGraphNodeCreator<UAnimGraphNode_BlendSpacePlayer> BlendSpaceNodeCreator(*AnimationGraph);
		UAnimGraphNode_BlendSpacePlayer* BlendSpaceNode = BlendSpaceNodeCreator.CreateNode();
		BlendSpaceNode->NodePosX = -650;
		BlendSpaceNode->NodePosY = 0;
		BlendSpaceNode->SetAnimationAsset(BlendSpace);
		// BlendSpace가 LeftPlant/RightPlant Sync Marker를 내보내야 C++ 전환 로직이 같은 발 위상을 읽을 수 있다.
		BlendSpaceNode->Node.SetGroupName(LocomotionSyncGroupName);
		BlendSpaceNode->Node.SetGroupRole(EAnimGroupRole::CanBeLeader);
		BlendSpaceNode->Node.SetGroupMethod(EAnimSyncMethod::SyncGroup);
		BlendSpaceNodeCreator.Finalize();
		if (!ShowOptionalInputPin(BlendSpaceNode, TEXT("BlendSpace")))
		{
			return nullptr;
		}

		FGraphNodeCreator<UAnimGraphNode_SequencePlayer> JumpNodeCreator(*AnimationGraph);
		UAnimGraphNode_SequencePlayer* JumpNode = JumpNodeCreator.CreateNode();
		JumpNode->NodePosX = -650;
		JumpNode->NodePosY = 240;
		JumpNode->SetAnimationAsset(JumpLoop);
		JumpNodeCreator.Finalize();
		if (!ShowOptionalInputPin(JumpNode, TEXT("Sequence")))
		{
			return nullptr;
		}

		FGraphNodeCreator<UAnimGraphNode_BlendListByBool> BlendByBoolNodeCreator(*AnimationGraph);
		UAnimGraphNode_BlendListByBool* BlendByBoolNode = BlendByBoolNodeCreator.CreateNode();
		BlendByBoolNode->Node.AddPose();
		BlendByBoolNode->Node.AddPose();
		BlendByBoolNode->NodePosX = -260;
		BlendByBoolNode->NodePosY = 120;
		BlendByBoolNodeCreator.Finalize();

		FGraphNodeCreator<UAnimGraphNode_Slot> SlotNodeCreator(*AnimationGraph);
		UAnimGraphNode_Slot* SlotNode = SlotNodeCreator.CreateNode();
		SlotNode->Node.SlotName = FName(TEXT("DefaultSlot"));
		SlotNode->NodePosX = 90;
		SlotNode->NodePosY = 120;
		SlotNodeCreator.Finalize();

		const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

		UK2Node_VariableGet* LocomotionBlendSpaceNode = K2Schema->SpawnVariableGetNode(FVector2D(-980.0, -200.0), AnimationGraph, TEXT("LocomotionBlendSpaceAsset"), UGP_FemaleAnimInstance::StaticClass());
		UK2Node_VariableGet* JumpLoopAssetNode = K2Schema->SpawnVariableGetNode(FVector2D(-980.0, 360.0), AnimationGraph, TEXT("JumpLoopAnimationAsset"), UGP_FemaleAnimInstance::StaticClass());
		UK2Node_VariableGet* GroundSpeedNode = K2Schema->SpawnVariableGetNode(FVector2D(-960.0, -40.0), AnimationGraph, TEXT("GroundSpeed"), UGP_FemaleAnimInstance::StaticClass());
		UK2Node_VariableGet* IsFallingNode = K2Schema->SpawnVariableGetNode(FVector2D(-960.0, 240.0), AnimationGraph, TEXT("bIsFalling"), UGP_FemaleAnimInstance::StaticClass());

		if (!LocomotionBlendSpaceNode || !JumpLoopAssetNode || !GroundSpeedNode || !IsFallingNode)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to spawn anim blueprint variable getter nodes"));
			return nullptr;
		}

		if (!ConnectPins(FindFirstOutputPin(LocomotionBlendSpaceNode), FindPinChecked(BlendSpaceNode, TEXT("BlendSpace"), EGPD_Input)))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to connect locomotion blendspace asset"));
			return nullptr;
		}

		if (!ConnectPins(FindFirstOutputPin(JumpLoopAssetNode), FindPinChecked(JumpNode, TEXT("Sequence"), EGPD_Input)))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to connect jump loop asset"));
			return nullptr;
		}

		if (!ConnectPins(FindFirstOutputPin(GroundSpeedNode), FindPinChecked(BlendSpaceNode, TEXT("X"), EGPD_Input)))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to connect GroundSpeed to blend space"));
			return nullptr;
		}

		if (!ConnectPins(FindFirstOutputPin(IsFallingNode), FindPinChecked(BlendByBoolNode, TEXT("bActiveValue"), EGPD_Input)))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to connect bIsFalling to blend node"));
			return nullptr;
		}

		// BlendListByBool uses input 0 when true and input 1 when false.
		if (!ConnectPins(FindPoseOutputPin(JumpNode), FindPinChecked(BlendByBoolNode, TEXT("BlendPose_0"), EGPD_Input)))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to connect jump pose"));
			return nullptr;
		}

		if (!ConnectPins(FindPoseOutputPin(BlendSpaceNode), FindPinChecked(BlendByBoolNode, TEXT("BlendPose_1"), EGPD_Input)))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to connect locomotion pose"));
			return nullptr;
		}

		if (!ConnectPins(FindPoseOutputPin(BlendByBoolNode), FindPinChecked(SlotNode, TEXT("Source"), EGPD_Input)))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to connect slot source pose"));
			return nullptr;
		}

		if (!ConnectPins(FindPoseOutputPin(SlotNode), FindPinChecked(RootNode, TEXT("Result"), EGPD_Input)))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to connect slot pose to root"));
			return nullptr;
		}

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(AnimBlueprint);
		FKismetEditorUtilities::CompileBlueprint(AnimBlueprint);

		if (!SaveAsset(AnimBlueprint))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to save female anim blueprint"));
			return nullptr;
		}

		return AnimBlueprint;
	}

	bool AssignPlayerBlueprint(USkeletalMesh* SkeletalMesh, UAnimBlueprint* AnimBlueprint, UPDA_CharacterAnimationSet* AnimationSet)
	{
		UBlueprint* PlayerBlueprint = LoadRequiredAsset<UBlueprint>(*PlayerBlueprintPath);
		if (!IsValid(PlayerBlueprint) || !PlayerBlueprint->GeneratedClass)
		{
			return false;
		}

		AActor* DefaultActor = Cast<AActor>(PlayerBlueprint->GeneratedClass->GetDefaultObject());
		if (!IsValid(DefaultActor))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to get player blueprint default actor"));
			return false;
		}

		USkeletalMeshComponent* MeshComponent = DefaultActor->FindComponentByClass<USkeletalMeshComponent>();
		if (!IsValid(MeshComponent))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to find player skeletal mesh component"));
			return false;
		}

		MeshComponent->Modify();
		MeshComponent->SetSkeletalMesh(SkeletalMesh);
		MeshComponent->SetAnimInstanceClass(AnimBlueprint->GeneratedClass);
		MeshComponent->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		MeshComponent->SetRelativeLocation(FVector(0.0f, 0.0f, -96.0f));
		MeshComponent->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
		MeshComponent->SetRelativeScale3D(FVector::OneVector);

		FObjectProperty* AnimationSetProperty = FindFProperty<FObjectProperty>(PlayerBlueprint->GeneratedClass, TEXT("AnimationSet"));
		if (!AnimationSetProperty)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to find AnimationSet property on player character"));
			return false;
		}

		DefaultActor->Modify();
		AnimationSetProperty->SetObjectPropertyValue_InContainer(DefaultActor, AnimationSet);

		PlayerBlueprint->MarkPackageDirty();
		return SaveAsset(PlayerBlueprint);
	}

	UInputAction* CreateOrUpdateDashAction()
	{
		const FString DashActionObjectPath = FString::Printf(TEXT("%s/%s.%s"), *DashActionPackagePath, *DashActionName, *DashActionName);
		UInputAction* DashAction = LoadObject<UInputAction>(nullptr, *DashActionObjectPath);
		if (!DashAction)
		{
			UInputAction_Factory* Factory = NewObject<UInputAction_Factory>();
			Factory->InputActionClass = UInputAction::StaticClass();

			DashAction = Cast<UInputAction>(FAssetToolsModule::GetModule().Get().CreateAsset(
				DashActionName,
				DashActionPackagePath,
				UInputAction::StaticClass(),
				Factory
			));
		}

		if (!IsValid(DashAction))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create IA_Dash input action"));
			return nullptr;
		}

		DashAction->Modify();
		DashAction->ValueType = EInputActionValueType::Boolean;
		DashAction->bConsumeInput = true;
		DashAction->MarkPackageDirty();

		if (!SaveAsset(DashAction))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to save IA_Dash input action"));
			return nullptr;
		}

		return DashAction;
	}

	bool AssignDashActionToMovementContext(UInputAction* DashAction)
	{
		UInputMappingContext* MovementContext = LoadRequiredAsset<UInputMappingContext>(*MovementMappingContextPath);
		if (!IsValid(MovementContext) || !IsValid(DashAction))
		{
			return false;
		}

		MovementContext->Modify();

		TArray<FKey> KeysToRemove;
		for (const FEnhancedActionKeyMapping& Mapping : MovementContext->GetMappings())
		{
			if (Mapping.Action == DashAction)
			{
				KeysToRemove.Add(Mapping.Key);
			}
		}

		for (const FKey& Key : KeysToRemove)
		{
			MovementContext->UnmapKey(DashAction, Key);
		}

		MovementContext->MapKey(DashAction, EKeys::LeftAlt);
		MovementContext->MarkPackageDirty();

		return SaveAsset(MovementContext);
	}

	bool AssignDashActionToPlayerController(UInputAction* DashAction)
	{
		UBlueprint* ControllerBlueprint = LoadRequiredAsset<UBlueprint>(*PlayerControllerBlueprintPath);
		if (!IsValid(ControllerBlueprint) || !ControllerBlueprint->GeneratedClass)
		{
			return false;
		}

		UObject* DefaultObject = ControllerBlueprint->GeneratedClass->GetDefaultObject();
		if (!IsValid(DefaultObject))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to get player controller default object"));
			return false;
		}

		FObjectProperty* DashActionProperty = FindFProperty<FObjectProperty>(ControllerBlueprint->GeneratedClass, TEXT("DashAction"));
		if (!DashActionProperty)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to find DashAction property on player controller"));
			return false;
		}

		DefaultObject->Modify();
		DashActionProperty->SetObjectPropertyValue_InContainer(DefaultObject, DashAction);
		ControllerBlueprint->MarkPackageDirty();

		return SaveAsset(ControllerBlueprint);
	}
}

#endif

bool UGP_AnimationSetupLibrary::CreateFemalePlayerAnimationSetup()
{
#if WITH_EDITOR
	USkeletalMesh* FemaleMesh = GPFemaleAnimationSetup::LoadRequiredAsset<USkeletalMesh>(*GPFemaleAnimationSetup::FemaleMeshPath);
	USkeleton* FemaleSkeleton = GPFemaleAnimationSetup::LoadRequiredAsset<USkeleton>(*GPFemaleAnimationSetup::FemaleSkeletonPath);
	UAnimSequence* JumpLoop = GPFemaleAnimationSetup::LoadRequiredAsset<UAnimSequence>(*GPFemaleAnimationSetup::FemaleJumpLoopPath);
	if (!FemaleMesh || !FemaleSkeleton || !JumpLoop)
	{
		return false;
	}

	UBlendSpace1D* BlendSpace = GPFemaleAnimationSetup::CreateOrUpdateFemaleLocomotionBlendSpace(FemaleSkeleton, FemaleMesh);
	if (!BlendSpace)
	{
		return false;
	}

	UAnimMontage* PrimaryMontage = GPFemaleAnimationSetup::CreateOrUpdatePrimaryMontage(FemaleSkeleton);
	if (!PrimaryMontage)
	{
		return false;
	}

	UAnimMontage* SprintEnterLeftMontage = GPFemaleAnimationSetup::CreateOrUpdateSprintTransitionMontage(
		FemaleSkeleton,
		GPFemaleAnimationSetup::FemaleSprintEnterLeftPath,
		GPFemaleAnimationSetup::SprintEnterLeftMontageName,
		GPFemaleAnimationSetup::SprintEnterMontageBlendInTime,
		GPFemaleAnimationSetup::SprintEnterMontageBlendOutTime,
		TEXT("female sprint enter left"));
	if (!SprintEnterLeftMontage)
	{
		return false;
	}

	UAnimMontage* SprintEnterRightMontage = GPFemaleAnimationSetup::CreateOrUpdateSprintTransitionMontage(
		FemaleSkeleton,
		GPFemaleAnimationSetup::FemaleSprintEnterRightPath,
		GPFemaleAnimationSetup::SprintEnterRightMontageName,
		GPFemaleAnimationSetup::SprintEnterMontageBlendInTime,
		GPFemaleAnimationSetup::SprintEnterMontageBlendOutTime,
		TEXT("female sprint enter right"));
	if (!SprintEnterRightMontage)
	{
		return false;
	}

	UAnimMontage* SprintExitLeftMontage = GPFemaleAnimationSetup::CreateOrUpdateSprintTransitionMontage(
		FemaleSkeleton,
		GPFemaleAnimationSetup::FemaleSprintExitLeftPath,
		GPFemaleAnimationSetup::SprintExitLeftMontageName,
		GPFemaleAnimationSetup::SprintExitMontageBlendInTime,
		GPFemaleAnimationSetup::SprintExitMontageBlendOutTime,
		TEXT("female sprint exit left"));
	if (!SprintExitLeftMontage)
	{
		return false;
	}

	UAnimMontage* SprintExitRightMontage = GPFemaleAnimationSetup::CreateOrUpdateSprintTransitionMontage(
		FemaleSkeleton,
		GPFemaleAnimationSetup::FemaleSprintExitRightPath,
		GPFemaleAnimationSetup::SprintExitRightMontageName,
		GPFemaleAnimationSetup::SprintExitMontageBlendInTime,
		GPFemaleAnimationSetup::SprintExitMontageBlendOutTime,
		TEXT("female sprint exit right"));
	if (!SprintExitRightMontage)
	{
		return false;
	}

	UPDA_CharacterAnimationSet* AnimationSet = GPFemaleAnimationSetup::CreateOrUpdateFemaleAnimationSet(
		FemaleMesh,
		BlendSpace,
		JumpLoop,
		PrimaryMontage,
		SprintEnterLeftMontage,
		SprintEnterRightMontage,
		SprintExitLeftMontage,
		SprintExitRightMontage);
	if (!AnimationSet)
	{
		return false;
	}

	UAnimBlueprint* AnimBlueprint = GPFemaleAnimationSetup::CreateOrUpdateFemaleAnimBlueprint(FemaleSkeleton, FemaleMesh, BlendSpace, JumpLoop);
	if (!AnimBlueprint)
	{
		return false;
	}

	if (!GPFemaleAnimationSetup::AssignPlayerBlueprint(FemaleMesh, AnimBlueprint, AnimationSet))
	{
		return false;
	}

	if (!CreateDashInputSetup())
	{
		return false;
	}

	UE_LOG(LogTemp, Display, TEXT("Female player animation setup completed successfully."));
	return true;
#else
	UE_LOG(LogTemp, Warning, TEXT("CreateFemalePlayerAnimationSetup is only available in editor builds."));
	return false;
#endif
}

bool UGP_AnimationSetupLibrary::CreateDashInputSetup()
{
#if WITH_EDITOR
	UInputAction* DashAction = GPFemaleAnimationSetup::CreateOrUpdateDashAction();
	if (!DashAction)
	{
		return false;
	}

	if (!GPFemaleAnimationSetup::AssignDashActionToMovementContext(DashAction))
	{
		return false;
	}

	if (!GPFemaleAnimationSetup::AssignDashActionToPlayerController(DashAction))
	{
		return false;
	}

	UE_LOG(LogTemp, Display, TEXT("Dash input setup completed successfully."));
	return true;
#else
	UE_LOG(LogTemp, Warning, TEXT("CreateDashInputSetup is only available in editor builds."));
	return false;
#endif
}
