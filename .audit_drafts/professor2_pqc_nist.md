# Professor Examiner #2 — Post-Quantum Cryptography & NIST Standards

Thesis: "AUTOSAR SecOC with Post-Quantum Cryptography on a Raspberry Pi 4 Ethernet Gateway"
Citations retrieved 2026-05-02.

---

## Cluster 1 — NIST PQC standardization process & timeline

### Q2.1: Why August 13, 2024, and what does "finalised" mean?

**Question:** "You write that ML-KEM and ML-DSA were *finalised* on 13 Aug 2024. Walk me through the standardization timeline from the 2016 call for proposals to that date — Round 1, 2, 3 and the July 2022 selection. Why did NIST take a year and a half *after* selection in July 2022 to publish FIPS 203/204/205?"

**Why this matters:** The candidate uses these standards as the core security argument. They must understand the process is ongoing, not a one-shot stamp.

**Expected/strong answer outline:**
- 2016 call for proposals; 69 round-1 submissions; rounds 2 (2019) and 3 (2020).
- July 2022: Kyber (KEM) + Dilithium, Falcon, SPHINCS+ (signatures) selected; round 4 continues for code-based KEMs.
- 2022-2024 was the standards-drafting phase (initial public drafts ipd in Aug 2023; comments; FIPS finalisation 13 Aug 2024).
- "Final" means published in the Federal Register; it does NOT mean CMVP-validated implementations exist.
- Should reference NIST IR 8413 (third-round status report).

**Citations:**
- [NIST Releases First 3 Finalized Post-Quantum Encryption Standards](https://www.nist.gov/news-events/news/2024/08/nist-releases-first-3-finalized-post-quantum-encryption-standards)
- [PQC Standardization Process: Announcing Four Candidates](https://www.nist.gov/news-events/news/2022/07/pqc-standardization-process-announcing-four-candidates-be-standardized-plus)
- [NIST IR 8413, Status Report on the Third Round](https://nvlpubs.nist.gov/nistpubs/ir/2022/NIST.IR.8413.pdf)

### Q2.2: Why Kyber/Dilithium and not NTRU or Saber?

**Question:** "Round 3 finalists included NTRU and Saber, both lattice-based KEMs with arguably nicer characteristics. Why did NIST pick Kyber and not NTRU? Why is your thesis tied to a Module-LWE construction rather than NTRU's older, more-studied lattice problem?"

**Expected/strong answer outline:**
- Kyber: Module-LWE, balanced sizes/perf, IP licensing cleared with the patent settlement.
- NTRU: oldest, no patents (post 2017), but slightly larger keys; NIST chose against incumbency to avoid IP risk and went with the Module-LWE family.
- Saber: Module-LWR; very fast but less analysis history than MLWE.
- See NIST IR 8413 §3 for the rationale matrix.

**Citations:**
- [NIST IR 8413 §3](https://nvlpubs.nist.gov/nistpubs/ir/2022/NIST.IR.8413.pdf)
- [Kyber paper, IACR ePrint 2017/634](https://eprint.iacr.org/2017/634)

### Q2.3: Why is FIPS 205 (SLH-DSA) called a "backup"?

**Question:** "FIPS 205 SLH-DSA is hash-based and was finalised the same day. NIST IR 8413 explicitly calls it a hedge. If a flaw is found in Module-LWE tomorrow, would your gateway be able to fall back to SLH-DSA? What in your design prevents that?"

**Expected/strong answer outline:**
- SLH-DSA = stateless hash-based; security rests only on hash preimage/collision resistance — orthogonal assumption.
- NIST keeps it precisely because lattice cryptanalysis could improve.
- SLH-DSA signatures are 7,856–49,856 B; verify is faster than sign.
- Candidate's design hard-codes ML-DSA-65 in CSM; algorithm agility is missing.

**Citations:**
- [FIPS 205 PDF](https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.205.pdf)
- [NIST IR 8413 §4](https://nvlpubs.nist.gov/nistpubs/ir/2022/NIST.IR.8413.pdf)

### Q2.4: Round 4 and the HQC selection — does it affect your design?

**Question:** "In March 2025 NIST announced HQC as the second standardised KEM (NIST IR 8545). Were you aware of this when you froze your design? Should an automotive gateway carry a code-based KEM as a hedge, given that lattice-based MLWE could be broken by an algorithmic advance?"

**Expected/strong answer outline:**
- HQC: code-based KEM, different mathematical assumption than ML-KEM (Hamming Quasi-Cyclic).
- NIST IR 8545 (March 2025) selected HQC as second KEM in case of MLWE break.
- Trade-off: HQC keys/ciphertexts are larger.
- Candidate should acknowledge no agility in current design; future work should support hybrid lattice + code.

**Citations:**
- [NIST IR 8545](https://nvlpubs.nist.gov/nistpubs/ir/2025/NIST.IR.8545.pdf)
- [NIST HQC announcement](https://www.nist.gov/news-events/news/2025/03/nist-pqc-standardization-process-hqc-announced-4th-round-selection)

---

## Cluster 2 — FIPS 203 internals (ML-KEM-768)

### Q2.5: Walk me through ML-KEM-768 KeyGen, Encaps, Decaps

**Question:** "On a whiteboard. What does `ML-KEM.KeyGen()` actually output? What does `Encaps(ek)` produce, mathematically? Why is the K-PKE inner CPA-secure scheme wrapped with the FO-transform variant in §7.3?"

**Expected/strong answer outline:**
- KeyGen: sample matrix A from seed ρ, secret s and error e; pk = (A, t = A·s + e), sk = s. (FIPS 203 §7.1, Algorithm 16)
- Encaps: derive m, K, r; compute c = K-PKE.Encrypt(pk, m, r); shared key K' from G(m‖H(pk)). (§7.2, Algorithm 17)
- Decaps: implicit-rejection FO transform. (§7.3, Algorithm 18)
- Module-LWE: q=3329, n=256, k=3 for ML-KEM-768. Public key 1,184 B, ciphertext 1,088 B, shared secret 32 B.

**Citations:**
- [FIPS 203 §7.1–7.3, §8](https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.203.pdf)
- [Kyber paper IACR ePrint 2017/634](https://eprint.iacr.org/2017/634.pdf)

### Q2.6: ML-KEM is IND-CCA but the *handshake using it* is not authenticated

**Question:** "FIPS 203 §3.3 says ML-KEM provides IND-CCA2. Your thesis says 'thus our session key is secure.' But IND-CCA is a property of the KEM against a *passive observer* of the public key. Define IND-CCA precisely and explain why it is *not* the same as authenticated key exchange."

**Expected/strong answer outline:**
- IND-CCA2: adversary cannot distinguish a real shared secret from random *given* a decapsulation oracle, except for the challenge ciphertext.
- It assumes the public key is *known and trusted* by the receiver.
- Provides confidentiality & integrity of the encapsulation, NOT identity binding.
- Active MitM with own pk substitution defeats KEM-only handshake.

**Citations:**
- [FIPS 203 §3.3](https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.203.pdf)
- [NIST SP 800-227 §4](https://nvlpubs.nist.gov/nistpubs/SpecialPublications/NIST.SP.800-227.pdf)

---

## Cluster 3 — FIPS 204 internals (ML-DSA-65)

### Q2.7: Fiat-Shamir with Aborts — explain the abort condition

**Question:** "ML-DSA-65 signing in FIPS 204 §5 is essentially Fiat-Shamir with rejection sampling. Why does the signer abort, and what is the *concrete* probability of abort per iteration for ML-DSA-65? What invariant would be violated if you skipped the rejection check?"

**Expected/strong answer outline:**
- Reject if `‖z‖∞ ≥ γ1 - β` or `‖LowBits(w - cs2)‖∞ ≥ γ2 - β` (FIPS 204 Algorithm 7).
- Rejection makes the signature distribution *independent* of the secret key — without it, signatures leak `s1, s2` after enough observations.
- Average abort rate ≈ 4–7 iterations for ML-DSA-65.
- Timing variation across abort iterations is itself a side channel.

**Citations:**
- [FIPS 204 §5](https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.204.pdf)
- [Dilithium paper IACR ePrint 2017/633](https://eprint.iacr.org/2017/633.pdf)

### Q2.8: Why is the ML-DSA-65 signature exactly 3,309 bytes?

**Question:** "Sketch how 3,309 bytes is composed: how many bytes for c̃, z, and h respectively at parameter set ML-DSA-65? Could you compress the signature without breaking EUF-CMA?"

**Expected/strong answer outline:**
- Signature = (c̃, z, h). For ML-DSA-65: c̃ length is from λ-based domain separation; z encoded with γ1-bit packing; h hint vector packed with ω+k bytes.
- 3,309 B is given in FIPS 204 §4 Table 2.
- Custom compression voids FIPS conformance and breaks verifier interop.

**Citations:**
- [FIPS 204 §4 Table 2](https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.204.pdf)

### Q2.9: EUF-CMA reduction to MLWE + MSIS

**Question:** "Dilithium's security claim is EUF-CMA in the (Q)ROM, reducing to MLWE for hiding and MSIS for binding. Sketch the reduction. What changes if your randomness source has only 64 bits of entropy?"

**Expected/strong answer outline:**
- MLWE: pk = A·s1 + s2 indistinguishable from random.
- MSIS: forging a signature implies finding a short non-zero solution.
- ROM/QROM reduction: Dilithium paper §6 + Kiltz-Lyubashevsky-Schaffner.
- Low-entropy r leads to s1, s2 leakage → key recovery.

**Citations:**
- [Dilithium paper §6](https://eprint.iacr.org/2017/633.pdf)
- [QROM Fiat-Shamir treatment ePrint 2017/916](https://eprint.iacr.org/2017/916)

---

## Cluster 4 — Why security level 3 (ML-KEM-768 / ML-DSA-65)?

### Q2.10: Why not level 5 for a 20-year vehicle lifetime?

**Question:** "A car shipped in 2026 may still be on the road in 2046. NIST Categories are 1, 3, 5 (≈ AES-128/192/256). Why did you pick *Category 3* and not Category 5 for a quantum-attack horizon you can't bound? Did you compute X+Y vs Z (Mosca's framework)?"

**Expected/strong answer outline:**
- NIST Categories anchored to AES-128/192/256 brute-force cost.
- Performance: ML-DSA-87 sign is slower; signature 4,627 B.
- Mosca: if X (migration) + Y (data lifetime ~20 yr) > Z (CRQC arrival), pick higher category.
- NIST IR 8547 recommends migration well before 2035.

**Citations:**
- [NIST IR 8547 ipd §3, §4](https://nvlpubs.nist.gov/nistpubs/ir/2024/NIST.IR.8547.ipd.pdf)
- [NIST PQC Security Categories](https://csrc.nist.gov/projects/post-quantum-cryptography/post-quantum-cryptography-standardization/evaluation-criteria/security-(evaluation-criteria))
- [Gidney & Ekerå arXiv:1905.09749](https://arxiv.org/abs/1905.09749)

### Q2.11: Why not level 1 (ML-KEM-512 / ML-DSA-44) for performance?

**Question:** "ML-DSA-44 signatures are 2,420 B and signing is faster. For high-rate ADAS messages on CAN-TP, every byte and µs matters. Why isn't level 1 sufficient? What threat model rules it out?"

**Expected/strong answer outline:**
- Level 1 ≈ AES-128 ≈ ~64 bits post-quantum (Grover).
- Acceptable for ephemeral session keys, marginal for long-lifetime automotive.
- For broadcast V2X with capable RSUs, level 1 is too aggressive.

**Citations:**
- [FIPS 204 §4](https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.204.pdf)
- [NIST IR 8547 §3](https://nvlpubs.nist.gov/nistpubs/ir/2024/NIST.IR.8547.ipd.pdf)

---

## Cluster 5 — Side channels & implementation security

### Q2.12: Is liboqs's ML-DSA constant-time? Did you measure?

**Question:** "Open Quantum Safe themselves say liboqs is for *prototyping*. The reference ML-DSA implementation has rejection-sampling loops. Did you instrument `dudect` on your Pi 4 binary? Has the rejection loop in `OQS_SIG_ml_dsa_65_sign` been verified constant-time on Cortex-A72?"

**Expected/strong answer outline:**
- liboqs FAQ explicitly warns against production use.
- ML-DSA rejection loop iteration count leaks unless masked.
- mlkem-native (in 0.15.0) has formal proofs; ML-DSA in liboqs does not.
- Honest answer: candidate did not measure.

**Citations:**
- [Open Quantum Safe FAQ](https://openquantumsafe.org/faq.html)
- [OQS ML-DSA algorithm page](https://openquantumsafe.org/liboqs/algorithms/sig/ml-dsa.html)
- [Liboqs 0.15.0 release notes](https://github.com/open-quantum-safe/liboqs/releases/tag/0.15.0)

### Q2.13: Power analysis on your Pi 4

**Question:** "Recent CPA attacks on hardware ML-DSA implementations recover the secret key from a few hundred traces (ePrint 2025/009; ePrint 2025/582). What countermeasures — masking, blinding, fault detection — does your design include?"

**Expected/strong answer outline:**
- Acknowledge attack surface: tow truck / dealership / supply chain.
- Liboqs has no masking countermeasures for ML-DSA.
- Mitigation: HSM/SE-backed signing.

**Citations:**
- [CPA Attack on ML-DSA Hardware ePrint 2025/009](https://eprint.iacr.org/2025/009.pdf)
- [ML-DSA SCA via rejected signatures ePrint 2025/582](https://eprint.iacr.org/2025/582)

### Q2.14: Why didn't you pick FN-DSA (Falcon)?

**Question:** "FN-DSA / Falcon would give you smaller signatures. What is the floating-point side-channel issue and why is ML-DSA *easier* to implement securely?"

**Expected/strong answer outline:**
- Falcon uses IEEE-754 doubles in FFT-based Gaussian sampling.
- FP side channels: Karabulut-Aysu DAC'21 "Falcon Down".
- Constant-time FP arithmetic is not portable; integer emulation is slow.
- ML-DSA uses integer arithmetic mod q=8380417 — easier to mask.

**Citations:**
- [Falcon Down ePrint 2021/772](https://eprint.iacr.org/2021/772)
- [Improved Power Analysis Attacks on Falcon ePrint 2023/224](https://eprint.iacr.org/2023/224.pdf)

---

## Cluster 6 — Randomness sources

### Q2.15: Where does your entropy come from on Pi 4?

**Question:** "FIPS 203 §3.3 requires a NIST SP 800-90A/B/C-compliant RBG. Where does liboqs get its entropy on Raspberry Pi 4? Is the Linux CSPRNG a SP 800-90A-approved DRBG?"

**Expected/strong answer outline:**
- liboqs default `OQS_randombytes_system` calls `getrandom(2)` on Linux.
- `getrandom()` reads from urandom (ChaCha20-based since 4.8).
- Linux CSPRNG is *not* CMVP/SP 800-90A validated by default.
- Candidate must articulate path to FIPS-validated RBG.

**Citations:**
- [FIPS 203 §3.3](https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.203.pdf)
- [NIST SP 800-90A Rev 1](https://nvlpubs.nist.gov/nistpubs/specialpublications/nist.sp.800-90ar1.pdf)
- [Linux getrandom(2)](https://man7.org/linux/man-pages/man2/getrandom.2.html)

### Q2.16: Randomized vs deterministic ML-DSA

**Question:** "FIPS 204 §3.7 lets you pick deterministic (rnd = 0^256) or hedged. You chose hedged. Walk me through why deterministic ML-DSA is *worse* for an automotive ECU exposed to fault injection."

**Expected/strong answer outline:**
- FIPS 204 §3.7: hedged is the default because deterministic ML-DSA is broken under differential fault attacks.
- Hedged adds 256 bits of fresh entropy mixed into the commitment.
- Cars are exactly such platforms (clock/voltage glitching).

**Citations:**
- [FIPS 204 §3.7](https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.204.pdf)
- [Fault-Injection Security of ML-DSA ePrint 2025/904](https://eprint.iacr.org/2025/904.pdf)

---

## Cluster 7 — Hybrid mode terminology

### Q2.17: "Hybrid" — your definition vs the IETF/NIST definition

**Question:** "Your thesis says 'Hybrid Mode = HMAC + ML-DSA.' The cryptographic community uses 'hybrid' for a *KEM combiner* such as X25519 + ML-KEM-768 — see draft-ietf-tls-hybrid-design and draft-ietf-tls-ecdhe-mlkem. Defend your naming."

**Expected/strong answer outline:**
- Acknowledge naming collision; better term would be "dual-authentication mode".
- IETF hybrid is for *defense in depth against PQ broken*: combine classical KEM + PQ KEM via concatenation KDF.
- Candidate's "hybrid" gives no extra security against quantum.
- Why not real hybrid KEM: scope; AUTOSAR CSM lacks combiner primitive.

**Citations:**
- [draft-ietf-tls-hybrid-design](https://datatracker.ietf.org/doc/html/draft-ietf-tls-hybrid-design)
- [draft-ietf-tls-ecdhe-mlkem](https://datatracker.ietf.org/doc/draft-ietf-tls-ecdhe-mlkem/)
- [NIST SP 800-56C Rev 2 §2](https://nvlpubs.nist.gov/nistpubs/SpecialPublications/NIST.SP.800-56Cr2.pdf)

---

## Cluster 8 — Unauthenticated KEM handshake (the hardest question)

### Q2.18: Why ship an admitted MitM-vulnerable handshake?

**Question:** "You write 'an active network adversary can substitute the public key during handshake and become MitM.' This is not minor — it makes your post-quantum claim *moot* in the presence of an active attacker. Why is this in your thesis and not in your future work?"

**Expected/strong answer outline:**
- Be honest: prototype-scope; pre-shared identity keys not yet provisioned.
- Compare with ECU manufacturing reality: identity keys must be burnt at OEM (TPM/SE).
- Right answer is sigma-style: ML-KEM + ML-DSA static identity over the transcript.
- SP 800-56C Rev 2 §5 mandates authentication for key-establishment.

**Citations:**
- [Krawczyk SIGMA paper](https://webee.technion.ac.il/~hugo/sigma-pdf.pdf)
- [NIST SP 800-56C Rev 2 §5](https://nvlpubs.nist.gov/nistpubs/SpecialPublications/NIST.SP.800-56Cr2.pdf)

### Q2.19: Why not just sign the KEM transcript?

**Question:** "You already have ML-DSA-65 in the system. Why not simply sign `(ek_A, ct_B, transcript_hash)` with a static long-term ML-DSA identity key on each side?"

**Expected/strong answer outline:**
- Yes — this gives a SIGMA-like authenticated KEM at near-zero marginal cost (1 sign + 1 verify per ~1-hour rekey).
- Identity key distribution: PKI rooted at OEM; pubkey hash burnt into ECU at flash time.
- This is the future work plan; thesis stops one step short.

**Citations:**
- [Krawczyk SIGMA paper §5](https://webee.technion.ac.il/~hugo/sigma-pdf.pdf)
- [NIST SP 800-227](https://nvlpubs.nist.gov/nistpubs/SpecialPublications/NIST.SP.800-227.pdf)

---

## Cluster 9 — Forward secrecy claim

### Q2.20: Define forward secrecy and identify your long-term key

**Question:** "You claim 'forward secrecy via per-session ML-KEM.' Define FS precisely. What is your *long-term key*? If `/etc/secoc/keys/mldsa_secoc.key` is dumped today, are past sessions before today compromised, or only future ones?"

**Expected/strong answer outline:**
- FS: compromise of long-term key today does NOT decrypt past recorded sessions.
- Long-term key = ML-DSA private key for ECU identity.
- Per-session ML-KEM ephemeral; FS holds for the *symmetric* session keys.
- Caveat: because the handshake is unauthenticated, FS claim is conditional on an honest handshake.

**Citations:**
- [NIST SP 800-56C Rev 2 §4–§5](https://nvlpubs.nist.gov/nistpubs/SpecialPublications/NIST.SP.800-56Cr2.pdf)
- [Krawczyk SIGMA paper §3](https://webee.technion.ac.il/~hugo/sigma-pdf.pdf)

---

## Cluster 10 — Signature non-repudiation in automotive

### Q2.21: Is non-repudiation actually wanted in vehicle BSW?

**Question:** "You list 'non-repudiation' as a security gain over HMAC. In V2X (IEEE 1609.2), persistent ECU-traceable signatures can be a *privacy* problem. Is non-repudiation a feature or a bug in your design?"

**Expected/strong answer outline:**
- Inside-vehicle BSW: non-repudiation is genuinely useful (event-data recorder, post-incident attribution).
- External (V2X): unwanted — IEEE 1609.2 uses pseudonym certificates.
- ML-DSA signatures are inherently linkable; group/ring signatures or pseudonym rotation needed for privacy.
- Candidate's gateway is internal-only — should clarify scope.

**Citations:**
- [IEEE 1609.2-2022](https://standards.ieee.org/ieee/1609.2/10258/)
- [V2X Security & Privacy survey](https://ieeexplore.ieee.org/ielaam/8782711/8815895/9108394-aam.pdf)

---

## Cluster 11 — Why not SLH-DSA / FN-DSA / HQC

### Q2.22: Why not SLH-DSA for cold-start authentication?

**Question:** "SLH-DSA (FIPS 205) doesn't have the side-channel issues of ML-DSA. For handshake-time auth (1/hour) it would be fine. Why didn't you put SLH-DSA on the static identity key and ML-DSA on the per-PDU path?"

**Expected/strong answer outline:**
- SLH-DSA-128s sign is slow but verify is acceptable for handshake-only.
- Hybrid lattice-and-hash is exactly what NIST IR 8413 / IR 8547 recommend for diversification.
- Honest answer: out of scope.

**Citations:**
- [FIPS 205 §11](https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.205.pdf)
- [NIST IR 8547 §3](https://nvlpubs.nist.gov/nistpubs/ir/2024/NIST.IR.8547.ipd.pdf)

### Q2.23: Stateful hash signatures (LMS/XMSS, SP 800-208) for boot/firmware

**Question:** "Have you considered stateful hash signatures (LMS or XMSS, NIST SP 800-208) for *firmware* signing on the ECU side?"

**Expected/strong answer outline:**
- LMS/XMSS are quantum-safe and approved since 2020.
- Stateful — only safe if signer state is never reused (problematic at scale, fine for low-rate firmware).
- Out of scope for SecOC (which is per-PDU).

**Citations:**
- [NIST SP 800-208](https://nvlpubs.nist.gov/nistpubs/SpecialPublications/NIST.SP.800-208.pdf)

---

## Cluster 12 — liboqs maturity & reproducibility

### Q2.24: Why pinning to a release candidate (0.15.0-rc1)?

**Question:** "You pinned `liboqs 0.15.0-rc1`. The `-rc1` is a *release candidate*, not a release. The final 0.15.0 ships ML-KEM via mlkem-native v1.0.0 with formal proofs. Defend the choice of an RC tag, and explain whether your build will be reproducible in 5 years."

**Expected/strong answer outline:**
- RC vs final: rc1 is feature-frozen but not a stable tag — semver guarantees do not apply.
- Reproducibility risk: pin commit SHA, vendor source.
- 0.15.0 final (Nov 2025) replaced the ML-KEM impl; rc1 may use the older non-formally-verified path.

**Citations:**
- [liboqs 0.15.0 release notes](https://github.com/open-quantum-safe/liboqs/releases/tag/0.15.0)
- [Open Quantum Safe FAQ](https://openquantumsafe.org/faq.html)

### Q2.25: liboqs HQC CVEs in 2025 — do they affect you?

**Question:** "CVE-2025-48946 and CVE-2025-52473 hit liboqs's HQC implementation in 2025. You don't enable HQC. How did you verify your build does not pull in HQC code?"

**Expected/strong answer outline:**
- Build flag: `-DOQS_ENABLE_KEM_hqc_*=OFF` is default since 0.13.0; verify with `nm` on `liboqs.so`.
- Adjacent-CVE concern: shared utility code, build-system bugs.
- Should pin SHA, audit C deps.

**Citations:**
- [CVE-2025-48946 advisory](https://github.com/open-quantum-safe/liboqs/security/advisories/GHSA-3rxw-4v8q-9gq5)
- [CVE-2025-52473 advisory](https://github.com/open-quantum-safe/liboqs/security/advisories/GHSA-qq3m-rq9v-jfgm)

---

## Cluster 13 — Quantum-attack model realism

### Q2.26: How many qubits to break ECC-256/RSA-2048?

**Question:** "Quantitatively: how many *logical* qubits and how many *physical* qubits does Gidney-Ekerå (2019) estimate are needed to factor RSA-2048 in 8 hours? What's the state of the art today?"

**Expected/strong answer outline:**
- Gidney-Ekerå 2019: ~20 million noisy physical qubits, surface code, ~8h, factoring 2048-bit RSA.
- 2025 update (arXiv:2505.15917): <1 million noisy qubits suffice.
- Best machines today (2025): ~1000 physical qubits, ~100 logical.
- NIST IR 8547 timeline: deprecate by 2030, disallow by 2035.

**Citations:**
- [Gidney & Ekerå arXiv:1905.09749](https://arxiv.org/abs/1905.09749)
- [Gidney 2025 arXiv:2505.15917](https://arxiv.org/abs/2505.15917)
- [NIST IR 8547 §4](https://nvlpubs.nist.gov/nistpubs/ir/2024/NIST.IR.8547.ipd.pdf)

---

## Cluster 14 — MAC vs signature for in-vehicle

### Q2.27: 552 msg/s — is that survivable for ADAS / V2X?

**Question:** "Your phase-3 measurement gives 552 msg/s compute-bound throughput on the gateway. A modern ADAS bus aggregates thousands of CAN frames per second. Defend the claim that PQC-signed SecOC is feasible."

**Expected/strong answer outline:**
- 552 msg/s ≪ raw CAN aggregate.
- Realistic deployment: signing only at trust boundaries (gateway, V2X), not per-CAN-frame.
- Critical PDUs only: brake commands, FOTA, diagnostics-write, key updates.
- Hardware accel brings ML-DSA sign down ~10×.

**Citations:**
- [IEEE 1609.2-2022](https://standards.ieee.org/ieee/1609.2/10258/)
- [FIPS 204](https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.204.pdf)

---

## Cluster 15 — HKDF use

### Q2.28: HKDF-SHA256 and domain separation

**Question:** "You use HKDF-SHA256 to derive the SecOC session key from the 32-byte ML-KEM shared secret. Walk through your `Extract(salt, IKM)` and `Expand(PRK, info, L)` parameters: what's in `salt`? what's in `info`? Do you domain-separate per peer, per epoch, per direction?"

**Expected/strong answer outline:**
- RFC 5869 §2: PRK = HMAC-SHA256(salt, IKM); OKM = expand of PRK with info.
- Best practice: salt = handshake transcript hash; info = "secoc-v1 | direction | epoch | peer-id".
- Per-direction (Tx/Rx) keys to avoid reflection.
- L = derived key length (e.g. 32 B for ChaCha20 / 16 B for AES-128).

**Citations:**
- [RFC 5869](https://datatracker.ietf.org/doc/html/rfc5869)
- [NIST SP 800-56C Rev 2 §5](https://nvlpubs.nist.gov/nistpubs/SpecialPublications/NIST.SP.800-56Cr2.pdf)
- [NIST SP 800-227](https://nvlpubs.nist.gov/nistpubs/SpecialPublications/NIST.SP.800-227.pdf)
