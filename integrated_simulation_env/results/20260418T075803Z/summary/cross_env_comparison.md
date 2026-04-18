# Cross-Environment PQC Comparison

The numbers below come from three distinct test campaigns. The ISE column is the only one where the full AUTOSAR stack (Com → PduR → SecOC → Csm → PQC → SoAd → virtual bus) was active; the other two are unit-level measurements kept here as a reference baseline for the thesis.

| environment | ML-DSA Sign [us] | ML-DSA Verify [us] | ML-KEM KeyGen [us] | end-to-end (ISE only) [us] |
| --- | --- | --- | --- | --- |
| ISE (full stack) | 188.9 | 77.6 | 30.0 | 2114.0 |
| Pi 4 unit-only | 370.0 | 84.0 | 80.0 | — |
| x86_64 unit-only | 370.0 | 79.2 | 80.0 | — |
