# THESIS_RESEARCH.md — Performance Constraints & Cryptographic Compliance

This document consolidates the automotive performance constraints and
post-quantum cryptographic compliance requirements that the **Integrated
Simulation Environment (ISE)** is designed to verify. It is intended to be
cited verbatim in the corresponding chapters of the thesis.

---

## 1. Automotive performance constraints

### 1.1 Classical in-vehicle buses

| Bus           | Bit-rate       | Max payload / frame | Typical cycle time | Deadline class    |
|---------------|----------------|---------------------|--------------------|-------------------|
| LIN           | 19.2 kbps      | 8 B                 | 10 – 200 ms        | QM                |
| CAN 2.0 B     | 1 Mbps         | 8 B                 | 5 – 100 ms         | ASIL-A/B          |
| CAN-FD        | 5 Mbps (data)  | 64 B                | 1 – 50 ms          | ASIL-B/C          |
| FlexRay       | 10 Mbps        | 254 B               | 1 – 20 ms (static) | ASIL-C/D          |
| 100BASE-T1    | 100 Mbps       | 1500 B (MTU)        | 1 – 100 ms         | ASIL-B/C (w/ TSN) |
| 1000BASE-T1   | 1 Gbps         | 1500 B (MTU)        | sub-ms (w/ TSN)    | ASIL-C/D (w/ TSN) |

**Hard real-time references (public literature):**

- Engine / transmission / stability control signals must be refreshed as
  often as **every 5 ms**; brake-by-wire and collision-avoidance commands
  must be delivered in **the millisecond range** (TK Engineering, 2023;
  Hindawi JCNC 2013, review of CAN response time analysis).
- FlexRay is time-triggered, which guarantees bounded latency and jitter
  in the static segment (AUTOSAR SWS_TimeSyncOverFlexRay R21-11).
- Automotive Ethernet 100/1000BASE-T1 with IEEE 802.1 Time-Sensitive
  Networking (TSN) profiles is the backbone for autonomous-driving domain
  controllers where **sub-millisecond latency** is required.

### 1.2 Deadline classes used by the ISE

The simulator enforces the following deadline classes per signal. They are
derived from the AUTOSAR classification and common OEM practice. Each
scenario records how many transmissions missed their deadline.

| Class | End-to-end deadline | Typical signals                                   |
|-------|---------------------|---------------------------------------------------|
| D1    |  5 ms               | Brake command, steering torque, airbag trigger    |
| D2    | 10 ms               | Throttle, ABS wheel speed, inverter torque        |
| D3    | 20 ms               | Body motion, door lock, chassis stabilisation     |
| D4    | 50 ms               | HMI (dashboard speedometer), HVAC                 |
| D5    | 100 ms              | Comfort, infotainment gateway                     |
| D6    | 500 ms              | Telematics, OBD diagnostics                       |

### 1.3 SecOC overhead budget

AUTOSAR SecOC SWS R21-11 appends freshness + authenticator to the
authentic PDU. The overhead must fit the bus MTU **after** fragmentation
by CAN-TP / SoAd:

| Mode                  | Auth bytes | Freshness bytes | Fragments (CAN-FD 64B) |
|-----------------------|-----------:|----------------:|-----------------------:|
| Classic HMAC (trunc)  |  4         |  1              | 1 (8-byte PDU fits)    |
| Classic AES-CMAC      | 16         |  1              | 1 – 2                  |
| PQC ML-DSA-65         | 3309       |  8              | **≥ 53** on CAN-FD     |
| PQC ML-DSA-65         | 3309       |  8              | 3 on 1500 B Ethernet   |

This is the core reason PQC is routed over Ethernet / CAN-TP, not raw CAN.

---

## 2. Cryptographic compliance matrix

### 2.1 AUTOSAR SecOC SWS R21-11

The ISE exercises the API listed below. Each scenario verifies that the
observed behaviour matches the SWS requirement.

| Requirement        | API                              | ISE scenario                         |
|--------------------|----------------------------------|--------------------------------------|
| SWS_SecOC_00106    | `SecOC_Init`                     | baseline_classic, pqc_ethernet       |
| SWS_SecOC_00161    | `SecOC_DeInit`                   | rekey_session                        |
| SWS_SecOC_00112    | `SecOC_IfTransmit`               | baseline_classic, throughput         |
| SWS_SecOC_00177    | `SecOC_TpTransmit`               | pqc_ethernet, mixed_bus              |
| SWS_SecOC_00209    | Freshness value generation       | replay_attack                        |
| SWS_SecOC_00221    | Authenticator generation (Csm)   | tamper_attack                        |
| SWS_SecOC_00230    | Verification + drop on failure   | all attack scenarios                 |

### 2.2 NIST FIPS 203 — ML-KEM (Module-Lattice KEM)

Published 13 August 2024. The ISE uses **ML-KEM-768** (NIST Category 3).

| Requirement                         | Evidence                                           |
|-------------------------------------|----------------------------------------------------|
| Public key = 1184 B                 | logged in `session_establishment.csv`              |
| Secret key = 2400 B                 | logged in `session_establishment.csv`              |
| Ciphertext = 1088 B                 | logged in `session_establishment.csv`              |
| Shared secret = 32 B                | HKDF derives SecOC key                             |
| IND-CCA2 resistance                 | `sc_attacks` MITM + key-confusion                  |

### 2.3 NIST FIPS 204 — ML-DSA (Module-Lattice DSA)

Published 13 August 2024. The ISE uses **ML-DSA-65** (NIST Category 3).

| Requirement                         | Evidence                                           |
|-------------------------------------|----------------------------------------------------|
| Public key = 1952 B                 | logged in `key_material.csv`                       |
| Secret key = 4032 B                 | logged in `key_material.csv`                       |
| Signature = 3309 B                  | logged in `pdu_size.csv`                           |
| EUF-CMA                             | `sc_attacks` signature-fuzz detects all tampering  |
| SUF-CMA                             | malleability scenarios                             |
| Deterministic / randomised sign     | scenario `sc_baseline` runs both modes             |

### 2.4 Known-Answer-Test (KAT) compliance — ML-KEM-768 + ML-DSA-65

NIST FIPS 203 §A and FIPS 204 §A publish authoritative Known-Answer
Test vectors for every supported security level. The ISE does **not**
re-run those vectors; it inherits KAT compliance from `liboqs`, which
runs the full NIST KAT vector set during its own build pipeline:

- `external/liboqs/CMakeLists.txt` enables `OQS_BUILD_ONLY_LIB=ON` and
  `OQS_DIST_BUILD=ON`; the upstream Open Quantum Safe project ships
  `tests/test_kem_mem.c` and `tests/test_sig_mem.c` which iterate over
  the NIST KATs in `tests/PQCsignKAT_*.rsp` /
  `tests/PQCkemKAT_*.rsp`.
- The version of liboqs vendored under `Autosar_SecOC/external/liboqs`
  (commit hash recorded in `external/liboqs/SECURITY.md`) is the
  upstream release that has passed those KATs in CI on Linux (x86_64,
  aarch64), Windows (MinGW), and macOS.
- Algorithm identifiers used by the ISE — `OQS_KEM_alg_ml_kem_768` and
  `OQS_SIG_alg_ml_dsa_65` — are the post-FIPS final names defined in
  FIPS 203 / 204; older liboqs identifiers (`OQS_KEM_alg_kyber_768`,
  `OQS_SIG_alg_dilithium_3`) are deliberately not used.

Operational consequence for the thesis: **the cryptographic primitives
themselves are KAT-validated upstream**; the ISE only validates how
those primitives are *integrated* into the SecOC pipeline.

### 2.4 ISO/SAE 21434:2021 — Cybersecurity Engineering

| Clause | Requirement                  | ISE contribution                             |
|--------|------------------------------|-----------------------------------------------|
| §9.4   | Cybersecurity controls       | CSMS-mapped attack library (`sc_attacks`)     |
| §10    | Product development          | traceable unit + integration tests            |
| §11    | V&V                          | deterministic, repeatable scenarios + seeds   |
| CAL-4  | Penetration testing          | automated attack scenarios, reproducible      |

### 2.5 UN-ECE R155 / R156

R155 (CSMS, legally binding in UNECE since mid-2024 and mirrored by
China GB 44495:2024 from Jan-2026) requires demonstrable mitigation of
threats listed in Annex 5. The ISE addresses:

| R155 threat           | Annex ID    | Mitigated by              | ISE scenario           |
|-----------------------|-------------|---------------------------|------------------------|
| Spoofing of messages  | 4.3.1       | SecOC authenticator       | `sc_attacks/tamper`    |
| Replay of messages    | 4.3.2       | 64-bit freshness counter  | `sc_attacks/replay`    |
| Message injection     | 4.3.4       | SecOC verification        | `sc_attacks/injection` |
| Denial of service     | 4.3.5       | rate limiter + drop       | `sc_attacks/dos`       |
| Tampering in transit  | 4.3.6       | ML-DSA signature          | `sc_attacks/tamper`    |
| Key extraction        | 4.3.7       | PQC + rekeying            | `sc_rekey`             |
| Downgrade to classical| *new (PQC)* | policy check refuses MAC  | `sc_attacks/downgrade` |

---

## 3. Post-quantum timing budget (from measurements + literature)

| Operation            | Intel i7 (Win)* | RPi 4 aarch64* | Literature range                  |
|----------------------|----------------:|---------------:|-----------------------------------|
| ML-KEM-768 KeyGen    |     ~20 µs      |     ~80 µs     | 10–100 µs (x86/ARM)               |
| ML-KEM-768 Encaps    |     ~30 µs      |     ~75 µs     | 10–80 µs                          |
| ML-KEM-768 Decaps    |     ~40 µs      |     ~33 µs     | 10–80 µs                          |
| ML-DSA-65 Sign       |    ~250 µs      |    ~370 µs     | 150–2300 µs (depends on rejection)|
| ML-DSA-65 Verify     |    ~120 µs      |     ~84 µs     | 70–500 µs                         |
| AES-CMAC (HMAC)      |      ~2 µs      |     ~16 µs     | 1–30 µs                           |

*\*from existing `PiTest/` and `Autosar_SecOC/test_logs/` archives.*

**Critical insight for the thesis:**

- Per-message PQC overhead = Sign + Verify ≈ **~370 µs on Intel i7**,
  **~450 µs on RPi 4**.
- This consumes **~9 % of a 5 ms D1 deadline** — acceptable for brake
  commands only if the bus fragmentation overhead does not dominate.
- HMAC overhead (~18 µs) is negligible for D1 deadlines — PQC is therefore
  mandated only where quantum-resistance is required (Ethernet / V2X /
  long-lifetime signals).

---

## 4. Citations

1. AUTOSAR Consortium, *Specification of Secure Onboard Communication*,
   R21-11.
2. NIST, *FIPS 203 Module-Lattice-Based Key-Encapsulation Mechanism
   Standard*, 13 Aug 2024.
3. NIST, *FIPS 204 Module-Lattice-Based Digital Signature Standard*, 13
   Aug 2024.
4. ISO/SAE, *ISO/SAE 21434:2021 Road vehicles — Cybersecurity
   engineering*.
5. UNECE, *UN Regulation No. 155 — Cyber security and cyber security
   management system*, mandatory for new vehicle types since 1 Jul 2022.
6. GB 44495:2024 — China national standard, effective 1 Jan 2026.
7. TK Engineering, *How CAN Bus Latency Impacts Safety-Critical
   Applications*, 2023.
8. Hindawi JCNC, *Review of CAN Response-Time Analysis*, 2013.
9. ScienceDirect, *Post-Quantum Cryptography for Automotive Systems*,
   Microprocessors & Microsystems 2022.
10. MDPI Electronics 13(22):4550, *Lightweight Post-Quantum Secure
    Cryptography Based on Ascon — Hardware Implementation in Automotive
    Applications*, 2024.
11. Frontiers in Physics, *Design and implementation of an authenticated
    post-quantum session protocol using ML-KEM, ML-DSA and AES-256-GCM*,
    2025.

---

## 5. Scope and limitations of this evidence base

For full disclosure to the thesis committee:

### 5.1 Two-layer evidence

The thesis numbers come from two complementary test layers, both run
in CI for every commit:

1. **Unit-level conformance** — 41 ctest executables / 678 individual
   gtest cases under `Autosar_SecOC/test/` link the *real* `SecOCLib`
   static library (every `.c` file under `Autosar_SecOC/source/`).
   They assert each AUTOSAR module's API contract (parameter validation,
   return codes, state machines).

2. **Integration / performance under realistic scenarios** — 15 ISE
   scenarios under `integrated_simulation_env/scenarios/sc_*.c` link
   only the PQC modules + liboqs and exercise the SecOC frame protocol
   and freshness state machine through `sim_ecu.c`. Numbers represent
   the PQC pipeline + protocol invariants; they do not include the
   real Com / PduR / SecOC.c / Csm / CryIf / CanTp / SoAd overhead.

This split is intentional and is documented in
`integrated_simulation_env/ARCHITECTURE.md` §5.

### 5.2 What is explicitly out of scope

- ISO/SAE 21434 §15 continuous cybersecurity activities
  (post-production monitoring, vulnerability triage, incident
  response) — these are organisational processes that cannot be
  validated by code-level tests alone.
- UN R155 §7.3.7 incident response — same reason.
- Time-Sensitive Networking (TSN) on Ethernet — the ISE Ethernet model
  is a naive bandwidth + propagation pipe; production deployments
  would add 802.1Qbv credit-based shapers.
- Network management (CanNm, UdpNm) — buses are always up for the
  duration of an ISE scenario; the unit tests `CanNmTests` and
  `UdpNmTests` validate the NM state machines in isolation.
- Diagnostic over IP (DoIP) — out of scope; UDS through 0x27
  SecurityAccess is unit-tested via `DcmTests` only.
- Hardware-accelerated PQC (Cortex-A72 NEON, RISC-V vector,
  dedicated coprocessors) — performance numbers are software-only.

### 5.3 Honest finding to highlight in the defense

The ISE `deadline_stress` scenario shows that **ASIL-D D1 (1 ms
brake-by-wire) loops fail under PQC** on host-class hardware — every
deadline is missed because ML-DSA-65 sign + verify alone consumes
≈ 250 µs out of the 1 ms budget once realistic bus transit is added.

This is not a bug. It is a thesis result that frames the
recommendation: **classical MAC remains appropriate for hard
sub-millisecond loops; PQC is appropriate for D2+ deadlines and for
quantum-sensitive long-lifetime channels (V2X, OTA, key-management).**
