# Professor Examiner #3 — Automotive Cybersecurity, E/E Architecture, Compliance

ISO/SAE 21434, UN R155/R156, ISO 26262, AUTOSAR Adaptive vs Classic, SDV / zonal architectures, type approval.
Citations retrieved 2026-05-03.

---

## Cluster A — ISO/SAE 21434: TARA, CAL, depth of self-assessment

### Q3.1: Where is the documented TARA, and is it Clause 15 conformant?

**Question:** "You claim CAL-1 through CAL-3 PASS. Show me your TARA. ISO/SAE 21434 Clause 15 prescribes a sequence: asset identification, threat scenario identification, impact rating, attack-path analysis, attack feasibility rating, risk determination, risk treatment. Walk me through each step for the Pi 4 gateway."

**Expected/strong answer outline:**
- Clause 15 holds the TARA method; Clause 8 covers continuous activities / vulnerability management.
- Concrete assets: long-term ML-DSA private key, ML-KEM private key, freshness counter state, secure boot keys, Linux rootfs, CAN bus traffic.
- Per-asset SFOP impact (e.g., long-term key compromise = High operational + privacy).
- Map threat scenarios (eavesdrop, inject, modify, replay) to attack paths and rate Attack Feasibility.
- Concede if the TARA is informal and identify what would make it audit-grade.

**Citations:**
- [ISO/SAE 21434:2021](https://www.iso.org/standard/70918.html)
- [TARA in Practice (UL SIS)](https://www.ul.com/sis/resources/threat-analyses-and-risk-assessment-in-practice-tara)

### Q3.2: Attack Feasibility Rating — which method, and why?

**Question:** "Annex G of ISO/SAE 21434 gives three permissible AFR methods: attack-potential (ISO/IEC 18045 style), CVSS-based, or attack-vector-based. Which did you use? Justify it for a Linux-on-COTS-Pi gateway."

**Expected/strong answer outline:**
- Name the chosen method explicitly.
- Map to threat surface: Ethernet exposed, root filesystem readable.
- If attack-potential: score Elapsed Time, Specialist Expertise, Knowledge of TOE, Window of Opportunity, Equipment.
- A Pi 4 in a lab is not representative of attacker access in a deployed vehicle.

**Citations:**
- [Comparative Evaluation of Cybersecurity Methods for AFR](https://www.researchgate.net/publication/339390034_Comparative_Evaluation_of_Cybersecurity_Methods_for_Attack_Feasibility_Rating_per_ISOSAE_DIS_21434)
- [ISO/SAE 21434:2021 §8.6, §8.7](https://www.iso.org/standard/70918.html)

### Q3.3: CAL vs ASIL — be precise

**Question:** "You report CAL levels and have used the phrase 'ASIL D2' once. Define each. They look similar — they are not. Where does 'ASIL D2' appear in any standard?"

**Expected/strong answer outline:**
- ASIL = ISO 26262 functional safety integrity from HARA over Severity/Exposure/Controllability.
- CAL = ISO/SAE 21434 cybersecurity assurance level, derived from impact and attack vector.
- ASIL levels are QM, A, B, C, D plus decomposition like 'ASIL B(D)' — there is no ASIL D2.
- Retract or restate as ASIL B(D) decomposition only if a real decomposition was performed.

**Citations:**
- [The Role of CALs in ISO 21434](https://www.code-intelligence.com/blog/iso21434-cybersecurity-assurance-levels)
- [ISO 26262-9:2018](https://www.iso.org/standard/68391.html)

### Q3.4: CAL-4 PARTIAL — what closes the gap?

**Question:** "CAL-4 is PARTIAL because you have only simulated attacks. Concretely, what would CAL-4 cybersecurity validation per Clause 15 require?"

**Expected/strong answer outline:**
- Independent assessor / accredited body (TÜV SÜD, TÜV NORD, UL SIS, DNV).
- Pen-test scope mirrors operational intended environment (TARA-driven test cases).
- Evidence: test plan, cases traceable to TARA, vulnerability triage, residual-risk acceptance.
- Sign-off by cybersecurity manager + independent confirmation review.

**Citations:**
- [TÜV SÜD ISO/SAE 21434](https://www.tuvsud.com/en-us/services/cyber-security/safety-components)
- [TÜV NORD ISO/SAE 21434](https://www.tuv-nord.com/us/en/services/cybersecurity/iso-sae-21434-cybersecurity-for-road-vehicles-csms/)

### Q3.5: Auditability of the 22/23 PASS claim

**Question:** "Suppose TÜV SÜD shows up Monday to reproduce your 22/23 R155 PASS and CAL-1..3 PASS. Where is the evidence package? Is each test case traceable to a TARA threat ID, a CAL requirement, and an R155 M-ID?"

**Expected/strong answer outline:**
- Traceability matrix: Requirement ID → TARA threat → R155 M-ID → Test case → Evidence.
- Tooling: Polarion/DOORS/Jama, or a complete CSV/markdown table for thesis scope.
- Identify gaps where claim is not yet evidenced.

**Citations:**
- [ISO/SAE 21434:2021 Clause 6](https://www.iso.org/standard/70918.html)

---

## Cluster B — UN ECE R155 Annex 5 and the M-ID question

### Q3.6: Defend the M-ID mapping — start with M21

**Question:** "Your thesis flags M21 as PARTIAL because keys live in `/etc/secoc/keys/`. Cite the canonical Annex 5 mitigation for cryptographic-key storage. Is it M21? In which Part — A (CSMS), B (vehicle), or C (backend)? Quote the text."

**Expected/strong answer outline:**
- Annex 5 has tables A1 (CSMS), B1 (vehicle), C1 (outside vehicle).
- Quote the regulation rather than third-party indexing.
- Concede Annex 5 is "examples of mitigations" — authoritative coverage requires mapping to underlying vulnerabilities + own threat model.
- Remediation: HSM/TEE-backed key storage replaces filesystem files.

**Citations:**
- [UN R155 consolidated PDF](https://unece.org/sites/default/files/2023-02/R155e%20(2).pdf)
- [UN R155 landing page](https://unece.org/transport/documents/2021/03/standards/un-regulation-no-155-cyber-security-and-cyber-security)

### Q3.7: Filesystem keys violate which exact R155 expectation?

**Question:** "Your design ships private keys as files at `/etc/secoc/keys/` with 0600 inside 0700. Walk me through why a Type Approval Authority would reject this."

**Expected/strong answer outline:**
- Mitigation against unauthorized key extraction expects hardware-backed isolation in production.
- POSIX 0600 is defeated by root compromise, kernel exploit, or SD-card extraction.
- Real ECU: AURIX HSM (EVITA Full), TPM 2.0, or ARM TrustZone-backed OP-TEE.
- Be honest that the Pi 4 has no HSM — this is a research demonstrator.

**Citations:**
- [UN R155 Annex 5](https://unece.org/sites/default/files/2023-02/R155e%20(2).pdf)
- [Infineon AURIX TC3xx HSM](https://www.infineon.com/dgdl/Infineon-AURIX_TC3xx_Hardware_Security_Module_Quick-Training-v01_00-EN.pdf?fileId=5546d46274cf54d50174da4ebc3f2265)
- [GlobalPlatform TEE](https://globalplatform.org/specs-library/tee-system-architecture/)

### Q3.8: HSM vs TEE vs SHE — define each

**Question:** "An OEM auditor will ask: do you mean an HSM, a TEE, or SHE? Define each. Which fits a body-domain ECU? A central compute? Which is mandated by Annex 5?"

**Expected/strong answer outline:**
- **SHE** (Secure Hardware Extension): HIS-consortium spec; AES-128 only, on-chip key store; CMAC + Miyaguchi-Preneel; no asymmetric.
- **HSM** (e.g., EVITA Full / AURIX TC3xx HSM): Full crypto core, ECC/RSA, TRNG, secure boot anchor.
- **TEE** (ARM TrustZone + OP-TEE per GlobalPlatform): Software isolation in a separate execution environment on the same SoC.
- Annex 5 does not mandate technology — it mandates protections; OEM TARA justifies which.
- Map: body ECU → SHE; gateway/V2X → HSM; SDV/HPC → TEE+HSM hybrid.

**Citations:**
- [EVITA HSM publication](https://evita-project.org/Publications/WG11.pdf)
- [AUTOSAR Crypto Driver R23-11](https://www.autosar.org/fileadmin/standards/R23-11/CP/AUTOSAR_CP_SWS_CryptoDriver.pdf)
- [GlobalPlatform TEE](https://globalplatform.org/specs-library/tee-system-architecture/)

### Q3.9: R155 covers 70+ vulnerabilities — what did you NOT cover?

**Question:** "Annex 5 lists threats organised by categories: communication channels, update process, unintended human actions, external connectivity, targets, vulnerabilities, data loss/breach, physical manipulation. You claim 22/23. Which mitigations did you NOT address?"

**Expected/strong answer outline:**
- Define explicit scope: in-session communication on the Ethernet leg.
- Out of scope (legitimate): backend/CSMS controls, OTA process (R156 territory), physical manipulation, supply chain, decommissioning.
- Provide explicit M-ID to test-case mapping with partials clearly marked.
- Distinguish "tested PASS" from "argued PASS by design".

**Citations:**
- [UN R155 Annex 5](https://unece.org/sites/default/files/2023-02/R155e%20(2).pdf)

---

## Cluster C — UN R156: Software Updates and SUMS

### Q3.10: Where does R156 fit, and is your gateway compatible?

**Question:** "Type approval requires both R155 and R156 since 2022. Your thesis is silent on R156. Walk me through SUMS: RXSWIN, secure update channel, integrity verification at the vehicle, rollback. Is your boot-time key provisioning compatible with R156?"

**Expected/strong answer outline:**
- R156 requires SUMS (process) plus per-vehicle technical capability (secure transfer, integrity, version logs, rollback).
- RXSWIN: regulatory software identification number for type-approval-relevant software.
- Concede the thesis does not implement OTA.
- Sketch minimum support: signed update bundles using ML-DSA, dual-bank A/B partitions, manifest+signature verification at boot, rollback on integrity failure.

**Citations:**
- [UN R156 consolidated](https://unece.org/sites/default/files/2024-03/R156e%20(2).pdf)
- [UN R156 landing page](https://unece.org/transport/documents/2021/03/standards/un-regulation-no-156-software-update-and-software-update)

### Q3.11: NIST SP 800-193 — does your boot story comply?

**Question:** "If R156 mandates secure update, NIST SP 800-193 is the de-facto reference for firmware resiliency: Protect, Detect, Recover. Does your Pi 4 satisfy any of the three?"

**Expected/strong answer outline:**
- SP 800-193: RoT for Update, Detection, Recovery — none present on stock Pi 4.
- Pi 4 boot ROM verifies the EEPROM bootloader (since 2020) but not all stages downstream.
- Concede Pi 4 is unsuitable as a production secure-boot anchor.

**Citations:**
- [NIST SP 800-193](https://csrc.nist.gov/pubs/sp/800/193/final)

---

## Cluster D — ISO 26262 absence and safety/security interaction

### Q3.12: How can a security-gateway thesis skip ISO 26262 entirely?

**Question:** "ISO 26262-2 explicitly requires consideration of the interaction between safety and security. Your gateway forwards CAN frames that may carry ASIL-rated signals. If your gateway delays or drops a frame because PQC verification took 370 µs, you have just affected functional safety. Defend skipping HARA."

**Expected/strong answer outline:**
- Acknowledge ISO 26262-2 §5.4.2 requires identifying interactions with cybersecurity.
- Concede a full HARA is out of scope; propose minimum: identify whether any forwarded signal is safety-relevant.
- Tie the 5 ms latency budget to a real source (chassis control loops ~10 ms, ASIL-D powertrain ~5 ms).
- Retract "ASIL D2" — there is no such level.

**Citations:**
- [ISO 26262-1:2018](https://www.iso.org/standard/68383.html)
- [ISO 26262-9:2018](https://www.iso.org/standard/68391.html)

### Q3.13: HAZOP / HARA — sketch one now

**Question:** "On the whiteboard: a HARA for the gateway as a 'forwarding ECU between CAN and Ethernet'. Identify one hazardous event, the operational situation, S/E/C ratings, and the resulting ASIL."

**Expected/strong answer outline:**
- Hazard: gateway forwards stale or corrupted brake-related signal.
- Operational situation: vehicle in motion, driver demanding brake.
- Severity: S3. Exposure: E4. Controllability: C3. → ASIL D.
- Safety goal: gateway shall fail-safe within X ms.

**Citations:**
- [ISO 26262-1:2018](https://www.iso.org/standard/68383.html)
- [ISO 26262 series](https://www.iso.org/publication/PUB200262.html)

---

## Cluster E — E/E architecture and the SDV trend

### Q3.14: Why a centralized gateway and not zonal HPC?

**Question:** "Tesla shipped a centralized vehicle computer in 2017. VW E3 1.2, BMW iX iDrive 8, and Bosch zonal-architecture roadmaps move the gateway function into central HPC + zonal ECUs. Why is your design — a single Pi 4 gateway — relevant to a 2027+ vehicle?"

**Expected/strong answer outline:**
- Acknowledge trend: domain → cross-domain → zonal + HPC.
- Defend the abstraction: SecOC and PQC primitives still apply at zonal-to-HPC link or HPC-to-HPC link.
- Pi 4 is a stand-in for an HPC running Linux/QNX, not a stand-in for an MCU.
- Limitation: zonal ECUs need PQC-aware HSM at the zone — exactly the AURIX/Renesas roadmap.

**Citations:**
- [Bosch E/E architecture page](https://www.bosch-mobility.com/en/mobility-topics/ee-architecture/)
- [Bosch zonal whitepaper](https://www.bosch-mobility.com/media/global/mobility-topics/connected-mobility/iot-device-on-wheels/e-e-architecture/whitepaper/230831-bosch-xc-whitepaper-ee-architektur-en.pdf)

### Q3.15: SOME/IP-SD over TLS vs SecOC — what's the future?

**Question:** "Adaptive AUTOSAR uses SOME/IP-SD over UDP/TCP, typically with TLS. SecOC was a Classic-AUTOSAR answer to bandwidth-constrained CAN. On a 100 Mbps Ethernet leg, TLS 1.3 with hybrid PQC ciphersuites would do everything SecOC does. Why is SecOC still right in 2026, on Ethernet?"

**Expected/strong answer outline:**
- SecOC operates at PDU layer, not socket layer; preserves PDU framing and freshness independent of transport.
- AUTOSAR Classic + Ethernet PDUs over UDP often skip TLS to keep the PDU model intact.
- Adaptive uses TLS for SOME/IP, but interop with Classic ECUs (the majority) keeps SecOC alive at the gateway boundary.
- Hybrid future: TLS 1.3 for SOA, SecOC for legacy Classic PDUs forwarded across the gateway. Both will coexist 10+ years.

**Citations:**
- [AUTOSAR SOME/IP Protocol R24-11](https://www.autosar.org/fileadmin/standards/R24-11/FO/AUTOSAR_FO_PRS_SOMEIPProtocol.pdf)
- [AUTOSAR SecOC Protocol R23-11](https://www.autosar.org/fileadmin/standards/R23-11/FO/AUTOSAR_FO_PRS_SecOcProtocol.pdf)
- [AUTOSAR Adaptive](https://www.autosar.org/standards/adaptive-platform)
- [AUTOSAR R25-11 release](https://www.autosar.org/news-events/detail/release-r25-11-is-now-available)

### Q3.16: Pi 4 representativeness — Cortex-A72 vs Cortex-R52

**Question:** "Your latency numbers come from a Cortex-A72 at 1.5 GHz running Linux. A typical automotive gateway MCU is a Cortex-R52 at 200-400 MHz running an RTOS or AUTOSAR Classic. Are your benchmarks transferable?"

**Expected/strong answer outline:**
- Cortex-A72: out-of-order superscalar, ARMv8-A with NEON; SIMD helps lattice multiplications.
- Cortex-R52: in-order, lockstep-capable for ASIL-D; no NEON in lockstep mode; smaller caches.
- Realistic projection: ML-DSA-65 sign on R52 may be 5-10x slower; verify 3-5x slower.
- Mitigation: hardware acceleration (AURIX HSM, dedicated PQC accelerator), or offload to HPC.

**Citations:**
- [Arm Cortex-A Comparison Table](https://www.arm.com/-/media/Arm%20Developer%20Community/PDF/Cortex-A%20R%20M%20datasheets/Arm%20Cortex-A%20Comparison%20Table_v4.pdf)
- [Arm Cortex-R Comparison Table](https://www.arm.com/-/media/Arm%20Developer%20Community/PDF/Cortex-A%20R%20M%20datasheets/Arm%20Cortex-R%20Comparison%20Table.pdf)

---

## Cluster F — NIST IR 8547 timeline and migration realism

### Q3.17: Map IR 8547 dates to a 2027 SOP vehicle

**Question:** "NIST IR 8547 sets 2030 deprecation for ~112-bit security (ECDSA P-256, RSA-2048) and 2035 disallowance for all quantum-vulnerable public-key. NSM-10 sets 2035 for federal systems. A vehicle with SOP 2027 and 15-year service life is on the road until 2042. Walk me through the migration policy."

**Expected/strong answer outline:**
- 2027 SOP: hybrid (classical + PQC) crypto so it interoperates with existing PKI.
- 2030: PKI must transition off RSA-2048 / ECDSA P-256 — requires OTA-driven key rotation.
- 2035: pure PQC; last classical certificate must expire or be re-issued PQC.
- 2042 EOL: continue receiving PQC-signed updates.

**Citations:**
- [NIST IR 8547 ipd](https://csrc.nist.gov/pubs/ir/8547/ipd)
- [NSM-10](https://bidenwhitehouse.archives.gov/briefing-room/statements-releases/2022/05/04/national-security-memorandum-on-promoting-united-states-leadership-in-quantum-computing-while-mitigating-risks-to-vulnerable-cryptographic-systems/)

### Q3.18: HNDL — vehicle threat or slogan?

**Question:** "HNDL is the standard PQC justification. For a vehicle, what payloads on the in-vehicle bus are confidentiality-critical 15 years out? CAN body-control frames? Probably not. Be specific about WHERE in the vehicle HNDL bites."

**Expected/strong answer outline:**
- HNDL targets confidentiality — recorded ciphertext decrypted later.
- SecOC is signing/MAC, not encryption — HNDL does not directly threaten SecOC integrity.
- Real HNDL targets: V2X pseudonym certificates and ECDH ephemerals, telematics TLS sessions (PII), OTA package encryption keys, occupant privacy data.
- The right argument for SecOC is: a CRQC can forge signatures on an old long-term key, so signature schemes must be PQC before CRQC arrives.

**Citations:**
- [ENISA Post-Quantum Cryptography](https://www.enisa.europa.eu/publications/post-quantum-cryptography-current-state-and-quantum-mitigation)
- [CISA/NSA/NIST Quantum-Readiness Factsheet](https://www.cisa.gov/sites/default/files/2023-08/Quantum%20Readiness_Final_CLEAR_508c%20(3).pdf)

---

## Cluster G — Threat model, key management, pen testing

### Q3.19: Excluding physical attacks — defensible?

**Question:** "Your adversary model excludes physical attacks and side channels. Real gateways sit behind OBD-II, the IVI, the head unit, and increasingly USB-C. Physical access is realistic for a parked vehicle. Defend the exclusion or restrict your claim."

**Expected/strong answer outline:**
- Acknowledge the exclusion is research-scope.
- R155 Annex 5 Part B explicitly addresses physical manipulation — cannot claim 22/23 if physical was out of scope.
- Concrete physical attacks on a Pi 4: SD-card extraction, JTAG/SWD on GPIO, UART console, OBD-II tester pivot.
- Production answer: HSM + sealed enclosure + tamper response.

**Citations:**
- [UN R155 Annex 5](https://unece.org/sites/default/files/2023-02/R155e%20(2).pdf)

### Q3.20: Side-channel attacks on ML-KEM / ML-DSA — known concerns?

**Question:** "ML-KEM and ML-DSA have published side-channel papers — timing leaks in rejection sampling, power-analysis on NTT. On a Pi 4 with no constant-time guarantees in liboqs by default, are your private keys actually safe from a co-located attacker?"

**Expected/strong answer outline:**
- Acknowledge documented timing/power side-channel literature.
- liboqs has constant-time variants but defaults vary; Pi 4 has no isolated execution.
- Production: hardened HSM with masked implementations.

**Citations:**
- [ENISA PQC Integration Study](https://www.enisa.europa.eu/publications/post-quantum-cryptography-integration-study)
- [FIPS 204](https://csrc.nist.gov/pubs/fips/204/final)

### Q3.21: ML-KEM handshake unauthenticated — critical limitation

**Question:** "You acknowledge the ML-KEM handshake is unauthenticated. So an active MITM can substitute their own ML-KEM public key and complete the KEM with both peers. Your downstream SecOC then runs over a session key the attacker knows. Why is this acceptable, even as research?"

**Expected/strong answer outline:**
- Confirm need for out-of-band authentication of the static public key.
- Production answer: certificate chain rooted at OEM CA, ML-DSA-signed; gateway verifies peer's ML-KEM public key via the certificate before encapsulation.
- Without this, the chain reduces to "trusted network" — unacceptable in a vehicle.

**Citations:**
- [FIPS 203](https://csrc.nist.gov/pubs/fips/203/final)
- [FIPS 204](https://csrc.nist.gov/pubs/fips/204/final)

### Q3.22: Key lifecycle — beyond "rekey every hour"

**Question:** "You re-key the session every hour — a session key. NIST SP 800-57 Pt 1 Rev 5 distinguishes lifecycle phases. Walk me through generation, distribution, storage, use, archival, revocation, and destruction for your *long-term ML-DSA identity key*. What's the cryptoperiod?"

**Expected/strong answer outline:**
- Generation: factory, in HSM, never extractable.
- Distribution: never — public key signed into device certificate at provisioning.
- Storage: HSM-backed, wrapped if mirrored.
- Use: signing only, rate-limited, audit-logged.
- Cryptoperiod: SP 800-57 typically 1-3 years for signing keys; for an automotive ECU bound to hardware, vehicle EOL or known compromise.
- Revocation: CRL-equivalent via R156 OTA.
- Destruction: zeroization on decommission per SP 800-88.

**Citations:**
- [NIST SP 800-57 Part 1 Rev 5](https://csrc.nist.gov/pubs/sp/800/57/pt1/r5/final)

### Q3.23: Penetration testing — what would a real engagement cover?

**Question:** "Describe an automotive-gateway pen-test engagement at CAL-3/4 depth: scope, methodology, tooling, deliverables. Map to OWASP IoT Security Testing Guide (ISTG) and ISO/SAE 21434 §15."

**Expected/strong answer outline:**
- Methodology: ISTG framework + 21434 §15 cybersecurity validation, plus TARA-driven test cases.
- Tooling: CANalyzer/CANoe, Wireshark, scapy, Burp on Ethernet, JTAG/UART probes, glitching for HSM.
- Deliverables: test report, CVSS-rated findings, retest evidence, residual-risk acceptance.
- Independence: tester not part of dev team; ideally external lab.

**Citations:**
- [OWASP IoT Security Testing Guide](https://owasp.org/owasp-istg/)
- [ISO/SAE 21434:2021 §15](https://www.iso.org/standard/70918.html)

---

## Cluster H — Type approval, R155/R156 in EU regulatory chain

### Q3.24: R155 to type approval — who certifies what, when?

**Question:** "Walk me through type approval. The OEM has a CSMS certificate from a TAA. They submit a vehicle type for VTA. Where does your gateway implementation fit in the OEM's evidence package, and which Tier-1 owns the cybersecurity-relevant evidence?"

**Expected/strong answer outline:**
- 1958 Agreement → WP.29 → GRVA drafts regulations → contracting parties type-approve.
- R155 dual approval: CSMS approval (process) + Vehicle Type Approval (product).
- The gateway is a Tier-1 component; Tier-1 provides cybersecurity case to OEM.
- Since July 2024 in EU: all new vehicles sold need R155/R156 compliance.
- Thesis sits at Tier-1 / research demonstrator level.

**Citations:**
- [UNECE WP.29](https://unece.org/wp29-introduction)
- [UNECE GRVA](https://unece.org/transport/road-transport/working-party-automatedautonomous-and-connected-vehicles-introduction)

### Q3.25: ASPICE CL1 — what does that actually mean here?

**Question:** "You self-assess ASPICE 3.1 CL1. CL1 means 'Performed Process' — the lowest meaningful level. What ASPICE base practices did you actually demonstrate, and what would CL2 (Managed Process) require?"

**Expected/strong answer outline:**
- CL1 = process performed and produces work products — not managed, controlled, or measured.
- CL2 requires planning, monitoring, controlling work products.
- ASPICE is process-side; ISO 21434 is product/cyber side. Complementary.
- OEMs require ASPICE CL2/CL3 from Tier-1s alongside 21434.

**Citations:**
- [Automotive SPICE — VDA QMC](https://vda-qmc.de/en/automotive-spice/)

### Q3.26: AUTOSAR R25-11 — what new PQC-relevant content?

**Question:** "AUTOSAR R25-11 was released November 2025. The thesis cites it. What in R25-11 is directly relevant to PQC-on-SecOC?"

**Expected/strong answer outline:**
- R25-11 release Dec 4, 2025; release dated Nov 27, 2025; ~1900 incorporation tasks.
- R25-11 highlights: Vehicle Data Protocol, DDS support on CP, security improvements.
- Concede whether the thesis pre-dates R25-11 PQC-relevant CryptoStack changes — frame work as forward-compatible.

**Citations:**
- [AUTOSAR R25-11 release](https://www.autosar.org/news-events/detail/release-r25-11-is-now-available)
- [AUTOSAR Crypto Driver R23-11](https://www.autosar.org/fileadmin/standards/R23-11/CP/AUTOSAR_CP_SWS_CryptoDriver.pdf)
