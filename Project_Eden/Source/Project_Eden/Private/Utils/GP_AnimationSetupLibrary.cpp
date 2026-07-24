#include "Utils/GP_AnimationSetupLibrary.h"

#if WITH_EDITOR

#include "Animation/AnimBlueprint.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimNodeBase.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/BlendSpace.h"
#include "Animation/BlendSpace1D.h"
#include "Animation/GP_BossAnimInstance.h"
#include "Animation/GP_FemaleAnimInstance.h"
#include "Animation/PDA_CharacterAnimationSet.h"
#include "Animation/PDA_EnemyAnimationSet.h"
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
#include "Characters/GP_CrystalSeraphBossCharacter.h"
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
	const FString FemaleJumpLoopPath = TEXT("/Game/Asset/CharacterAction/female/Animations/femaleJump_Loop");
	// 기본 공격도 루트모션 몽타주를 사용한다.
	const FString FemaleSwordAttackPath = TEXT("/Game/Asset/CharacterAction/female/Animations/femaleSword_Attack_RM");
	const TArray<FString> FemaleLightAttackPaths =
	{
		TEXT("/Game/Asset/CharacterAction/female/Animations/femaleSword_Light_A"),
		TEXT("/Game/Asset/CharacterAction/female/Animations/femaleSword_Light_B"),
		TEXT("/Game/Asset/CharacterAction/female/Animations/femaleSword_Light_C"),
		TEXT("/Game/Asset/CharacterAction/female/Animations/femaleSword_Light_D")
	};
	const TArray<FString> FemaleHeavyAttackPaths =
	{
		TEXT("/Game/Asset/CharacterAction/female/Animations/femaleSword_Heavy_A"),
		TEXT("/Game/Asset/CharacterAction/female/Animations/femaleSword_Heavy_B"),
		TEXT("/Game/Asset/CharacterAction/female/Animations/femaleSword_Heavy_C"),
		TEXT("/Game/Asset/CharacterAction/female/Animations/femaleSword_Heavy_D")
	};
	// 구르기는 루트모션 몽타주를 사용해서 애니메이션 전진량으로 이동한다.
	const FString FemaleDashMontagePath = TEXT("/Game/Asset/CharacterAction/female/Montages/AM_Female_Roll_RM");

	const FString BlendSpacePackagePath = TEXT("/Game/Asset/CharacterAction/female/BlendSpaces");
	const FString BlendSpaceName = TEXT("BS_Female_Locomotion");
	const FString AnimBlueprintPackagePath = TEXT("/Game/Asset/CharacterAction/female/AnimBlueprints");
	const FString AnimBlueprintName = TEXT("ABP_Female_Player");
	const FString AnimationSetPackagePath = TEXT("/Game/Asset/CharacterAction/female/DataAssets");
	const FString AnimationSetName = TEXT("PDA_FemaleAnimationSet");
	const FString MontagePackagePath = TEXT("/Game/Asset/CharacterAction/female/Montages");
	const FString PrimaryMontageName = TEXT("AM_Female_Primary_RM");
	const TArray<FString> LightAttackMontageNames =
	{
		TEXT("AM_Female_Light_A"),
		TEXT("AM_Female_Light_B"),
		TEXT("AM_Female_Light_C"),
		TEXT("AM_Female_Light_D")
	};
	const TArray<FString> HeavyAttackMontageNames =
	{
		TEXT("AM_Female_Heavy_A"),
		TEXT("AM_Female_Heavy_B"),
		TEXT("AM_Female_Heavy_C"),
		TEXT("AM_Female_Heavy_D")
	};
	const FName LocomotionSyncGroupName(TEXT("Locomotion"));
	const float LocomotionInputSmoothingTime = 0.12f;
	const float LocomotionSampleWeightSpeed = 8.0f;
	const float AttackMontageBlendInTime = 0.08f;
	const float AttackMontageBlendOutTime = 0.12f;
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

	TArray<UAnimMontage*> CreateOrUpdateAttackComboMontages(
		USkeleton* Skeleton,
		const TArray<FString>& SourceAnimationPaths,
		const TArray<FString>& MontageNames,
		const TCHAR* LogPrefix)
	{
		TArray<UAnimMontage*> Montages;
		if (!IsValid(Skeleton) || SourceAnimationPaths.Num() != MontageNames.Num())
		{
			return Montages;
		}

		for (int32 Index = 0; Index < SourceAnimationPaths.Num(); ++Index)
		{
			const FString LogName = FString::Printf(TEXT("%s combo %d"), LogPrefix, Index + 1);
			UAnimMontage* Montage = CreateOrUpdateMontageFromSequence(
				Skeleton,
				SourceAnimationPaths[Index],
				MontageNames[Index],
				*LogName);
			if (!IsValid(Montage))
			{
				Montages.Reset();
				return Montages;
			}

			Montage->Modify();
			// Combo montages keep short blends so queued A-B-C-D attacks connect without feeling like one long clip.
			Montage->BlendIn.SetBlendOption(EAlphaBlendOption::Cubic);
			Montage->BlendIn.SetBlendTime(AttackMontageBlendInTime);
			Montage->BlendOut.SetBlendOption(EAlphaBlendOption::Cubic);
			Montage->BlendOut.SetBlendTime(AttackMontageBlendOutTime);
			Montage->BlendOutTriggerTime = -1.0f;
			Montage->MarkPackageDirty();

			if (!SaveAsset(Montage))
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to save %s attack combo montage"), *LogName);
				Montages.Reset();
				return Montages;
			}

			Montages.Add(Montage);
		}

		return Montages;
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
		UAnimMontage* PrimaryMontage,
		const TArray<UAnimMontage*>& LightAttackMontages,
		const TArray<UAnimMontage*>& HeavyAttackMontages)
	{
		if (!IsValid(SkeletalMesh) || !IsValid(PrimaryMontage) || LightAttackMontages.Num() == 0 || HeavyAttackMontages.Num() == 0)
		{
			return nullptr;
		}

		for (UAnimMontage* AttackMontage : LightAttackMontages)
		{
			if (!IsValid(AttackMontage))
			{
				return nullptr;
			}
		}

		for (UAnimMontage* AttackMontage : HeavyAttackMontages)
		{
			if (!IsValid(AttackMontage))
			{
				return nullptr;
			}
		}

		UAnimMontage* DashMontage = LoadRequiredAsset<UAnimMontage>(*FemaleDashMontagePath);
		if (!IsValid(DashMontage))
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
		AnimationSet->RollMontages.Roll_RM = DashMontage;
		AnimationSet->PrimaryAttackMontage = PrimaryMontage;
		AnimationSet->LightAttackMontages.Reset();
		for (UAnimMontage* AttackMontage : LightAttackMontages)
		{
			AnimationSet->LightAttackMontages.Add(AttackMontage);
		}
		AnimationSet->HeavyAttackMontages.Reset();
		for (UAnimMontage* AttackMontage : HeavyAttackMontages)
		{
			AnimationSet->HeavyAttackMontages.Add(AttackMontage);
		}
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

namespace GPSansBossAnimationSetup
{
	const FString SansMeshPath = TEXT("/Game/Asset/BossAction/Sans/Sans");
	const FString SansSkeletonPath = TEXT("/Game/Asset/BossAction/Sans/Sans_Skeleton");
	const FString SansIdlePath = TEXT("/Game/Asset/BossAction/Sans/Animations/Sans_Idle_Rail_Loop");
	const FString SansWalkPath = TEXT("/Game/Asset/BossAction/Sans/Animations/Sans_Zombie_Walk_Fwd_Loop");
	const FString SansRunPath = TEXT("/Game/Asset/BossAction/Sans/Animations/Sans_Zombie_Run_Fwd_Loop");
	const FString SansJumpLoopPath = TEXT("/Game/Asset/BossAction/Sans/Animations/Sans_Jump_Loop");
	const FString SansBasicAttackPath = TEXT("/Game/Asset/BossAction/Sans/Animations/Sans_Zombie_Scratch");
	const FString SansSweepAttackPath = TEXT("/Game/Asset/BossAction/Sans/Animations/Sans_Sword_Heavy_A");
	const FString SansHeavyAttackPath = TEXT("/Game/Asset/BossAction/Sans/Animations/Sans_Sword_Heavy_B");
	const FString SansAreaAttackPath = TEXT("/Game/Asset/BossAction/Sans/Animations/Sans_Sword_GroundPound_RM");
	const FString BlendSpacePackagePath = TEXT("/Game/Asset/BossAction/Sans/BlendSpaces");
	const FString BlendSpaceName = TEXT("BS_Sans_Boss_Locomotion");
	const FString AnimBlueprintPackagePath = TEXT("/Game/Asset/BossAction/Sans/AnimBlueprints");
	const FString AnimBlueprintName = TEXT("ABP_Sans_Boss");
	const FString AnimationSetPackagePath = TEXT("/Game/Asset/BossAction/Sans/DataAssets");
	const FString AnimationSetName = TEXT("PDA_SansBossAnimationSet");
	const FString MontagePackagePath = TEXT("/Game/Asset/BossAction/Sans/Montages");
	const FString BasicAttackMontageName = TEXT("AM_Sans_Zombie_Scratch");
	const FString SweepAttackMontageName = TEXT("AM_Sans_BossSweep");
	const FString HeavyAttackMontageName = TEXT("AM_Sans_BossHeavy");
	const FString AreaAttackMontageName = TEXT("AM_Sans_BossArea");
	const FString BossBlueprintPath = TEXT("/Game/Characters/EnemyCharacter/Boss/BP_Boss_Sans/BP_Boss_Sans");
	const FName LocomotionSyncGroupName(TEXT("BossLocomotion"));
	const float AttackMontageBlendInTime = 0.08f;
	const float AttackMontageBlendOutTime = 0.12f;

	UBlendSpace1D* CreateOrUpdateLocomotionBlendSpace(USkeleton* Skeleton, USkeletalMesh* SkeletalMesh)
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
				Factory));
		}

		if (!IsValid(BlendSpace))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create Sans boss locomotion blend space"));
			return nullptr;
		}

		BlendSpace->Modify();
		if (FStructProperty* BlendParametersProperty = CastField<FStructProperty>(GPFemaleAnimationSetup::FindPropertyChecked(UBlendSpace::StaticClass(), TEXT("BlendParameters"))))
		{
			FBlendParameter* BlendParameters = BlendParametersProperty->ContainerPtrToValuePtr<FBlendParameter>(BlendSpace);
			BlendParameters[0].DisplayName = TEXT("Speed");
			BlendParameters[0].Min = 0.0f;
			BlendParameters[0].Max = 450.0f;
			BlendParameters[0].GridNum = 4;
			BlendParameters[0].bSnapToGrid = false;
			BlendParameters[0].bWrapInput = false;
		}

		for (int32 SampleIndex = BlendSpace->GetNumberOfBlendSamples() - 1; SampleIndex >= 0; --SampleIndex)
		{
			BlendSpace->DeleteSample(SampleIndex);
		}

		UAnimSequence* Idle = GPFemaleAnimationSetup::LoadRequiredAsset<UAnimSequence>(*SansIdlePath);
		UAnimSequence* Walk = GPFemaleAnimationSetup::LoadRequiredAsset<UAnimSequence>(*SansWalkPath);
		UAnimSequence* Run = GPFemaleAnimationSetup::LoadRequiredAsset<UAnimSequence>(*SansRunPath);
		if (!Idle || !Walk || !Run)
		{
			return nullptr;
		}

		// Sans boss locomotion mirrors the player data layout: idle, walk, and run are driven by one Speed axis.
		BlendSpace->AddSample(Idle, FVector(0.0f, 0.0f, 0.0f));
		BlendSpace->AddSample(Walk, FVector(180.0f, 0.0f, 0.0f));
		BlendSpace->AddSample(Run, FVector(450.0f, 0.0f, 0.0f));
		BlendSpace->ValidateSampleData();
		BlendSpace->ResampleData();
		BlendSpace->MarkPackageDirty();

		return GPFemaleAnimationSetup::SaveAsset(BlendSpace) ? BlendSpace : nullptr;
	}

	UAnimMontage* CreateOrUpdateMontageFromSequence(USkeleton* Skeleton, const FString& SourceAnimationPath, const FString& MontageName, const TCHAR* LogName)
	{
		if (!IsValid(Skeleton))
		{
			return nullptr;
		}

		const FString MontageObjectPath = FString::Printf(TEXT("%s/%s.%s"), *MontagePackagePath, *MontageName, *MontageName);
		UAnimMontage* Montage = LoadObject<UAnimMontage>(nullptr, *MontageObjectPath);
		if (!Montage)
		{
			UAnimSequence* SourceAnimation = GPFemaleAnimationSetup::LoadRequiredAsset<UAnimSequence>(SourceAnimationPath);
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
				Factory));
		}

		if (!IsValid(Montage))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create %s montage"), LogName);
			return nullptr;
		}

		Montage->Modify();
		// Boss attack montages use the same short blends as the player attack structure.
		Montage->BlendIn.SetBlendOption(EAlphaBlendOption::Cubic);
		Montage->BlendIn.SetBlendTime(AttackMontageBlendInTime);
		Montage->BlendOut.SetBlendOption(EAlphaBlendOption::Cubic);
		Montage->BlendOut.SetBlendTime(AttackMontageBlendOutTime);
		Montage->BlendOutTriggerTime = -1.0f;
		Montage->MarkPackageDirty();

		return GPFemaleAnimationSetup::SaveAsset(Montage) ? Montage : nullptr;
	}

	UAnimBlueprint* CreateOrUpdateAnimBlueprint(
		USkeleton* Skeleton,
		USkeletalMesh* SkeletalMesh,
		UAnimSequence* DefaultIdle)
	{
		if (!IsValid(Skeleton) || !IsValid(SkeletalMesh) || !IsValid(DefaultIdle))
		{
			return nullptr;
		}

		const FString BlueprintObjectPath = FString::Printf(TEXT("%s/%s.%s"), *AnimBlueprintPackagePath, *AnimBlueprintName, *AnimBlueprintName);
		UAnimBlueprint* AnimBlueprint = LoadObject<UAnimBlueprint>(nullptr, *BlueprintObjectPath);
		if (!AnimBlueprint)
		{
			UAnimBlueprintFactory* Factory = NewObject<UAnimBlueprintFactory>();
			Factory->ParentClass = UGP_BossAnimInstance::StaticClass();
			Factory->TargetSkeleton = Skeleton;
			Factory->PreviewSkeletalMesh = SkeletalMesh;

			AnimBlueprint = Cast<UAnimBlueprint>(FAssetToolsModule::GetModule().Get().CreateAsset(
				AnimBlueprintName,
				AnimBlueprintPackagePath,
				UAnimBlueprint::StaticClass(),
				Factory));
		}

		if (!IsValid(AnimBlueprint))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create Sans boss anim blueprint"));
			return nullptr;
		}

		AnimBlueprint->ParentClass = UGP_BossAnimInstance::StaticClass();
		AnimBlueprint->TargetSkeleton = Skeleton;

		UEdGraph* AnimationGraph = nullptr;
		for (UEdGraph* Graph : AnimBlueprint->FunctionGraphs)
		{
			if (Graph && Graph->GetSchema()->IsA(UAnimationGraphSchema::StaticClass()))
			{
				AnimationGraph = Graph;
				break;
			}
		}

		if (!AnimationGraph)
		{
			UE_LOG(LogTemp, Error, TEXT("Sans boss anim blueprint has no animation graph"));
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
			UE_LOG(LogTemp, Error, TEXT("Sans boss anim blueprint root node was not found"));
			return nullptr;
		}

		// Sans floats during normal gameplay, so a single authored rail idle is
		// the stable base pose. Keep the asset directly on the sequence player:
		// a nullable runtime asset pin would override it with Ref Pose (T-pose).
		FGraphNodeCreator<UAnimGraphNode_SequencePlayer> IdleNodeCreator(*AnimationGraph);
		UAnimGraphNode_SequencePlayer* IdleNode = IdleNodeCreator.CreateNode();
		IdleNode->NodePosX = -520;
		IdleNode->NodePosY = 0;
		IdleNode->SetAnimationAsset(DefaultIdle);
		IdleNodeCreator.Finalize();

		FGraphNodeCreator<UAnimGraphNode_Slot> SlotNodeCreator(*AnimationGraph);
		UAnimGraphNode_Slot* SlotNode = SlotNodeCreator.CreateNode();
		SlotNode->Node.SlotName = FName(TEXT("DefaultSlot"));
		SlotNode->NodePosX = -120;
		SlotNode->NodePosY = 0;
		SlotNodeCreator.Finalize();

		if (!GPFemaleAnimationSetup::ConnectPins(
				GPFemaleAnimationSetup::FindPoseOutputPin(IdleNode),
				GPFemaleAnimationSetup::FindPinChecked(SlotNode, TEXT("Source"), EGPD_Input))
			|| !GPFemaleAnimationSetup::ConnectPins(GPFemaleAnimationSetup::FindPoseOutputPin(SlotNode), GPFemaleAnimationSetup::FindPinChecked(RootNode, TEXT("Result"), EGPD_Input)))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to wire Sans boss anim blueprint graph"));
			return nullptr;
		}

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(AnimBlueprint);
		FKismetEditorUtilities::CompileBlueprint(AnimBlueprint);
		return GPFemaleAnimationSetup::SaveAsset(AnimBlueprint) ? AnimBlueprint : nullptr;
	}

	UPDA_CharacterAnimationSet* CreateOrUpdateAnimationSet(
		USkeletalMesh* SkeletalMesh,
		UAnimBlueprint* AnimBlueprint,
		UAnimMontage* BasicAttackMontage,
		UAnimMontage* SweepAttackMontage,
		UAnimMontage* HeavyAttackMontage,
		UAnimMontage* AreaAttackMontage)
	{
		if (!IsValid(SkeletalMesh) || !IsValid(AnimBlueprint) || !IsValid(BasicAttackMontage) || !IsValid(SweepAttackMontage) || !IsValid(HeavyAttackMontage) || !IsValid(AreaAttackMontage))
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
				Factory));
		}

		if (!IsValid(AnimationSet))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create Sans boss animation set"));
			return nullptr;
		}

		AnimationSet->Modify();
		AnimationSet->CharacterMesh = SkeletalMesh;
		AnimationSet->AnimBlueprintClass = AnimBlueprint->GeneratedClass;
		AnimationSet->PrimaryAttackMontage = BasicAttackMontage;
		AnimationSet->LightAttackMontages.Reset();
		AnimationSet->LightAttackMontages.Add(BasicAttackMontage);
		AnimationSet->HeavyAttackMontages.Reset();
		AnimationSet->HeavyAttackMontages.Add(SweepAttackMontage);
		AnimationSet->BossSweepAttackMontage = SweepAttackMontage;
		AnimationSet->BossHeavyAttackMontage = HeavyAttackMontage;
		AnimationSet->BossAreaAttackMontage = AreaAttackMontage;
		AnimationSet->MarkPackageDirty();

		return GPFemaleAnimationSetup::SaveAsset(AnimationSet) ? AnimationSet : nullptr;
	}

	bool AssignBossBlueprint(USkeletalMesh* SkeletalMesh, UAnimBlueprint* AnimBlueprint, UPDA_CharacterAnimationSet* AnimationSet)
	{
		UBlueprint* BossBlueprint = GPFemaleAnimationSetup::LoadRequiredAsset<UBlueprint>(*BossBlueprintPath);
		if (!IsValid(BossBlueprint) || !BossBlueprint->GeneratedClass || !IsValid(SkeletalMesh) || !IsValid(AnimBlueprint) || !IsValid(AnimationSet))
		{
			return false;
		}

		AActor* DefaultActor = Cast<AActor>(BossBlueprint->GeneratedClass->GetDefaultObject());
		USkeletalMeshComponent* MeshComponent = IsValid(DefaultActor) ? DefaultActor->FindComponentByClass<USkeletalMeshComponent>() : nullptr;
		if (!IsValid(DefaultActor) || !IsValid(MeshComponent))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to find Sans boss blueprint mesh component"));
			return false;
		}

		MeshComponent->Modify();
		MeshComponent->SetSkeletalMesh(SkeletalMesh);
		MeshComponent->SetAnimInstanceClass(AnimBlueprint->GeneratedClass);
		MeshComponent->SetAnimationMode(EAnimationMode::AnimationBlueprint);

		FObjectProperty* AnimationSetProperty = FindFProperty<FObjectProperty>(BossBlueprint->GeneratedClass, TEXT("AnimationSet"));
		if (!AnimationSetProperty)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to find AnimationSet property on Sans boss blueprint"));
			return false;
		}

		DefaultActor->Modify();
		// The boss Blueprint points at the same data asset that abilities use for basic and sweep montages.
		AnimationSetProperty->SetObjectPropertyValue_InContainer(DefaultActor, AnimationSet);
		BossBlueprint->MarkPackageDirty();
		return GPFemaleAnimationSetup::SaveAsset(BossBlueprint);
	}
}

namespace GPCrystalSeraphAnimationSetup
{
	const FString AssetRootPath = TEXT("/Game/Characters/EnemyCharacter/Boss/BP_Boss_CrystalSeraph");
	const FString AnimationPackagePath = TEXT("/Game/Characters/EnemyCharacter/Boss/BP_Boss_CrystalSeraph/Animation");
	const FString CrystalSeraphMeshPath = TEXT("/Game/Characters/EnemyCharacter/Boss/BP_Boss_CrystalSeraph/CrystalSeraph");
	const FString CrystalSeraphSkeletonPath = TEXT("/Game/Characters/EnemyCharacter/Boss/BP_Boss_CrystalSeraph/CrystalSeraph_Skeleton");
	const FString HoverIdlePath = TEXT("/Game/Characters/EnemyCharacter/Boss/BP_Boss_CrystalSeraph/Animation/TravelMode_Hover_Idle");
	const FString BasicAttackEnterSequencePath = TEXT("/Game/Characters/EnemyCharacter/Boss/BP_Boss_CrystalSeraph/Animation/UEFN_Spell_Simple_Enter");
	const FString BasicAttackShootSequencePath = TEXT("/Game/Characters/EnemyCharacter/Boss/BP_Boss_CrystalSeraph/Animation/UEFN_Spell_Simple_Shoot");
	const FString BasicAttackHoldSequencePath = TEXT("/Game/Characters/EnemyCharacter/Boss/BP_Boss_CrystalSeraph/Animation/UEFN_Spell_Simple_Idle_Loop");
	const FString BasicAttackExitSequencePath = TEXT("/Game/Characters/EnemyCharacter/Boss/BP_Boss_CrystalSeraph/Animation/UEFN_Spell_Simple_Exit");
	const FString LaserAttackEnterSequencePath = TEXT("/Game/Characters/EnemyCharacter/Boss/BP_Boss_CrystalSeraph/Animation/UEFN_Spell_Double_Enter");
	const FString LaserAttackShootSequencePath = TEXT("/Game/Characters/EnemyCharacter/Boss/BP_Boss_CrystalSeraph/Animation/UEFN_Spell_Double_Shoot_Loop");
	const FString LaserAttackHoldSequencePath = TEXT("/Game/Characters/EnemyCharacter/Boss/BP_Boss_CrystalSeraph/Animation/UEFN_Spell_Double_Idle_Loop");
	const FString LaserAttackExitSequencePath = TEXT("/Game/Characters/EnemyCharacter/Boss/BP_Boss_CrystalSeraph/Animation/UEFN_Spell_Double_Exit");
	const FString DeathSequencePath = TEXT("/Game/Characters/EnemyCharacter/Boss/BP_Boss_CrystalSeraph/Animation/Death_A");
	const FString BossBlueprintPath = TEXT("/Game/Characters/EnemyCharacter/Boss/BP_Boss_CrystalSeraph/BP_Crystal_Seraph");
	const FString AnimBlueprintName = TEXT("ABP_CrystalSeraph");
	const FString AnimationSetName = TEXT("PDA_CrystalSeraphAnimationSet");
	const FString BasicAttackMontageName = TEXT("AM_CrystalSeraph_Basic_Simple");
	const FString LaserAttackMontageName = TEXT("AM_CrystalSeraph_Laser_Double");
	const FString DeathMontageName = TEXT("AM_CrystalSeraph_Death");
	constexpr float BasicAttackHoldTime = 0.45f;
	constexpr float LaserAttackHoldTime = 0.65f;

	struct FPatternMontageClip
	{
		FString SourceAnimationPath;
		float MaxDuration = 0.0f;
	};

	float AppendPatternMontageSegment(FAnimTrack& AnimTrack, UAnimSequenceBase* SourceAnimation, float StartTime, float MaxDuration)
	{
		if (!IsValid(SourceAnimation))
		{
			return StartTime;
		}

		const float SourceLength = SourceAnimation->GetPlayLength();
		const float SegmentLength = MaxDuration > 0.0f ? FMath::Min(SourceLength, MaxDuration) : SourceLength;
		if (SegmentLength <= UE_KINDA_SMALL_NUMBER)
		{
			return StartTime;
		}

		FAnimSegment Segment;
		Segment.SetAnimReference(SourceAnimation, true);
		Segment.StartPos = StartTime;
		Segment.AnimStartTime = 0.0f;
		Segment.AnimEndTime = SegmentLength;
		Segment.AnimPlayRate = 1.0f;
		Segment.LoopingCount = 1;
#if WITH_EDITOR
		Segment.UpdateCachedPlayLength();
#endif
		AnimTrack.AnimSegments.Add(Segment);
		return StartTime + Segment.GetLength();
	}

	UAnimMontage* CreateOrUpdateMontageFromSequence(USkeleton* Skeleton, const FString& SourceAnimationPath, const FString& MontageName, const TCHAR* LogName)
	{
		if (!IsValid(Skeleton))
		{
			return nullptr;
		}

		const FString MontageObjectPath = FString::Printf(TEXT("%s/%s.%s"), *AnimationPackagePath, *MontageName, *MontageName);
		UAnimMontage* Montage = LoadObject<UAnimMontage>(nullptr, *MontageObjectPath);
		if (!Montage)
		{
			UAnimSequence* SourceAnimation = GPFemaleAnimationSetup::LoadRequiredAsset<UAnimSequence>(SourceAnimationPath);
			if (!SourceAnimation)
			{
				return nullptr;
			}

			UAnimMontageFactory* Factory = NewObject<UAnimMontageFactory>();
			Factory->TargetSkeleton = Skeleton;
			Factory->SourceAnimation = SourceAnimation;

			Montage = Cast<UAnimMontage>(FAssetToolsModule::GetModule().Get().CreateAsset(
				MontageName,
				AnimationPackagePath,
				UAnimMontage::StaticClass(),
				Factory));
		}

		if (!IsValid(Montage))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create %s montage"), LogName);
			return nullptr;
		}

		Montage->Modify();
		// Crystal Seraph pattern montages layer through DefaultSlot over the hover idle base pose.
		Montage->BlendIn.SetBlendOption(EAlphaBlendOption::Cubic);
		Montage->BlendIn.SetBlendTime(GPFemaleAnimationSetup::AttackMontageBlendInTime);
		Montage->BlendOut.SetBlendOption(EAlphaBlendOption::Cubic);
		Montage->BlendOut.SetBlendTime(GPFemaleAnimationSetup::AttackMontageBlendOutTime);
		Montage->BlendOutTriggerTime = -1.0f;
		Montage->MarkPackageDirty();

		return GPFemaleAnimationSetup::SaveAsset(Montage) ? Montage : nullptr;
	}

	UAnimMontage* CreateOrUpdatePatternMontage(USkeleton* Skeleton, const TArray<FPatternMontageClip>& Clips, const FString& MontageName, const TCHAR* LogName)
	{
		if (!IsValid(Skeleton) || Clips.IsEmpty())
		{
			return nullptr;
		}

		const FString MontageObjectPath = FString::Printf(TEXT("%s/%s.%s"), *AnimationPackagePath, *MontageName, *MontageName);
		UAnimMontage* Montage = LoadObject<UAnimMontage>(nullptr, *MontageObjectPath);
		if (!Montage)
		{
			UAnimSequence* FirstSourceAnimation = GPFemaleAnimationSetup::LoadRequiredAsset<UAnimSequence>(Clips[0].SourceAnimationPath);
			if (!FirstSourceAnimation)
			{
				return nullptr;
			}

			UAnimMontageFactory* Factory = NewObject<UAnimMontageFactory>();
			Factory->TargetSkeleton = Skeleton;
			Factory->SourceAnimation = FirstSourceAnimation;

			Montage = Cast<UAnimMontage>(FAssetToolsModule::GetModule().Get().CreateAsset(
				MontageName,
				AnimationPackagePath,
				UAnimMontage::StaticClass(),
				Factory));
		}

		if (!IsValid(Montage))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create %s montage"), LogName);
			return nullptr;
		}

		Montage->Modify();
		Montage->SlotAnimTracks.Reset();
		FSlotAnimationTrack& SlotTrack = Montage->AddSlot(FName(TEXT("DefaultSlot")));
		SlotTrack.AnimTrack.AnimSegments.Reset();

		float CurrentTime = 0.0f;
		for (const FPatternMontageClip& Clip : Clips)
		{
			UAnimSequenceBase* ClipAnimation = GPFemaleAnimationSetup::LoadRequiredAsset<UAnimSequenceBase>(Clip.SourceAnimationPath);
			CurrentTime = AppendPatternMontageSegment(SlotTrack.AnimTrack, ClipAnimation, CurrentTime, Clip.MaxDuration);
		}

		if (SlotTrack.AnimTrack.AnimSegments.IsEmpty())
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to populate %s montage segments"), LogName);
			return nullptr;
		}

		// Pattern montages intentionally keep a short post-fire hold so the spell pose remains visible before returning to hover idle.
		Montage->CompositeSections.Reset();
#if WITH_EDITOR
		Montage->AddAnimCompositeSection(FName(TEXT("Default")), 0.0f);
#endif
		Montage->SetCompositeLength(CurrentTime);
		Montage->BlendIn.SetBlendOption(EAlphaBlendOption::Cubic);
		Montage->BlendIn.SetBlendTime(GPFemaleAnimationSetup::AttackMontageBlendInTime);
		Montage->BlendOut.SetBlendOption(EAlphaBlendOption::Cubic);
		Montage->BlendOut.SetBlendTime(GPFemaleAnimationSetup::AttackMontageBlendOutTime);
		Montage->BlendOutTriggerTime = -1.0f;
#if WITH_EDITOR
		Montage->UpdateLinkableElements();
#endif
		Montage->RefreshCacheData();
		Montage->MarkPackageDirty();

		return GPFemaleAnimationSetup::SaveAsset(Montage) ? Montage : nullptr;
	}

	UAnimBlueprint* CreateOrUpdateAnimBlueprint(USkeleton* Skeleton, USkeletalMesh* SkeletalMesh, UAnimSequence* HoverIdle)
	{
		if (!IsValid(Skeleton) || !IsValid(SkeletalMesh) || !IsValid(HoverIdle))
		{
			return nullptr;
		}

		const FString BlueprintObjectPath = FString::Printf(TEXT("%s/%s.%s"), *AnimationPackagePath, *AnimBlueprintName, *AnimBlueprintName);
		UAnimBlueprint* AnimBlueprint = LoadObject<UAnimBlueprint>(nullptr, *BlueprintObjectPath);
		if (!AnimBlueprint)
		{
			UAnimBlueprintFactory* Factory = NewObject<UAnimBlueprintFactory>();
			Factory->ParentClass = UGP_BossAnimInstance::StaticClass();
			Factory->TargetSkeleton = Skeleton;
			Factory->PreviewSkeletalMesh = SkeletalMesh;

			AnimBlueprint = Cast<UAnimBlueprint>(FAssetToolsModule::GetModule().Get().CreateAsset(
				AnimBlueprintName,
				AnimationPackagePath,
				UAnimBlueprint::StaticClass(),
				Factory));
		}

		if (!IsValid(AnimBlueprint))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create Crystal Seraph anim blueprint"));
			return nullptr;
		}

		AnimBlueprint->ParentClass = UGP_BossAnimInstance::StaticClass();
		AnimBlueprint->TargetSkeleton = Skeleton;

		UEdGraph* AnimationGraph = nullptr;
		for (UEdGraph* Graph : AnimBlueprint->FunctionGraphs)
		{
			if (Graph && Graph->GetSchema() && Graph->GetSchema()->IsA(UAnimationGraphSchema::StaticClass()))
			{
				AnimationGraph = Graph;
				break;
			}
		}

		if (!AnimationGraph)
		{
			TArray<UEdGraph*> AllGraphs;
			AnimBlueprint->GetAllGraphs(AllGraphs);
			for (UEdGraph* Graph : AllGraphs)
			{
				if (Graph && Graph->GetSchema() && Graph->GetSchema()->IsA(UAnimationGraphSchema::StaticClass()))
				{
					AnimationGraph = Graph;
					break;
				}
			}
		}

		if (!AnimationGraph)
		{
			UE_LOG(LogTemp, Error, TEXT("Crystal Seraph anim blueprint has no animation graph"));
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
			UE_LOG(LogTemp, Error, TEXT("Crystal Seraph anim blueprint root node was not found"));
			return nullptr;
		}

		FGraphNodeCreator<UAnimGraphNode_SequencePlayer> HoverIdleNodeCreator(*AnimationGraph);
		UAnimGraphNode_SequencePlayer* HoverIdleNode = HoverIdleNodeCreator.CreateNode();
		HoverIdleNode->NodePosX = -520;
		HoverIdleNode->NodePosY = 0;
		HoverIdleNode->SetAnimationAsset(HoverIdle);
		HoverIdleNodeCreator.Finalize();

		FGraphNodeCreator<UAnimGraphNode_Slot> SlotNodeCreator(*AnimationGraph);
		UAnimGraphNode_Slot* SlotNode = SlotNodeCreator.CreateNode();
		SlotNode->Node.SlotName = FName(TEXT("DefaultSlot"));
		SlotNode->NodePosX = -120;
		SlotNode->NodePosY = 0;
		SlotNodeCreator.Finalize();

		if (!GPFemaleAnimationSetup::ConnectPins(GPFemaleAnimationSetup::FindPoseOutputPin(HoverIdleNode), GPFemaleAnimationSetup::FindPinChecked(SlotNode, TEXT("Source"), EGPD_Input))
			|| !GPFemaleAnimationSetup::ConnectPins(GPFemaleAnimationSetup::FindPoseOutputPin(SlotNode), GPFemaleAnimationSetup::FindPinChecked(RootNode, TEXT("Result"), EGPD_Input)))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to wire Crystal Seraph anim blueprint graph"));
			return nullptr;
		}

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(AnimBlueprint);
		FKismetEditorUtilities::CompileBlueprint(AnimBlueprint);
		return GPFemaleAnimationSetup::SaveAsset(AnimBlueprint) ? AnimBlueprint : nullptr;
	}

	UPDA_EnemyAnimationSet* CreateOrUpdateAnimationSet(
		USkeletalMesh* SkeletalMesh,
		UAnimSequence* HoverIdle,
		UAnimBlueprint* AnimBlueprint,
		UAnimMontage* BasicAttackMontage,
		UAnimMontage* LaserAttackMontage,
		UAnimMontage* DeathMontage)
	{
		if (!IsValid(SkeletalMesh) || !IsValid(HoverIdle) || !IsValid(AnimBlueprint) || !IsValid(BasicAttackMontage) || !IsValid(LaserAttackMontage))
		{
			return nullptr;
		}

		const FString AnimationSetObjectPath = FString::Printf(TEXT("%s/%s.%s"), *AnimationPackagePath, *AnimationSetName, *AnimationSetName);
		UPDA_EnemyAnimationSet* AnimationSet = LoadObject<UPDA_EnemyAnimationSet>(nullptr, *AnimationSetObjectPath);
		if (!AnimationSet)
		{
			UDataAssetFactory* Factory = NewObject<UDataAssetFactory>();
			Factory->DataAssetClass = UPDA_EnemyAnimationSet::StaticClass();

			AnimationSet = Cast<UPDA_EnemyAnimationSet>(FAssetToolsModule::GetModule().Get().CreateAsset(
				AnimationSetName,
				AnimationPackagePath,
				UPDA_EnemyAnimationSet::StaticClass(),
				Factory));
		}

		if (!IsValid(AnimationSet))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create Crystal Seraph enemy animation set"));
			return nullptr;
		}

		AnimationSet->Modify();
		// EnemyAnimationSet is the shared enemy path that applies mesh and ABP from AGP_EnemyCharacter::UpdateAnimationSet.
		AnimationSet->CharacterMesh = SkeletalMesh;
		AnimationSet->AnimBlueprintClass = AnimBlueprint->GeneratedClass;
		AnimationSet->IdleAnimation = HoverIdle;
		AnimationSet->PrimaryAttackMontage = BasicAttackMontage;
		AnimationSet->LightAttackMontages.Reset();
		AnimationSet->LightAttackMontages.Add(BasicAttackMontage);
		AnimationSet->LightAttackMontages.Add(LaserAttackMontage);
		AnimationSet->DeathMontages.Reset();
		if (IsValid(DeathMontage))
		{
			AnimationSet->DeathMontages.Add(DeathMontage);
		}
		AnimationSet->MarkPackageDirty();

		return GPFemaleAnimationSetup::SaveAsset(AnimationSet) ? AnimationSet : nullptr;
	}

	bool AssignBossBlueprint(
		USkeletalMesh* SkeletalMesh,
		UAnimBlueprint* AnimBlueprint,
		UPDA_EnemyAnimationSet* AnimationSet,
		UAnimMontage* BasicAttackMontage,
		UAnimMontage* LaserAttackMontage)
	{
		UBlueprint* BossBlueprint = GPFemaleAnimationSetup::LoadRequiredAsset<UBlueprint>(*BossBlueprintPath);
		if (!IsValid(BossBlueprint) || !BossBlueprint->GeneratedClass || !IsValid(SkeletalMesh) || !IsValid(AnimBlueprint) || !IsValid(AnimationSet))
		{
			return false;
		}

		AGP_CrystalSeraphBossCharacter* DefaultBoss = Cast<AGP_CrystalSeraphBossCharacter>(BossBlueprint->GeneratedClass->GetDefaultObject());
		USkeletalMeshComponent* MeshComponent = IsValid(DefaultBoss) ? DefaultBoss->FindComponentByClass<USkeletalMeshComponent>() : nullptr;
		if (!IsValid(DefaultBoss) || !IsValid(MeshComponent))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to find Crystal Seraph blueprint mesh component"));
			return false;
		}

		MeshComponent->Modify();
		MeshComponent->SetSkeletalMesh(SkeletalMesh);
		MeshComponent->SetAnimInstanceClass(AnimBlueprint->GeneratedClass);
		MeshComponent->SetAnimationMode(EAnimationMode::AnimationBlueprint);

		FObjectProperty* EnemyAnimationSetProperty = FindFProperty<FObjectProperty>(BossBlueprint->GeneratedClass, TEXT("EnemyAnimationSet"));
		FObjectProperty* BasicMontageProperty = FindFProperty<FObjectProperty>(BossBlueprint->GeneratedClass, TEXT("BasicAttackMontage"));
		FObjectProperty* LaserMontageProperty = FindFProperty<FObjectProperty>(BossBlueprint->GeneratedClass, TEXT("LaserAttackMontage"));
		if (!EnemyAnimationSetProperty || !BasicMontageProperty || !LaserMontageProperty)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to find Crystal Seraph animation properties on boss blueprint"));
			return false;
		}

		DefaultBoss->Modify();
		// Keep the BP editable: designers can later swap montages or the entire enemy animation set in Class Defaults.
		EnemyAnimationSetProperty->SetObjectPropertyValue_InContainer(DefaultBoss, AnimationSet);
		BasicMontageProperty->SetObjectPropertyValue_InContainer(DefaultBoss, BasicAttackMontage);
		LaserMontageProperty->SetObjectPropertyValue_InContainer(DefaultBoss, LaserAttackMontage);
		BossBlueprint->MarkPackageDirty();
		return GPFemaleAnimationSetup::SaveAsset(BossBlueprint);
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

	TArray<UAnimMontage*> LightAttackMontages = GPFemaleAnimationSetup::CreateOrUpdateAttackComboMontages(
		FemaleSkeleton,
		GPFemaleAnimationSetup::FemaleLightAttackPaths,
		GPFemaleAnimationSetup::LightAttackMontageNames,
		TEXT("female light attack"));
	if (LightAttackMontages.Num() != GPFemaleAnimationSetup::LightAttackMontageNames.Num())
	{
		return false;
	}

	TArray<UAnimMontage*> HeavyAttackMontages = GPFemaleAnimationSetup::CreateOrUpdateAttackComboMontages(
		FemaleSkeleton,
		GPFemaleAnimationSetup::FemaleHeavyAttackPaths,
		GPFemaleAnimationSetup::HeavyAttackMontageNames,
		TEXT("female heavy attack"));
	if (HeavyAttackMontages.Num() != GPFemaleAnimationSetup::HeavyAttackMontageNames.Num())
	{
		return false;
	}

	UPDA_CharacterAnimationSet* AnimationSet = GPFemaleAnimationSetup::CreateOrUpdateFemaleAnimationSet(
		FemaleMesh,
		PrimaryMontage,
		LightAttackMontages,
		HeavyAttackMontages);
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

bool UGP_AnimationSetupLibrary::CreateSansBossAnimationSetup()
{
#if WITH_EDITOR
	USkeletalMesh* SansMesh = GPFemaleAnimationSetup::LoadRequiredAsset<USkeletalMesh>(*GPSansBossAnimationSetup::SansMeshPath);
	USkeleton* SansSkeleton = GPFemaleAnimationSetup::LoadRequiredAsset<USkeleton>(*GPSansBossAnimationSetup::SansSkeletonPath);
	UAnimSequence* DefaultIdle = GPFemaleAnimationSetup::LoadRequiredAsset<UAnimSequence>(*GPSansBossAnimationSetup::SansIdlePath);
	if (!SansMesh || !SansSkeleton || !DefaultIdle)
	{
		return false;
	}

	UAnimMontage* BasicAttackMontage = GPSansBossAnimationSetup::CreateOrUpdateMontageFromSequence(
		SansSkeleton,
		GPSansBossAnimationSetup::SansBasicAttackPath,
		GPSansBossAnimationSetup::BasicAttackMontageName,
		TEXT("Sans zombie scratch"));
	if (!BasicAttackMontage)
	{
		return false;
	}

	UAnimMontage* SweepAttackMontage = GPSansBossAnimationSetup::CreateOrUpdateMontageFromSequence(
		SansSkeleton,
		GPSansBossAnimationSetup::SansSweepAttackPath,
		GPSansBossAnimationSetup::SweepAttackMontageName,
		TEXT("Sans sword heavy A sweep"));
	if (!SweepAttackMontage)
	{
		return false;
	}

	UAnimMontage* HeavyAttackMontage = GPSansBossAnimationSetup::CreateOrUpdateMontageFromSequence(
		SansSkeleton,
		GPSansBossAnimationSetup::SansHeavyAttackPath,
		GPSansBossAnimationSetup::HeavyAttackMontageName,
		TEXT("Sans sword heavy B"));
	if (!HeavyAttackMontage)
	{
		return false;
	}

	UAnimMontage* AreaAttackMontage = GPSansBossAnimationSetup::CreateOrUpdateMontageFromSequence(
		SansSkeleton,
		GPSansBossAnimationSetup::SansAreaAttackPath,
		GPSansBossAnimationSetup::AreaAttackMontageName,
		TEXT("Sans sword ground pound"));
	if (!AreaAttackMontage)
	{
		return false;
	}

	UAnimBlueprint* AnimBlueprint = GPSansBossAnimationSetup::CreateOrUpdateAnimBlueprint(
		SansSkeleton,
		SansMesh,
		DefaultIdle);
	if (!AnimBlueprint)
	{
		return false;
	}

	UPDA_CharacterAnimationSet* AnimationSet = GPSansBossAnimationSetup::CreateOrUpdateAnimationSet(
		SansMesh,
		AnimBlueprint,
		BasicAttackMontage,
		SweepAttackMontage,
		HeavyAttackMontage,
		AreaAttackMontage);
	if (!AnimationSet)
	{
		return false;
	}

	if (!GPSansBossAnimationSetup::AssignBossBlueprint(SansMesh, AnimBlueprint, AnimationSet))
	{
		return false;
	}

	UE_LOG(LogTemp, Display, TEXT("Sans boss animation setup completed successfully."));
	return true;
#else
	UE_LOG(LogTemp, Warning, TEXT("CreateSansBossAnimationSetup is only available in editor builds."));
	return false;
#endif
}

bool UGP_AnimationSetupLibrary::CreateCrystalSeraphAnimationSetup()
{
#if WITH_EDITOR
	USkeletalMesh* CrystalSeraphMesh = GPFemaleAnimationSetup::LoadRequiredAsset<USkeletalMesh>(*GPCrystalSeraphAnimationSetup::CrystalSeraphMeshPath);
	USkeleton* CrystalSeraphSkeleton = GPFemaleAnimationSetup::LoadRequiredAsset<USkeleton>(*GPCrystalSeraphAnimationSetup::CrystalSeraphSkeletonPath);
	UAnimSequence* HoverIdle = GPFemaleAnimationSetup::LoadRequiredAsset<UAnimSequence>(*GPCrystalSeraphAnimationSetup::HoverIdlePath);
	if (!CrystalSeraphMesh || !CrystalSeraphSkeleton || !HoverIdle)
	{
		return false;
	}

	UAnimMontage* BasicAttackMontage = GPCrystalSeraphAnimationSetup::CreateOrUpdatePatternMontage(
		CrystalSeraphSkeleton,
		{
			{ GPCrystalSeraphAnimationSetup::BasicAttackEnterSequencePath },
			{ GPCrystalSeraphAnimationSetup::BasicAttackShootSequencePath },
			{ GPCrystalSeraphAnimationSetup::BasicAttackHoldSequencePath, GPCrystalSeraphAnimationSetup::BasicAttackHoldTime },
			{ GPCrystalSeraphAnimationSetup::BasicAttackExitSequencePath }
		},
		GPCrystalSeraphAnimationSetup::BasicAttackMontageName,
		TEXT("Crystal Seraph simple basic attack"));
	if (!BasicAttackMontage)
	{
		return false;
	}

	UAnimMontage* LaserAttackMontage = GPCrystalSeraphAnimationSetup::CreateOrUpdatePatternMontage(
		CrystalSeraphSkeleton,
		{
			{ GPCrystalSeraphAnimationSetup::LaserAttackEnterSequencePath },
			{ GPCrystalSeraphAnimationSetup::LaserAttackShootSequencePath },
			{ GPCrystalSeraphAnimationSetup::LaserAttackHoldSequencePath, GPCrystalSeraphAnimationSetup::LaserAttackHoldTime },
			{ GPCrystalSeraphAnimationSetup::LaserAttackExitSequencePath }
		},
		GPCrystalSeraphAnimationSetup::LaserAttackMontageName,
		TEXT("Crystal Seraph double laser attack"));
	if (!LaserAttackMontage)
	{
		return false;
	}

	UAnimMontage* DeathMontage = GPCrystalSeraphAnimationSetup::CreateOrUpdateMontageFromSequence(
		CrystalSeraphSkeleton,
		GPCrystalSeraphAnimationSetup::DeathSequencePath,
		GPCrystalSeraphAnimationSetup::DeathMontageName,
		TEXT("Crystal Seraph death"));

	UAnimBlueprint* AnimBlueprint = GPCrystalSeraphAnimationSetup::CreateOrUpdateAnimBlueprint(CrystalSeraphSkeleton, CrystalSeraphMesh, HoverIdle);
	if (!AnimBlueprint)
	{
		return false;
	}

	UPDA_EnemyAnimationSet* AnimationSet = GPCrystalSeraphAnimationSetup::CreateOrUpdateAnimationSet(
		CrystalSeraphMesh,
		HoverIdle,
		AnimBlueprint,
		BasicAttackMontage,
		LaserAttackMontage,
		DeathMontage);
	if (!AnimationSet)
	{
		return false;
	}

	if (!GPCrystalSeraphAnimationSetup::AssignBossBlueprint(CrystalSeraphMesh, AnimBlueprint, AnimationSet, BasicAttackMontage, LaserAttackMontage))
	{
		return false;
	}

	UE_LOG(LogTemp, Display, TEXT("Crystal Seraph animation setup completed successfully."));
	return true;
#else
	UE_LOG(LogTemp, Warning, TEXT("CreateCrystalSeraphAnimationSetup is only available in editor builds."));
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
