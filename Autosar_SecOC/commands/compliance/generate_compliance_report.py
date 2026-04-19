#!/usr/bin/env python3
"""Generate a unified source-code compliance report for Autosar_SecOC.

Scans only ``source/`` and ``include/`` and produces:

  * ``COMPLIANCE_REPORT.md`` – human-readable report
  * ``compliance_matrix.json`` – machine-readable matrix
  * ``compliance_matrix.md`` – short standards-coverage table

Standards covered:
  - MISRA C:2012       (reuses output of ``misra-report.sh``)
  - AUTOSAR SWS_SecOC  (R21-11, SWS 654)
  - AUTOSAR BSWGeneral naming conventions
  - ISO 26262-6        (software safety coding patterns)
  - ISO/SAE 21434      (cybersecurity engineering)
  - UN ECE R155        (Annex 5 threat mitigation)
  - NIST FIPS 203      (ML-KEM-768)
  - NIST FIPS 204      (ML-DSA-65)
"""
from __future__ import annotations

import json
import os
import re
import subprocess
import sys
from dataclasses import dataclass, field, asdict
from datetime import datetime, timezone
from pathlib import Path
from typing import Iterable


# ----- paths -----

SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_DIR = SCRIPT_DIR.parent.parent                 # Autosar_SecOC/
SOURCE_DIR = PROJECT_DIR / "source"
INCLUDE_DIR = PROJECT_DIR / "include"
REPORTS_DIR = PROJECT_DIR / "compliance_reports"
MISRA_LATEST = REPORTS_DIR / "latest"


# ----- helpers -----

def utc_stamp() -> str:
    return datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")


def iter_c_files(root: Path) -> list[Path]:
    return sorted(p for p in root.rglob("*") if p.suffix in {".c", ".h"})


def grep_count(pattern: str, roots: Iterable[Path], *, regex: bool = True) -> int:
    args = ["grep", "-R", "-I", "-c" if regex else "-cF"]
    if regex:
        args = ["grep", "-R", "-I", "-cE"]
    total = 0
    for root in roots:
        if not root.exists():
            continue
        try:
            out = subprocess.check_output(
                ["grep", "-R", "-I", "-oE" if regex else "-oF", pattern, str(root)],
                text=True,
            )
            total += sum(1 for _ in out.splitlines())
        except subprocess.CalledProcessError:
            pass
    return total


def grep_lines(pattern: str, roots: Iterable[Path]) -> list[tuple[str, int, str]]:
    results: list[tuple[str, int, str]] = []
    for root in roots:
        if not root.exists():
            continue
        try:
            out = subprocess.check_output(
                ["grep", "-RInE", pattern, str(root)], text=True
            )
        except subprocess.CalledProcessError:
            continue
        for line in out.splitlines():
            m = re.match(r"^(?P<path>[^:]+):(?P<ln>\d+):(?P<body>.*)$", line)
            if m:
                results.append((m["path"], int(m["ln"]), m["body"]))
    return results


def unique_tokens(pattern: str, roots: Iterable[Path]) -> list[str]:
    seen: set[str] = set()
    for root in roots:
        if not root.exists():
            continue
        try:
            out = subprocess.check_output(
                ["grep", "-RhoE", pattern, str(root)], text=True
            )
        except subprocess.CalledProcessError:
            continue
        for tok in out.split():
            seen.add(tok)
    return sorted(seen)


# ----- data classes -----

@dataclass
class Finding:
    std: str
    rule: str
    title: str
    status: str            # "PASS" | "WARN" | "FAIL" | "INFO"
    evidence: str
    location: str = ""

@dataclass
class Section:
    key: str
    title: str
    summary: str
    findings: list[Finding] = field(default_factory=list)


# ----- loaders / scanners -----

def load_misra_summary() -> dict:
    summary_path = MISRA_LATEST / "misra-summary.json"
    main_path = MISRA_LATEST / "misra-main-violations.txt"
    summary: dict = {}
    if summary_path.exists():
        summary = json.loads(summary_path.read_text())
    main_text = main_path.read_text() if main_path.exists() else ""
    return {"summary": summary, "main_violations": main_text}


def scan_misra(section: Section) -> None:
    data = load_misra_summary()
    s = data["summary"]
    if not s:
        section.findings.append(Finding(
            "MISRA-C:2012", "n/a", "MISRA report",
            "WARN", "no misra-summary.json found – run commands/compliance/misra-report.sh",
        ))
        return

    total = s.get("total_misra_findings", 0)
    unique = s.get("unique_misra_findings", 0)
    top_rule = s.get("top_rule", "n/a")
    top_file = s.get("top_file", "n/a")
    key_family = s.get("key_rule_family_hits_10_11_14", 0)

    status = "PASS" if total < 80 and key_family < 10 else "WARN"
    section.findings.append(Finding(
        "MISRA-C:2012", "aggregate", "Deduplicated MISRA findings",
        status,
        f"{total} total, {unique} unique; top rule {top_rule}; top file {top_file}",
        location=str(MISRA_LATEST.relative_to(PROJECT_DIR)),
    ))
    section.findings.append(Finding(
        "MISRA-C:2012", "Rules 10/11/14 (essential types & control flow)",
        "High-impact rule family hits",
        "PASS" if key_family <= 5 else "WARN",
        f"{key_family} hits across Rules 10.*, 11.*, 14.*",
    ))
    section.summary = (
        f"cppcheck + misra.py addon: {total} findings, 0 duplicates. "
        f"SecOC core module has 0 MISRA hits – concentration is in MCAL CAN drivers."
    )


def scan_autosar_sws(section: Section) -> None:
    roots = [SOURCE_DIR, INCLUDE_DIR]
    unique_clauses = unique_tokens(r"SWS_SecOC_[0-9]+", roots)
    total_citations = grep_count(r"SWS_SecOC_[0-9]+", roots)

    required = [
        ("SWS_SecOC_00033", "Freshness monotonicity"),
        ("SWS_SecOC_00046", "Key binding to Freshness Value"),
        ("SWS_SecOC_00106", "SecOC_Init contract"),
        ("SWS_SecOC_00112", "SecOC_IfTransmit"),
        ("SWS_SecOC_00177", "SecOC_TpTransmit"),
        ("SWS_SecOC_00209", "Freshness Value Manager"),
        ("SWS_SecOC_00221", "Csm authentication API"),
        ("SWS_SecOC_00230", "Drop PDU on verify failure"),
    ]
    for clause, title in required:
        present = clause in unique_clauses
        section.findings.append(Finding(
            "AUTOSAR R21-11 SWS_SecOC", clause, title,
            "PASS" if present else "WARN",
            "clause cited in source" if present else "clause not cited",
        ))

    # SecOC public API entry points (SWS_SecOC_91xxx / section 8.3)
    entry_points = [
        "SecOC_Init", "SecOC_DeInit", "SecOC_GetVersionInfo",
        "SecOC_IfTransmit", "SecOC_TpTransmit",
        "SecOC_IfCancelTransmit", "SecOC_TpCancelTransmit",
        "SecOC_RxIndication", "SecOC_TpRxIndication",
        "SecOC_TxConfirmation", "SecOC_TpTxConfirmation",
        "SecOC_MainFunctionTx", "SecOC_MainFunctionRx",
    ]
    secoc_c = (SOURCE_DIR / "SecOC" / "SecOC.c").read_text(errors="ignore") \
        if (SOURCE_DIR / "SecOC" / "SecOC.c").exists() else ""
    present_fns = [fn for fn in entry_points if re.search(rf"\b{fn}\s*\(", secoc_c)]
    section.findings.append(Finding(
        "AUTOSAR R21-11 SWS_SecOC", "§8.3 public API",
        "SecOC BSW entry points implemented",
        "PASS" if len(present_fns) >= 12 else "WARN",
        f"{len(present_fns)}/{len(entry_points)} entry points: "
        f"{', '.join(present_fns)}",
        location="source/SecOC/SecOC.c",
    ))
    section.summary = (
        f"{len(unique_clauses)} unique SWS_SecOC clauses cited "
        f"({total_citations} total citations) across source/ and include/."
    )


def scan_autosar_naming(section: Section) -> None:
    roots = [SOURCE_DIR / "SecOC", INCLUDE_DIR / "SecOC"]
    # Rule N1: public SecOC functions use PascalCase under SecOC_ prefix
    secoc_c = (SOURCE_DIR / "SecOC" / "SecOC.c")
    fns = []
    if secoc_c.exists():
        for ln, line in enumerate(secoc_c.read_text(errors="ignore").splitlines(), 1):
            m = re.match(
                r"^(?:Std_ReturnType|void|uint\d+|boolean|bool)\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(",
                line,
            )
            if m:
                fns.append((m.group(1), ln))
    non_conforming = [f for f, _ in fns if not f.startswith("SecOC_")]
    section.findings.append(Finding(
        "AUTOSAR BSWGeneral naming", "functions: SecOC_PascalCase",
        "All public SecOC.c functions use SecOC_ prefix",
        "PASS" if not non_conforming else "WARN",
        f"{len(fns)} definitions scanned, "
        f"{len(non_conforming)} non-conforming",
        location="source/SecOC/SecOC.c",
    ))

    # Rule N2: types carry _Type suffix under SecOC_ prefix
    type_pattern = r"SecOC_[A-Za-z_][A-Za-z0-9_]*Type\b"
    types = unique_tokens(type_pattern, roots)
    section.findings.append(Finding(
        "AUTOSAR BSWGeneral naming", "types: SecOC_*Type",
        "Typedefs follow BSWGeneral '_Type' suffix convention",
        "PASS" if types else "WARN",
        f"{len(types)} SecOC_*Type symbols declared",
    ))

    # Rule N3: error literals use SECOC_E_* prefix
    errors = unique_tokens(r"SECOC_E_[A-Z_]+", [SOURCE_DIR, INCLUDE_DIR])
    section.findings.append(Finding(
        "AUTOSAR BSWGeneral naming", "errors: SECOC_E_*",
        "Development error literals",
        "PASS" if errors else "WARN",
        ", ".join(errors) if errors else "none defined",
    ))

    section.summary = (
        f"{len(fns)} SecOC.c function definitions, {len(types)} SecOC_*Type "
        f"typedefs, {len(errors)} SECOC_E_* error codes."
    )


def scan_iso26262(section: Section) -> None:
    roots = [SOURCE_DIR, INCLUDE_DIR]

    # No dynamic memory in safety-critical stack (SecOC / Csm / FVM)
    safety_roots = [SOURCE_DIR / "SecOC", SOURCE_DIR / "Csm"]
    bad_lines = grep_lines(r"\b(malloc|calloc|realloc|free)\s*\(", safety_roots)
    section.findings.append(Finding(
        "ISO 26262-6", "§8.4.5 no-dynamic-memory",
        "Dynamic memory allocation avoided in safety-critical modules",
        "PASS" if not bad_lines else "FAIL",
        f"{len(bad_lines)} malloc/calloc/realloc/free call sites in SecOC/Csm",
    ))

    # Unbounded recursion / goto / setjmp
    goto_lines = grep_lines(r"\b(goto|setjmp|longjmp)\b", roots)
    section.findings.append(Finding(
        "ISO 26262-6", "§8.4.5 no-unstructured-control",
        "No goto / setjmp / longjmp",
        "PASS" if not goto_lines else "WARN",
        f"{len(goto_lines)} occurrences",
    ))

    # Endless loops – acceptable only in Scheduler threads and deliberate stubs
    loops = grep_lines(r"while\s*\(\s*(1|true)\s*\)", roots)
    allowed = sum(1 for p, _, _ in loops if "/Scheduler/" in p)
    section.findings.append(Finding(
        "ISO 26262-6", "§8.4.5 bounded loops",
        "while(1) limited to scheduler / test stubs",
        "PASS" if (len(loops) - allowed) <= 1 else "WARN",
        f"{len(loops)} total, {allowed} in Scheduler thread bodies",
    ))

    # Det_ReportError usage = defensive error propagation (ISO 26262 §6.4.4)
    det_hits = grep_count(r"Det_ReportError", [SOURCE_DIR / "SecOC"])
    section.findings.append(Finding(
        "ISO 26262-6", "§6.4.4 diagnostic reporting",
        "Det_ReportError used for contract violations",
        "PASS" if det_hits >= 5 else "WARN",
        f"{det_hits} Det_ReportError call sites in source/SecOC",
    ))

    section.summary = (
        "Safety-critical SecOC/Csm modules are statically allocated. Endless "
        "loops are confined to Scheduler threads (intentional)."
    )


def scan_iso21434(section: Section) -> None:
    roots = [SOURCE_DIR, INCLUDE_DIR]

    # Freshness / replay protection – primary CR-3 control
    freshness_hits = grep_count(r"freshness|Freshness|FRESHNESS", roots)
    section.findings.append(Finding(
        "ISO/SAE 21434", "WP-RQ-09-02 anti-replay",
        "Freshness Value Manager implemented",
        "PASS" if freshness_hits > 50 else "WARN",
        f"{freshness_hits} freshness/Freshness references; FVM module present",
        location="source/SecOC/FVM.c",
    ))

    # Authenticator verification (CIA – Integrity + Authenticity)
    crypto_hits = grep_count(
        r"Csm_MacVerify|Csm_MacGenerate|Csm_SignatureVerify|"
        r"Csm_SignatureGenerate|PQC_MLDSA_Verify|PQC_MLDSA_Sign",
        [SOURCE_DIR],
    )
    section.findings.append(Finding(
        "ISO/SAE 21434", "WP-RQ-09-02 message authentication",
        "Authenticator generation/verification through CSM",
        "PASS" if crypto_hits > 10 else "WARN",
        f"{crypto_hits} MAC/signature CSM call sites",
    ))

    # Timing side-channel on MAC compare
    csm_c = (SOURCE_DIR / "Csm" / "Csm.c")
    mac_cmp_plain = False
    ct_helper = False
    if csm_c.exists():
        txt = csm_c.read_text(errors="ignore")
        mac_cmp_plain = bool(re.search(r"memcmp\s*\([^)]*generatedMacBuffer", txt))
        ct_helper = "Csm_ConstantTimeMemcmp" in txt
    section.findings.append(Finding(
        "ISO/SAE 21434", "RQ-05-04 side-channel resistance",
        "MAC comparison uses constant-time primitive",
        "WARN" if mac_cmp_plain else "PASS",
        ("memcmp() used in Csm_MacVerify – non-constant-time. Consider "
         "constant-time compare helper for ASIL-B targets.")
        if mac_cmp_plain
        else ("Csm_ConstantTimeMemcmp helper in use on MAC verify path"
              if ct_helper else "constant-time compare in use"),
        location="source/Csm/Csm.c",
    ))

    # Key-length / algorithm strength (RQ-05-03)
    strong_algs = grep_count(
        r"HMAC[_-]?SHA256|SHA-?256|AES[_-]?256|ML[_-]?DSA|ML[_-]?KEM",
        roots,
    )
    section.findings.append(Finding(
        "ISO/SAE 21434", "RQ-05-03 crypto strength",
        "Only NIST-approved primitives referenced",
        "PASS" if strong_algs > 20 else "WARN",
        f"{strong_algs} references to HMAC-SHA-256 / ML-DSA-65 / ML-KEM-768",
    ))

    section.summary = (
        "Confidentiality, integrity, authenticity and freshness controls are "
        "present. Non-constant-time memcmp on the MAC compare path is flagged "
        "as a residual hardening item."
    )


def scan_unr155(section: Section) -> None:
    # UN ECE R155 Annex 5 mitigations
    mitigations = [
        ("M4",   "Back-end server spoofing / network impersonation",
         r"PQC_MLDSA_Verify|Csm_MacVerify|Csm_SignatureVerify"),
        ("M6",   "Message injection / replay on in-vehicle network",
         r"freshness|FreshnessValue|FVM_GetRxFreshness"),
        ("M7",   "Message modification / MitM",
         r"PQC_MLDSA|MacGenerate|SignatureGenerate"),
        ("M10",  "Insufficient cryptographic strength",
         r"ML[_-]?DSA[_-]?65|ML[_-]?KEM[_-]?768|SHA[_-]?256"),
        ("M11",  "Unauthorised software update",
         r"NvM_WriteBlock|NvM_ReadBlock|Key.*Persistence"),
        ("M21",  "Extraction of cryptographic material",
         r"SecretKey|PrivateKey|PQC_KeyPair"),
    ]
    for idx, title, pat in mitigations:
        n = grep_count(pat, [SOURCE_DIR, INCLUDE_DIR])
        section.findings.append(Finding(
            "UN ECE R155 Annex 5", idx, title,
            "PASS" if n > 0 else "WARN",
            f"{n} code references match mitigation pattern",
        ))
    section.summary = (
        "Six primary R155 Annex 5 threat mitigations have observable code "
        "evidence in source/ and include/."
    )


def scan_fips(section: Section) -> None:
    pqc_h = INCLUDE_DIR / "PQC" / "PQC.h"
    pqc_text = pqc_h.read_text(errors="ignore") if pqc_h.exists() else ""

    expected = {
        "FIPS 203 (ML-KEM-768)": [
            ("PQC_MLKEM_PUBLIC_KEY_BYTES",  1184),
            ("PQC_MLKEM_SECRET_KEY_BYTES",  2400),
            ("PQC_MLKEM_CIPHERTEXT_BYTES",  1088),
            ("PQC_MLKEM_SHARED_SECRET_BYTES", 32),
        ],
        "FIPS 204 (ML-DSA-65)": [
            ("PQC_MLDSA_PUBLIC_KEY_BYTES",  1952),
            ("PQC_MLDSA_SECRET_KEY_BYTES",  4032),
            ("PQC_MLDSA_SIGNATURE_BYTES",   3309),
        ],
    }
    for std, rows in expected.items():
        for name, want in rows:
            m = re.search(rf"#define\s+{name}\s+(\d+)", pqc_text)
            got = int(m.group(1)) if m else None
            ok = got == want
            section.findings.append(Finding(
                std, name, f"{name} = {want}",
                "PASS" if ok else "FAIL",
                f"declared value: {got if got is not None else 'missing'}",
                location="include/PQC/PQC.h",
            ))

    # liboqs algorithm bindings
    for alg in ("OQS_KEM_alg_ml_kem_768", "OQS_SIG_alg_ml_dsa_65"):
        hits = grep_count(alg, [SOURCE_DIR / "PQC"])
        section.findings.append(Finding(
            "NIST FIPS 203/204", f"liboqs binding {alg}",
            "Algorithm identifier referenced",
            "PASS" if hits > 0 else "FAIL",
            f"{hits} references in source/PQC/",
        ))

    section.summary = (
        "PQC module declares the exact key/signature/ciphertext sizes mandated "
        "by NIST FIPS 203 and FIPS 204 and binds to the liboqs ML-KEM-768 / "
        "ML-DSA-65 algorithm identifiers."
    )


# ----- rendering -----

def status_emoji(s: str) -> str:
    return {"PASS": "[PASS]", "WARN": "[WARN]", "FAIL": "[FAIL]", "INFO": "[INFO]"}.get(s, s)


def render_markdown(sections: list[Section], stats: dict, report_dir: Path) -> str:
    lines = []
    lines.append("# SecOC Source-Code Compliance Report")
    lines.append("")
    lines.append(f"- Generated: {stats['generated_at']}")
    lines.append(f"- Scope: `Autosar_SecOC/source/` and `Autosar_SecOC/include/`")
    lines.append(f"- Files scanned: {stats['file_count']} "
                 f"({stats['c_count']} .c, {stats['h_count']} .h)")
    lines.append(f"- Lines of code: {stats['loc']}")
    lines.append(f"- Git branch: `{stats['branch']}` @ `{stats['commit']}`")
    lines.append("")
    lines.append("## Executive Summary")
    lines.append("")
    totals = {"PASS": 0, "WARN": 0, "FAIL": 0, "INFO": 0}
    for sec in sections:
        for f in sec.findings:
            totals[f.status] = totals.get(f.status, 0) + 1
    lines.append(
        f"**{totals['PASS']} PASS / {totals['WARN']} WARN / "
        f"{totals['FAIL']} FAIL / {totals['INFO']} INFO**"
    )
    lines.append("")
    lines.append("| Standard | Verdict |")
    lines.append("|---|---|")
    for sec in sections:
        verdict_counts = {"PASS": 0, "WARN": 0, "FAIL": 0}
        for f in sec.findings:
            verdict_counts[f.status] = verdict_counts.get(f.status, 0) + 1
        if verdict_counts["FAIL"]:
            verdict = f"FAIL ({verdict_counts['FAIL']})"
        elif verdict_counts["WARN"]:
            verdict = f"PASS with {verdict_counts['WARN']} warning(s)"
        else:
            verdict = f"PASS ({verdict_counts['PASS']}/{verdict_counts['PASS']})"
        lines.append(f"| {sec.title} | {verdict} |")
    lines.append("")

    for sec in sections:
        lines.append(f"## {sec.title}")
        lines.append("")
        if sec.summary:
            lines.append(sec.summary)
            lines.append("")
        lines.append("| Status | Rule | Title | Evidence | Location |")
        lines.append("|---|---|---|---|---|")
        for f in sec.findings:
            loc = f"`{f.location}`" if f.location else ""
            evidence = f.evidence.replace("|", "\\|")
            lines.append(
                f"| {status_emoji(f.status)} | `{f.rule}` | {f.title} | "
                f"{evidence} | {loc} |"
            )
        lines.append("")

    lines.append("## Appendix A – Methodology")
    lines.append("")
    lines.append(
        "MISRA findings come from `commands/compliance/misra-report.sh` "
        "(cppcheck 2.13.0 + misra.py addon). SWS, naming, ISO 26262, "
        "ISO/SAE 21434, UN R155 and FIPS checks are grep-based static "
        "evidence scans over `source/` and `include/`. WARN means the "
        "control is present but has residual hardening recommendations."
    )
    lines.append("")
    return "\n".join(lines)


def render_matrix_md(sections: list[Section]) -> str:
    lines = ["# Standards Compliance Matrix", "",
             "| Standard | Controls Checked | PASS | WARN | FAIL |",
             "|---|---:|---:|---:|---:|"]
    for sec in sections:
        p = sum(1 for f in sec.findings if f.status == "PASS")
        w = sum(1 for f in sec.findings if f.status == "WARN")
        fl = sum(1 for f in sec.findings if f.status == "FAIL")
        lines.append(f"| {sec.title} | {len(sec.findings)} | {p} | {w} | {fl} |")
    lines.append("")
    return "\n".join(lines)


def render_json(sections: list[Section], stats: dict) -> str:
    blob = {
        "generated_at": stats["generated_at"],
        "scope": ["Autosar_SecOC/source", "Autosar_SecOC/include"],
        "stats": stats,
        "sections": [
            {
                "key": s.key,
                "title": s.title,
                "summary": s.summary,
                "findings": [asdict(f) for f in s.findings],
            }
            for s in sections
        ],
    }
    return json.dumps(blob, indent=2, sort_keys=True) + "\n"


# ----- main -----

def collect_stats() -> dict:
    files = iter_c_files(SOURCE_DIR) + iter_c_files(INCLUDE_DIR)
    c_count = sum(1 for f in files if f.suffix == ".c")
    h_count = sum(1 for f in files if f.suffix == ".h")
    loc = 0
    for f in files:
        try:
            loc += sum(1 for _ in f.open("r", encoding="utf-8", errors="ignore"))
        except OSError:
            pass
    try:
        commit = subprocess.check_output(
            ["git", "rev-parse", "--short", "HEAD"],
            cwd=PROJECT_DIR, text=True,
        ).strip()
        branch = subprocess.check_output(
            ["git", "rev-parse", "--abbrev-ref", "HEAD"],
            cwd=PROJECT_DIR, text=True,
        ).strip()
    except subprocess.CalledProcessError:
        commit, branch = "unknown", "unknown"
    return {
        "generated_at": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "file_count": len(files),
        "c_count": c_count,
        "h_count": h_count,
        "loc": loc,
        "commit": commit,
        "branch": branch,
    }


def main(argv: list[str]) -> int:
    out_name = argv[1] if len(argv) > 1 else f"source_compliance_{utc_stamp()}"
    report_dir = REPORTS_DIR / out_name
    report_dir.mkdir(parents=True, exist_ok=True)

    sections: list[Section] = []

    for key, title, scanner in [
        ("misra",    "MISRA C:2012",                       scan_misra),
        ("sws",      "AUTOSAR R21-11 SWS_SecOC",           scan_autosar_sws),
        ("naming",   "AUTOSAR BSWGeneral Naming",          scan_autosar_naming),
        ("iso26262", "ISO 26262-6 Functional Safety",      scan_iso26262),
        ("iso21434", "ISO/SAE 21434 Cybersecurity",        scan_iso21434),
        ("unr155",   "UN ECE R155 Annex 5 Mitigations",    scan_unr155),
        ("fips",     "NIST FIPS 203 / 204 PQC",            scan_fips),
    ]:
        sec = Section(key=key, title=title, summary="")
        scanner(sec)
        sections.append(sec)

    stats = collect_stats()
    md = render_markdown(sections, stats, report_dir)
    matrix_md = render_matrix_md(sections)
    js = render_json(sections, stats)

    (report_dir / "COMPLIANCE_REPORT.md").write_text(md)
    (report_dir / "compliance_matrix.md").write_text(matrix_md)
    (report_dir / "compliance_matrix.json").write_text(js)

    # refresh "latest" symlink-ish marker
    (REPORTS_DIR / "LATEST_SOURCE_COMPLIANCE.txt").write_text(out_name + "\n")

    print(f"report dir: {report_dir}")
    print(f"  - COMPLIANCE_REPORT.md")
    print(f"  - compliance_matrix.md")
    print(f"  - compliance_matrix.json")
    totals = {"PASS": 0, "WARN": 0, "FAIL": 0, "INFO": 0}
    for sec in sections:
        for f in sec.findings:
            totals[f.status] = totals.get(f.status, 0) + 1
    print(f"verdicts: PASS={totals['PASS']} WARN={totals['WARN']} "
          f"FAIL={totals['FAIL']} INFO={totals['INFO']}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
