# Compliance Commands

Project-local compliance command set for automotive workflows.

## Available Scripts

- `run-all.sh` — runs all compliance checks
- `misra-report.sh` — runs cppcheck MISRA analysis, de-duplicates diagnostics, and writes top-violations summary
- `aspice-audit.sh` — runs ASPICE audit script
- `iso26262-checklist.sh` — runs ISO 26262 checklist script
- `unr155-assess.sh` — runs UNECE R155 assessment script
- `homologation-check.sh` — runs homologation check script

## Usage

From project root:

```bash
bash commands/compliance/run-all.sh
```

Allow missing external scripts (for local dry-runs only):

```bash
bash commands/compliance/run-all.sh --allow-stub
```

Single check:

```bash
bash commands/compliance/misra-report.sh
```

## Output

Reports are generated under:

- `compliance_reports/<timestamp>/`
- `compliance_reports/latest` (symbolic link if available)
- `compliance_reports/LATEST.txt` (always written fallback pointer)

Each run now includes:

- `run-all-status.txt`
- `checks.json`
- `manifest.json` (run metadata for traceability)
- `misra-cppcheck.txt` (raw cppcheck MISRA output)
- `misra-cppcheck-clean.txt` (de-duplicated diagnostics with `[xN]` count)
- `misra-main-violations.txt` (top MISRA rules/files, plus all-diagnostic context)
- `misra-metrics-integrity.txt` (count-consistency checks for audit traceability)
- `misra-summary.json` (raw/unique/duplicate totals, MISRA top fields, and all-diagnostic top fields)

## Environment Overrides

- `COMPLIANCE_EXT_ROOT` — location of external compliance scripts  
  (default: `/mnt/d/Long-Dev/automotive-claude-code-agents/commands/compliance`)
- `ALLOW_STUB` — `true|false` (default `false`)
- `MISRA_SUPPRESS_CONFIG_NOISE` — `true|false` (default `true`)
- `MISRA_TOP_N` — top entries to show in `misra-main-violations.txt` (default `20`)
- `MISRA_STRICT_METRICS` — fail run on metric inconsistency (`true|false`, default `true`)

