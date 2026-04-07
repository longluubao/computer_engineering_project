# Defense MISRA Mitigation Table

## Important Tooling Note

The command `/automotive/compliance-misra-report` currently runs a template script that emits fixed summary values.  
Use `cppcheck` with MISRA addon for real per-file diagnostics when preparing final evidence.

## Green/Yellow/Red Status for Defense

| Area | Status | What to claim | Evidence to show |
|---|---|---|---|
| Build and integration baseline | Green | Build passes after ETH gateway hardening and SoAd/PQC refactor | `build/` output and generated test executables |
| AUTOSAR boundary cleanup (SoAd vs SecOC internals) | Green | SoAd route decisions are now driven by SoAd config structures, not SecOC internal tables | `include/SoAd/SoAd.h`, `source/SoAd/SoAd.c` |
| ETH direct bypass deactivation | Green | Legacy direct RX path is disabled and retained only as shim compatibility layer | `source/Ethernet/ethernet.c`, `source/Ethernet/ethernet_windows.c` |
| PQC control-plane flow completeness | Yellow | Control messages are now explicit and asynchronous; production trust anchoring remains future work | `include/SoAd/SoAd_PQC.h`, `source/SoAd/SoAd_PQC.c`, `source/Csm/Csm.c` |
| MISRA audit evidence quality | Yellow | High-level MISRA summary exists; detailed line-by-line rule closure still needed for audit-grade package | `misra-report.json`, `misra-cppcheck.txt`, `SAST_MISRA_DEVIATION_LOG.md` |
| Functional safety/certification package completeness | Red | Not claiming ISO 26262 certification; this is a thesis prototype with documented deviations | `COMPLIANCE_TRACEABILITY_MATRIX.md`, `SAST_MISRA_DEVIATION_LOG.md` |

## MISRA-Focused Mitigation Table (for Top Findings)

| MISRA Rule | Risk Pattern | Typical Code Pattern | Mitigation | Defense Wording |
|---|---|---|---|---|
| 10.4 | Mixed essential types in expressions | arithmetic/bitwise with mixed signedness or implicit promotions | Add explicit casts to the final target type and use typed constants (`0U`, `1U`, `((uint16)...)`) | "Type-conversion risks are controlled through explicit-width casting and unsigned literal discipline." |
| 11.3 | Cast between pointer types | `const uint8*` to `uint8*` or unrelated pointer casts | Remove unsafe casts, preserve `const`, and isolate unavoidable casts behind documented wrappers | "Pointer-cast exceptions are minimized and tracked in the deviation log with rationale and containment." |
| 14.4 | Non-boolean controlling expressions | `if (value)` where value is not explicitly boolean | Use explicit comparisons (`!= 0U`, `== TRUE`, `== E_OK`) for all control flow decisions | "Control-flow conditions were normalized to explicit boolean-style expressions." |

## Slide Checklist (What to show)

1. A single summary slide with the Green/Yellow/Red table above.
2. One evidence slide for each Green claim, with file paths and short code snippets.
3. One "Open Risks and Deviation Plan" slide covering Yellow/Red items and next actions.
4. One appendix slide mapping requirement -> code -> test from `COMPLIANCE_TRACEABILITY_MATRIX.md`.

## Next Actions to reach audit-strong package

1. Run `cppcheck` MISRA with project compile configuration and collect only project-owned actionable findings.
2. Triage each finding into: fix now, justified deviation, false positive.
3. Update `SAST_MISRA_DEVIATION_LOG.md` with owner/date/status for every non-fixed item.
4. Freeze a reproducible compliance bundle (command lines, reports, commit hash, test evidence).

