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


def build_material():
    """Build SourceUV = (WidgetUV - 0.5) / MapZoom + MapCenterUV."""
    material = None
    if unreal.EditorAssetLibrary.does_asset_exist(FULL_ASSET_PATH):
        material = unreal.EditorAssetLibrary.load_asset(FULL_ASSET_PATH)
        # Rebuilding in place keeps references stable while making this script safe to rerun after graph changes.
        unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
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
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)

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

    unreal.MaterialEditingLibrary.connect_material_expressions(tex_coord, "", subtract, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(half_uv, "", subtract, "B")
    unreal.MaterialEditingLibrary.connect_material_expressions(subtract, "", divide, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(zoom, "", divide, "B")
    unreal.MaterialEditingLibrary.connect_material_expressions(divide, "", add, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(center_u, "", center_uv, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(center_v, "", center_uv, "B")
    unreal.MaterialEditingLibrary.connect_material_expressions(center_uv, "", add, "B")
    unreal.MaterialEditingLibrary.connect_material_expressions(add, "", map_sample, "UVs")
    unreal.MaterialEditingLibrary.connect_material_property(
        map_sample, "RGB", unreal.MaterialProperty.MP_EMISSIVE_COLOR
    )

    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
    unreal.log(f"Created minimap UI material: {FULL_ASSET_PATH}")
    return material


build_material()
