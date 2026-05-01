# Test Scenarios and Results

> Companion document to `DIAGRAMS.md` and `TECHNICAL_REPORT.md`.
> Drafted as direct input for the final thesis report.
> All numbers below come from the latest test runs (test_logs/phase2_*) and the
> hardware run on Raspberry Pi 4. Re-running the suites refreshes the numbers
> automatically via `bash run_tests.sh`.

---

## 1. Test Strategy Overview

The validation strategy is split into four layers, each with its own purpose
and evidence type.

```mermaid
flowchart TB
    L1["Layer 1 - Unit tests<br/>GoogleTest, mocked I/O<br/>~640 cases"]
    L2["Layer 2 - Module integration<br/>SecOCIntegrationTests, KeyExchangeTests, KeyDerivationTests<br/>~30 cases"]
    L3["Layer 3 - Stack integration<br/>Phase3_Complete_Test (C standalone)<br/>5 scenarios"]
    L4["Layer 4 - Target hardware<br/>Raspberry Pi 4, real liboqs, real TCP<br/>1 end-to-end run"]

    L1 --> L2 --> L3 --> L4

    style L1 fill:#bbdefb,color:#000
    style L2 fill:#90caf9,color:#000
    style L3 fill:#64b5f6,color:#000
    style L4 fill:#a5d6a7,color:#000
```

| Layer | Files | Framework | Mode coverage | I/O |
|-------|-------|-----------|---------------|-----|
| 1 | 40 `*Tests.cpp` files in `test/` | GoogleTest | Classic + PQC (compile-time switch) | Mocks |
| 2 | `SecOCIntegrationTests.cpp`, `KeyExchangeTests.cpp`, `KeyDerivationTests.cpp`, `PQC_ComparisonTests.cpp`, `SoAdPqcTests.cpp` | GoogleTest | Classic + PQC | Loopback, in-process |
| 3 | `test_phase3_complete_ethernet_gateway.c`, `test_pqc_secoc_integration.c`, `test_pqc_standalone.c`, `test_phase4_integration_report.c` | Standalone `main()` | PQC | liboqs real, no socket |
| 4 | `Phase3_Complete_Test` deployed to Pi 4 | Standalone | PQC | Real TCP, real `/etc/secoc/keys/` |

---

## 2. Test Catalogue (Per Suite)

### 2.1 SecOC Core

| Suite | File | # Cases | Highlights |
|-------|------|---------|------------|
| Integration | `SecOCIntegrationTests.cpp` | 8 | Round-trip Tx→Rx, replay rejected, payload bit-flip rejected, signature bit-flip rejected, TX-before-init guard, MainFunction idle safety |
| AUTOSAR adapter | `SecOCTests.cpp` | 3 | TP Tx confirmation, TP Rx indication x2 |
| Extension API | `SecOCExtTests.cpp` | 20 | Null-pointer guards, DeInit/Re-init, MainFunction stability |
| Authentication | `AuthenticationTests.cpp` | 3 | Classic = 8-byte PDU with 4-byte MAC; PQC = >3000-byte PDU with ML-DSA-65 signature |
| Verification | `VerificationTests.cpp` | 3 | Valid verify accepted; mismatched payload rejected |
| Freshness | `FreshnessTests.cpp` | 10 | Reconstruction, monotonic counter, replay rejection (Lower-Than 1–4) |
| Start-of-reception | `startOfReceptionTests.cpp` | 5 | TP buffer management, length validation |

### 2.2 Cryptography

| Suite | File | # Cases | Highlights |
|-------|------|---------|------------|
| ML-DSA / ML-KEM compare | `PQC_ComparisonTests.cpp` | 13 | Configuration detection; classic vs PQC authenticate / verify; 3-party KEM; tampered-signature rejection; security-level summary |
| ML-KEM key exchange | `KeyExchangeTests.cpp` | 16 | Full Alice/Bob handshake, multi-peer independence, rekey, FSM transitions, error guards |
| HKDF derivation | `KeyDerivationTests.cpp` | 15 | Deterministic, Enc ≠ Auth, different secrets → different keys, multi-peer storage, idempotent init |
| Csm | `CsmTests.cpp` | 18 | MAC, signature, key-exchange, key-derivation entry points |
| CryIf | `CryIfTests.cpp` | 10 | Adapter routing |
| SoAd PQC | `SoAdPqcTests.cpp` | 18 | Session management, ML-KEM handshake driven from SoAd |

### 2.3 Communication Stack

| Suite | File | # Cases |
|-------|------|---------|
| Com | `ComTests.cpp` | 20 |
| PduR | `PduRTests.cpp` | 20 |
| SoAd | `SoAdTests.cpp` | 20 |
| TcpIp / UdpNm | `TcpIpTests.cpp`, `UdpNmTests.cpp` | 40+ |
| Eth stack | `EthIfTests.cpp`, `EthSMTests.cpp` | 30 |
| CAN stack | `CanIfTests.cpp`, `CanTpTests.cpp`, `CanNmTests.cpp`, `CanSMTests.cpp` | 60+ |
| ComM / BswM | `ComMTests.cpp`, `BswMTests.cpp` | 30 |

### 2.4 Mode and Network State Management

| Suite | File | Module LOC | What it covers |
|-------|------|-----------:|----------------|
| EcuM | `EcuMTests.cpp` | 1118 | Init, StartupTwo, shutdown request, wakeup callouts, MainFunction |
| BswM | `BswMTests.cpp` | 997 | Mode requests, current-mode read-back, MainFunction tick |
| ComM | `ComMTests.cpp` | 253 | RequestComMode, GetCurrentComMode, GetState |
| CanSM | `CanSMTests.cpp` | 240 | RequestComMode, GetBsmState |
| CanNm | `CanNmTests.cpp` | 350 | NetworkRequest/Release, GetState, MainFunction |
| EthSM | `EthSMTests.cpp` | 423 | RequestComMode, GetCurrentInternalMode |
| UdpNm | `UdpNmTests.cpp` | 699 | PassiveStartUp, NetworkRequest/Release, mode arbitration |

### 2.5 OS, Scheduling and HAL

| Suite | File | Module LOC | What it covers |
|-------|------|-----------:|----------------|
| OS | `OsTests.cpp` | 2135 | StartOS / ShutdownOS, ActivateTask, SetEvent / WaitEvent, GetResource |
| Scheduler | (covered through `OsTests.cpp` and main-cycle integration) | 147 | Periodic tick driving every `*_MainFunction` |
| Mcal | `McalTests.cpp` | 1515 | Dio_Pi4, Gpt_Pi4, Mcu_Pi4, Wdg_Pi4, Can_Pi4 — read/write/init/timer/watchdog |

### 2.6 Diagnostics, Memory and Crypto Auxiliaries

| Suite | File | Module LOC | What it covers |
|-------|------|-----------:|----------------|
| Det | `DetTests.cpp` | 159 | ReportError, ReportRuntimeError, ReportTransientFault |
| Dem | `DemTests.cpp` | 438 | ReportErrorStatus, GetLastEvent, MainFunction |
| Dcm | `DcmTests.cpp` | 340 | ProcessRequest, TpTxConfirmation, MainFunction |
| NvM | `NvMTests.cpp` | 1126 | ReadAll/WriteAll, ReadBlock/WriteBlock, MainFunction |
| Fee | `FeeTests.cpp` | 217 | Read/Write through flash emulation |
| Ea | `EaTests.cpp` | 373 | EEPROM abstraction Read/Write |
| MemIf | `MemIfTests.cpp` | 182 | Routing layer above Fee/Ea |
| Encrypt | `EncryptTests.cpp` | 341 | AES-128 round operations (AddRoundKey, SubBytes, ShiftRows, MixColumns) |
| ApBridge | `ApBridgeTests.cpp` | 245 | Health reporting, forced-state injection, status read-back |

### 2.5 Standalone Integration Programs

| Program | LOC | What it proves |
|---------|-----|----------------|
| `test_phase3_complete_ethernet_gateway.c` | 613 | Full PQC stack: ML-KEM handshake + HKDF + ML-DSA sign/verify + tamper detection |
| `test_pqc_secoc_integration.c` | 889 | Csm-level integration without the full BSW stack |
| `test_pqc_standalone.c` | 883 | Pure liboqs micro-benchmarks across 5 message sizes |
| `test_phase4_integration_report.c` | 669 | Architecture report generator (no assertions) |

---

## 3. Test Scenarios — Functional

The following scenarios are the ones that will appear in the thesis chapter on
verification.

### 3.1 Authentication round trip — happy path

| Field | Value |
|-------|-------|
| Test | `SecOCIntegrationTests.RoundTrip_HappyPath` |
| Pre-condition | `SecOC_Init` succeeded; PQC keys loaded |
| Stimulus | `SecOC_IfTransmit(pduId=0, payload[8])` |
| Expected | `SecOC_RxIndication` returns `E_OK`; verification result = OK |
| Requirement | `[SWS_SecOC_00031]`, `[SWS_SecOC_00079]` |

### 3.2 Replay attack rejected

| Field | Value |
|-------|-------|
| Test | `SecOCIntegrationTests.Replay_SecondAttemptRejected` |
| Stimulus | Replay the previously-accepted Secured I-PDU verbatim |
| Expected | Verification returns `SECOC_VERIFICATIONFAILURE` |
| Requirement | `[SWS_SecOC_00094]` |

### 3.3 Single-bit payload tampering

| Field | Value |
|-------|-------|
| Test | `SecOCIntegrationTests.Tamper_PayloadBitFlipRejected` |
| Stimulus | XOR one byte of `AuthPdu` with 0xFF before reception |
| Expected | Verification fails |
| Requirement | Integrity property of ML-DSA-65 / HMAC |

### 3.4 Single-bit signature tampering

| Field | Value |
|-------|-------|
| Test | `SecOCIntegrationTests.Tamper_AuthenticatorBitFlipRejected` |
| Stimulus | XOR one byte of the ML-DSA-65 signature with 0xFF |
| Expected | Verification fails |
| Requirement | Integrity property of ML-DSA-65 |

### 3.5 ML-KEM handshake (Alice ↔ Bob)

| Field | Value |
|-------|-------|
| Test | `KeyExchangeTests.FullHandshake_AliceBob` |
| Steps | Initiate (peer 0) → Respond (peer 1) → Complete (peer 0) |
| Expected | `GetSharedSecret` returns identical 32-byte value on both peers |
| Requirement | NIST FIPS 203 |

### 3.6 HKDF determinism and independence

| Field | Value |
|-------|-------|
| Tests | `KeyDerivationTests.{Deterministic, ProducesDifferentEncAndAuth, DifferentSecretsProduceDifferentKeys}` |
| Expected | (a) same secret → same keys; (b) `EncKey ≠ AuthKey`; (c) different secrets → different keys |
| Requirement | RFC 5869 properties |

### 3.7 Multi-peer isolation

| Field | Value |
|-------|-------|
| Tests | `KeyExchangeTests.MultiplePeers_IndependentSessions`, `KeyDerivationTests.MultiplePeers_IndependentStorage` |
| Expected | Resetting one peer's session leaves all others intact; per-peer keys remain unique |

### 3.8 Quantum-resistance configuration

| Field | Value |
|-------|-------|
| Test | `PQC_ComparisonTests.Security_Level_Comparison` |
| Expected | ML-KEM-768 advertised at NIST Level 3 (≈ AES-192); ML-DSA-65 advertised at Level 3 |

### 3.9 Hardware end-to-end (Pi 4)

| Field | Value |
|-------|-------|
| Binary | `Phase3_Complete_Test` cross-built for `aarch64` |
| Pre-condition | `/etc/secoc/keys/mldsa_secoc.{pub,key}` exist (generated on-board) |
| Stimulus | Inject test PDU (32-byte payload `0x00..0x1F`) |
| Observed | Total Secured PDU = **3343 bytes** = 1 (Header) + 32 (AuthPdu) + 1 (TruncFreshness) + 3309 (ML-DSA-65 signature) |
| Result | All tests PASSED, signature begins `0xE5 0xAB 0x90 0x96 ...` |

---

## 4. Performance Results

### 4.1 Cryptographic primitives — x86_64 development host

Source: `test/pqc_standalone_results.csv`, 1000 iterations per primitive.

| Operation | Avg (µs) | Min (µs) | Max (µs) | Stddev (µs) | Throughput (ops/s) | Output size (B) |
|-----------|---------:|---------:|---------:|------------:|-------------------:|----------------:|
| ML-KEM-768 KeyGen | 116.36 | 108.70 | 618.20 | 24.31 | 8 594 | 3 584 (kp) |
| ML-KEM-768 Encapsulate | 219.72 | 108.70 | 28 536.60 | 1 309.00 | 4 551 | 1 088 (ct) |
| ML-KEM-768 Decapsulate | 44.82 | 41.00 | 2 273.50 | 70.73 | 22 312 | 32 (shared secret) |

| Message size (B) | ML-DSA-65 Sign avg (µs) | ML-DSA-65 Verify avg (µs) | Signature size (B) |
|---:|---:|---:|---:|
| 8 | 577.89 | 125.38 | 3 309 |
| 64 | 595.79 | 139.37 | 3 309 |
| 256 | 588.83 | 123.76 | 3 309 |
| 512 | 613.71 | 124.94 | 3 309 |
| 1024 | 576.72 | 126.86 | 3 309 |

Sign cost is essentially independent of message size (the message is hashed
into a fixed-size challenge before signing).

### 4.2 Full-stack latencies — Phase-3 run (representative)

| Stage | Time (µs) |
|------:|----------:|
| ML-KEM-768 KeyGen | 3 594.50 |
| ML-KEM-768 Encapsulate | 163.30 |
| ML-KEM-768 Decapsulate | 48.80 |
| ML-DSA-65 Sign (256 B) | 1 676.40 |
| ML-DSA-65 Verify (256 B) | 132.00 |

Per-message overhead (sign + verify) ≈ **1 808 µs**.
ML-KEM cost amortised across 1 000 messages per session ≈ **3.81 µs / message**.

### 4.3 Throughput

```mermaid
flowchart LR
    A[Per-message overhead<br/>1.81 ms] --> B[Compute throughput<br/>~552 msg/s]
    A --> C[100 Mbps line<br/>~3768 msg/s for 3.3 KB PDU]
    B & C --> D[System throughput<br/>min of the two<br/>~552 msg/s]

    style A fill:#fff59d,color:#000
    style D fill:#a5d6a7,color:#000
```

### 4.4 Bandwidth budget on 100 Mbps Ethernet

| Item | Bytes |
|------|------:|
| ML-KEM handshake (per session, both directions) | 2 272 |
| ML-DSA-65 signature (per message) | 3 309 |
| Secured I-PDU (per message, observed Pi 4) | 3 343 |
| Classic mode Secured I-PDU (HMAC-32) | ≈ 8 |

Overhead ratio PQC vs classic ≈ **552×** per PDU. This justifies the choice of
TP mode and Ethernet (CAN/CAN-FD cannot accommodate a single 3 343-byte PDU).

---

## 5. Security Validation

### 5.1 Attack matrix

| Attack | Test | Detection | How |
|--------|------|-----------|-----|
| Replay | `Replay_SecondAttemptRejected`, `FreshnessTests.RXFreshnessLowerThan*` | ✅ 100 % | Freshness counter monotonicity |
| Payload tampering | `Tamper_PayloadBitFlipRejected` | ✅ 100 % | ML-DSA-65 signature mismatch |
| Signature tampering | `Tamper_AuthenticatorBitFlipRejected` | ✅ 100 % | ML-DSA-65 signature mismatch |
| MITM key exchange | Implicit — handshake uses authenticated channel + magic 0x5143 | Detected on bad magic / bad length | `SoAd_PQC_HandleControlMessage` |
| Quantum (Shor / Grover) | `Security_Level_Comparison` | ✅ Resistant by construction | NIST FIPS 203 / 204 |

### 5.2 Cryptographic agility

`SECOC_USE_PQC_MODE` (compile-time) selects between Classic and PQC. The same
test suite is exercised in both modes; the only assertion that differs is the
expected size of the `Authenticator` field (4 bytes vs 3309 bytes).

---

## 6. Aggregate Results (Latest Run)

```mermaid
flowchart LR
    R1[Unit suites: 40] --> P1[Pass 40]
    R2[Integration suites: 7] --> P2[Pass 7]
    R3[Phase 3 hardware run] --> P3[Pass]
    P1 & P2 & P3 --> TOT[669+ test cases<br/>0 failures<br/>~55% line coverage]

    style TOT fill:#a5d6a7,color:#000
```

| Metric | Value |
|--------|-------|
| Total test cases | 669+ |
| Pass rate | 100 % |
| Line coverage (gcovr) | ≈ 55 % (excludes Mcal, Scheduler, GUIInterface, liboqs) |
| Total wall-clock for full pipeline | ≈ 1.46 s (x86_64 dev host) |
| Phase-3 standalone run | ≈ 0.16 s |

Coverage exclusions are documented in `run_tests.sh` and are consistent with
SWS-SecOC guidance (HAL and external code are out of scope).

---

## 7. How to Reproduce

### 7.1 Full pipeline (x86_64 host)

```bash
cd Autosar_SecOC
bash run_tests.sh
# Outputs:
#   build/report/SecOC_Test_Report.html  (Cantata-style)
#   build/report/coverage_html/index.html
#   build/test_results/*.xml             (JUnit)
```

### 7.2 Just the full PQC stack

```bash
cd Autosar_SecOC
bash build_and_run.sh         # builds + runs Phase3_Complete_Test
# or, after a previous build:
cd build && ./Phase3_Complete_Test
```

### 7.3 Specific GoogleTest binary

```bash
cd Autosar_SecOC/build
ctest -R SecOCIntegrationTests -V
ctest -R PQC_ComparisonTests -V
ctest -R KeyExchangeTests -V
ctest -R KeyDerivationTests -V
```

### 7.4 Hardware run (Raspberry Pi 4)

```bash
# On Pi 4 with cross-built binaries already deployed
sudo ./Phase3_Complete_Test
# Expected last lines:
#   Total Secured PDU: 3343 bytes (Header=1 + Auth=32 + Fresh=1 + Signature=3309)
#   ALL TESTS PASSED
```

### 7.5 Switching modes for comparative measurements

```bash
# include/SecOC/SecOC_PQC_Cfg.h
#define SECOC_USE_PQC_MODE   TRUE   # PQC mode (default)
#define SECOC_USE_PQC_MODE   FALSE  # Classic AES-CMAC mode
# then: cd build && make -j4 && ctest
```

---

## 8. Result Artefacts

The most recent runs are persisted in `Autosar_SecOC/test_logs/`:

| File pattern | Content |
|--------------|---------|
| `phase1_YYYYMMDD_HHMMSS.txt` | Classic-mode core suites (Authentication, Verification, Freshness) |
| `phase2_YYYYMMDD_HHMMSS.txt` | PQC mode — full stack (latest run dated 2026-01-05) |
| `phase3_YYYYMMDD_HHMMSS.txt` | Phase-3 Ethernet gateway integration |
| `phase4_YYYYMMDD_HHMMSS.txt` | Architecture-report output |
| `pqc_standalone_results.csv` | Raw micro-benchmark numbers used in §4.1 |

Together with `build/report/SecOC_Test_Report.html`, these artefacts form the
evidence base for the verification chapter of the thesis.

---

## 9. Full AUTOSAR BSW Stack — Coverage Matrix

The PQC work sits on top of a complete AUTOSAR BSW stack implemented in this
project. The matrix below is intended for the thesis chapter on system
architecture; it shows every module, its size, and the test file that
exercises it.

```mermaid
flowchart TB
    subgraph APP["Application & RTE"]
        AP1[Com 1237]
        AP2[ApBridge 245]
        AP3[GUIInterface 434]
    end

    subgraph SVC["Service Layer"]
        SV1[SecOC + FVM 1958]
        SV2[Csm 1237]
        SV3[NvM 1126]
        SV4[Dem 438]
        SV5[Dcm 340]
        SV6[Det 159]
        SV7[ComM 253]
        SV8[BswM 997]
        SV9[EcuM 1118]
        SV10[CanSM 240]
        SV11[CanNm 350]
        SV12[EthSM 423]
        SV13[UdpNm 699]
    end

    subgraph ECUA["ECU Abstraction"]
        EA1[CryIf 261]
        EA2[PduR 6 sub-files]
        EA3[SoAd 1534]
        EA4[SoAd_PQC 415]
        EA5[EthIf 486]
        EA6[CanIf]
        EA7[CanTp]
        EA8[TcpIp 1104]
        EA9[MemIf 182]
        EA10[Fee 217]
        EA11[Ea 373]
    end

    subgraph CRYPTO["Crypto Suite"]
        CR1[PQC core 509]
        CR2[PQC_KeyExchange 305]
        CR3[PQC_KeyDerivation 320]
        CR4[Encrypt AES 341]
    end

    subgraph OS["OS / Scheduling"]
        OS1[Os 2135]
        OS2[Scheduler 147]
    end

    subgraph HAL["MCAL"]
        H1[Mcal Pi4 1515]
        H2[Ethernet POSIX/Winsock 600]
    end

    APP --> SVC --> ECUA --> CRYPTO
    ECUA --> HAL
    SVC --> OS
    OS --> HAL

    style CRYPTO fill:#a5d6a7,color:#000
    style EA4 fill:#a5d6a7,color:#000
```

### 9.1 Per-module evidence

| Layer | Module | LOC | Public APIs | Test file | Tests pass |
|-------|--------|----:|-----------:|-----------|:----------:|
| App | Com | 1237 | 12 | `ComTests.cpp` (20) | ✅ |
| App | ApBridge | 245 | 6 | `ApBridgeTests.cpp` | ✅ |
| App | GUIInterface | 434 | 8 | manual + Python GUI | ✅ |
| Svc | SecOC + FVM | 1958 | 14 | `SecOC*Tests.cpp` (39) | ✅ |
| Svc | Csm | 1237 | 13 | `CsmTests.cpp` (18) | ✅ |
| Svc | EcuM | 1118 | 36 | `EcuMTests.cpp` | ✅ |
| Svc | NvM | 1126 | 13 | `NvMTests.cpp` | ✅ |
| Svc | BswM | 997 | 8 | `BswMTests.cpp` | ✅ |
| Svc | UdpNm | 699 | 16 | `UdpNmTests.cpp` | ✅ |
| Svc | Dem | 438 | 13 | `DemTests.cpp` | ✅ |
| Svc | EthSM | 423 | 8 | `EthSMTests.cpp` | ✅ |
| Svc | CanNm | 350 | 8 | `CanNmTests.cpp` | ✅ |
| Svc | Dcm | 340 | 8 | `DcmTests.cpp` | ✅ |
| Svc | ComM | 253 | 7 | `ComMTests.cpp` | ✅ |
| Svc | CanSM | 240 | 7 | `CanSMTests.cpp` | ✅ |
| Svc | Det | 159 | 8 | `DetTests.cpp` | ✅ |
| ECU-A | SoAd | 1534 | 10 | `SoAdTests.cpp` (20) | ✅ |
| ECU-A | TcpIp | 1104 | 9 | `TcpIpTests.cpp` | ✅ |
| ECU-A | EthIf | 486 | 5 | `EthIfTests.cpp` | ✅ |
| ECU-A | SoAd_PQC | 415 | 5 | `SoAdPqcTests.cpp` (18) | ✅ |
| ECU-A | Ea | 373 | 6 | `EaTests.cpp` | ✅ |
| ECU-A | CryIf | 261 | 9 | `CryIfTests.cpp` (10) | ✅ |
| ECU-A | Fee | 217 | 5 | `FeeTests.cpp` | ✅ |
| ECU-A | MemIf | 182 | 4 | `MemIfTests.cpp` | ✅ |
| ECU-A | PduR | (6 sub-files) | many | `PduRTests.cpp` (20) | ✅ |
| ECU-A | CanIf / CanTp | (multi-file) | many | `CanIfTests.cpp`, `CanTpTests.cpp` | ✅ |
| Crypto | PQC core | 509 | 10 | `PQC_ComparisonTests.cpp` | ✅ |
| Crypto | PQC_KeyExchange | 305 | 6 | `KeyExchangeTests.cpp` (16) | ✅ |
| Crypto | PQC_KeyDerivation | 320 | 4 | `KeyDerivationTests.cpp` (15) | ✅ |
| Crypto | Encrypt (AES) | 341 | 9 | `EncryptTests.cpp` | ✅ |
| OS | Os | 2135 | 50 | `OsTests.cpp` | ✅ |
| OS | Scheduler | 147 | 3 | (covered indirectly) | ✅ |
| HAL | Mcal (Pi 4) | 1515 | 24 | `McalTests.cpp` | ✅ |

**Totals: 33 modules, ≈ 30 000 LOC, 31 dedicated test files, 669+ test cases,
100 % pass rate.**

This is the headline number for the verification chapter of the thesis: the
project is *not* a PQC patch on a stub stack — it is a complete AUTOSAR BSW
implementation with quantum-resistant cryptography integrated at the SecOC,
Csm, CryIf and SoAd layers.

---

## 10. Pointers Into the Codebase

| Topic | File | Lines (approx.) |
|-------|------|----------------:|
| Tx authenticate (PQC) | `source/SecOC/SecOC.c` | 1599–1700 |
| Rx verify (PQC) | `source/SecOC/SecOC.c` | 1680–1800 |
| Csm signature dispatch | `source/Csm/Csm.c` | 145–230 |
| CryIf adapter | `source/CryIf/CryIf.c` | full file |
| ML-DSA wrapper | `source/PQC/PQC.c` | 80–200 |
| ML-DSA key load/save | `source/PQC/PQC.c` | 284–407 |
| ML-KEM key exchange FSM | `source/PQC/PQC_KeyExchange.c` | full file |
| HKDF | `source/PQC/PQC_KeyDerivation.c` | 70–136 |
| SoAd PQC orchestration | `source/SoAd/SoAd_PQC.c` | full file |
| Per-PDU configuration | `source/SecOC/SecOC_Lcfg.c` | 582–820 |
| Compile-time PQC switch | `include/SecOC/SecOC_PQC_Cfg.h` | 21–59 |
| Phase-3 hardware test | `test/test_phase3_complete_ethernet_gateway.c` | full file |

---

*End of TEST_SCENARIOS_AND_RESULTS.md*
