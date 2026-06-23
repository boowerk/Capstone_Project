"""Create the UI-domain material used to pan and zoom the one-shot minimap capture."""

import unreal


ASSET_PATH = "/Game/UI/HUD/Minimap/Materials"
ASSET_NAME = "M_UI_Minimap_StaticMap"
FULL_ASSET_PATH = f"{ASSET_PATH}/{ASSET_NAME}"


def create_expression(material, expression_class, x, y):
    """Create and place one graph expression so the generated asset stays readable in the editor."""
    return unreal.MaterialEditingLibrary.create_material_expression(
        material, expression_class, x, y
    )


def connect(source, output_name, target, input_name):
    """Fail loudly when a material pin name changes between engine versions."""
    if not unreal.MaterialEditingLibrary.connect_material_expressions(
        source, output_name, target, input_name
    ):
        raise RuntimeError(f"Unable to connect material pin: {input_name}")


def build_material():
    """Build SourceUV = (WidgetUV - 0.5) / MapZoom + MapCenterUV with a circular UI opacity mask."""
    material = None
    if unreal.EditorAssetLibrary.does_asset_exist(FULL_ASSET_PATH):
        material = unreal.EditorAssetLibrary.load_asset(FULL_ASSET_PATH)
        if material.get_editor_property("blend_mode") == unreal.BlendMode.BLEND_TRANSLUCENT:
            unreal.log(f"Minimap UI material already supports circular opacity: {FULL_ASSET_PATH}")
            return material
        # UE 5.7 can assert when deleting rooted material expressions in commandlets, so patch the existing graph in place.
        patch_circle_mask(material)
        return material
    else:
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            ASSET_NAME,
            ASSET_PATH,
            unreal.Material,
            unreal.MaterialFactoryNew(),
        )
    if not material:
        raise RuntimeError("Failed to create static minimap UI material")

    material.set_editor_property("material_domain", unreal.MaterialDomain.MD_UI)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)

    tex_coord = create_expression(material, unreal.MaterialExpressionTextureCoordinate, -1200, 0)
    half_uv = create_expression(material, unreal.MaterialExpressionConstant2Vector, -1200, 180)
    half_uv.set_editor_property("r", 0.5)
    half_uv.set_editor_property("g", 0.5)

    subtract = create_expression(material, unreal.MaterialExpressionSubtract, -950, 0)
    zoom = create_expression(material, unreal.MaterialExpressionScalarParameter, -950, 220)
    zoom.set_editor_property("parameter_name", "MapZoom")
    zoom.set_editor_property("default_value", 3.0)
    divide = create_expression(material, unreal.MaterialExpressionDivide, -700, 0)

    center_u = create_expression(material, unreal.MaterialExpressionScalarParameter, -700, 180)
    center_u.set_editor_property("parameter_name", "MapCenterU")
    center_u.set_editor_property("default_value", 0.5)
    center_v = create_expression(material, unreal.MaterialExpressionScalarParameter, -700, 300)
    center_v.set_editor_property("parameter_name", "MapCenterV")
    center_v.set_editor_property("default_value", 0.5)
    center_uv = create_expression(material, unreal.MaterialExpressionAppendVector, -475, 220)
    add = create_expression(material, unreal.MaterialExpressionAdd, -450, 0)

    map_sample = create_expression(material, unreal.MaterialExpressionTextureSampleParameter2D, -150, 0)
    map_sample.set_editor_property("parameter_name", "MapTexture")
    preview_texture = unreal.EditorAssetLibrary.load_asset(
        "/Game/UI/HUD/Minimap/Textures/T_UI_Minimap_Map_AshenField"
    )
    if preview_texture:
        map_sample.set_editor_property("texture", preview_texture)

    uv_length = create_expression(material, unreal.MaterialExpressionLength, -700, -220)
    mask_radius = create_expression(material, unreal.MaterialExpressionScalarParameter, -700, -380)
    mask_radius.set_editor_property("parameter_name", "CircleMaskRadius")
    mask_radius.set_editor_property("default_value", 0.49)
    one = create_expression(material, unreal.MaterialExpressionConstant, -470, -380)
    one.set_editor_property("r", 1.0)
    zero = create_expression(material, unreal.MaterialExpressionConstant, -470, -280)
    zero.set_editor_property("r", 0.0)
    circle_mask = create_expression(material, unreal.MaterialExpressionIf, -250, -260)

    connect(tex_coord, "", subtract, "A")
    connect(half_uv, "", subtract, "B")
    connect(subtract, "", divide, "A")
    connect(zoom, "", divide, "B")
    connect(divide, "", add, "A")
    connect(center_u, "", center_uv, "A")
    connect(center_v, "", center_uv, "B")
    connect(center_uv, "", add, "B")
    connect(add, "", map_sample, "UVs")
    connect(subtract, "", uv_length, "")
    connect(mask_radius, "", circle_mask, "A")
    connect(uv_length, "", circle_mask, "B")
    connect(one, "", circle_mask, "A > B")
    connect(zero, "", circle_mask, "A < B")
    unreal.MaterialEditingLibrary.connect_material_property(
        map_sample, "RGB", unreal.MaterialProperty.MP_EMISSIVE_COLOR
    )
    unreal.MaterialEditingLibrary.connect_material_property(
        circle_mask, "", unreal.MaterialProperty.MP_OPACITY
    )

    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
    unreal.log(f"Created minimap UI material: {FULL_ASSET_PATH}")
    return material


def patch_circle_mask(material):
    """Add/refresh the circular opacity mask without touching the existing pan/zoom color graph."""
    if not material:
        raise RuntimeError("Missing minimap material for circle-mask patch")

    material.set_editor_property("material_domain", unreal.MaterialDomain.MD_UI)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)

    tex_coord = create_expression(material, unreal.MaterialExpressionTextureCoordinate, -1200, -520)
    half_uv = create_expression(material, unreal.MaterialExpressionConstant2Vector, -1200, -360)
    half_uv.set_editor_property("r", 0.5)
    half_uv.set_editor_property("g", 0.5)
    centered_uv = create_expression(material, unreal.MaterialExpressionSubtract, -980, -520)
    uv_length = create_expression(material, unreal.MaterialExpressionLength, -760, -520)
    mask_radius = create_expression(material, unreal.MaterialExpressionScalarParameter, -760, -680)
    mask_radius.set_editor_property("parameter_name", "CircleMaskRadius")
    mask_radius.set_editor_property("default_value", 0.49)
    one = create_expression(material, unreal.MaterialExpressionConstant, -520, -680)
    one.set_editor_property("r", 1.0)
    zero = create_expression(material, unreal.MaterialExpressionConstant, -520, -580)
    zero.set_editor_property("r", 0.0)
    circle_mask = create_expression(material, unreal.MaterialExpressionIf, -300, -540)

    connect(tex_coord, "", centered_uv, "A")
    connect(half_uv, "", centered_uv, "B")
    connect(centered_uv, "", uv_length, "")
    connect(mask_radius, "", circle_mask, "A")
    connect(uv_length, "", circle_mask, "B")
    connect(one, "", circle_mask, "A > B")
    connect(zero, "", circle_mask, "A < B")
    if not unreal.MaterialEditingLibrary.connect_material_property(
        circle_mask, "", unreal.MaterialProperty.MP_OPACITY
    ):
        raise RuntimeError("Unable to connect minimap circle opacity")

    unreal.MaterialEditingLibrary.recompile_material(material)
    if not unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False):
        raise RuntimeError(f"Unable to save minimap UI material: {FULL_ASSET_PATH}")
    unreal.log(f"Patched minimap UI material with circular opacity: {FULL_ASSET_PATH}")


build_material()
