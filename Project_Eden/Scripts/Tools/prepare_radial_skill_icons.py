"""Resize chroma-keyed radial skill icons into Unreal-ready source PNGs."""

from __future__ import annotations

import argparse
from pathlib import Path

from PIL import Image


SKILL_ICON_NAMES = (
    "BigHammer",
    "IceMist",
    "LightningStrike",
    "CrystalTorrent",
    "DarkSoloProjectile",
    "DarkStone",
    "MagmaShot",
)

OUTPUT_SIZE = (256, 256)


def project_root() -> Path:
    return Path(__file__).resolve().parents[2]


def prepare_icon(source: Path, destination: Path) -> None:
    with Image.open(source) as image:
        rgba = image.convert("RGBA")
        resized = rgba.resize(OUTPUT_SIZE, Image.Resampling.LANCZOS)

    destination.parent.mkdir(parents=True, exist_ok=True)
    resized.save(destination, format="PNG", optimize=True)

    with Image.open(destination) as saved:
        alpha = saved.getchannel("A")
        if saved.mode != "RGBA" or saved.size != OUTPUT_SIZE:
            raise RuntimeError(f"Invalid output format: {destination}")
        if alpha.getextrema() != (0, 255):
            raise RuntimeError(f"Output must contain transparent and opaque pixels: {destination}")
        if alpha.getpixel((0, 0)) != 0:
            raise RuntimeError(f"Top-left corner must be transparent: {destination}")


def main() -> None:
    root = project_root()
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--source-dir",
        type=Path,
        default=root / "Saved" / "CodexAutomation" / "SkillIconsChroma",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=root / "Content" / "UI" / "Asset" / "SkillIcons" / "Radial" / "Source",
    )
    args = parser.parse_args()

    for skill_name in SKILL_ICON_NAMES:
        source = args.source_dir / f"{skill_name}_rgba.png"
        destination = args.output_dir / f"T_SkillIcon_Radial_{skill_name}.png"
        if not source.is_file():
            raise FileNotFoundError(source)

        prepare_icon(source, destination)
        print(f"{skill_name}: {destination}")


if __name__ == "__main__":
    main()
