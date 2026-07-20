"""Generate deterministic Landscape-only Region Visual Slots V2.2 textures.

This scratch generator deliberately leaves the authoritative hard Region ID
texture, gameplay classification, PCG, maps, and Unreal assets untouched.  It
derives a visual-only partition of unity from a boundary-aware copy of the
4096 Pair texture's primary labels.  Each junction-to-junction region-pair
boundary receives its own deterministic one-dimensional displacement along
the boundary normal.  Non-uniform 36-260m anchor gaps, independently varied
4-26m anchor magnitudes, and independently biased random signs avoid a repeating
wave cadence.  Junctions and the map perimeter stay fixed.  No shared
two-dimensional noise lattice is used, so unrelated boundaries cannot reveal
a map-wide grid.  This changes presentation only; it never changes the
gameplay/PCG Region ID texture.

Output contract
---------------
``T_GameMap1_RegionVisualWeightsV22.png`` (RGBA8)
    Four fixed visual slots.  Each region is assigned one slot by a stable
    four-colouring of the 32-texel expanded hard-region conflict graph.  A
    region is weight 1 inside its visual label and has a smooth compact-support
    influence outside it.  The half-width varies independently along each
    region-pair boundary from 18 to 32 texels around the original 24-texel
    baseline, using a second deterministic low-frequency PCHIP field shared by
    both sides.  Junctions, the perimeter, and short segments retain 24 texels.
    All active influences are normalized and quantized so R+G+B+A is exactly
    255 at every source texel.

``T_GameMap1_RegionVisualIDs01V22.png`` and
``T_GameMap1_RegionVisualIDs23V22.png`` (two G8 textures)
    Each byte packs two 4-bit Region IDs: ``id0 | id1<<4`` and
    ``id2 | id3<<4``.  Splitting the four IDs into two normalized bytes keeps
    decoding exact even when Unreal material expressions use float16.  Each
    slot stores the nearest region assigned to that slot.  Same-slot regions
    are separated by the guarded colouring, so an ID transition occurs only
    in a corridor where that slot's output weight is zero.

Correctness-first Unreal import settings
----------------------------------------
Weights: sRGB off, uncompressed RGBA8 (VectorDisplacementmap), Bilinear,
NoMipmaps, Clamp, Virtual Texture Streaming off.  BC compression is excluded
from the baseline because block compression can disturb exact normalization
and reintroduce 4x4 boundary artefacts.

IDs: sRGB off, G8 Grayscale compression, Nearest, NoMipmaps, Clamp, Virtual
Texture Streaming off.  Decode each normalized sample with
``packedByte = round(sample.r * 255)``; the low ID is ``packedByte % 16`` and
the high ID is ``floor(packedByte / 16)``.  Exhaustive validation covers all
256 byte values after normalized float16 sampling.
"""

from __future__ import annotations

import hashlib
import io
import json
from pathlib import Path

import numpy as np
from PIL import Image
from scipy import ndimage as ndi
from scipy.interpolate import PchipInterpolator


REGION_COUNT = 15
SLOT_COUNT = 4
TARGET_SIZE = (4096, 4096)
BLEND_RADIUS_TEXELS = 24.0
BLEND_RADIUS_RANGE_TEXELS = (18.0, 32.0)
BLEND_WIDTH_SEED = 83491
BLEND_WIDTH_ANCHOR_SPACING_METERS = (80.0, 240.0)
BLEND_WIDTH_ANCHOR_SPACING_MEDIAN_METERS = 155.0
BLEND_WIDTH_ANCHOR_SPACING_LOG_SIGMA = 0.50
BLEND_WIDTH_ENDPOINT_FADE_RANGE_METERS = (18.0, 55.0)
BLEND_WIDTH_ENDPOINT_FADE_TO_SEGMENT_LENGTH = 0.25
BLEND_WIDTH_ENDPOINT_FADE_TO_ENDPOINT_SPACING = 0.35
COLOR_GUARD_RADIUS_TEXELS = 32.0
METERS_PER_TEXEL = 1071.0 / TARGET_SIZE[0]
VISUAL_BOUNDARY_SEED = 22037
ANCHOR_SPACING_METERS = (36.0, 260.0)
ANCHOR_SPACING_MEDIAN_METERS = 135.0
ANCHOR_SPACING_LOG_SIGMA = 0.65
ANCHOR_AMPLITUDE_METERS = (4.0, 26.0)
ANCHOR_AMPLITUDE_BETA_SHAPE = (1.4, 1.7)
ANCHOR_ACCENT_PROBABILITY = 0.25
ANCHOR_ACCENT_MINIMUM_METERS = 14.0
ANCHOR_AMPLITUDE_TO_SPACING_LIMIT = 0.15
ANCHOR_AMPLITUDE_TO_SEGMENT_LENGTH_LIMIT = 0.125
ANCHOR_SIGN_PERSISTENCE = 0.50
MAX_BOUNDARY_DISPLACEMENT_METERS = 16.0
SEGMENT_MEAN_TO_RMS_LIMIT = 0.04
JUNCTION_FADE_RANGE_METERS = (12.0, 42.0)
JUNCTION_FADE_TO_SEGMENT_LENGTH = 0.26
JUNCTION_FADE_TO_ENDPOINT_SPACING = 0.30
JUNCTION_CORE_RADIUS_TEXELS = 4
PERIMETER_CORE_RADIUS_TEXELS = 4

EXTERNAL_ROOT = Path(
    r"C:\Users\dyk66\Documents\ProjectEden_Workspace\VoronoIDTextureGen"
)
AUTHORITATIVE_PAIR = (
    EXTERNAL_ROOT
    / "GameMap1_RegionID_Smoothed"
    / "T_GameMap1_RegionID_Pair.png"
)

OUTPUT_DIR = Path(__file__).resolve().parent
OUTPUT_WEIGHTS = OUTPUT_DIR / "T_GameMap1_RegionVisualWeightsV22.png"
OUTPUT_IDS01 = OUTPUT_DIR / "T_GameMap1_RegionVisualIDs01V22.png"
OUTPUT_IDS23 = OUTPUT_DIR / "T_GameMap1_RegionVisualIDs23V22.png"
OUTPUT_REPORT = OUTPUT_DIR / "region_visual_slots_v22_validation.json"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_path(path: Path) -> str:
    return sha256_bytes(path.read_bytes())


def sha256_array(array: np.ndarray) -> str:
    return sha256_bytes(np.ascontiguousarray(array).tobytes())


def encode_png(image: Image.Image) -> bytes:
    buffer = io.BytesIO()
    image.save(buffer, format="PNG", optimize=True)
    return buffer.getvalue()


def decode_ids(channel: np.ndarray) -> np.ndarray:
    """Decode the project's normalized byte convention into IDs 0..14."""
    values = np.asarray(channel, dtype=np.float32)
    return np.floor(values * (REGION_COUNT - 1) / 255.0 + 0.5).astype(
        np.uint8
    )


def encode_ids(ids: np.ndarray) -> np.ndarray:
    lookup = np.rint(
        np.arange(REGION_COUNT, dtype=np.float32)
        * 255.0
        / (REGION_COUNT - 1)
    ).astype(np.uint8)
    return lookup[ids]


def adjacency_pairs(ids: np.ndarray) -> set[tuple[int, int]]:
    rows: list[np.ndarray] = []
    for first, second in (
        (ids[:, :-1], ids[:, 1:]),
        (ids[:-1, :], ids[1:, :]),
    ):
        changed = first != second
        ordered = np.sort(
            np.stack((first[changed], second[changed]), axis=1), axis=1
        )
        rows.append(ordered)
    unique = np.unique(np.concatenate(rows, axis=0), axis=0)
    return {tuple(int(value) for value in row) for row in unique}


def component_counts(ids: np.ndarray) -> list[int]:
    structure = np.ones((3, 3), dtype=bool)
    return [
        int(ndi.label(ids == region_id, structure=structure)[1])
        for region_id in range(REGION_COUNT)
    ]


def load_authoritative_primary() -> tuple[np.ndarray, dict[str, object]]:
    require(
        AUTHORITATIVE_PAIR.is_file(),
        f"Missing authoritative Pair texture: {AUTHORITATIVE_PAIR}",
    )
    with Image.open(AUTHORITATIVE_PAIR) as image:
        require(image.size == TARGET_SIZE, f"Unexpected Pair size: {image.size}")
        pair_rgb = np.asarray(image.convert("RGB"))

    primary = decode_ids(pair_rgb[..., 0])
    expected = set(range(REGION_COUNT))
    actual = {int(value) for value in np.unique(primary)}
    require(actual == expected, f"Expected Region IDs 0..14, got {sorted(actual)}")
    return primary, {
        "authoritative_pair": str(AUTHORITATIVE_PAIR),
        "authoritative_pair_sha256": sha256_path(AUTHORITATIVE_PAIR),
        "authoritative_primary_sha256": sha256_array(primary),
        "authoritative_size": list(TARGET_SIZE),
    }


def quintic_smoothstep01(values: np.ndarray) -> np.ndarray:
    """C2 endpoint envelope: both displacement and slope settle to zero."""
    values = np.clip(values, 0.0, 1.0).astype(np.float32)
    return values * values * values * (
        values * (values * 6.0 - 15.0) + 10.0
    )


def build_junction_and_perimeter_anchor(
    primary: np.ndarray,
) -> tuple[np.ndarray, dict[str, int]]:
    """Protect every raster cell where at least three labels meet."""
    block = np.sort(
        np.stack(
            (
                primary[:-1, :-1],
                primary[:-1, 1:],
                primary[1:, :-1],
                primary[1:, 1:],
            ),
            axis=-1,
        ),
        axis=-1,
    )
    distinct = np.ones(block.shape[:2], dtype=np.uint8)
    for index in range(1, block.shape[-1]):
        distinct += block[..., index] != block[..., index - 1]
    junction_cells = distinct >= 3
    junction_seed = np.zeros(primary.shape, dtype=bool)
    junction_seed[:-1, :-1] |= junction_cells
    junction_seed[:-1, 1:] |= junction_cells
    junction_seed[1:, :-1] |= junction_cells
    junction_seed[1:, 1:] |= junction_cells
    del block, distinct, junction_cells

    structure = np.ones((3, 3), dtype=bool)
    junction_anchor = ndi.binary_dilation(
        junction_seed,
        structure=structure,
        iterations=JUNCTION_CORE_RADIUS_TEXELS,
    )
    perimeter_seed = np.zeros(primary.shape, dtype=bool)
    perimeter_seed[0, :] = True
    perimeter_seed[-1, :] = True
    perimeter_seed[:, 0] = True
    perimeter_seed[:, -1] = True
    perimeter_anchor = ndi.binary_dilation(
        perimeter_seed,
        structure=structure,
        iterations=PERIMETER_CORE_RADIUS_TEXELS,
    )
    protected = junction_anchor | perimeter_anchor
    junction_count = int(ndi.label(junction_seed, structure=structure)[1])
    stats = {
        "junction_count": junction_count,
        "junction_seed_pixels": int(np.count_nonzero(junction_seed)),
        "junction_anchor_pixels": int(np.count_nonzero(junction_anchor)),
        "perimeter_anchor_pixels": int(np.count_nonzero(perimeter_anchor)),
        "protected_anchor_pixels": int(np.count_nonzero(protected)),
    }
    del junction_seed, junction_anchor, perimeter_seed, perimeter_anchor
    return protected, stats


def stable_segment_seed(
    region_a: int,
    region_b: int,
    component_index: int,
) -> int:
    payload = (
        f"{VISUAL_BOUNDARY_SEED}:{region_a}:{region_b}:{component_index}"
    ).encode("ascii")
    return int.from_bytes(hashlib.sha256(payload).digest()[:8], "little")


def sample_truncated_lognormal(
    rng: np.random.Generator,
    median: float,
    log_sigma: float,
    minimum: float,
    maximum: float,
) -> float:
    """Sample a bounded positive distance without a uniform cadence."""
    if maximum - minimum <= 1.0e-9:
        return minimum
    for _ in range(64):
        value = float(rng.lognormal(np.log(median), log_sigma))
        if minimum <= value <= maximum:
            return value
    return float(np.clip(median, minimum, maximum))


def random_anchor_pchip_signal(
    positions: np.ndarray,
    start: float,
    end: float,
    spacing_range: tuple[float, float],
    spacing_median: float,
    spacing_log_sigma: float,
    amplitude_range: tuple[float, float],
    sign_persistence: float,
    rng: np.random.Generator,
) -> tuple[np.ndarray, dict[str, object]]:
    """Build one non-periodic signal from feasible non-uniform anchors."""
    length = end - start
    minimum_spacing, maximum_spacing = spacing_range
    require(minimum_spacing > 0.0, "Anchor spacing must be positive")
    require(
        maximum_spacing >= minimum_spacing,
        "Anchor spacing range is reversed",
    )
    require(
        0.0 <= sign_persistence <= 1.0,
        "Sign persistence must be a probability",
    )
    if length < minimum_spacing:
        return np.zeros_like(positions, dtype=np.float32), {
            "skipped": True,
            "skip_reason": "shorter than minimum anchor spacing",
            "anchor_count": 0,
            "anchor_spacings_meters": [],
            "anchor_values_meters": [],
            "anchor_amplitude_caps_meters": [],
            "accent_anchor_count": 0,
            "same_sign_interval_count": 0,
            "sign_flip_count": 0,
        }

    minimum_interval_count = int(np.ceil(length / maximum_spacing))
    maximum_interval_count = int(np.floor(length / minimum_spacing))
    require(
        minimum_interval_count <= maximum_interval_count,
        "No feasible anchor interval count",
    )
    target_spacing = sample_truncated_lognormal(
        rng,
        spacing_median,
        spacing_log_sigma,
        minimum_spacing,
        maximum_spacing,
    )
    interval_count = int(
        np.clip(
            round(length / target_spacing),
            minimum_interval_count,
            maximum_interval_count,
        )
    )

    intervals: list[float] = []
    remaining = length
    for interval_index in range(interval_count - 1):
        intervals_left = interval_count - interval_index - 1
        feasible_minimum = max(
            minimum_spacing,
            remaining - maximum_spacing * intervals_left,
        )
        feasible_maximum = min(
            maximum_spacing,
            remaining - minimum_spacing * intervals_left,
        )
        require(
            feasible_maximum + 1.0e-9 >= feasible_minimum,
            "Anchor spacing feasibility collapsed",
        )
        gap = sample_truncated_lognormal(
            rng,
            spacing_median,
            spacing_log_sigma,
            feasible_minimum,
            feasible_maximum,
        )
        intervals.append(gap)
        remaining -= gap
    intervals.append(remaining)
    interval_array = np.asarray(intervals, dtype=np.float64)
    require(
        bool(
            np.all(interval_array >= minimum_spacing - 1.0e-7)
            and np.all(interval_array <= maximum_spacing + 1.0e-7)
        ),
        "Generated anchor spacing left the requested range",
    )

    knot_positions = np.concatenate(
        (
            np.asarray([start], dtype=np.float64),
            start + np.cumsum(interval_array, dtype=np.float64),
        )
    )
    knot_positions[-1] = end
    local_spacing = np.empty(knot_positions.size, dtype=np.float64)
    local_spacing[0] = interval_array[0]
    local_spacing[-1] = interval_array[-1]
    if knot_positions.size > 2:
        local_spacing[1:-1] = np.minimum(
            interval_array[:-1],
            interval_array[1:],
        )
    local_amplitude_caps = np.minimum.reduce(
        (
            np.full(knot_positions.size, amplitude_range[1]),
            local_spacing * ANCHOR_AMPLITUDE_TO_SPACING_LIMIT,
            np.full(
                knot_positions.size,
                length * ANCHOR_AMPLITUDE_TO_SEGMENT_LENGTH_LIMIT,
            ),
        )
    )
    require(
        bool(np.all(local_amplitude_caps >= amplitude_range[0])),
        "Local amplitude cap fell below the requested minimum",
    )
    beta_draws = rng.beta(
        ANCHOR_AMPLITUDE_BETA_SHAPE[0],
        ANCHOR_AMPLITUDE_BETA_SHAPE[1],
        knot_positions.size,
    )
    magnitudes = amplitude_range[0] + beta_draws * (
        local_amplitude_caps - amplitude_range[0]
    )
    accent_minimum = ANCHOR_ACCENT_MINIMUM_METERS / METERS_PER_TEXEL
    accent_eligible = (
        length * METERS_PER_TEXEL >= 120.0
    ) & (local_amplitude_caps >= accent_minimum)
    accent_mask = (
        rng.random(knot_positions.size) < ANCHOR_ACCENT_PROBABILITY
    ) & accent_eligible
    if np.any(accent_mask):
        magnitudes[accent_mask] = rng.uniform(
            accent_minimum,
            local_amplitude_caps[accent_mask],
        )
    signs = np.empty(knot_positions.size, dtype=np.float64)
    signs[0] = -1.0 if rng.random() < 0.5 else 1.0
    for knot_index in range(1, knot_positions.size):
        if rng.random() < sign_persistence:
            signs[knot_index] = signs[knot_index - 1]
        else:
            signs[knot_index] = -signs[knot_index - 1]
    knot_values = signs * magnitudes
    signal = PchipInterpolator(knot_positions, knot_values)(positions)
    signal = np.asarray(signal, dtype=np.float32)
    sign_flips = int(np.count_nonzero(signs[1:] != signs[:-1]))
    same_sign = int(signs.size - 1 - sign_flips)
    return signal, {
        "skipped": False,
        "anchor_count": int(knot_positions.size),
        "target_spacing_meters": target_spacing * METERS_PER_TEXEL,
        "anchor_spacings_meters": (
            interval_array * METERS_PER_TEXEL
        ).tolist(),
        "anchor_values_meters": (
            knot_values * METERS_PER_TEXEL
        ).tolist(),
        "anchor_amplitude_caps_meters": (
            local_amplitude_caps * METERS_PER_TEXEL
        ).tolist(),
        "accent_anchor_count": int(np.count_nonzero(accent_mask)),
        "same_sign_interval_count": same_sign,
        "sign_flip_count": sign_flips,
    }


def build_segment_displacement(
    positions: np.ndarray,
    region_a: int,
    region_b: int,
    component_index: int,
) -> tuple[np.ndarray, dict[str, object]]:
    """Create one independent irregular signal and pin both endpoints."""
    start = float(np.min(positions))
    end = float(np.max(positions))
    length = end - start
    require(length > 1.0, "Boundary component has no measurable length")
    rng = np.random.default_rng(
        stable_segment_seed(region_a, region_b, component_index)
    )

    signal, anchor_stats = random_anchor_pchip_signal(
        positions,
        start,
        end,
        tuple(value / METERS_PER_TEXEL for value in ANCHOR_SPACING_METERS),
        ANCHOR_SPACING_MEDIAN_METERS / METERS_PER_TEXEL,
        ANCHOR_SPACING_LOG_SIGMA,
        tuple(value / METERS_PER_TEXEL for value in ANCHOR_AMPLITUDE_METERS),
        ANCHOR_SIGN_PERSISTENCE,
        rng,
    )
    if bool(anchor_stats["skipped"]):
        return np.zeros_like(positions, dtype=np.float32), {
            **anchor_stats,
            "length_texels": length,
            "length_meters": length * METERS_PER_TEXEL,
            "fade_length_texels": 0.0,
            "fade_length_meters": 0.0,
            "maximum_absolute_displacement_texels": 0.0,
            "rms_displacement_texels": 0.0,
        }
    anchor_spacings_meters = anchor_stats["anchor_spacings_meters"]
    endpoint_spacing_limit = min(
        float(anchor_spacings_meters[0]),
        float(anchor_spacings_meters[-1]),
    ) * JUNCTION_FADE_TO_ENDPOINT_SPACING
    fade_length_meters = min(
        JUNCTION_FADE_RANGE_METERS[1],
        length * METERS_PER_TEXEL * JUNCTION_FADE_TO_SEGMENT_LENGTH,
        endpoint_spacing_limit,
    )
    fade_length_meters = max(
        JUNCTION_FADE_RANGE_METERS[0],
        fade_length_meters,
    )
    fade_length = fade_length_meters / METERS_PER_TEXEL
    endpoint_distance = np.minimum(positions - start, end - positions)
    envelope = quintic_smoothstep01(endpoint_distance / fade_length)
    envelope_sum = float(np.sum(envelope, dtype=np.float64))
    require(envelope_sum > 0.0, "Boundary fade envelope has no support")
    raw_displacement = signal * envelope
    raw_commanded_mean = float(
        np.mean(raw_displacement, dtype=np.float64)
    )
    raw_commanded_rms = float(
        np.sqrt(
            np.mean(
                raw_displacement * raw_displacement,
                dtype=np.float64,
            )
        )
    )
    raw_mean_to_rms = abs(raw_commanded_mean) / max(
        raw_commanded_rms,
        1.0e-12,
    )
    bias_correction = 0.0
    if raw_mean_to_rms > SEGMENT_MEAN_TO_RMS_LIMIT:
        # Subtracting this value from the pre-envelope signal makes the final
        # commanded displacement exactly zero-mean.  Find the smallest
        # fraction of that correction whose *post-correction* mean/RMS ratio
        # reaches the requested limit.  This preserves broad one-sided bends
        # while making the limit mathematically true for the actual warp.
        zero_mean_correction = float(
            np.sum(signal * envelope, dtype=np.float64) / envelope_sum
        )
        lower = 0.0
        upper = 1.0
        for _ in range(64):
            fraction = (lower + upper) * 0.5
            candidate = (
                signal - zero_mean_correction * fraction
            ) * envelope
            candidate_mean = float(np.mean(candidate, dtype=np.float64))
            candidate_rms = float(
                np.sqrt(np.mean(candidate * candidate, dtype=np.float64))
            )
            candidate_ratio = abs(candidate_mean) / max(
                candidate_rms,
                1.0e-12,
            )
            if candidate_ratio > SEGMENT_MEAN_TO_RMS_LIMIT:
                lower = fraction
            else:
                upper = fraction
        bias_correction = zero_mean_correction * upper
    signal = signal - bias_correction
    displacement = signal * envelope
    maximum = MAX_BOUNDARY_DISPLACEMENT_METERS / METERS_PER_TEXEL
    unscaled_maximum = float(np.max(np.abs(displacement)))
    amplitude_scale = min(1.0, maximum / max(unscaled_maximum, 1.0e-12))
    displacement *= amplitude_scale
    post_bias_commanded_mean = float(
        np.mean(displacement, dtype=np.float64)
    )
    post_bias_commanded_rms = float(
        np.sqrt(np.mean(displacement * displacement, dtype=np.float64))
    )
    post_bias_mean_to_rms = abs(post_bias_commanded_mean) / max(
        post_bias_commanded_rms,
        1.0e-12,
    )
    require(
        post_bias_mean_to_rms <= SEGMENT_MEAN_TO_RMS_LIMIT + 1.0e-7,
        "Post-bias segment mean/RMS limit was not satisfied",
    )
    return displacement.astype(np.float32), {
        **anchor_stats,
        "length_texels": length,
        "length_meters": length * METERS_PER_TEXEL,
        "fade_length_texels": fade_length,
        "fade_length_meters": fade_length * METERS_PER_TEXEL,
        "raw_commanded_mean_displacement_meters": (
            raw_commanded_mean * METERS_PER_TEXEL
        ),
        "raw_commanded_rms_displacement_meters": (
            raw_commanded_rms * METERS_PER_TEXEL
        ),
        "raw_commanded_mean_to_rms_ratio": raw_mean_to_rms,
        "bias_correction_meters": bias_correction * METERS_PER_TEXEL,
        "post_bias_commanded_mean_displacement_meters": (
            post_bias_commanded_mean * METERS_PER_TEXEL
        ),
        "post_bias_commanded_rms_displacement_meters": (
            post_bias_commanded_rms * METERS_PER_TEXEL
        ),
        "post_bias_commanded_mean_to_rms_ratio": post_bias_mean_to_rms,
        "mean_to_rms_limit": SEGMENT_MEAN_TO_RMS_LIMIT,
        "post_bias_amplitude_scale": amplitude_scale,
        "maximum_absolute_displacement_texels": float(
            np.max(np.abs(displacement))
        ),
        "rms_displacement_texels": float(
            np.sqrt(np.mean(displacement * displacement, dtype=np.float64))
        ),
    }


def build_visual_primary(
    hard_primary: np.ndarray,
) -> tuple[np.ndarray, dict[str, object]]:
    """Move only pair boundaries; keep every junction and perimeter fixed."""
    height, width = hard_primary.shape
    structure = np.ones((3, 3), dtype=bool)
    cross = np.array(
        [[False, True, False], [True, True, True], [False, True, False]],
        dtype=bool,
    )
    protected, anchor_stats = build_junction_and_perimeter_anchor(hard_primary)
    proposal = np.full(hard_primary.shape, 255, dtype=np.uint8)
    proposal_claims = np.zeros(hard_primary.shape, dtype=np.uint8)
    segment_stats: list[dict[str, object]] = []
    skipped_segment_stats: list[dict[str, object]] = []
    fully_protected_pair_stats: list[dict[str, object]] = []
    maximum_commanded = 0.0
    squared_displacement_sum = 0.0
    displacement_sample_count = 0

    hard_region_pairs = sorted(adjacency_pairs(hard_primary))
    for region_a, region_b in hard_region_pairs:
        mask_a = hard_primary == region_a
        mask_b = hard_primary == region_b
        pair_boundary = (
            (mask_a & ndi.binary_dilation(mask_b, structure=cross))
            | (mask_b & ndi.binary_dilation(mask_a, structure=cross))
        )
        raw_pair_boundary_pixels = int(np.count_nonzero(pair_boundary))
        pair_boundary &= ~protected
        unprotected_pair_boundary_pixels = int(np.count_nonzero(pair_boundary))
        if unprotected_pair_boundary_pixels == 0:
            fully_protected_pair_stats.append(
                {
                    "region_pair": [region_a, region_b],
                    "boundary_pixels": raw_pair_boundary_pixels,
                    "skip_reason": (
                        "entire adjacency lies inside the fixed junction or "
                        "perimeter anchor"
                    ),
                }
            )
            del mask_a, mask_b, pair_boundary
            continue
        components, component_count = ndi.label(
            pair_boundary,
            structure=structure,
        )
        del mask_a, mask_b, pair_boundary

        for component_index in range(1, component_count + 1):
            component = components == component_index
            coordinates = np.argwhere(component)
            if coordinates.shape[0] < 8:
                span = np.ptp(coordinates, axis=0) if coordinates.size else 0.0
                skipped_segment_stats.append(
                    {
                        "region_pair": [region_a, region_b],
                        "component_index": component_index,
                        "boundary_pixels": int(coordinates.shape[0]),
                        "approximate_length_meters": float(
                            np.linalg.norm(span) * METERS_PER_TEXEL
                        ),
                        "skip_reason": "fewer than eight boundary pixels",
                    }
                )
                continue

            centered = coordinates.astype(np.float64)
            centered -= np.mean(centered, axis=0, keepdims=True)
            covariance = centered.T @ centered
            eigenvalues, eigenvectors = np.linalg.eigh(covariance)
            tangent_yx = eigenvectors[:, int(np.argmax(eigenvalues))]
            if tangent_yx[1] < 0.0 or (
                tangent_yx[1] == 0.0 and tangent_yx[0] < 0.0
            ):
                tangent_yx *= -1.0
            positions = centered @ tangent_yx
            displacement, stats = build_segment_displacement(
                positions,
                region_a,
                region_b,
                component_index,
            )
            stats.update(
                {
                    "region_pair": [region_a, region_b],
                    "component_index": component_index,
                    "boundary_pixels": int(coordinates.shape[0]),
                    "pca_secondary_to_primary_ratio": float(
                        eigenvalues[0] / max(eigenvalues[-1], 1.0)
                    ),
                }
            )
            if bool(stats["skipped"]):
                skipped_segment_stats.append(stats)
                del (
                    component,
                    coordinates,
                    centered,
                    covariance,
                    eigenvalues,
                    eigenvectors,
                    positions,
                    displacement,
                )
                continue
            maximum_commanded = max(
                maximum_commanded,
                float(np.max(np.abs(displacement))),
            )
            squared_displacement_sum += float(
                np.sum(displacement * displacement, dtype=np.float64)
            )
            displacement_sample_count += displacement.size

            margin = int(
                np.ceil(MAX_BOUNDARY_DISPLACEMENT_METERS / METERS_PER_TEXEL)
            ) + 2
            y0 = max(0, int(np.min(coordinates[:, 0])) - margin)
            y1 = min(height, int(np.max(coordinates[:, 0])) + margin + 1)
            x0 = max(0, int(np.min(coordinates[:, 1])) - margin)
            x1 = min(width, int(np.max(coordinates[:, 1])) + margin + 1)
            component_crop = component[y0:y1, x0:x1]
            displacement_crop = np.zeros(component_crop.shape, dtype=np.float32)
            local_y = coordinates[:, 0] - y0
            local_x = coordinates[:, 1] - x0
            displacement_crop[local_y, local_x] = displacement
            distance, nearest = ndi.distance_transform_edt(
                ~component_crop,
                return_indices=True,
            )
            propagated = displacement_crop[nearest[0], nearest[1]]
            hard_crop = hard_primary[y0:y1, x0:x1]
            protected_crop = protected[y0:y1, x0:x1]
            half_pixel_distance = distance + np.float32(0.5)
            signed_distance = np.where(
                hard_crop == region_a,
                half_pixel_distance,
                -half_pixel_distance,
            )
            pair_zone = (hard_crop == region_a) | (hard_crop == region_b)
            within_reach = distance <= (
                MAX_BOUNDARY_DISPLACEMENT_METERS / METERS_PER_TEXEL + 1.0
            )
            proposed_a = signed_distance + propagated >= 0.0
            proposed_label = np.where(
                proposed_a,
                region_a,
                region_b,
            ).astype(np.uint8)
            changed_here = (
                pair_zone
                & within_reach
                & ~protected_crop
                & (proposed_label != hard_crop)
            )
            claims_crop = proposal_claims[y0:y1, x0:x1]
            proposal_crop = proposal[y0:y1, x0:x1]
            claims_crop[changed_here] += np.uint8(1)
            unset = changed_here & (proposal_crop == 255)
            proposal_crop[unset] = proposed_label[unset]
            disagreement = (
                changed_here
                & (proposal_crop != 255)
                & (proposal_crop != proposed_label)
            )
            proposal_crop[disagreement] = np.uint8(254)

            stats.update(
                {
                    "changed_pixels_proposed": int(
                        np.count_nonzero(changed_here)
                    ),
                }
            )
            segment_stats.append(stats)
            del (
                component,
                coordinates,
                centered,
                covariance,
                eigenvalues,
                eigenvectors,
                positions,
                displacement,
                component_crop,
                displacement_crop,
                local_y,
                local_x,
                distance,
                nearest,
                propagated,
                hard_crop,
                protected_crop,
                half_pixel_distance,
                signed_distance,
                pair_zone,
                within_reach,
                proposed_a,
                proposed_label,
                changed_here,
                claims_crop,
                proposal_crop,
                unset,
                disagreement,
            )
        del components

    proposal_conflicts = int(np.count_nonzero(proposal == 254))
    multiple_claim_pixels = int(np.count_nonzero(proposal_claims > 1))
    require(
        proposal_conflicts == 0,
        f"Boundary segment proposals conflict at {proposal_conflicts} pixels",
    )
    require(
        multiple_claim_pixels == 0,
        f"Boundary segment influence bands overlap at {multiple_claim_pixels} pixels",
    )
    visual_primary = hard_primary.copy()
    proposed = proposal < 254
    visual_primary[proposed] = proposal[proposed]
    del proposal, proposed

    changed = visual_primary != hard_primary
    boundary, _ = transition_endpoints(hard_primary)
    conservative_halo = ndi.binary_dilation(
        boundary,
        structure=np.ones((3, 3), dtype=bool),
        iterations=int(
            np.ceil(MAX_BOUNDARY_DISPLACEMENT_METERS / METERS_PER_TEXEL)
        )
        + 2,
    )
    changed_outside_halo = int(np.count_nonzero(changed & ~conservative_halo))
    changed_in_protected_anchor = int(np.count_nonzero(changed & protected))
    changed_pixels = int(np.count_nonzero(changed))
    maximum_observed = float(
        np.max(ndi.distance_transform_edt(~boundary)[changed])
    )
    area_stats: dict[str, dict[str, float | int]] = {}
    maximum_relative_area_change = 0.0
    for region_id in range(REGION_COUNT):
        hard_area = int(np.count_nonzero(hard_primary == region_id))
        visual_area = int(np.count_nonzero(visual_primary == region_id))
        relative_change = (visual_area - hard_area) * 100.0 / hard_area
        maximum_relative_area_change = max(
            maximum_relative_area_change,
            abs(relative_change),
        )
        area_stats[str(region_id)] = {
            "hard_pixels": hard_area,
            "visual_pixels": visual_area,
            "relative_change_percent": relative_change,
        }
    rms_commanded = (
        float(np.sqrt(squared_displacement_sum / displacement_sample_count))
        if displacement_sample_count
        else 0.0
    )
    anchor_spacing_values = np.asarray(
        [
            value
            for stats in segment_stats
            for value in stats["anchor_spacings_meters"]
        ],
        dtype=np.float64,
    )
    anchor_amplitude_values = np.abs(
        np.asarray(
            [
                value
                for stats in segment_stats
                for value in stats["anchor_values_meters"]
            ],
            dtype=np.float64,
        )
    )
    same_sign_intervals = sum(
        int(stats["same_sign_interval_count"]) for stats in segment_stats
    )
    sign_flip_intervals = sum(
        int(stats["sign_flip_count"]) for stats in segment_stats
    )
    accent_anchor_count = sum(
        int(stats["accent_anchor_count"]) for stats in segment_stats
    )
    post_bias_mean_to_rms_values = np.asarray(
        [
            float(stats["post_bias_commanded_mean_to_rms_ratio"])
            for stats in segment_stats
        ],
        dtype=np.float64,
    )
    processed_region_pairs = sorted(
        {
            tuple(int(value) for value in stats["region_pair"])
            for stats in segment_stats
        }
    )
    skipped_region_pairs = {
        tuple(int(value) for value in stats["region_pair"])
        for stats in skipped_segment_stats
    }
    fully_protected_region_pairs = {
        tuple(int(value) for value in stats["region_pair"])
        for stats in fully_protected_pair_stats
    }
    classified_region_pairs = sorted(
        set(processed_region_pairs)
        | skipped_region_pairs
        | fully_protected_region_pairs
    )
    require(anchor_spacing_values.size > 0, "No boundary anchors were generated")
    require(anchor_amplitude_values.size > 0, "No anchor amplitudes were generated")
    require(
        post_bias_mean_to_rms_values.size > 0,
        "No post-bias segment statistics were generated",
    )
    del changed, boundary, conservative_halo, protected, proposal_claims

    return visual_primary, {
        "method": (
            "independent junction-pinned irregular-anchor region-pair PCHIP "
            "normal displacement"
        ),
        "coordinate_basis": "PCA projection of each near-linear component",
        "seed": VISUAL_BOUNDARY_SEED,
        "meters_per_texel": METERS_PER_TEXEL,
        "raw_anchor_amplitude_range_meters": list(ANCHOR_AMPLITUDE_METERS),
        "raw_anchor_amplitude_distribution": {
            "type": "beta with rare high-amplitude accents",
            "beta_shape": list(ANCHOR_AMPLITUDE_BETA_SHAPE),
            "accent_probability": ANCHOR_ACCENT_PROBABILITY,
            "accent_minimum_meters": ANCHOR_ACCENT_MINIMUM_METERS,
            "amplitude_to_spacing_limit": (
                ANCHOR_AMPLITUDE_TO_SPACING_LIMIT
            ),
            "amplitude_to_segment_length_limit": (
                ANCHOR_AMPLITUDE_TO_SEGMENT_LENGTH_LIMIT
            ),
        },
        "anchor_spacing_range_meters": list(ANCHOR_SPACING_METERS),
        "anchor_spacing_distribution": {
            "type": "truncated lognormal",
            "median_meters": ANCHOR_SPACING_MEDIAN_METERS,
            "log_sigma": ANCHOR_SPACING_LOG_SIGMA,
        },
        "anchor_sign_persistence": ANCHOR_SIGN_PERSISTENCE,
        "segment_mean_to_rms_limit": SEGMENT_MEAN_TO_RMS_LIMIT,
        "actual_anchor_spacing_meters": {
            "minimum": float(np.min(anchor_spacing_values)),
            "maximum": float(np.max(anchor_spacing_values)),
            "mean": float(np.mean(anchor_spacing_values)),
            "standard_deviation": float(np.std(anchor_spacing_values)),
        },
        "actual_raw_anchor_amplitude_meters": {
            "minimum": float(np.min(anchor_amplitude_values)),
            "maximum": float(np.max(anchor_amplitude_values)),
            "mean": float(np.mean(anchor_amplitude_values)),
            "standard_deviation": float(np.std(anchor_amplitude_values)),
        },
        "same_sign_interval_count": same_sign_intervals,
        "sign_flip_interval_count": sign_flip_intervals,
        "accent_anchor_count": accent_anchor_count,
        "maximum_post_bias_segment_mean_to_rms_ratio": float(
            np.max(post_bias_mean_to_rms_values)
        ),
        "maximum_displacement_meters": MAX_BOUNDARY_DISPLACEMENT_METERS,
        "junction_fade_range_meters": list(JUNCTION_FADE_RANGE_METERS),
        "junction_fade_to_segment_length": (
            JUNCTION_FADE_TO_SEGMENT_LENGTH
        ),
        "junction_fade_to_endpoint_spacing": (
            JUNCTION_FADE_TO_ENDPOINT_SPACING
        ),
        "junction_core_radius_texels": JUNCTION_CORE_RADIUS_TEXELS,
        "perimeter_core_radius_texels": PERIMETER_CORE_RADIUS_TEXELS,
        "maximum_commanded_displacement_texels": maximum_commanded,
        "maximum_commanded_displacement_meters": (
            maximum_commanded * METERS_PER_TEXEL
        ),
        "rms_commanded_displacement_texels": rms_commanded,
        "rms_commanded_displacement_meters": (
            rms_commanded * METERS_PER_TEXEL
        ),
        "maximum_observed_changed_distance_texels": maximum_observed,
        "maximum_observed_changed_distance_meters": (
            maximum_observed * METERS_PER_TEXEL
        ),
        "changed_label_pixels": changed_pixels,
        "changed_label_percent": changed_pixels * 100.0 / hard_primary.size,
        "changed_pixels_outside_conservative_boundary_halo": (
            changed_outside_halo
        ),
        "changed_pixels_inside_protected_junction_or_perimeter_anchor": (
            changed_in_protected_anchor
        ),
        "proposal_conflict_pixels": proposal_conflicts,
        "multiple_segment_claim_pixels": multiple_claim_pixels,
        "maximum_relative_region_area_change_percent": (
            maximum_relative_area_change
        ),
        "region_area_stats": area_stats,
        "anchor_stats": anchor_stats,
        "hard_adjacency_pair_count": len(hard_region_pairs),
        "processed_region_pair_count": len(processed_region_pairs),
        "processed_region_pairs": [list(pair) for pair in processed_region_pairs],
        "classified_region_pair_count": len(classified_region_pairs),
        "classified_region_pairs": [list(pair) for pair in classified_region_pairs],
        "fully_protected_region_pair_count": len(fully_protected_pair_stats),
        "fully_protected_region_pairs": fully_protected_pair_stats,
        "segment_count": len(segment_stats),
        "segments": segment_stats,
        "skipped_segment_count": len(skipped_segment_stats),
        "skipped_segments": skipped_segment_stats,
        "visual_primary_sha256": sha256_array(visual_primary),
    }


def build_guard_conflict_graph(
    primary: np.ndarray,
) -> tuple[list[set[int]], list[tuple[int, int]], dict[str, int]]:
    """Connect regions whose 32px Euclidean expansions overlap."""
    claims = np.zeros(primary.shape, dtype=np.uint16)
    adjacency = [set() for _ in range(REGION_COUNT)]
    expanded_pixels: dict[str, int] = {}

    for region_id in range(REGION_COUNT):
        distance = ndi.distance_transform_edt(primary != region_id)
        expanded = distance <= COLOR_GUARD_RADIUS_TEXELS
        expanded_pixels[str(region_id)] = int(np.count_nonzero(expanded))
        prior_bits = int(np.bitwise_or.reduce(claims[expanded]))
        for other_id in range(region_id):
            if prior_bits & (1 << other_id):
                adjacency[region_id].add(other_id)
                adjacency[other_id].add(region_id)
        np.bitwise_or(
            claims,
            np.uint16(1 << region_id),
            out=claims,
            where=expanded,
        )
        del distance, expanded

    edges = [
        (first, second)
        for first in range(REGION_COUNT)
        for second in sorted(adjacency[first])
        if first < second
    ]
    del claims
    return adjacency, edges, expanded_pixels


def canonicalize_colours(colours: list[int]) -> list[int]:
    remap: dict[int, int] = {}
    canonical: list[int] = []
    for colour in colours:
        if colour not in remap:
            remap[colour] = len(remap)
        canonical.append(remap[colour])
    return canonical


def colour_graph(
    adjacency: list[set[int]], maximum_colours: int
) -> list[int] | None:
    """Deterministic exact DSATUR backtracking colouring."""
    colours = [-1] * len(adjacency)

    def search(coloured_count: int) -> bool:
        if coloured_count == len(adjacency):
            return True

        uncoloured = [
            vertex for vertex, colour in enumerate(colours) if colour < 0
        ]

        def priority(vertex: int) -> tuple[int, int, int]:
            neighbour_colours = {
                colours[neighbour]
                for neighbour in adjacency[vertex]
                if colours[neighbour] >= 0
            }
            return (
                len(neighbour_colours),
                len(adjacency[vertex]),
                -vertex,
            )

        vertex = max(uncoloured, key=priority)
        forbidden = {
            colours[neighbour]
            for neighbour in adjacency[vertex]
            if colours[neighbour] >= 0
        }
        for colour in range(maximum_colours):
            if colour in forbidden:
                continue
            colours[vertex] = colour
            if search(coloured_count + 1):
                return True
            colours[vertex] = -1
        return False

    if not search(0):
        return None
    return canonicalize_colours(colours)


def stable_blend_width_seed(
    region_a: int,
    region_b: int,
    component_index: int,
) -> int:
    """Keep blend-width randomness independent from boundary displacement."""
    payload = (
        f"{BLEND_WIDTH_SEED}:{region_a}:{region_b}:{component_index}"
    ).encode("ascii")
    return int.from_bytes(hashlib.sha256(payload).digest()[:8], "little")


def build_blend_width_anchor_gaps(
    length_texels: float,
    rng: np.random.Generator,
) -> tuple[np.ndarray, float] | None:
    minimum = BLEND_WIDTH_ANCHOR_SPACING_METERS[0] / METERS_PER_TEXEL
    maximum = BLEND_WIDTH_ANCHOR_SPACING_METERS[1] / METERS_PER_TEXEL
    median = BLEND_WIDTH_ANCHOR_SPACING_MEDIAN_METERS / METERS_PER_TEXEL
    if length_texels < minimum:
        return None

    minimum_interval_count = int(np.ceil(length_texels / maximum))
    maximum_interval_count = int(np.floor(length_texels / minimum))
    if minimum_interval_count > maximum_interval_count:
        return None
    target = sample_truncated_lognormal(
        rng,
        median,
        BLEND_WIDTH_ANCHOR_SPACING_LOG_SIGMA,
        minimum,
        maximum,
    )
    interval_count = int(
        np.clip(
            round(length_texels / target),
            minimum_interval_count,
            maximum_interval_count,
        )
    )

    intervals: list[float] = []
    remaining = float(length_texels)
    for interval_index in range(interval_count - 1):
        intervals_left = interval_count - interval_index - 1
        feasible_minimum = max(
            minimum,
            remaining - maximum * intervals_left,
        )
        feasible_maximum = min(
            maximum,
            remaining - minimum * intervals_left,
        )
        require(
            feasible_maximum + 1.0e-9 >= feasible_minimum,
            "Blend-width anchor feasibility collapsed",
        )
        gap = sample_truncated_lognormal(
            rng,
            median,
            BLEND_WIDTH_ANCHOR_SPACING_LOG_SIGMA,
            feasible_minimum,
            feasible_maximum,
        )
        intervals.append(gap)
        remaining -= gap
    intervals.append(remaining)
    interval_array = np.asarray(intervals, dtype=np.float64)
    require(
        bool(
            np.all(interval_array >= minimum - 1.0e-7)
            and np.all(interval_array <= maximum + 1.0e-7)
        ),
        "Blend-width anchor gap left the requested range",
    )
    return interval_array, float(target)


def build_segment_blend_radii(
    positions: np.ndarray,
    region_a: int,
    region_b: int,
    component_index: int,
) -> tuple[np.ndarray, dict[str, object]]:
    """Build one non-periodic half-width signal and pin both endpoints."""
    start = float(np.min(positions))
    end = float(np.max(positions))
    length = end - start
    rng = np.random.default_rng(
        stable_blend_width_seed(region_a, region_b, component_index)
    )
    sampled = build_blend_width_anchor_gaps(length, rng)
    if sampled is None:
        return np.full(
            positions.shape,
            BLEND_RADIUS_TEXELS,
            dtype=np.float32,
        ), {
            "skipped": True,
            "skip_reason": "segment shorter than minimum 80m anchor gap",
            "length_meters": length * METERS_PER_TEXEL,
            "anchor_count": 0,
            "endpoint_fade_meters": 0.0,
            "maximum_endpoint_deviation_texels": 0.0,
        }

    gaps, target = sampled
    knot_positions = np.concatenate(
        ([start], start + np.cumsum(gaps, dtype=np.float64))
    )
    knot_positions[-1] = end
    knot_radii = rng.uniform(
        BLEND_RADIUS_RANGE_TEXELS[0],
        BLEND_RADIUS_RANGE_TEXELS[1],
        knot_positions.size,
    )
    signal = np.asarray(
        PchipInterpolator(knot_positions, knot_radii)(positions),
        dtype=np.float32,
    )

    endpoint_spacing_meters = min(gaps[0], gaps[-1]) * METERS_PER_TEXEL
    length_meters = length * METERS_PER_TEXEL
    fade_meters = min(
        BLEND_WIDTH_ENDPOINT_FADE_RANGE_METERS[1],
        length_meters * BLEND_WIDTH_ENDPOINT_FADE_TO_SEGMENT_LENGTH,
        endpoint_spacing_meters
        * BLEND_WIDTH_ENDPOINT_FADE_TO_ENDPOINT_SPACING,
    )
    fade_meters = min(
        max(BLEND_WIDTH_ENDPOINT_FADE_RANGE_METERS[0], fade_meters),
        length_meters * 0.45,
    )
    fade_texels = max(fade_meters / METERS_PER_TEXEL, 1.0)
    endpoint_distance = np.minimum(positions - start, end - positions)
    envelope = quintic_smoothstep01(endpoint_distance / fade_texels)
    radii = BLEND_RADIUS_TEXELS + (
        signal - BLEND_RADIUS_TEXELS
    ) * envelope
    radii = np.clip(
        radii,
        BLEND_RADIUS_RANGE_TEXELS[0],
        BLEND_RADIUS_RANGE_TEXELS[1],
    ).astype(np.float32)
    endpoint_samples = np.isclose(positions, start) | np.isclose(
        positions,
        end,
    )
    return radii, {
        "skipped": False,
        "length_meters": length_meters,
        "anchor_count": int(knot_positions.size),
        "target_anchor_spacing_meters": target * METERS_PER_TEXEL,
        "anchor_spacings_meters": (gaps * METERS_PER_TEXEL).tolist(),
        "anchor_radii_texels": knot_radii.tolist(),
        "endpoint_fade_meters": fade_meters,
        "minimum_radius_texels": float(np.min(radii)),
        "maximum_radius_texels": float(np.max(radii)),
        "maximum_endpoint_deviation_texels": float(
            np.max(np.abs(radii[endpoint_samples] - BLEND_RADIUS_TEXELS))
        ),
    }


def build_boundary_blend_radius_field(
    primary: np.ndarray,
) -> tuple[np.ndarray, np.ndarray, np.ndarray, dict[str, object]]:
    """Assign one shared low-frequency half-width field per region pair."""
    cross = np.asarray(
        [[False, True, False], [True, True, True], [False, True, False]],
        dtype=bool,
    )
    structure = np.ones((3, 3), dtype=bool)
    protected, protected_stats = build_junction_and_perimeter_anchor(primary)
    boundary, _ = transition_endpoints(primary)
    radius = np.full(primary.shape, BLEND_RADIUS_TEXELS, dtype=np.float32)
    claimed = np.zeros(primary.shape, dtype=bool)
    ambiguous = np.zeros(primary.shape, dtype=bool)
    segment_stats: list[dict[str, object]] = []
    skipped_segment_stats: list[dict[str, object]] = []

    for region_a, region_b in sorted(adjacency_pairs(primary)):
        mask_a = primary == region_a
        mask_b = primary == region_b
        pair_boundary = (
            (mask_a & ndi.binary_dilation(mask_b, structure=cross))
            | (mask_b & ndi.binary_dilation(mask_a, structure=cross))
        ) & ~protected
        components, component_count = ndi.label(
            pair_boundary,
            structure=structure,
        )
        del mask_a, mask_b, pair_boundary

        for component_index in range(1, component_count + 1):
            coordinates = np.argwhere(components == component_index)
            if coordinates.shape[0] < 8:
                skipped_segment_stats.append(
                    {
                        "region_pair": [region_a, region_b],
                        "component_index": component_index,
                        "boundary_pixels": int(coordinates.shape[0]),
                        "skip_reason": "fewer than eight boundary pixels",
                    }
                )
                continue

            centered = coordinates.astype(np.float64)
            centered -= np.mean(centered, axis=0, keepdims=True)
            covariance = centered.T @ centered
            eigenvalues, eigenvectors = np.linalg.eigh(covariance)
            tangent_yx = eigenvectors[:, int(np.argmax(eigenvalues))]
            if tangent_yx[1] < 0.0 or (
                tangent_yx[1] == 0.0 and tangent_yx[0] < 0.0
            ):
                tangent_yx *= -1.0
            positions = centered @ tangent_yx
            radii, stats = build_segment_blend_radii(
                positions,
                region_a,
                region_b,
                component_index,
            )
            ys = coordinates[:, 0]
            xs = coordinates[:, 1]
            collision = claimed[ys, xs]
            fresh = ~collision
            radius[ys[fresh], xs[fresh]] = radii[fresh]
            ambiguous[ys[collision], xs[collision]] = True
            claimed[ys, xs] = True
            stats.update(
                {
                    "region_pair": [region_a, region_b],
                    "component_index": component_index,
                    "boundary_pixels": int(coordinates.shape[0]),
                    "pca_secondary_to_primary_ratio": float(
                        eigenvalues[0] / max(eigenvalues[-1], 1.0)
                    ),
                }
            )
            (
                skipped_segment_stats
                if bool(stats["skipped"])
                else segment_stats
            ).append(stats)
        del components

    # Multiple pair claims are junction-like raster ambiguities.  The neutral
    # baseline is safer there than choosing either pair's width field.
    radius[ambiguous | protected] = BLEND_RADIUS_TEXELS
    radius[~boundary] = BLEND_RADIUS_TEXELS
    boundary_values = radius[boundary]
    unassigned_unprotected = boundary & ~claimed & ~protected
    gap_values = np.asarray(
        [
            gap
            for stats in segment_stats
            for gap in stats["anchor_spacings_meters"]
        ],
        dtype=np.float64,
    )
    maximum_endpoint_deviation = max(
        (
            float(stats["maximum_endpoint_deviation_texels"])
            for stats in segment_stats
        ),
        default=0.0,
    )
    return radius, boundary, protected, {
        "method": (
            "one independent continuous PCA/PCHIP half-width signal per "
            "region pair, sampled on both raster sides; each region influence "
            "reads its own nearest boundary pixel"
        ),
        "seed": BLEND_WIDTH_SEED,
        "base_half_width_texels": BLEND_RADIUS_TEXELS,
        "requested_half_width_range_texels": list(
            BLEND_RADIUS_RANGE_TEXELS
        ),
        "requested_half_width_range_meters": [
            value * METERS_PER_TEXEL for value in BLEND_RADIUS_RANGE_TEXELS
        ],
        "anchor_spacing_range_meters": list(
            BLEND_WIDTH_ANCHOR_SPACING_METERS
        ),
        "anchor_spacing_median_meters": (
            BLEND_WIDTH_ANCHOR_SPACING_MEDIAN_METERS
        ),
        "anchor_spacing_log_sigma": BLEND_WIDTH_ANCHOR_SPACING_LOG_SIGMA,
        "actual_anchor_spacing_meters": {
            "minimum": float(np.min(gap_values)) if gap_values.size else 0.0,
            "maximum": float(np.max(gap_values)) if gap_values.size else 0.0,
            "mean": float(np.mean(gap_values)) if gap_values.size else 0.0,
            "standard_deviation": (
                float(np.std(gap_values)) if gap_values.size else 0.0
            ),
        },
        "actual_boundary_half_width_texels": {
            "minimum": float(np.min(boundary_values)),
            "maximum": float(np.max(boundary_values)),
            "mean": float(np.mean(boundary_values)),
            "standard_deviation": float(np.std(boundary_values)),
        },
        "boundary_pixels": int(np.count_nonzero(boundary)),
        "assigned_boundary_pixels": int(
            np.count_nonzero(claimed & ~ambiguous)
        ),
        "unassigned_unprotected_boundary_pixels": int(
            np.count_nonzero(unassigned_unprotected)
        ),
        "ambiguous_pair_pixels_reset_to_base": int(
            np.count_nonzero(ambiguous)
        ),
        "protected_boundary_pixels_at_base": int(
            np.count_nonzero(protected & boundary)
        ),
        "maximum_endpoint_deviation_texels": maximum_endpoint_deviation,
        "segment_count": len(segment_stats),
        "short_or_tiny_segment_count": len(skipped_segment_stats),
        "segments": segment_stats,
        "short_or_tiny_segments": skipped_segment_stats,
        "protected_anchor_stats": protected_stats,
    }


def smooth_compact_influence(
    distance: np.ndarray,
    radius: np.ndarray,
) -> np.ndarray:
    """Cubic smoothstep with compact local support and value 1 at d=0."""
    t = np.clip(
        1.0 - distance / radius,
        0.0,
        1.0,
    ).astype(np.float32)
    return t * t * (3.0 - 2.0 * t)


def build_raw_slot_weights(
    primary: np.ndarray,
    region_to_slot: list[int],
    boundary_blend_radius: np.ndarray,
) -> tuple[np.ndarray, np.ndarray, dict[str, object]]:
    height, width = primary.shape
    raw = np.zeros((height, width, SLOT_COUNT), dtype=np.float32)
    contributor = np.full((height, width, SLOT_COUNT), 255, dtype=np.uint8)
    guard_claimed = np.zeros((height, width, SLOT_COUNT), dtype=bool)

    support_overlap_pixels = 0
    guard_overlap_pixels = 0
    region_support_pixels: dict[str, int] = {}
    same_slot_separations: list[dict[str, float | int]] = []

    for region_id in range(REGION_COUNT):
        slot = region_to_slot[region_id]
        distance64, nearest = ndi.distance_transform_edt(
            primary != region_id,
            return_indices=True,
        )
        distance = distance64.astype(np.float32)
        del distance64

        expanded = distance <= COLOR_GUARD_RADIUS_TEXELS
        guard_overlap = guard_claimed[..., slot] & expanded
        guard_overlap_pixels += int(np.count_nonzero(guard_overlap))
        guard_claimed[..., slot] |= expanded

        local_radius = boundary_blend_radius[nearest[0], nearest[1]]
        influence = smooth_compact_influence(distance, local_radius)
        active = influence > 0.0
        overlap = (raw[..., slot] > 0.0) & active
        support_overlap_pixels += int(np.count_nonzero(overlap))
        raw[..., slot][active] = influence[active]
        contributor[..., slot][active] = region_id
        region_support_pixels[str(region_id)] = int(np.count_nonzero(active))

        require(
            bool(np.all(influence[primary == region_id] == 1.0)),
            f"Region {region_id} is not weight 1 throughout its hard label",
        )
        for other_id in range(region_id + 1, REGION_COUNT):
            if region_to_slot[other_id] != slot:
                continue
            minimum = float(np.min(distance[primary == other_id]))
            same_slot_separations.append(
                {
                    "region_a": region_id,
                    "region_b": other_id,
                    "minimum_label_distance_texels": minimum,
                }
            )
        del (
            distance,
            nearest,
            expanded,
            guard_overlap,
            local_radius,
            influence,
            active,
            overlap,
        )

    del guard_claimed
    require(
        support_overlap_pixels == 0,
        f"Same-slot variable supports overlap at {support_overlap_pixels} pixels",
    )
    require(
        guard_overlap_pixels == 0,
        f"Same-slot 32px guards overlap at {guard_overlap_pixels} pixels",
    )
    return raw, contributor, {
        "same_slot_support_overlap_pixels": support_overlap_pixels,
        "same_slot_guard_overlap_pixels": guard_overlap_pixels,
        "region_support_pixels": region_support_pixels,
        "same_slot_pair_separations": same_slot_separations,
        "minimum_same_slot_label_distance_texels": min(
            entry["minimum_label_distance_texels"]
            for entry in same_slot_separations
        ),
    }


def quantize_normalized_weights(normalized: np.ndarray) -> np.ndarray:
    """Cumulative rounding preserves zero channels and an exact byte sum."""
    height, width, channels = normalized.shape
    require(channels == SLOT_COUNT, "Expected exactly four normalized slots")
    output = np.empty((height, width, channels), dtype=np.uint8)
    cumulative = np.zeros((height, width), dtype=np.float32)
    previous = np.zeros((height, width), dtype=np.uint16)

    for slot in range(SLOT_COUNT - 1):
        cumulative += normalized[..., slot] * 255.0
        rounded = np.floor(cumulative + 0.5).astype(np.uint16)
        require(bool(np.all(rounded >= previous)), "Non-monotonic quantization")
        output[..., slot] = (rounded - previous).astype(np.uint8)
        previous = rounded
    output[..., SLOT_COUNT - 1] = (255 - previous).astype(np.uint8)
    return output


def active_count_histogram(weights: np.ndarray) -> dict[str, int]:
    counts = np.count_nonzero(weights, axis=-1)
    histogram = np.bincount(counts.ravel(), minlength=SLOT_COUNT + 1)
    return {str(index): int(value) for index, value in enumerate(histogram)}


def build_slot_id_maps(
    primary: np.ndarray,
    region_to_slot: list[int],
) -> tuple[np.ndarray, dict[str, list[int]]]:
    height, width = primary.shape
    slot_ids = np.zeros((height, width, SLOT_COUNT), dtype=np.uint8)
    slot_regions: dict[str, list[int]] = {}
    region_slots = np.asarray(region_to_slot, dtype=np.uint8)
    primary_slots = region_slots[primary]

    for slot in range(SLOT_COUNT):
        regions = [
            region_id
            for region_id, assigned_slot in enumerate(region_to_slot)
            if assigned_slot == slot
        ]
        slot_regions[str(slot)] = regions
        if not regions:
            continue
        seeds = primary_slots == slot
        indices = ndi.distance_transform_edt(
            ~seeds,
            return_distances=False,
            return_indices=True,
        )
        nearest = primary[indices[0], indices[1]]
        require(
            bool(np.all(region_slots[nearest] == slot)),
            f"Nearest-region ID map escaped slot {slot}",
        )
        slot_ids[..., slot] = nearest
        del seeds, indices, nearest
    del primary_slots
    return slot_ids, slot_regions


def pack_slot_id_pairs(slot_ids: np.ndarray) -> tuple[np.ndarray, np.ndarray]:
    require(
        int(slot_ids.min()) >= 0 and int(slot_ids.max()) < REGION_COUNT,
        "Slot IDs must fit valid Region IDs 0..14",
    )
    packed_01 = (
        slot_ids[..., 0] | (slot_ids[..., 1] << np.uint8(4))
    ).astype(np.uint8)
    packed_23 = (
        slot_ids[..., 2] | (slot_ids[..., 3] << np.uint8(4))
    ).astype(np.uint8)
    return packed_01, packed_23


def unpack_slot_id_pairs(
    packed_01: np.ndarray,
    packed_23: np.ndarray,
) -> np.ndarray:
    return np.stack(
        [
            packed_01 & np.uint8(0xF),
            packed_01 >> np.uint8(4),
            packed_23 & np.uint8(0xF),
            packed_23 >> np.uint8(4),
        ],
        axis=-1,
    )


def validate_float16_normalized_byte_decode() -> dict[str, int]:
    """Prove every G8 code survives the material's normalized half path."""
    source = np.arange(256, dtype=np.uint16)
    normalized_half = np.float16(source.astype(np.float32) / 255.0)
    decoded = np.floor(
        (normalized_half * np.float16(255.0)).astype(np.float32) + 0.5
    ).astype(np.uint16)
    absolute_error = np.abs(decoded.astype(np.int16) - source.astype(np.int16))
    return {
        "tested_byte_codes": int(source.size),
        "mismatch_codes": int(np.count_nonzero(decoded != source)),
        "maximum_absolute_byte_error": int(np.max(absolute_error)),
    }


def transition_endpoints(ids: np.ndarray) -> tuple[np.ndarray, int]:
    endpoints = np.zeros(ids.shape, dtype=bool)
    horizontal = ids[:, :-1] != ids[:, 1:]
    endpoints[:, :-1] |= horizontal
    endpoints[:, 1:] |= horizontal
    vertical = ids[:-1, :] != ids[1:, :]
    endpoints[:-1, :] |= vertical
    endpoints[1:, :] |= vertical
    return endpoints, int(np.count_nonzero(horizontal) + np.count_nonzero(vertical))


def validate_id_transition_guards(
    slot_ids: np.ndarray,
    weight_bytes: np.ndarray,
) -> tuple[list[dict[str, int]], int]:
    """Use a 3x3 halo, conservatively covering every touching 2x2 footprint."""
    per_slot: list[dict[str, int]] = []
    total_violations = 0
    structure = np.ones((3, 3), dtype=bool)
    for slot in range(SLOT_COUNT):
        endpoints, adjacency_count = transition_endpoints(slot_ids[..., slot])
        halo = ndi.binary_dilation(endpoints, structure=structure)
        violations = int(np.count_nonzero(halo & (weight_bytes[..., slot] > 0)))
        total_violations += violations
        per_slot.append(
            {
                "slot": slot,
                "id_transition_adjacencies": adjacency_count,
                "id_transition_endpoint_pixels": int(np.count_nonzero(endpoints)),
                "conservative_3x3_zero_weight_halo_pixels": int(
                    np.count_nonzero(halo)
                ),
                "positive_weight_pixels_in_halo": violations,
            }
        )
        del endpoints, halo
    return per_slot, total_violations


def main() -> None:
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    hard_primary, input_metadata = load_authoritative_primary()
    visual_primary, visual_warp = build_visual_primary(hard_primary)
    (
        boundary_blend_radius,
        blend_boundary,
        blend_protected,
        blend_width_stats,
    ) = build_boundary_blend_radius_field(visual_primary)
    (
        repeated_blend_radius,
        repeated_blend_boundary,
        repeated_blend_protected,
        _,
    ) = build_boundary_blend_radius_field(visual_primary)
    blend_width_repeat_exact = bool(
        np.array_equal(boundary_blend_radius, repeated_blend_radius)
        and np.array_equal(blend_boundary, repeated_blend_boundary)
        and np.array_equal(blend_protected, repeated_blend_protected)
    )
    del (
        repeated_blend_radius,
        repeated_blend_boundary,
        repeated_blend_protected,
    )

    hard_encoded = encode_ids(hard_primary)
    hard_roundtrip = decode_ids(hard_encoded)
    hard_adjacency = adjacency_pairs(hard_primary)
    roundtrip_adjacency = adjacency_pairs(hard_roundtrip)
    hard_components = component_counts(hard_primary)
    roundtrip_components = component_counts(hard_roundtrip)
    visual_adjacency = adjacency_pairs(visual_primary)
    visual_components = component_counts(visual_primary)

    adjacency, conflict_edges, expanded_pixels = build_guard_conflict_graph(
        hard_primary
    )
    minimum_colours = None
    selected_colours = None
    for colour_count in range(1, SLOT_COUNT + 1):
        candidate = colour_graph(adjacency, colour_count)
        if candidate is not None:
            minimum_colours = colour_count
            selected_colours = candidate
            break
    require(selected_colours is not None, "Guard graph is not four-colourable")
    repeated_colours = colour_graph(adjacency, SLOT_COUNT)
    require(
        repeated_colours == selected_colours,
        "Deterministic graph colouring repeat differed",
    )
    colour_conflict_violations = sum(
        selected_colours[first] == selected_colours[second]
        for first, second in conflict_edges
    )

    raw, contributor, overlap_stats = build_raw_slot_weights(
        visual_primary,
        selected_colours,
        boundary_blend_radius,
    )
    raw_total = np.sum(raw, axis=-1, dtype=np.float32)
    require(float(raw_total.min()) > 0.0, "A pixel has no visual influence")
    raw /= raw_total[..., None]
    normalized_sum_error = float(
        np.max(np.abs(np.sum(raw, axis=-1, dtype=np.float32) - 1.0))
    )
    raw_active_histogram = active_count_histogram(raw)
    raw_max_active = int(np.max(np.count_nonzero(raw, axis=-1)))

    weight_bytes = quantize_normalized_weights(raw)
    repeated_weight_bytes = quantize_normalized_weights(raw)
    quantization_repeat_exact = bool(
        np.array_equal(weight_bytes, repeated_weight_bytes)
    )
    del repeated_weight_bytes, raw_total

    zero_influence_became_nonzero = int(
        np.count_nonzero((raw == 0.0) & (weight_bytes > 0))
    )
    weight_sum = np.sum(weight_bytes, axis=-1, dtype=np.uint16)
    weight_sum_mismatch_pixels = int(np.count_nonzero(weight_sum != 255))
    quantized_active_histogram = active_count_histogram(weight_bytes)
    quantized_max_active = int(
        np.max(np.count_nonzero(weight_bytes, axis=-1))
    )
    del raw, weight_sum

    # Keep the existing fixed-slot ID maps byte-identical.  The boundary-only
    # visual displacement moves weights, and the validation below proves every
    # moved influence still resolves to the correct unwarped slot ID.
    slot_ids, slot_regions = build_slot_id_maps(hard_primary, selected_colours)
    active_id_mismatch_pixels = int(
        np.count_nonzero(
            (weight_bytes > 0)
            & (slot_ids != contributor)
        )
    )
    region_weight_pixels = {
        str(region_id): int(
            np.count_nonzero(
                (contributor[..., selected_colours[region_id]] == region_id)
                & (weight_bytes[..., selected_colours[region_id]] > 0)
            )
        )
        for region_id in range(REGION_COUNT)
    }
    del contributor

    transition_stats, transition_guard_violations = validate_id_transition_guards(
        slot_ids, weight_bytes
    )
    packed_ids_01, packed_ids_23 = pack_slot_id_pairs(slot_ids)
    float16_byte_decode = validate_float16_normalized_byte_decode()

    weights_image = Image.fromarray(weight_bytes, mode="RGBA")
    ids_01_image = Image.fromarray(packed_ids_01, mode="L")
    ids_23_image = Image.fromarray(packed_ids_23, mode="L")
    weights_png = encode_png(weights_image)
    ids_01_png = encode_png(ids_01_image)
    ids_23_png = encode_png(ids_23_image)
    weights_png_repeat = encode_png(weights_image)
    ids_01_png_repeat = encode_png(ids_01_image)
    ids_23_png_repeat = encode_png(ids_23_image)
    png_encoding_repeat_exact = (
        weights_png == weights_png_repeat
        and ids_01_png == ids_01_png_repeat
        and ids_23_png == ids_23_png_repeat
    )
    OUTPUT_WEIGHTS.write_bytes(weights_png)
    OUTPUT_IDS01.write_bytes(ids_01_png)
    OUTPUT_IDS23.write_bytes(ids_23_png)

    with Image.open(OUTPUT_WEIGHTS) as image:
        written_weights_size = image.size
        written_weights = np.asarray(image.convert("RGBA"))
    with Image.open(OUTPUT_IDS01) as image:
        written_ids_01_size = image.size
        written_ids_01_mode = image.mode
        written_packed_ids_01 = np.asarray(image.convert("L"))
    with Image.open(OUTPUT_IDS23) as image:
        written_ids_23_size = image.size
        written_ids_23_mode = image.mode
        written_packed_ids_23 = np.asarray(image.convert("L"))

    decoded_written_ids = unpack_slot_id_pairs(
        written_packed_ids_01,
        written_packed_ids_23,
    )
    written_weight_sum_mismatch_pixels = int(
        np.count_nonzero(
            np.sum(written_weights, axis=-1, dtype=np.uint16) != 255
        )
    )
    written_invalid_nibble_pixels = int(
        np.count_nonzero(decoded_written_ids >= REGION_COUNT)
    )

    checks = {
        "authoritative_size_is_4096": hard_primary.shape == TARGET_SIZE[::-1],
        "hard_region_ids_are_exactly_0_through_14": (
            set(int(value) for value in np.unique(hard_primary))
            == set(range(REGION_COUNT))
        ),
        "hard_id_encode_decode_roundtrip_is_exact": bool(
            np.array_equal(hard_primary, hard_roundtrip)
        ),
        "hard_id_adjacencies_are_unchanged": hard_adjacency == roundtrip_adjacency,
        "hard_id_components_are_unchanged": (
            hard_components == roundtrip_components
        ),
        "visual_warp_changes_some_boundary_labels": (
            visual_warp["changed_label_pixels"] > 0
        ),
        "visual_boundary_stays_within_requested_amplitude": (
            visual_warp["maximum_commanded_displacement_texels"]
            <= MAX_BOUNDARY_DISPLACEMENT_METERS / METERS_PER_TEXEL + 1.0e-5
        ),
        "visual_boundary_observed_distance_stays_within_raster_tolerance": (
            visual_warp["maximum_observed_changed_distance_meters"]
            <= MAX_BOUNDARY_DISPLACEMENT_METERS + METERS_PER_TEXEL
        ),
        "all_anchor_spacings_are_within_requested_range": (
            visual_warp["actual_anchor_spacing_meters"]["minimum"]
            >= ANCHOR_SPACING_METERS[0] - 1.0e-6
            and visual_warp["actual_anchor_spacing_meters"]["maximum"]
            <= ANCHOR_SPACING_METERS[1] + 1.0e-6
        ),
        "all_raw_anchor_amplitudes_are_within_requested_range": (
            visual_warp["actual_raw_anchor_amplitude_meters"]["minimum"]
            >= ANCHOR_AMPLITUDE_METERS[0] - 1.0e-6
            and visual_warp["actual_raw_anchor_amplitude_meters"]["maximum"]
            <= ANCHOR_AMPLITUDE_METERS[1] + 1.0e-6
        ),
        "anchor_spacing_is_materially_nonuniform": (
            visual_warp["actual_anchor_spacing_meters"][
                "standard_deviation"
            ]
            > 1.0
        ),
        "raw_anchor_amplitude_is_materially_nonuniform": (
            visual_warp["actual_raw_anchor_amplitude_meters"][
                "standard_deviation"
            ]
            > 0.25
        ),
        "all_post_bias_segment_mean_to_rms_ratios_are_within_limit": (
            visual_warp["maximum_post_bias_segment_mean_to_rms_ratio"]
            <= SEGMENT_MEAN_TO_RMS_LIMIT + 1.0e-7
        ),
        "anchor_signs_mix_persistence_and_flips": (
            visual_warp["same_sign_interval_count"] > 0
            and visual_warp["sign_flip_interval_count"] > 0
        ),
        "visual_boundary_changes_only_the_boundary_halo": (
            visual_warp[
                "changed_pixels_outside_conservative_boundary_halo"
            ]
            == 0
        ),
        "visual_boundary_keeps_junctions_and_perimeter_fixed": (
            visual_warp[
                "changed_pixels_inside_protected_junction_or_perimeter_anchor"
            ]
            == 0
        ),
        "visual_boundary_segment_proposals_do_not_conflict": (
            visual_warp["proposal_conflict_pixels"] == 0
            and visual_warp["multiple_segment_claim_pixels"] == 0
        ),
        "visual_boundary_has_no_unexpected_skipped_segments": (
            visual_warp["skipped_segment_count"] == 0
        ),
        "visual_boundary_classifies_every_hard_adjacency": (
            visual_warp["classified_region_pair_count"]
            == visual_warp["hard_adjacency_pair_count"]
        ),
        "visual_boundary_region_area_change_is_below_half_percent": (
            visual_warp["maximum_relative_region_area_change_percent"] < 0.5
        ),
        "visual_warp_preserves_region_ids": (
            set(int(value) for value in np.unique(visual_primary))
            == set(range(REGION_COUNT))
        ),
        "visual_warp_preserves_region_adjacencies": (
            visual_adjacency == hard_adjacency
        ),
        "visual_warp_preserves_region_components": (
            visual_components == hard_components
        ),
        "visual_warp_preserves_texture_perimeter": (
            bool(np.array_equal(visual_primary[0, :], hard_primary[0, :]))
            and bool(np.array_equal(visual_primary[-1, :], hard_primary[-1, :]))
            and bool(np.array_equal(visual_primary[:, 0], hard_primary[:, 0]))
            and bool(np.array_equal(visual_primary[:, -1], hard_primary[:, -1]))
        ),
        "blend_width_field_repeat_is_exact": blend_width_repeat_exact,
        "blend_width_stays_inside_requested_range": bool(
            np.all(
                boundary_blend_radius[blend_boundary]
                >= BLEND_RADIUS_RANGE_TEXELS[0]
            )
            and np.all(
                boundary_blend_radius[blend_boundary]
                <= BLEND_RADIUS_RANGE_TEXELS[1]
            )
        ),
        "blend_width_maximum_respects_existing_32px_guard": (
            BLEND_RADIUS_RANGE_TEXELS[1]
            <= COLOR_GUARD_RADIUS_TEXELS
        ),
        "blend_width_is_materially_nonuniform": (
            blend_width_stats["actual_boundary_half_width_texels"][
                "standard_deviation"
            ]
            > 1.0
        ),
        "blend_width_anchor_spacings_stay_inside_requested_range": (
            blend_width_stats["actual_anchor_spacing_meters"]["minimum"]
            >= BLEND_WIDTH_ANCHOR_SPACING_METERS[0] - 1.0e-6
            and blend_width_stats["actual_anchor_spacing_meters"]["maximum"]
            <= BLEND_WIDTH_ANCHOR_SPACING_METERS[1] + 1.0e-6
        ),
        "blend_width_anchor_spacing_is_materially_nonuniform": (
            blend_width_stats["actual_anchor_spacing_meters"][
                "standard_deviation"
            ]
            > 1.0
        ),
        "blend_width_segment_endpoints_return_to_24px": (
            blend_width_stats["maximum_endpoint_deviation_texels"]
            <= 1.0e-6
        ),
        "blend_width_junction_and_perimeter_anchors_are_24px": bool(
            np.all(
                boundary_blend_radius[blend_protected]
                == BLEND_RADIUS_TEXELS
            )
        ),
        "blend_width_assigns_open_boundary_pixels": (
            blend_width_stats["assigned_boundary_pixels"] > 0
        ),
        "blend_width_has_no_unassigned_unprotected_boundary_pixels": (
            blend_width_stats["unassigned_unprotected_boundary_pixels"] == 0
        ),
        "guard_graph_is_four_colourable": minimum_colours is not None
        and minimum_colours <= SLOT_COUNT,
        "guard_graph_colouring_has_no_edge_conflicts": (
            colour_conflict_violations == 0
        ),
        "guard_graph_colouring_repeat_is_exact": (
            repeated_colours == selected_colours
        ),
        # Legacy key retained so downstream report comparisons remain stable.
        "same_slot_24px_support_overlap_is_zero": (
            overlap_stats["same_slot_support_overlap_pixels"] == 0
        ),
        "same_slot_32px_guard_overlap_is_zero": (
            overlap_stats["same_slot_guard_overlap_pixels"] == 0
        ),
        "normalized_float_weights_sum_to_one": normalized_sum_error <= 1.0e-6,
        "raw_maximum_active_regions_is_at_most_four": raw_max_active <= SLOT_COUNT,
        "quantized_maximum_active_regions_is_at_most_four": (
            quantized_max_active <= SLOT_COUNT
        ),
        "quantized_weight_byte_sum_is_exactly_255": (
            weight_sum_mismatch_pixels == 0
        ),
        "zero_float_influence_stays_zero_after_quantization": (
            zero_influence_became_nonzero == 0
        ),
        "active_slot_id_matches_weight_contributor": (
            active_id_mismatch_pixels == 0
        ),
        "every_region_has_positive_quantized_weight": all(
            count > 0 for count in region_weight_pixels.values()
        ),
        "per_slot_id_transitions_have_zero_weight_2x2_guard": (
            transition_guard_violations == 0
        ),
        "weight_quantization_repeat_is_exact": quantization_repeat_exact,
        "png_encoding_repeat_is_exact": png_encoding_repeat_exact,
        "written_weight_size_is_4096": written_weights_size == TARGET_SIZE,
        "written_id01_size_is_4096": written_ids_01_size == TARGET_SIZE,
        "written_id23_size_is_4096": written_ids_23_size == TARGET_SIZE,
        "written_id01_mode_is_g8": written_ids_01_mode == "L",
        "written_id23_mode_is_g8": written_ids_23_mode == "L",
        "written_weights_match_generated_array": bool(
            np.array_equal(written_weights, weight_bytes)
        ),
        "written_packed_ids01_match_generated_array": bool(
            np.array_equal(written_packed_ids_01, packed_ids_01)
        ),
        "written_packed_ids23_match_generated_array": bool(
            np.array_equal(written_packed_ids_23, packed_ids_23)
        ),
        "written_packed_id_pairs_decode_exactly": bool(
            np.array_equal(decoded_written_ids, slot_ids)
        ),
        "written_packed_id_pairs_have_no_reserved_15_nibble": (
            written_invalid_nibble_pixels == 0
        ),
        "all_g8_codes_roundtrip_through_normalized_float16": (
            float16_byte_decode["mismatch_codes"] == 0
            and float16_byte_decode["maximum_absolute_byte_error"] == 0
        ),
        "written_weight_byte_sum_is_exactly_255": (
            written_weight_sum_mismatch_pixels == 0
        ),
    }
    require(all(checks.values()), f"V2.2 validation failed: {checks}")

    report = {
        "format": {
            "weights": "RGBA8 fixed visual slots; bytes sum to 255 per texel",
            "ids_01": "G8 packed id0 | id1<<4",
            "ids_23": "G8 packed id2 | id3<<4",
            "id_decode": {
                "packed_01": "round(normalized_G8_ID01_sample * 255)",
                "packed_23": "round(normalized_G8_ID23_sample * 255)",
                "slot_0": "packed_01 % 16",
                "slot_1": "floor(packed_01 / 16)",
                "slot_2": "packed_23 % 16",
                "slot_3": "floor(packed_23 / 16)",
            },
            "recommended_unreal_import": {
                "weights": {
                    "srgb": False,
                    "compression": "VectorDisplacementmap / uncompressed RGBA8 baseline",
                    "filter": "Bilinear",
                    "mipmaps": "NoMipmaps",
                    "address_x_y": "Clamp",
                    "virtual_texture_streaming": False,
                    "note": "Do not use BC Masks for correctness validation; test BC7 only after baseline passes",
                },
                "ids_01_and_23": {
                    "srgb": False,
                    "compression": "Grayscale G8; verify runtime PF_G8",
                    "filter": "Nearest",
                    "mipmaps": "NoMipmaps",
                    "address_x_y": "Clamp",
                    "virtual_texture_streaming": False,
                    "precision": "normalized G8 byte decode is exact in float16",
                },
            },
        },
        "parameters": {
            "region_count": REGION_COUNT,
            "fixed_slot_count": SLOT_COUNT,
            "target_size": list(TARGET_SIZE),
            "blend_radius_texels_each_side": BLEND_RADIUS_TEXELS,
            "blend_radius_range_texels_each_side": list(
                BLEND_RADIUS_RANGE_TEXELS
            ),
            "guard_expansion_radius_texels_each_region": (
                COLOR_GUARD_RADIUS_TEXELS
            ),
            "guard_conflict_definition": (
                "edge when two independently 32px-expanded hard region masks overlap"
            ),
            "weight_kernel": (
                "smoothstep(1 - outside_distance / local_pair_half_width), "
                "compact at d>=local_pair_half_width"
            ),
            "visual_boundary_warp": visual_warp,
            "visual_boundary_blend_width": blend_width_stats,
        },
        "inputs": input_metadata,
        "hard_id_integrity": {
            "region_ids": sorted(
                int(value) for value in np.unique(hard_primary)
            ),
            "hard_primary_roundtrip_mismatch_pixels": int(
                np.count_nonzero(hard_primary != hard_roundtrip)
            ),
            "hard_adjacency_count": len(hard_adjacency),
            "hard_roundtrip_adjacency_count": len(roundtrip_adjacency),
            "hard_adjacency_pairs": [list(pair) for pair in sorted(hard_adjacency)],
            "hard_component_counts": hard_components,
            "hard_roundtrip_component_counts": roundtrip_components,
        },
        "visual_partition": {
            "region_ids": sorted(
                int(value) for value in np.unique(visual_primary)
            ),
            "adjacency_count": len(visual_adjacency),
            "adjacency_pairs": [
                list(pair) for pair in sorted(visual_adjacency)
            ],
            "component_counts": visual_components,
            "hard_to_visual_changed_pixels": visual_warp[
                "changed_label_pixels"
            ],
        },
        "guard_graph": {
            "edge_count": len(conflict_edges),
            "edges": [list(edge) for edge in conflict_edges],
            "degrees": {
                str(region_id): len(adjacency[region_id])
                for region_id in range(REGION_COUNT)
            },
            "expanded_pixels_per_region": expanded_pixels,
            "minimum_colours": minimum_colours,
            "region_to_fixed_slot": {
                str(region_id): selected_colours[region_id]
                for region_id in range(REGION_COUNT)
            },
            "slot_to_regions": slot_regions,
            "colour_conflict_violations": colour_conflict_violations,
        },
        "weights": {
            **overlap_stats,
            "normalized_float_maximum_sum_error": normalized_sum_error,
            "raw_maximum_active_regions": raw_max_active,
            "raw_active_region_count_histogram": raw_active_histogram,
            "quantized_maximum_active_regions": quantized_max_active,
            "quantized_active_region_count_histogram": (
                quantized_active_histogram
            ),
            "source_weight_sum_mismatch_pixels": weight_sum_mismatch_pixels,
            "written_weight_sum_mismatch_pixels": (
                written_weight_sum_mismatch_pixels
            ),
            "zero_influence_became_nonzero_pixels": (
                zero_influence_became_nonzero
            ),
            "active_slot_id_mismatch_pixels": active_id_mismatch_pixels,
            "positive_weight_pixels_per_region": region_weight_pixels,
        },
        "id_transition_safety": {
            "contract": (
                "when a slot ID changes, that same slot weight is zero in a conservative 3x3 halo covering every touching 2x2 footprint"
            ),
            "per_slot": transition_stats,
            "total_positive_weight_pixels_in_transition_halos": (
                transition_guard_violations
            ),
            "written_ids_01_mode": written_ids_01_mode,
            "written_ids_23_mode": written_ids_23_mode,
            "written_invalid_reserved_15_nibble_pixels": (
                written_invalid_nibble_pixels
            ),
            "float16_normalized_g8_decode": float16_byte_decode,
        },
        "determinism": {
            "graph_colouring_repeat_exact": repeated_colours == selected_colours,
            "blend_width_field_repeat_exact": blend_width_repeat_exact,
            "weight_quantization_repeat_exact": quantization_repeat_exact,
            "png_encoding_repeat_exact": png_encoding_repeat_exact,
        },
        "outputs": {
            "weights": str(OUTPUT_WEIGHTS),
            "ids_01": str(OUTPUT_IDS01),
            "ids_23": str(OUTPUT_IDS23),
            "report": str(OUTPUT_REPORT),
            "weights_sha256": sha256_bytes(weights_png),
            "ids_01_sha256": sha256_bytes(ids_01_png),
            "ids_23_sha256": sha256_bytes(ids_23_png),
            "generator_sha256": sha256_path(Path(__file__).resolve()),
            "weights_size": list(written_weights_size),
            "ids_01_size": list(written_ids_01_size),
            "ids_23_size": list(written_ids_23_size),
        },
        "checks": checks,
        "all_checks_passed": True,
    }
    report_text = json.dumps(report, indent=2, sort_keys=True) + "\n"
    OUTPUT_REPORT.write_text(report_text, encoding="utf-8")

    summary = {
        "all_checks_passed": True,
        "visual_boundary_warp": visual_warp,
        "visual_boundary_blend_width": blend_width_stats,
        "minimum_colours": minimum_colours,
        "region_to_fixed_slot": report["guard_graph"]["region_to_fixed_slot"],
        "minimum_same_slot_label_distance_texels": overlap_stats[
            "minimum_same_slot_label_distance_texels"
        ],
        "raw_active_histogram": raw_active_histogram,
        "quantized_active_histogram": quantized_active_histogram,
        "id_transition_guard_violations": transition_guard_violations,
        "weights_sha256": report["outputs"]["weights_sha256"],
        "ids_01_sha256": report["outputs"]["ids_01_sha256"],
        "ids_23_sha256": report["outputs"]["ids_23_sha256"],
        "report": str(OUTPUT_REPORT),
    }
    print(json.dumps(summary, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
