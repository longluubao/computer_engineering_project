# Cross-Environment PQC Comparison

The numbers below come from three distinct test campaigns. The ISE column links the **real** PQC modules from `Autosar_SecOC/source/PQC/*.c` + liboqs and adds simulated SecOC framing, freshness handling and bus transit; the other two columns measure raw PQC primitives only. Numbers are *not* directly comparable (different payloads, different cache states, different host CPUs) — they are kept here as upper and lower bounds.

AUTOSAR upper-stack (Com / PduR / SecOC.c / Csm / CryIf / CanTp / SoAd / NvM …) conformance is asserted by the 41-executable / 678-case gtest suite under `Autosar_SecOC/test/`, not by the ISE.

| environment | ML-DSA Sign [us] | ML-DSA Verify [us] | ML-KEM KeyGen [us] | end-to-end (ISE only) [us] |
| --- | --- | --- | --- | --- |
| ISE (PQC + SecOC-protocol harness) | 111.9 | 39.0 | 10.3 | 1090.9 |
| Pi 4 unit-only | 10581.3 | 517.9 | n/a | — |
| x86_64 unit-only | 2047.6 | 129.1 | n/a | — |
