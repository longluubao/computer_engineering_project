# SAST and MISRA Deviation Log (Thesis Baseline)

## Scope

- Project: `Autosar_SecOC`
- Baseline intent: AUTOSAR Classic-aligned prototype with PQC data path
- Tooling target: cppcheck MISRA profile + compiler warnings

## Static Analysis Checklist

- [ ] Run cppcheck MISRA profile on `source/` and `include/`
- [ ] Capture warning list and classify (must-fix / accepted deviation)
- [ ] Confirm no new high-severity warnings in:
  - `source/SecOC/FVM.c`
  - `source/SecOC/SecOC.c`
  - `source/SoAd/SoAd.c`
  - `source/SoAd/SoAd_PQC.c`
  - `source/Csm/Csm.c`

## Deviation Entries

| ID | File | Rule/Category | Rationale | Mitigation | Status |
|---|---|---|---|---|---|
| DEV-001 | `source/SoAd/SoAd.c` | AUTOSAR/MISRA complexity | TP RX state machine is intentionally explicit for robustness | Unit/integration tests + bounded queues | Open |
| DEV-002 | `source/SoAd/SoAd_PQC.c` | Prototype control-plane | Asynchronous key exchange framing added; full secure channel hardening is future work | Documented thesis scope limitation | Open |
| DEV-003 | `source/Csm/Csm.c` | Key provisioning model | Strict file bootstrap used as baseline; HSM callback path supported but not default | Manufacturing/HSM provisioning planned | Open |

## Workflow Evidence Package

- Build log (successful): `Autosar_SecOC/build`
- Traceability matrix: `COMPLIANCE_TRACEABILITY_MATRIX.md`
- Test references:
  - `test/AuthenticationTests.cpp`
  - `test/VerificationTests.cpp`
  - `test/FreshnessTests.cpp`
  - `test/DirectTxTests.cpp`
  - `test/DirectRxTests.cpp`

