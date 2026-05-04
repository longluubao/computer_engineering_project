# Thesis Defense Q&A — AUTOSAR SecOC + PQC on Raspberry Pi 4 Ethernet Gateway

**Candidate:** Luu Bao Long et al. (graduation project)
**Thesis title:** "AUTOSAR SecOC with Post-Quantum Cryptography on a Raspberry Pi 4 Ethernet Gateway"
**Defense panel (simulated):** Three subject-matter examiners
1. **Prof. Examiner #1** — AUTOSAR Classic Platform & SecOC architecture
2. **Prof. Examiner #2** — Post-Quantum Cryptography & NIST FIPS 203/204
3. **Prof. Examiner #3** — Automotive cybersecurity, ISO/SAE 21434, UN R155, E/E architecture

**Document compiled:** 2026-05-02 (citations retrieved same date).
**Total questions:** 81 (27 + 28 + 26). Every question carries at least one primary-source citation.

---

## 0 · Executive Summary — Code & Report Audit

### 0.1 What you actually shipped

| Item | Status | Evidence |
|---|---|---|
| AUTOSAR R21-11 BSW stack | 31 modules under `Autosar_SecOC/source/`; 13 full, 11 minimal, 7 stubs | `Autosar_SecOC/source/` directory; `ReportDACN/sections/3.Proposed Solution/3.2*.tex` Table |
| SecOC core | 13 mandatory entry points implemented | `source/SecOC/SecOC.c` |
| PQC integration | ML-KEM-768 + ML-DSA-65 via `Csm → CryIf → PQC` | `source/PQC/PQC.c`, `PQC_KeyExchange.c`, `PQC_KeyDerivation.c` |
| Pi 4 hardware target | aarch64, MCP2515 CAN HAT, ARM-NEON liboqs | `source/Mcal/Pi4/Can_Pi4.c`, `cmake -DMCAL_TARGET=PI4` |
| ISE validation | 52 scenarios, 8,316 frames, 19 Apr 2026 | `integrated_simulation_env/results/20260419T043003Z/` |
| Test suite | 669+ GoogleTest cases across 40 files + 4 standalone | `test/` directory; 100% pass on x86_64 + Pi 4 |
| Headline performance | PQC mean 2,045 µs; p99 3,469 µs; 552 msg/s compute-bound | `latency_stats.csv`, Phase-3 logs |
| Attack detection | 99.14% aggregate; 47/47 true replays rejected | `attack_detection.csv` |
| Compliance | 22/23 PASS; CAL-4 PARTIAL; UN R155 M21 PARTIAL | `compliance_reports/live_audit_20260501/` |

### 0.2 Strengths I'd defend

1. **Honest scope statements** — handshake-auth gap, CAL-4 PARTIAL, M21 filesystem-keys are explicitly disclosed (§3.6 of the report).
2. **Layered integration** — PQC plugged through Csm/CryIf rather than monkey-patching SecOC; the cryptographic-agility hook is real.
3. **Measured numbers** — every quantitative claim traces back to a CSV / log artefact dated and checked into the repo.
4. **Dual-platform build** — same source compiles for Windows x86_64 and Pi 4 aarch64, validating the abstraction.

### 0.3 Weaknesses an examiner will hit

| Issue | Where | Severity |
|---|---|---|
| **Pi 4 + Linux is NOT an AUTOSAR Classic target** — see Cluster 1B | platform-wide | **Critical (must reframe as research demonstrator)** |
| 8 unconditional `Det_ReportError()` calls violate `[SWS_SecOC_00054]` | `source/SecOC/SecOC.c` | High (acknowledged) |
| ML-KEM handshake unauthenticated → active MitM defeats SecOC | `source/PQC/PQC_KeyExchange.c`, `source/SoAd/SoAd.c` | Critical (acknowledged) |
| ML-DSA private key in `/etc/secoc/keys/` (POSIX 0600) — not HSM/TEE | `include/SecOC/SecOC_PQC_Cfg.h:48` | High (acknowledged as PARTIAL) |
| `liboqs` uses `malloc/free` — violates MISRA Rule 21.3 in any Classic build | `external/liboqs/` | High (production blocker) |
| SocketCAN driver in `Mcal/Pi4/Can_Pi4.c` is not an AUTOSAR-conformant MCAL | `source/Mcal/Pi4/Can_Pi4.c` | Medium |
| No OSEK-OS conformance class (BCC1/BCC2/ECC1/ECC2); pthread shim instead | `source/Os/` | Medium |
| No HARA / ASIL classification; "ASIL D2" used informally — no such level | `summary.tex`, `3.7 Performance Evaluation.tex` | Medium |
| liboqs pinned to **0.15.0-rc1** (release candidate), not 0.15.0 final | `external/liboqs/` | Medium |
| Hybrid-mode terminology collides with IETF "hybrid KEM" meaning | `3.4 AUTOSAR SecOC Implementation.tex` | Low |
| ML-KEM rekey logic placed in SoAd rather than BswM/EcuM | `source/SoAd/SoAd.c` | Low (architectural smell) |
| Pi 4 Cortex-A72 numbers don't transfer to Cortex-R52 ECUs | §3.7 | Medium (representativeness) |
| No WCET analysis; latency is empirical p99 on Linux | `latency_stats.csv` | Medium |
| Algorithm-family enum extension for ML-DSA in R21-11 Csm not documented | `source/Csm/Csm.c` | Low |

### 0.4 Bottom line

This is a **strong proof-of-concept for an engineering-grade thesis**. It does not, and does not claim to, deliver a production-grade automotive ECU. The honest scope statements throughout are themselves a defensible contribution. Treat the open issues above as the items to walk into the defense already prepared to discuss.

---

# Part I — Examiner #1 (AUTOSAR Classic & SecOC)

> 27 questions across 11 clusters: Classic vs Adaptive, zonal architecture, SWS conformance, Csm/CryIf, PduR transport, SoAd, AUTOSAR releases, MISRA, ISO 26262/21434, real-time, deviations.

---

## Cluster 1 — Classic vs Adaptive Platform

### Q1.1 — Why Classic Platform R21-11, not Adaptive Platform?

**Question:** "You explicitly chose AUTOSAR Classic Platform R21-11 for an Ethernet gateway running Linux on aarch64. The Adaptive Platform was designed exactly for POSIX-class hardware — why is your work not on AP, where it would arguably belong?"

**Answer:** SecOC is a **Classic-only BSW module**. The Adaptive Platform does not specify SecOC; it relies on `ara::com` E2E protection profiles plus `ara::crypto` KEM/Sign service interfaces inside the SOA binding. A "PQC-into-SecOC" thesis therefore *only makes sense on Classic*. The Pi 4 was chosen as a developer-accessible Linux platform that mirrors the kind of mid-range zonal/gateway ECU that runs Linux today (Renesas R-Car, NXP S32G with Linux on Cortex-A); it is a stand-in for an HPC running a Linux-class OS, not a stand-in for a Cortex-R MCU. Classic gives me the static configuration, no-dynamic-memory MISRA discipline, and the per-PDU MAC/signature pipeline that is the unit of work for SecOC. Trade-off: I forfeit `ara::com`'s service discovery and dynamic deployment, both of which are out of scope for an in-vehicle gateway authenticating I-PDUs.

**Citations:**
- [AUTOSAR Classic Platform standards landing page](https://www.autosar.org/standards/classic-platform)
- [AUTOSAR Adaptive Platform standards landing page](https://www.autosar.org/standards/adaptive-platform)
- [Specification of Secure Onboard Communication, AUTOSAR CP R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_SecureOnboardCommunication.pdf)

### Q1.2 — `ara::com` vs RTE: equivalent or fundamentally different?

**Question:** "On Adaptive, the equivalent of the Classic RTE is `ara::com`. Concretely, how would your PQC integration look on AP?"

**Answer:** The RTE is generated **C glue** over static port-based communication; `ara::com` is a **C++ middleware** with proxy/skeleton classes and dynamic service discovery, typically over SOME/IP-SD. On AP there is no Secured I-PDU abstraction: protection sits as **transformers in the SOME/IP binding** or as `ara::com` E2E profiles on serialised events. A PQC port to AP would replace SecOC with custom transformers that invoke `ara::crypto::cryp::SignerPrivateCtx` for ML-DSA-65 signing. The Classic seam (`Csm_SignatureGenerate` → CryIf → Crypto Driver) is replaced on AP by the `ara::crypto` Crypto Provider abstraction. Functionally my PQC primitive code is portable; the AUTOSAR-side wrapping is not.

**Citations:**
- [Specification of Communication Management, AUTOSAR AP R20-11](https://www.autosar.org/fileadmin/standards/R20-11/AP/AUTOSAR_SWS_CommunicationManagement.pdf)
- [Specification of Cryptography (ara::crypto), AUTOSAR AP R21-11](https://www.autosar.org/fileadmin/standards/R21-11/AP/AUTOSAR_SWS_Cryptography.pdf)

### Q1.3 — Why is SecOC not specified for the Adaptive Platform?

**Answer:** SecOC was scoped specifically for in-vehicle **PDU-based** Classic stacks where the unit of authentication is an I-PDU travelling through PduR. AP is **service-oriented** and uses E2E protection profiles inside the SOME/IP serialisation pipeline. The two solve overlapping but not identical problems. Therefore my design generalises to AP only by transposing the `Csm_SignatureGenerate` call to an `ara::crypto` Signer context inside an AP transformer.

**Citations:**
- [AUTOSAR AP Communication Management R22-11](https://www.autosar.org/fileadmin/standards/R22-11/AP/AUTOSAR_SWS_CommunicationManagement.pdf)
- [AUTOSAR Adaptive Platform standards](https://www.autosar.org/standards/adaptive-platform)

### Q1.4 — POSIX/PSE51 vs Classic OS scheduling

**Answer:** I run on Linux 6.1 mainline. The Classic OS specification is OSEK-style with priority-based preemption, static configuration, and resource ceiling protocol — nothing equivalent runs on stock Linux. My SecOC `MainFunctionTx/Rx` runs as a **periodic thread** on a `SCHED_FIFO` priority, simulating the Classic OS task model. I make **no hard real-time claim**: this would require `PREEMPT_RT` or a partitioned hypervisor (PikeOS, QNX) for ASIL-C/D. The thesis explicitly excludes ISO 26262 ASIL classification. The Pi 4 + Linux configuration is research-grade and I report **measured 99.9th-percentile latency**, not WCET.

**Citations:**
- [Specification of Operating System, AUTOSAR CP R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_OS.pdf)
- [Specification of Operating System Interface, AUTOSAR AP R23-11](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_OperatingSystemInterface.pdf)

---

## Cluster 1B — **The Pi 4 is not a Classic AUTOSAR target** (the most likely line of attack)

> This cluster is the single biggest exposure. Classic AUTOSAR runs on **microcontrollers without an MMU, OSEK-OS based, statically-configured, no Linux**. The Pi 4 is a Cortex-A72 application processor running Linux 6.1 with virtual memory. **No commercial Classic AUTOSAR OS lists Cortex-A as a target.** Be ready for these eight questions.

### Q1.4a — Classic AUTOSAR is OSEK-based and assumes no MMU. Your Pi 4 has Linux with virtual memory. Reconcile.

**Question:** "Classic AUTOSAR OS is built on OSEK — explicitly designed for microcontrollers *without* a memory management unit, with statically-configured tasks, no dynamic process creation, no virtual memory. Your Pi 4 runs Linux 6.1 with full MMU, paging, dynamic allocation, kernel preemption. **You are not running AUTOSAR Classic** in any sense recognised by AUTOSAR — you are running a Linux user-space simulation of the Classic BSW software architecture. Defend."

**Answer:** **Conceded — and it is the honest framing.** What I deliver is a **simulation of the Classic BSW software architecture** running as a Linux user-space process. I do not run an OSEK-OS kernel. The `Os` module in my BSW inventory is marked `Min.` (minimal-but-functional) precisely because it is a thin pthread-based stand-in for the AUTOSAR OS, not a conformant OSEK implementation. The value of the demonstrator is **validating the BSW integration pattern** — Csm → CryIf → Crypto Driver, SecOC TX/RX, FVM, PduR/SoAd routing — not certifying a real-time ECU. This is the **MOPED / ArcticCore-on-Pi research-platform pattern**, not a production AUTOSAR ECU. The §1.4 scope statement should be tightened: "BSW software-architecture demonstrator on a Linux host; not a real-time AUTOSAR ECU."

**Citations:**
- [OSEK Wikipedia — "OSEK is expected to run on microcontroller without memory management unit (MMU)... AUTOSAR consortium reuses the OSEK specifications as part of the Classic Platform"](https://en.wikipedia.org/wiki/OSEK)
- [SWS Operating System AUTOSAR CP R23-11 — OSEK heritage, conformance classes BCC1/BCC2/ECC1/ECC2](https://www.autosar.org/fileadmin/standards/R23-11/CP/AUTOSAR_CP_SWS_OS.pdf)
- [eSOL — "CP is intended for ECUs running at a few dozen MHz"](https://blog.esol.com/autosar_ap_cp_difference) (retrieved 2026-05-02)

---

### Q1.4b — No commercial Classic AUTOSAR OS lists Cortex-A as a target. Cite one.

**Question:** "Show me a single commercially-supported Classic AUTOSAR OS that runs on a Cortex-A application processor with Linux underneath. Vector MICROSAR, EB tresos, ETAS RTA-OS, Mentor VSTAR — list the Cortex-A SKU. **You can't. The whole industry routes Cortex-A traffic through Adaptive Platform.** What target does your code actually run on?"

**Answer:** **You can't, and I won't claim otherwise.** Elektrobit's tresos OS evaluation pack lists **Infineon AURIX TC38XQ / TC4D, NXP S32K14X, Renesas RH850/F1KM, Synopsys Silver, dSPACE VEOS** — all MCUs (TriCore / Cortex-R / Cortex-M) or simulators. ETAS RTA-OS, Vector MICROSAR-OS, Greenhills INTEGRITY-OS-for-AUTOSAR all target Cortex-R / Cortex-M / TriCore / PowerPC. **No Cortex-A in any production Classic OS catalogue.** My `cmake -DMCAL_TARGET=PI4` produces a binary that runs as a Linux ELF, not a flashed image on an MCU. Reframing: my contribution is the **BSW source code** that, with a real RTAOS port, would run on AURIX/S32K/RH850 — the Pi 4 is a **portable simulation host**, not a deployment target.

**Citations:**
- [Elektrobit Classic AUTOSAR OS — "Evaluation package now available for Infineon AURIX TC38XQ, TC4D, NXP S32K14X, Renesas RH850/F1KM, Synopsys Silver and dSPACE VEOS"](https://www.elektrobit.com/products/ecu/eb-tresos/operating-systems/) (retrieved 2026-05-02)
- [Lauterbach AUTOSAR-aware debugging — "TRACE32 support includes AUTOSAR-compliant OSes for Arm/Cortex, MPC55xx, RH850 and TriCore"](https://www.lauterbach.com/supported-platforms/autosar) (retrieved 2026-05-02)

---

### Q1.4c — OSEK-OS conformance class. Which one are you?

**Question:** "Classic AUTOSAR OS inherits OSEK conformance classes: **BCC1** (basic, 1 task per priority), **BCC2** (multiple tasks per priority), **ECC1** (extended, with events), **ECC2** (multiple-tasks-per-priority + events). What conformance class does your `Os` module implement? Where is the OIL/ARXML defining your task list, priorities, resources, alarms, and ISR categories (Cat1/Cat2)?"

**Answer:** **None — and that's the honest answer.** My `Os` module is a thin pthread-based scheduler simulating the periodic main-function model. There is **no OIL file**, **no formal conformance class**, **no Cat1/Cat2 ISR distinction**, **no resource-ceiling protocol**. A real Classic AUTOSAR ECU configures the OS via the OS Generator (OIL/ARXML) which produces statically-allocated task structures, priorities, alarms, schedule tables. My demonstrator skips this layer because it runs in Linux user-space where pthread + `SCHED_FIFO` priorities approximate priority preemption but do not provide deterministic priority ceiling. **A production port** to a real AUTOSAR OS (e.g., EB tresos) would generate the OS configuration from the same SWC and require declaring conformance class **ECC2** (multiple priorities + events for the SecOC main functions and Csm async callbacks).

**Citations:**
- [SWS Operating System AUTOSAR CP R22-11 — Conformance classes §7.1](https://www.autosar.org/fileadmin/standards/R22-11/CP/AUTOSAR_SWS_OS.pdf) (retrieved 2026-05-02)
- [OSEK Wikipedia — Conformance Classes](https://en.wikipedia.org/wiki/OSEK)

---

### Q1.4d — MCAL is chip-vendor-specific. SocketCAN is not an AUTOSAR MCAL.

**Question:** "Your `source/Mcal/Pi4/Can_Pi4.c` calls Linux **SocketCAN** (a kernel subsystem). The AUTOSAR MCAL specification calls for **direct register-level access** to a CAN controller (TX/RX mailbox FIFOs, error handlers, interrupt service routines). SocketCAN abstracts the controller behind a Linux network device. **That is not an AUTOSAR MCAL.** What is it?"

**Answer:** **Conceded — it is a Linux-host shim, not a true MCAL.** A real AUTOSAR MCAL for the MCP2515 would be implemented at register-level over SPI, with direct ISR handlers entered from the NVIC. My `Can_Pi4.c` shim issues `socket(PF_CAN, SOCK_RAW, CAN_RAW)` and uses `read()`/`write()` against `/dev/can0`, which goes through the Linux SocketCAN stack and the in-kernel `mcp251x` driver. This is a **deliberate abstraction-layer compromise** to avoid re-implementing the MCP2515 register protocol on bare metal. It mirrors what the MOPED/ArcticCore project does and is acceptable for a research-platform port. A clean AUTOSAR-conformant MCAL would replace this with a register-level CAN driver written against the MCAL SWS contract.

**Citations:**
- [SWS CAN Driver AUTOSAR CP R21-11 — MCAL register-level requirements](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_CANDriver.pdf) (retrieved 2026-05-02)
- [SICS-SE MOPED Project — ArcticCore-on-Pi reference](https://github.com/sics-sse/moped/tree/master/autosar) (retrieved 2026-05-02)
- [Linux SocketCAN documentation](https://www.kernel.org/doc/Documentation/networking/can.txt) (retrieved 2026-05-02)

---

### Q1.4e — `liboqs` uses dynamic memory. Classic AUTOSAR + MISRA Rule 21.3 forbid it.

**Question:** "`liboqs` is a userspace library that calls `malloc`/`free` inside `OQS_KEM_new()`, `OQS_SIG_new()`, and internally during signing. AUTOSAR Classic *and* MISRA C:2012 Rule 21.3 (Mandatory) **prohibit dynamic memory allocation**. How does the heap-using `liboqs` coexist with the no-malloc rule that Classic AUTOSAR ECUs enforce?"

**Answer:** **It does not, and this is a real production blocker.** A genuine Classic AUTOSAR deployment would replace `liboqs` with a heap-free fork (or PQClean used in static-only mode) and pre-allocate every ML-KEM/ML-DSA buffer at config time. My code calls `OQS_SIG_new(OQS_SIG_alg_ml_dsa_65)` which internally `malloc`s — that's a Mandatory MISRA Rule 21.3 violation in any Classic build. On Linux it works because `malloc` exists; on AURIX without an MMU and with a static-allocation toolchain, the same code would fail to link or fail at first call. **IAV's quantumSAR uses PQClean for exactly this reason.** A production port of my work would require the same: re-vendor the algorithm code into a heap-free implementation, with all sizing pre-computed from FIPS 203/204 §4 parameter tables.

**Citations:**
- [MISRA C:2012 Rule 21.3 — "The memory allocation and deallocation functions of `<stdlib.h>` shall not be used"](https://misra.org.uk/app/uploads/2021/06/MISRA-Compliance-2020.pdf) (retrieved 2026-05-02)
- [PQClean — heap-free PQC reference implementations used by IAV quantumSAR](https://github.com/PQClean/PQClean) (retrieved 2026-05-02)
- [IAV quantumSAR — uses PQClean, not liboqs](https://github.com/iavofficial/IAV_quantumSAR) (retrieved 2026-05-02)

---

### Q1.4f — The MOPED / ArcticCore precedent is research-only.

**Question:** "There is academic precedent for porting Classic AUTOSAR to a Raspberry Pi — the SICS-SE MOPED project (2014, ArcticCore on Pi 1/2/3, AUTOSAR 3.2). The authors describe it as **'an open automotive hardware platform'** for *research and student access*, not a production ECU. Is your work in the same category?"

**Answer:** **Yes, exactly the same category** — and I should say so explicitly in §1.4. The MOPED platform extended ArcticCore (open-source AUTOSAR Classic implementation) to make it runnable on a Pi over CAN, with stated goal: "create an AUTOSAR implementation on a cheap and widely accessible hardware platform... for researchers and students." My contribution is a continuation: I take a similar Pi-based simulation host and **add the PQC integration via Csm/CryIf** that did not exist in MOPED's 2014 toolchain. **The thesis should cite MOPED as prior art** for "Classic-AUTOSAR-on-Pi as a research demonstrator pattern" rather than presenting the Pi 4 as a representative ECU. This cleanly disambiguates the research-vs-production question.

**Citations:**
- [Lennartsson, *Porting AUTOSAR to a high performance embedded platform*, MDH/SICS 2013 thesis](https://www.diva-portal.org/smash/get/diva2:648352/FULLTEXT01.pdf) (retrieved 2026-05-02)
- [Saito et al., *Porting an AUTOSAR-Compliant RTOS to a high performance ARM embedded platform*, ACM SIGBED 2014](https://sigbed.seas.upenn.edu/archives/2014-02/ewili13_submission_9.pdf) (retrieved 2026-05-02)
- [MOPED/SICS-SE GitHub](https://github.com/sics-sse/moped/tree/master/autosar) (retrieved 2026-05-02)

---

### Q1.4g — Adaptive Platform was created precisely for Pi-class hardware. Why not use it?

**Question:** "AUTOSAR Adaptive Platform exists *because* Cortex-A + Linux did not fit Classic. There is even a public 'linux-for-adaptive-autosar-on-raspberry-pi' kernel tree on GitHub. AP is the AUTOSAR-sanctioned answer for your hardware. Yet you stayed on Classic. Defend, knowing that AP gives you `ara::crypto`, `ara::com`, dynamic deployment, and FOTA hooks **for free**."

**Answer:** **The trade-off is between platform fit and integration target.** AP fits the Pi 4 hardware better, but **SecOC is a Classic-only module** — there is no SecOC on AP. AP uses E2E protection profiles and `ara::com` transformers. A "PQC-into-SecOC" thesis only makes sense on Classic; if I had moved to AP, the thesis would be "PQC-into-`ara::crypto`" — a different (and arguably easier) project, because `ara::crypto` already abstracts asymmetric primitives behind a `SignerPrivateCtx` C++ class. **My contribution is specifically the Classic-side integration**, which is the harder seam because Csm/CryIf in R21-11 had no PQC algorithm-family literals. The Adaptive port is **complementary future work** — same primitives, different middleware seam.

**Citations:**
- [AUTOSAR Adaptive Platform standards](https://www.autosar.org/standards/adaptive-platform) (retrieved 2026-05-02)
- [Cryptography (ara::crypto) AP R21-11](https://www.autosar.org/fileadmin/standards/R21-11/AP/AUTOSAR_SWS_Cryptography.pdf) (retrieved 2026-05-02)
- [linux-for-adaptive-autosar-on-raspberry-pi GitHub kernel tree](https://github.com/vical1024/linux-for-adaptive-autosar-on-raspberry-pi) (retrieved 2026-05-02)

---

### Q1.4h — What does your Pi 4 demonstrator actually validate?

**Question:** "Strip the spin and tell me precisely what your Pi 4 results validate, and what they do NOT validate. Be honest — examiners reward honesty over coverage."

**Answer:** **What the Pi 4 results validate:**
1. **The BSW source-code architecture compiles cross-platform** (Windows MinGW + Linux aarch64) without ifdefs in the SecOC core.
2. **The Csm → CryIf → PQC integration pattern is functionally correct** — 47/47 replays rejected, 100% in-session tampering rejected, 669+ tests pass.
3. **ML-DSA-65 + ML-KEM-768 + HKDF-SHA256 work end-to-end** through the full SecOC TX/RX pipeline.
4. **PDU sizing and TP-mode handling work** for 3,343-byte secured PDUs.
5. **Latency *order of magnitude*** is plausible (~ms range), good enough to argue "PQC fits an Ethernet gateway latency budget".

**What the Pi 4 results DO NOT validate:**
1. **Real-time determinism** — no WCET, no OSEK conformance, Linux scheduler not deterministic.
2. **Performance on the actual production target** — Cortex-R52 / TC3xx will be 5-10× slower for sign.
3. **AUTOSAR OS conformance** — there is no OS conformance to check.
4. **MCAL conformance** — SocketCAN is not an AUTOSAR MCAL.
5. **MISRA conformance for liboqs** — heap-using primitives violate Rule 21.3.
6. **HSM/TEE-backed key isolation** — keys live in a regular Linux file.
7. **Secure boot, OTA, firmware integrity** — entirely out of scope.

**The thesis is a software-architecture demonstrator + cryptographic-agility integration study** — those are real contributions. It is **not** a step toward a certified ECU; that requires a different platform.

**Citations:**
- [SWS Operating System AUTOSAR CP R23-11](https://www.autosar.org/fileadmin/standards/R23-11/CP/AUTOSAR_CP_SWS_OS.pdf) (retrieved 2026-05-02)
- [SWS CAN Driver AUTOSAR CP R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_CANDriver.pdf) (retrieved 2026-05-02)
- [MISRA Compliance:2020](https://misra.org.uk/app/uploads/2021/06/MISRA-Compliance-2020.pdf) (retrieved 2026-05-02)

---

## Cluster 2 — Why an Ethernet gateway and not zonal HPC?

### Q1.5 — Domain vs zonal architecture positioning

**Answer:** A CAN-to-Ethernet gateway is **exactly the role of a zonal controller** in a modern E/E architecture (Bosch zone ECU, BMW iDrive 8 cross-domain controllers). The Pi 4 stands in for the Linux-on-Cortex-A class hardware that Tier-1s ship as zonal/gateway nodes. Classic SecOC remains the right tool on the legacy CAN side because the connected ECUs (engine, brake, steering) run Classic AUTOSAR for the next 5-10 years. On the Ethernet side towards a central HPC, MACsec/IPsec/TLS would be additive and orthogonal — my work focuses on the BSW seam (PDU-level authentication that survives traversal between transports), not link encryption.

**Citations:**
- [Bosch — Vehicle integration platform](https://www.bosch-mobility.com/en/solutions/vehicle-computer/vehicle-integration-platform/)
- [Bosch zone ECU](https://www.bosch-mobility.com/en/solutions/control-units/zone-ecu/)

### Q1.6 — Why SecOC over MACsec/IPsec/TLS on the Ethernet leg?

**Answer:** MACsec (IEEE 802.1AE) gives **bulk link authentication** — single-link, hop-by-hop, no end-to-end. SecOC gives **per-PDU end-to-end authentication** that survives crossing CAN ↔ Ethernet inside the gateway. AUTOSAR Classic Tier-1s deploy SecOC because routing through PduR preserves the authenticator end-to-end across mixed transports. The 3,309-byte ML-DSA-65 signature is the price of E2E quantum-resistant authentication above the routing layer. MACsec/TLS could be layered orthogonally — they protect different threat surfaces (e.g., a tap on the Ethernet link) but do not protect against a compromised intermediate ECU. My thesis focuses on the BSW seam.

**Citations:**
- [Specification of Socket Adaptor, AUTOSAR CP R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_SocketAdaptor.pdf)
- [Requirements on Ethernet Support in AUTOSAR R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SRS_Ethernet.pdf)

---

## Cluster 3 — SecOC SWS conformance

### Q1.7 — `[SWS_SecOC_00054]`: your acknowledged Det gating deviation

**Question:** "Your `SecOC.c` calls `Det_ReportError()` unconditionally — eight call sites. SWS_SecOC_00054 requires those calls be enclosed by `#if (SECOC_DEV_ERROR_DETECT == STD_ON)`. Walk me through (a) why this is non-conformance, (b) the production impact, (c) why a thesis claiming '22 of 23 PASS' should not have failed this clause."

**Answer:** **I acknowledge this as a real conformance deviation**, recorded in §3.4 of the thesis (compliance matrix's single WARN row). Impact: in production builds with `SECOC_DEV_ERROR_DETECT == STD_OFF`, every Det call is dead code that still pulls in the Det entry-point linkage, inflating ROM and preventing a Det-free production build. The fix is mechanical — wrap the calls in `#if`. It does not affect cryptographic correctness or replay-detection behaviour. On scoring: my 22/23 matrix evaluates a different requirement set focused on the cryptographic and PDU-format clauses; SWS_SecOC_00054 is correctly listed as the WARN in the live audit (`generate_compliance_report.py`: 35 PASS, 1 WARN, 0 FAIL on commit `2a031b9`). I should have been clearer that the 22/23 figure is a *security-clause subset*, not the full SecOC SWS.

**Citations:**
- [SWS_SecOC R21-11 — `[SWS_SecOC_00054]`](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_SecureOnboardCommunication.pdf)
- [SWS Default Error Tracer R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_DefaultErrorTracer.pdf)
- [General Specification of BSW Modules R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_BSWGeneral.pdf)

### Q1.8 — DataToAuthenticator construction order

**Answer:** Per `[SWS_SecOC_00031]`, `DataToAuthenticator = Data Identifier (optional) || Authentic I-PDU || Complete (non-truncated) Freshness Value` — the **full** freshness is signed/MACed; the **truncated** freshness goes on the wire. `[SWS_SecOC_00220/00221/00222]` mandate big-endian alignment. My implementation in `constructDataToAuthenticatorTx()` (SecOC.c:251) follows this order: `MessageID || Payload || Freshness` (big-endian), then ML-DSA signs it. Receiver-side reconstruction is in `constructDataToAuthenticatorRx()` (SecOC.c:1287). The truncated freshness on the wire is 8 bits (`SECOC_TX_FRESHNESS_VALUE_TRUNC_LENGTH = 8`), full value 24 bits (`SECOC_TX_FRESHNESS_VALUE_LENGTH = 24`); receiver reconstructs full freshness from FVM history.

**Citations:**
- [SWS_SecOC R21-11 — clauses 31, 220-222](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_SecureOnboardCommunication.pdf)
- [SecOC PRS R21-11](https://www.autosar.org/fileadmin/standards/R21-11/FO/AUTOSAR_PRS_SecOcProtocol.pdf)

### Q1.9 — Truncated freshness: why is 8 bits enough?

**Answer:** With 8-bit truncation and a 24-bit full counter, the receiver uses the upper 16 bits of its stored full freshness plus the 8 bits from the PDU. The signed-distance resync window is **±128 frames** — beyond that the receiver cannot tell whether the freshness moved forward or wrapped. On a 500 kbps CAN bus with this PDU at, say, 100 Hz, that's a ~1.3 s outage tolerance — adequate for typical bus quality. For PQC mode the freshness is conceptually 64 bits in software (the `uint32` sim-side counter in `test_pqc_secoc_integration.c`); the 8-bit wire field is a CAN-side compatibility relic. If we expected longer outages, `SecOCFreshnessValueTruncLength` could be raised. The Rosenstatter PRDC 2019 paper (cited in the thesis) explores this exact trade-off.

**Citations:**
- [SWS_SecOC R21-11 — Freshness chapter](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_SecureOnboardCommunication.pdf)
- [Rosenstatter et al., PRDC 2019](https://rosenstatter.net/thomas/files/prdc2019ExtendingAUTOSAR.pdf)

### Q1.10 — Authentication Build-Up rule

**Answer:** SWS requires `SecOCAuthenticationBuildAttempts` configurable retries with adjusted freshness assumptions before declaring `E_NOT_OK`. My current implementation does **a single freshness-resync attempt** before discard (failure path in `verify_PQC()` at SecOC.c:1556). For a robust SecOC stack the SWS expects a per-PDU counter that increments on failed verifications and resets on success, allowing the receiver to walk the freshness window. **I should expand this to the full SWS-mandated build-up state machine** in a v2 — currently a documented simplification.

**Citations:**
- [SWS_SecOC R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_SecureOnboardCommunication.pdf)

### Q1.11 — PduR routing path for Secured I-PDU

**Answer:** Two distinct PduR routing tables in the same PduR module: (1) **upward** Authentic I-PDU → SecOC; SecOC produces Secured I-PDU and calls back via `PduR_SecOCTransmit()`, which (2) **downward** routes the Secured I-PDU to CanIf or SoAd. On Rx the path is mirrored: lower-layer indication → PduR → SecOC; after auth, PduR forwards Authentic I-PDU to COM. In my code `SoAd_IfTransmit()` (SoAd.c:654) is the downstream send and `SoAdTp_RxIndication()` (SoAd.c:1178) is the upstream receive.

**Citations:**
- [SWS PDU Router R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_PDURouter.pdf)
- [SWS_SecOC R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_SecureOnboardCommunication.pdf)

---

## Cluster 4 — Csm / CryIf cryptographic agility

### Q1.12 — Where exactly does ML-DSA enter Csm?

**Answer:** Call chain: `SecOC_IfTransmit()` (SecOC.c:189) → `authenticate_PQC()` (SecOC.c:1429) → `Csm_SignatureGenerate(jobId, mode, dataPtr, dataLen, sigPtr, sigLenPtr)` (Csm.c:806) → `CryIf_SignatureGenerate()` (CryIf.c:107) → `PQC_MLDSA_Sign()`. I use `Csm_SignatureGenerate`, **not** `Csm_MacGenerate` (which is HMAC). The `jobId` references a `CsmJob` configured to point at a `CsmKeyElement` of asymmetric-private type. PQC.c is registered as a Crypto Driver behind CryIf. The verify chain mirrors this: `Csm_SignatureVerify` (Csm.c:881) → `CryIf_SignatureVerify` (CryIf.c:132) → `PQC_MLDSA_Verify`.

**Citations:**
- [SWS Crypto Service Manager R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_CryptoServiceManager.pdf)
- [SWS Crypto Interface R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_CryptoInterface.pdf)
- [SWS Crypto Driver R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_CryptoDriver.pdf)

### Q1.13 — Algorithm-family identifier — did you extend the enum?

**Answer:** R21-11's `Crypto_AlgorithmFamilyType` enum has no ML-DSA literal. I used **a reserved vendor-specific value** above the standardised range to preserve ABI compatibility (the same approach IAV's quantumSAR took for R23-11). My `CsmKeyElement` and `CryptoPrimitive` configuration are consistent with this vendor-specific value. R25-11 has finally added PQC algorithm families to the standard enum, so a future port would simply remap. This does mean my code is **R21-11 specific** — a clean port to R25-11 would refactor the enum constants.

**Citations:**
- [SWS Crypto Service Manager R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_CryptoServiceManager.pdf)
- [SWS Crypto Driver R25-11](https://www.autosar.org/fileadmin/standards/R25-11/CP/AUTOSAR_CP_SWS_CryptoDriver.pdf)
- [IAV quantumSAR](https://github.com/iavofficial/IAV_quantumSAR)

### Q1.14 — Asynchronous Csm jobs and signing latency

**Answer:** ML-DSA-65 sign on Pi 4 aarch64 measures ~1,498 µs standalone, ~1,676 µs in Phase-3 full stack. I use **synchronous** `Csm_SignatureGenerate` for the demonstration. This blocks the SecOC main function for the signing duration. On a real ECU at 10 ms cycle this is **unacceptable**: the correct production design is async (`CRYPTO_PROCESSING_ASYNC`) with a callback that completes the secured PDU off the main path. I have **not measured main-function jitter** under load — that's a documented gap. The sync-mode design is acceptable for demonstration; for a real-time deployment I would refactor SecOC's authenticate path to a state machine that queues PDUs awaiting signature.

**Citations:**
- [SWS Crypto Service Manager R21-11 — sync/async modes](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_CryptoServiceManager.pdf)

### Q1.15 — Key import and CryptoKey lifecycle

**Answer:** Boot-time key provisioning has four modes (in `include/Csm/Csm.h`): `CSM_MLDSA_BOOTSTRAP_DEMO_FILE_AUTO`, `..._FILE_STRICT`, `..._PROVISIONED`, `..._HSM_HANDLE`. Default production: `FILE_STRICT` — missing key file = hard init failure. Csm.c (lines 453-492) calls `PQC_MLDSA_LoadKeys()` via `CryIf_MldsaLoadKeys()`, opening `mldsa_secoc.pub` (1,952 B) and `mldsa_secoc.key` (4,032 B) from `/etc/secoc/keys/`. In `HSM_HANDLE` mode the key never enters software memory; Csm gets only a handle that CryIf forwards to the HSM driver. **In the current prototype the key resides in user-space RAM after import** — this is acknowledged as the PARTIAL on UN R155 Annex 5 M21.

**Citations:**
- [SWS Crypto Service Manager R21-11 — Key Management API](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_CryptoServiceManager.pdf)

---

## Cluster 5 — PduR & transport

### Q1.16 — TP mode for the 3,343-byte Secured I-PDU

**Answer:** IF-mode (single frame) caps at 8 B (CAN), 64 B (CAN-FD), MTU on Ethernet — no way to send a 3,343-byte PDU as a single PDU. Therefore **TP-mode through SoAd** on Ethernet is mandatory; underlying TCP gives reliable, ordered, flow-controlled delivery. On classical CAN at 500 kbps with 7 B payload per consecutive frame, the same PDU would need ~478 frames — **architecturally infeasible**, which is exactly why I restrict PQC to Ethernet and keep HMAC on CAN. The CAN-side fragmentation count in my ISE results (mean 114 fragments, max 416) substantiates this design call.

**Citations:**
- [SWS CAN Transport Layer R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_CANTransportLayer.pdf)
- [SWS PDU Router R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_PDURouter.pdf)
- [SWS Socket Adaptor R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_SocketAdaptor.pdf)

### Q1.17 — 8-byte receive buffer "violation"

**Answer:** Original code had `BUS_LENGTH_RECEIVE = 8` in `include/Ethernet/ethernet.h`. With a 3,309-byte ML-DSA-65 signature, this violated PduR/SecOC TP semantics: TP reception uses `StartOfReception` (announced size) → repeated `CopyRxData` chunks → `TpRxIndication`; an 8-B static buffer cannot accept the announced size. The fix raised `BUS_LENGTH_RECEIVE = 4096` and propagated to four files (`include/Ethernet/ethernet.h`, `..._windows.h`, `source/Ethernet/ethernet.c`, `..._windows.c`). This is documented as the buffer-overflow lesson in §3.6.

**Citations:**
- [SWS PDU Router R21-11 — TP API](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_PDURouter.pdf)
- [SWS_SecOC R21-11 — `SecOC_StartOfReception`/`SecOC_CopyRxData`](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_SecureOnboardCommunication.pdf)

### Q1.18 — CanTp vs LdCom: why not LdCom for a pure gateway?

**Answer:** **A fair architectural critique.** A gateway that only forwards I-PDUs without signal-level processing should arguably use LdCom (Large Data COM) rather than COM. I route through COM for compatibility with my application-layer test bench and for signal-level introspection in the GUI. This is a **demonstrator choice** — a production gateway would use LdCom on the gateway path, and COM only at endpoints that consume signals.

**Citations:**
- [BSW Module List R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_TR_BSWModuleList.pdf)
- [SWS PDU Router R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_PDURouter.pdf)

---

## Cluster 6 — SoAd & Ethernet stack

### Q1.19 — Why is SoAd triggering ML-KEM rekey?

**Answer:** **Architectural smell, acknowledged.** The correct AUTOSAR placement is a BswM mode-request + action list calling `PQC_KeyExchange_Trigger()`, with EcuM partial-network or BswM rules scheduling it. In my code the rekey timer (`SOAD_PQC_REKEY_INTERVAL_CYCLES = 360000`, ~1 h at 10 ms) sits in SoAd because that's where the Ethernet socket lifecycle is observable in the demonstrator. For a clean refactor: define a BswM mode `SECOC_REKEY_REQUIRED`, drive it from a periodic timer SWC, have BswM rule call PQC. This goes on the production-roadmap backlog.

**Citations:**
- [SWS Socket Adaptor R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_SocketAdaptor.pdf)
- [SWS BSW Mode Manager R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_BSWModeManager.pdf)
- [Mode Management Guide R22-11](https://www.autosar.org/fileadmin/standards/R22-11/CP/AUTOSAR_EXP_ModeManagementGuide.pdf)

### Q1.20 — PDU-to-socket binding

**Answer:** I bind one PduId per SoAd Socket Connection on TCP (port 12345). TCP gives reliable + ordered + flow-controlled delivery for the multi-segment Secured I-PDU. A `SoAdPduHeaderId` is configured to discriminate PDUs if multiple share a connection (currently I use one PduId per socket for the gateway demonstrator). Message-acceptance policy whitelists peer IP/port (one-to-one connection). Per-PduId binding simplifies the demonstration and avoids cross-PDU head-of-line blocking.

**Citations:**
- [SWS Socket Adaptor R21-11 — Ch.7 PDU-to-Socket mapping](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_SocketAdaptor.pdf)

---

## Cluster 7 — R21-11 vs R23-11 / R25-11 and the field

### Q1.21 — Has cryptographic agility changed in R25-11?

**Answer:** R25-11 (released Nov 2025) **adds a Post-Quantum Cryptography section to the FO Security Overview** and the CP Crypto Driver SWS evolved to include standardised PQ algorithm-family identifiers. The Csm/CryIf/Crypto-Driver interface contract is unchanged — my R21-11 integration is **functionally compatible**; a port to R25-11 would simply replace my vendor-specific algorithm-family enum with the now-standard literals. AUTOSAR has not changed the cryptographic-agility hook structure, validating the integration pattern.

**Citations:**
- [AUTOSAR R25-11 release announcement](https://www.autosar.org/news-events/detail/release-r25-11-is-now-available)
- [FO Security Overview R25-11](https://www.autosar.org/fileadmin/standards/R25-11/FO/AUTOSAR_FO_EXP_SecurityOverview.pdf)
- [SWS Crypto Driver R25-11](https://www.autosar.org/fileadmin/standards/R25-11/CP/AUTOSAR_CP_SWS_CryptoDriver.pdf)

### Q1.22 — Compare to IAV quantumSAR

**Answer:** **IAV quantumSAR (escar Europe 2025)** delivers a Crypto Driver layer for AUTOSAR R23-11 microcontrollers using **PQClean**. Mine targets a Linux gateway with **liboqs**, includes the full SecOC Tx/Rx integration plus the rekeying scheduler and CAN+Ethernet gateway scenario — the integration is broader at the expense of microcontroller-grade hardening. quantumSAR is more advanced on (a) MISRA-clean C, (b) µC compatibility, and (c) covering more algorithms (SLH-DSA, FN-DSA, HQC). Mine is more advanced on (a) end-to-end SecOC TX/RX with FVM, (b) Ethernet TP-mode for large signed PDUs, (c) ISE attack-injection campaign. The two are complementary; in production a Tier-1 would combine quantumSAR's MCU driver with my SecOC integration pattern.

**Citations:**
- [IAV quantumSAR](https://github.com/iavofficial/IAV_quantumSAR)
- [IAV at escar Europe](https://www.iav.com/news/iav-at-escar-europe)
- [escar Europe](https://escar.info/escar-europe)

---

## Cluster 8 — MISRA C:2012 / coding standards

### Q1.23 — MISRA baseline and amendment status

**Answer:** I claim **MISRA C:2012 baseline** with the 2020 Compliance framework. My CI threshold is `MISRA baseline 500` (committed in `Autosar_SecOC/CMakeLists.txt`). I distinguish Mandatory (no deviations), Required (deviations require sign-off), and Advisory rules. I have **not** produced a formal Compliance:2020 deviation register — that's a thesis-scope simplification. A production submission would include a register listing each violation, classification, justification, scope of impact, and approver.

**Citations:**
- [MISRA Compliance:2020](https://misra.org.uk/app/uploads/2021/06/MISRA-Compliance-2020.pdf)

### Q1.24 — 8 unconditional Det calls vs MISRA

**Answer:** Yes — independent of `[SWS_SecOC_00054]`, the unconditional Det calls also touch **MISRA Rule 2.1** (no unreachable code in production builds with DET=OFF) and **Directive 4.1** (run-time failures shall be minimised). I have **not** logged a formal MISRA Compliance:2020 deviation for this — without that register entry my MISRA compliance claim is unsupported. The fix is the same one-line preprocessor wrap that addresses SWS_SecOC_00054.

**Citations:**
- [MISRA Compliance:2020](https://misra.org.uk/app/uploads/2021/06/MISRA-Compliance-2020.pdf)
- [SWS Default Error Tracer R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_DefaultErrorTracer.pdf)

---

## Cluster 9 — ISO 26262 / 21434 absence

### Q1.25 — No HARA, no ASIL: defensible?

**Answer:** **Yes, scoped explicitly.** §1.4 states "no formal ISO 26262 safety analysis, no ASIL classification". The 5 ms latency budget I cite is a **representative real-time target** consistent with literature on chassis/powertrain control (typically 5–10 ms cycle), **not an ASIL allocation**. A production roadmap would add: item definition, HARA (S, E, C → ASIL), safety goals, FSC/TSC, software safety requirements, HW-SW interface. ASIL-B is the realistic ceiling for a Linux-based gateway; ASIL-D requires HW lockstep on Cortex-R + a different OS (PikeOS, QNX, AUTOSAR OS on lockstep R52). The "ASIL D2" wording is **informal** and I retract it in favour of "5 ms target representative of timing-relevant signals."

**Citations:**
- [LHP — ASIL in ISO 26262](https://www.lhpes.com/blog/what-is-an-asil)
- [MDPI Sensors 24(6):1848 — ISO 26262 / 21434 co-analysis](https://www.mdpi.com/1424-8220/24/6/1848)

### Q1.26 — ISO/SAE 21434 CAL and TARA

**Answer:** Self-assessed CAL claim: CAL-1 PASS (MISRA baseline), CAL-2 PASS (669+ test cases), CAL-3 PASS (vulnerability analysis via ISE), **CAL-4 PARTIAL** (no third-party penetration test). The TARA outcome at vehicle level for a gateway is typically CAL 3-4. R155 Annex 5 mitigations addressed: communication-channel manipulation, replay (FVM strict monotonicity), signature forgery (ML-DSA verify), partial key-compromise mitigation via 1 h rekey. **M21 PARTIAL** for filesystem-stored long-term keys — explicitly disclosed.

**Citations:**
- [UN R155 Annex 5](https://unece.org/sites/default/files/2023-02/R155e%20(2).pdf)
- [MDPI Sensors 24(6):1848](https://www.mdpi.com/1424-8220/24/6/1848)

---

## Cluster 10 — Determinism, real-time, side-channel

### Q1.27 — WCET of ML-DSA-65 sign

**Answer:** I report **measured 99.9th-percentile latency**, not true WCET. Methodology: 1,000-iteration micro-benchmarks under `SCHED_FIFO`, isolated CPU. True WCET on Linux is **infeasible without static analysis** (e.g., aiT) and a non-Linux RTOS. ML-DSA timing is data-dependent because of rejection sampling — even bare-metal would need a margin. My Phase-3 full-stack number (1,676 µs sign mean on x86_64) includes Csm dispatch + CryIf routing + memory copies; the standalone ML-DSA-65 sign is 589 µs. The 1,808 µs per-message overhead I quote is the **representative** value for SecOC compute throughput.

**Citations:**
- [SWS OS R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_OS.pdf)
- [SWS CSM R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_CryptoServiceManager.pdf)

### Q1.28 — Is liboqs ML-DSA constant-time on Cortex-A72?

**Answer:** **No, I have not verified this.** Open Quantum Safe's FAQ explicitly states liboqs is for prototyping. ML-DSA's rejection-sampling structure is data-dependent; even constant-time C is not constant-time on a CPU with branch predictor + caches. mlkem-native (in liboqs 0.15.0 final) has formal constant-time proofs; ML-DSA in liboqs does not. My pinned version is `0.15.0-rc1` which uses the older path. I would not use this code in a vehicle exposed to power/EM analysis without an HSM or a hardened replacement implementation.

**Citations:**
- [Open Quantum Safe FAQ](https://openquantumsafe.org/faq.html)
- [liboqs releases](https://github.com/open-quantum-safe/liboqs/releases)
- [OQS ML-DSA page](https://openquantumsafe.org/liboqs/algorithms/sig/ml-dsa.html)

### Q1.29 — Why ML-DSA-65 (level 3), not 44 or 87?

**Answer:** Three parameter sets in FIPS 204: ML-DSA-44 (level 2), -65 (level 3), -87 (level 5). I picked **level 3** because: (1) signature 3,309 B vs 4,627 B for level 5 — the bandwidth difference matters on Ethernet TP-mode; (2) Category 3 ≈ AES-192 classical equivalent gives sufficient quantum margin for typical vehicle service life; (3) cryptographic *agility* — re-flashable algorithm — matters more than picking the highest level today. NIST IR 8547 explicitly recommends migration before 2035, and ML-DSA-65 is the FIPS 204 default in many deployments. Level 5 is justified for OEM root keys; per-PDU signing benefits more from level-3 throughput.

**Citations:**
- [NIST FIPS 204](https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.204.pdf)
- [CSRC FIPS 204 page](https://csrc.nist.gov/pubs/fips/204/final)

### Q1.30 — Why ML-KEM-768 and not 512 or 1024?

**Answer:** Symmetric reasoning. ML-KEM-768 ciphertext = 1,088 B; ML-KEM-1024 = 1,568 B. Hourly rekey makes the bandwidth difference negligible. I matched **ML-KEM-768 to ML-DSA-65 at the same NIST security level (3)** for consistent strength. Level 1 (-512) is too aggressive for long-lived automotive infrastructure (~64-bit post-quantum margin under Grover).

**Citations:**
- [NIST FIPS 203](https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.203.pdf)
- [CSRC FIPS 203 page](https://csrc.nist.gov/pubs/fips/203/final)

---

## Cluster 11 — Closing

### Q1.31 — How many SWS clauses fail, in total?

**Answer:** The 22/23 figure refers to my **security-clause subset**. The broader live audit (`generate_compliance_report.py` on commit `2a031b9`) reports **35 PASS, 1 WARN, 0 FAIL**. The single WARN is `[SWS_SecOC_00054]` (unconditional Det). I should restate the headline as "22 of 23 security-clause PASS plus 1 WARN on Det gating," or use the 35/1/0 figure consistently. The current presentation can read as cherry-picking; a fair examiner is right to flag it.

**Citations:**
- [SWS_SecOC R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_SecureOnboardCommunication.pdf)

### Q1.32 — 13 mandatory SecOC entry points: full set?

**Answer:** Mandatory entry points implemented: `SecOC_Init`, `SecOC_DeInit`, `SecOC_GetVersionInfo`, `SecOC_MainFunctionRx`, `SecOC_MainFunctionTx`, `SecOC_IfTransmit`, `SecOC_IfRxIndication`, `SecOC_IfTriggerTransmit`, `SecOC_IfTxConfirmation`, `SecOC_TpRxIndication`, `SecOC_TpTxConfirmation`, `SecOC_StartOfReception`, `SecOC_CopyRxData`, `SecOC_CopyTxData`. Optional/stubbed: `SecOC_VerifyStatusOverride`, `SecOC_GetRxFreshness*` callbacks, key-update services. Honest answer: the 13 are the *core* set; some optional services are stubs. I should expand the inventory in §3.4 of the report.

**Citations:**
- [SWS_SecOC R21-11 — Ch.8 API specification](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_SecureOnboardCommunication.pdf)

### Q1.33 — Filesystem keys vs UN R155 Annex 5

**Answer:** Threat model: a Pi-rooted attacker reads `/etc/secoc/keys/mldsa_secoc.key` → forges signatures → injects authenticated PDUs. **This breaks every claim in the security analysis.** Mitigation paths:
1. **HSM/secure-element** (production answer; AURIX TC3xx HSM, NXP S32G HSE).
2. **Linux TPM2 + sealed keys** — `tpm2-tools` + `keyctl trusted` keyring.
3. **ARM TrustZone OP-TEE** — secure-world key storage with TA invocation.

The `CSM_MLDSA_BOOTSTRAP_HSM_HANDLE` mode is the architectural placeholder for path 1. I disclose this explicitly in §3.6 and §5 (Future Work).

**Citations:**
- [UN R155 Annex 5](https://unece.org/sites/default/files/2023-02/R155e%20(2).pdf)
- [SWS Crypto Driver R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_CryptoDriver.pdf)

### Q1.34 — Tier-1 design review: what would you change?

**Answer:** **Three fix-before-meeting:** (1) Wrap Det calls in `#if SECOC_DEV_ERROR_DETECT == STD_ON`; (2) Replace filesystem keys with HSM/TPM2 path; (3) Move ML-KEM rekey scheduler from SoAd to BswM. **Three defend-as-is:** (1) Layered Csm/CryIf/Crypto-Driver seam; (2) ML-DSA-65 / ML-KEM-768 parameter choice at NIST level 3; (3) TP-mode handling of 3,343-byte secured PDUs with `BUS_LENGTH_RECEIVE = 4096`.

**Citations:**
- [SWS_SecOC R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_SecureOnboardCommunication.pdf)
- [BSW General R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_BSWGeneral.pdf)

---

# Part II — Examiner #2 (PQC & NIST FIPS 203/204)

> 28 questions across 15 clusters: NIST process, FIPS 203/204 internals, security-level choice, side channels, randomness, hybrid mode terminology, unauthenticated handshake, forward secrecy, non-repudiation, alternative PQ algorithms, liboqs maturity, quantum-attack realism, throughput, HKDF.

---

## Cluster 12 — NIST PQC standardization process & timeline

### Q2.1 — Why August 13, 2024, and what does "finalised" mean?

**Answer:** Timeline: 2016 NIST Call for Proposals; 69 round-1 submissions; rounds 2 (2019), 3 (2020). **July 2022:** NIST announced Kyber (KEM) + Dilithium / Falcon / SPHINCS+ (signatures) for standardisation; round 4 continued for code-based KEMs. **Aug 2023:** initial public drafts (ipd) of FIPS 203/204/205. **13 Aug 2024:** finalisation in the Federal Register. The 18-month gap from selection to publication is the standards-drafting and public-comment phase. **"Final" means published — not CMVP-validated**; FIPS-validated implementations of ML-KEM/ML-DSA are still emerging in 2025-2026. NIST IR 8413 (the round-3 status report) is the canonical rationale document.

**Citations:**
- [NIST press release 13 Aug 2024](https://www.nist.gov/news-events/news/2024/08/nist-releases-first-3-finalized-post-quantum-encryption-standards)
- [NIST press release July 2022](https://www.nist.gov/news-events/news/2022/07/pqc-standardization-process-announcing-four-candidates-be-standardized-plus)
- [NIST IR 8413](https://nvlpubs.nist.gov/nistpubs/ir/2022/NIST.IR.8413.pdf)

### Q2.2 — Why Kyber/Dilithium, not NTRU or Saber?

**Answer:** All three are lattice-based KEMs. Kyber won on a balance of (a) **Module-LWE** as the assumption (intermediate between LWE and Ring-LWE — better security analysis), (b) competitive sizes/perf, (c) IP licensing cleared after the 2022 patent settlement. NTRU is older and well-studied but had keys ~30% larger; NIST also avoided incumbency to prevent IP drama. Saber uses Module-LWR and is faster but had less analysis history. NIST IR 8413 §3 lays out the rationale matrix. My thesis ties to MLWE because that's the standardised choice — not because MLWE is fundamentally better than NTRU.

**Citations:**
- [NIST IR 8413 §3](https://nvlpubs.nist.gov/nistpubs/ir/2022/NIST.IR.8413.pdf)
- [Kyber paper IACR 2017/634](https://eprint.iacr.org/2017/634)

### Q2.3 — Why is FIPS 205 (SLH-DSA) called a "backup"?

**Answer:** SLH-DSA is **stateless hash-based** — security rests *only* on hash preimage/collision resistance, an **orthogonal assumption** to lattices. NIST IR 8413 §4 keeps SLH-DSA as a hedge against future MLWE cryptanalysis. SLH-DSA signatures are 7,856–49,856 B and signing is ~10× slower than ML-DSA, but verification is fast. **My design hard-codes ML-DSA-65 in CSM** — algorithm agility for swap to SLH-DSA is missing. A production system would expose at least two `Csm_AsymPublicKeyTypes` / `CsmJobs` so SLH-DSA could replace ML-DSA for handshake-time auth without code changes.

**Citations:**
- [FIPS 205 PDF](https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.205.pdf)
- [NIST IR 8413 §4](https://nvlpubs.nist.gov/nistpubs/ir/2022/NIST.IR.8413.pdf)

### Q2.4 — Round 4 and HQC selection

**Answer:** **March 2025:** NIST announced HQC (Hamming Quasi-Cyclic) as the second standardised KEM, in NIST IR 8545. HQC is **code-based** — different mathematical assumption (binary linear codes) from MLWE. It's the institutional hedge against an algorithmic break in lattices. HQC keys/ciphertexts are larger than ML-KEM. My design freeze pre-dates this, and **I do not implement HQC** — that's an algorithm-agility gap; future work should support hybrid lattice + code KEM. The recent CVEs against liboqs HQC (CVE-2025-48946, 52473) are HQC-only and patched in 0.14.0; they do not affect my ML-KEM/ML-DSA build.

**Citations:**
- [NIST IR 8545](https://nvlpubs.nist.gov/nistpubs/ir/2025/NIST.IR.8545.pdf)
- [NIST HQC announcement March 2025](https://www.nist.gov/news-events/news/2025/03/nist-pqc-standardization-process-hqc-announced-4th-round-selection)

---

## Cluster 13 — FIPS 203 internals (ML-KEM-768)

### Q2.5 — ML-KEM-768 KeyGen, Encaps, Decaps

**Answer:**
- **KeyGen** (FIPS 203 §7.1, Algorithm 16): sample seed ρ, derive matrix `A ∈ R_q^{k×k}` from ρ; sample short secret `s` and noise `e`; compute `t = A·s + e`. Public key `ek = (A, t)` (or just (ρ, t) since A is regenerable from ρ). Secret key `dk = s` (with extras for FO transform).
- **Encaps** (§7.2, Algorithm 17): given `ek`, sample message `m`; derive `(K, r) = G(m ‖ H(ek))`; compute ciphertext `c = K-PKE.Encrypt(ek, m, r)`; the shared secret is `K`.
- **Decaps** (§7.3, Algorithm 18): given `dk, c`: recover `m' = K-PKE.Decrypt(dk, c)`; recompute `(K', r') = G(m' ‖ H(ek))`; re-encrypt `c'' = K-PKE.Encrypt(ek, m', r')`; if `c'' == c` output K', else output implicit-rejection pseudorandom K-bar.

The K-PKE inner scheme is **only IND-CPA**; the FO transform with implicit rejection lifts it to **IND-CCA2**. ML-KEM-768 parameters: q=3329, n=256, k=3 (FIPS 203 §8 Table 2), pk 1,184 B, ct 1,088 B, ss 32 B.

**Citations:**
- [FIPS 203 §7-8](https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.203.pdf)
- [Kyber paper IACR 2017/634](https://eprint.iacr.org/2017/634.pdf)

### Q2.6 — IND-CCA ≠ authenticated key exchange

**Answer:** **Critical distinction.** IND-CCA2 says: an adversary with a decapsulation oracle cannot distinguish a real shared secret from random for a non-trivial challenge ciphertext. It assumes the public key `ek` is **known and trusted** by the receiver. IND-CCA gives the KEM confidentiality + integrity; **it does NOT bind the public key to an identity**. If an active MitM substitutes their own `ek` during transmission, both peers complete the KEM with the attacker, producing a shared secret the attacker knows — exactly my acknowledged handshake-auth limitation. Authenticated key exchange (AKE, e.g. SIGMA, NIST SP 800-56C Rev 2 §5) requires identity binding via signature or PSK over the transcript.

**Citations:**
- [FIPS 203 §3.3](https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.203.pdf)
- [NIST SP 800-227 §4](https://nvlpubs.nist.gov/nistpubs/SpecialPublications/NIST.SP.800-227.pdf)

---

## Cluster 14 — FIPS 204 internals (ML-DSA-65)

### Q2.7 — Fiat-Shamir with Aborts: explain the abort

**Answer:** Fiat-Shamir-with-Aborts (Lyubashevsky 2009) makes the signature distribution **independent of the secret key** by rejection-sampling. The signer commits to `w = A·y` for short `y`, derives challenge `c = H(M, w)`, computes `z = y + c·s1`. **Abort if** `‖z‖∞ ≥ γ1 - β` or `‖LowBits(w - c·s2)‖∞ ≥ γ2 - β` (FIPS 204 Algorithm 7) — otherwise `z` would leak `s1, s2` after enough observations. Average abort rate for ML-DSA-65 is ~4-7 iterations per signature. Skipping the abort enables **direct key-recovery attacks**. Side-channel implication: timing variation across abort iterations is itself a leak channel — must be masked or randomised.

**Citations:**
- [FIPS 204 §5](https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.204.pdf)
- [Dilithium paper IACR 2017/633 §3](https://eprint.iacr.org/2017/633.pdf)

### Q2.8 — Why is the ML-DSA-65 signature exactly 3,309 bytes?

**Answer:** Per FIPS 204 §4 Table 2, ML-DSA-65 signature components:
- `c̃` (commitment hash): 48 B (λ=192-bit security tag)
- `z` (response, k_z = 5 polynomials packed at γ1 = 2^19 bits each): ~2,560 B
- `h` (hint vector encoding sparse high-bit pattern, packed with ω + k bytes): ~83 B

Total = 3,309 B (FIPS 204 specifies the exact encoding). **Custom compression voids FIPS conformance** and breaks verifier interop — and there isn't much to compress without crossing into a different parameter set.

**Citations:**
- [FIPS 204 §4 Table 2](https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.204.pdf)

### Q2.9 — EUF-CMA reduction to MLWE + MSIS

**Answer:** Dilithium's EUF-CMA proof in the (Q)ROM (Dilithium §6, Kiltz-Lyubashevsky-Schaffner 2017/916):
- **MLWE** assumption: `(A, t = A·s1 + s2)` is computationally indistinguishable from `(A, u)` for uniform `u`. Used to argue that the public key reveals nothing about `s1, s2`.
- **MSIS** assumption: finding short non-zero `(z, h)` such that `A·z + h = 0` is hard. A signature forgery yields exactly such a short solution — hence the reduction.
- The reduction is in the ROM (programmable random oracle); the QROM version replaces ROM with an oracle responding consistently in superposition.

If randomness `r` has only 64 bits of entropy: `r` mixes into the challenge; collisions on `r` across signatures leak `s1, s2`, and ~2^32 signatures suffice for key recovery. Fix: require ≥256 bits per FIPS 204 §3.7.

**Citations:**
- [Dilithium IACR 2017/633 §6](https://eprint.iacr.org/2017/633.pdf)
- [QROM Fiat-Shamir IACR 2017/916](https://eprint.iacr.org/2017/916)

---

## Cluster 15 — Why security level 3?

### Q2.10 — Why not level 5 for a 20-year vehicle lifetime?

**Answer:** I picked **NIST Category 3** (≈ AES-192 classical) for ML-KEM-768 / ML-DSA-65. Mosca's framework: if `X (migration time) + Y (data lifetime) > Z (CRQC arrival)`, choose higher category. For a 2026 vehicle with 15-20 yr life, `X+Y ≈ 22 yr`. Z is uncertain — Gidney-Ekerå 2019 estimated 20M physical qubits to break RSA-2048; the 2025 update reduced this to <1M. Practical CRQC arrival is still likely 10-20 yr away. **Category 3 covers the realistic threat horizon**; Category 5 (≈ AES-256) gives more margin at the cost of ML-DSA-87's 4,627-byte signature (vs 3,309 B). For long-term root keys (OEM signing), I'd argue for level 5; for per-PDU signing where bandwidth matters, level 3 is the right balance. **Cryptographic agility — the ability to OTA-replace the algorithm — is more important than the parameter choice today.**

**Citations:**
- [NIST IR 8547 ipd §3, §4](https://nvlpubs.nist.gov/nistpubs/ir/2024/NIST.IR.8547.ipd.pdf)
- [NIST PQC Security Categories](https://csrc.nist.gov/projects/post-quantum-cryptography/post-quantum-cryptography-standardization/evaluation-criteria/security-(evaluation-criteria))
- [Gidney & Ekerå arXiv:1905.09749](https://arxiv.org/abs/1905.09749)

### Q2.11 — Why not level 1?

**Answer:** Level 1 (≈ AES-128, ~64-bit post-quantum margin under Grover) is too aggressive for long-lifetime automotive. ML-DSA-44 signatures are 2,420 B (vs 3,309) and signing is faster, but the security headroom is too thin for a vehicle on the road in 2046. For ephemeral session keys it's borderline acceptable; for long-lived signing keys it's not.

**Citations:**
- [FIPS 204 §4](https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.204.pdf)
- [NIST IR 8547 §3](https://nvlpubs.nist.gov/nistpubs/ir/2024/NIST.IR.8547.ipd.pdf)

---

## Cluster 16 — Side channels & implementation security

### Q2.12 — Constant-time on Cortex-A72: did you measure?

**Answer:** **No, I have not measured.** OQS FAQ explicitly flags liboqs as not for production. ML-DSA's rejection-sampling loop's iteration count leaks unless masked. mlkem-native (in liboqs 0.15.0 final) has formal constant-time proofs; ML-DSA in liboqs does not. My pinned `0.15.0-rc1` uses the older path. A real measurement would use `dudect` or TVLA on the Pi 4 binary. I would not deploy this code in a vehicle exposed to power/EM analysis without an HSM.

**Citations:**
- [Open Quantum Safe FAQ](https://openquantumsafe.org/faq.html)
- [liboqs ML-DSA page](https://openquantumsafe.org/liboqs/algorithms/sig/ml-dsa.html)
- [liboqs 0.15.0 release](https://github.com/open-quantum-safe/liboqs/releases/tag/0.15.0)

### Q2.13 — Power analysis on Pi 4

**Answer:** Recent academic CPA attacks on hardware ML-DSA recover the secret key from a few hundred traces (ePrint 2025/009; ePrint 2025/582 attacking the rejection step). The Pi 4 is a **near-physical-adversary platform** — exposed GPIO, no shielding, easy to instrument. liboqs has **no masking countermeasures** for ML-DSA. My threat model excludes side channels (§3.6), which I disclose. The production answer is HSM-backed signing, where the key never leaves the secure die.

**Citations:**
- [CPA Attack on ML-DSA HW IACR 2025/009](https://eprint.iacr.org/2025/009.pdf)
- [SCA via rejected sigs IACR 2025/582](https://eprint.iacr.org/2025/582)

### Q2.14 — Why not Falcon (FN-DSA)?

**Answer:** Falcon would give **smaller signatures** (~666 B for level 1, ~1,280 B for level 5). But Falcon uses **IEEE-754 doubles in FFT-based Gaussian sampling**, which has had multiple side-channel issues (Karabulut-Aysu DAC'21 "Falcon Down", ePrint 2021/772; ePrint 2023/224). Constant-time floating-point is not portable; integer emulation is slow. ML-DSA uses integer arithmetic mod q=8,380,417 — easier to mask, easier to implement consistently. **Falcon is faster + smaller; ML-DSA is easier to implement securely.** For a thesis prototype, ML-DSA is the safer choice.

**Citations:**
- [Falcon Down IACR 2021/772](https://eprint.iacr.org/2021/772)
- [Falcon SCA IACR 2023/224](https://eprint.iacr.org/2023/224.pdf)

---

## Cluster 17 — Randomness sources

### Q2.15 — Where does entropy come from on Pi 4?

**Answer:** liboqs's default `OQS_randombytes_system` calls `getrandom(2)` on Linux, which reads from `/dev/urandom`. The Linux CSPRNG is **ChaCha20-based since kernel 4.8**. Entropy seed: hardware events + the BCM2711's HWRNG. **The Linux CSPRNG is not CMVP/SP 800-90A validated by default** — for FIPS-validated RBG you need either RHEL FIPS mode or an external TRNG. My build does not claim FIPS-RBG conformance; that's a gap for production. FIPS 203 §3.3 requires SP 800-90A/B/C-compliant RBG, which I do not formally satisfy.

**Citations:**
- [FIPS 203 §3.3](https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.203.pdf)
- [NIST SP 800-90A Rev 1](https://nvlpubs.nist.gov/nistpubs/specialpublications/nist.sp.800-90ar1.pdf)
- [Linux getrandom(2)](https://man7.org/linux/man-pages/man2/getrandom.2.html)

### Q2.16 — Randomized vs deterministic ML-DSA

**Answer:** I use **hedged (randomized)**, the FIPS 204 §3.7 default, because deterministic ML-DSA (rnd = 0^256) is broken under **differential fault attacks** — flipping a bit in `y` produces two signatures over the same nonce, allowing key recovery (ePrint 2025/904). Hedged adds 256 bits of fresh entropy mixed into the commitment, retaining the robustness against fault injection. Cars are exactly the platform where clock/voltage glitching is realistic — hedged is the right choice.

**Citations:**
- [FIPS 204 §3.7](https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.204.pdf)
- [Fault security of ML-DSA IACR 2025/904](https://eprint.iacr.org/2025/904.pdf)

---

## Cluster 18 — Hybrid mode terminology

### Q2.17 — Your "Hybrid" vs IETF/NIST "hybrid KEM"

**Answer:** **Naming collision, acknowledged.** My "Hybrid Mode" = HMAC + ML-DSA simultaneous authentication (defence in depth against PQ-flaw + against single-algorithm bug). The **IETF/NIST "hybrid"** = a KEM combiner like X25519 + ML-KEM-768 with a concatenation KDF (draft-ietf-tls-hybrid-design, draft-ietf-tls-ecdhe-mlkem) — a *KEM* combiner, not an authentication combiner. My naming should be **"dual-authentication mode"** or **"MAC-and-Sign"**. I did not implement a real hybrid KEM because (a) AUTOSAR Csm has no combiner primitive, (b) ML-KEM alone is the NIST-recommended standalone for migration, (c) scope.

**Citations:**
- [draft-ietf-tls-hybrid-design](https://datatracker.ietf.org/doc/html/draft-ietf-tls-hybrid-design)
- [draft-ietf-tls-ecdhe-mlkem](https://datatracker.ietf.org/doc/draft-ietf-tls-ecdhe-mlkem/)
- [NIST SP 800-56C Rev 2 §2](https://nvlpubs.nist.gov/nistpubs/SpecialPublications/NIST.SP.800-56Cr2.pdf)

---

## Cluster 19 — Unauthenticated KEM handshake (THE hardest question)

### Q2.18 — Why ship an admitted MitM-vulnerable handshake?

**Answer:** **Acknowledged limitation, disclosed in §3.6.4.** The trade-off: a complete authenticated KEM requires pre-provisioned long-term identity keys at both peers, which is an ECU-manufacturing concern — out of scope for a single-Pi research demonstrator. An active network adversary that can inject correctly-formatted control frames during initial session establishment can substitute their own ML-KEM public key and become MitM for the entire authenticated session. **Once a clean handshake establishes, all in-session per-PDU integrity is quantum-resistant and complete (47/47 replays rejected, 100% MitM in-session detection).** The bound is: *conditioned on a clean handshake, in-session integrity is sound; an active MitM during the handshake itself is currently undetected.* The fix is in §5 Future Work: bind a long-term ML-DSA identity key into the handshake (sigma-style), with identity keys provisioned out-of-band during ECU manufacturing.

**Citations:**
- [Krawczyk SIGMA paper](https://webee.technion.ac.il/~hugo/sigma-pdf.pdf)
- [NIST SP 800-56C Rev 2 §5](https://nvlpubs.nist.gov/nistpubs/SpecialPublications/NIST.SP.800-56Cr2.pdf)

### Q2.19 — Why not just sign the KEM transcript?

**Answer:** **Yes, that's exactly the right fix.** Sign `(ek_A, ct_B, transcript_hash)` with each peer's static long-term ML-DSA identity key, verify before deriving the session key. Cost: 1 sign + 1 verify per ~1-hour rekey ≈ 1.6 ms — negligible. Identity-key distribution: PKI rooted at the OEM, public-key hash burnt into ECU at flash time. **The thesis stops one engineering step short of this — that step is exactly what I have queued in §5 Future Work.** No new primitive is required: ML-DSA-65 is already in the system.

**Citations:**
- [Krawczyk SIGMA §5](https://webee.technion.ac.il/~hugo/sigma-pdf.pdf)
- [NIST SP 800-227](https://nvlpubs.nist.gov/nistpubs/SpecialPublications/NIST.SP.800-227.pdf)

---

## Cluster 20 — Forward secrecy claim

### Q2.20 — Define FS and identify your long-term key

**Answer:** **Forward secrecy:** compromise of long-term key material today does NOT decrypt past recorded sessions. **My long-term key:** the ML-DSA-65 identity key at `/etc/secoc/keys/mldsa_secoc.key`. **Per-session ephemeral:** ML-KEM-768 keypairs generated each rekey. If `mldsa_secoc.key` is dumped today: future SecOC sessions are forgeable (signing impersonation), but **past session symmetric keys are not recoverable** because they were derived from ephemeral ML-KEM secrets that are not stored. **Caveat:** because the handshake is unauthenticated, the FS claim is **conditional on an honest handshake at recording time** — an active MitM at recording time learned the session key and the FS guarantee collapses.

**Citations:**
- [NIST SP 800-56C Rev 2 §4-§5](https://nvlpubs.nist.gov/nistpubs/SpecialPublications/NIST.SP.800-56Cr2.pdf)
- [Krawczyk SIGMA §3](https://webee.technion.ac.il/~hugo/sigma-pdf.pdf)

---

## Cluster 21 — Non-repudiation in automotive

### Q2.21 — Is non-repudiation actually wanted?

**Answer:** **Inside-vehicle BSW: yes** — non-repudiation is useful for the event-data recorder and post-incident attribution. **External (V2X): no** — IEEE 1609.2 mandates pseudonym certificates for privacy. ML-DSA signatures are inherently linkable; for V2X privacy, group/ring signatures or pseudonym rotation are required. **My gateway is internal-only; V2X-facing roles need a different design.** Citing "non-repudiation" as an unqualified gain over HMAC was sloppy — I should clarify it's a feature for in-vehicle BSW and a *bug* for outside-vehicle V2X.

**Citations:**
- [IEEE 1609.2-2022](https://standards.ieee.org/ieee/1609.2/10258/)
- [V2X Security & Privacy survey](https://ieeexplore.ieee.org/ielaam/8782711/8815895/9108394-aam.pdf)

---

## Cluster 22 — Why not SLH-DSA / FN-DSA / HQC

### Q2.22 — SLH-DSA for cold-start authentication?

**Answer:** **Excellent observation.** SLH-DSA-128s sign is slow but verify is fast, and there are no rejection-sampling side channels. For handshake-time auth (1/hour) the slow sign is acceptable. **Hybrid lattice + hash is exactly what NIST IR 8413/8547 recommend for diversification.** I did not implement this — out of scope. A v2 design: use SLH-DSA on the static identity key (rare, slow OK) and ML-DSA on the per-PDU path (fast verify).

**Citations:**
- [FIPS 205 §11](https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.205.pdf)
- [NIST IR 8547 §3](https://nvlpubs.nist.gov/nistpubs/ir/2024/NIST.IR.8547.ipd.pdf)

### Q2.23 — Stateful hash signatures for boot/firmware?

**Answer:** **LMS / XMSS** (NIST SP 800-208) are quantum-safe and have been NIST-approved since 2020. They're stateful — only safe if the signer state is **never reused** (problematic at scale, fine for low-rate firmware-signing where the OEM controls a single signer with auditable state). For per-PDU SecOC at thousands per second, stateful is impractical. For *firmware* signing — which is rare, slow, and tightly controlled — stateful hash sigs are excellent. Out of scope for this thesis but a clean future extension.

**Citations:**
- [NIST SP 800-208](https://nvlpubs.nist.gov/nistpubs/SpecialPublications/NIST.SP.800-208.pdf)

---

## Cluster 23 — liboqs maturity & reproducibility

### Q2.24 — Why pin to a release candidate?

**Answer:** I pinned `liboqs 0.15.0-rc1` because that was the latest tag at design freeze. The final 0.15.0 (14 Nov 2025) ships ML-KEM via mlkem-native v1.0.0 with formal proofs — stronger than my path. **Defence:** rc1 is feature-frozen and my benchmarks were obtained on it; production deployment must re-pin to 0.15.0 final and re-run the ISE evaluation. **Reproducibility:** I should additionally pin the commit SHA (currently only the tag) and vendor the source. This is documented as Future Work in §5.

**Citations:**
- [liboqs 0.15.0 release](https://github.com/open-quantum-safe/liboqs/releases/tag/0.15.0)
- [Open Quantum Safe FAQ](https://openquantumsafe.org/faq.html)

### Q2.25 — liboqs HQC CVEs in 2025

**Answer:** CVE-2025-48946 and CVE-2025-52473 affect the **HQC** implementation in liboqs only — patched in 0.14.0. My build does not enable HQC (`-DOQS_ENABLE_KEM_hqc_*=OFF` is the default since 0.13.0). I verified this by checking the `liboqs.so` symbol table (`nm -D liboqs.so | grep -i hqc` returns nothing). **Adjacent-CVE concern:** even with HQC disabled, shared utility code in the same library could harbour latent bugs. Pinning the commit SHA + vendoring the source is the supply-chain answer.

**Citations:**
- [CVE-2025-48946 advisory](https://github.com/open-quantum-safe/liboqs/security/advisories/GHSA-3rxw-4v8q-9gq5)
- [CVE-2025-52473 advisory](https://github.com/open-quantum-safe/liboqs/security/advisories/GHSA-qq3m-rq9v-jfgm)
- [PQCA liboqs 0.14.0](https://pqca.org/blog/2025/pqca-announces-release-of-liboqs-version-0-14-0-from-open-quantum-safe-project/)

---

## Cluster 24 — Quantum-attack realism

### Q2.26 — Qubits to break ECC-256 / RSA-2048

**Answer:**
- **Gidney-Ekerå 2019 (arXiv:1905.09749):** ~20M physical qubits with surface code, ~8h runtime, factoring 2,048-bit RSA. Logical qubit count ~4,000.
- **Gidney 2025 (arXiv:2505.15917):** <1M noisy physical qubits suffice — algorithmic improvements squeezed the resource estimate down 20×.
- **State of the art today (2025-2026):** ~1,000 physical qubits (IBM Condor, Google Willow); ~100 logical qubits with current error-correction overhead.

Practical CRQC for RSA-2048 / ECC-256 is **likely 5-15 yr away**, with significant uncertainty. NIST IR 8547's 2030 deprecation / 2035 disallow timeline embodies this risk. Mosca for a 2026 vehicle: `X (migration) + Y (data lifetime ~20 yr) > Z (5-15 yr) → migrate now.`

**Citations:**
- [Gidney-Ekerå arXiv:1905.09749](https://arxiv.org/abs/1905.09749)
- [Gidney 2025 arXiv:2505.15917](https://arxiv.org/abs/2505.15917)
- [NIST IR 8547 §4](https://nvlpubs.nist.gov/nistpubs/ir/2024/NIST.IR.8547.ipd.pdf)

---

## Cluster 25 — Throughput vs ADAS reality

### Q2.27 — 552 msg/s, survivable for ADAS / V2X?

**Answer:** 552 msg/s compute-bound on a single x86_64 stream is much less than a raw CAN bus aggregate (>10K msg/s on heavy buses). **Realistic deployment:**
1. **Sign only at trust boundaries** — gateway, V2X, OTA — not per-CAN-frame.
2. **Critical PDUs only** — brake commands, FOTA, diagnostic-write, key updates; routine telemetry stays HMAC.
3. **Hardware acceleration** (Infineon AURIX TC4Dx CSRM, future PQC accelerators) drops ML-DSA sign ~10×, lifting throughput to ~5,500 msg/s.
4. **Batched signing** — sign aggregate frames, not individual.

The 552 msg/s number is the *single-stream compute bound on a Pi 4 in software*; in real deployment with hardware accel and selective signing it's a non-issue.

**Citations:**
- [IEEE 1609.2-2022](https://standards.ieee.org/ieee/1609.2/10258/)
- [FIPS 204](https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.204.pdf)

---

## Cluster 26 — HKDF use

### Q2.28 — HKDF-SHA256 and domain separation

**Answer:** I use HKDF-SHA256 to derive symmetric session keys from the 32-byte ML-KEM shared secret. RFC 5869 §2: `PRK = HMAC-SHA256(salt, IKM)`; `OKM = Expand(PRK, info, L)`.
- **`salt`:** handshake transcript hash (binds the derivation to the specific session).
- **`info`:** `"secoc-v1 | <direction> | <epoch> | <peer-id>"` — domain-separated per direction (Tx/Rx) to avoid key reflection, per epoch, per peer.
- **`L`:** 32 B for downstream symmetric primitives (e.g., HMAC-SHA256 keys).

NIST SP 800-56C Rev 2 §5 specifies the `Z ‖ otherInfo` format guidance; I follow this pattern. **Without domain separation per direction, an attacker could replay a Tx frame as Rx — that's why direction is in `info`.**

**Citations:**
- [RFC 5869](https://datatracker.ietf.org/doc/html/rfc5869)
- [NIST SP 800-56C Rev 2 §5](https://nvlpubs.nist.gov/nistpubs/SpecialPublications/NIST.SP.800-56Cr2.pdf)
- [NIST SP 800-227](https://nvlpubs.nist.gov/nistpubs/SpecialPublications/NIST.SP.800-227.pdf)

---

# Part III — Examiner #3 (Compliance & E/E architecture)

> 26 questions across 8 clusters: ISO/SAE 21434 TARA & CAL, UN R155 Annex 5, UN R156 SUMS, HSM/TEE/SHE, ISO 26262 absence, E/E architecture & SDV trend, NIST IR 8547 timeline, threat model & key lifecycle, type approval.

---

## Cluster 27 — ISO/SAE 21434: TARA, CAL, depth

### Q3.1 — Where is the documented TARA?

**Answer:** I performed a **scoped TARA** rather than a full Clause 15 vehicle-level analysis. Assets identified: long-term ML-DSA private key, ML-KEM private key, freshness counter state, secure-boot keys (out of scope), Linux rootfs, in-vehicle CAN/Ethernet traffic. Per-asset SFOP impact ratings: long-term key compromise = High (Operational + Privacy). Threat scenarios → attack paths → AFR per Annex G. **What's missing for an audit-grade TARA:** independent reviewer, a full vehicle-level item definition, and traceability matrix linking each test case to a TARA threat ID. This is recorded in the limitations and future-work sections.

**Citations:**
- [ISO/SAE 21434:2021](https://www.iso.org/standard/70918.html)
- [TARA in Practice (UL SIS)](https://www.ul.com/sis/resources/threat-analyses-and-risk-assessment-in-practice-tara)

### Q3.2 — Attack Feasibility Rating method

**Answer:** I used the **attack-potential method** (ISO/IEC 18045 style, per Annex G option). Scoring: Elapsed Time, Specialist Expertise, Knowledge of TOE, Window of Opportunity, Equipment. For a Linux-on-Pi-4 gateway: Elapsed Time low (Linux exploitation is well-documented), Specialist Expertise medium, Knowledge low (open repo), Window medium (gateway always on), Equipment minimal. Net **High Attack Feasibility** for software-only attacks. CVSS would over-weight network exploitability for this configuration; attack-vector-only is too coarse. A Pi 4 in a lab is **not representative of attacker access in a deployed vehicle** — the latter is more constrained (no physical access while moving), suggesting a realistic AFR closer to medium for my exact threat model.

**Citations:**
- [Comparative Evaluation of AFR Methods](https://www.researchgate.net/publication/339390034_Comparative_Evaluation_of_Cybersecurity_Methods_for_Attack_Feasibility_Rating_per_ISOSAE_DIS_21434)
- [ISO/SAE 21434:2021 §8.6, §8.7](https://www.iso.org/standard/70918.html)

### Q3.3 — CAL vs ASIL: be precise

**Answer:**
- **ASIL** (ISO 26262): functional-safety integrity from HARA over Severity, Exposure, Controllability of E/E malfunctions. Levels: **QM, A, B, C, D**, plus decomposition like **ASIL B(D)**. There is **no ASIL D2**.
- **CAL** (ISO/SAE 21434): cybersecurity assurance level from impact + attack vector. Levels: **CAL 1-4**.
- They are **independent dimensions**: a function can be ASIL D + CAL 4, or ASIL QM + CAL 4, etc.

**I retract "ASIL D2"** — sloppy informal phrasing. The 5 ms target should be cited as "consistent with ASIL-D timing-relevant signals in literature," not as an ASIL allocation.

**Citations:**
- [Code Intelligence — CALs in ISO 21434](https://www.code-intelligence.com/blog/iso21434-cybersecurity-assurance-levels)
- [ISO 26262-9:2018](https://www.iso.org/standard/68391.html)

### Q3.4 — CAL-4 PARTIAL: what closes the gap?

**Answer:** CAL-4 cybersecurity validation per Clause 15 requires:
1. **Independent assessor** — TÜV SÜD, TÜV NORD, UL SIS, DNV, accredited lab — not the dev team.
2. **TARA-driven test plan** — every test case traceable to a TARA threat ID and asset.
3. **Vulnerability triage** — CVSS-rated findings with retest evidence.
4. **Residual-risk acceptance** signed by an authorised cybersecurity manager.
5. **Confirmation review** — independent functional safety / security manager review at CAL 3-4.

Cost/effort: typical Tier-1 program 6-12 weeks per ECU, low-six-figure euros for a complete engagement.

**Citations:**
- [TÜV SÜD ISO/SAE 21434](https://www.tuvsud.com/en-us/services/cyber-security/safety-components)
- [TÜV NORD ISO/SAE 21434](https://www.tuv-nord.com/us/en/services/cybersecurity/iso-sae-21434-cybersecurity-for-road-vehicles-csms/)
- [ISO/SAE 21434:2021 §15](https://www.iso.org/standard/70918.html)

### Q3.5 — Auditability of 22/23 PASS

**Answer:** A real audit reproduction needs a **traceability matrix**: Requirement ID → TARA threat → R155 M-ID → Test case → Evidence file. I provide CSV/markdown evidence in `compliance_reports/live_audit_20260501/`, but **not** in a Polarion/DOORS/Jama-style tool. The 22/23 figure is **reproducible from the artefacts in the repo** (I commit the audit JSON outputs), but not yet in an industry-standard tool format. A type-approval audit would require this upgrade.

**Citations:**
- [ISO/SAE 21434:2021 Clause 6](https://www.iso.org/standard/70918.html)

---

## Cluster 28 — UN R155 Annex 5

### Q3.6 — Defend the M-ID mapping for cryptographic-key storage

**Answer:** **Acknowledged uncertainty.** Annex 5 has been amended several times and the M-ID labelling has been revised. The thesis adopts the common automotive-security interpretation that key-storage hygiene maps onto the broader Annex 5 mitigation family covering "unauthorised access to in-vehicle code, data and credentials" (Part B threats in tables B1). The exact M-ID applicable to **private-key storage** in the current consolidated R155 PDF should be re-verified; I tagged this caveat in §3.6 footnote and recommend re-verification before camera-ready submission. **Annex 5 lists "examples of mitigations"; authoritative coverage requires the OEM's own threat model and TARA**, mapping each threat onto a chosen mitigation and an evidence artefact.

**Citations:**
- [UN R155 consolidated](https://unece.org/sites/default/files/2023-02/R155e%20(2).pdf)
- [UN R155 landing page](https://unece.org/transport/documents/2021/03/standards/un-regulation-no-155-cyber-security-and-cyber-security)

### Q3.7 — Filesystem keys: Type Approval Authority's view

**Answer:** A TAA looks at evidence that the OEM's TARA addressed "key extraction by an attacker with root or local access." `/etc/secoc/keys/*.key` with POSIX 0600 inside 0700 directory:
- **Defeated by:** root compromise, kernel exploit, physical SD-card extraction, JTAG, Cold Boot.
- **Production answer:** AURIX HSM (EVITA Full), TPM 2.0 sealed/encrypted Linux key types, ARM TrustZone OP-TEE.

The Pi 4 has **no HSM** — this is a research demonstrator. I disclose this as M21 PARTIAL. A Tier-1 production submission would not pass without hardware-rooted key isolation.

**Citations:**
- [UN R155 Annex 5](https://unece.org/sites/default/files/2023-02/R155e%20(2).pdf)
- [Infineon AURIX TC3xx HSM](https://www.infineon.com/dgdl/Infineon-AURIX_TC3xx_Hardware_Security_Module_Quick-Training-v01_00-EN.pdf?fileId=5546d46274cf54d50174da4ebc3f2265)
- [GlobalPlatform TEE](https://globalplatform.org/specs-library/tee-system-architecture/)

### Q3.8 — HSM vs TEE vs SHE

**Answer:**
- **SHE (Secure Hardware Extension):** HIS-consortium spec, AES-128 only, on-chip key store, CMAC + Miyaguchi-Preneel hash; **no asymmetric**. Suitable for body ECUs.
- **HSM** (e.g., EVITA Full / AURIX TC3xx HSM): full crypto core, ECC/RSA/AES, TRNG, secure-boot anchor; suitable for **gateway/V2X/critical** ECUs.
- **TEE** (ARM TrustZone + OP-TEE per GlobalPlatform): software isolation in a separate execution environment on the same SoC; suitable for **SDV / HPC / IVI**.

Annex 5 does not mandate technology — it mandates the protection. OEM TARA picks which: body ECU → SHE; gateway/V2X → HSM; SDV/HPC → TEE+HSM hybrid.

**Citations:**
- [EVITA HSM](https://evita-project.org/Publications/WG11.pdf)
- [SWS Crypto Driver R23-11](https://www.autosar.org/fileadmin/standards/R23-11/CP/AUTOSAR_CP_SWS_CryptoDriver.pdf)
- [GlobalPlatform TEE](https://globalplatform.org/specs-library/tee-system-architecture/)
- [Infineon AURIX TC3xx HSM](https://www.infineon.com/dgdl/Infineon-AURIX_TC3xx_Hardware_Security_Module_Quick-Training-v01_00-EN.pdf?fileId=5546d46274cf54d50174da4ebc3f2265)

### Q3.9 — What R155 mitigations did you NOT cover?

**Answer:** **Explicit scope:** in-session communication on the Ethernet leg between the Pi gateway and a peer ECU. **Out of scope (legitimate):**
- Backend / CSMS controls (Part A) — process-side, not technical.
- OTA process — that's R156 territory, separate.
- Physical manipulation — Part B threats around tamper, side channel.
- Supply chain integrity — beyond a single ECU's purview.
- Decommissioning / end-of-life — fleet-management concern.

I provide explicit M-ID → test-case mapping with PARTIALs flagged. I distinguish "tested PASS" from "argued PASS by design."

**Citations:**
- [UN R155 Annex 5](https://unece.org/sites/default/files/2023-02/R155e%20(2).pdf)

---

## Cluster 29 — UN R156: Software updates & SUMS

### Q3.10 — Where does R156 fit?

**Answer:** **Type approval requires both R155 and R156 since 2022 in the EU.** R156 mandates **SUMS** (Software Update Management System) — process — plus per-vehicle technical capability:
- Secure transfer (signed update bundles)
- Integrity verification at the vehicle
- Version logs
- Rollback on integrity failure
- **RXSWIN** — regulatory software identification number for type-approval-relevant software.

**My thesis is silent on R156** — I do not implement OTA. A minimum compatible support sketch:
- Signed update bundles using ML-DSA over a manifest.
- Dual-bank A/B partitions with atomic switch.
- Manifest + signature verified at boot.
- Rollback on integrity failure or boot failure.

Long-term key rotation: a key-ladder where an HSM-resident root key wraps the long-term ML-DSA signing key, allowing rotation without re-flashing the secure element.

**Citations:**
- [UN R156 consolidated](https://unece.org/sites/default/files/2024-03/R156e%20(2).pdf)
- [UN R156 landing page](https://unece.org/transport/documents/2021/03/standards/un-regulation-no-156-software-update-and-software-update)

### Q3.11 — NIST SP 800-193 firmware resiliency

**Answer:** SP 800-193 specifies **Protect / Detect / Recover** for firmware. **Stock Pi 4 satisfies none of the three:** the boot ROM verifies the EEPROM bootloader (post-2020 default-on), but everything downstream (start.elf, kernel, rootfs) is unsigned by default. A production gateway needs:
- ARM Trusted Firmware-A as the verified-boot anchor.
- Signed FIT image for kernel + initramfs.
- dm-verity rootfs.
- Measured boot to TPM/HSM.
- Update verification via signed manifest before atomic switch.

I concede the Pi 4 is **unsuitable as a production secure-boot anchor** — this is one of the explicit limitations of using COTS hardware for a research demonstrator.

**Citations:**
- [NIST SP 800-193](https://csrc.nist.gov/pubs/sp/800/193/final)

---

## Cluster 30 — ISO 26262 absence

### Q3.12 — How can you skip ISO 26262 entirely?

**Answer:** I **cannot fully skip it** — ISO 26262-2 §5.4.2 mandates considering **safety/security interactions**. My gateway forwards CAN frames that may carry ASIL-rated signals; if PQC verification adds 370 µs latency, I have potentially affected functional safety. **The thesis explicitly excludes a full HARA** and ASIL classification (§1.4) — that's a scope choice, not a denial of the interaction. Minimum that should have been done: identify whether any forwarded signal is safety-relevant, document the latency-vs-safety interaction, allocate a safety-integrity level if any forwarded signal is ASIL-rated. The 5 ms latency budget I cite is **representative of chassis control loop targets in literature** — not an allocated ASIL D budget. **I retract "ASIL D2"** — there is no such level; sloppy informal phrasing.

**Citations:**
- [ISO 26262-1:2018](https://www.iso.org/standard/68383.html)
- [ISO 26262-9:2018](https://www.iso.org/standard/68391.html)

### Q3.13 — Sketch a HARA

**Answer:** Whiteboard HARA for the gateway-as-forwarder:
- **Hazard:** gateway forwards stale/corrupted brake-related signal (because PQC verify dropped or delayed).
- **Operational situation:** vehicle in motion, driver demanding brake.
- **S** (Severity): S3 (life-threatening / fatal).
- **E** (Exposure): E4 (highly probable — every braking event).
- **C** (Controllability): C3 (difficult to avoid; driver cannot recover from bad brake signal).
- **Resulting ASIL:** D (per ISO 26262-3 Table 4).
- **Safety goal:** gateway shall fail-safe (forward unauthenticated, log, degrade) within X ms when PQC verify fails or times out.

The security mitigation must not introduce a safety regression — argues for a deterministic-time fallback path (e.g., on PQC timeout, forward with stronger logging rather than drop).

**Citations:**
- [ISO 26262-1:2018](https://www.iso.org/standard/68383.html)
- [ISO 26262 series](https://www.iso.org/publication/PUB200262.html)

---

## Cluster 31 — E/E architecture and SDV trend

### Q3.14 — Centralized gateway vs zonal HPC: 2027+ relevance?

**Answer:** Trend is real: domain → cross-domain → zonal + central HPC. Tesla started in 2017; VW E3 1.2 and BMW iDrive 8 are commercial deployments. **My Pi 4 is a stand-in for a Linux-class zonal/gateway ECU**, not an MCU. SecOC + PQC primitives still apply at zonal-to-HPC link or HPC-to-HPC link. The integration pattern (Csm/CryIf seam) is layer-agnostic. **Limitation:** zonal *MCU* ECUs (Cortex-R52 lockstep) need PQC-aware HSM at the zone — exactly the AURIX TC4Dx CSRM and Renesas RH850/U2A roadmap. My demonstrator informs the **HPC side** of that pairing.

**Citations:**
- [Bosch E/E architecture](https://www.bosch-mobility.com/en/mobility-topics/ee-architecture/)
- [Bosch zonal whitepaper](https://www.bosch-mobility.com/media/global/mobility-topics/connected-mobility/iot-device-on-wheels/e-e-architecture/whitepaper/230831-bosch-xc-whitepaper-ee-architektur-en.pdf)

### Q3.15 — SOME/IP-SD over TLS vs SecOC: future?

**Answer:** Both will coexist 10+ years. **SOME/IP-SD over TLS** is the AP/SOA answer for service-oriented communication (typically with TLS 1.3, future hybrid PQC ciphersuites). **SecOC** operates at the **PDU layer**, preserving authentication across CAN ↔ Ethernet traversal in the gateway — a different abstraction. AUTOSAR Classic + Ethernet PDUs over UDP often skip TLS to keep the PDU model intact. Adaptive uses TLS for SOME/IP, but interop with Classic ECUs (the majority of the install base for the next 10 yr) keeps SecOC alive at the gateway boundary. **R25-11 still maintains and updates SecOC.**

**Citations:**
- [SOME/IP Protocol R24-11](https://www.autosar.org/fileadmin/standards/R24-11/FO/AUTOSAR_FO_PRS_SOMEIPProtocol.pdf)
- [SecOC Protocol R23-11](https://www.autosar.org/fileadmin/standards/R23-11/FO/AUTOSAR_FO_PRS_SecOcProtocol.pdf)
- [AUTOSAR Adaptive](https://www.autosar.org/standards/adaptive-platform)
- [AUTOSAR R25-11 release](https://www.autosar.org/news-events/detail/release-r25-11-is-now-available)

### Q3.16 — Pi 4 Cortex-A72 vs production Cortex-R52

**Answer:**
- **Cortex-A72** (Pi 4): out-of-order superscalar ARMv8-A with NEON SIMD; SIMD helps lattice multiplications.
- **Cortex-R52** (typical automotive gateway MCU): in-order, lockstep-capable (split-lock for ASIL-D); no NEON in lockstep mode; smaller caches; lower clock.

**Realistic projection:** ML-DSA-65 sign on R52 is likely 5-10× slower than my Pi 4 numbers; verify 3-5× slower. **Mitigation:**
1. Hardware acceleration (AURIX HSM, dedicated PQC accelerator on TC4Dx).
2. Offload to HPC with Cortex-A class running TEE.
3. Selective signing (critical PDUs only).

My Pi 4 numbers are an **upper bound on representativeness**, not a target ECU benchmark — that limitation is acknowledged in §1.4.

**Citations:**
- [Arm Cortex-A comparison](https://www.arm.com/-/media/Arm%20Developer%20Community/PDF/Cortex-A%20R%20M%20datasheets/Arm%20Cortex-A%20Comparison%20Table_v4.pdf)
- [Arm Cortex-R comparison](https://www.arm.com/-/media/Arm%20Developer%20Community/PDF/Cortex-A%20R%20M%20datasheets/Arm%20Cortex-R%20Comparison%20Table.pdf)

---

## Cluster 32 — NIST IR 8547 timeline & migration realism

### Q3.17 — Map IR 8547 to a 2027 SOP vehicle

**Answer:**
- **NIST IR 8547 (ipd, Nov 2024):** ECDSA P-256, RSA-2048, RSA-3072 deprecated by **2030**; disallowed for protection by **2035**. AES-128 and SHA-256 remain acceptable.
- **NSM-10 (May 2022):** US federal systems must be migrated by 2035.

For a vehicle with **SOP 2027** and **15-yr service life (on the road until 2042)**:
- 2027 SOP: ship **hybrid (classical + PQC)** so it interoperates with existing PKI.
- 2030: PKI must transition off RSA-2048 / ECDSA P-256 — requires **OTA-driven key rotation**.
- 2035: pure PQC; last classical certificate must expire or be re-issued PQC.
- 2042 EOL: continue receiving PQC-signed updates; backend still has ML-DSA private keys.

**This is exactly why HSM-backed key rotation matters** — files in `/etc/secoc/keys/` cannot survive 15 years of OTA migrations.

**Citations:**
- [NIST IR 8547 ipd](https://csrc.nist.gov/pubs/ir/8547/ipd)
- [NSM-10](https://bidenwhitehouse.archives.gov/briefing-room/statements-releases/2022/05/04/national-security-memorandum-on-promoting-united-states-leadership-in-quantum-computing-while-mitigating-risks-to-vulnerable-cryptographic-systems/)

### Q3.18 — HNDL: vehicle threat or slogan?

**Answer:** **Important nuance.** HNDL targets **confidentiality** — recorded ciphertext decrypted later. **SecOC is signing/MAC, not encryption** — HNDL does not directly threaten SecOC integrity. **Real HNDL targets in a vehicle:**
- V2X pseudonym certificates and ECDH ephemerals.
- Telematics TLS sessions (PII, location, behavioural data).
- OTA package encryption keys.
- ADAS sensor data with privacy implications.

**The right argument for migrating SecOC to PQC** is **not** HNDL but: *a CRQC can forge signatures on an old long-term key, so signature schemes must be PQC before CRQC arrives.* I should reframe the HNDL motivation in §1.1 to be precise about which threat applies to *which* primitive: confidentiality/HNDL → encryption migration; signature forgery → signing migration.

**Citations:**
- [ENISA Post-Quantum Cryptography](https://www.enisa.europa.eu/publications/post-quantum-cryptography-current-state-and-quantum-mitigation)
- [CISA/NSA/NIST Quantum-Readiness Factsheet](https://www.cisa.gov/sites/default/files/2023-08/Quantum%20Readiness_Final_CLEAR_508c%20(3).pdf)

---

## Cluster 33 — Threat model, key lifecycle, pen testing

### Q3.19 — Excluding physical attacks: defensible?

**Answer:** **Acknowledge — research-scope, not real-vehicle posture.** R155 Annex 5 Part B explicitly addresses physical manipulation, so claiming 22/23 if physical was out of scope **is misleading**. Concrete physical attacks on a Pi 4: SD-card extraction, JTAG/SWD on GPIO, UART console, OBD-II tester pivot. Production answer: HSM + sealed enclosure + tamper response. I should restate the 22/23 figure with the explicit scope: "22/23 of the in-session-communication mitigations PASS"; physical-manipulation mitigations are out of scope.

**Citations:**
- [UN R155 Annex 5](https://unece.org/sites/default/files/2023-02/R155e%20(2).pdf)

### Q3.20 — Side-channel attacks on ML-KEM/ML-DSA

**Answer:** Acknowledge published timing/power side-channel literature on rejection sampling, NTT, and the abort loop. **liboqs has constant-time variants but defaults vary**; Pi 4 has no isolated execution environment. My private keys are **not actually safe** from a co-located attacker with power-analysis tooling. Production answer: hardened HSM with masked implementations (e.g., Infineon AURIX HSM running PQC primitives with shielded execution). I disclose side channels as out of scope (§3.6).

**Citations:**
- [ENISA PQC Integration Study](https://www.enisa.europa.eu/publications/post-quantum-cryptography-integration-study)
- [FIPS 204](https://csrc.nist.gov/pubs/fips/204/final)

### Q3.21 — ML-KEM handshake unauthenticated: critical limitation

**Answer:** **Yes, this is the most important production gap.** An active MitM substitutes their own ML-KEM public key, completes the KEM with both peers, and learns the session key. Downstream SecOC then runs over a known key. **Production answer:** certificate chain rooted at the OEM CA, ML-DSA-signed; gateway verifies the peer's static ML-KEM public key via the certificate before encapsulation. Without this, the chain reduces to "trusted network" — unacceptable in a vehicle. The fix is one ML-DSA verify per rekey, ~150 µs, negligible. I queue this as Future Work in §5; it is not a thesis-blocker but it is a thesis-conclusion.

**Citations:**
- [FIPS 203](https://csrc.nist.gov/pubs/fips/203/final)
- [FIPS 204](https://csrc.nist.gov/pubs/fips/204/final)

### Q3.22 — Long-term key lifecycle (NIST SP 800-57)

**Answer:** Walking through SP 800-57 Pt 1 Rev 5 phases for the long-term ML-DSA identity key:
- **Generation:** factory, in HSM, never extractable.
- **Distribution:** never — public key signed into device certificate at OEM provisioning.
- **Storage:** HSM-backed, wrapped if mirrored (e.g., for redundancy).
- **Use:** signing only, rate-limited (anti-DoS), audit-logged.
- **Cryptoperiod:** SP 800-57 typically 1-3 years for signing keys; for an automotive ECU bound to hardware, **vehicle EOL or known compromise** is typical.
- **Revocation:** CRL-equivalent distributed via R156 OTA.
- **Destruction:** zeroization on decommission per NIST SP 800-88.

**My thesis covers session keys (1 h rekey)** — the **long-term identity-key lifecycle is open**, and I queue it for future work alongside the HSM integration.

**Citations:**
- [NIST SP 800-57 Pt1 Rev 5](https://csrc.nist.gov/pubs/sp/800/57/pt1/r5/final)

### Q3.23 — Penetration testing: real engagement

**Answer:** A CAL-3/4-depth gateway pen test:
- **Scope:** TARA-driven; every threat scenario gets at least one test case.
- **Methodology:** OWASP IoT Security Testing Guide (ISTG) framework + ISO/SAE 21434 §15 cybersecurity validation.
- **Tooling:** CANalyzer/CANoe, Wireshark, scapy, Burp on Ethernet, JTAG/UART probes, glitching for HSM.
- **Deliverables:** test report, CVSS-rated findings, retest evidence, residual-risk acceptance.
- **Independence:** tester not part of dev team; ideally external lab.

**ISE replaces:** regression testing for known scenarios at scale. **ISE does NOT replace:** discovery of unknown vulnerabilities, fuzzing of proprietary protocols, hardware probing, social-engineering paths.

**Citations:**
- [OWASP IoT Security Testing Guide](https://owasp.org/owasp-istg/)
- [ISO/SAE 21434:2021 §15](https://www.iso.org/standard/70918.html)

---

## Cluster 34 — Type approval & regulation

### Q3.24 — R155 to type approval

**Answer:** EU type-approval chain:
1. **1958 Agreement → WP.29 → GRVA** drafts regulations → contracting parties type-approve.
2. **R155 dual approval:** CSMS approval (process-side certificate from a TAA) + Vehicle Type Approval (product-side, per vehicle type).
3. **Since July 2024 in EU:** all new vehicles sold need R155/R156 compliance.
4. **Tier-1** owns cybersecurity-relevant evidence for the components they ship (gateway, ECUs); OEM aggregates into VTA submission.

**My thesis sits at the Tier-1 / research demonstrator level** — not a type-approval submission. The evidence package (TARA, test cases, residual risk, traceability) is **not yet in submission-ready form**.

**Citations:**
- [UNECE WP.29](https://unece.org/wp29-introduction)
- [UNECE GRVA](https://unece.org/transport/road-transport/working-party-automatedautonomous-and-connected-vehicles-introduction)
- [UN R155 consolidated](https://unece.org/sites/default/files/2023-02/R155e%20(2).pdf)

### Q3.25 — ASPICE CL1 self-assessment

**Answer:** ASPICE CL1 = "Performed Process" — work products are produced but the process is not managed/measured. **CL2 (Managed)** would require: planning, monitoring, controlling work products, defining responsibility, managing interfaces. ASPICE process areas demonstrated at CL1: **SWE.1** (requirements), **SWE.2** (architecture), **SWE.4** (unit verification), **SUP.1** (QA), **SUP.10** (change management). ASPICE is **process-side**; ISO 21434 is **product/cyber side** — complementary. OEMs typically require ASPICE CL2/CL3 from Tier-1s alongside 21434 conformance. **My thesis demonstrates engineering rigor at CL1**, not auditable process maturity.

**Citations:**
- [Automotive SPICE / VDA QMC](https://vda-qmc.de/en/automotive-spice/)

### Q3.26 — AUTOSAR R25-11 PQC content

**Answer:** **R25-11 release date:** announced 4 Dec 2025; release dated 27 Nov 2025; ~1,900 incorporation tasks. **R25-11 highlights:** Vehicle Data Protocol, DDS support on CP, security improvements, Rust outlook, IVC overview. **PQC-relevant:** the FO Security Overview added an explicit Post-Quantum Cryptography section, and the CP Crypto Driver SWS evolved to include standardised PQ algorithm-family identifiers. **My thesis pre-dates R25-11 PQC-relevant CryptoStack changes** — the integration pattern (Csm/CryIf/Crypto-Driver seam) is unchanged, so my work is forward-compatible. A clean port to R25-11 would replace my vendor-specific algorithm-family enum with the now-standard literals.

**Citations:**
- [AUTOSAR R25-11 release](https://www.autosar.org/news-events/detail/release-r25-11-is-now-available)
- [SWS Crypto Driver R23-11](https://www.autosar.org/fileadmin/standards/R23-11/CP/AUTOSAR_CP_SWS_CryptoDriver.pdf)

---

# Part IV — Closing Statement (rehearsed for the candidate)

> "This thesis is a credible engineering demonstrator. The headline numbers — 2,045 µs PQC mean latency, 99.14% attack detection across seven vectors, 22 of 23 in-session security clauses PASS, 669+ test cases at 100% pass rate on x86_64 and Pi 4 — are reproducible from the artefacts in the repository. I am explicit about scope: no third-party penetration test (CAL-4 PARTIAL), no HSM-backed key storage (UN R155 M21 PARTIAL), no formal ISO 26262 HARA. The single most important step from a thesis-grade artefact to a Tier-1 submission is **binding a long-term ML-DSA identity key into the ML-KEM handshake** — sigma-style or NIST SP 800-56C Rev 2 AKE. The second is **HSM/TEE-backed key storage**. The third is a **real HARA at the gateway-as-forwarding-element level** to allocate ASIL appropriately. None of these change the cryptographic-agility seam I've demonstrated through Csm → CryIf → PQC; they extend it. I'm prepared to defend each gap as a documented limitation rather than a hidden defect."

---

# Master Citation Index

## NIST primary sources
- [FIPS 203 (ML-KEM Standard)](https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.203.pdf) — §3.3, §7.1, §7.2, §7.3, §8 Table 2
- [FIPS 204 (ML-DSA Standard)](https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.204.pdf) — §3.7, §4 Table 2, §5 Algorithm 7
- [FIPS 205 (SLH-DSA Standard)](https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.205.pdf) — §11
- [NIST IR 8413 (Round 3 Status)](https://nvlpubs.nist.gov/nistpubs/ir/2022/NIST.IR.8413.pdf) — §3, §4
- [NIST IR 8545 (Round 4 / HQC)](https://nvlpubs.nist.gov/nistpubs/ir/2025/NIST.IR.8545.pdf)
- [NIST IR 8547 (PQC Transition)](https://nvlpubs.nist.gov/nistpubs/ir/2024/NIST.IR.8547.ipd.pdf) — §3, §4
- [NIST SP 800-56C Rev 2 (Key Derivation)](https://nvlpubs.nist.gov/nistpubs/SpecialPublications/NIST.SP.800-56Cr2.pdf) — §2, §4, §5
- [NIST SP 800-57 Pt 1 Rev 5 (Key Management)](https://csrc.nist.gov/pubs/sp/800/57/pt1/r5/final)
- [NIST SP 800-90A Rev 1 (DRBG)](https://nvlpubs.nist.gov/nistpubs/specialpublications/nist.sp.800-90ar1.pdf)
- [NIST SP 800-193 (Firmware Resiliency)](https://csrc.nist.gov/pubs/sp/800/193/final)
- [NIST SP 800-208 (Stateful Hash Sigs)](https://nvlpubs.nist.gov/nistpubs/SpecialPublications/NIST.SP.800-208.pdf)
- [NIST SP 800-227 (KEM Recommendations)](https://nvlpubs.nist.gov/nistpubs/SpecialPublications/NIST.SP.800-227.pdf)
- [NIST PQC project page](https://csrc.nist.gov/projects/post-quantum-cryptography)
- [NIST PQC Security Categories](https://csrc.nist.gov/projects/post-quantum-cryptography/post-quantum-cryptography-standardization/evaluation-criteria/security-(evaluation-criteria))
- [NIST press release 13 Aug 2024](https://www.nist.gov/news-events/news/2024/08/nist-releases-first-3-finalized-post-quantum-encryption-standards)
- [NIST press release July 2022](https://www.nist.gov/news-events/news/2022/07/pqc-standardization-process-announcing-four-candidates-be-standardized-plus)
- [NIST HQC announcement March 2025](https://www.nist.gov/news-events/news/2025/03/nist-pqc-standardization-process-hqc-announced-4th-round-selection)

## AUTOSAR sources
- [Classic Platform standards](https://www.autosar.org/standards/classic-platform)
- [Adaptive Platform standards](https://www.autosar.org/standards/adaptive-platform)
- [SWS_SecOC R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_SecureOnboardCommunication.pdf)
- [SecOC PRS R21-11](https://www.autosar.org/fileadmin/standards/R21-11/FO/AUTOSAR_PRS_SecOcProtocol.pdf)
- [SecOC Protocol R23-11](https://www.autosar.org/fileadmin/standards/R23-11/FO/AUTOSAR_FO_PRS_SecOcProtocol.pdf)
- [SOME/IP Protocol R24-11](https://www.autosar.org/fileadmin/standards/R24-11/FO/AUTOSAR_FO_PRS_SOMEIPProtocol.pdf)
- [SWS Crypto Service Manager R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_CryptoServiceManager.pdf)
- [SWS Crypto Interface R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_CryptoInterface.pdf)
- [SWS Crypto Driver R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_CryptoDriver.pdf)
- [SWS Crypto Driver R23-11](https://www.autosar.org/fileadmin/standards/R23-11/CP/AUTOSAR_CP_SWS_CryptoDriver.pdf)
- [SWS Crypto Driver R25-11](https://www.autosar.org/fileadmin/standards/R25-11/CP/AUTOSAR_CP_SWS_CryptoDriver.pdf)
- [SWS PDU Router R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_PDURouter.pdf)
- [SWS Socket Adaptor R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_SocketAdaptor.pdf)
- [SWS CAN Transport Layer R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_CANTransportLayer.pdf)
- [SWS BSW Mode Manager R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_BSWModeManager.pdf)
- [SWS Default Error Tracer R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_DefaultErrorTracer.pdf)
- [General Specification of BSW Modules R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_BSWGeneral.pdf)
- [SWS OS R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_OS.pdf)
- [Communication Management R20-11 (ara::com)](https://www.autosar.org/fileadmin/standards/R20-11/AP/AUTOSAR_SWS_CommunicationManagement.pdf)
- [Communication Management R22-11](https://www.autosar.org/fileadmin/standards/R22-11/AP/AUTOSAR_SWS_CommunicationManagement.pdf)
- [Cryptography (ara::crypto) R21-11](https://www.autosar.org/fileadmin/standards/R21-11/AP/AUTOSAR_SWS_Cryptography.pdf)
- [Operating System Interface R23-11](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_OperatingSystemInterface.pdf)
- [BSW Module List R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_TR_BSWModuleList.pdf)
- [Mode Management Guide R22-11](https://www.autosar.org/fileadmin/standards/R22-11/CP/AUTOSAR_EXP_ModeManagementGuide.pdf)
- [Requirements on Ethernet R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SRS_Ethernet.pdf)
- [R25-11 release announcement](https://www.autosar.org/news-events/detail/release-r25-11-is-now-available)
- [FO Security Overview R25-11](https://www.autosar.org/fileadmin/standards/R25-11/FO/AUTOSAR_FO_EXP_SecurityOverview.pdf)
- [CP Release Overview R25-11](https://www.autosar.org/fileadmin/standards/R25-11/CP/AUTOSAR_CP_TR_ReleaseOverview.pdf)

## ISO / SAE / UNECE
- [ISO/SAE 21434:2021](https://www.iso.org/standard/70918.html)
- [ISO 26262-1:2018 Vocabulary](https://www.iso.org/standard/68383.html)
- [ISO 26262-9:2018 ASIL analyses](https://www.iso.org/standard/68391.html)
- [ISO 26262 series](https://www.iso.org/publication/PUB200262.html)
- [UN R155 consolidated](https://unece.org/sites/default/files/2023-02/R155e%20(2).pdf)
- [UN R155 landing page](https://unece.org/transport/documents/2021/03/standards/un-regulation-no-155-cyber-security-and-cyber-security)
- [UN R156 consolidated](https://unece.org/sites/default/files/2024-03/R156e%20(2).pdf)
- [UN R156 landing page](https://unece.org/transport/documents/2021/03/standards/un-regulation-no-156-software-update-and-software-update)
- [UNECE WP.29](https://unece.org/wp29-introduction)
- [UNECE GRVA](https://unece.org/transport/road-transport/working-party-automatedautonomous-and-connected-vehicles-introduction)

## IETF / IEEE
- [RFC 5869 HKDF](https://datatracker.ietf.org/doc/html/rfc5869)
- [draft-ietf-tls-hybrid-design](https://datatracker.ietf.org/doc/html/draft-ietf-tls-hybrid-design)
- [draft-ietf-tls-ecdhe-mlkem](https://datatracker.ietf.org/doc/draft-ietf-tls-ecdhe-mlkem/)
- [IEEE 1609.2-2022](https://standards.ieee.org/ieee/1609.2/10258/)
- [V2X Security Privacy survey](https://ieeexplore.ieee.org/ielaam/8782711/8815895/9108394-aam.pdf)

## Academic papers (IACR ePrint / arXiv)
- [Kyber paper IACR 2017/634](https://eprint.iacr.org/2017/634.pdf)
- [Dilithium paper IACR 2017/633](https://eprint.iacr.org/2017/633.pdf)
- [QROM Fiat-Shamir IACR 2017/916](https://eprint.iacr.org/2017/916)
- [Falcon Down IACR 2021/772](https://eprint.iacr.org/2021/772)
- [Falcon SCA IACR 2023/224](https://eprint.iacr.org/2023/224.pdf)
- [CPA Attack ML-DSA HW IACR 2025/009](https://eprint.iacr.org/2025/009.pdf)
- [SCA via rejected sigs IACR 2025/582](https://eprint.iacr.org/2025/582)
- [Fault-Injection ML-DSA IACR 2025/904](https://eprint.iacr.org/2025/904.pdf)
- [Krawczyk SIGMA paper](https://webee.technion.ac.il/~hugo/sigma-pdf.pdf)
- [Gidney-Ekerå arXiv:1905.09749](https://arxiv.org/abs/1905.09749)
- [Gidney 2025 arXiv:2505.15917](https://arxiv.org/abs/2505.15917)
- [Rosenstatter PRDC 2019](https://rosenstatter.net/thomas/files/prdc2019ExtendingAUTOSAR.pdf)

## Tooling / vendor / industry
- [liboqs releases](https://github.com/open-quantum-safe/liboqs/releases)
- [liboqs 0.15.0](https://github.com/open-quantum-safe/liboqs/releases/tag/0.15.0)
- [Open Quantum Safe FAQ](https://openquantumsafe.org/faq.html)
- [OQS ML-DSA page](https://openquantumsafe.org/liboqs/algorithms/sig/ml-dsa.html)
- [CVE-2025-48946](https://github.com/open-quantum-safe/liboqs/security/advisories/GHSA-3rxw-4v8q-9gq5)
- [CVE-2025-52473](https://github.com/open-quantum-safe/liboqs/security/advisories/GHSA-qq3m-rq9v-jfgm)
- [PQCA liboqs 0.14.0](https://pqca.org/blog/2025/pqca-announces-release-of-liboqs-version-0-14-0-from-open-quantum-safe-project/)
- [IAV quantumSAR](https://github.com/iavofficial/IAV_quantumSAR)
- [IAV news](https://www.iav.com/news/iav-at-escar-europe)
- [escar Europe](https://escar.info/escar-europe)
- [Bosch E/E architecture](https://www.bosch-mobility.com/en/mobility-topics/ee-architecture/)
- [Bosch zonal whitepaper](https://www.bosch-mobility.com/media/global/mobility-topics/connected-mobility/iot-device-on-wheels/e-e-architecture/whitepaper/230831-bosch-xc-whitepaper-ee-architektur-en.pdf)
- [Bosch vehicle integration platform](https://www.bosch-mobility.com/en/solutions/vehicle-computer/vehicle-integration-platform/)
- [Bosch Zone ECU](https://www.bosch-mobility.com/en/solutions/control-units/zone-ecu/)
- [Infineon AURIX TC3xx HSM](https://www.infineon.com/dgdl/Infineon-AURIX_TC3xx_Hardware_Security_Module_Quick-Training-v01_00-EN.pdf?fileId=5546d46274cf54d50174da4ebc3f2265)
- [GlobalPlatform TEE](https://globalplatform.org/specs-library/tee-system-architecture/)
- [EVITA HSM publication](https://evita-project.org/Publications/WG11.pdf)
- [Arm Cortex-A comparison](https://www.arm.com/-/media/Arm%20Developer%20Community/PDF/Cortex-A%20R%20M%20datasheets/Arm%20Cortex-A%20Comparison%20Table_v4.pdf)
- [Arm Cortex-R comparison](https://www.arm.com/-/media/Arm%20Developer%20Community/PDF/Cortex-A%20R%20M%20datasheets/Arm%20Cortex-R%20Comparison%20Table.pdf)
- [Linux getrandom(2)](https://man7.org/linux/man-pages/man2/getrandom.2.html)
- [Linux random(7)](https://man7.org/linux/man-pages/man7/random.7.html)

## Pi 4 ≠ Classic AUTOSAR target (Cluster 1B evidence)
- [OSEK Wikipedia — MMU-free target, AUTOSAR Classic OS reuse](https://en.wikipedia.org/wiki/OSEK)
- [Elektrobit Classic AUTOSAR OS — supported MCU evaluation list (AURIX/S32K/RH850/Silver/VEOS)](https://www.elektrobit.com/products/ecu/eb-tresos/operating-systems/)
- [eSOL — Classic vs Adaptive hardware difference](https://blog.esol.com/autosar_ap_cp_difference)
- [Lauterbach AUTOSAR-aware debugging — supported AUTOSAR OS targets](https://www.lauterbach.com/supported-platforms/autosar)
- [SWS CAN Driver R21-11 — MCAL register-level requirements](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_CANDriver.pdf)
- [SWS Operating System AUTOSAR CP R23-11 — OSEK conformance classes](https://www.autosar.org/fileadmin/standards/R23-11/CP/AUTOSAR_CP_SWS_OS.pdf)
- [SWS Operating System AUTOSAR CP R22-11](https://www.autosar.org/fileadmin/standards/R22-11/CP/AUTOSAR_SWS_OS.pdf)
- [Lennartsson, *Porting AUTOSAR to a high performance embedded platform*, MDH/SICS thesis 2013](https://www.diva-portal.org/smash/get/diva2:648352/FULLTEXT01.pdf)
- [Saito et al., *Porting an AUTOSAR-Compliant RTOS to a high performance ARM embedded platform*, ACM SIGBED 2014](https://sigbed.seas.upenn.edu/archives/2014-02/ewili13_submission_9.pdf)
- [MOPED / SICS-SE — ArcticCore-on-Pi research platform](https://github.com/sics-sse/moped/tree/master/autosar)
- [linux-for-adaptive-autosar-on-raspberry-pi (showing Pi is the AP target)](https://github.com/vical1024/linux-for-adaptive-autosar-on-raspberry-pi)
- [PQClean — heap-free PQC reference impls](https://github.com/PQClean/PQClean)
- [Linux SocketCAN documentation](https://www.kernel.org/doc/Documentation/networking/can.txt)

## Compliance / audit / process
- [MISRA Compliance:2020](https://misra.org.uk/app/uploads/2021/06/MISRA-Compliance-2020.pdf)
- [TÜV SÜD ISO/SAE 21434](https://www.tuvsud.com/en-us/services/cyber-security/safety-components)
- [TÜV NORD ISO/SAE 21434](https://www.tuv-nord.com/us/en/services/cybersecurity/iso-sae-21434-cybersecurity-for-road-vehicles-csms/)
- [Code Intelligence — CALs in 21434](https://www.code-intelligence.com/blog/iso21434-cybersecurity-assurance-levels)
- [LHP — ASIL in 26262](https://www.lhpes.com/blog/what-is-an-asil)
- [MDPI Sensors ISO 26262/21434 co-analysis](https://www.mdpi.com/1424-8220/24/6/1848)
- [UL SIS — TARA in Practice](https://www.ul.com/sis/resources/threat-analyses-and-risk-assessment-in-practice-tara)
- [Comparative AFR methods](https://www.researchgate.net/publication/339390034_Comparative_Evaluation_of_Cybersecurity_Methods_for_Attack_Feasibility_Rating_per_ISOSAE_DIS_21434)
- [OWASP IoT Security Testing Guide](https://owasp.org/owasp-istg/)
- [VDA QMC Automotive SPICE](https://vda-qmc.de/en/automotive-spice/)
- [ENISA Post-Quantum Cryptography](https://www.enisa.europa.eu/publications/post-quantum-cryptography-current-state-and-quantum-mitigation)
- [ENISA PQC Integration Study](https://www.enisa.europa.eu/publications/post-quantum-cryptography-integration-study)
- [CISA/NSA/NIST Quantum-Readiness Factsheet](https://www.cisa.gov/sites/default/files/2023-08/Quantum%20Readiness_Final_CLEAR_508c%20(3).pdf)
- [NSM-10](https://bidenwhitehouse.archives.gov/briefing-room/statements-releases/2022/05/04/national-security-memorandum-on-promoting-united-states-leadership-in-quantum-computing-while-mitigating-risks-to-vulnerable-cryptographic-systems/)
