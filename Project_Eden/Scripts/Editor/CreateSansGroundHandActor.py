"""Create the designer-facing visual Blueprint for Sans's native Ground Hands strike."""

import unreal


ASSET_DIRECTORY = "/Game/Characters/EnemyCharacter/Boss/BP_Boss_Sans"
ASSET_NAME = "BP_BossGroundHandActor"
ASSET_PATH = f"{ASSET_DIRECTORY}/{ASSET_NAME}"
PARENT_CLASS_PATH = "/Script/Project_Eden.GP_BossGroundHandActor"
DEFAULT_HAND_VISUAL_SCALE = 0.35


def create_sans_ground_hand_blueprint() -> None:
    """Create a thin BP that changes presentation defaults without owning gameplay collision."""
    parent_class = unreal.load_class(None, PARENT_CLASS_PATH)
    if parent_class is None:
        raise RuntimeError(f"Unable to load Ground Hands parent class: {PARENT_CLASS_PATH}")

    blueprint = unreal.EditorAssetLibrary.load_asset(ASSET_PATH)
    was_created = blueprint is None
    if was_created:
        # Damage, timing, motion, and hit collision stay in native code; this asset is for visual tuning only.
        factory = unreal.BlueprintFactory()
        factory.set_editor_property("parent_class", parent_class)
        blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            ASSET_NAME,
            ASSET_DIRECTORY,
            unreal.Blueprint,
            factory,
        )
    if blueprint is None:
        raise RuntimeError(f"Unable to create Ground Hands Blueprint: {ASSET_PATH}")

    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    if was_created:
        generated_class = blueprint.generated_class()
        default_actor = unreal.get_default_object(generated_class)
        # Start smaller than the imported source mesh while preserving future designer edits on script reruns.
        default_actor.set_editor_property("hand_visual_scale", DEFAULT_HAND_VISUAL_SCALE)

    if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False):
        raise RuntimeError(f"Unable to save Ground Hands Blueprint: {ASSET_PATH}")
    unreal.log(f"Created or refreshed Ground Hands visual Blueprint: {ASSET_PATH}")


create_sans_ground_hand_blueprint()
