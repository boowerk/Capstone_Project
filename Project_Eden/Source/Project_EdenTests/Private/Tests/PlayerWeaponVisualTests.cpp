#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Animation/AnimMontage.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Animation/Skeleton.h"
#include "Components/StaticMeshComponent.h"
#include "Characters/GP_PlayerCharacter.h"
#include "Engine/Blueprint.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Materials/MaterialInterface.h"
#include "Misc/AutomationTest.h"
#include "NiagaraEmitter.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraMeshRendererProperties.h"
#include "NiagaraSystem.h"
#include "UObject/UnrealType.h"

namespace GP_PlayerVisualTests
{
	const TCHAR* PrimaryMontagePaths[] = {
		TEXT("/Game/Characters/UEFN_Mannequin/Animations/Montage/MeleeAttacks/Sword/Light/AM_UEFN_Sword_Light_A.AM_UEFN_Sword_Light_A"),
		TEXT("/Game/Characters/UEFN_Mannequin/Animations/Montage/MeleeAttacks/Sword/Light/AM_UEFN_Sword_Light_B.AM_UEFN_Sword_Light_B"),
		TEXT("/Game/Characters/UEFN_Mannequin/Animations/Montage/MeleeAttacks/Sword/Light/AM_UEFN_Sword_Light_C.AM_UEFN_Sword_Light_C"),
		TEXT("/Game/Characters/UEFN_Mannequin/Animations/Montage/MeleeAttacks/Sword/Light/AM_UEFN_Sword_Light_D.AM_UEFN_Sword_Light_D"),
	};

	const UObject* GetNiagaraTemplate(const UObject* NotifyObject)
	{
		if (!NotifyObject)
		{
			return nullptr;
		}

		const FObjectPropertyBase* TemplateProperty =
			FindFProperty<FObjectPropertyBase>(NotifyObject->GetClass(), TEXT("Template"));
		return TemplateProperty
			? TemplateProperty->GetObjectPropertyValue_InContainer(NotifyObject)
			: nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGPPlayerWeaponNiagaraSourceTest,
	"ProjectEden.Player.WeaponVisual.NiagaraSource",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGPPlayerWeaponNiagaraSourceTest::RunTest(const FString& Parameters)
{
	const UNiagaraSystem* BigSwordSystem = LoadObject<UNiagaraSystem>(
		nullptr,
		TEXT("/Game/Mixed_Magic_VFX_Pack/VFX/NS_Big_Sword.NS_Big_Sword"));
	const UStaticMesh* ExpectedSwordMesh = LoadObject<UStaticMesh>(
		nullptr,
		TEXT("/Game/Mixed_Magic_VFX_Pack/Static_Meshes/SM_7.SM_7"));
	const UMaterialInterface* ExpectedSwordMaterial = LoadObject<UMaterialInterface>(
		nullptr,
		TEXT("/Game/Mixed_Magic_VFX_Pack/Materials/Instance_Materials/MI_Ice_Inst_4.MI_Ice_Inst_4"));

	TestNotNull(TEXT("The authored Niagara big-sword system exists"), BigSwordSystem);
	TestNotNull(TEXT("The Niagara sword mesh exists"), ExpectedSwordMesh);
	TestNotNull(TEXT("The Niagara sword material exists"), ExpectedSwordMaterial);
	if (!BigSwordSystem || !ExpectedSwordMesh || !ExpectedSwordMaterial)
	{
		return false;
	}

	bool bFoundExpectedMesh = false;
	bool bFoundExpectedMaterialOverride = false;
	for (const FNiagaraEmitterHandle& EmitterHandle : BigSwordSystem->GetEmitterHandles())
	{
		const FVersionedNiagaraEmitterData* EmitterData = EmitterHandle.GetEmitterData();
		if (!EmitterData)
		{
			continue;
		}

		for (const UNiagaraRendererProperties* Renderer : EmitterData->GetRenderers())
		{
			const UNiagaraMeshRendererProperties* MeshRenderer =
				Cast<UNiagaraMeshRendererProperties>(Renderer);
			if (!MeshRenderer)
			{
				continue;
			}

			for (const FNiagaraMeshRendererMeshProperties& MeshProperties : MeshRenderer->Meshes)
			{
				AddInfo(FString::Printf(
					TEXT("Niagara sword renderer mesh: %s"),
					*GetPathNameSafe(MeshProperties.Mesh)));
				bFoundExpectedMesh |= MeshProperties.Mesh == ExpectedSwordMesh;
			}

			for (const FNiagaraMeshMaterialOverride& MaterialOverride : MeshRenderer->OverrideMaterials)
			{
				AddInfo(FString::Printf(
					TEXT("Niagara sword renderer material override: %s"),
					*GetPathNameSafe(MaterialOverride.ExplicitMat)));
				bFoundExpectedMaterialOverride |= MaterialOverride.ExplicitMat == ExpectedSwordMaterial;
			}
		}
	}

	TestTrue(TEXT("NS_Big_Sword uses SM_7 in its mesh renderer"), bFoundExpectedMesh);
	TestTrue(
		TEXT("NS_Big_Sword overrides that renderer with MI_Ice_Inst_4"),
		bFoundExpectedMaterialOverride);

	UBlueprint* PlayerBlueprint = LoadObject<UBlueprint>(
		nullptr,
		TEXT("/Game/Characters/PlayerCharacter/BP_GP_PlayerCharacter.BP_GP_PlayerCharacter"));
	const AGP_PlayerCharacter* PlayerDefaults =
		PlayerBlueprint && PlayerBlueprint->GeneratedClass
		? Cast<AGP_PlayerCharacter>(PlayerBlueprint->GeneratedClass->GetDefaultObject())
		: nullptr;
	const UStaticMeshComponent* WeaponMesh =
		PlayerDefaults ? PlayerDefaults->GetPersistentWeaponMesh() : nullptr;

	TestNotNull(TEXT("The production player Blueprint uses AGP_PlayerCharacter"), PlayerDefaults);
	TestNotNull(TEXT("The production player owns an always-visible weapon mesh"), WeaponMesh);
	TestTrue(
		TEXT("The persistent weapon reuses NS_Big_Sword's SM_7"),
		WeaponMesh && WeaponMesh->GetStaticMesh() == ExpectedSwordMesh);
	TestTrue(
		TEXT("The persistent weapon reuses NS_Big_Sword's MI_Ice_Inst_4"),
		WeaponMesh && WeaponMesh->GetMaterial(0) == ExpectedSwordMaterial);
	TestTrue(
		TEXT("The weapon follows the visible deforming character mesh"),
		PlayerDefaults && WeaponMesh && WeaponMesh->GetAttachParent() == PlayerDefaults->GetMesh());
	TestEqual(
		TEXT("The weapon uses the shared per-character hand socket"),
		WeaponMesh ? WeaponMesh->GetAttachSocketName() : NAME_None,
		FName(TEXT("hand_rSocket")));
	TestFalse(
		TEXT("The weapon inherits each socket's authored scale"),
		WeaponMesh && WeaponMesh->IsUsingAbsoluteScale());
	TestTrue(
		TEXT("The weapon adds no location offset on top of the socket"),
		WeaponMesh && WeaponMesh->GetRelativeLocation().IsNearlyZero());
	TestTrue(
		TEXT("The weapon adds no rotation offset on top of the socket"),
		WeaponMesh && WeaponMesh->GetRelativeRotation().IsNearlyZero());
	TestTrue(
		TEXT("The weapon adds no scale offset on top of the socket"),
		WeaponMesh && WeaponMesh->GetRelativeScale3D().Equals(FVector::OneVector));
	TestTrue(
		TEXT("The decorative weapon has no collision"),
		WeaponMesh && WeaponMesh->GetCollisionEnabled() == ECollisionEnabled::NoCollision);

	const TCHAR* PlayerSkeletonPaths[] = {
		TEXT("/Game/Characters/MaskMan/SK_MaskMan_Skeleton.SK_MaskMan_Skeleton"),
		TEXT("/Game/Characters/Stylized_Paladin/Stylized_Paladin_Skeleton.Stylized_Paladin_Skeleton"),
		TEXT("/Game/Characters/daelithra/Stylized_Deamon_Girl_Daelithra_Skeleton.Stylized_Deamon_Girl_Daelithra_Skeleton"),
	};
	for (const TCHAR* SkeletonPath : PlayerSkeletonPaths)
	{
		const USkeleton* Skeleton = LoadObject<USkeleton>(nullptr, SkeletonPath);
		const USkeletalMeshSocket* HandSocket =
			Skeleton ? Skeleton->FindSocket(FName(TEXT("hand_rSocket"))) : nullptr;

		TestNotNull(
			FString::Printf(TEXT("Playable skeleton contains hand_rSocket: %s"), SkeletonPath),
			HandSocket);
		TestEqual(
			FString::Printf(TEXT("hand_rSocket follows the deforming hand bone: %s"), SkeletonPath),
			HandSocket ? HandSocket->BoneName : NAME_None,
			FName(TEXT("hand_r")));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGPPlayerPrimaryFreeMagicSlashRemovedTest,
	"ProjectEden.Player.WeaponVisual.PrimaryFreeMagicSlashRemoved",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGPPlayerPrimaryFreeMagicSlashRemovedTest::RunTest(const FString& Parameters)
{
	const UNiagaraSystem* FreeMagicSlash = LoadObject<UNiagaraSystem>(
		nullptr,
		TEXT("/Game/Imported_VFX/Free_Magic/VFX_Niagara/NS_Free_Magic_Slash.NS_Free_Magic_Slash"));
	const UNiagaraSystem* ArrowTrailMagic = LoadObject<UNiagaraSystem>(
		nullptr,
		TEXT("/Game/Imported_VFX/ArrowTrail/FX/NS_ArrowTrail_Magic.NS_ArrowTrail_Magic"));

	TestNotNull(TEXT("The legacy Free Magic Slash asset exists for regression detection"), FreeMagicSlash);
	TestNotNull(TEXT("The retained timed Arrow Trail asset exists"), ArrowTrailMagic);
	if (!FreeMagicSlash || !ArrowTrailMagic)
	{
		return false;
	}

	for (const TCHAR* MontagePath : GP_PlayerVisualTests::PrimaryMontagePaths)
	{
		const UAnimMontage* Montage = LoadObject<UAnimMontage>(nullptr, MontagePath);
		TestNotNull(FString::Printf(TEXT("Primary montage exists: %s"), MontagePath), Montage);
		if (!Montage)
		{
			continue;
		}

		bool bHasFreeMagicSlash = false;
		bool bHasTimedArrowTrail = false;
		bool bHasGameplayEvent = false;
		for (const FAnimNotifyEvent& NotifyEvent : Montage->Notifies)
		{
			const UAnimNotify* Notify = NotifyEvent.Notify;
			const UAnimNotifyState* NotifyState = NotifyEvent.NotifyStateClass;
			bHasFreeMagicSlash |=
				GP_PlayerVisualTests::GetNiagaraTemplate(Notify) == FreeMagicSlash
				|| GP_PlayerVisualTests::GetNiagaraTemplate(NotifyState) == FreeMagicSlash;
			bHasTimedArrowTrail |=
				GP_PlayerVisualTests::GetNiagaraTemplate(NotifyState) == ArrowTrailMagic;
			bHasGameplayEvent |=
				Notify && Notify->GetClass()->GetName().Contains(TEXT("GP_AnimNotify_SendGameplayEvent"));
		}

		TestFalse(
			FString::Printf(TEXT("%s no longer spawns NS_Free_Magic_Slash"), *Montage->GetName()),
			bHasFreeMagicSlash);
		TestTrue(
			FString::Printf(TEXT("%s retains its timed sword trail"), *Montage->GetName()),
			bHasTimedArrowTrail);
		TestTrue(
			FString::Printf(TEXT("%s retains its gameplay event notifies"), *Montage->GetName()),
			bHasGameplayEvent);
	}

	return true;
}

#endif
