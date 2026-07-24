"""Persistently disable the two production player AnimBP debug outputs."""

import unreal


ANIM_BLUEPRINT_PATHS = (
    "/Game/Characters/PlayerCharacter/ABP_UEFNSource_Player",
    "/Game/Characters/MaskMan/ABP_MaskMan_Player",
)


def disable_debug_output(asset_path: str) -> None:
    blueprint = unreal.load_asset(asset_path)
    if blueprint is None:
        raise RuntimeError(f"Anim Blueprint is missing: {asset_path}")

    generated_class = blueprint.generated_class()
    if generated_class is None:
        raise RuntimeError(f"Anim Blueprint has no generated class: {asset_path}")

    class_defaults = unreal.get_default_object(generated_class)
    if not class_defaults.get_editor_property("enable_debug_log"):
        unreal.log(f"[AnimDebug] Already disabled: {asset_path}")
        return

    class_defaults.modify()
    blueprint.modify()
    class_defaults.set_editor_property("enable_debug_log", False)

    if not unreal.EditorAssetLibrary.save_loaded_asset(
        blueprint, only_if_is_dirty=False
    ):
        raise RuntimeError(f"Unable to save Anim Blueprint: {asset_path}")

    unreal.log(f"[AnimDebug] Disabled bEnableDebugLog: {asset_path}")


for anim_blueprint_path in ANIM_BLUEPRINT_PATHS:
    disable_debug_output(anim_blueprint_path)
