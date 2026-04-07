# AUTOSAR Classic Traceability Matrix

This matrix links key AUTOSAR-oriented requirements to implementation and verification artifacts for thesis defense.

| Requirement ID | Requirement Summary | Implementation | Verification | Evidence |
|---|---|---|---|---|
| SEC-001 | Secured ETH PDU shall use TP path for large PQC payloads | `source/SecOC/SecOC_Lcfg.c` (`SECOC_SECURED_PDU_SOADTP`) | Build + TP path tests | `build/`, `test/DirectTxTests.cpp` |
| SEC-002 | Rx freshness shall reject stale/replay values | `source/SecOC/FVM.c` | Freshness unit tests | `test/FreshnessTests.cpp` |
| SEC-003 | Secured PDU verification shall use ML-DSA in PQC mode | `source/SecOC/SecOC.c` (`verify_PQC`) | Verification tests | `test/VerificationTests.cpp` |
| ETH-001 | Legacy direct Ethernet RX bypass shall be disabled | `source/Ethernet/ethernet.c`, `source/Ethernet/ethernet_windows.c` | Code inspection + build | `include/Ethernet/*.h` |
| SOAD-001 | SoAd shall use explicit route config and SoCon config | `include/SoAd/SoAd.h`, `source/SoAd/SoAd.c` | Build + runtime checks | `build/`, SoAd logs |
| SOAD-002 | SoAd routing shall respect routing groups per PDU | `source/SoAd/SoAd.c` | Integration test flows | `source/BswM/BswM.c` + test logs |
| PQC-001 | Key-exchange control messages shall be parsed and handled explicitly | `source/SoAd/SoAd_PQC.c` | Integration testing | `test_phase3_complete_ethernet_gateway.c` |
| CSM-001 | Default key bootstrap shall be strict (no auto demo generation) | `source/Csm/Csm.c` | Startup/build verification | Runtime logs |

## Notes

- This matrix is thesis-level engineering traceability, not a formal certification package.
- For stronger audit posture, export this table into your final report and include concrete test run IDs.
