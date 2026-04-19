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
└───────────────────────────────┬───────────────────────────────┘ │    0.76 µs
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
│     OQS_SIG_sign → 3309-byte signature                        │  │  218.36 µs
└───────────────────────────────┬───────────────────────────────┘ ─┘
                                ▼
         (back to SecOC, authenticator appended to PDU)
                                ▼                                ─┐
┌───────────────────────────────────────────────────────────────┐ │
│ [SecOC] → PduR_SecOCTransmit()                                │ │
│ [PduR] → CanTp_Transmit() (CAN/CAN-FD) or                     │ │ cantp_frag
│         SoAd_IfTransmit()/SoAd_TpTransmit() (Ethernet)        │ │
│ [CanTp] source/Can/CanTp.c  —  ISO 15765-2 FF/CF fragments    │ │
│         → CanIf_Transmit(fragment[i])                         │ │    2.00 µs
└───────────────────────────────┬───────────────────────────────┘ ─┘
                                ▼                                ─┐
┌───────────────────────────────────────────────────────────────┐ │
│ [CanIf]  source/Can/CanIf.c    → Can_Write()                  │ │
│ [Can]    source/Can/Can.c      → hardware frame TX            │ │
│   OR                                                          │ │ bus_transit
│ [EthIf]  source/EthIf/EthIf.c  → EthDrv_Send()                │ │
│ [EthDrv] source/Ethernet/ethernet*.c → send()/sendto()        │ │
└───────────────────────────────┬───────────────────────────────┘ │  106.17 µs
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

End-to-end (app Tx → app Rx): 327.28 µs
```

## Layer budget verification

| AUTOSAR layer(s)                             | Measured (µs) | Budget (µs) | Pass |
|----------------------------------------------|---------------|-------------|------|
| Com + PduR + SecOC header + FVM (residual)    |          0.76 | ≤       200 | PASS |
| Csm + CryIf + PQC + liboqs sign               |        218.36 | ≤       500 | PASS |
| CanTp / SoAd fragmentation                    |          2.00 | ≤       300 | PASS |
| CanIf / EthIf / driver + physical bus transit |        106.17 | ≤      2000 | PASS |
| Csm + CryIf + PQC + liboqs verify             |          0.00 | ≤       500 | PASS |
| End-to-end (application → application)        |        327.28 | ≤      5000 | PASS |

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

## ECU startup chain (Phase 1 — EcuM orchestration)

`main()` in `Autosar_SecOC/source/main.c` invokes `EcuM_Init`, which walks the BSW init chain in AUTOSAR-mandated order. None of the boxes below are exercised by the hot TX/RX measurements — they run once at power-on.

```
main() ──► EcuM_Init() ─┐
                        │  Phase 0 — MCAL / Microcontroller
                        ├──► Mcal_Init → Mcu_Init, Gpt_Init,
                        │               Wdg_Init, Dio_Init
                        │  Phase 1 — Diagnostics + Memory
                        ├──► Det_Init,  Det_Start   (error tracer)
                        ├──► MemIf_Init → Ea, Fee   (EEPROM/Flash
                        │                            abstraction)
                        ├──► NvM_Init     + NvM_ReadAll
                        │        └─ restores freshness counters,
                        │           ML-DSA private key slot
                        ├──► Dem_Init              (DTC storage)
                        ├──► Dcm_Init              (UDS / OBD-II)
                        ├──► Csm_Init              (crypto svc mgr)
                        │  Phase 2 — Bus drivers / SM layer
                        ├──► Can_Init              (CAN driver)
                        ├──► CanNm_Init, CanSM_Init, ComM_Init
                        ├──► EthSM_Init, UdpNm_Init (Eth side)
                        ├──► BswM_Init             (mode mgr)
                        │  Phase 2b — EcuM_StartupTwo
                        ├──► EthIf_Init, TcpIp_Init, SoAd_Init
                        ├──► CanIf_Init, CanTp_Init
                        ├──► SecOC_Init(&SecOC_Config)
                        ├──► Com_Init              (IPDU groups)
                        ├──► ApBridge_Init         (gateway hc)
                        └──► EcuM_GotoRun  → BswM enables COM
```

## Cross-cutting runtime modules (Phase 3 — always-on services)

These modules are not in the linear TX/RX chain but are consulted on every message (error paths), on periodic timers, or on state transitions. The ISE simulates their contractual behaviour rather than calling them through the real BSW stack.

```
              ┌──────────────────────────────────────┐
              │ [Det] Default Error Tracer           │
              │   Det_ReportError(moduleId,instId,   │
              │                   apiId, errorId)    │
              │ Called from EVERY module on bad args │
              │ / verify fail / freshness rollback.  │
              └──────────────────────────────────────┘
              ┌──────────────────────────────────────┐
              │ [Dem] Diagnostic Event Manager       │
              │   Dem_SetEventStatus(eventId, …)     │
              │ SecOC verify-fail, downgrade attack, │
              │ replay attempts → DTC + NvM block.   │
              └──────────────────────────────────────┘
              ┌──────────────────────────────────────┐
              │ [NvM] Non-Volatile Memory            │
              │   NvM_WriteBlock(FreshnessBlockId)   │
              │ Persist FVM counter across power     │
              │ cycles (SWS_SecOC_00194).            │
              │   NvM_ReadBlock(MLDSA_PrivKey)       │
              └──────────────────────────────────────┘
              ┌──────────────────────────────────────┐
              │ [Os] / [Scheduler]                   │
              │   SchM_Call_SecOC_MainFunctionTx()   │
              │   SchM_Call_SecOC_MainFunctionRx()   │
              │ Drives the 1 ms periodic MainFns.    │
              └──────────────────────────────────────┘
              ┌──────────────────────────────────────┐
              │ [BswM] + [ComM] + [CanSM] + [EthSM]  │
              │   BswM_RequestMode(…)                │
              │   ComM_RequestComMode(COMM_FULL)     │
              │   CanSM_RequestComMode(CANSM_ONLINE) │
              │   EthSM_RequestComMode(ETHSM_ONLINE) │
              │ Gates whether TX is even allowed.    │
              └──────────────────────────────────────┘
              ┌──────────────────────────────────────┐
              │ [CanNm] / [UdpNm]                    │
              │   NM vote frames keep the bus alive  │
              │   while application is idle.         │
              └──────────────────────────────────────┘
              ┌──────────────────────────────────────┐
              │ [Dcm] UDS diagnostics                │
              │   0x27 SecurityAccess, 0x2E WriteDID │
              │ Rekey / re-handshake request path    │
              │ (PQC_KeyExchange invoked via CryIf). │
              └──────────────────────────────────────┘
              ┌──────────────────────────────────────┐
              │ [ApBridge] Application bridge        │
              │   Monitors gateway health, toggles   │
              │   SoAd routing on fault.             │
              └──────────────────────────────────────┘
              ┌──────────────────────────────────────┐
              │ [GUIInterface] demo / debug hook     │
              │   Not exercised at runtime.          │
              └──────────────────────────────────────┘
```

## ECU shutdown chain (Phase 4)

On power-down request BswM calls `EcuM_GoDown`. The teardown walks the init chain in reverse so that in-flight PDUs are flushed and persistent state is saved before the MCU halts.

```
EcuM_GoDown  ──► ComM_CommunicationAllowed(FALSE)
             ──► Com_DeInit / Com_IpduGroupStop
             ──► SecOC_DeInit   (flush FVM counters)
             ──► SoAd_CloseSoCon / CanSM_RequestComMode(OFFLINE)
             ──► CanTp_Shutdown / CanIf_SetControllerMode(STOP)
             ──► NvM_WriteAll   (persist FVM + DTC blocks)
             ──► Dem_Shutdown / Dcm_DeInit / Csm_DeInit
             ──► Mcal_Wdg_Disable / Mcu_PerformReset
```

## Module inventory — all 31 BSW modules under `Autosar_SecOC/source`

Category legend: **D** = TX/RX data path, **S** = startup/mode, **X** = diagnostics, **M** = memory stack, **B** = bus-state manager, **H** = HAL, **U** = utility.

| Module | Cat. | Main API | Called by | Calls |
|--------|------|----------|-----------|-------|
| Mcal        | H | `Mcu_Init`, `Gpt_Init`, `Wdg_Init`, `Dio_Init`  | EcuM          | Hardware registers |
| EcuM        | S | `EcuM_Init`, `EcuM_GoDown`                     | `main()`      | All BSW init funcs |
| Det         | X | `Det_ReportError`                              | Every module  | Console / log |
| BswM        | S | `BswM_Init`, `BswM_RequestMode`                | EcuM, ComM    | ComM, SoAd |
| ComM        | S | `ComM_RequestComMode`                          | EcuM, BswM    | CanSM, EthSM |
| CanSM       | B | `CanSM_RequestComMode`                         | ComM          | CanNm, CanIf |
| EthSM       | B | `EthSM_RequestComMode`                         | ComM          | EthIf, TcpIp |
| CanNm       | B | `CanNm_PassiveStartUp`                         | CanSM         | CanIf |
| UdpNm       | B | `UdpNm_PassiveStartUp`                         | EthSM         | TcpIp |
| Os          | S | `SchM_Call_*MainFunction*`                     | main / tick   | Task handlers |
| Scheduler   | S | cooperative task loop                          | main          | MainFunctions |
| MemIf       | M | `MemIf_Read/Write`                             | NvM           | Fee, Ea |
| Fee         | M | `Fee_Read/Write`                               | MemIf         | Mcal Flash |
| Ea          | M | `Ea_Read/Write`                                | MemIf         | Mcal EEPROM |
| NvM         | M | `NvM_ReadBlock`, `NvM_WriteBlock`, `NvM_ReadAll` | EcuM, Dem, SecOC | MemIf |
| Dem         | X | `Dem_SetEventStatus`                           | SecOC, BswM   | NvM |
| Dcm         | X | `Dcm_MainFunction`                             | Os/Scheduler  | PduR, NvM, PQC |
| Csm         | D | `Csm_SignatureGenerate/Verify`, `Csm_MacGenerate/Verify` | SecOC | CryIf |
| CryIf       | D | `CryIf_SignatureGenerate/Verify`               | Csm           | PQC, Encrypt |
| Encrypt     | D | HMAC / AES primitives                          | CryIf         | Mcal crypto (opt) |
| PQC         | D | `PQC_MLDSA_Sign/Verify`, `PQC_MLKEM_Encapsulate/Decapsulate` | CryIf | liboqs |
| Com         | D | `Com_SendSignal`, `Com_RxIndication`           | SW-C, SecOC   | PduR |
| PduR        | D | `PduR_ComTransmit`, `PduR_SecOCTransmit`, `PduR_*RxIndication` | Com, SecOC, CanIf, CanTp, SoAd | SecOC, CanTp, SoAd, Com |
| SecOC       | D | `SecOC_IfTransmit`, `SecOC_TpTransmit`, `SecOC_RxIndication`, `SecOC_MainFunctionTx/Rx` | PduR | Csm, PduR |
| CanTp       | D | `CanTp_Transmit`, `CanTp_RxIndication`         | PduR          | CanIf |
| CanIf       | D | `CanIf_Transmit`, `CanIf_RxIndication`         | CanTp, PduR   | Can |
| Can         | H | `Can_Write`, `Can_RxIndicationCallback`        | CanIf         | Mcal_Can |
| SoAd        | D | `SoAd_IfTransmit`, `SoAd_TpTransmit`, `SoAd_RxIndication` | SecOC, PduR | TcpIp |
| TcpIp       | D | `TcpIp_TcpTransmit`, `TcpIp_UdpTransmit`       | SoAd          | EthIf |
| EthIf       | H | `EthIf_Transmit`                               | TcpIp         | Ethernet drv |
| Ethernet    | H | raw socket / NIC driver                        | EthIf         | OS sockets |
| ApBridge    | B | `ApBridge_Init`, `ApBridge_ReportHeartbeat`    | SoAd, EcuM    | SoAd, Det |
| GUIInterface| U | demo hook                                      | main          | Dcm, Det |


## ISE mapping — what the simulator actually exercises

The ISE keeps the full hot-path (`Com → PduR → SecOC → Csm → CryIf → PQC → liboqs → PduR → CanTp/SoAd → CanIf/EthIf → driver → bus`) in C and measures each bucket directly. Other modules are represented as follows:

| Module     | ISE representation                                       |
|------------|----------------------------------------------------------|
| EcuM       | implicit — `sim_ecu_create` + `sim_ecu_init_stack`       |
| BswM/ComM  | implicit — buses are opened before the iteration loop    |
| CanSM/EthSM/CanNm/UdpNm | N/A — bus is always up for the duration     |
| Os/Scheduler | the scenario's for-loop stands in for the 1 ms MainFn  |
| Mcal       | replaced by `sim_bus.c` (deterministic bit-rate model)   |
| NvM/MemIf/Fee/Ea | freshness counters kept in RAM (no crash-safety in ISE) |
| Dem/Dcm    | attack detections logged to `raw/*_attacks.csv`          |
| Det        | `sim_log(SIM_LOG_ERROR, …)` on any failure               |
| ApBridge / GUIInterface | not exercised (no external UI in ISE)       |

This keeps the thesis numbers honest: every µs reported in `layer_latency.csv` corresponds to real code that runs on the Pi 4 target; startup + cross-cutting services are documented here for completeness but do not inflate the hot-path figures.
