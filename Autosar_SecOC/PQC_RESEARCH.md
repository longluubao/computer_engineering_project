# Post-Quantum Cryptography Integration for AUTOSAR SecOC
## Research Document and Implementation Plan

**Date:** November 13, 2025
**Target:** Ethernet Gateway SecOC with PQC
**Algorithms:** ML-KEM (FIPS 203) and ML-DSA (FIPS 204)

---

## 1. Executive Summary

This document outlines the integration of NIST-standardized Post-Quantum Cryptography (PQC) algorithms into the AUTOSAR SecOC (Secure Onboard Communication) module, specifically targeting Ethernet Gateway applications. The integration replaces traditional MAC-based authentication with quantum-resistant digital signatures (ML-DSA) and implements secure key exchange using ML-KEM.

### Key Changes:
- **Authentication Method:** MAC → ML-DSA Digital Signatures
- **Key Management:** Static keys → ML-KEM Key Encapsulation
- **Focus Area:** CAN/CAN-TP/FlexRay → **Ethernet Gateway only**
- **Security Level:** Classical → Quantum-Resistant

---

## 2. Background: Post-Quantum Cryptography

### 2.1 The Quantum Threat

Quantum computers pose a significant threat to current cryptographic systems:
- **Shor's Algorithm:** Breaks RSA, ECC, and DH key exchange
- **Grover's Algorithm:** Weakens symmetric encryption (requires doubling key sizes)
- **Timeline:** NIST mandates transition by 2030 for U.S. government systems

### 2.2 NIST PQC Standardization (August 2024)

NIST released three finalized standards:

#### **FIPS 203: ML-KEM (Module-Lattice-Based Key-Encapsulation Mechanism)**
- **Formerly:** CRYSTALS-Kyber
- **Purpose:** Quantum-resistant key exchange
- **Security Basis:** Module Learning With Errors (M-LWE) problem
- **Parameter Sets:**
  - **ML-KEM-512:** NIST Security Level 1 (equiv. to AES-128)
  - **ML-KEM-768:** NIST Security Level 3 (equiv. to AES-192) **← Recommended**
  - **ML-KEM-1024:** NIST Security Level 5 (equiv. to AES-256)

**Key Sizes (ML-KEM-768):**
- Public Key: 1,184 bytes
- Secret Key: 2,400 bytes
- Ciphertext: 1,088 bytes
- Shared Secret: 32 bytes

#### **FIPS 204: ML-DSA (Module-Lattice-Based Digital Signature Algorithm)**
- **Formerly:** CRYSTALS-Dilithium
- **Purpose:** Quantum-resistant digital signatures
- **Security Basis:** Module Learning With Errors (M-LWE) problem
- **Parameter Sets:**
  - **ML-DSA-44:** NIST Security Level 2
  - **ML-DSA-65:** NIST Security Level 3 **← Recommended**
  - **ML-DSA-87:** NIST Security Level 5

**Sizes (ML-DSA-65):**
- Public Key: 1,952 bytes
- Secret Key: 4,000 bytes
- Signature: ~3,293 bytes (average)

---

## 3. Current SecOC Architecture Analysis

### 3.1 Existing MAC-Based Flow

#### **Transmission (Tx):**
```
Authentic PDU → Get Freshness → Construct DataToAuth
→ Csm_MacGenerate() → Append MAC + Freshness → Secured PDU
```

#### **Reception (Rx):**
```
Secured PDU → Parse PDU → Extract MAC + Freshness
→ Csm_MacVerify() → Validate Freshness → Forward Authentic PDU
```

### 3.2 Key Components

1. **SecOC Core** (`source/SecOC/SecOC.c`):
   - `authenticate()`: Generates MAC
   - `verify()`: Verifies MAC
   - `parseSecuredPdu()`: Extracts MAC and freshness
   - `constructDataToAuthenticator()`: Prepares data for MAC

2. **CSM Layer** (`source/Csm/Csm.c`):
   - `Csm_MacGenerate()`: Stub MAC generation (uses simple encryption)
   - `Csm_MacVerify()`: Stub MAC verification

3. **Freshness Value Manager** (`source/SecOC/FVM.c`):
   - Counter-based freshness management
   - Prevents replay attacks

4. **PduR** (`source/PduR/`):
   - Routes PDUs between layers
   - Ethernet: `PduR_SoAd.c`

---

## 4. PQC-SecOC Architecture Design

### 4.1 Architectural Changes

#### **Replace MAC with Digital Signatures**

**Current:**
```
Authenticator = HMAC(AuthenticPDU || Freshness, SharedKey)
```

**New (PQC):**
```
Signature = ML-DSA_Sign(AuthenticPDU || Freshness, PrivateKey)
Verify = ML-DSA_Verify(Signature, AuthenticPDU || Freshness, PublicKey)
```

#### **Add ML-KEM Key Exchange**

**Initial Key Establishment:**
```
Gateway A                          Gateway B
---------                          ---------
Generate ML-KEM KeyPair
  ↓
PubKey_A ─────────────────────────→
                                     Encapsulate(PubKey_A)
                                     → (Ciphertext, SharedSecret_B)
         ←─────────────────────────── Ciphertext
Decapsulate(Ciphertext, PrivKey_A)
  → SharedSecret_A

SharedSecret_A == SharedSecret_B ✓
```

**Shared Secret Usage:**
- Derive session keys for AES-256 encryption (if payload encryption needed)
- Derive signing keys for ML-DSA (optional hybrid mode)

### 4.2 Modified PDU Structure

#### **Current Secured PDU:**
```
[Header (opt)] [Authentic PDU] [Freshness (1 byte)] [MAC (4 bytes)]
Example: 3 - 1 2 3 - 5 - 14 33 208 10
```

#### **New PQC Secured PDU:**
```
[Header (opt)] [Authentic PDU] [Freshness (8 bytes)] [Signature (~3,300 bytes)]
Example (Ethernet MTU 1500 bytes):
- Fragmentation required for large signatures
- Use TP mode (SOAD-TP) for segmentation
```

**Key Differences:**
- **Signature Size:** 3,293 bytes (vs. 4 bytes MAC) - **~823x larger!**
- **Freshness:** Increased to 8 bytes (64-bit counter for quantum resistance)
- **Transmission:** MUST use TP mode due to size

### 4.3 Crypto Service Manager (CSM) Redesign

**New CSM Functions:**

```c
// ML-DSA Operations
Std_ReturnType Csm_SignatureGenerate(
    uint32 jobId,
    const uint8* dataPtr,
    uint32 dataLength,
    uint8* signaturePtr,
    uint32* signatureLengthPtr
);

Std_ReturnType Csm_SignatureVerify(
    uint32 jobId,
    const uint8* dataPtr,
    uint32 dataLength,
    const uint8* signaturePtr,
    uint32 signatureLength,
    Crypto_VerifyResultType* verifyPtr
);

// ML-KEM Operations
Std_ReturnType Csm_KeyEncapsulate(
    const uint8* publicKeyPtr,
    uint32 publicKeyLength,
    uint8* ciphertextPtr,
    uint32* ciphertextLengthPtr,
    uint8* sharedSecretPtr
);

Std_ReturnType Csm_KeyDecapsulate(
    const uint8* ciphertextPtr,
    uint32 ciphertextLength,
    const uint8* secretKeyPtr,
    uint32 secretKeyLength,
    uint8* sharedSecretPtr
);

// Key Management
Std_ReturnType Csm_GenerateKeyPair(
    PQC_Algorithm_Type algorithm,  // ML_KEM or ML_DSA
    uint8* publicKeyPtr,
    uint32* publicKeyLengthPtr,
    uint8* secretKeyPtr,
    uint32* secretKeyLengthPtr
);
```

---

## 5. Implementation Plan

### 5.1 Phase 1: Library Integration

**Task 1.1:** Download and integrate PQC libraries
- **ML-KEM:** https://github.com/pq-code-package/mlkem-native
- **ML-DSA:** https://github.com/pq-code-package/mldsa-native
- **Alternative:** https://github.com/open-quantum-safe/liboqs (comprehensive)

**Task 1.2:** Create PQC wrapper layer
- Location: `source/PQC/`
- Files: `PQC_MLKEM.c`, `PQC_MLDSA.c`, `PQC.h`
- Interface: Clean AUTOSAR-style API

**Task 1.3:** Update CMakeLists.txt
- Add PQC library paths
- Link against `mlkem-native.a` and `mldsa-native.a`
- Add compiler flags for optimization

### 5.2 Phase 2: CSM Layer Modification

**Task 2.1:** Implement new CSM functions (listed in Section 4.3)

**Task 2.2:** Update CSM configuration
- Add key storage structures
- ML-DSA public/private key pairs
- ML-KEM ephemeral keys
- Session keys

**Task 2.3:** Key lifecycle management
- Generate keys on init
- Periodic key rotation (configurable)
- Secure key storage (consider TPM if available)

### 5.3 Phase 3: SecOC Core Modifications

**Task 3.1:** Update `authenticate()` function
```c
STATIC Std_ReturnType authenticate_PQC(
    const PduIdType TxPduId,
    PduInfoType* AuthPdu,
    PduInfoType* SecPdu
) {
    // 1. Get 64-bit freshness value
    // 2. Construct DataToSign = AuthPdu || Freshness
    // 3. Call Csm_SignatureGenerate()
    // 4. Build SecuredPDU = Header || AuthPdu || Freshness || Signature
    // 5. Return E_OK
}
```

**Task 3.2:** Update `verify()` function
```c
STATIC Std_ReturnType verify_PQC(
    PduIdType RxPduId,
    PduInfoType* SecPdu,
    SecOC_VerificationResultType* verification_result
) {
    // 1. Parse SecuredPDU
    // 2. Extract Freshness and validate (must be > last seen)
    // 3. Extract Signature
    // 4. Reconstruct DataToSign
    // 5. Call Csm_SignatureVerify()
    // 6. Set verification_result
    // 7. Return status
}
```

**Task 3.3:** Update PDU parsing
- `parseSecuredPdu()`: Handle large signatures with TP reassembly
- Update buffer sizes: `SECOC_SECPDU_MAX_LENGTH` → 4096 bytes minimum

**Task 3.4:** Update freshness management
- FVM: Change counter from uint8 to uint64
- Increase freshness truncation length in config

### 5.4 Phase 4: Ethernet Gateway Focus

**Task 4.1:** Remove CAN/CAN-TP/FlexRay code (optional - for cleaner codebase)
- Keep only: `SoAd.c`, `PduR_SoAd.c`, `ethernet.c`/`ethernet_windows.c`

**Task 4.2:** Optimize SoAd for large PDUs
- Ensure proper TP segmentation/reassembly for 3KB+ signatures
- Update MTU handling

**Task 4.3:** Add ML-KEM key exchange to SoAd
- New function: `SoAd_KeyExchange()`
- Triggered on gateway connection establishment
- Store shared secret for session

### 5.5 Phase 5: Configuration Updates

**Task 5.1:** Update `SecOC_Cfg.h`
```c
#define SECOC_PQC_MODE                      TRUE
#define SECOC_PQC_ALGORITHM_SIG             ML_DSA_65
#define SECOC_PQC_ALGORITHM_KEM             ML_KEM_768
#define SECOC_SIGNATURE_MAX_LENGTH          3400  // bytes
#define SECOC_FRESHNESS_LENGTH              64     // bits
#define SECOC_SECPDU_MAX_LENGTH             4096   // bytes
```

**Task 5.2:** Update `SecOC_PBcfg.c`
- Configure Ethernet-only PDUs
- Update authenticator length: 32 bits → ~26,400 bits (3,300 bytes)
- Update freshness length: 8 bits → 64 bits

### 5.6 Phase 6: GUI Updates

**Task 6.1:** Modify `simple_gui.py`
- Remove CAN configurations, keep only Ethernet options
- Add "Key Exchange" button
- Display: Public keys, shared secrets, signature sizes
- Add PQC-specific attack simulations

**Task 6.2:** Add key management UI
- Generate new key pairs
- Export/import public keys
- Show key exchange status

**Task 6.3:** Update payload display
- Handle large signatures (show truncated + hash)
- Display freshness as 64-bit value

### 5.7 Phase 7: Testing

**Task 7.1:** Unit tests
- ML-KEM encapsulation/decapsulation
- ML-DSA sign/verify
- Key generation

**Task 7.2:** Integration tests
- End-to-end PQC-SecOC flow
- Key exchange + signed communication
- Freshness validation
- Replay attack detection

**Task 7.3:** Attack simulation tests
- Alter signature → verification failure
- Replay old message → freshness failure
- Man-in-the-middle (no shared secret) → verification failure

### 5.8 Phase 8: Performance Analysis

**Metrics to measure:**
1. **Latency:**
   - ML-KEM key generation time
   - ML-KEM encap/decap time
   - ML-DSA key generation time
   - ML-DSA sign time
   - ML-DSA verify time
   - End-to-end authentication time

2. **Throughput:**
   - Messages per second (with PQC vs. MAC)
   - Network bandwidth utilization

3. **Resource Usage:**
   - CPU cycles
   - Memory footprint
   - Flash storage for keys

4. **Comparison:**
   - PQC vs. traditional MAC
   - Impact on real-time constraints

---

## 6. Expected Performance Characteristics

### 6.1 Benchmark Data (from NIST submissions)

**ML-KEM-768 (on modern x86):**
- KeyGen: ~20 µs
- Encapsulate: ~30 µs
- Decapsulate: ~40 µs

**ML-DSA-65 (on modern x86):**
- KeyGen: ~80 µs
- Sign: ~250 µs
- Verify: ~120 µs

**Comparison with HMAC-SHA256:**
- MAC generation: ~2 µs (for small messages)
- **Sign is ~125x slower, Verify is ~60x slower**

### 6.2 Implications for AUTOSAR SecOC

**Challenges:**
1. **Signature Size:** 3,293 bytes vs. 4 bytes (823x increase)
   - **Solution:** Use Ethernet with TP mode for fragmentation
   - **Not suitable for CAN** (limited to 8 bytes per frame)

2. **Computational Overhead:** 100-250 µs per operation
   - **Impact:** May affect real-time guarantees
   - **Mitigation:**
     - Use hardware acceleration if available
     - Implement asynchronous signing
     - Reduce signing frequency (sign batches)

3. **Network Bandwidth:** ~3.3 KB overhead per message
   - **Impact:** Significant for high-frequency messages
   - **Mitigation:**
     - Sign only critical messages
     - Use session keys to reduce signature frequency

4. **Memory Requirements:** ~8 KB for key storage per ECU
   - **Generally acceptable** for modern Ethernet gateways

---

## 7. Security Analysis

### 7.1 Quantum Resistance

**ML-KEM-768 and ML-DSA-65:**
- Provide NIST Security Level 3 (equiv. to AES-192)
- **Secure against:**
  - Shor's algorithm (quantum factoring/discrete log)
  - Grover's algorithm (quantum search)
  - Known lattice reduction attacks
- **Estimated security:** ~2^152 operations for best known attack

### 7.2 Threat Model

**Protected Against:**
1. **Quantum Computer Attacks:** Full protection
2. **Man-in-the-Middle:** Prevented by ML-KEM key exchange
3. **Replay Attacks:** Prevented by 64-bit freshness counter
4. **Signature Forgery:** Computationally infeasible with ML-DSA
5. **Eavesdropping:** Signatures don't encrypt data, but ensure authenticity

**Not Protected Against (without additional measures):**
1. **Side-Channel Attacks:** Implementation must use constant-time ops
2. **Payload Confidentiality:** Signatures don't encrypt (need AES with derived keys)
3. **Denial of Service:** Large signatures increase attack surface

---

## 8. Compliance and Standards

### 8.1 NIST Standards Compliance
- **FIPS 203** (ML-KEM): ✓ Compliant
- **FIPS 204** (ML-DSA): ✓ Compliant

### 8.2 AUTOSAR Compliance
- **AUTOSAR R21-11 SecOC SWS:** Requires adaptation
  - MAC-based specs need PQC addendum
  - PDU structure modifications
  - CSM interface extensions

### 8.3 Automotive Standards
- **ISO/SAE 21434 (Cybersecurity):** Enhanced by PQC
- **UNECE WP.29 R155:** Quantum-ready security

---

## 9. Future Work

1. **Hybrid Cryptography:** Combine ML-DSA with ECDSA for backward compatibility
2. **Hardware Acceleration:** FPGA/ASIC for ML-KEM/ML-DSA
3. **Compressed Signatures:** Explore ML-DSA parameter optimization
4. **Multi-ECU Key Management:** Centralized key distribution system
5. **Formal Verification:** Prove correctness of PQC integration
6. **Performance Optimization:** SIMD, assembly optimizations

---

## 10. References

1. NIST FIPS 203: Module-Lattice-Based Key-Encapsulation Mechanism Standard
   https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.203.pdf

2. NIST FIPS 204: Module-Lattice-Based Digital Signature Standard
   https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.204.pdf

3. mlkem-native: C90 implementation of ML-KEM
   https://github.com/pq-code-package/mlkem-native

4. mldsa-native: C90 implementation of ML-DSA
   https://github.com/pq-code-package/mldsa-native

5. liboqs: Open Quantum Safe library
   https://github.com/open-quantum-safe/liboqs

6. AUTOSAR SecOC SWS R21-11
   https://www.autosar.org/

7. CRYSTALS-Kyber website
   https://pq-crystals.org/kyber/

8. CRYSTALS-Dilithium website
   https://pq-crystals.org/dilithium/

---

## Document Version
- **Version:** 1.0
- **Author:** Claude (AI Assistant)
- **Last Updated:** 2025-11-13
