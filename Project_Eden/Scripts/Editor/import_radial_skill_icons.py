"""Import and assign the seven player-pool radial skill icons.

Expected source files:
    Content/UI/Asset/SkillIcons/Radial/Source/
        T_SkillIcon_Radial_<Skill>.png

Imported assets:
    /Game/UI/Asset/SkillIcons/Radial/Textures/
        T_SkillIcon_Radial_<Skill>

The script performs a read-only preflight before changing any asset.  Source
PNGs must be non-interlaced 256x256 RGBA8 images with both visible and
transparent pixels.  Imports are idempotent: an existing texture is reimported
only when its source is newer, unless ``--force-reimport`` is supplied.

Recommended editor use:
    1. Close the seven SkillData assets if they are open.
    2. Run this file with Tools > Execute Python Script.
    3. Verify the radial skill UI before committing the changed assets.

Commandlet use is also supported after the main editor is closed:
    UnrealEditor-Cmd.exe Project_Eden.uproject -run=pythonscript \
        -script=Scripts/Editor/import_radial_skill_icons.py \
        -unattended -nop4 -nosplash -stdout -FullStdOutLogOutput

Pass ``--dry-run`` to validate every PNG, target texture, and SkillData
reference without importing or saving anything.  Pass ``--force-reimport`` to
reimport all seven PNGs even when the destination uassets are newer.

UE 5.7 exposes the UI compression preset as ``TC_EDITOR_ICON`` rather than the
older ``TC_USER_INTERFACE2D`` spelling.  Enum and optional-property handling
below intentionally follows the compatibility pattern used by
``apply_player_status_hud.py``.
"""

from __future__ import annotations

import os
import struct
import sys
import zlib
from pathlib import Path
from typing import Iterable

import unreal


LOG_PREFIX = "[RadialSkillIcons]"
DRY_RUN = "--dry-run" in sys.argv or os.environ.get(
    "EDEN_RADIAL_SKILL_ICON_DRY_RUN", ""
).lower() in {"1", "true", "yes", "on"}
FORCE_REIMPORT = "--force-reimport" in sys.argv

PROJECT_ROOT = Path(__file__).resolve().parents[2]
SOURCE_DIRECTORY = (
    PROJECT_ROOT
    / "Content"
    / "UI"
    / "Asset"
    / "SkillIcons"
    / "Radial"
    / "Source"
)
DESTINATION_FILESYSTEM_DIRECTORY = (
    PROJECT_ROOT
    / "Content"
    / "UI"
    / "Asset"
    / "SkillIcons"
    / "Radial"
    / "Textures"
)
DESTINATION_DIRECTORY = "/Game/UI/Asset/SkillIcons/Radial/Textures"

SKILL_SPECS = (
    (
        "BigHammer",
        "/Game/GAS_Pattern/AbilitySystem/SkillData/Area/DA_Skill_BigHammer",
    ),
    (
        "IceMist",
        "/Game/GAS_Pattern/AbilitySystem/SkillData/Area/DA_Skill_IceMist",
    ),
    (
        "LightningStrike",
        "/Game/GAS_Pattern/AbilitySystem/SkillData/Area/DA_Skill_LightningStrike",
    ),
    (
        "CrystalTorrent",
        "/Game/GAS_Pattern/AbilitySystem/SkillData/Projectile/DA_Skill_CrystalTorrent",
    ),
    (
        "DarkSoloProjectile",
        "/Game/GAS_Pattern/AbilitySystem/SkillData/Projectile/DA_Skill_DarkSoloProjectile",
    ),
    (
        "DarkStone",
        "/Game/GAS_Pattern/AbilitySystem/SkillData/Projectile/DA_Skill_DarkStone",
    ),
    (
        "MagmaShot",
        "/Game/GAS_Pattern/AbilitySystem/SkillData/Projectile/DA_Skill_MagmaShot",
    ),
)

PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"
EXPECTED_PNG_SIZE = 256
errors: list[str] = []


def log(message: str) -> None:
    mode = "[DRY-RUN]" if DRY_RUN else ""
    unreal.log(f"{LOG_PREFIX}{mode} {message}")


def warn(message: str) -> None:
    unreal.log_warning(f"{LOG_PREFIX} {message}")


def record_error(message: str) -> None:
    errors.append(message)
    unreal.log_error(f"{LOG_PREFIX} {message}")


def raise_recorded_errors(stage: str) -> None:
    if not errors:
        return
    summary = "\n".join(f"  - {message}" for message in errors)
    raise RuntimeError(
        f"{LOG_PREFIX} {stage} failed with {len(errors)} error(s):\n{summary}"
    )


def texture_name_for_skill(skill_name: str) -> str:
    return f"T_SkillIcon_Radial_{skill_name}"


def source_file_for_skill(skill_name: str) -> Path:
    return SOURCE_DIRECTORY / f"{texture_name_for_skill(skill_name)}.png"


def texture_asset_path(skill_name: str) -> str:
    return f"{DESTINATION_DIRECTORY}/{texture_name_for_skill(skill_name)}"


def texture_object_path(skill_name: str) -> str:
    asset_path = texture_asset_path(skill_name)
    return f"{asset_path}.{texture_name_for_skill(skill_name)}"


def texture_uasset_file(skill_name: str) -> Path:
    return (
        DESTINATION_FILESYSTEM_DIRECTORY
        / f"{texture_name_for_skill(skill_name)}.uasset"
    )


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


def set_required_property(obj, property_name: str, value) -> bool:
    """Set a required editor property and report whether it changed."""
    try:
        current_value = obj.get_editor_property(property_name)
    except Exception as exc:
        raise RuntimeError(
            f"{describe_object(obj)}: unable to read required property "
            f"'{property_name}': {exc}"
        ) from exc

    try:
        if current_value == value:
            return False
    except Exception:
        # Some reflected wrapper types do not implement Python comparison.
        pass

    try:
        obj.set_editor_property(property_name, value)
    except Exception as exc:
        raise RuntimeError(
            f"{describe_object(obj)}: unable to set required property "
            f"'{property_name}': {exc}"
        ) from exc
    return True


def set_optional_property(obj, property_name: str, value) -> bool:
    try:
        current_value = obj.get_editor_property(property_name)
        if current_value == value:
            return False
        obj.set_editor_property(property_name, value)
        return True
    except Exception as exc:
        warn(
            f"{describe_object(obj)}: optional property "
            f"'{property_name}' skipped: {exc}"
        )
        return False


def should_import(source_file: Path, uasset_file: Path) -> bool:
    if FORCE_REIMPORT or not uasset_file.exists():
        return True
    try:
        return source_file.stat().st_mtime_ns > uasset_file.stat().st_mtime_ns
    except OSError:
        # When timestamps cannot be trusted, explicit reimport is safer.
        return True


def paeth_predictor(left: int, above: int, upper_left: int) -> int:
    prediction = left + above - upper_left
    distance_left = abs(prediction - left)
    distance_above = abs(prediction - above)
    distance_upper_left = abs(prediction - upper_left)
    if distance_left <= distance_above and distance_left <= distance_upper_left:
        return left
    if distance_above <= distance_upper_left:
        return above
    return upper_left


def validate_rgba_png(source_file: Path) -> dict[str, int]:
    """Validate a non-interlaced RGBA8 PNG and inspect its alpha channel."""
    if not source_file.is_file():
        raise RuntimeError(f"Missing PNG source: {source_file}")

    try:
        data = source_file.read_bytes()
    except OSError as exc:
        raise RuntimeError(f"Unable to read PNG source {source_file}: {exc}") from exc

    if not data.startswith(PNG_SIGNATURE):
        raise RuntimeError(f"Not a PNG file or invalid signature: {source_file}")

    offset = len(PNG_SIGNATURE)
    ihdr = None
    idat_parts: list[bytes] = []
    found_iend = False

    while offset < len(data):
        if offset + 12 > len(data):
            raise RuntimeError(
                f"Truncated PNG chunk header at byte {offset}: {source_file}"
            )

        chunk_length = struct.unpack(">I", data[offset : offset + 4])[0]
        chunk_type = data[offset + 4 : offset + 8]
        chunk_data_start = offset + 8
        chunk_data_end = chunk_data_start + chunk_length
        chunk_end = chunk_data_end + 4
        if chunk_end > len(data):
            chunk_name = chunk_type.decode("ascii", errors="replace")
            raise RuntimeError(
                f"Truncated PNG {chunk_name} chunk at byte {offset}: {source_file}"
            )

        chunk_data = data[chunk_data_start:chunk_data_end]
        stored_crc = struct.unpack(">I", data[chunk_data_end:chunk_end])[0]
        calculated_crc = zlib.crc32(chunk_type)
        calculated_crc = zlib.crc32(chunk_data, calculated_crc) & 0xFFFFFFFF
        if stored_crc != calculated_crc:
            chunk_name = chunk_type.decode("ascii", errors="replace")
            raise RuntimeError(
                f"PNG {chunk_name} CRC mismatch at byte {offset}: {source_file}"
            )

        if chunk_type == b"IHDR":
            if ihdr is not None:
                raise RuntimeError(f"PNG contains multiple IHDR chunks: {source_file}")
            ihdr = chunk_data
        elif chunk_type == b"IDAT":
            idat_parts.append(chunk_data)
        elif chunk_type == b"IEND":
            found_iend = True
            offset = chunk_end
            break

        offset = chunk_end

    if ihdr is None or len(ihdr) != 13:
        raise RuntimeError(f"PNG is missing a valid IHDR chunk: {source_file}")
    if not found_iend:
        raise RuntimeError(f"PNG is missing its IEND chunk: {source_file}")
    if not idat_parts:
        raise RuntimeError(f"PNG contains no IDAT image data: {source_file}")

    (
        width,
        height,
        bit_depth,
        color_type,
        compression_method,
        filter_method,
        interlace_method,
    ) = struct.unpack(">IIBBBBB", ihdr)

    if (width, height) != (EXPECTED_PNG_SIZE, EXPECTED_PNG_SIZE):
        raise RuntimeError(
            f"Expected {EXPECTED_PNG_SIZE}x{EXPECTED_PNG_SIZE}, "
            f"got {width}x{height}: {source_file}"
        )
    if bit_depth != 8 or color_type != 6:
        raise RuntimeError(
            f"Expected RGBA8 PNG (bit_depth=8, color_type=6), got "
            f"bit_depth={bit_depth}, color_type={color_type}: {source_file}"
        )
    if compression_method != 0 or filter_method != 0:
        raise RuntimeError(
            f"Unsupported PNG compression/filter method "
            f"({compression_method}/{filter_method}): {source_file}"
        )
    if interlace_method != 0:
        raise RuntimeError(
            f"Interlaced PNGs are not supported by the alpha validator; "
            f"export as non-interlaced RGBA8: {source_file}"
        )

    try:
        filtered_pixels = zlib.decompress(b"".join(idat_parts))
    except zlib.error as exc:
        raise RuntimeError(f"Unable to decompress PNG pixels {source_file}: {exc}") from exc

    bytes_per_pixel = 4
    row_stride = width * bytes_per_pixel
    expected_filtered_size = height * (row_stride + 1)
    if len(filtered_pixels) != expected_filtered_size:
        raise RuntimeError(
            f"Unexpected decompressed PNG size {len(filtered_pixels)}; "
            f"expected {expected_filtered_size}: {source_file}"
        )

    previous_row = bytearray(row_stride)
    cursor = 0
    alpha_min = 255
    alpha_max = 0
    fully_transparent_pixels = 0
    translucent_pixels = 0
    fully_opaque_pixels = 0

    for row_index in range(height):
        filter_type = filtered_pixels[cursor]
        cursor += 1
        row = bytearray(filtered_pixels[cursor : cursor + row_stride])
        cursor += row_stride

        if filter_type > 4:
            raise RuntimeError(
                f"Unsupported PNG row filter {filter_type} on row {row_index}: "
                f"{source_file}"
            )

        for byte_index in range(row_stride):
            left = row[byte_index - bytes_per_pixel] if byte_index >= bytes_per_pixel else 0
            above = previous_row[byte_index]
            upper_left = (
                previous_row[byte_index - bytes_per_pixel]
                if byte_index >= bytes_per_pixel
                else 0
            )

            if filter_type == 1:
                predictor = left
            elif filter_type == 2:
                predictor = above
            elif filter_type == 3:
                predictor = (left + above) // 2
            elif filter_type == 4:
                predictor = paeth_predictor(left, above, upper_left)
            else:
                predictor = 0

            row[byte_index] = (row[byte_index] + predictor) & 0xFF

        for alpha in row[3::bytes_per_pixel]:
            alpha_min = min(alpha_min, alpha)
            alpha_max = max(alpha_max, alpha)
            if alpha == 0:
                fully_transparent_pixels += 1
            elif alpha == 255:
                fully_opaque_pixels += 1
            else:
                translucent_pixels += 1

        previous_row = row

    if alpha_max == 0:
        raise RuntimeError(f"PNG is fully transparent and has no visible icon: {source_file}")
    if alpha_min == 255:
        raise RuntimeError(
            f"PNG has an RGBA channel but no transparent pixels: {source_file}"
        )

    return {
        "width": width,
        "height": height,
        "alpha_min": alpha_min,
        "alpha_max": alpha_max,
        "fully_transparent_pixels": fully_transparent_pixels,
        "translucent_pixels": translucent_pixels,
        "fully_opaque_pixels": fully_opaque_pixels,
    }


def load_required_skill_data(skill_name: str, skill_data_path: str):
    skill_data = unreal.EditorAssetLibrary.load_asset(skill_data_path)
    if skill_data is None:
        raise RuntimeError(
            f"{skill_name}: unable to load SkillData asset: {skill_data_path}"
        )

    reflected_type = getattr(unreal, "GP_SkillData", None)
    if reflected_type is not None and not isinstance(skill_data, reflected_type):
        raise RuntimeError(
            f"{skill_name}: expected GP_SkillData at {skill_data_path}, "
            f"got {type(skill_data).__name__}"
        )

    try:
        skill_data.get_editor_property("skill_icon")
    except Exception as exc:
        raise RuntimeError(
            f"{skill_name}: asset does not expose the required 'skill_icon' "
            f"property: {skill_data_path}: {exc}"
        ) from exc
    return skill_data


def load_existing_target_texture(skill_name: str):
    asset_path = texture_asset_path(skill_name)
    if not unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        return None
    texture = unreal.EditorAssetLibrary.load_asset(asset_path)
    if texture is None:
        raise RuntimeError(
            f"{skill_name}: target asset exists but could not be loaded: {asset_path}"
        )
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError(
            f"{skill_name}: expected Texture2D at {asset_path}, "
            f"got {type(texture).__name__}"
        )
    return texture


def run_import_task(source_file: Path, skill_name: str) -> None:
    asset_name = texture_name_for_skill(skill_name)
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(source_file))
    task.set_editor_property("destination_path", DESTINATION_DIRECTORY)
    task.set_editor_property("destination_name", asset_name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("replace_existing_settings", False)
    # Save after the deterministic UI texture settings are applied below.
    task.set_editor_property("save", False)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    imported_paths = list(task.get_editor_property("imported_object_paths"))
    expected_path = texture_asset_path(skill_name)
    if not unreal.EditorAssetLibrary.does_asset_exist(expected_path):
        raise RuntimeError(
            f"{skill_name}: import did not create '{expected_path}'. "
            f"Imported paths: {imported_paths}"
        )


def configure_ui_texture(texture) -> bool:
    compression = resolve_enum(
        unreal.TextureCompressionSettings,
        ("TC_EDITOR_ICON", "TC_USER_INTERFACE2D"),
    )
    no_mips = resolve_enum(
        unreal.TextureMipGenSettings,
        ("TMGS_NO_MIPMAPS", "NO_MIPMAPS"),
    )
    clamp = resolve_enum(unreal.TextureAddress, ("TA_CLAMP", "CLAMP"))
    ui_group = resolve_enum(unreal.TextureGroup, ("TEXTUREGROUP_UI", "UI"))

    changed = False
    changed |= set_required_property(texture, "compression_settings", compression)
    changed |= set_required_property(texture, "mip_gen_settings", no_mips)
    changed |= set_required_property(texture, "address_x", clamp)
    changed |= set_required_property(texture, "address_y", clamp)
    changed |= set_required_property(texture, "lod_group", ui_group)
    changed |= set_required_property(texture, "srgb", True)
    changed |= set_optional_property(texture, "never_stream", True)
    changed |= set_optional_property(texture, "virtual_texture_streaming", False)
    return changed


def import_and_configure_texture(skill_name: str):
    source_file = source_file_for_skill(skill_name)
    uasset_file = texture_uasset_file(skill_name)
    asset_path = texture_asset_path(skill_name)
    needs_import = should_import(source_file, uasset_file)
    action = "reimport" if uasset_file.exists() else "import"

    if needs_import:
        log(f"{action.capitalize()}ing {source_file.name} -> {asset_path}")
        run_import_task(source_file, skill_name)
    else:
        log(f"Texture source is up to date: {asset_path}")

    texture = unreal.EditorAssetLibrary.load_asset(asset_path)
    if texture is None:
        raise RuntimeError(
            f"{skill_name}: unable to load imported texture: {asset_path}"
        )
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError(
            f"{skill_name}: expected Texture2D at {asset_path}, "
            f"got {type(texture).__name__}"
        )

    settings_changed = configure_ui_texture(texture)
    if needs_import or settings_changed:
        if not unreal.EditorAssetLibrary.save_loaded_asset(
            texture, only_if_is_dirty=False
        ):
            raise RuntimeError(
                f"{skill_name}: unable to save configured texture: {asset_path}"
            )
        log(
            f"Configured and saved {asset_path}: uncompressed RGBA8 UI, "
            "no mips, clamp, UI LOD group, sRGB on"
        )
    else:
        log(f"Texture settings already match: {asset_path}")
    return texture


def describe_reference(reference) -> str:
    if reference is None:
        return "None"

    get_path_name = getattr(reference, "get_path_name", None)
    if callable(get_path_name):
        try:
            return str(get_path_name())
        except Exception:
            pass

    get_asset_path_string = getattr(reference, "get_asset_path_string", None)
    if callable(get_asset_path_string):
        try:
            return str(get_asset_path_string())
        except Exception:
            pass

    get_editor_property = getattr(reference, "get_editor_property", None)
    if callable(get_editor_property):
        for property_name in ("asset_path_name", "asset_path"):
            try:
                value = get_editor_property(property_name)
                if value:
                    return str(value)
            except Exception:
                continue

    return str(reference)


def reference_matches(reference, expected_asset_path: str) -> bool:
    return expected_asset_path in describe_reference(reference).replace("\\", "/")


def set_skill_icon_reference(
    skill_name: str,
    skill_data_path: str,
    skill_data,
    texture,
) -> None:
    expected_asset_path = texture_asset_path(skill_name)
    current_reference = skill_data.get_editor_property("skill_icon")
    if reference_matches(current_reference, expected_asset_path):
        log(f"{skill_name}: SkillIcon already references {expected_asset_path}")
        return

    previous_description = describe_reference(current_reference)
    try:
        skill_data.set_editor_property("skill_icon", texture)
    except Exception as direct_error:
        soft_object_path_type = getattr(unreal, "SoftObjectPath", None)
        if soft_object_path_type is None:
            raise RuntimeError(
                f"{skill_name}: unable to assign Texture2D to TSoftObjectPtr "
                f"SkillIcon and unreal.SoftObjectPath is unavailable: {direct_error}"
            ) from direct_error

        expected_object_path = texture_object_path(skill_name)
        soft_reference = None
        constructor_errors = []
        for constructor in (
            lambda: soft_object_path_type(expected_object_path),
            lambda: soft_object_path_type(asset_path_name=expected_object_path),
        ):
            try:
                soft_reference = constructor()
                break
            except Exception as exc:
                constructor_errors.append(str(exc))

        if soft_reference is None:
            raise RuntimeError(
                f"{skill_name}: unable to construct SoftObjectPath for "
                f"{expected_object_path}. Direct assignment error: {direct_error}; "
                f"constructor errors: {constructor_errors}"
            ) from direct_error

        try:
            skill_data.set_editor_property("skill_icon", soft_reference)
        except Exception as soft_error:
            raise RuntimeError(
                f"{skill_name}: unable to assign SkillIcon as Texture2D or "
                f"SoftObjectPath. Direct error: {direct_error}; "
                f"soft-path error: {soft_error}"
            ) from soft_error

    assigned_reference = skill_data.get_editor_property("skill_icon")
    if not reference_matches(assigned_reference, expected_asset_path):
        raise RuntimeError(
            f"{skill_name}: SkillIcon assignment verification failed. Expected "
            f"{expected_asset_path}, got {describe_reference(assigned_reference)}"
        )

    if not unreal.EditorAssetLibrary.save_loaded_asset(
        skill_data, only_if_is_dirty=False
    ):
        raise RuntimeError(
            f"{skill_name}: unable to save SkillData asset: {skill_data_path}"
        )
    log(
        f"{skill_name}: SkillIcon changed from {previous_description} "
        f"to {expected_asset_path}; saved {skill_data_path}"
    )


def main() -> None:
    log(
        f"Starting. source={SOURCE_DIRECTORY}, "
        f"destination={DESTINATION_DIRECTORY}, "
        f"force_reimport={FORCE_REIMPORT}"
    )

    png_info_by_skill = {}
    skill_data_by_skill = {}
    existing_texture_by_skill = {}

    # Preflight every input and every destination before any import or save.
    for skill_name, skill_data_path in SKILL_SPECS:
        try:
            source_file = source_file_for_skill(skill_name)
            png_info = validate_rgba_png(source_file)
            png_info_by_skill[skill_name] = png_info
            log(
                f"Validated {source_file.name}: "
                f"{png_info['width']}x{png_info['height']} RGBA8, "
                f"alpha={png_info['alpha_min']}..{png_info['alpha_max']}, "
                f"transparent={png_info['fully_transparent_pixels']}, "
                f"translucent={png_info['translucent_pixels']}, "
                f"opaque={png_info['fully_opaque_pixels']}"
            )
        except Exception as exc:
            record_error(f"{skill_name} PNG: {exc}")

        try:
            skill_data_by_skill[skill_name] = load_required_skill_data(
                skill_name, skill_data_path
            )
        except Exception as exc:
            record_error(f"{skill_name} SkillData: {exc}")

        try:
            existing_texture_by_skill[skill_name] = load_existing_target_texture(
                skill_name
            )
        except Exception as exc:
            record_error(f"{skill_name} target texture: {exc}")

    raise_recorded_errors("Preflight; no assets were changed")

    if DRY_RUN:
        for skill_name, skill_data_path in SKILL_SPECS:
            source_file = source_file_for_skill(skill_name)
            uasset_file = texture_uasset_file(skill_name)
            asset_path = texture_asset_path(skill_name)
            needs_import = should_import(source_file, uasset_file)
            if needs_import:
                action = "reimport" if uasset_file.exists() else "import"
                log(f"Would {action} and configure {source_file.name} -> {asset_path}")
            else:
                log(f"Would verify existing UI texture settings: {asset_path}")

            skill_data = skill_data_by_skill[skill_name]
            current_reference = skill_data.get_editor_property("skill_icon")
            if reference_matches(current_reference, asset_path):
                log(f"{skill_name}: SkillIcon already references {asset_path}")
            else:
                log(
                    f"{skill_name}: would change SkillIcon from "
                    f"{describe_reference(current_reference)} to {asset_path} "
                    f"and save {skill_data_path}"
                )

        log("Dry run completed successfully; no assets were changed.")
        return

    textures_by_skill = {}
    for skill_name, _skill_data_path in SKILL_SPECS:
        try:
            textures_by_skill[skill_name] = import_and_configure_texture(skill_name)
        except Exception as exc:
            record_error(f"{skill_name} texture import/configuration: {exc}")

    raise_recorded_errors(
        "Texture import/configuration; SkillData references were not changed"
    )

    for skill_name, skill_data_path in SKILL_SPECS:
        try:
            set_skill_icon_reference(
                skill_name,
                skill_data_path,
                skill_data_by_skill[skill_name],
                textures_by_skill[skill_name],
            )
        except Exception as exc:
            record_error(f"{skill_name} SkillIcon assignment: {exc}")

    raise_recorded_errors("SkillIcon assignment")
    log("Completed successfully. Seven SkillData assets now use radial icons.")


if __name__ == "__main__":
    main()
