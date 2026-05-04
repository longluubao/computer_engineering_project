# Professor Examiner #1: AUTOSAR Classic Platform & SecOC architecture specialist

Thesis under defense: *"AUTOSAR SecOC with Post-Quantum Cryptography on a Raspberry Pi 4 Ethernet Gateway"*. All citations retrieved 2026-05-02.

---

## Cluster 1 — Classic vs Adaptive Platform

### Q1.1: Why Classic Platform R21-11 and not the Adaptive Platform?

**Question:** "You explicitly chose AUTOSAR Classic Platform R21-11 for an Ethernet gateway running Linux on aarch64. The Adaptive Platform was designed exactly for POSIX-class hardware, service-oriented communication and OTA updatability — all of which a quantum-secure gateway needs. Why is your work not on AP, where it would arguably belong, and what does Classic give you that AP would not?"

**Why this matters:** The thesis builds a Classic stack on a SoC that fits the AP target profile. The candidate must justify this architectural mismatch.

**Expected/strong answer outline:**
- Classic targets statically configured deeply-embedded ECUs with OSEK-style OS; AP targets POSIX PSE51-conformant HPC nodes.
- SecOC is currently a Classic BSW module; AP uses `ara::com` E2E + `ara::crypto`, not SecOC, so a "SecOC-with-PQC" thesis only makes sense on Classic.
- Classic gives compile-time determinism, no dynamic memory, and a per-PDU MAC/signature pipeline.
- Trade-off: AP would give SOME/IP, dynamic deployment, and `ara::crypto` KEM hooks at the cost of a different security paradigm.

**Citations:**
- [AUTOSAR Classic Platform — Standards landing page](https://www.autosar.org/standards/classic-platform) (retrieved 2026-05-02)
- [AUTOSAR Adaptive Platform — Standards landing page](https://www.autosar.org/standards/adaptive-platform) (retrieved 2026-05-02)
- [Specification of Secure Onboard Communication, AUTOSAR CP R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_SecureOnboardCommunication.pdf) (retrieved 2026-05-02)

---

### Q1.2: ara::com vs RTE — equivalent or fundamentally different?

**Question:** "On Adaptive, the equivalent of the Classic RTE is `ara::com`. Concretely, how would your PQC integration look if you had to plug into `ara::com` event/method signatures and the AP service-oriented model instead of into PduR + SecOC? Where in your code would the seam move?"

**Why this matters:** Tests whether the candidate understands AP communication is service-oriented and proxy/skeleton-based, not PDU-based — so the SecOC seam disappears.

**Expected/strong answer outline:**
- RTE is generated C glue over static port-based comm; `ara::com` is C++ middleware with proxy/skeleton classes and service discovery (typically SOME/IP/SD).
- On AP there is no "Secured I-PDU" abstraction; protection is end-to-end on serialised events.
- A PQC port to AP would replace SecOC with custom transformers in the binding, or use `ara::crypto` KEM/Sign primitives directly.
- The Classic-side seam (`Csm_SignatureGenerate` → CryIf → Crypto Driver) does not exist on AP; AP uses a Crypto Provider abstraction inside `ara::crypto`.

**Citations:**
- [Specification of Communication Management, AUTOSAR AP R20-11 (ara::com)](https://www.autosar.org/fileadmin/standards/R20-11/AP/AUTOSAR_SWS_CommunicationManagement.pdf) (retrieved 2026-05-02)
- [Specification of Cryptography (ara::crypto), AUTOSAR AP R21-11](https://www.autosar.org/fileadmin/standards/R21-11/AP/AUTOSAR_SWS_Cryptography.pdf) (retrieved 2026-05-02)

---

### Q1.3: Why is SecOC not specified for the Adaptive Platform?

**Question:** "Why does the AUTOSAR Adaptive Platform not have a SecOC module? Walk me through what AP uses instead, and whether your design therefore generalises to AP at all."

**Why this matters:** Probes platform knowledge — many candidates incorrectly assume SecOC is platform-agnostic.

**Expected/strong answer outline:**
- SecOC is defined only for Classic CP.
- AP relies on E2E protection profiles (in `ara::com` bindings) and SecOC-style protection at the SOME/IP transformer rather than as a separate BSW module.
- Reuse story: `ara::crypto`'s KEM and Signature service interfaces could host ML-KEM/ML-DSA, but PDU framing and FvM design would need re-derivation.

**Citations:**
- [Specification of Communication Management, AUTOSAR AP R22-11](https://www.autosar.org/fileadmin/standards/R22-11/AP/AUTOSAR_SWS_CommunicationManagement.pdf) (retrieved 2026-05-02)
- [AUTOSAR Adaptive Platform — Standards landing page](https://www.autosar.org/standards/adaptive-platform) (retrieved 2026-05-02)

---

### Q1.4: POSIX/PSE51 vs Classic OS scheduling

**Question:** "Your target runs Linux 6.1 on a Raspberry Pi. The Classic Platform OS is OSEK-style, statically configured, with priority-based preemption and no dynamic process creation. What does your 'OS abstraction' actually run on top of, and how do you reconcile Classic OS semantics (AUTOSAR_SWS_OS task scheduling, resource ceiling) with mainline Linux scheduling?"

**Why this matters:** A Classic OS is not Linux. The candidate must explicitly identify the abstraction gap.

**Expected/strong answer outline:**
- AP allows POSIX PSE51; Classic does not.
- Mainline Linux gives no hard real-time guarantees; PREEMPT_RT or a hypervisor partition would be needed for ASIL-C/D claims.
- Likely answer: a thin pthread-based scheduler simulates the periodic main function; this is research-grade, not production.

**Citations:**
- [Specification of Operating System, AUTOSAR CP R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_OS.pdf) (retrieved 2026-05-02)
- [Specification of Operating System Interface, AUTOSAR AP R23-11](https://www.autosar.org/fileadmin/standards/R23-11/AP/AUTOSAR_AP_SWS_OperatingSystemInterface.pdf) (retrieved 2026-05-02)

---

## Cluster 2 — Why an Ethernet gateway and not a Zonal HPC?

### Q2.1: Domain vs Zonal architecture positioning

**Question:** "Modern E/E architectures from Bosch, NXP, etc., are moving from domain-based ECUs to zonal controllers feeding a central vehicle HPC over multi-gig automotive Ethernet. Your thesis presents a CAN-Ethernet gateway in Classic — essentially the previous-generation pattern. How does your work map onto a zonal architecture, and is Classic SecOC the right tool there?"

**Why this matters:** Examiners want to see industrial relevance.

**Expected/strong answer outline:**
- A CAN-to-Ethernet gateway is exactly the role of a *zonal* controller, so the demonstrator is plausible if framed that way.
- Modern central HPCs typically run AP/Linux; the BSW-style gateway sits in the zonal layer.
- SecOC handles the legacy CAN side; the Ethernet side towards the HPC would more typically use IPsec/MACsec/TLS, which the thesis does not.

**Citations:**
- [Bosch Mobility — Vehicle integration platform](https://www.bosch-mobility.com/en/solutions/vehicle-computer/vehicle-integration-platform/) (retrieved 2026-05-02)
- [Bosch Mobility — Zone ECU product page](https://www.bosch-mobility.com/en/solutions/control-units/zone-ecu/) (retrieved 2026-05-02)

---

### Q2.2: Why SecOC over MACsec/IPsec/TLS on the Ethernet leg?

**Question:** "On 100 Mbps automotive Ethernet, MACsec (IEEE 802.1AE) and IPsec are the production-deployed link/network-layer protections. Why are you authenticating per Secured I-PDU at the SecOC layer with a 3,309-byte ML-DSA-65 signature, instead of using a link-layer protection with much smaller per-frame overhead?"

**Why this matters:** Forces engineering trade-off discussion: SecOC gives end-to-end PDU-granular auth; MACsec gives bulk link auth.

**Expected/strong answer outline:**
- SecOC provides end-to-end authentication across mixed transports (CAN + Ethernet); MACsec only protects single Ethernet links.
- AUTOSAR requires SecOC for in-vehicle PDU authentication regardless of underlying link.
- Trade-off: 3,309 B per Secured I-PDU is the price of E2E PQ-authenticity above the routing layer.
- MACsec/TLS could be layered orthogonally; this thesis focuses on the BSW seam.

**Citations:**
- [Specification of Socket Adaptor, AUTOSAR CP R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_SocketAdaptor.pdf) (retrieved 2026-05-02)
- [Requirements on Ethernet Support in AUTOSAR, R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SRS_Ethernet.pdf) (retrieved 2026-05-02)

---

## Cluster 3 — SecOC SWS conformance

### Q3.1: SWS_SecOC_00054 — your acknowledged Det gating deviation

**Question:** "Your `SecOC.c` calls `Det_ReportError()` unconditionally — eight call sites, by your own admission. SWS_SecOC_00054 in R21-11 requires that calls to `Det_ReportError` be enclosed by `#if (SECOC_DEV_ERROR_DETECT == STD_ON)`. Walk me through (a) why this is a non-conformance, (b) the production impact (code size, runtime hit in series ECUs that compile DET out), and (c) why a thesis that 'PASSES 22 of 23 standards clauses' should not have failed this one."

**Why this matters:** The candidate already lists this as a deviation; examiner will press for a real defence.

**Expected/strong answer outline:**
- Acknowledge: this is a documented requirement violation, not stylistic.
- Impact: in production builds with `SECOC_DEV_ERROR_DETECT == STD_OFF`, every Det call is dead code that still pulls Det symbols, inflating ROM and possibly preventing DET removal.
- Mitigation: trivially fixed by wrapping the calls.
- Thesis-defence framing: a quality-process gap, not a security flaw, but it would block a Tier-1 code-review gate.

**Citations:**
- [Specification of Secure Onboard Communication, AUTOSAR CP R21-11 — `[SWS_SecOC_00054]`](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_SecureOnboardCommunication.pdf) (retrieved 2026-05-02)
- [Specification of Default Error Tracer, AUTOSAR CP R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_DefaultErrorTracer.pdf) (retrieved 2026-05-02)
- [General Specification of Basic Software Modules, AUTOSAR CP R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_BSWGeneral.pdf) (retrieved 2026-05-02)

---

### Q3.2: DataToAuthenticator construction order

**Question:** "Define exactly what bytes go into the `DataToAuthenticator` for SecOC, in what order, including the role of the truncated freshness vs the full freshness, and where SWS_SecOC_00031 / 00220 / 00221 / 00222 fix that order. Then tell me whether your implementation matches the SWS byte order or has its own."

**Why this matters:** This is the most tested-on detail of SecOC. Examiners want bit-accurate knowledge.

**Expected/strong answer outline:**
- DataToAuthenticator = Data Identifier (optional) || Authentic I-PDU || Complete (non-truncated) Freshness Value.
- Full freshness is signed/MACed; truncated freshness goes on the wire (length = `SecOCFreshnessValueTruncLength`).
- Receiver reconstructs the full freshness from its own state + the truncated bits.
- Bit alignment: freshness aligned MSB-first per SWS_SecOC_00220.

**Citations:**
- [Specification of Secure Onboard Communication, AUTOSAR CP R21-11 — `[SWS_SecOC_00031]`, `[SWS_SecOC_00220]`, `[SWS_SecOC_00221]`, `[SWS_SecOC_00222]`](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_SecureOnboardCommunication.pdf) (retrieved 2026-05-02)
- [Specification of SecOC Protocol (PRS), AUTOSAR FO R21-11](https://www.autosar.org/fileadmin/standards/R21-11/FO/AUTOSAR_PRS_SecOcProtocol.pdf) (retrieved 2026-05-02)

---

### Q3.3: Truncated freshness — why is 8 bits enough?

**Question:** "You configure `SECOC_TX_FRESHNESS_VALUE_LENGTH=24` for the data-to-authenticate but only `..._TRUNC_LENGTH=8` on the wire. Walk me through the wraparound and de-synchronisation analysis: how big is the rolling resync window, what happens after >256 missed messages on the bus, and how does that interact with your 64-bit PQC counter?"

**Why this matters:** Truncated freshness is the most subtle SecOC detail; an 8-bit wire counter is dangerous on a busy CAN bus.

**Expected/strong answer outline:**
- Receiver uses upper 16 bits of stored full freshness + 8 bits from PDU; resync window = 128 frames (signed-distance argument).
- Beyond that the receiver cannot tell whether the freshness moved forward or wrapped.
- For PQC-mode the freshness is conceptually 64 bits; the 8-bit wire field is a CAN-side compatibility relic.
- Counter-jump-handling and FvM resync rules.

**Citations:**
- [Specification of Secure Onboard Communication, AUTOSAR CP R21-11 — Freshness chapter](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_SecureOnboardCommunication.pdf) (retrieved 2026-05-02)
- [Rosenstatter et al., *Extending AUTOSAR's Counter-Based Solution for Freshness of Authenticated Messages in Vehicles*, PRDC 2019](https://rosenstatter.net/thomas/files/prdc2019ExtendingAUTOSAR.pdf) (retrieved 2026-05-02)

---

### Q3.4: Authentication Build-Up rule

**Question:** "Explain the SecOC 'authentication build-up' counter: when a verification fails, what does the SWS require you to do before declaring `E_NOT_OK` to upper layers, and how does that interact with the freshness resync mechanism?"

**Why this matters:** The Authentication Attempt Counter is mandated by the SWS — many implementations skip it.

**Expected/strong answer outline:**
- The SWS requires a configurable number of re-verification attempts using adjusted freshness assumptions before declaring failure.
- Counter is per-PDU and reset on success.
- Candidate should cite `SecOCAuthenticationBuildAttempts`.

**Citations:**
- [Specification of Secure Onboard Communication, AUTOSAR CP R21-11 — Authentication build-up section](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_SecureOnboardCommunication.pdf) (retrieved 2026-05-02)

---

### Q3.5: PduR routing path for Secured I-PDU

**Question:** "Draw me the PduR routing path for a Secured I-PDU on the Tx side: where does PduR see the *Authentic* I-PDU vs the *Secured* I-PDU, and which API on PduR does SecOC call back into so the Secured PDU is forwarded onwards to CanIf or SoAd?"

**Why this matters:** Tests core BSW understanding. SecOC sits between two PduR routing paths.

**Expected/strong answer outline:**
- PduR sees Authentic I-PDU → routes upward to SecOC; SecOC produces Secured I-PDU and calls `PduR_SecOCTransmit` which routes downward to CanIf/SoAd.
- Two distinct routing tables; same PduR module.
- Rx: lower-layer indication → PduR → SecOC → after auth → PduR forwards Authentic I-PDU to COM.

**Citations:**
- [Specification of PDU Router, AUTOSAR CP R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_PDURouter.pdf) (retrieved 2026-05-02)
- [Specification of Secure Onboard Communication, AUTOSAR CP R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_SecureOnboardCommunication.pdf) (retrieved 2026-05-02)

---

## Cluster 4 — Csm / CryIf cryptographic agility

### Q4.1: Where exactly does ML-DSA enter Csm?

**Question:** "Trace, by SWS service ID and `Csm_*` API name, the call chain from `SecOC_Authenticate` through Csm down to your `PQC_MLDSA_Sign`. Which Csm API are you using — `Csm_MacGenerate` (wrong, that's HMAC) or `Csm_SignatureGenerate`? What `CryptoKeyId` and what `CryptoJobInfo` fields did you populate?"

**Why this matters:** The crypto-agility seam is the entire engineering claim of the thesis.

**Expected/strong answer outline:**
- Correct: `Csm_SignatureGenerate(jobId, mode, dataPtr, dataLen, signaturePtr, signatureLenPtr)`.
- `jobId` references a configured `CsmJob` whose `CsmKey` references a `CsmKeyElement` of type "asymmetric private".
- `CsmJobInfo` carries algorithm family — but pre-R25-11 SWS did not list ML-DSA family literals; you had to use `CRYPTO_ALGOFAM_CUSTOM` or extend the enum.
- CryIf dispatches to the configured Crypto Driver; PQC.c is registered as a Crypto Driver.

**Citations:**
- [Specification of Crypto Service Manager, AUTOSAR CP R21-11 — `Csm_SignatureGenerate`](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_CryptoServiceManager.pdf) (retrieved 2026-05-02)
- [Specification of Crypto Interface, AUTOSAR CP R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_CryptoInterface.pdf) (retrieved 2026-05-02)
- [Specification of Crypto Driver, AUTOSAR CP R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_CryptoDriver.pdf) (retrieved 2026-05-02)

---

### Q4.2: Algorithm family identifier — did you extend the enum?

**Question:** "In R21-11, `Crypto_AlgorithmFamilyType` does not contain an ML-DSA literal. Did you extend `CRYPTO_ALGOFAM_*` with a new enumerator? If yes, did you renumber existing values (which would break ABI), and how did you tell Csm/CryIf at config time to dispatch ML-DSA jobs to your driver?"

**Why this matters:** This is a real conformance issue — pre-R25-11 the algorithm-family enum did not have PQC.

**Expected/strong answer outline:**
- Two clean options: (a) reserve a vendor-specific value above the standard range (preserves ABI), or (b) wait for R25-11 which adds PQC families.
- Whatever choice, `CsmKeyElement`/`CryptoPrimitive` configuration must agree.
- IAV's quantumSAR took option (a) for R23-11.

**Citations:**
- [Specification of Crypto Service Manager, AUTOSAR CP R21-11 — `Crypto_AlgorithmFamilyType`](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_CryptoServiceManager.pdf) (retrieved 2026-05-02)
- [Specification of Crypto Driver, AUTOSAR CP R25-11](https://www.autosar.org/fileadmin/standards/R25-11/CP/AUTOSAR_CP_SWS_CryptoDriver.pdf) (retrieved 2026-05-02)
- [IAV quantumSAR — GitHub](https://github.com/iavofficial/IAV_quantumSAR) (retrieved 2026-05-02)

---

### Q4.3: Asynchronous Csm jobs and 250 µs sign latency

**Question:** "ML-DSA-65 sign on aarch64 is around 250 µs. Csm jobs can be sync or async. If you used a sync `Csm_SignatureGenerate`, you blocked the SecOC main function for the entire signing cost; if async, you have to manage the callback. Which mode did you use, and did you measure SecOC main-function jitter?"

**Why this matters:** Real-time correctness depends on this.

**Expected/strong answer outline:**
- Recommended: async (`CRYPTO_PROCESSING_ASYNC`) so SecOC main function does not block.
- Async forces a state-machine in SecOC's authenticate path (queued PDUs awaiting signature).
- Synchronous on Linux/Pi may be acceptable for demonstration but is unsuitable for a real-time ECU at 10 ms cycle.

**Citations:**
- [Specification of Crypto Service Manager, AUTOSAR CP R21-11 — sync/async processing modes](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_CryptoServiceManager.pdf) (retrieved 2026-05-02)

---

### Q4.4: Key import and CryptoKey lifecycle

**Question:** "Your boot-time provisioning has four modes (DEMO_FILE_AUTO, FILE_STRICT, PROVISIONED, HSM_HANDLE). At which Csm/CryIf API do you import the ML-DSA private key into a `CryptoKey` slot, and how do you ensure the key never appears in plaintext memory in PROVISIONED/HSM modes?"

**Why this matters:** Tests understanding that Csm key management is a separate API surface from the crypto-operation API.

**Expected/strong answer outline:**
- `Csm_KeyElementSet` (or `Csm_KeyElementCopy`) populates a `CsmKeyElement`; lifecycle managed by `Csm_KeyDeriv*`/`Csm_KeyExchangeCalcSecret`.
- HSM_HANDLE mode → key never enters software memory; Csm just gets a handle that CryIf forwards to the HSM driver.
- PROVISIONED file → key still touches RAM but only at boot; should be locked/zeroized after import.

**Citations:**
- [Specification of Crypto Service Manager, AUTOSAR CP R21-11 — Key Management API chapter](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_CryptoServiceManager.pdf) (retrieved 2026-05-02)

---

## Cluster 5 — PduR & transport

### Q5.1: TP mode for the 3,343-byte Secured I-PDU

**Question:** "An ML-DSA-65 signature is 3,309 bytes; with the Authentic PDU + freshness + header your Secured I-PDU is roughly 3,343 bytes. On Ethernet you must use TP-mode through SoAd, not IF-mode. Why? And on the CAN side, how would the *same* PDU even be transmittable — would CanTp segment it across hundreds of consecutive frames?"

**Why this matters:** The candidate must show they understand the 8/64-byte CAN frame limit vs IF/TP semantics.

**Expected/strong answer outline:**
- IF-mode (single-frame) caps at 8 B (CAN), 64 B (CAN-FD), MTU on Ethernet.
- TP-mode (CanTp / SoAdTp) provides segmentation+reassembly.
- 3,343 B over CAN classical = ~478 consecutive frames at 7 B payload — wholly impractical at 500 kbps.
- On Ethernet TP-mode with TCP underneath gives reliable delivery.

**Citations:**
- [Specification of CAN Transport Layer, AUTOSAR CP R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_CANTransportLayer.pdf) (retrieved 2026-05-02)
- [Specification of PDU Router, AUTOSAR CP R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_PDURouter.pdf) (retrieved 2026-05-02)
- [Specification of Socket Adaptor, AUTOSAR CP R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_SocketAdaptor.pdf) (retrieved 2026-05-02)

---

### Q5.2: 8-byte receive buffer "violation"

**Question:** "Your earlier code had a fixed 8-byte receive buffer in the SecOC Rx path. Why was that a violation of PduR / SecOC requirements once a TP-mode Secured I-PDU is in play, and exactly which buffer-allocation API (`PduR_SecOCStartOfReception` / `..CopyRxData`) did you have to change?"

**Why this matters:** Direct test of the bug-fix narrative against the SWS API surface.

**Expected/strong answer outline:**
- TP reception uses `StartOfReception` (size announced) → repeated `CopyRxData` chunks → `TpRxIndication`.
- A static 8-B buffer cannot accept the 3,343-B PQC PDU.
- Fix: dynamically sized buffer (or static `SECOC_RX_TP_BUFFER_LENGTH` ≥ max-PDU + header).

**Citations:**
- [Specification of PDU Router, AUTOSAR CP R21-11 — TP API](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_PDURouter.pdf) (retrieved 2026-05-02)
- [Specification of Secure Onboard Communication, AUTOSAR CP R21-11 — `SecOC_StartOfReception` / `SecOC_CopyRxData`](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_SecureOnboardCommunication.pdf) (retrieved 2026-05-02)

---

### Q5.3: CanTp vs LdCom — why not LdCom for the gateway?

**Question:** "LdCom (Large Data COM) is the AUTOSAR-prescribed module for application data that does not need COM signal-level processing. For a pure gateway between CAN and Ethernet, would LdCom not be a more correct upper-layer than COM? Why does your design route through COM at all?"

**Why this matters:** Sanity-check on the upper-layer choice.

**Expected/strong answer outline:**
- A gateway that only forwards I-PDUs without unpacking signals should use LdCom.
- COM is needed only where signal-level routing/E2E exists.
- Acceptable answer: candidate explicitly chose COM for compatibility with their test bench.

**Citations:**
- [List of Basic Software Modules, AUTOSAR Release R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_TR_BSWModuleList.pdf) (retrieved 2026-05-02)
- [Specification of PDU Router, AUTOSAR CP R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_PDURouter.pdf) (retrieved 2026-05-02)

---

## Cluster 6 — SoAd & Ethernet stack

### Q6.1: Why is SoAd triggering ML-KEM rekey?

**Question:** "You trigger ML-KEM-768 re-keying every 360 000 cycles (~1 hour) inside SoAd. SoAd is a *socket adapter* — its job is to map I-PDUs to TCP/UDP sockets, not to manage cryptographic state. Mode-driven activities like rekeying belong in BswM (mode arbitration) or EcuM (lifecycle). Why is rekey logic in SoAd?"

**Why this matters:** Architectural smell. AUTOSAR has a clear separation: BswM decides, modules execute.

**Expected/strong answer outline:**
- Correct architecture: a BswM mode-request port "Rekey_Required" toggled by a timer SWC → BswM action list calls `PQC_KeyExchange_Trigger()`.
- EcuM partial-network style or BswM rule could schedule it.
- Acknowledge SoAd placement is expedient for the demonstrator; refactor on roadmap.

**Citations:**
- [Specification of Socket Adaptor, AUTOSAR CP R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_SocketAdaptor.pdf) (retrieved 2026-05-02)
- [Specification of Basic Software Mode Manager, AUTOSAR CP R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_BSWModeManager.pdf) (retrieved 2026-05-02)
- [Guide to Mode Management, AUTOSAR CP R22-11](https://www.autosar.org/fileadmin/standards/R22-11/CP/AUTOSAR_EXP_ModeManagementGuide.pdf) (retrieved 2026-05-02)

---

### Q6.2: PDU-to-socket binding

**Question:** "SoAd binds each PduId to a SoAd Socket Connection. For your 3,343-byte Secured I-PDU you presumably use a TCP socket (TP-API). What `SoAdSocketProtocol` (TCP/UDP), `SoAdPduHeaderId`, and message-acceptance policy did you configure, and does it accept multiple PduIds on the same connection or one-PduId-per-socket?"

**Why this matters:** Forces the candidate to be concrete about SoAd configuration.

**Expected/strong answer outline:**
- TCP for reliable + ordered + flow-controlled delivery of a multi-segment Secured I-PDU.
- A SoAd PDU header is required if multiple PDUs share a connection.
- Message-acceptance policy whitelists peer IP/port to prevent off-bus attackers.

**Citations:**
- [Specification of Socket Adaptor, AUTOSAR CP R21-11 — Ch. 7 (PDU to Socket Connection mapping)](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_SocketAdaptor.pdf) (retrieved 2026-05-02)

---

## Cluster 7 — R21-11 vs R23-11 / R25-11 and the field

### Q7.1: Has cryptographic agility changed in R25-11?

**Question:** "AUTOSAR R25-11 (released December 2025) explicitly addresses post-quantum cryptography in the security overview. Does R25-11 change the Csm / CryIf / Crypto Driver interfaces in a way that would invalidate or simplify your PQC integration done on R21-11?"

**Why this matters:** Knowing the standards trajectory shows research currency.

**Expected/strong answer outline:**
- R25-11 FO Security Overview adds a Post-Quantum Cryptography section.
- The CP Crypto Driver SWS evolved to recognise PQ algorithm families.
- Candidate's R21-11 work would need re-mapping of vendor-specific algorithm-family IDs to the now-standardised ones; functionally compatible.

**Citations:**
- [AUTOSAR News — Release R25-11 is Now Available](https://www.autosar.org/news-events/detail/release-r25-11-is-now-available) (retrieved 2026-05-02)
- [Explanation of Security Overview, AUTOSAR FO R25-11](https://www.autosar.org/fileadmin/standards/R25-11/FO/AUTOSAR_FO_EXP_SecurityOverview.pdf) (retrieved 2026-05-02)
- [Specification of Crypto Driver, AUTOSAR CP R25-11](https://www.autosar.org/fileadmin/standards/R25-11/CP/AUTOSAR_CP_SWS_CryptoDriver.pdf) (retrieved 2026-05-02)

---

### Q7.2: Compare your work to IAV quantumSAR

**Question:** "IAV's quantumSAR project, presented at escar Europe 2025, is essentially the same idea — Crystals-Dilithium / Kyber / SPHINCS+ / Falcon plugged into AUTOSAR R23-11 as a Crypto Driver, MISRA-checked. What does your thesis offer that quantumSAR does not, and where does quantumSAR do something better?"

**Why this matters:** Thesis must position itself relative to industrial state of the art.

**Expected/strong answer outline:**
- Differences: thesis includes full SecOC Rx/Tx integration on a Linux gateway; quantumSAR focuses on the Crypto Driver layer for microcontrollers.
- quantumSAR uses PQClean (different lib than liboqs); both target Classic.
- Thesis adds CAN+Ethernet gateway scenario and re-keying scheduler.

**Citations:**
- [IAV quantumSAR — GitHub](https://github.com/iavofficial/IAV_quantumSAR) (retrieved 2026-05-02)
- [IAV at escar Europe — IAV news page](https://www.iav.com/news/iav-at-escar-europe) (retrieved 2026-05-02)
- [escar Europe — official conference page](https://escar.info/escar-europe) (retrieved 2026-05-02)

---

## Cluster 8 — MISRA C:2012 / coding standards

### Q8.1: MISRA baseline and amendment status

**Question:** "Which MISRA C:2012 amendment level are you claiming compliance with — original, AMD1 (2016), AMD2/AMD3, or MISRA Compliance:2020? And do you distinguish Mandatory, Required, and Advisory rules in your deviation register?"

**Why this matters:** "MISRA-compliant" without a level claim is meaningless.

**Expected/strong answer outline:**
- MISRA Compliance:2020 is the modern framework: claim must list Mandatory (no deviations) and Required (deviations require formal sign-off).
- A defensible answer cites the specific amendment level.

**Citations:**
- [MISRA Compliance:2020 PDF](https://misra.org.uk/app/uploads/2021/06/MISRA-Compliance-2020.pdf) (retrieved 2026-05-02)

---

### Q8.2: 8 unconditional Det calls vs MISRA & SWS

**Question:** "You ship eight unconditional `Det_ReportError` calls. Independent of SWS_SecOC_00054, is that a MISRA deviation as well? Specifically Rule 2.1 (unreachable code in production builds with DET=OFF) and Directive 4.1 (run-time failures shall be minimised). Have you logged a formal deviation per MISRA Compliance:2020?"

**Why this matters:** Combines the SWS issue with MISRA hygiene.

**Expected/strong answer outline:**
- Rule 2.1 (no dead code) potentially triggers if Det implementation is empty in production.
- Directive 4.1 covers defensive-programming usage of Det reports.
- Without a formal deviation in the register, MISRA compliance is unsupported.

**Citations:**
- [MISRA Compliance:2020 PDF](https://misra.org.uk/app/uploads/2021/06/MISRA-Compliance-2020.pdf) (retrieved 2026-05-02)
- [Specification of Default Error Tracer, AUTOSAR CP R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_DefaultErrorTracer.pdf) (retrieved 2026-05-02)

---

## Cluster 9 — ISO 26262 / 21434 absence

### Q9.1: No HARA, no ASIL — defensible?

**Question:** "There is no Hazard Analysis and Risk Assessment, no item definition, no ASIL classification. You quote a 5 ms latency budget — but a 5 ms budget without an ASIL allocation is just a number. How would you go from this thesis to an ISO 26262 ASIL-B/C/D claim, and which work products are missing today?"

**Why this matters:** Examiner will check whether the candidate knows what *would* be required.

**Expected/strong answer outline:**
- Missing: item definition, HARA (S, E, C → ASIL), safety goals, FSC/TSC, software safety requirements, HW-SW interface, ASIL-decomposition rationale.
- 5 ms is plausible only if mapped to a brake/steer FTTI; without a hazard, unjustified.
- Roadmap: ASIL-B is the realistic ceiling for a gateway; ASIL-D requires HW lockstep on the host MCU and a different OS.

**Citations:**
- [LHP Engineering Solutions — Understanding ASIL in ISO 26262](https://www.lhpes.com/blog/what-is-an-asil) (retrieved 2026-05-02)
- [MDPI Sensors 24(6):1848 — Co-analysis ISO 26262 / ISO 21434](https://www.mdpi.com/1424-8220/24/6/1848) (retrieved 2026-05-02)

---

### Q9.2: ISO/SAE 21434 cybersecurity assurance level (CAL)

**Question:** "ISO/SAE 21434 introduces Cybersecurity Assurance Levels (CAL 1-4). What CAL would your gateway target, what is the corresponding TARA outcome, and how do your PQC mitigations map onto specific UN R155 Annex 5 threats?"

**Why this matters:** Maps the security posture onto regulatory framework.

**Expected/strong answer outline:**
- TARA at vehicle level → CAL 3-4 for a gateway.
- R155 Annex 5 threats addressed: communication-channel manipulation, replay, signature forgery, partly key compromise via rekey.
- Gap acknowledged: M21 key management is only partial (filesystem-stored keys).

**Citations:**
- [UN ECE R155 (E), official UNECE PDF](https://unece.org/sites/default/files/2023-02/R155e%20(2).pdf) (retrieved 2026-05-02)
- [MDPI Sensors 24(6):1848 — ISO 26262 / ISO 21434 co-analysis](https://www.mdpi.com/1424-8220/24/6/1848) (retrieved 2026-05-02)

---

## Cluster 10 — Determinism, real-time, side-channel

### Q10.1: WCET of ML-DSA-65 sign

**Question:** "What worst-case execution time did you measure for `Csm_SignatureGenerate` end-to-end (Csm dispatch + CryIf + liboqs ML-DSA sign + return) on the Pi 4? How do you separate WCET from average-case in a Linux environment that has no WCET tooling?"

**Why this matters:** Real-time claims need defensible measurement, not Python timeit.

**Expected/strong answer outline:**
- Likely: 95th-/99th-percentile latency from N runs, isolated CPU (`isolcpus`), `SCHED_FIFO`.
- Acknowledge true WCET on Linux is impossible without static analysis (e.g., aiT) and a non-Linux RTOS.
- ML-DSA timing is data-dependent (rejection sampling) — even bare-metal needs a margin.

**Citations:**
- [Specification of Operating System, AUTOSAR CP R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_OS.pdf) (retrieved 2026-05-02)
- [Specification of Crypto Service Manager, AUTOSAR CP R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_CryptoServiceManager.pdf) (retrieved 2026-05-02)

---

### Q10.2: Is liboqs ML-DSA constant-time and production-ready?

**Question:** "liboqs publicly states it is *not* recommended for production. ML-DSA's rejection-sampling structure is data-dependent. Have you (a) verified the specific liboqs version you ship is the post-2024 ct-fixed branch, and (b) measured timing variance under chosen-message workloads to rule out side-channel leakage on your Pi 4?"

**Why this matters:** PQC + automotive + side-channel is the open research frontier.

**Expected/strong answer outline:**
- liboqs README explicitly disclaims production use.
- A June 2024 release fixed a non-constant-time issue in ML-KEM/Kyber; check release notes.
- Even constant-time C is not constant-time on a CPU with branch predictor + caches.

**Citations:**
- [open-quantum-safe/liboqs — README](https://github.com/open-quantum-safe/liboqs) (retrieved 2026-05-02)
- [open-quantum-safe/liboqs — Releases](https://github.com/open-quantum-safe/liboqs/releases) (retrieved 2026-05-02)
- [Open Quantum Safe — ML-DSA algorithm page](https://openquantumsafe.org/liboqs/algorithms/sig/ml-dsa.html) (retrieved 2026-05-02)

---

### Q10.3: Why ML-DSA-65 (NIST level 3), not 44 or 87?

**Question:** "FIPS 204 defines three parameter sets: ML-DSA-44, ML-65, ML-DSA-87 (NIST levels 2/3/5). You picked ML-DSA-65. Justify the level-3 choice in the *automotive lifetime* context — vehicles run 15-20 years. Is level-3 still adequate against an adversary with quantum capability in 2040?"

**Why this matters:** Forces explicit security-margin reasoning vs vehicle lifetime.

**Expected/strong answer outline:**
- Level 3 ≈ AES-192 classical equivalent; balances signature size (3,309 B vs 4,627 B for level 5) against margin.
- Vehicle on-road life of 15-20 years → cryptographic-agility (re-flashable algorithm) matters more than picking the highest level today.
- ML-DSA-65 is the FIPS 204 default in many deployments.

**Citations:**
- [NIST FIPS 204 (final)](https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.204.pdf) (retrieved 2026-05-02)
- [CSRC — FIPS 204 final landing page](https://csrc.nist.gov/pubs/fips/204/final) (retrieved 2026-05-02)

---

### Q10.4: Why ML-KEM-768 and not 512 or 1024?

**Question:** "Same question for KEM: ML-KEM-768 is NIST level 3. What KEM ciphertext size do you ship over the wire on each rekey, and why did you not pick ML-KEM-1024 given that rekey is rare (every hour)?"

**Why this matters:** Symmetry with Q10.3.

**Expected/strong answer outline:**
- ML-KEM-768 ciphertext = 1,088 B; ML-KEM-1024 = 1,568 B.
- Hourly rekey: bandwidth cost of the larger KEM is negligible.
- Defensible answer: matched ML-KEM-768 to ML-DSA-65 at the same NIST security level.

**Citations:**
- [NIST FIPS 203 (final)](https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.203.pdf) (retrieved 2026-05-02)
- [CSRC — FIPS 203 final landing page](https://csrc.nist.gov/pubs/fips/203/final) (retrieved 2026-05-02)

---

## Cluster 11 — Code-of-conduct deviations and conclusion

### Q11.1: How many SWS clauses fail, in total?

**Question:** "You report '22 of 23 standards clauses PASS'. List the 23 clauses and the one that failed by ID, and explain why the SWS_SecOC_00054 Det issue does not appear there. Are you under-reporting the conformance gap?"

**Why this matters:** Forces an honest conformance scoreboard.

**Expected/strong answer outline:**
- Candidate must enumerate the 23 SWS clauses they evaluated.
- If SWS_SecOC_00054 was outside that set, methodology cherry-picked.
- A defensible answer identifies the failed clause and the selection methodology.

**Citations:**
- [Specification of Secure Onboard Communication, AUTOSAR CP R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_SecureOnboardCommunication.pdf) (retrieved 2026-05-02)

---

### Q11.2: 13 mandatory SecOC entry points — full set?

**Question:** "You claim '13 mandatory SecOC entry points present'. The SWS chapter 8 lists more than 13 service IDs (init, deinit, getversioninfo, mainfunctionrx, mainfunctiontx, ifrxindication, iftriggertransmit, iftxconfirmation, tprxindication, tptxconfirmation, startofreception, copyrxdata, copytxdata, ...). Did you implement all of these including the optional ones? Which ones did you stub?"

**Why this matters:** Probes API completeness vs SWS chapter 8.

**Expected/strong answer outline:**
- Mandatory: `SecOC_Init`, `SecOC_DeInit`, `SecOC_GetVersionInfo`, `SecOC_MainFunctionRx/Tx`, IF/TP indication and confirmation callbacks.
- Optional: `SecOC_VerifyStatusOverride`, `SecOC_GetRxFreshness*`, key-update services.
- Honest answer: list which are stubs.

**Citations:**
- [Specification of Secure Onboard Communication, AUTOSAR CP R21-11 — Chapter 8 API specification](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_SecureOnboardCommunication.pdf) (retrieved 2026-05-02)

---

### Q11.3: Filesystem keys vs UN R155 Annex 5 M21

**Question:** "UN R155 Annex 5 mitigation M21 ('Verify the authenticity and integrity of messages received') and M22 ('Cryptographic keys shall be stored securely') require keys to be in tamper-resistant storage. You store keys in `/etc/secoc/keys/` — flat files on a Linux filesystem. You self-rate this 'PARTIAL'. Walk me through the threat model where a Pi-rooted attacker reads those files and forges signatures: what is the actual residual risk, and what would *FULL* compliance look like?"

**Why this matters:** This is the most important practical-security gap.

**Expected/strong answer outline:**
- Pi root → full key compromise → signature forgery → vehicle-network injection.
- Mitigation paths: (a) HSM/secure-element, (b) Linux TPM2 + sealed keys, (c) ARM TrustZone OP-TEE.
- The HSM_HANDLE provisioning mode is the placeholder for (a).

**Citations:**
- [UN ECE R155 (E), Annex 5](https://unece.org/sites/default/files/2023-02/R155e%20(2).pdf) (retrieved 2026-05-02)
- [Specification of Crypto Driver, AUTOSAR CP R21-11 — HSM-backed key handling](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_CryptoDriver.pdf) (retrieved 2026-05-02)

---

### Q11.4: Closing — what would you take to a Tier-1 design review?

**Question:** "If you walked into a Tier-1 cybersecurity design review tomorrow with this thesis, what are the three things you would change *before* the meeting, and what are the three things you would defend as-is?"

**Why this matters:** Final sanity-check; lets candidate self-rate critical gaps.

**Expected/strong answer outline:**
- Fix-before: gate Det calls on `SECOC_DEV_ERROR_DETECT`; replace filesystem keys with HSM/TPM; move rekey scheduler from SoAd to BswM.
- Defend-as-is: layered Csm/CryIf/Crypto-Driver seam; ML-DSA-65 / ML-KEM-768 parameter choice at NIST level 3; TP-mode handling for large signed PDUs.

**Citations:**
- [Specification of Secure Onboard Communication, AUTOSAR CP R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_SecureOnboardCommunication.pdf) (retrieved 2026-05-02)
- [General Specification of Basic Software Modules, AUTOSAR CP R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_BSWGeneral.pdf) (retrieved 2026-05-02)
