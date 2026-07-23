"""Import and apply the player-status HUD textures without rebuilding UMG trees.

This script intentionally edits only the existing ``HealthBar`` ProgressBar
template inside the three attribute Widget Blueprints.  It does not touch
``WBP_PlayerHUDWidget`` or add/remove any widgets.

Recommended use:
    1. Close WBP_PlayerHealthBar, WBP_PlayerManaBar, and
       WBP_PlayerStaminaBar in the Widget Blueprint editor.
    2. Run this file with Tools > Execute Python Script.
    3. Reopen the widgets and visually verify them before committing.

Commandlet use is also supported after the main editor is closed:
    UnrealEditor-Cmd.exe Project_Eden.uproject -run=pythonscript \
        -script=Scripts/Editor/apply_player_status_hud.py \
        -unattended -nop4 -nosplash -stdout -FullStdOutLogOutput

Pass ``--dry-run`` to validate source files, assets, and widget templates
without importing, compiling, or saving.  Pass ``--force-reimport`` to import
the PNGs even when their corresponding uassets are newer than the source.

UE 5.7 does not expose UWidgetBlueprint.WidgetTree through Python in this
engine build.  The script therefore resolves the existing ProgressBar template
by its stable subobject path, with a tightly filtered ObjectIterator fallback.
"""

from __future__ import annotations

import os
import sys
from pathlib import Path
from typing import Iterable

import unreal


LOG_PREFIX = "[PlayerStatusHUD]"
DRY_RUN = "--dry-run" in sys.argv or os.environ.get(
    "EDEN_HUD_DRY_RUN", ""
).lower() in {"1", "true", "yes", "on"}
FORCE_REIMPORT = "--force-reimport" in sys.argv

PROJECT_ROOT = Path(__file__).resolve().parents[2]
SOURCE_DIRECTORY = PROJECT_ROOT / "Content" / "UI" / "HUD" / "PlayerStatus" / "Textures"
DESTINATION_DIRECTORY = "/Game/UI/HUD/PlayerStatus/Textures"

TEXTURE_FILES = (
    "T_UI_HUD_AttributeFillMask_B.png",
    "T_UI_HUD_AttributeTrack_B.png",
    "T_UI_HUD_TopLeft_Accent_B2.png",
    "T_UI_HUD_TopLeft_Backplate_B2.png",
)

FILL_TEXTURE_NAME = "T_UI_HUD_AttributeFillMask_B"
TRACK_TEXTURE_NAME = "T_UI_HUD_AttributeTrack_B"

WIDGET_SPECS = (
    (
        "/Game/UI/HUD/WBP_PlayerHealthBar",
        # Display sRGB #7A2824 converted to FLinearColor.
        unreal.LinearColor(0.194618, 0.021219, 0.017642, 1.0),
        "health",
    ),
    (
        "/Game/UI/HUD/WBP_PlayerManaBar",
        # Display sRGB #315E73 converted to FLinearColor.
        unreal.LinearColor(0.030713, 0.111932, 0.171441, 1.0),
        "mana",
    ),
    (
        "/Game/UI/HUD/WBP_PlayerStaminaBar",
        # Display sRGB #77713A converted to FLinearColor.
        unreal.LinearColor(0.184475, 0.165132, 0.042311, 1.0),
        "stamina",
    ),
)

errors: list[str] = []


def log(message: str) -> None:
    mode = "[DRY-RUN]" if DRY_RUN else ""
    unreal.log(f"{LOG_PREFIX}{mode} {message}")


def warn(message: str) -> None:
    unreal.log_warning(f"{LOG_PREFIX} {message}")


def record_error(message: str) -> None:
    errors.append(message)
    unreal.log_error(f"{LOG_PREFIX} {message}")


def asset_path_for_texture(texture_name: str) -> str:
    return f"{DESTINATION_DIRECTORY}/{texture_name}"


def resolve_enum(enum_type, names: Iterable[str]):
    """Resolve enum spellings defensively across nearby Unreal versions."""
    for name in names:
        value = getattr(enum_type, name, None)
        if value is not None:
            return value
    raise RuntimeError(
        f"Unable to resolve {enum_type.__name__} member from {tuple(names)}"
    )


def describe_object(obj) -> str:
    get_name = getattr(obj, "get_name", None)
    if callable(get_name):
        try:
            return get_name()
        except Exception:
            pass
    return type(obj).__name__


def set_required_property(obj, property_name: str, value) -> None:
    try:
        obj.set_editor_property(property_name, value)
    except Exception as exc:
        raise RuntimeError(
            f"{describe_object(obj)}: unable to set required property "
            f"'{property_name}': {exc}"
        ) from exc


def set_optional_property(obj, property_name: str, value) -> None:
    try:
        obj.set_editor_property(property_name, value)
    except Exception as exc:
        warn(
            f"{describe_object(obj)}: optional property "
            f"'{property_name}' skipped: {exc}"
        )


def should_import(source_file: Path, uasset_file: Path) -> bool:
    if FORCE_REIMPORT or not uasset_file.exists():
        return True
    try:
        return source_file.stat().st_mtime_ns > uasset_file.stat().st_mtime_ns
    except OSError:
        # When timestamps cannot be trusted, explicit reimport is safer.
        return True


def run_import_task(source_file: Path, asset_name: str) -> None:
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(source_file))
    task.set_editor_property("destination_path", DESTINATION_DIRECTORY)
    task.set_editor_property("destination_name", asset_name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("replace_existing_settings", False)
    # Save after applying the deterministic UI texture settings below.
    task.set_editor_property("save", False)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    imported_paths = list(task.get_editor_property("imported_object_paths"))
    expected_path = asset_path_for_texture(asset_name)
    if not unreal.EditorAssetLibrary.does_asset_exist(expected_path):
        raise RuntimeError(
            f"Import did not create '{expected_path}'. Imported paths: {imported_paths}"
        )


def configure_ui_texture(texture, *, srgb: bool) -> None:
    compression = resolve_enum(
        unreal.TextureCompressionSettings,
        ("TC_EDITOR_ICON", "TC_USER_INTERFACE2D"),
    )
    no_mips = resolve_enum(
        unreal.TextureMipGenSettings,
        ("TMGS_NO_MIPMAPS", "NO_MIPMAPS"),
    )
    clamp = resolve_enum(unreal.TextureAddress, ("TA_CLAMP", "CLAMP"))
    ui_group = resolve_enum(
        unreal.TextureGroup,
        ("TEXTUREGROUP_UI", "UI"),
    )

    set_required_property(texture, "compression_settings", compression)
    set_required_property(texture, "mip_gen_settings", no_mips)
    set_required_property(texture, "address_x", clamp)
    set_required_property(texture, "address_y", clamp)
    set_required_property(texture, "lod_group", ui_group)
    set_required_property(texture, "srgb", srgb)
    set_optional_property(texture, "never_stream", True)
    set_optional_property(texture, "virtual_texture_streaming", False)


def import_and_configure_texture(filename: str):
    source_file = SOURCE_DIRECTORY / filename
    asset_name = source_file.stem
    asset_path = asset_path_for_texture(asset_name)
    uasset_file = source_file.with_suffix(".uasset")

    if not source_file.is_file():
        raise RuntimeError(f"Missing PNG source: {source_file}")

    needs_import = should_import(source_file, uasset_file)
    action = "reimport" if uasset_file.exists() else "import"
    if DRY_RUN:
        if needs_import:
            log(f"Would {action} {source_file.name} -> {asset_path}")
        else:
            log(f"Texture source is up to date: {asset_path}")
        return unreal.EditorAssetLibrary.load_asset(asset_path)

    if needs_import:
        log(f"{action.capitalize()}ing {source_file.name} -> {asset_path}")
        run_import_task(source_file, asset_name)
    else:
        log(f"Texture source is up to date: {asset_path}")

    texture = unreal.EditorAssetLibrary.load_asset(asset_path)
    if texture is None:
        raise RuntimeError(f"Unable to load imported texture: {asset_path}")
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError(
            f"Expected Texture2D at {asset_path}, got {type(texture).__name__}"
        )

    is_fill_mask = asset_name == FILL_TEXTURE_NAME
    configure_ui_texture(texture, srgb=not is_fill_mask)
    if not unreal.EditorAssetLibrary.save_loaded_asset(
        texture, only_if_is_dirty=False
    ):
        raise RuntimeError(f"Unable to save configured texture: {asset_path}")

    log(
        f"Configured {asset_path}: UI compression, no mips, clamp, "
        f"sRGB={'off' if is_fill_mask else 'on'}"
    )
    return texture


def find_progress_bar(widget_blueprint, blueprint_asset_path: str):
    """Find the existing ProgressBar template without traversing WidgetTree."""
    asset_name = blueprint_asset_path.rsplit("/", 1)[-1]
    exact_path = (
        f"{blueprint_asset_path}.{asset_name}:WidgetTree.HealthBar"
    )

    try:
        exact_match = unreal.find_object(None, exact_path)
    except Exception:
        exact_match = None
    if isinstance(exact_match, unreal.ProgressBar):
        return exact_match

    iterator_type = getattr(unreal, "ObjectIterator", None)
    if iterator_type is None:
        raise RuntimeError(
            f"ProgressBar subobject not found at '{exact_path}', and "
            "unreal.ObjectIterator is unavailable"
        )

    expected_prefix = f"{blueprint_asset_path}.{asset_name}:WidgetTree."
    matches = [
        candidate
        for candidate in iterator_type(unreal.ProgressBar)
        if candidate.get_name() == "HealthBar"
        and candidate.get_path_name().startswith(expected_prefix)
    ]
    if len(matches) != 1:
        paths = [candidate.get_path_name() for candidate in matches]
        raise RuntimeError(
            f"Expected exactly one ProgressBar named HealthBar in "
            f"{blueprint_asset_path}; found {len(matches)}: {paths}"
        )
    return matches[0]


def make_image_brush(texture, width: float, height: float):
    brush = unreal.SlateBrush()
    set_required_property(brush, "resource_object", texture)

    # UE 5.7 changed FSlateBrush.ImageSize from FVector2D to the
    # single-precision FDeprecateSlateVector2D wrapper.  Older versions still
    # expect Vector2D, so try the native type for each engine generation.
    image_size_errors = []
    for vector_type_name in (
        "DeprecateSlateVector2D",
        "Vector2f",
        "Vector2D",
    ):
        vector_type = getattr(unreal, vector_type_name, None)
        if vector_type is None:
            continue
        try:
            try:
                image_size = vector_type(float(width), float(height))
            except TypeError:
                # BlueprintInternalUseOnly structs such as UE 5.7's
                # DeprecateSlateVector2D expose only a zero-argument Python
                # constructor even though their X/Y fields are editable.
                image_size = vector_type()
                try:
                    image_size.set_editor_property("x", float(width))
                    image_size.set_editor_property("y", float(height))
                except Exception:
                    image_size.x = float(width)
                    image_size.y = float(height)
            brush.set_editor_property(
                "image_size",
                image_size,
            )
            break
        except Exception as exc:
            image_size_errors.append(f"{vector_type_name}: {exc}")
    else:
        raise RuntimeError(
            "SlateBrush: unable to set image_size with any supported vector "
            f"type: {image_size_errors}"
        )

    draw_as = resolve_enum(unreal.SlateBrushDrawType, ("IMAGE",))
    no_tile = resolve_enum(unreal.SlateBrushTileType, ("NO_TILE",))
    set_required_property(brush, "draw_as", draw_as)
    set_required_property(brush, "tiling", no_tile)
    set_optional_property(brush, "margin", unreal.Margin(0.0))
    return brush


def style_attribute_widget(
    blueprint_asset_path: str,
    fill_tint,
    label: str,
    track_texture,
    fill_texture,
) -> None:
    blueprint = unreal.EditorAssetLibrary.load_asset(blueprint_asset_path)
    if blueprint is None:
        raise RuntimeError(f"Unable to load Widget Blueprint: {blueprint_asset_path}")
    if not isinstance(blueprint, unreal.WidgetBlueprint):
        raise RuntimeError(
            f"Expected WidgetBlueprint at {blueprint_asset_path}, "
            f"got {type(blueprint).__name__}"
        )

    progress_bar = find_progress_bar(blueprint, blueprint_asset_path)
    log(f"Resolved {label} ProgressBar: {progress_bar.get_path_name()}")

    if DRY_RUN:
        if track_texture is None or fill_texture is None:
            log(
                f"Would assign imported track/fill brushes and {label} tint "
                "(texture uassets do not exist yet)"
            )
        else:
            log(f"Would assign shared track/fill brushes and {label} tint")
        return

    if track_texture is None or fill_texture is None:
        raise RuntimeError(
            f"Cannot style {blueprint_asset_path}: track/fill texture is missing"
        )

    style = progress_bar.get_editor_property("widget_style")
    style.set_editor_property(
        "background_image",
        make_image_brush(track_texture, 512.0, 24.0),
    )
    style.set_editor_property(
        "fill_image",
        make_image_brush(fill_texture, 512.0, 24.0),
    )
    set_optional_property(style, "enable_fill_animation", False)
    set_required_property(progress_bar, "widget_style", style)
    set_required_property(progress_bar, "fill_color_and_opacity", fill_tint)

    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    if not unreal.EditorAssetLibrary.save_loaded_asset(
        blueprint, only_if_is_dirty=False
    ):
        raise RuntimeError(f"Unable to save Widget Blueprint: {blueprint_asset_path}")
    log(f"Compiled and saved {blueprint_asset_path} with {label} styling")


def main() -> None:
    log(
        f"Starting. source={SOURCE_DIRECTORY}, "
        f"force_reimport={FORCE_REIMPORT}"
    )

    textures = {}
    for filename in TEXTURE_FILES:
        try:
            texture = import_and_configure_texture(filename)
            textures[Path(filename).stem] = texture
        except Exception as exc:
            record_error(f"{filename}: {exc}")

    track_texture = textures.get(TRACK_TEXTURE_NAME)
    fill_texture = textures.get(FILL_TEXTURE_NAME)

    for blueprint_path, fill_tint, label in WIDGET_SPECS:
        try:
            style_attribute_widget(
                blueprint_path,
                fill_tint,
                label,
                track_texture,
                fill_texture,
            )
        except Exception as exc:
            record_error(f"{blueprint_path}: {exc}")

    if errors:
        summary = "\n".join(f"  - {message}" for message in errors)
        raise RuntimeError(
            f"{LOG_PREFIX} Finished with {len(errors)} error(s):\n{summary}"
        )

    if DRY_RUN:
        log("Dry run completed successfully; no assets were changed.")
    else:
        log(
            "Completed successfully. WBP_PlayerHUDWidget was intentionally "
            "left unchanged."
        )


if __name__ == "__main__":
    main()
