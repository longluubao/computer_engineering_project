# Cross-Environment PQC Comparison

The numbers below come from three distinct test campaigns. The ISE column is the only one where the full AUTOSAR stack (Com → PduR → SecOC → Csm → PQC → SoAd → virtual bus) was active; the other two are unit-level measurements kept here as a reference baseline for the thesis.

| environment | ML-DSA Sign [us] | ML-DSA Verify [us] | ML-KEM KeyGen [us] | end-to-end (ISE only) [us] |
| --- | --- | --- | --- | --- |
| ISE (full stack) | 101.0 | 34.5 | 10.7 | 1015.9 |
| Pi 4 unit-only | 10581.3 | 517.9 | n/a | — |
| x86_64 unit-only | 2047.6 | 129.1 | n/a | — |
