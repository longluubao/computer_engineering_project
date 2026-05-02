#!/usr/bin/env python3
"""
export_diagrams.py — Extract every Mermaid block from the docs/ markdown
files and render each to PNG + SVG with a meaningful filename.

Usage:
    python3 export_diagrams.py                  # render PNG + SVG (default)
    python3 export_diagrams.py --format png     # PNG only
    python3 export_diagrams.py --format svg     # SVG only (smaller, vector for LaTeX)
    python3 export_diagrams.py --format pdf     # PDF (one file per diagram)

Filenames follow the pattern:
    docs/images/<NN>-<slug-of-section-heading>.png
e.g. docs/images/22-complete-module-inventory-current-codebase.png

Requires:
    Node.js (already present on this machine)
    Mermaid CLI — installed on first run via `npx -p @mermaid-js/mermaid-cli`
    No global install needed.

The script is idempotent: re-running overwrites existing images.
"""

import argparse
import re
import shutil
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent
SRC_FILES = [
    REPO_ROOT / "docs" / "DIAGRAMS.md",
    REPO_ROOT / "docs" / "TEST_SCENARIOS_AND_RESULTS.md",
]
OUT_DIR = REPO_ROOT / "docs" / "images"

HEADING_RE = re.compile(r"^(#{1,6})\s+(.+?)\s*$")
MERMAID_OPEN = re.compile(r"^```mermaid\s*$")
MERMAID_CLOSE = re.compile(r"^```\s*$")


def slugify(text: str, max_len: int = 60) -> str:
    text = re.sub(r"[^\w\s-]", "", text).strip().lower()
    text = re.sub(r"[\s_-]+", "-", text)
    return text[:max_len].strip("-") or "diagram"


def find_mermaid_blocks(md_path: Path):
    """Yield (idx, heading_used, mermaid_source) for every block in the file."""
    lines = md_path.read_text(encoding="utf-8").splitlines()
    last_heading = md_path.stem
    block_idx = 0
    i = 0
    while i < len(lines):
        h = HEADING_RE.match(lines[i])
        if h:
            last_heading = h.group(2)
            i += 1
            continue
        if MERMAID_OPEN.match(lines[i]):
            i += 1
            buf = []
            while i < len(lines) and not MERMAID_CLOSE.match(lines[i]):
                buf.append(lines[i])
                i += 1
            i += 1  # skip closing fence
            block_idx += 1
            yield block_idx, last_heading, "\n".join(buf) + "\n"
            continue
        i += 1


def ensure_mmdc():
    """Verify that npx + mermaid-cli will work. Pre-warm the package cache."""
    if shutil.which("npx") is None:
        sys.exit("ERROR: npx not found. Install Node.js (>= 16) first.")
    print("Pre-fetching @mermaid-js/mermaid-cli (first run only, ~150 MB) ...")
    subprocess.run(
        ["npx", "--yes", "-p", "@mermaid-js/mermaid-cli", "mmdc", "--version"],
        check=True,
    )


def write_puppeteer_config() -> Path:
    """Puppeteer requires --no-sandbox when running as root or in a container.
    Writing a tiny config keeps the script portable across hosts."""
    cfg = OUT_DIR / ".puppeteer-config.json"
    cfg.write_text('{"args": ["--no-sandbox", "--disable-setuid-sandbox"]}\n')
    return cfg


def render(mmd_file: Path, out_file: Path, fmt: str, puppeteer_cfg: Path):
    """Render mmd → image via mmdc. Background white so it embeds cleanly."""
    cmd = [
        "npx", "--yes", "-p", "@mermaid-js/mermaid-cli", "mmdc",
        "-i", str(mmd_file),
        "-o", str(out_file),
        "-b", "white",
        "-t", "default",
        "--puppeteerConfigFile", str(puppeteer_cfg),
    ]
    if fmt == "png":
        cmd += ["--scale", "2"]  # 2x DPI for sharp embedding
    subprocess.run(cmd, check=True)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument(
        "--format",
        choices=["png", "svg", "pdf", "all"],
        default="all",
        help="Output format (default: all = png + svg)",
    )
    ap.add_argument(
        "--clean",
        action="store_true",
        help="Wipe docs/images/ before rendering",
    )
    args = ap.parse_args()

    if args.clean and OUT_DIR.exists():
        shutil.rmtree(OUT_DIR)
    OUT_DIR.mkdir(parents=True, exist_ok=True)

    ensure_mmdc()
    puppeteer_cfg = write_puppeteer_config()

    formats = ["png", "svg"] if args.format == "all" else [args.format]

    total = 0
    failed = []
    seen_names = {}
    for src in SRC_FILES:
        if not src.exists():
            print(f"SKIP (not found): {src}")
            continue
        print(f"\nScanning {src.relative_to(REPO_ROOT)} ...")
        for idx, heading, body in find_mermaid_blocks(src):
            slug = slugify(heading)
            base = f"{total + 1:02d}-{slug}"

            # Disambiguate if the same heading produced multiple diagrams
            if base in seen_names:
                seen_names[base] += 1
                base = f"{base}-{seen_names[base]}"
            else:
                seen_names[base] = 1

            mmd_file = OUT_DIR / f"{base}.mmd"
            mmd_file.write_text(body, encoding="utf-8")

            for fmt in formats:
                out_file = OUT_DIR / f"{base}.{fmt}"
                print(f"  -> {out_file.relative_to(REPO_ROOT)}")
                try:
                    render(mmd_file, out_file, fmt, puppeteer_cfg)
                except subprocess.CalledProcessError:
                    print(f"     FAILED — kept .mmd source for manual fix")
                    failed.append(f"{base}.{fmt}")

            total += 1

    print(f"\nDONE. {total} diagram(s) processed, "
          f"{total * len(formats) - len(failed)} image(s) written, "
          f"{len(failed)} failed.")
    if failed:
        print("Failed renders:")
        for n in failed:
            print(f"  {n}")
    print("Files written:")
    for p in sorted(OUT_DIR.iterdir()):
        print(f"  {p.relative_to(REPO_ROOT)}")


if __name__ == "__main__":
    main()
