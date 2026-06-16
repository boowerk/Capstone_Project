#include "Commandlets/GP_DownsizeLavaTexturesCommandlet.h"

#include "Engine/Texture.h"
#include "Interfaces/ITargetPlatformManagerModule.h"
#include "Misc/PackageName.h"
#include "TextureImportSettings.h"
#include "TextureSourceDataUtils.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace
{
struct FLavaTextureResizeTarget
{
	const TCHAR* AssetPath;
	int32 MaxSourceSize;
};

const FLavaTextureResizeTarget GLavaTextureResizeTargets[] = {
	{ TEXT("/Game/Fab/Lava_Material/Textures/T_Lava_01/T_Lava_01_basecolor"), 2048 },
	{ TEXT("/Game/Fab/Lava_Material/Textures/T_Lava_01/T_Lava_01_normal"), 2048 },
	{ TEXT("/Game/Fab/Lava_Material/Textures/T_Lava_01/T_Lava_01_emissive"), 2048 },
	{ TEXT("/Game/Fab/Lava_Material/Textures/T_Lava_01/T_Lava_01_ambientocclusion"), 1024 },
	{ TEXT("/Game/Fab/Lava_Material/Textures/T_Lava_01/T_Lava_01_roughness"), 1024 },
	{ TEXT("/Game/Fab/Lava_Material/Textures/T_Lava_01/T_Lava_01_height"), 1024 },
};

bool SaveTexturePackage(UTexture* Texture)
{
	UPackage* Package = Texture ? Texture->GetOutermost() : nullptr;
	if (!Package)
	{
		return false;
	}

	FString PackageFilename;
	if (!FPackageName::TryConvertLongPackageNameToFilename(Package->GetName(), PackageFilename, FPackageName::GetAssetPackageExtension()))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to convert package path: %s"), *Package->GetName());
		return false;
	}

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.SaveFlags = SAVE_NoError;

	return UPackage::SavePackage(Package, Texture, *PackageFilename, SaveArgs);
}
} // namespace

UGP_DownsizeLavaTexturesCommandlet::UGP_DownsizeLavaTexturesCommandlet()
{
	IsClient = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UGP_DownsizeLavaTexturesCommandlet::Main(const FString& Params)
{
#if WITH_EDITOR
	const ITargetPlatform* RunningPlatform = GetTargetPlatformManagerRef().GetRunningTargetPlatform();
	if (!RunningPlatform)
	{
		UE_LOG(LogTemp, Error, TEXT("No running target platform."));
		return 1;
	}

	int32 FailureCount = 0;

	for (const FLavaTextureResizeTarget& Target : GLavaTextureResizeTargets)
	{
		UTexture* Texture = LoadObject<UTexture>(nullptr, Target.AssetPath);
		if (!Texture)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to load texture: %s"), Target.AssetPath);
			++FailureCount;
			continue;
		}

		const FIntPoint BeforeSize = Texture->Source.GetLogicalSize();
		UE_LOG(LogTemp, Display, TEXT("Downsize lava texture: %s source=%dx%d target<=%d"),
			Target.AssetPath,
			BeforeSize.X,
			BeforeSize.Y,
			Target.MaxSourceSize);

		const bool bResized = UE::TextureUtilitiesCommon::Experimental::DownsizeTextureSourceData(
			Texture,
			Target.MaxSourceSize,
			RunningPlatform);

		const FIntPoint AfterSize = Texture->Source.GetLogicalSize();
		if (bResized)
		{
			Texture->LODBias = 0;
			UE::TextureUtilitiesCommon::ApplyDefaultsForNewlyImportedTextures(Texture, true);
			Texture->PostEditChange();
			Texture->MarkPackageDirty();
		}

		if (!SaveTexturePackage(Texture))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to save texture package: %s"), Target.AssetPath);
			++FailureCount;
			continue;
		}

		UE_LOG(LogTemp, Display, TEXT("Saved lava texture: %s source=%dx%d resized=%s"),
			Target.AssetPath,
			AfterSize.X,
			AfterSize.Y,
			bResized ? TEXT("true") : TEXT("false"));
	}

	return FailureCount == 0 ? 0 : 1;
#else
	return 1;
#endif
}
