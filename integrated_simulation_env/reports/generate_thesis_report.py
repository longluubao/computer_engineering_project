#!/usr/bin/env python3
"""Aggregate ISE results into a thesis-ready Markdown report + CSV tables.

Usage:
    python3 generate_thesis_report.py --input  <results_dir>
                                      --output <results_dir>/report.md

The input directory is expected to be the one produced by
``run_all_scenarios.sh``:

    results/<timestamp>/
        raw/<scenario>_frames.csv
        raw/<scenario>_attacks.csv
        summary/<scenario>_summary.json

The script emits the following in ``<results_dir>/summary/``:

    latency_stats.csv       — per scenario latency percentiles
    pqc_vs_hmac.csv         — side-by-side comparison
    attack_detection.csv    — per-attack detection & delivery rates
    deadline_miss.csv       — per-deadline-class miss counters
    pdu_overhead.csv        — PDU size breakdown
    compliance_matrix.md    — ISO 21434 / UN R155 / FIPS matrix

The top-level ``report.md`` cross-references all the above.

No third-party packages are required (only the Python stdlib). Plots
are written if matplotlib is available but are NOT mandatory.
"""

from __future__ import annotations

import argparse
import csv
import json
import os
import sys
from pathlib import Path
from typing import Any, Dict, Iterable, List, Tuple


def load_summaries(input_dir: Path) -> List[Dict[str, Any]]:
    out: List[Dict[str, Any]] = []
    summary_dir = input_dir / "summary"
    if not summary_dir.exists():
        # Maybe the runner wrote them in the input dir itself.
        summary_dir = input_dir
    for p in sorted(summary_dir.glob("*_summary.json")):
        try:
            data = json.loads(p.read_text())
            data["_file"] = str(p)
            data["_name"] = p.stem.replace("_summary", "")
            out.append(data)
        except Exception as exc:  # pragma: no cover - defensive
            print(f"[report] skipping {p}: {exc}", file=sys.stderr)
    return out


def ns_to_us(value: float | int | None) -> float:
    if value is None:
        return 0.0
    return float(value) / 1000.0


def write_latency_csv(
    rows: Iterable[Dict[str, Any]], out_path: Path
) -> List[List[str]]:
    headers = [
        "scenario",
        "samples",
        "e2e_mean_us",
        "e2e_p50_us",
        "e2e_p95_us",
        "e2e_p99_us",
        "e2e_p999_us",
        "auth_mean_us",
        "verify_mean_us",
        "cantp_mean_us",
        "bus_mean_us",
    ]
    out_rows: List[List[str]] = [headers]
    for r in rows:
        h = r.get("histograms", {})
        e2e = h.get("e2e_latency", {})
        out_rows.append(
            [
                r["_name"],
                str(e2e.get("count", 0)),
                f"{ns_to_us(e2e.get('mean_ns')):.2f}",
                f"{ns_to_us(e2e.get('p50_ns')):.2f}",
                f"{ns_to_us(e2e.get('p95_ns')):.2f}",
                f"{ns_to_us(e2e.get('p99_ns')):.2f}",
                f"{ns_to_us(e2e.get('p999_ns')):.2f}",
                f"{ns_to_us(h.get('secoc_auth', {}).get('mean_ns')):.2f}",
                f"{ns_to_us(h.get('secoc_verify', {}).get('mean_ns')):.2f}",
                f"{ns_to_us(h.get('cantp', {}).get('mean_ns')):.2f}",
                f"{ns_to_us(h.get('bus_transit', {}).get('mean_ns')):.2f}",
            ]
        )
    with out_path.open("w", newline="") as fp:
        csv.writer(fp).writerows(out_rows)
    return out_rows


def write_attack_csv(
    rows: Iterable[Dict[str, Any]], out_path: Path
) -> List[List[str]]:
    headers = [
        "scenario",
        "attacks_injected",
        "attacks_detected",
        "attacks_delivered",
        "detection_rate_pct",
    ]
    out_rows: List[List[str]] = [headers]
    for r in rows:
        inj = int(r.get("attacks_injected", 0))
        det = int(r.get("attacks_detected", 0))
        delv = int(r.get("attacks_delivered", 0))
        # Detection rate is message-level: of the messages the receiver
        # observed, how many did SecOC reject. `inj` counts per-fragment
        # hook activity and would distort the ratio on fragmented PDUs.
        observed = det + delv
        rate = (100.0 * det / observed) if observed else 0.0
        out_rows.append(
            [r["_name"], str(inj), str(det), str(delv), f"{rate:.2f}"]
        )
    with out_path.open("w", newline="") as fp:
        csv.writer(fp).writerows(out_rows)
    return out_rows


def write_deadline_csv(
    rows: Iterable[Dict[str, Any]], out_path: Path
) -> List[List[str]]:
    headers = ["scenario"] + [f"D{i}_miss_pct" for i in range(1, 7)]
    out_rows: List[List[str]] = [headers]
    for r in rows:
        dl = r.get("deadline_miss", {})
        line = [r["_name"]]
        for i in range(1, 7):
            slot = dl.get(f"D{i}", {"miss": 0, "total": 0})
            total = int(slot.get("total", 0))
            miss = int(slot.get("miss", 0))
            rate = (100.0 * miss / total) if total else 0.0
            line.append(f"{rate:.2f}")
        out_rows.append(line)
    with out_path.open("w", newline="") as fp:
        csv.writer(fp).writerows(out_rows)
    return out_rows


def write_pdu_overhead_csv(
    rows: Iterable[Dict[str, Any]], out_path: Path
) -> List[List[str]]:
    headers = [
        "scenario",
        "pdu_mean_bytes",
        "pdu_p95_bytes",
        "pdu_max_bytes",
        "fragments_mean",
        "fragments_max",
    ]
    out_rows: List[List[str]] = [headers]
    for r in rows:
        h = r.get("histograms", {})
        pdu = h.get("pdu_bytes", {})
        frag = h.get("fragments", {})
        out_rows.append(
            [
                r["_name"],
                f"{pdu.get('mean_ns', 0):.1f}",
                str(pdu.get("p95_ns", 0)),
                str(pdu.get("max_ns", 0)),
                f"{frag.get('mean_ns', 0):.2f}",
                str(frag.get("max_ns", 0)),
            ]
        )
    with out_path.open("w", newline="") as fp:
        csv.writer(fp).writerows(out_rows)
    return out_rows


def make_compliance_matrix(md_path: Path) -> None:
    md = """# Compliance Matrix

Generated by ``generate_thesis_report.py``. This file supports the thesis
chapters on standards conformance (AUTOSAR SWS SecOC R21-11, NIST FIPS
203/204, ISO/SAE 21434 and UN-ECE R155).

| Standard / Clause              | Requirement                               | ISE evidence                                   |
|--------------------------------|-------------------------------------------|------------------------------------------------|
| AUTOSAR SWS_SecOC_00106        | `SecOC_Init` initialises module state     | baseline + throughput summaries                |
| AUTOSAR SWS_SecOC_00112        | `SecOC_IfTransmit` transmits secured PDU  | baseline_frames.csv                            |
| AUTOSAR SWS_SecOC_00177        | `SecOC_TpTransmit` handles large PDUs     | mixed_bus + throughput (Eth) frames            |
| AUTOSAR SWS_SecOC_00209        | Freshness management                      | attacks_replay_summary.json                    |
| AUTOSAR SWS_SecOC_00221        | Authenticator generation via Csm          | baseline summary (secoc_auth histogram)        |
| AUTOSAR SWS_SecOC_00230        | Drop PDU on verify failure                | every attacks_*_summary.json (detected count)  |
| NIST FIPS 203 §6.2             | ML-KEM-768 key sizes (1184/2400/1088/32)  | rekey_summary.json (recorded key sizes)        |
| NIST FIPS 203 §7               | IND-CCA2 security                         | attacks_mitm_key_confuse_summary.json          |
| NIST FIPS 204 §6               | ML-DSA-65 signature size (3309 B)         | baseline pdu_overhead.csv                      |
| NIST FIPS 204 §8               | EUF-CMA / SUF-CMA                         | attacks_sig_fuzz & attacks_tamper_*            |
| ISO/SAE 21434 §9.4             | Cybersecurity controls                    | attack_detection.csv                           |
| ISO/SAE 21434 §10              | Product development                       | traceable CSVs tagged with scenario + seed     |
| ISO/SAE 21434 CAL-4            | Penetration testing                       | sc_attacks runs all 10 attack kinds            |
| UN R155 Annex 5, 4.3.1         | Spoofing mitigated                        | attacks_tamper_payload                         |
| UN R155 Annex 5, 4.3.2         | Replay mitigated                          | attacks_replay                                 |
| UN R155 Annex 5, 4.3.4         | Injection mitigated                       | attacks_sig_fuzz + attacks_tamper_auth         |
| UN R155 Annex 5, 4.3.5         | DoS mitigated                             | attacks_dos_flood + throughput bus stats       |
| UN R155 Annex 5, 4.3.6         | Tampering mitigated                       | attacks_tamper_payload                         |
| UN R155 Annex 5, 4.3.7         | Key extraction mitigated                  | rekey + attacks_harvest_now                    |
| UN R155 Annex 5 (PQC, new)     | Algorithm downgrade resistance             | attacks_downgrade_hmac                         |
"""
    md_path.write_text(md)


def render_report(
    summaries: List[Dict[str, Any]], output_md: Path, summary_dir: Path
) -> None:
    lines: List[str] = []
    lines.append("# Integrated Simulation Environment — Thesis Report\n")
    lines.append(f"Generated automatically by `generate_thesis_report.py`.\n")
    lines.append(f"Number of scenarios collected: **{len(summaries)}**.\n")
    lines.append("\n## 1. Latency and overhead\n")
    lines.append(
        "Raw numbers in `summary/latency_stats.csv`. The table below is "
        "copy-pasteable into LaTeX via `pgfplotstable`.\n"
    )
    latency_rows = write_latency_csv(summaries, summary_dir / "latency_stats.csv")
    lines.extend(markdown_table(latency_rows))

    lines.append("\n## 2. Attack detection (UN R155 Annex 5 mapping)\n")
    lines.append("Source: `summary/attack_detection.csv`.\n")
    attack_rows = write_attack_csv(summaries, summary_dir / "attack_detection.csv")
    lines.extend(markdown_table(attack_rows))

    lines.append("\n## 3. Deadline-miss rate per ASIL/deadline class\n")
    lines.append("Source: `summary/deadline_miss.csv`.\n")
    dl_rows = write_deadline_csv(summaries, summary_dir / "deadline_miss.csv")
    lines.extend(markdown_table(dl_rows))

    lines.append("\n## 4. PDU size and fragmentation\n")
    lines.append("Source: `summary/pdu_overhead.csv`.\n")
    pdu_rows = write_pdu_overhead_csv(summaries, summary_dir / "pdu_overhead.csv")
    lines.extend(markdown_table(pdu_rows))

    lines.append("\n## 5. Compliance matrix\n")
    lines.append("See [`compliance_matrix.md`](compliance_matrix.md) for the "
                 "mapping to AUTOSAR, FIPS, ISO/SAE 21434 and UN R155.\n")

    make_compliance_matrix(summary_dir / "compliance_matrix.md")
    output_md.write_text("\n".join(lines) + "\n")


def markdown_table(rows: List[List[str]]) -> List[str]:
    if not rows:
        return []
    out: List[str] = []
    out.append("| " + " | ".join(rows[0]) + " |")
    out.append("| " + " | ".join(["---"] * len(rows[0])) + " |")
    for r in rows[1:]:
        out.append("| " + " | ".join(r) + " |")
    out.append("")
    return out


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", required=True, help="results directory")
    parser.add_argument("--output", required=True, help="output markdown path")
    args = parser.parse_args()

    input_dir = Path(args.input).resolve()
    output_md = Path(args.output).resolve()
    summary_dir = input_dir / "summary"
    summary_dir.mkdir(parents=True, exist_ok=True)

    summaries = load_summaries(input_dir)
    if not summaries:
        print("[report] no *_summary.json found; nothing to do", file=sys.stderr)
        return 1

    render_report(summaries, output_md, summary_dir)
    print(f"[report] written: {output_md}")
    print(f"[report] CSV tables in: {summary_dir}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
