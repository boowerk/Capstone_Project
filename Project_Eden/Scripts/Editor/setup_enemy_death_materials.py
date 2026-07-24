"""Create and assign the shared enemy-death material pipeline.

The script is intentionally idempotent:

* Existing master material graphs are never deleted or rebuilt.
* Material instances are refreshed from the current production mesh slots.
* Blueprint defaults are configured only after the matching native helper
  functions have been compiled.

Commandlet examples (the editor must be closed):

    UnrealEditor-Cmd.exe Project_Eden.uproject -run=pythonscript \
        -script=Scripts/Editor/setup_enemy_death_materials.py \
        --materials-only -unattended -nop4 -nosplash -nullrhi

    UnrealEditor-Cmd.exe Project_Eden.uproject -run=pythonscript \
        -script=Scripts/Editor/setup_enemy_death_materials.py \
        -unattended -nop4 -nosplash -nullrhi
"""

from __future__ import annotations

import re
import sys
from dataclasses import dataclass

import unreal


LOG_PREFIX = "[EnemyDeathMaterials]"
MATERIALS_ONLY = (
    "--materials-only" in sys.argv
    or "--materials-only" in unreal.SystemLibrary.get_command_line()
)
ASSIGN_ONLY = (
    "--assign-only" in sys.argv
    or "--assign-only" in unreal.SystemLibrary.get_command_line()
)

ASSET_DIRECTORY = "/Game/Niagara/Dissolve_SK/EnemyMaterials"
BODY_MASTER_PATH = f"{ASSET_DIRECTORY}/M_EnemyDeath_Dissolve"
PARTICLE_MASTER_PATH = f"{ASSET_DIRECTORY}/M_EnemyDeath_AbsorbParticle"
DEFAULT_BODY_INSTANCE_PATH = f"{ASSET_DIRECTORY}/MI_EnemyDeath_Default"
DEFAULT_PARTICLE_INSTANCE_PATH = f"{ASSET_DIRECTORY}/MI_EnemyDeathParticle_Default"

WHITE_TEXTURE_PATH = "/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture"
NOISE_TEXTURE_PATH = (
    "/Engine/EngineMaterials/Good64x64TilingNoiseHighFreq."
    "Good64x64TilingNoiseHighFreq"
)


@dataclass(frozen=True)
class EnemyMaterialProfile:
    label: str
    blueprint_path: str
    tint: unreal.LinearColor
    edge: unreal.LinearColor
    metallic: float = 0.1
    roughness: float = 0.48
    is_boss: bool = False
    fragment_source_material: str | None = None
    # An empty entry intentionally uses the white fallback. Missing entries use
    # source-material parameter inspection.
    slot_base_texture_paths: tuple[str, ...] = ()


PROFILES = (
    EnemyMaterialProfile(
        "FurnaceWalker",
        "/Game/Characters/EnemyCharacter/Monsters/FurnaceWalker/BP_FurnaceWalker",
        unreal.LinearColor(0.18, 0.035, 0.008, 1.0),
        unreal.LinearColor(1.0, 0.16, 0.01, 1.0),
        metallic=0.22,
        roughness=0.42,
        slot_base_texture_paths=(
            "/Game/Characters/EnemyCharacter/Monsters/FurnaceWalker/FurnaceWalker_0_Diffuse_nonVT",
            "/Game/Characters/EnemyCharacter/Monsters/FurnaceWalker/FurnaceWalker_0_Diffuse_nonVT",
            "/Game/Characters/EnemyCharacter/Monsters/FurnaceWalker/FurnaceWalker_0_Diffuse3",
            "/Game/Fab/Lava_Material/Textures/T_Lava_01/T_Lava_01_emissive",
            "/Game/Characters/EnemyCharacter/Monsters/FurnaceWalker/FurnaceWalker_0_Diffuse3",
            "/Game/Characters/EnemyCharacter/Monsters/FurnaceWalker/FurnaceWalker_0_Diffuse_nonVT",
        ),
    ),
    EnemyMaterialProfile(
        "FurnaceStomper",
        "/Game/Characters/EnemyCharacter/Monsters/FurnaceStomper/BP_FurnaceStomper",
        unreal.LinearColor(0.24, 0.055, 0.01, 1.0),
        unreal.LinearColor(1.0, 0.30, 0.015, 1.0),
        metallic=0.18,
        roughness=0.46,
        slot_base_texture_paths=(
            "/Game/Characters/EnemyCharacter/Monsters/FurnaceStomper/FurnaceStomper_basecolor",
        )
        * 4,
    ),
    EnemyMaterialProfile(
        "CyclopsSpecter",
        "/Game/Characters/EnemyCharacter/Monsters/CyclopsSpecter/BP_CyclopsSpecter",
        unreal.LinearColor(0.055, 0.018, 0.18, 1.0),
        unreal.LinearColor(0.42, 0.08, 1.0, 1.0),
        roughness=0.38,
        slot_base_texture_paths=(
            "/Game/Characters/EnemyCharacter/Monsters/CyclopsSpecter/CyclopsSpecter_fbx_basecolor",
        ),
    ),
    EnemyMaterialProfile(
        "CrystalSeraph",
        "/Game/Characters/EnemyCharacter/Boss/BP_Boss_CrystalSeraph/BP_Crystal_Seraph",
        unreal.LinearColor(0.0999, 0.4179, 1.0, 1.0),
        unreal.LinearColor(0.35, 0.78, 1.0, 1.0),
        metallic=0.08,
        roughness=0.24,
        is_boss=True,
        fragment_source_material=(
            "/Game/Characters/EnemyCharacter/Boss/BP_Boss_CrystalSeraph/"
            "Materials/MI_CrystalSeraph_Boss"
        ),
        slot_base_texture_paths=("",),
    ),
    EnemyMaterialProfile(
        "DarkArmorKnight",
        "/Game/Characters/EnemyCharacter/Boss/BP_Boss_DarkArmorKnight/BP_DarkArmorKnight",
        unreal.LinearColor(0.008, 0.005, 0.018, 1.0),
        unreal.LinearColor(0.32, 0.025, 0.72, 1.0),
        metallic=0.68,
        roughness=0.31,
        is_boss=True,
        fragment_source_material=(
            "/Game/Characters/EnemyCharacter/Boss/BP_Boss_DarkArmorKnight/MI_ArmorPlate"
        ),
        slot_base_texture_paths=(
            "/Game/Characters/EnemyCharacter/Boss/BP_Boss_DarkArmorKnight/tripo_rgb_9859e95d-277d-46a8-9556-b85d91575d77_nonVT",
            "/Game/Characters/EnemyCharacter/Boss/BP_Boss_DarkArmorKnight/MI_ArmorPlate_BaseColor_nonVT",
            "/Game/Characters/EnemyCharacter/Boss/BP_Boss_DarkArmorKnight/MI_ArmorPlate_BaseColor1_nonVT",
        ),
    ),
    EnemyMaterialProfile(
        "Matador",
        "/Game/Characters/EnemyCharacter/Boss/BP_Boss_Matador/BP_Boss_Matador",
        unreal.LinearColor(0.28, 0.012, 0.006, 1.0),
        unreal.LinearColor(1.0, 0.075, 0.018, 1.0),
        roughness=0.56,
        is_boss=True,
        slot_base_texture_paths=(
            "/Game/Characters/MaskMan/Textures/MI_MaskManFace_Diffuse_nonVT",
            "/Game/Characters/MaskMan/Textures/MI_MaskManBody_Diffuse_nonVT",
        ),
    ),
    EnemyMaterialProfile(
        "Sans",
        "/Game/Characters/EnemyCharacter/Boss/BP_Boss_Sans/BP_Boss_Sans",
        unreal.LinearColor(0.12, 0.12, 0.17, 1.0),
        unreal.LinearColor(0.52, 0.10, 1.0, 1.0),
        roughness=0.36,
        is_boss=True,
        fragment_source_material="/Game/Niagara/Examples/MI_FresnelExample",
        slot_base_texture_paths=("",),
    ),
)


def log(message: str) -> None:
    unreal.log(f"{LOG_PREFIX} {message}")


def expression(material, expression_class, x: int, y: int):
    return unreal.MaterialEditingLibrary.create_material_expression(
        material, expression_class, x, y
    )


def connect(source, output_name: str, target, input_name: str) -> None:
    if not unreal.MaterialEditingLibrary.connect_material_expressions(
        source, output_name, target, input_name
    ):
        raise RuntimeError(
            f"Unable to connect {source.get_name()}:{output_name} "
            f"to {target.get_name()}:{input_name}"
        )


def connect_property(source, output_name: str, material_property) -> None:
    if not unreal.MaterialEditingLibrary.connect_material_property(
        source, output_name, material_property
    ):
        raise RuntimeError(
            f"Unable to connect {source.get_name()} to {material_property}"
        )


def scalar(material, name: str, value: float, x: int, y: int):
    node = expression(material, unreal.MaterialExpressionScalarParameter, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", value)
    return node


def vector(material, name: str, value: unreal.LinearColor, x: int, y: int):
    node = expression(material, unreal.MaterialExpressionVectorParameter, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", value)
    return node


def constant(material, value: float, x: int, y: int):
    node = expression(material, unreal.MaterialExpressionConstant, x, y)
    node.set_editor_property("r", value)
    return node


def create_material_asset(asset_path: str):
    package_path, asset_name = asset_path.rsplit("/", 1)
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        asset_name,
        package_path,
        unreal.Material,
        unreal.MaterialFactoryNew(),
    )
    if material is None:
        raise RuntimeError(f"Unable to create material: {asset_path}")
    return material


def set_usage_flag(material, property_name: str) -> None:
    try:
        material.set_editor_property(property_name, True)
    except Exception as exc:  # Engine builds expose different usage flags.
        unreal.log_warning(
            f"{LOG_PREFIX} Could not set {property_name} on "
            f"{material.get_path_name()}: {exc}"
        )


def build_body_master():
    existing = unreal.load_asset(BODY_MASTER_PATH)
    if existing is not None:
        return existing

    material = create_material_asset(BODY_MASTER_PATH)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    material.set_editor_property("two_sided", True)
    material.set_editor_property("opacity_mask_clip_value", 0.333)
    set_usage_flag(material, "used_with_skeletal_mesh")

    white_texture = unreal.load_asset(WHITE_TEXTURE_PATH)
    noise_texture = unreal.load_asset(NOISE_TEXTURE_PATH)
    if white_texture is None or noise_texture is None:
        raise RuntimeError("Required engine death-material textures are missing")

    tex_coord = expression(
        material, unreal.MaterialExpressionTextureCoordinate, -1500, -180
    )
    base_texture = expression(
        material, unreal.MaterialExpressionTextureSampleParameter2D, -1240, -360
    )
    base_texture.set_editor_property("parameter_name", "BaseTexture")
    base_texture.set_editor_property("texture", white_texture)
    connect(tex_coord, "", base_texture, "UVs")

    death_tint = vector(
        material,
        "DeathTint",
        unreal.LinearColor(0.25, 0.42, 0.65, 1.0),
        -980,
        -500,
    )
    tinted_base = expression(material, unreal.MaterialExpressionMultiply, -700, -360)
    connect(base_texture, "RGB", tinted_base, "A")
    connect(death_tint, "RGB", tinted_base, "B")

    noise_tiling = scalar(material, "NoiseTiling", 5.0, -1500, 80)
    tiled_uv = expression(material, unreal.MaterialExpressionMultiply, -1240, 40)
    connect(tex_coord, "", tiled_uv, "A")
    connect(noise_tiling, "", tiled_uv, "B")
    noise_sample = expression(
        material, unreal.MaterialExpressionTextureSampleParameter2D, -980, 40
    )
    noise_sample.set_editor_property("parameter_name", "DissolveNoise")
    noise_sample.set_editor_property("texture", noise_texture)
    connect(tiled_uv, "", noise_sample, "UVs")

    dissolve_progress = scalar(
        material, "DissolveProgress", 0.0, -980, 260
    )
    threshold_scale = constant(material, 1.10, -760, 300)
    scaled_progress = expression(
        material, unreal.MaterialExpressionMultiply, -560, 250
    )
    connect(dissolve_progress, "", scaled_progress, "A")
    connect(threshold_scale, "", scaled_progress, "B")
    threshold_bias = constant(material, 0.05, -560, 390)
    threshold = expression(material, unreal.MaterialExpressionSubtract, -340, 250)
    connect(scaled_progress, "", threshold, "A")
    connect(threshold_bias, "", threshold, "B")

    one = constant(material, 1.0, -340, 430)
    zero = constant(material, 0.0, -340, 500)
    visible_mask = expression(material, unreal.MaterialExpressionIf, -80, 210)
    connect(noise_sample, "R", visible_mask, "A")
    connect(threshold, "", visible_mask, "B")
    connect(one, "", visible_mask, "A > B")
    connect(zero, "", visible_mask, "A < B")

    edge_delta = expression(material, unreal.MaterialExpressionSubtract, -80, 20)
    connect(noise_sample, "R", edge_delta, "A")
    connect(threshold, "", edge_delta, "B")
    edge_abs = expression(material, unreal.MaterialExpressionAbs, 130, 20)
    connect(edge_delta, "", edge_abs, "")
    edge_width = scalar(material, "EdgeWidth", 0.075, 130, 110)
    edge_normalized = expression(material, unreal.MaterialExpressionDivide, 340, 20)
    connect(edge_abs, "", edge_normalized, "A")
    connect(edge_width, "", edge_normalized, "B")
    edge_inverse = expression(material, unreal.MaterialExpressionOneMinus, 540, 20)
    connect(edge_normalized, "", edge_inverse, "")
    edge_saturated = expression(material, unreal.MaterialExpressionSaturate, 730, 20)
    connect(edge_inverse, "", edge_saturated, "")

    edge_gate_scale = constant(material, 10.0, 130, 240)
    edge_gate_product = expression(
        material, unreal.MaterialExpressionMultiply, 340, 210
    )
    connect(dissolve_progress, "", edge_gate_product, "A")
    connect(edge_gate_scale, "", edge_gate_product, "B")
    edge_gate = expression(material, unreal.MaterialExpressionSaturate, 540, 210)
    connect(edge_gate_product, "", edge_gate, "")
    gated_edge = expression(material, unreal.MaterialExpressionMultiply, 940, 60)
    connect(edge_saturated, "", gated_edge, "A")
    connect(edge_gate, "", gated_edge, "B")

    edge_color = vector(
        material,
        "EdgeColor",
        unreal.LinearColor(0.35, 0.78, 1.0, 1.0),
        520,
        -250,
    )
    edge_intensity = scalar(material, "EdgeIntensity", 8.0, 520, -130)
    bright_edge = expression(material, unreal.MaterialExpressionMultiply, 750, -220)
    connect(edge_color, "RGB", bright_edge, "A")
    connect(edge_intensity, "", bright_edge, "B")
    emissive = expression(material, unreal.MaterialExpressionMultiply, 1160, -100)
    connect(bright_edge, "", emissive, "A")
    connect(gated_edge, "", emissive, "B")

    roughness = scalar(material, "Roughness", 0.48, 980, -390)
    metallic = scalar(material, "Metallic", 0.10, 980, -310)

    connect_property(tinted_base, "", unreal.MaterialProperty.MP_BASE_COLOR)
    connect_property(emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    connect_property(visible_mask, "", unreal.MaterialProperty.MP_OPACITY_MASK)
    connect_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
    connect_property(metallic, "", unreal.MaterialProperty.MP_METALLIC)

    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
    log(f"Created body master {BODY_MASTER_PATH}")
    return material


def build_particle_master():
    existing = unreal.load_asset(PARTICLE_MASTER_PATH)
    if existing is not None:
        return existing

    material = create_material_asset(PARTICLE_MASTER_PATH)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    material.set_editor_property("two_sided", True)
    material.set_editor_property("opacity_mask_clip_value", 0.25)
    set_usage_flag(material, "used_with_niagara_sprites")

    particle_color = expression(
        material, unreal.MaterialExpressionParticleColor, -850, -160
    )
    death_tint = vector(
        material,
        "DeathTint",
        unreal.LinearColor(0.25, 0.42, 0.65, 1.0),
        -850,
        -360,
    )
    tinted_particle = expression(
        material, unreal.MaterialExpressionMultiply, -600, -250
    )
    connect(particle_color, "RGB", tinted_particle, "A")
    connect(death_tint, "RGB", tinted_particle, "B")
    brightness = scalar(material, "ParticleBrightness", 2.5, -600, -80)
    emissive = expression(material, unreal.MaterialExpressionMultiply, -350, -250)
    connect(tinted_particle, "", emissive, "A")
    connect(brightness, "", emissive, "B")

    tex_coord = expression(
        material, unreal.MaterialExpressionTextureCoordinate, -850, 120
    )
    center = expression(
        material, unreal.MaterialExpressionConstant2Vector, -850, 250
    )
    center.set_editor_property("r", 0.5)
    center.set_editor_property("g", 0.5)
    centered_uv = expression(material, unreal.MaterialExpressionSubtract, -600, 140)
    connect(tex_coord, "", centered_uv, "A")
    connect(center, "", centered_uv, "B")
    radial_distance = expression(material, unreal.MaterialExpressionLength, -370, 140)
    connect(centered_uv, "", radial_distance, "")
    radius = scalar(material, "ParticleRadius", 0.52, -370, 260)
    one = constant(material, 1.0, -370, 350)
    zero = constant(material, 0.0, -370, 420)
    circle_mask = expression(material, unreal.MaterialExpressionIf, -120, 160)
    connect(radius, "", circle_mask, "A")
    connect(radial_distance, "", circle_mask, "B")
    connect(one, "", circle_mask, "A > B")
    connect(zero, "", circle_mask, "A < B")
    opacity_mask = expression(material, unreal.MaterialExpressionMultiply, 100, 160)
    connect(circle_mask, "", opacity_mask, "A")
    connect(particle_color, "A", opacity_mask, "B")

    connect_property(tinted_particle, "", unreal.MaterialProperty.MP_BASE_COLOR)
    connect_property(emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    connect_property(opacity_mask, "", unreal.MaterialProperty.MP_OPACITY_MASK)

    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
    log(f"Created particle master {PARTICLE_MASTER_PATH}")
    return material


def get_or_create_instance(asset_path: str, parent):
    instance = unreal.load_asset(asset_path)
    if instance is None:
        package_path, asset_name = asset_path.rsplit("/", 1)
        instance = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            asset_name,
            package_path,
            unreal.MaterialInstanceConstant,
            unreal.MaterialInstanceConstantFactoryNew(),
        )
        if instance is None:
            raise RuntimeError(f"Unable to create material instance: {asset_path}")
    instance.set_editor_property("parent", parent)
    return instance


def set_instance_values(
    instance,
    tint: unreal.LinearColor,
    edge: unreal.LinearColor,
    *,
    texture=None,
    metallic: float = 0.1,
    roughness: float = 0.48,
    edge_intensity: float = 8.0,
) -> None:
    resolved_texture = texture or unreal.load_asset(WHITE_TEXTURE_PATH)
    if resolved_texture is None:
        raise RuntimeError("White fallback texture is missing")
    unreal.MaterialEditingLibrary.set_material_instance_texture_parameter_value(
        instance, "BaseTexture", resolved_texture
    )
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
        instance, "DeathTint", tint
    )
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
        instance, "EdgeColor", edge
    )
    for name, value in (
        ("Metallic", metallic),
        ("Roughness", roughness),
        ("EdgeIntensity", edge_intensity),
        ("EdgeWidth", 0.075),
        ("NoiseTiling", 5.0),
    ):
        unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(
            instance, name, value
        )
    unreal.MaterialEditingLibrary.update_material_instance(instance)
    unreal.EditorAssetLibrary.save_loaded_asset(instance, only_if_is_dirty=False)


def choose_base_texture(source_material):
    if source_material is None:
        return None
    try:
        parameter_names = list(
            unreal.MaterialEditingLibrary.get_texture_parameter_names(source_material)
        )
    except Exception:
        return None

    candidates = []
    for parameter_name in parameter_names:
        name = str(parameter_name)
        lowered = name.lower()
        if any(
            token in lowered
            for token in (
                "normal",
                "rough",
                "metal",
                "mask",
                "orm",
                "opacity",
                "height",
                "noise",
                "emissive",
            )
        ):
            continue
        score = 0
        if "basecolor" in lowered or "base_color" in lowered:
            score += 100
        if "albedo" in lowered or "diffuse" in lowered:
            score += 80
        if "color" in lowered:
            score += 30
        if "texture" in lowered or "map" in lowered:
            score += 10
        try:
            if isinstance(source_material, unreal.MaterialInstanceConstant):
                texture = (
                    unreal.MaterialEditingLibrary
                    .get_material_instance_texture_parameter_value(
                        source_material, parameter_name
                    )
                )
            elif isinstance(source_material, unreal.Material):
                texture = (
                    unreal.MaterialEditingLibrary
                    .get_material_default_texture_parameter_value(
                        source_material, parameter_name
                    )
                )
            else:
                texture = None
        except Exception:
            texture = None
        if texture is not None:
            if texture.get_path_name().startswith(
                "/Engine/EngineMaterials/BaseFlatten"
            ):
                continue
            candidates.append((score, name, texture))

    if not candidates:
        return None
    candidates.sort(key=lambda item: (-item[0], item[1].lower()))
    return candidates[0][2]


def load_blueprint_cdo(blueprint_path: str):
    blueprint = unreal.load_asset(blueprint_path)
    if blueprint is None:
        raise RuntimeError(f"Blueprint is missing: {blueprint_path}")
    generated_class = blueprint.generated_class()
    if generated_class is None:
        raise RuntimeError(f"Blueprint has no generated class: {blueprint_path}")
    return blueprint, unreal.get_default_object(generated_class)


def find_main_mesh(default_actor):
    try:
        mesh = default_actor.get_editor_property("mesh")
        if mesh is not None:
            return mesh
    except Exception:
        pass
    components = list(
        default_actor.get_components_by_class(unreal.SkeletalMeshComponent)
    )
    for component in components:
        if component.get_name() == "CharacterMesh0":
            return component
    return components[0] if components else None


def safe_asset_name(value: str) -> str:
    return re.sub(r"[^A-Za-z0-9_]+", "_", value).strip("_")


def create_profile_assets(profile: EnemyMaterialProfile, body_master, particle_master):
    blueprint, default_actor = load_blueprint_cdo(profile.blueprint_path)
    mesh = find_main_mesh(default_actor)
    if mesh is None:
        raise RuntimeError(f"Main skeletal mesh is missing: {profile.blueprint_path}")

    slot_materials = []
    slot_count = max(1, mesh.get_num_materials())
    for slot_index in range(slot_count):
        source_material = mesh.get_material(slot_index)
        if slot_index < len(profile.slot_base_texture_paths):
            explicit_texture_path = profile.slot_base_texture_paths[slot_index]
            source_texture = (
                unreal.load_asset(explicit_texture_path)
                if explicit_texture_path
                else None
            )
            if explicit_texture_path and source_texture is None:
                raise RuntimeError(
                    f"Explicit base texture is missing: {explicit_texture_path}"
                )
        else:
            source_texture = choose_base_texture(source_material)
        instance_path = (
            f"{ASSET_DIRECTORY}/MI_EnemyDeath_{safe_asset_name(profile.label)}"
            f"_Slot{slot_index:02d}"
        )
        instance = get_or_create_instance(instance_path, body_master)
        set_instance_values(
            instance,
            profile.tint,
            profile.edge,
            texture=source_texture,
            metallic=profile.metallic,
            roughness=profile.roughness,
            edge_intensity=9.0 if profile.is_boss else 7.5,
        )
        slot_materials.append(instance)
        log(
            f"{profile.label} slot {slot_index}: "
            f"{source_material.get_path_name() if source_material else 'None'} -> "
            f"{instance_path}; texture="
            f"{source_texture.get_path_name() if source_texture else 'white'}"
        )

    auxiliary_path = (
        f"{ASSET_DIRECTORY}/MI_EnemyDeath_{safe_asset_name(profile.label)}"
        "_Auxiliary"
    )
    auxiliary_instance = get_or_create_instance(auxiliary_path, body_master)
    set_instance_values(
        auxiliary_instance,
        profile.tint,
        profile.edge,
        metallic=profile.metallic,
        roughness=profile.roughness,
        edge_intensity=9.0 if profile.is_boss else 7.5,
    )

    particle_path = (
        f"{ASSET_DIRECTORY}/MI_EnemyDeathParticle_{safe_asset_name(profile.label)}"
    )
    particle_instance = get_or_create_instance(particle_path, particle_master)
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
        particle_instance, "DeathTint", profile.edge
    )
    unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(
        particle_instance,
        "ParticleBrightness",
        3.25 if profile.is_boss else 2.5,
    )
    unreal.MaterialEditingLibrary.update_material_instance(particle_instance)
    unreal.EditorAssetLibrary.save_loaded_asset(
        particle_instance, only_if_is_dirty=False
    )

    fragment_instance = None
    if profile.is_boss:
        source_fragment_material = (
            unreal.load_asset(profile.fragment_source_material)
            if profile.fragment_source_material
            else None
        )
        fragment_texture = choose_base_texture(source_fragment_material)
        fragment_path = (
            f"{ASSET_DIRECTORY}/MI_BossDeathFragment_"
            f"{safe_asset_name(profile.label)}"
        )
        fragment_instance = get_or_create_instance(fragment_path, body_master)
        set_instance_values(
            fragment_instance,
            profile.tint,
            profile.edge,
            texture=fragment_texture,
            metallic=profile.metallic,
            roughness=profile.roughness,
            edge_intensity=11.0,
        )

    return (
        blueprint,
        default_actor,
        auxiliary_instance,
        slot_materials,
        particle_instance,
        fragment_instance,
    )


def find_component(default_actor, property_name: str, component_name: str):
    try:
        component = default_actor.get_editor_property(property_name)
        if component is not None:
            return component
    except Exception:
        pass
    for component in default_actor.get_components_by_class(unreal.ActorComponent):
        if component.get_name() == component_name:
            return component
    return None


def configure_profile_blueprint(
    blueprint,
    default_actor,
    auxiliary_instance,
    slot_materials,
    particle_instance,
    fragment_instance,
    profile: EnemyMaterialProfile,
) -> None:
    absorption_component = find_component(
        default_actor,
        "enemy_death_absorption_component",
        "EnemyDeathAbsorptionComponent",
    )
    if absorption_component is None:
        raise RuntimeError(
            f"Death absorption component is missing: {profile.blueprint_path}"
        )
    configure_death_materials = getattr(
        absorption_component, "configure_death_materials", None
    )
    if not callable(configure_death_materials):
        raise RuntimeError(
            "Native ConfigureDeathMaterials is unavailable. Build Project_EdenEditor "
            "before running the assignment pass."
        )

    blueprint.modify()
    absorption_component.modify()
    configure_death_materials(
        auxiliary_instance, slot_materials, particle_instance
    )

    if profile.is_boss:
        presentation_component = find_component(
            default_actor,
            "boss_death_presentation_component",
            "BossDeathPresentationComponent",
        )
        if presentation_component is None:
            raise RuntimeError(
                f"Boss presentation component is missing: {profile.blueprint_path}"
            )
        configure_fragment_material = getattr(
            presentation_component, "configure_fragment_material", None
        )
        if not callable(configure_fragment_material):
            raise RuntimeError(
                "Native ConfigureFragmentMaterial is unavailable. "
                "Build Project_EdenEditor before running the assignment pass."
            )
        presentation_component.modify()
        # Death absorption owns the source-mesh dissolve/hide lifecycle.
        configure_fragment_material(fragment_instance, False)

    if not unreal.EditorAssetLibrary.save_loaded_asset(
        blueprint, only_if_is_dirty=False
    ):
        raise RuntimeError(f"Unable to save Blueprint: {profile.blueprint_path}")
    log(f"Configured {profile.blueprint_path}")


def load_profile_assignment_assets(profile: EnemyMaterialProfile):
    blueprint, default_actor = load_blueprint_cdo(profile.blueprint_path)
    mesh = find_main_mesh(default_actor)
    if mesh is None:
        raise RuntimeError(f"Main skeletal mesh is missing: {profile.blueprint_path}")

    slot_materials = []
    for slot_index in range(max(1, mesh.get_num_materials())):
        instance_path = (
            f"{ASSET_DIRECTORY}/MI_EnemyDeath_{safe_asset_name(profile.label)}"
            f"_Slot{slot_index:02d}"
        )
        instance = unreal.load_asset(instance_path)
        if instance is None:
            raise RuntimeError(f"Prepared slot material is missing: {instance_path}")
        slot_materials.append(instance)

    auxiliary_path = (
        f"{ASSET_DIRECTORY}/MI_EnemyDeath_{safe_asset_name(profile.label)}"
        "_Auxiliary"
    )
    auxiliary_instance = unreal.load_asset(auxiliary_path)
    if auxiliary_instance is None:
        raise RuntimeError(
            f"Prepared auxiliary material is missing: {auxiliary_path}"
        )

    particle_path = (
        f"{ASSET_DIRECTORY}/MI_EnemyDeathParticle_{safe_asset_name(profile.label)}"
    )
    particle_instance = unreal.load_asset(particle_path)
    if particle_instance is None:
        raise RuntimeError(f"Prepared particle material is missing: {particle_path}")

    fragment_instance = None
    if profile.is_boss:
        fragment_path = (
            f"{ASSET_DIRECTORY}/MI_BossDeathFragment_"
            f"{safe_asset_name(profile.label)}"
        )
        fragment_instance = unreal.load_asset(fragment_path)
        if fragment_instance is None:
            raise RuntimeError(f"Prepared fragment material is missing: {fragment_path}")

    return (
        blueprint,
        default_actor,
        auxiliary_instance,
        slot_materials,
        particle_instance,
        fragment_instance,
    )


def create_default_instances(body_master, particle_master) -> None:
    default_body = get_or_create_instance(DEFAULT_BODY_INSTANCE_PATH, body_master)
    set_instance_values(
        default_body,
        unreal.LinearColor(0.18, 0.32, 0.48, 1.0),
        unreal.LinearColor(0.38, 0.78, 1.0, 1.0),
    )

    default_particle = get_or_create_instance(
        DEFAULT_PARTICLE_INSTANCE_PATH, particle_master
    )
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
        default_particle,
        "DeathTint",
        unreal.LinearColor(0.38, 0.78, 1.0, 1.0),
    )
    unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(
        default_particle, "ParticleBrightness", 2.5
    )
    unreal.MaterialEditingLibrary.update_material_instance(default_particle)
    unreal.EditorAssetLibrary.save_loaded_asset(
        default_particle, only_if_is_dirty=False
    )


def configure_niagara_material_binding() -> None:
    system = unreal.load_asset(
        "/Game/Niagara/Dissolve_SK/NS_EnemyDeath_Absorb"
    )
    if system is None:
        raise RuntimeError("NS_EnemyDeath_Absorb is missing")
    # UGP_DeathVFXSetupLibrary keeps the project prefix separated in C++.
    # Python name normalization differs between engine versions, so accept both
    # forms while still requiring the exact native function.
    library = getattr(unreal, "GPDeathVFXSetupLibrary", None)
    if library is None:
        library = getattr(unreal, "GP_DeathVFXSetupLibrary", None)
    configure_binding = (
        getattr(library, "configure_absorption_material_binding", None)
        if library is not None
        else None
    )
    if not callable(configure_binding):
        raise RuntimeError(
            "Native ConfigureAbsorptionMaterialBinding is unavailable. "
            "Build Project_EdenEditor before running the assignment pass."
        )
    if not configure_binding(system, "User.DeathParticleMaterial"):
        raise RuntimeError("No Sprite Renderer was configured on the absorption system")
    if not unreal.EditorAssetLibrary.save_loaded_asset(
        system, only_if_is_dirty=False
    ):
        raise RuntimeError("Unable to save NS_EnemyDeath_Absorb")
    log("Configured NS_EnemyDeath_Absorb User.DeathParticleMaterial binding")


def main() -> None:
    if MATERIALS_ONLY and ASSIGN_ONLY:
        raise RuntimeError("--materials-only and --assign-only are mutually exclusive")

    if ASSIGN_ONLY:
        configure_niagara_material_binding()
        for profile in PROFILES:
            (
                blueprint,
                default_actor,
                auxiliary_instance,
                slot_materials,
                particle_instance,
                fragment_instance,
            ) = load_profile_assignment_assets(profile)
            configure_profile_blueprint(
                blueprint,
                default_actor,
                auxiliary_instance,
                slot_materials,
                particle_instance,
                fragment_instance,
                profile,
            )
        log("Enemy death Blueprint assignment completed successfully")
        return

    unreal.EditorAssetLibrary.make_directory(ASSET_DIRECTORY)
    body_master = build_body_master()
    particle_master = build_particle_master()
    create_default_instances(body_master, particle_master)

    for profile in PROFILES:
        prepared_profile = create_profile_assets(
            profile, body_master, particle_master
        )
        if not MATERIALS_ONLY:
            configure_profile_blueprint(
                *prepared_profile,
                profile,
            )

    if MATERIALS_ONLY:
        log("Material-only pass completed")
        return

    configure_niagara_material_binding()
    log("Enemy death material setup completed successfully")


main()
