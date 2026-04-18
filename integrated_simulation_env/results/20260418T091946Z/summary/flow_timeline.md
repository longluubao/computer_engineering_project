# AUTOSAR Signal Flow — Per-Layer Latency Timeline

Shows one representative PDU traversing the AUTOSAR BSW stack in the ISE. Numbers are per-layer time in microseconds (µs).

**Source file:** `raw/baseline_pqc_r1_frames.csv`  
**Signal:** id=0x01, ASIL=4, deadline-class D1, PDU=3327 B, auth=3309 B, fragments=52  

```
TX side                                      RX side
┌──────────────────────┐                  ┌──────────────────────┐
│ Com / PduR           │  ← residual      │ Com / PduR           │
│   (app → SecOC)      │    0.41 µs     │   (SecOC → app)      │
├──────────────────────┤                  ├──────────────────────┤
│ SecOC build header   │                  │ SecOC parse header   │
│ + freshness counter  │                  │ + FVM check          │
├──────────────────────┤                  ├──────────────────────┤
│ Csm / PQC sign       │  261.61 µs     │ Csm / PQC verify     │    0.00 µs
│ (ML-DSA-65 / HMAC)   │                  │ (ML-DSA-65 / HMAC)   │
├──────────────────────┤                  ├──────────────────────┤
│ CanTP / SoAd frag.   │    2.42 µs     │ CanTP / SoAd reasm.  │
├──────────────────────┤                  ├──────────────────────┤
│ CanIf / EthIf TX     │  133.26 µs     │ CanIf / EthIf RX     │
└──────────┬───────────┘                  └──────────▲───────────┘
           │     ╔════════════════════════════════╗  │
           └────►║  Virtual bus (CAN-FD / Eth)   ║──┘
                 ╚════════════════════════════════╝

End-to-end (application → application): 397.69 µs
```

## Layer budget verification

| Layer | Measured (µs) | AUTOSAR-typical budget | Pass |
|-------|---------------|------------------------|------|
| Com / PduR + SecOC header | 0.41 | ≤ 200 | PASS |
| Csm sign (ML-DSA / HMAC) | 261.61 | ≤ 500 | PASS |
| CanTP / SoAd fragmentation | 2.42 | ≤ 300 | PASS |
| Physical bus transit | 133.26 | ≤ 2000 | PASS |
| Csm verify (ML-DSA / HMAC) | 0.00 | ≤ 500 | PASS |
| End-to-end | 397.69 | ≤ 5000 | PASS |

Budgets are representative of production AUTOSAR deployments (Com 100–200 µs, Csm PQC sign ≤ 500 µs on Cortex-A72, CAN-TP ≤ 300 µs per 8-byte fragment, physical bus ≤ 2 ms). Tight real-time targets (ASIL-D brake-by-wire) may require a lower end-to-end budget — see `summary/compliance_constraints.md`.
