"""Create the deferred-decal material used by the Sans sweep warning fan."""

import math
import unreal


ASSET_DIRECTORY = "/Game/Effects"
ASSET_NAME = "M_BossSweepTelegraph_Decal"
ASSET_PATH = f"{ASSET_DIRECTORY}/{ASSET_NAME}"


def expression(material, expression_class, x, y):
    """Create one positioned expression so the generated graph remains readable."""
    return unreal.MaterialEditingLibrary.create_material_expression(
        material, expression_class, x, y
    )


def connect(source, output_name, target, input_name):
    """Fail immediately when an engine version changes a required material pin."""
    if not unreal.MaterialEditingLibrary.connect_material_expressions(
        source, output_name, target, input_name
    ):
        raise RuntimeError(f"Unable to connect material pin: {input_name}")


def build_sweep_decal_material():
    """Build a radius-and-forward-angle mask inside one square decal projection."""
    material = unreal.EditorAssetLibrary.load_asset(ASSET_PATH)
    if material is not None:
        # UE 5.7 cannot safely garbage-collect rooted material expressions in commandlets; preserve the authored asset.
        unreal.log(f"Sweep telegraph decal material already exists: {ASSET_PATH}")
        return
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        ASSET_NAME,
        ASSET_DIRECTORY,
        unreal.Material,
        unreal.MaterialFactoryNew(),
    )
    if material is None:
        raise RuntimeError(f"Unable to create sweep decal material: {ASSET_PATH}")

    material.set_editor_property("material_domain", unreal.MaterialDomain.MD_DEFERRED_DECAL)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)

    tex_coord = expression(material, unreal.MaterialExpressionTextureCoordinate, -1300, -80)
    center = expression(material, unreal.MaterialExpressionConstant2Vector, -1300, 80)
    center.set_editor_property("r", 0.5)
    center.set_editor_property("g", 0.5)
    centered_uv = expression(material, unreal.MaterialExpressionSubtract, -1080, -80)
    connect(tex_coord, "", centered_uv, "A")
    connect(center, "", centered_uv, "B")

    uv_length = expression(material, unreal.MaterialExpressionLength, -860, -180)
    normalized_uv = expression(material, unreal.MaterialExpressionNormalize, -860, 20)
    connect(centered_uv, "", uv_length, "")
    connect(centered_uv, "", normalized_uv, "")

    radius = expression(material, unreal.MaterialExpressionScalarParameter, -860, -340)
    radius.set_editor_property("parameter_name", "NormalizedRadius")
    radius.set_editor_property("default_value", 0.5)
    forward = expression(material, unreal.MaterialExpressionConstant2Vector, -860, 180)
    # DeferredDecal UV.y comes from local +Y; the component's yaw correction maps it to actor-forward.
    forward.set_editor_property("r", 0.0)
    forward.set_editor_property("g", 1.0)
    forward_dot = expression(material, unreal.MaterialExpressionDotProduct, -640, 40)
    connect(normalized_uv, "", forward_dot, "A")
    connect(forward, "", forward_dot, "B")

    half_angle_cos = expression(material, unreal.MaterialExpressionScalarParameter, -640, 200)
    half_angle_cos.set_editor_property("parameter_name", "HalfAngleCos")
    half_angle_cos.set_editor_property("default_value", math.cos(math.radians(82.5)))

    one = expression(material, unreal.MaterialExpressionConstant, -640, 360)
    one.set_editor_property("r", 1.0)
    zero = expression(material, unreal.MaterialExpressionConstant, -640, 460)
    zero.set_editor_property("r", 0.0)

    radius_mask = expression(material, unreal.MaterialExpressionIf, -400, -220)
    connect(radius, "", radius_mask, "A")
    connect(uv_length, "", radius_mask, "B")
    connect(one, "", radius_mask, "A > B")
    connect(zero, "", radius_mask, "A < B")

    angle_mask = expression(material, unreal.MaterialExpressionIf, -400, 80)
    connect(forward_dot, "", angle_mask, "A")
    connect(half_angle_cos, "", angle_mask, "B")
    connect(one, "", angle_mask, "A > B")
    connect(zero, "", angle_mask, "A < B")

    fan_mask = expression(material, unreal.MaterialExpressionMultiply, -150, -80)
    connect(radius_mask, "", fan_mask, "A")
    connect(angle_mask, "", fan_mask, "B")

    opacity = expression(material, unreal.MaterialExpressionScalarParameter, -150, 180)
    opacity.set_editor_property("parameter_name", "TelegraphOpacity")
    opacity.set_editor_property("default_value", 0.48)
    masked_opacity = expression(material, unreal.MaterialExpressionMultiply, 80, 20)
    connect(fan_mask, "", masked_opacity, "A")
    connect(opacity, "", masked_opacity, "B")

    color = expression(material, unreal.MaterialExpressionVectorParameter, 80, -180)
    color.set_editor_property("parameter_name", "TelegraphColor")
    color.set_editor_property("default_value", unreal.LinearColor(1.0, 0.08, 0.02, 1.0))

    if not unreal.MaterialEditingLibrary.connect_material_property(
        color, "RGB", unreal.MaterialProperty.MP_BASE_COLOR
    ):
        raise RuntimeError("Unable to connect sweep decal base color")
    if not unreal.MaterialEditingLibrary.connect_material_property(
        masked_opacity, "", unreal.MaterialProperty.MP_OPACITY
    ):
        raise RuntimeError("Unable to connect sweep decal opacity")

    unreal.MaterialEditingLibrary.recompile_material(material)
    if not unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False):
        raise RuntimeError(f"Unable to save sweep decal material: {ASSET_PATH}")
    unreal.log(f"Created or refreshed sweep telegraph decal material: {ASSET_PATH}")


build_sweep_decal_material()
