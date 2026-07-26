"""Create the lighting-independent circular decal shared by combat telegraphs."""

import unreal


ASSET_DIRECTORY = "/Game/Effects"
ASSET_NAME = "M_EmissiveCircleTelegraph_Decal"
ASSET_PATH = f"{ASSET_DIRECTORY}/{ASSET_NAME}"


def expression(material, expression_class, x, y):
    """Create one positioned expression so the generated graph stays readable."""
    result = unreal.MaterialEditingLibrary.create_material_expression(
        material, expression_class, x, y
    )
    if result is None:
        raise RuntimeError(f"Unable to create {expression_class.get_name()}")
    return result


def connect(source, output_name, target, input_name):
    """Fail immediately when an engine update changes a required material pin."""
    if not unreal.MaterialEditingLibrary.connect_material_expressions(
        source, output_name, target, input_name
    ):
        raise RuntimeError(f"Unable to connect material pin: {input_name}")


def build_emissive_circle_telegraph_material():
    """Build a soft circular decal whose emissive output remains visible in darkness."""
    if unreal.EditorAssetLibrary.does_asset_exist(ASSET_PATH):
        unreal.log(f"Emissive circle telegraph material already exists: {ASSET_PATH}")
        return

    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        ASSET_NAME,
        ASSET_DIRECTORY,
        unreal.Material,
        unreal.MaterialFactoryNew(),
    )
    if material is None:
        raise RuntimeError(f"Unable to create telegraph material: {ASSET_PATH}")

    material.set_editor_property(
        "material_domain", unreal.MaterialDomain.MD_DEFERRED_DECAL
    )
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)

    tex_coord = expression(
        material, unreal.MaterialExpressionTextureCoordinate, -1200, -100
    )
    center = expression(
        material, unreal.MaterialExpressionConstant2Vector, -1200, 60
    )
    center.set_editor_property("r", 0.5)
    center.set_editor_property("g", 0.5)
    centered_uv = expression(
        material, unreal.MaterialExpressionSubtract, -980, -100
    )
    connect(tex_coord, "", centered_uv, "A")
    connect(center, "", centered_uv, "B")

    distance_from_center = expression(
        material, unreal.MaterialExpressionLength, -760, -100
    )
    connect(centered_uv, "", distance_from_center, "")

    radius = expression(
        material, unreal.MaterialExpressionScalarParameter, -760, -300
    )
    radius.set_editor_property("parameter_name", "NormalizedRadius")
    radius.set_editor_property("default_value", 0.48)

    signed_circle_distance = expression(
        material, unreal.MaterialExpressionSubtract, -530, -120
    )
    connect(radius, "", signed_circle_distance, "A")
    connect(distance_from_center, "", signed_circle_distance, "B")

    edge_softness = expression(
        material, unreal.MaterialExpressionScalarParameter, -530, 60
    )
    edge_softness.set_editor_property("parameter_name", "EdgeSoftness")
    edge_softness.set_editor_property("default_value", 0.025)

    normalized_edge = expression(
        material, unreal.MaterialExpressionDivide, -300, -100
    )
    connect(signed_circle_distance, "", normalized_edge, "A")
    connect(edge_softness, "", normalized_edge, "B")
    circle_mask = expression(
        material, unreal.MaterialExpressionSaturate, -80, -100
    )
    connect(normalized_edge, "", circle_mask, "")

    opacity = expression(
        material, unreal.MaterialExpressionScalarParameter, -80, 100
    )
    opacity.set_editor_property("parameter_name", "TelegraphOpacity")
    opacity.set_editor_property("default_value", 0.38)
    masked_opacity = expression(
        material, unreal.MaterialExpressionMultiply, 170, 20
    )
    connect(circle_mask, "", masked_opacity, "A")
    connect(opacity, "", masked_opacity, "B")

    color = expression(
        material, unreal.MaterialExpressionVectorParameter, 170, -240
    )
    color.set_editor_property("parameter_name", "TelegraphColor")
    color.set_editor_property(
        "default_value", unreal.LinearColor(0.18, 0.78, 0.72, 1.0)
    )

    emissive_strength = expression(
        material, unreal.MaterialExpressionScalarParameter, 170, -100
    )
    emissive_strength.set_editor_property(
        "parameter_name", "TelegraphEmissiveStrength"
    )
    emissive_strength.set_editor_property("default_value", 1.5)
    emissive = expression(
        material, unreal.MaterialExpressionMultiply, 410, -180
    )
    emissive.set_editor_property(
        "desc", "LIGHTING_INDEPENDENT_TELEGRAPH_EMISSIVE"
    )
    connect(color, "RGB", emissive, "A")
    connect(emissive_strength, "", emissive, "B")

    if not unreal.MaterialEditingLibrary.connect_material_property(
        color, "RGB", unreal.MaterialProperty.MP_BASE_COLOR
    ):
        raise RuntimeError("Unable to connect telegraph base color")
    if not unreal.MaterialEditingLibrary.connect_material_property(
        emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR
    ):
        raise RuntimeError("Unable to connect telegraph emissive color")
    if not unreal.MaterialEditingLibrary.connect_material_property(
        masked_opacity, "", unreal.MaterialProperty.MP_OPACITY
    ):
        raise RuntimeError("Unable to connect telegraph opacity")

    unreal.MaterialEditingLibrary.recompile_material(material)
    if not unreal.EditorAssetLibrary.save_loaded_asset(
        material, only_if_is_dirty=False
    ):
        raise RuntimeError(f"Unable to save telegraph material: {ASSET_PATH}")
    unreal.log(f"Created emissive circle telegraph material: {ASSET_PATH}")


build_emissive_circle_telegraph_material()
