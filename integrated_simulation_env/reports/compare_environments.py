#!/usr/bin/env python3
"""Compare ISE results against the existing Raspberry Pi 4 and Windows
Intel Core i7 archives.

The thesis wants a side-by-side table:

    environment      | avg_ml_dsa_sign_us | avg_ml_dsa_verify_us |
    -----------------+-------------------+----------------------+
    ISE (full stack) |                   |                      |
    Pi4 (unit only)  |                   |                      |
    Win x86 (unit)   |                   |                      |

This script parses the human-readable ``PiTest/test_summary.txt`` and
``Autosar_SecOC/test_logs/phase*.txt`` files (regex) and the ISE JSON
summaries, then emits ``summary/cross_env_comparison.csv`` plus a
Markdown table.

Usage:
    python3 compare_environments.py \\
        --ise     results/<timestamp> \\
        --pi      ../PiTest \\
        --win     ../Autosar_SecOC/test_logs \\
        --output  results/<timestamp>/summary/cross_env_comparison.md
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Dict, List, Tuple


# Require a ':' after the metric name so we only match log lines of the
# form "ML-DSA-65 Sign:   10581.30 µs" and skip prose/tables that quote
# theoretical numbers like "~370 us" inside README-style text.
SIGN_RE = re.compile(
    r"ML[-_]?DSA[\w\- ]*Sign[^\n:]*:\s*(\d+[\d,.]*)\s*(us|µs)",
    re.IGNORECASE,
)
VERIFY_RE = re.compile(
    r"ML[-_]?DSA[\w\- ]*Verify[^\n:]*:\s*(\d+[\d,.]*)\s*(us|µs)",
    re.IGNORECASE,
)
KEMGEN_RE = re.compile(
    r"ML[-_]?KEM[\w\- ]*KeyGen[^\n:]*:\s*(\d+[\d,.]*)\s*(us|µs)",
    re.IGNORECASE,
)

# Lines containing a "~" prefix before the number are estimates from docs,
# not measurements — drop them.
APPROX_RE = re.compile(r"~\s*\d")


def grep_microseconds(path: Path, regex: re.Pattern) -> float | None:
    """Return the *median* of all measurement-like matches in the file.

    Using median rather than min avoids being dragged down by docstrings
    ("~370 us"); still stable against a single outlier measurement.
    """
    try:
        text = path.read_text(errors="ignore")
    except OSError:
        return None
    values: List[float] = []
    for line in text.splitlines():
        if APPROX_RE.search(line):
            continue
        m = regex.search(line)
        if not m:
            continue
        val = m.group(1).replace(",", "")
        try:
            values.append(float(val))
        except ValueError:
            pass
    if not values:
        return None
    values.sort()
    return values[len(values) // 2]


def scan_dir_for_value(dir_: Path, regex: re.Pattern) -> float | None:
    if not dir_.exists():
        return None
    samples: List[float] = []
    for p in dir_.rglob("*.txt"):
        v = grep_microseconds(p, regex)
        if v is not None:
            samples.append(v)
    if not samples:
        return None
    samples.sort()
    return samples[len(samples) // 2]


def load_ise_mean(ise_dir: Path, scenario: str, field: str) -> float | None:
    summ_dir = ise_dir / "summary"
    if not summ_dir.exists():
        summ_dir = ise_dir
    candidates = list(summ_dir.glob(f"{scenario}*_summary.json"))
    if not candidates:
        return None
    for c in candidates:
        try:
            data = json.loads(c.read_text())
        except Exception:
            continue
        h = data.get("histograms", {}).get(field, {})
        if h.get("count", 0) > 0:
            return float(h.get("mean_ns", 0)) / 1000.0
    return None


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--ise", required=True)
    parser.add_argument("--pi",  default=None)
    parser.add_argument("--win", default=None)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()

    ise_dir = Path(args.ise).resolve()
    out_md = Path(args.output).resolve()
    out_md.parent.mkdir(parents=True, exist_ok=True)

    # ML-DSA sign/verify come from the PQC baseline; ML-KEM keygen from rekey.
    ise_sign   = load_ise_mean(ise_dir, "baseline_pqc", "secoc_auth")   or \
                 load_ise_mean(ise_dir, "baseline",     "secoc_auth")
    ise_verify = load_ise_mean(ise_dir, "baseline_pqc", "secoc_verify") or \
                 load_ise_mean(ise_dir, "baseline",     "secoc_verify")
    ise_kem    = load_ise_mean(ise_dir, "rekey",        "secoc_auth")
    ise_e2e    = load_ise_mean(ise_dir, "baseline_pqc", "e2e_latency") or \
                 load_ise_mean(ise_dir, "baseline",     "e2e_latency")

    pi_sign  = scan_dir_for_value(Path(args.pi),  SIGN_RE)   if args.pi  else None
    pi_ver   = scan_dir_for_value(Path(args.pi),  VERIFY_RE) if args.pi  else None
    pi_kem   = scan_dir_for_value(Path(args.pi),  KEMGEN_RE) if args.pi  else None
    win_sign = scan_dir_for_value(Path(args.win), SIGN_RE)   if args.win else None
    win_ver  = scan_dir_for_value(Path(args.win), VERIFY_RE) if args.win else None
    win_kem  = scan_dir_for_value(Path(args.win), KEMGEN_RE) if args.win else None

    rows: List[Tuple[str, str, str, str, str]] = [
        ("environment", "ML-DSA Sign [us]", "ML-DSA Verify [us]",
         "ML-KEM KeyGen [us]", "end-to-end (ISE only) [us]"),
        ("ISE (full stack)",
         f"{ise_sign:.1f}" if ise_sign is not None else "n/a",
         f"{ise_verify:.1f}" if ise_verify is not None else "n/a",
         f"{ise_kem:.1f}" if ise_kem is not None else "n/a",
         f"{ise_e2e:.1f}" if ise_e2e is not None else "n/a"),
        ("Pi 4 unit-only",
         f"{pi_sign:.1f}" if pi_sign is not None else "n/a",
         f"{pi_ver:.1f}"  if pi_ver  is not None else "n/a",
         f"{pi_kem:.1f}"  if pi_kem  is not None else "n/a",
         "—"),
        ("x86_64 unit-only",
         f"{win_sign:.1f}" if win_sign is not None else "n/a",
         f"{win_ver:.1f}"  if win_ver  is not None else "n/a",
         f"{win_kem:.1f}"  if win_kem  is not None else "n/a",
         "—"),
    ]

    md = ["# Cross-Environment PQC Comparison\n",
          "The numbers below come from three distinct test campaigns. The "
          "ISE column is the only one where the full AUTOSAR stack "
          "(Com → PduR → SecOC → Csm → PQC → SoAd → virtual bus) was "
          "active; the other two are unit-level measurements kept here "
          "as a reference baseline for the thesis.\n"]
    md.append("| " + " | ".join(rows[0]) + " |")
    md.append("| " + " | ".join(["---"] * len(rows[0])) + " |")
    for r in rows[1:]:
        md.append("| " + " | ".join(r) + " |")
    out_md.write_text("\n".join(md) + "\n")

    # CSV sibling
    csv_path = out_md.with_suffix(".csv")
    csv_path.write_text("\n".join(",".join(r) for r in rows) + "\n")
    print(f"[compare] written {out_md}")
    print(f"[compare] written {csv_path}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
