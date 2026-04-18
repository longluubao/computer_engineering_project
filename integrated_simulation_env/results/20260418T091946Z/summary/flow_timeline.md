# AUTOSAR Signal Flow — Per-Layer Latency Timeline

Traces one representative PDU through the full AUTOSAR BSW call chain implemented under `Autosar_SecOC/source/`. Each box is a real module in the repository; the API function shown is the one actually invoked by the caller on the left.

**Source file:** `raw/baseline_pqc_r1_frames.csv`  
**Signal:** id=0x01, ASIL=4, deadline-class D1, PDU=3327 B, auth=3309 B, fragments=52  
**Protection mode:** PQC (ML-DSA-65)

## TX path — application → bus

```
┌───────────────────────────────────────────────────────────────┐
│ [Application / SW-C]  runnable fires on period / event        │
│     Com_SendSignal(signal_id, data_ptr)                       │
└───────────────────────────────┬───────────────────────────────┘
                                ▼                                ─┐
┌───────────────────────────────────────────────────────────────┐ │
│ [Com]  source/Com/Com.c                                       │ │
│     Com_SendSignal   ───▶   PduR_ComTransmit(PduId, Info)     │ │
└───────────────────────────────┬───────────────────────────────┘ │
                                ▼                                 │ residual
┌───────────────────────────────────────────────────────────────┐ │ (Com + PduR
│ [PduR] source/PduR/PduR.c  —  central router (TX dispatch)    │ │  + SecOC hdr
│     PduR_ComTransmit ──▶ SecOC_IfTransmit (short PDUs) or     │ │  + FVM)
│                          SecOC_TpTransmit (PQC / large PDUs)  │ │
└───────────────────────────────┬───────────────────────────────┘ │
                                ▼                                 │
┌───────────────────────────────────────────────────────────────┐ │
│ [SecOC] source/SecOC/SecOC.c                                  │ │
│   1. FVM: next freshness = last_tx + 1  (8-byte counter)      │ │
│   2. Build Secured PDU = [hdr 2B | freshness 8B | payload]    │ │
│   3. Csm_SignatureGenerate() (PQC) / Csm_MacGenerate() (HMAC) │ │
└───────────────────────────────┬───────────────────────────────┘ │    0.41 µs
                                ▼                                ─┘
┌───────────────────────────────────────────────────────────────┐ ─┐
│ [Csm] source/Csm/Csm.c  —  Crypto Service Manager             │  │
│     Csm_SignatureGenerate  ───▶  CryIf_SignatureGenerate      │  │
└───────────────────────────────┬───────────────────────────────┘  │
                                ▼                                  │
┌───────────────────────────────────────────────────────────────┐  │
│ [CryIf] source/CryIf/CryIf.c  —  Crypto I/F driver selector   │  │ csm_sign
│     CryIf_SignatureGenerate  ───▶  PQC_MLDSA_Sign()           │  │ (Csm +
│                              ───▶  startEncryption() (HMAC)   │  │  CryIf +
└───────────────────────────────┬───────────────────────────────┘  │  PQC +
                                ▼                                  │  liboqs)
┌───────────────────────────────────────────────────────────────┐  │
│ [PQC] source/PQC/PQC.c  —  ML-DSA-65 wrapper (FIPS 204)       │  │
│     PQC_MLDSA_Sign  ───▶  OQS_SIG_sign(alg_ml_dsa_65)         │  │
└───────────────────────────────┬───────────────────────────────┘  │
                                ▼                                  │
┌───────────────────────────────────────────────────────────────┐  │
│ [liboqs] external/liboqs  —  lattice signer                   │  │
│     OQS_SIG_sign → 3309-byte signature                        │  │  261.61 µs
└───────────────────────────────┬───────────────────────────────┘ ─┘
                                ▼
         (back to SecOC, authenticator appended to PDU)
                                ▼                                ─┐
┌───────────────────────────────────────────────────────────────┐ │
│ [SecOC] → PduR_SecOCTransmit()                                │ │
│ [PduR] → CanTp_Transmit() (CAN/CAN-FD) or                     │ │ cantp_frag
│         SoAd_IfTransmit()/SoAd_TpTransmit() (Ethernet)        │ │
│ [CanTp] source/Can/CanTp.c  —  ISO 15765-2 FF/CF fragments    │ │
│         → CanIf_Transmit(fragment[i])                         │ │    2.42 µs
└───────────────────────────────┬───────────────────────────────┘ ─┘
                                ▼                                ─┐
┌───────────────────────────────────────────────────────────────┐ │
│ [CanIf]  source/Can/CanIf.c    → Can_Write()                  │ │
│ [Can]    source/Can/Can.c      → hardware frame TX            │ │
│   OR                                                          │ │ bus_transit
│ [EthIf]  source/EthIf/EthIf.c  → EthDrv_Send()                │ │
│ [EthDrv] source/Ethernet/ethernet*.c → send()/sendto()        │ │
└───────────────────────────────┬───────────────────────────────┘ │  133.26 µs
                                ▼                                ─┘
                ╔═══════════════════════════════╗
                ║  Virtual bus (CAN-FD / Eth)   ║
                ╚═══════════════╦═══════════════╝
```

## RX path — bus → application

```
                ╔═══════════════════════════════╗
                ║  Virtual bus (CAN-FD / Eth)   ║
                ╚═══════════════╦═══════════════╝
                                ▼
┌───────────────────────────────────────────────────────────────┐
│ [Can driver] Can_RxIndicationCallback() (ISR)                 │
│   OR [EthIf]  SoAd_RxIndication() (socket rx)                 │
└───────────────────────────────┬───────────────────────────────┘
                                ▼
┌───────────────────────────────────────────────────────────────┐
│ [CanIf] CanIf_RxIndication → CanTp_RxIndication               │
│ [CanTp] reassemble FF/CF → PduR_CanTpRxIndication             │
│ [SoAd]  SoAd_RxIndication → PduR_SoAdRxIndication             │
└───────────────────────────────┬───────────────────────────────┘
                                ▼
┌───────────────────────────────────────────────────────────────┐
│ [PduR] RX dispatch  →  SecOC_RxIndication(PduId, Info)        │
└───────────────────────────────┬───────────────────────────────┘
                                ▼                                ─┐
┌───────────────────────────────────────────────────────────────┐ │
│ [SecOC] source/SecOC/SecOC.c                                  │ │
│   1. Parse [hdr | freshness | payload | authenticator]        │ │
│   2. FVM check: freshness > last_rx ?  (replay reject)        │ │ csm_verify
│   3. Csm_SignatureVerify() (PQC) / Csm_MacVerify() (HMAC)     │ │
└───────────────────────────────┬───────────────────────────────┘ │
                                ▼                                 │
┌───────────────────────────────────────────────────────────────┐ │
│ [Csm] Csm_SignatureVerify   ───▶   CryIf_SignatureVerify      │ │
│ [CryIf] CryIf_SignatureVerify ───▶ PQC_MLDSA_Verify()         │ │
│ [PQC]   PQC_MLDSA_Verify    ───▶   OQS_SIG_verify             │ │
│ [liboqs] OQS_SIG_verify (constant-time)                       │ │    0.00 µs
└───────────────────────────────┬───────────────────────────────┘ ─┘
                                ▼
   If verify FAIL  →  SecOC drops PDU, DET_ReportError          
   If verify PASS  →  update last_rx = freshness, continue      
                                ▼
┌───────────────────────────────────────────────────────────────┐
│ [SecOC] → PduR_SecOCIfRxIndication()                          │
│ [PduR]  → Com_RxIndication()                                  │
│ [Com]   → deliver signal to SW-C (application)                │
└───────────────────────────────────────────────────────────────┘

End-to-end (app Tx → app Rx): 397.69 µs
```

## Layer budget verification

| AUTOSAR layer(s)                             | Measured (µs) | Budget (µs) | Pass |
|----------------------------------------------|---------------|-------------|------|
| Com + PduR + SecOC header + FVM (residual)    |          0.41 | ≤       200 | PASS |
| Csm + CryIf + PQC + liboqs sign               |        261.61 | ≤       500 | PASS |
| CanTp / SoAd fragmentation                    |          2.42 | ≤       300 | PASS |
| CanIf / EthIf / driver + physical bus transit |        133.26 | ≤      2000 | PASS |
| Csm + CryIf + PQC + liboqs verify             |          0.00 | ≤       500 | PASS |
| End-to-end (application → application)        |        397.69 | ≤      5000 | PASS |

Budgets reflect production AUTOSAR targets on Cortex-A72 (Com/PduR/SecOC book-keeping 100–200 µs, Csm PQC sign/verify ≤ 500 µs, CanTP fragmentation ≤ 300 µs, physical bus transit ≤ 2 ms). Tighter real-time loops (ASIL-D brake-by-wire, D1 class ≤ 1 ms end-to-end) require the ASIL-specific deadline row in `summary/compliance_constraints.md`.

## Module call chain reference

Each module in the diagram above is a real directory under `Autosar_SecOC/source/`:

| Layer    | TX function                        | Calls                                  |
|----------|------------------------------------|----------------------------------------|
| Com      | `Com_SendSignal`                   | `PduR_ComTransmit`                     |
| PduR     | `PduR_ComTransmit`                 | `SecOC_IfTransmit` / `SecOC_TpTransmit`|
| SecOC    | `SecOC_IfTransmit`                 | `Csm_MacGenerate` / `Csm_SignatureGenerate` |
| Csm      | `Csm_SignatureGenerate`            | `CryIf_SignatureGenerate`              |
| CryIf    | `CryIf_SignatureGenerate`          | `PQC_MLDSA_Sign` (PQC) / `startEncryption` (HMAC) |
| PQC      | `PQC_MLDSA_Sign`                   | `OQS_SIG_sign` (liboqs ML-DSA-65)      |
| SecOC    | `PduR_SecOCTransmit`               | `CanTp_Transmit` / `SoAd_TpTransmit`   |
| CanTp    | `CanTp_Transmit`                   | `CanIf_Transmit` (per FF/CF fragment)  |
| CanIf    | `CanIf_Transmit`                   | `Can_Write`                            |
| SoAd     | `SoAd_TpTransmit` / `SoAd_IfTransmit` | `EthIf_Transmit`                    |
| EthIf    | `EthIf_Transmit`                   | `EthDrv_Send`                          |

RX path is the exact mirror: `Can_RxIndicationCallback` / `SoAd_RxIndication` → `CanIf_RxIndication` → `CanTp_RxIndication` → `PduR_CanTpRxIndication` → `SecOC_RxIndication` → `SecOC_MainFunctionRx` → FVM check → `Csm_SignatureVerify` → `CryIf_SignatureVerify` → `PQC_MLDSA_Verify` → `OQS_SIG_verify` → `PduR_SecOCIfRxIndication` → `Com_RxIndication`.
