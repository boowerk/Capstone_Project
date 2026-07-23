"""Create the circular red deferred-decal material used by Sans Ground Hands."""

import unreal


ASSET_DIRECTORY = "/Game/Effects"
ASSET_NAME = "M_BossGroundHandTelegraph_Decal"
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


def build_ground_hand_decal_material():
    """Build a soft-edged circle mask inside the hand warning's square projection."""
    if unreal.EditorAssetLibrary.does_asset_exist(ASSET_PATH):
        # Commandlet graph deletion is unsafe in UE 5.7, so never mutate an existing production material.
        unreal.log(f"Ground-hand telegraph decal material already exists: {ASSET_PATH}")
        return

    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        ASSET_NAME,
        ASSET_DIRECTORY,
        unreal.Material,
        unreal.MaterialFactoryNew(),
    )
    if material is None:
        raise RuntimeError(f"Unable to create ground-hand decal material: {ASSET_PATH}")

    material.set_editor_property("material_domain", unreal.MaterialDomain.MD_DEFERRED_DECAL)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)

    tex_coord = expression(material, unreal.MaterialExpressionTextureCoordinate, -1100, -80)
    center = expression(material, unreal.MaterialExpressionConstant2Vector, -1100, 80)
    center.set_editor_property("r", 0.5)
    center.set_editor_property("g", 0.5)
    centered_uv = expression(material, unreal.MaterialExpressionSubtract, -880, -80)
    connect(tex_coord, "", centered_uv, "A")
    connect(center, "", centered_uv, "B")

    distance_from_center = expression(material, unreal.MaterialExpressionLength, -660, -80)
    connect(centered_uv, "", distance_from_center, "")

    radius = expression(material, unreal.MaterialExpressionScalarParameter, -660, -260)
    radius.set_editor_property("parameter_name", "NormalizedRadius")
    radius.set_editor_property("default_value", 0.48)
    signed_circle_distance = expression(material, unreal.MaterialExpressionSubtract, -430, -100)
    connect(radius, "", signed_circle_distance, "A")
    connect(distance_from_center, "", signed_circle_distance, "B")

    edge_softness = expression(material, unreal.MaterialExpressionScalarParameter, -430, 80)
    edge_softness.set_editor_property("parameter_name", "EdgeSoftness")
    edge_softness.set_editor_property("default_value", 0.025)
    normalized_edge = expression(material, unreal.MaterialExpressionDivide, -200, -80)
    connect(signed_circle_distance, "", normalized_edge, "A")
    connect(edge_softness, "", normalized_edge, "B")
    circle_mask = expression(material, unreal.MaterialExpressionSaturate, 20, -80)
    connect(normalized_edge, "", circle_mask, "")

    opacity = expression(material, unreal.MaterialExpressionScalarParameter, 20, 100)
    opacity.set_editor_property("parameter_name", "TelegraphOpacity")
    opacity.set_editor_property("default_value", 0.62)
    masked_opacity = expression(material, unreal.MaterialExpressionMultiply, 250, 20)
    connect(circle_mask, "", masked_opacity, "A")
    connect(opacity, "", masked_opacity, "B")

    color = expression(material, unreal.MaterialExpressionVectorParameter, 250, -180)
    color.set_editor_property("parameter_name", "TelegraphColor")
    color.set_editor_property("default_value", unreal.LinearColor(1.0, 0.0, 0.0, 1.0))

    if not unreal.MaterialEditingLibrary.connect_material_property(
        color, "RGB", unreal.MaterialProperty.MP_BASE_COLOR
    ):
        raise RuntimeError("Unable to connect ground-hand decal base color")
    if not unreal.MaterialEditingLibrary.connect_material_property(
        masked_opacity, "", unreal.MaterialProperty.MP_OPACITY
    ):
        raise RuntimeError("Unable to connect ground-hand decal opacity")

    unreal.MaterialEditingLibrary.recompile_material(material)
    if not unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False):
        raise RuntimeError(f"Unable to save ground-hand decal material: {ASSET_PATH}")
    unreal.log(f"Created circular red ground-hand decal material: {ASSET_PATH}")


build_ground_hand_decal_material()
