from __future__ import annotations

import argparse
import shutil
import zipfile
from pathlib import Path

from PIL import Image, ImageOps


APP_LIST_SIZES = [
    16, 20, 24, 30, 32, 36, 40, 48,
    60, 64, 72, 80, 96, 256
]

# Microsoft manifest scale dimensions.
SQUARE44_SCALES = {
    100: 44,
    125: 55,
    150: 66,
    200: 88,
    250: 110,
    300: 132,
    400: 176,
}

SQUARE150_SCALES = {
    100: 150,
    125: 188,
    150: 225,
    200: 300,
    250: 375,
    300: 450,
    400: 600,
}

# Store publishing requires these scale qualifiers.
STORE_LOGO_SCALES = {
    100: 50,
    125: 63,
    150: 75,
    200: 100,
    400: 200,
}

# Windows 11 does not use tiles, but Microsoft currently requires
# at least the Medium tile 100% asset for Store publishing.
# Generating the complete normal scale set costs us nothing.
MED_TILE_SCALES = {
    100: 150,
    125: 188,
    150: 225,
    200: 300,
    400: 600,
}

ICO_SIZES = [16, 24, 32, 48, 256]


def load_master(path: Path) -> Image.Image:
    img = Image.open(path).convert("RGBA")

    # Preserve the entire artwork. If someone gives us a non-square source,
    # center it on a transparent square rather than cropping it.
    if img.width != img.height:
        side = max(img.width, img.height)
        canvas = Image.new("RGBA", (side, side), (0, 0, 0, 0))
        contained = ImageOps.contain(
            img,
            (side, side),
            method=Image.Resampling.LANCZOS,
        )
        x = (side - contained.width) // 2
        y = (side - contained.height) // 2
        canvas.alpha_composite(contained, (x, y))
        img = canvas

    return img


def resize(master: Image.Image, size: int) -> Image.Image:
    return master.resize(
        (size, size),
        Image.Resampling.LANCZOS,
    )


def save_png(master: Image.Image, path: Path, size: int) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)

    img = resize(master, size)

    img.save(
        path,
        format="PNG",
        optimize=True,
    )


def generate_app_list(master: Image.Image, assets: Path) -> dict[Path, tuple[int, int]]:
    expected = {}

    for size in APP_LIST_SIZES:
        files = [
            f"AppList.targetsize-{size}.png",
            f"AppList.targetsize-{size}_altform-unplated.png",
            f"AppList.targetsize-{size}_altform-lightunplated.png",
        ]

        for filename in files:
            path = assets / filename
            save_png(master, path, size)
            expected[path] = (size, size)

    return expected


def generate_scaled_assets(
    master: Image.Image,
    assets: Path,
    base_name: str,
    scales: dict[int, int],
    include_base_file: bool = True,
) -> dict[Path, tuple[int, int]]:
    expected = {}

    if include_base_file:
        base_size = scales[100]
        base_path = assets / f"{base_name}.png"

        save_png(master, base_path, base_size)
        expected[base_path] = (base_size, base_size)

    for scale, size in scales.items():
        path = assets / f"{base_name}.scale-{scale}.png"

        save_png(master, path, size)
        expected[path] = (size, size)

    return expected


def generate_ico(master: Image.Image, output: Path) -> Path:
    ico_path = output / "HeadsUp.ico"

    # ICO supports up to 256x256. Build from a clean 256 master.
    ico_master = resize(master, 256)

    ico_master.save(
        ico_path,
        format="ICO",
        sizes=[(size, size) for size in ICO_SIZES],
    )

    return ico_path


def validate_pngs(expected: dict[Path, tuple[int, int]]) -> None:
    errors = []

    for path, expected_size in expected.items():
        if not path.exists():
            errors.append(f"MISSING: {path.name}")
            continue

        try:
            with Image.open(path) as img:
                if img.size != expected_size:
                    errors.append(
                        f"WRONG SIZE: {path.name}: "
                        f"{img.size} != {expected_size}"
                    )

                if img.mode not in ("RGBA", "LA", "P"):
                    errors.append(
                        f"NO ALPHA-CAPABLE MODE: "
                        f"{path.name}: {img.mode}"
                    )

        except Exception as exc:
            errors.append(f"INVALID IMAGE: {path.name}: {exc}")

    if errors:
        print("\nVALIDATION FAILED")
        print("=" * 60)

        for error in errors:
            print(error)

        raise SystemExit(1)


def make_zip(output: Path) -> Path:
    zip_path = output.parent / f"{output.name}.zip"

    if zip_path.exists():
        zip_path.unlink()

    with zipfile.ZipFile(
        zip_path,
        "w",
        compression=zipfile.ZIP_DEFLATED,
    ) as archive:
        for file in output.rglob("*"):
            if file.is_file():
                archive.write(
                    file,
                    file.relative_to(output),
                )

    return zip_path


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Generate Microsoft Windows / Store app icon assets."
    )

    parser.add_argument(
        "source",
        type=Path,
        help="Master PNG/JPG image.",
    )

    parser.add_argument(
        "-o",
        "--output",
        type=Path,
        default=Path("HeadsUp-Windows-Assets"),
        help="Output directory.",
    )

    args = parser.parse_args()

    source: Path = args.source.resolve()
    output: Path = args.output.resolve()
    assets = output / "Assets"

    if not source.exists():
        raise SystemExit(f"Source image does not exist: {source}")

    if output.exists():
        shutil.rmtree(output)

    assets.mkdir(parents=True, exist_ok=True)

    master = load_master(source)

    print(f"Source: {source}")
    print(f"Master: {master.width}x{master.height}")
    print()

    expected: dict[Path, tuple[int, int]] = {}

    # Canonical master copied into the output set.
    canonical = output / "headsup-logo.png"
    master.save(canonical, "PNG", optimize=True)

    print("Generating Win32 ICO...")
    ico_path = generate_ico(master, output)

    print("Generating AppList target-size assets...")
    expected.update(
        generate_app_list(master, assets)
    )

    print("Generating Square44x44Logo assets...")
    expected.update(
        generate_scaled_assets(
            master,
            assets,
            "Square44x44Logo",
            SQUARE44_SCALES,
        )
    )

    print("Generating Square150x150Logo assets...")
    expected.update(
        generate_scaled_assets(
            master,
            assets,
            "Square150x150Logo",
            SQUARE150_SCALES,
        )
    )

    print("Generating Microsoft Store logo assets...")
    expected.update(
        generate_scaled_assets(
            master,
            assets,
            "StoreLogo",
            STORE_LOGO_SCALES,
        )
    )

    print("Generating Medium tile assets...")
    expected.update(
        generate_scaled_assets(
            master,
            assets,
            "MedTile",
            MED_TILE_SCALES,
            include_base_file=False,
        )
    )

    print("Validating PNG dimensions...")
    validate_pngs(expected)

    print("Creating ZIP...")
    zip_path = make_zip(output)

    print()
    print("=" * 60)
    print("WINDOWS ASSET GENERATION COMPLETE")
    print("=" * 60)
    print(f"PNG assets : {len(expected)}")
    print(f"ICO        : {ico_path.name}")
    print(f"Output     : {output}")
    print(f"ZIP        : {zip_path}")
    print()
    print("AppList variants:")
    print(f"  {len(APP_LIST_SIZES)} default")
    print(f"  {len(APP_LIST_SIZES)} dark/unplated")
    print(f"  {len(APP_LIST_SIZES)} light/unplated")
    print()
    print("ICO sizes:")
    print("  " + ", ".join(map(str, ICO_SIZES)))


if __name__ == "__main__":
    main()
