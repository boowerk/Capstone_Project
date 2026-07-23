"""Add one shared world-space macro variation sample to the region landscape material."""

import unreal


MATERIAL_PATH = "/Game/RegionSystem/Materials/Masters/M_StateMask"
INSTANCE_PATH = "/Game/RegionSystem/Materials/Instances/MI_RegionLandscape_GameMap2"
NOISE_TEXTURE_PATH = (
    "/Game/RegionSystem/Textures/Ground/T_RegionGround_MacroNoise_1024"
)

ENABLE_PARAMETER = "UseRegionMacroVariation"
TEXTURE_PARAMETER = "RegionMacroNoise"
SIZE_PARAMETER = "RegionMacroSizeMeters"
STRENGTH_PARAMETER = "RegionMacroStrength"
PARAMETER_GROUP = "Region Macro Variation"

DEFAULT_SIZE_METERS = 180.0
DEFAULT_STRENGTH = 0.16

# Fixed pins shared by Break/Make Material Attributes in UE 5.7. Customized UVs
# are intentionally excluded because this ground path does not publish them.
PASSTHROUGH_ATTRIBUTE_PINS = (
    "Metallic",
    "Specular",
    "Roughness",
    "Anisotropy",
    "EmissiveColor",
    "Opacity",
    "OpacityMask",
    "Normal",
    "Tangent",
    "WorldPositionOffset",
    "SubsurfaceColor",
    "ClearCoat",
    "ClearCoatRoughness",
    "AmbientOcclusion",
    "Refraction",
    "PixelDepthOffset",
    "ShadingModel",
    "Displacement",
)


def create_expression(material, expression_class, x, y, description=""):
    """Create one positioned expression and attach a stable automation marker."""
    expression = unreal.MaterialEditingLibrary.create_material_expression(
        material, expression_class, x, y
    )
    if expression is None:
        raise RuntimeError(f"Unable to create {expression_class.get_name()}")
    if description:
        expression.set_editor_property("desc", description)
    return expression


def connect(source, output_name, target, input_name):
    """Fail before saving if a required material pin changed."""
    if not unreal.MaterialEditingLibrary.connect_material_expressions(
        source, output_name, target, input_name
    ):
        raise RuntimeError(
            f"Unable to connect {source.get_name()}.{output_name} "
            f"to {target.get_name()}.{input_name}"
        )


def configure_parameter(expression, name, group, sort_priority):
    """Keep all new controls together in the Material Instance editor."""
    expression.set_editor_property("parameter_name", name)
    expression.set_editor_property("group", group)
    expression.set_editor_property("sort_priority", sort_priority)


def parameter_names(material, method_name):
    """Normalize Unreal's Name array into ordinary strings."""
    method = getattr(unreal.MaterialEditingLibrary, method_name)
    return {str(name) for name in method(material)}


def graph_already_patched(material):
    """Reject a partial/manual graph instead of silently treating it as complete."""
    if ENABLE_PARAMETER not in parameter_names(
        material, "get_static_switch_parameter_names"
    ):
        return False

    final_source = unreal.MaterialEditingLibrary.get_material_property_input_node(
        material, unreal.MaterialProperty.MP_MATERIAL_ATTRIBUTES
    )
    enable_switch = find_upstream_parameter(
        material, final_source, ENABLE_PARAMETER
    )
    if enable_switch is None:
        raise RuntimeError(
            f"{ENABLE_PARAMETER} exists but is disconnected from Material Attributes"
        )
    if str(enable_switch.get_editor_property("desc")) != "CODEX_REGION_MACRO_VARIATION":
        raise RuntimeError(
            f"{ENABLE_PARAMETER} exists without the expected automation marker"
        )

    direct_final_inputs = list(
        unreal.MaterialEditingLibrary.get_inputs_for_material_expression(
            material, final_source
        )
    )
    slope_blends = [
        expression
        for expression in direct_final_inputs
        if expression.get_class().get_name()
        == "MaterialExpressionBlendMaterialAttributes"
    ]
    if len(slope_blends) != 1:
        raise RuntimeError(
            f"Patched graph expected one slope attribute blend, found {len(slope_blends)}"
        )
    slope_enable_switch = find_upstream_parameter(
        material, slope_blends[0], ENABLE_PARAMETER
    )
    if (
        slope_enable_switch is None
        or slope_enable_switch.get_path_name() != enable_switch.get_path_name()
    ):
        raise RuntimeError(
            f"{ENABLE_PARAMETER} is not connected to the slope ground input"
        )
    return True


def find_upstream_parameter(material, expression, parameter_name, seen=None):
    """Find one named parameter on the connected branch without protected graph access."""
    if expression is None:
        return None
    if seen is None:
        seen = set()
    path = expression.get_path_name()
    if path in seen:
        return None
    seen.add(path)
    try:
        if str(expression.get_editor_property("parameter_name")) == parameter_name:
            return expression
    except Exception:
        pass
    for input_expression in unreal.MaterialEditingLibrary.get_inputs_for_material_expression(
        material, expression
    ):
        found = find_upstream_parameter(
            material, input_expression, parameter_name, seen
        )
        if found is not None:
            return found
    return None


def add_macro_graph(material, noise_texture):
    """Vary V2.2 ground BaseColor before the optional slope/cliff overlay."""
    final_source = unreal.MaterialEditingLibrary.get_material_property_input_node(
        material, unreal.MaterialProperty.MP_MATERIAL_ATTRIBUTES
    )
    if final_source is None:
        raise RuntimeError("M_StateMask has no final Material Attributes source")
    if (
        str(final_source.get_editor_property("parameter_name"))
        != "UseSlopeCliffOverlay"
    ):
        raise RuntimeError(
            "Unexpected final Material Attributes source: "
            f"{final_source.get_class().get_name()} {final_source.get_name()}"
        )

    source = find_upstream_parameter(
        material, final_source, "UseRegionVisualBlendV22"
    )
    if source is None:
        raise RuntimeError("Unable to find the V2.2 region ground attribute source")
    source_output = ""

    direct_final_inputs = list(
        unreal.MaterialEditingLibrary.get_inputs_for_material_expression(
            material, final_source
        )
    )
    slope_blends = [
        expression
        for expression in direct_final_inputs
        if expression.get_class().get_name()
        == "MaterialExpressionBlendMaterialAttributes"
    ]
    if len(slope_blends) != 1:
        raise RuntimeError(
            f"Expected one slope attribute blend, found {len(slope_blends)}"
        )
    slope_blend = slope_blends[0]

    source_x, source_y = (
        unreal.MaterialEditingLibrary.get_material_expression_node_position(source)
    )
    graph_x = source_x + 300
    graph_y = source_y

    break_attributes = create_expression(
        material,
        unreal.MaterialExpressionBreakMaterialAttributes,
        graph_x,
        graph_y - 180,
        "CODEX_REGION_MACRO_BREAK_ATTRIBUTES",
    )

    make_attributes = create_expression(
        material,
        unreal.MaterialExpressionMakeMaterialAttributes,
        graph_x + 1080,
        graph_y - 100,
        "CODEX_REGION_MACRO_MAKE_ATTRIBUTES",
    )
    make_input_names = {
        str(name)
        for name in unreal.MaterialEditingLibrary.get_material_expression_input_names(
            make_attributes
        )
    }
    required_input_names = {"BaseColor", *PASSTHROUGH_ATTRIBUTE_PINS}
    missing_input_names = required_input_names - make_input_names
    if missing_input_names:
        raise RuntimeError(
            "Make Material Attributes is missing expected UE 5.7 pins: "
            f"{missing_input_names}; actual={make_input_names}"
        )

    world_position = create_expression(
        material,
        unreal.MaterialExpressionWorldPosition,
        graph_x - 120,
        graph_y + 420,
        "CODEX_REGION_MACRO_WORLD_POSITION",
    )
    world_xy = create_expression(
        material,
        unreal.MaterialExpressionComponentMask,
        graph_x + 100,
        graph_y + 420,
    )
    world_xy.set_editor_property("r", True)
    world_xy.set_editor_property("g", True)
    world_xy.set_editor_property("b", False)
    world_xy.set_editor_property("a", False)

    size_meters = create_expression(
        material,
        unreal.MaterialExpressionScalarParameter,
        graph_x + 100,
        graph_y + 620,
    )
    configure_parameter(
        size_meters, SIZE_PARAMETER, PARAMETER_GROUP, sort_priority=2
    )
    size_meters.set_editor_property("default_value", DEFAULT_SIZE_METERS)
    size_meters.set_editor_property("slider_min", 25.0)
    size_meters.set_editor_property("slider_max", 500.0)

    centimeters_per_meter = create_expression(
        material,
        unreal.MaterialExpressionConstant,
        graph_x + 320,
        graph_y + 720,
    )
    centimeters_per_meter.set_editor_property("r", 100.0)
    size_centimeters = create_expression(
        material,
        unreal.MaterialExpressionMultiply,
        graph_x + 520,
        graph_y + 640,
    )
    world_uv = create_expression(
        material,
        unreal.MaterialExpressionDivide,
        graph_x + 740,
        graph_y + 460,
        "CODEX_REGION_MACRO_WORLD_UV",
    )

    noise_sample = create_expression(
        material,
        unreal.MaterialExpressionTextureSampleParameter2D,
        graph_x + 980,
        graph_y + 420,
        "CODEX_REGION_MACRO_SAMPLE",
    )
    configure_parameter(
        noise_sample, TEXTURE_PARAMETER, PARAMETER_GROUP, sort_priority=1
    )
    noise_sample.set_editor_property("texture", noise_texture)
    noise_sample.set_editor_property(
        "sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR
    )

    two = create_expression(
        material,
        unreal.MaterialExpressionConstant,
        graph_x + 1200,
        graph_y + 560,
    )
    two.set_editor_property("r", 2.0)
    remap_multiply = create_expression(
        material,
        unreal.MaterialExpressionMultiply,
        graph_x + 1400,
        graph_y + 420,
    )
    one_for_subtract = create_expression(
        material,
        unreal.MaterialExpressionConstant,
        graph_x + 1400,
        graph_y + 600,
    )
    one_for_subtract.set_editor_property("r", 1.0)
    centered_noise = create_expression(
        material,
        unreal.MaterialExpressionSubtract,
        graph_x + 1600,
        graph_y + 420,
    )

    strength = create_expression(
        material,
        unreal.MaterialExpressionScalarParameter,
        graph_x + 1580,
        graph_y + 620,
    )
    configure_parameter(
        strength, STRENGTH_PARAMETER, PARAMETER_GROUP, sort_priority=3
    )
    strength.set_editor_property("default_value", 0.0)
    strength.set_editor_property("slider_min", 0.0)
    strength.set_editor_property("slider_max", 0.5)

    scaled_noise = create_expression(
        material,
        unreal.MaterialExpressionMultiply,
        graph_x + 1800,
        graph_y + 420,
    )
    one_for_factor = create_expression(
        material,
        unreal.MaterialExpressionConstant,
        graph_x + 1800,
        graph_y + 620,
    )
    one_for_factor.set_editor_property("r", 1.0)
    macro_factor = create_expression(
        material,
        unreal.MaterialExpressionAdd,
        graph_x + 2000,
        graph_y + 420,
        "CODEX_REGION_MACRO_FACTOR",
    )
    varied_base_color = create_expression(
        material,
        unreal.MaterialExpressionMultiply,
        graph_x + 840,
        graph_y - 180,
        "CODEX_REGION_MACRO_BASE_COLOR",
    )

    enable_switch = create_expression(
        material,
        unreal.MaterialExpressionStaticSwitchParameter,
        graph_x + 1320,
        graph_y - 100,
        "CODEX_REGION_MACRO_VARIATION",
    )
    configure_parameter(
        enable_switch, ENABLE_PARAMETER, PARAMETER_GROUP, sort_priority=0
    )
    enable_switch.set_editor_property("default_value", False)

    connect(source, source_output, break_attributes, "Attr")
    connect(break_attributes, "BaseColor", varied_base_color, "A")
    connect(macro_factor, "", varied_base_color, "B")
    connect(varied_base_color, "", make_attributes, "BaseColor")
    for attribute_pin in PASSTHROUGH_ATTRIBUTE_PINS:
        connect(
            break_attributes,
            attribute_pin,
            make_attributes,
            attribute_pin,
        )

    connect(world_position, "", world_xy, "")
    connect(size_meters, "", size_centimeters, "A")
    connect(centimeters_per_meter, "", size_centimeters, "B")
    connect(world_xy, "", world_uv, "A")
    connect(size_centimeters, "", world_uv, "B")
    connect(world_uv, "", noise_sample, "UVs")
    connect(noise_sample, "R", remap_multiply, "A")
    connect(two, "", remap_multiply, "B")
    connect(remap_multiply, "", centered_noise, "A")
    connect(one_for_subtract, "", centered_noise, "B")
    connect(centered_noise, "", scaled_noise, "A")
    connect(strength, "", scaled_noise, "B")
    connect(scaled_noise, "", macro_factor, "A")
    connect(one_for_factor, "", macro_factor, "B")

    connect(make_attributes, "", enable_switch, "True")
    connect(source, source_output, enable_switch, "False")
    connect(enable_switch, "", slope_blend, "A")
    connect(enable_switch, "", final_source, "False")


def instance_needs_configuration(instance, noise_texture):
    """Preserve later artist tuning when the intended overrides already exist."""
    unreal.MaterialEditingLibrary.update_material_instance(instance)
    enabled = (
        unreal.MaterialEditingLibrary.get_material_instance_static_switch_parameter_value(
            instance, ENABLE_PARAMETER
        )
    )
    texture = (
        unreal.MaterialEditingLibrary.get_material_instance_texture_parameter_value(
            instance, TEXTURE_PARAMETER
        )
    )
    return not enabled or texture != noise_texture


def configure_instance(instance, noise_texture):
    """Install restrained first-pass values only when GameMap2 is not configured."""
    # The instance may already have been loaded before its parent graph changed.
    # Refresh inherited parameter metadata before writing the new overrides.
    unreal.MaterialEditingLibrary.update_material_instance(instance)
    # UE 5.7's MaterialEditingLibrary setter implementations always return false
    # even after successfully writing the editor-only override. Verify by readback
    # after saving instead of trusting those broken return values.
    unreal.MaterialEditingLibrary.set_material_instance_static_switch_parameter_value(
        instance, ENABLE_PARAMETER, True
    )
    unreal.MaterialEditingLibrary.set_material_instance_texture_parameter_value(
        instance, TEXTURE_PARAMETER, noise_texture
    )
    unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(
        instance, SIZE_PARAMETER, DEFAULT_SIZE_METERS
    )
    unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(
        instance, STRENGTH_PARAMETER, DEFAULT_STRENGTH
    )
    unreal.MaterialEditingLibrary.update_material_instance(instance)


def configure_noise_texture(noise_texture):
    """Treat generated grayscale noise as linear data, not display color."""
    if not noise_texture.get_editor_property("srgb"):
        return False
    noise_texture.modify()
    noise_texture.set_editor_property("srgb", False)
    return True


def verify(material, instance, noise_texture):
    """Read back the properties that protect the existing V2.2 setup."""
    if material.get_editor_property("blend_mode") != unreal.BlendMode.BLEND_OPAQUE:
        raise RuntimeError("M_StateMask must remain Opaque")
    if noise_texture.get_editor_property("srgb"):
        raise RuntimeError("Macro noise must remain linear (sRGB disabled)")
    if not unreal.MaterialEditingLibrary.get_material_instance_static_switch_parameter_value(
        instance, "UseRegionVisualBlendV22"
    ):
        raise RuntimeError("Existing UseRegionVisualBlendV22 was disabled")
    if not unreal.MaterialEditingLibrary.get_material_instance_static_switch_parameter_value(
        instance, ENABLE_PARAMETER
    ):
        raise RuntimeError("Macro variation switch did not persist")

    actual_texture = (
        unreal.MaterialEditingLibrary.get_material_instance_texture_parameter_value(
            instance, TEXTURE_PARAMETER
        )
    )
    if actual_texture != noise_texture:
        raise RuntimeError(
            f"Unexpected macro texture: {actual_texture.get_path_name() if actual_texture else None}"
        )

    actual_size = (
        unreal.MaterialEditingLibrary.get_material_instance_scalar_parameter_value(
            instance, SIZE_PARAMETER
        )
    )
    actual_strength = (
        unreal.MaterialEditingLibrary.get_material_instance_scalar_parameter_value(
            instance, STRENGTH_PARAMETER
        )
    )
    if actual_size <= 0.0:
        raise RuntimeError(f"Macro size must be positive: {actual_size}")
    if actual_strength < 0.0 or actual_strength > 0.5:
        raise RuntimeError(
            f"Macro strength is outside the authored 0..0.5 range: {actual_strength}"
        )

    unreal.log_warning(
        "CODEX_REGION_MACRO_VERIFIED "
        f"enabled=True size_m={actual_size:.3f} strength={actual_strength:.3f} "
        f"texture={actual_texture.get_path_name()} "
        "v22=True blend=Opaque"
    )


def apply():
    """Patch the clean master graph, then preserve and extend the current MI."""
    material = unreal.EditorAssetLibrary.load_asset(MATERIAL_PATH)
    noise_texture = unreal.EditorAssetLibrary.load_asset(NOISE_TEXTURE_PATH)
    if material is None or noise_texture is None:
        raise RuntimeError(
            "Missing material or macro noise texture: "
            f"{material}, {noise_texture}"
        )

    was_patched = graph_already_patched(material)
    if was_patched:
        unreal.log_warning("CODEX_REGION_MACRO_GRAPH already patched; reusing it")
    else:
        material.modify()
        add_macro_graph(material, noise_texture)
        unreal.MaterialEditingLibrary.recompile_material(material)
        if not unreal.EditorAssetLibrary.save_loaded_asset(
            material, only_if_is_dirty=False
        ):
            raise RuntimeError(f"Unable to save {MATERIAL_PATH}")

    if configure_noise_texture(noise_texture):
        if not unreal.EditorAssetLibrary.save_loaded_asset(
            noise_texture, only_if_is_dirty=False
        ):
            raise RuntimeError(f"Unable to save {NOISE_TEXTURE_PATH}")

    instance = unreal.EditorAssetLibrary.load_asset(INSTANCE_PATH)
    if instance is None:
        raise RuntimeError(f"Missing material instance: {INSTANCE_PATH}")
    if instance_needs_configuration(instance, noise_texture):
        instance.modify()
        configure_instance(instance, noise_texture)
        if not unreal.EditorAssetLibrary.save_loaded_asset(
            instance, only_if_is_dirty=False
        ):
            raise RuntimeError(f"Unable to save {INSTANCE_PATH}")

    verify(material, instance, noise_texture)


apply()
