import unreal


ASSET_DIRECTORY = "/Game/Characters/EnemyCharacter/Boss/BP_Boss_DarkArmorKnight"
ASSET_NAME = "BP_DarkArmorKnight"
ASSET_PATH = f"{ASSET_DIRECTORY}/{ASSET_NAME}"
PARENT_CLASS_PATH = "/Script/Project_Eden.GP_DarkArmorKnightBossCharacter"


def create_dark_armor_knight_blueprint() -> None:
    """Create the thin Blueprint shell used for level placement and later art overrides."""
    parent_class = unreal.load_class(None, PARENT_CLASS_PATH)
    if parent_class is None:
        raise RuntimeError(f"Unable to load Dark Armor Knight parent class: {PARENT_CLASS_PATH}")

    blueprint = unreal.EditorAssetLibrary.load_asset(ASSET_PATH)
    if blueprint is None:
        # Runtime behavior stays in C++; this Blueprint is intentionally limited to designer-facing asset overrides.
        factory = unreal.BlueprintFactory()
        factory.set_editor_property("parent_class", parent_class)
        blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            ASSET_NAME,
            ASSET_DIRECTORY,
            unreal.Blueprint,
            factory,
        )
    if blueprint is None:
        raise RuntimeError(f"Unable to create Dark Armor Knight Blueprint: {ASSET_PATH}")

    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False):
        raise RuntimeError(f"Unable to save Dark Armor Knight Blueprint: {ASSET_PATH}")
    unreal.log(f"Created or refreshed Dark Armor Knight Blueprint: {ASSET_PATH}")


create_dark_armor_knight_blueprint()
