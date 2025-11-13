# PQC-SecOC Implementation Roadmap
## Step-by-Step Implementation Guide

---

## Implementation Timeline

**Total Estimated Time:** 4-6 weeks
**Complexity:** High
**Prerequisites:** C programming, cryptography basics, AUTOSAR SecOC knowledge

---

## Week 1: Foundation & Library Integration

### Day 1-2: Environment Setup & Library Download

**Tasks:**
1. ✅ Research completed (PQC_RESEARCH.md created)
2. Download PQC libraries
3. Verify library builds on Windows/MSYS

**Commands:**
```bash
cd ~/Documents/Long-Stuff/HCMUT/computer_engineering_project/Autosar_SecOC
mkdir external
cd external

# Download liboqs (comprehensive PQC library)
git clone --depth=1 https://github.com/open-quantum-safe/liboqs.git
cd liboqs
mkdir build && cd build
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..
mingw32-make -j4

# Alternative: Download individual libraries
cd ../..
git clone https://github.com/pq-code-package/mlkem-native.git
git clone https://github.com/pq-code-package/mldsa-native.git
```

**Deliverables:**
- [ ] `external/liboqs/` with compiled library
- [ ] Test programs verifying ML-KEM and ML-DSA work

### Day 3-4: PQC Wrapper Layer

**Create:** `source/PQC/` directory

**Files to create:**

1. **`include/PQC/PQC.h`:**
```c
#ifndef INCLUDE_PQC_H_
#define INCLUDE_PQC_H_

#include "Std_Types.h"

/* Algorithm types */
typedef enum {
    PQC_MLKEM_512,
    PQC_MLKEM_768,  /* Recommended */
    PQC_MLKEM_1024,
    PQC_MLDSA_44,
    PQC_MLDSA_65,   /* Recommended */
    PQC_MLDSA_87
} PQC_Algorithm_Type;

/* ML-KEM Key sizes for ML-KEM-768 */
#define MLKEM768_PUBLIC_KEY_BYTES   1184
#define MLKEM768_SECRET_KEY_BYTES   2400
#define MLKEM768_CIPHERTEXT_BYTES   1088
#define MLKEM768_SHARED_SECRET_BYTES 32

/* ML-DSA Key sizes for ML-DSA-65 */
#define MLDSA65_PUBLIC_KEY_BYTES    1952
#define MLDSA65_SECRET_KEY_BYTES    4000
#define MLDSA65_SIGNATURE_BYTES     3309

/* Key structures */
typedef struct {
    uint8 publicKey[MLKEM768_PUBLIC_KEY_BYTES];
    uint8 secretKey[MLKEM768_SECRET_KEY_BYTES];
} PQC_MLKEM_KeyPair;

typedef struct {
    uint8 publicKey[MLDSA65_PUBLIC_KEY_BYTES];
    uint8 secretKey[MLDSA65_SECRET_KEY_BYTES];
} PQC_MLDSA_KeyPair;

/* Function prototypes */
Std_ReturnType PQC_MLKEM_KeyGen(PQC_MLKEM_KeyPair* keyPair);
Std_ReturnType PQC_MLKEM_Encapsulate(
    const uint8* publicKey,
    uint8* ciphertext,
    uint8* sharedSecret
);
Std_ReturnType PQC_MLKEM_Decapsulate(
    const uint8* ciphertext,
    const uint8* secretKey,
    uint8* sharedSecret
);

Std_ReturnType PQC_MLDSA_KeyGen(PQC_MLDSA_KeyPair* keyPair);
Std_ReturnType PQC_MLDSA_Sign(
    const uint8* message,
    uint32 messageLen,
    const uint8* secretKey,
    uint8* signature,
    uint32* signatureLen
);
Std_ReturnType PQC_MLDSA_Verify(
    const uint8* message,
    uint32 messageLen,
    const uint8* signature,
    uint32 signatureLen,
    const uint8* publicKey
);

#endif /* INCLUDE_PQC_H_ */
```

2. **`source/PQC/PQC_MLKEM.c`** - Wrapper for ML-KEM operations
3. **`source/PQC/PQC_MLDSA.c`** - Wrapper for ML-DSA operations

**Deliverables:**
- [ ] PQC wrapper layer compiles
- [ ] Unit tests for key generation, sign/verify, encap/decap

### Day 5-7: CSM Layer Extension

**Modify:** `source/Csm/` and `include/Csm/`

**Tasks:**
1. Add new CSM functions to `Csm.h`
2. Implement in `Csm.c`
3. Add key storage structures
4. Initialize keys on `Csm_Init()`

**Key Code Additions:**

```c
/* In Csm.h */
#define CSM_PQC_MODE  TRUE

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
```

**Deliverables:**
- [ ] CSM compiles with new PQC functions
- [ ] Keys generated and stored on init
- [ ] Sign/verify operations work end-to-end

---

## Week 2: SecOC Core Modifications

### Day 8-10: Authenticate Function

**Modify:** `source/SecOC/SecOC.c` - `authenticate()` function

**Changes:**
1. Increase freshness to 64 bits
2. Call `Csm_SignatureGenerate()` instead of `Csm_MacGenerate()`
3. Build secured PDU with signature
4. Update buffer sizes

**Pseudo-code:**
```c
STATIC Std_ReturnType authenticate_PQC(...) {
    // 1. Get 64-bit freshness
    uint64 freshness = FVM_GetTxFreshness64(...);

    // 2. Build DataToSign = AuthPdu || Freshness (8 bytes)
    memcpy(DataToSign, AuthPdu->SduDataPtr, AuthPdu->SduLength);
    memcpy(DataToSign + AuthPdu->SduLength, &freshness, 8);
    uint32 DataToSignLen = AuthPdu->SduLength + 8;

    // 3. Sign
    uint32 sigLen = MLDSA65_SIGNATURE_BYTES;
    result = Csm_SignatureGenerate(jobId, DataToSign, DataToSignLen,
                                    signature, &sigLen);

    // 4. Build SecuredPDU = [Header] [AuthPdu] [Freshness] [Signature]
    uint32 cursor = 0;
    if (headerLen > 0) {
        memcpy(SecPdu->SduDataPtr, &AuthPdu->SduLength, headerLen);
        cursor += headerLen;
    }
    memcpy(SecPdu->SduDataPtr + cursor, AuthPdu->SduDataPtr, AuthPdu->SduLength);
    cursor += AuthPdu->SduLength;
    memcpy(SecPdu->SduDataPtr + cursor, &freshness, 8);
    cursor += 8;
    memcpy(SecPdu->SduDataPtr + cursor, signature, sigLen);
    cursor += sigLen;

    SecPdu->SduLength = cursor;
    return E_OK;
}
```

**Deliverables:**
- [ ] `authenticate_PQC()` compiles
- [ ] Generates valid secured PDUs with signatures
- [ ] Buffer sizes adequate (4096+ bytes)

### Day 11-12: Verify Function

**Modify:** `source/SecOC/SecOC.c` - `verify()` function

**Changes:**
1. Parse signature from secured PDU
2. Extract 64-bit freshness
3. Validate freshness (must be > last seen)
4. Call `Csm_SignatureVerify()`

**Pseudo-code:**
```c
STATIC Std_ReturnType verify_PQC(...) {
    // 1. Parse SecuredPDU
    parseSecuredPdu_PQC(RxPduId, SecPdu, &intermediate);

    // 2. Check freshness
    if (intermediate.freshness64 <= lastSeenFreshness[RxPduId]) {
        *verification_result = SECOC_FRESHNESSFAILURE;
        return E_NOT_OK;
    }

    // 3. Reconstruct DataToSign
    memcpy(DataToSign, intermediate.authenticPdu, intermediate.authenticPduLen);
    memcpy(DataToSign + intermediate.authenticPduLen,
           &intermediate.freshness64, 8);
    uint32 DataToSignLen = intermediate.authenticPduLen + 8;

    // 4. Verify signature
    result = Csm_SignatureVerify(
        jobId, DataToSign, DataToSignLen,
        intermediate.signature, intermediate.signatureLen,
        publicKey, &verifyResult
    );

    if (verifyResult != CRYPTO_E_VER_OK) {
        *verification_result = SECOC_VERIFICATIONFAILURE;
        return E_NOT_OK;
    }

    // 5. Update freshness
    lastSeenFreshness[RxPduId] = intermediate.freshness64;
    *verification_result = SECOC_VERIFICATIONSUCCESS;
    return E_OK;
}
```

**Deliverables:**
- [ ] `verify_PQC()` correctly validates signatures
- [ ] Freshness validation prevents replay attacks
- [ ] Integration test: sign → transmit → receive → verify

### Day 13-14: FVM Updates

**Modify:** `source/SecOC/FVM.c`

**Changes:**
1. Change counter from `uint8` to `uint64`
2. Update `FVM_GetTxFreshness()` → `FVM_GetTxFreshness64()`
3. Update `FVM_GetRxFreshness()` → `FVM_GetRxFreshness64()`
4. Increase freshness truncation length

**Deliverables:**
- [ ] 64-bit freshness counters implemented
- [ ] Counter overflow handling (wrap-around at 2^64-1)
- [ ] Freshness validation logic correct

---

## Week 3: Ethernet Focus & ML-KEM Integration

### Day 15-16: Ethernet-Only Configuration

**Tasks:**
1. Update `CMakeLists.txt` to exclude CAN files (optional)
2. Simplify PduR to route only Ethernet
3. Ensure SoAd handles large PDUs (4KB+)

**Configuration changes in `SecOC_PBcfg.c`:**
```c
/* Only Ethernet SOAD-TP PDUs */
const SecOC_TxPduProcessingType SecOCTxPduProcessing[] = {
    {
        .SecOCTxPduProcessingId = 0,
        .SecOCTxAuthenticPduLayer = &EthernetAuthPdu,
        .SecOCTxSecuredPduLayer = &EthernetSecuredPdu,
        .SecOCAuthInfoTruncLength = MLDSA65_SIGNATURE_BYTES * 8, // bits
        .SecOCFreshnessValueTruncLength = 64, // 8 bytes
        .SecOCTxPduType = SECOC_TPPDU, // Must use TP for large signatures
        // ... rest of config
    }
};
```

**Deliverables:**
- [ ] Ethernet-only SecOC builds and runs
- [ ] Large PDUs (4KB) transmit correctly via SoAd-TP
- [ ] GUI shows only Ethernet options

### Day 17-19: ML-KEM Key Exchange

**Create:** `source/PQC/PQC_KeyExchange.c`

**Implement key exchange protocol:**

```c
/* Gateway A (Initiator) */
Std_ReturnType PQC_InitiateKeyExchange(uint8 gatewayId) {
    // 1. Generate ML-KEM keypair
    PQC_MLKEM_KeyPair keyPair;
    PQC_MLKEM_KeyGen(&keyPair);

    // 2. Send public key to Gateway B
    SendPublicKey(gatewayId, keyPair.publicKey, MLKEM768_PUBLIC_KEY_BYTES);

    // 3. Wait for ciphertext response
    // ... (handled in callback)
}

/* Gateway B (Responder) */
Std_ReturnType PQC_RespondKeyExchange(
    uint8 gatewayId,
    const uint8* receivedPublicKey
) {
    // 1. Encapsulate using received public key
    uint8 ciphertext[MLKEM768_CIPHERTEXT_BYTES];
    uint8 sharedSecret[MLKEM768_SHARED_SECRET_BYTES];
    PQC_MLKEM_Encapsulate(receivedPublicKey, ciphertext, sharedSecret);

    // 2. Store shared secret
    StoreSharedSecret(gatewayId, sharedSecret);

    // 3. Send ciphertext back to Gateway A
    SendCiphertext(gatewayId, ciphertext, MLKEM768_CIPHERTEXT_BYTES);

    return E_OK;
}

/* Gateway A (Complete) */
Std_ReturnType PQC_CompleteKeyExchange(
    uint8 gatewayId,
    const uint8* receivedCiphertext
) {
    // 1. Decapsulate ciphertext with secret key
    uint8 sharedSecret[MLKEM768_SHARED_SECRET_BYTES];
    PQC_MLKEM_Decapsulate(receivedCiphertext, storedSecretKey, sharedSecret);

    // 2. Store shared secret
    StoreSharedSecret(gatewayId, sharedSecret);

    // 3. Derive session keys from shared secret (optional)
    DeriveSessionKeys(sharedSecret);

    return E_OK;
}
```

**GUI Integration:**
- Add "Initiate Key Exchange" button
- Display: Public keys, ciphertext, shared secret status

**Deliverables:**
- [ ] ML-KEM key exchange works between two gateways
- [ ] Shared secrets match on both sides
- [ ] GUI shows key exchange flow
- [ ] Keys stored securely

### Day 20-21: Configuration Updates

**Update all configuration files:**

1. **`SecOC_Cfg.h`:**
```c
#define SECOC_PQC_ENABLED                   TRUE
#define SECOC_PQC_ALGORITHM_SIG             PQC_MLDSA_65
#define SECOC_PQC_ALGORITHM_KEM             PQC_MLKEM_768
#define SECOC_SIGNATURE_MAX_LENGTH          3400
#define SECOC_FRESHNESS_VALUE_LENGTH        64
#define SECOC_SECPDU_MAX_LENGTH             4096
#define SECOC_ETHERNET_ONLY                 TRUE
```

2. **`SecOC_PBcfg.c`:**
- Update all PDU configurations for Ethernet
- Set signature length = 3309 bytes
- Set freshness length = 64 bits

**Deliverables:**
- [ ] All configurations updated
- [ ] System compiles with new config
- [ ] Sanity tests pass

---

## Week 4: Testing, Performance, and GUI

### Day 22-24: Comprehensive Testing

**Test Plan:**

1. **Unit Tests** (`test/PQC_Tests.cpp`):
```cpp
TEST(PQC_MLKEM, KeyGeneration) {
    PQC_MLKEM_KeyPair keyPair;
    ASSERT_EQ(PQC_MLKEM_KeyGen(&keyPair), E_OK);
    // Verify keys are non-zero
}

TEST(PQC_MLKEM, EncapDecap) {
    // Generate keypair
    // Encapsulate
    // Decapsulate
    // Assert shared secrets match
}

TEST(PQC_MLDSA, SignVerify) {
    // Generate keypair
    // Sign message
    // Verify signature → should pass
    // Alter message
    // Verify signature → should fail
}
```

2. **Integration Tests:**
```cpp
TEST(PQC_SecOC, EndToEndAuthentication) {
    // Gateway A: Authenticate message
    // Transmit over Ethernet
    // Gateway B: Receive and verify
    // Assert: Verification success
}

TEST(PQC_SecOC, ReplayAttack) {
    // Send message 1 with freshness N
    // Send message 2 with freshness N (replay)
    // Assert: Freshness failure
}

TEST(PQC_SecOC, SignatureTampering) {
    // Send message with valid signature
    // Alter signature bytes
    // Verify
    // Assert: Verification failure
}
```

3. **Attack Simulation Tests:**
- Signature tampering
- Freshness replay
- Man-in-the-middle (no shared secret)
- Malformed PDUs

**Deliverables:**
- [ ] All unit tests pass
- [ ] All integration tests pass
- [ ] Attack simulations demonstrate security

### Day 25-26: Performance Analysis

**Create:** `test/Performance_Tests.cpp`

**Benchmark Code:**
```cpp
#include <chrono>

void BenchmarkMLKEM() {
    auto start = std::chrono::high_resolution_clock::now();

    // Perform 1000 key generations
    for (int i = 0; i < 1000; i++) {
        PQC_MLKEM_KeyPair keyPair;
        PQC_MLKEM_KeyGen(&keyPair);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "ML-KEM KeyGen: " << duration.count() / 1000.0 << " µs/op" << std::endl;
}

void BenchmarkMLDSA() {
    // Similar for Sign and Verify operations
}

void BenchmarkEndToEnd() {
    // Measure full authenticate → transmit → receive → verify cycle
}
```

**Metrics to collect:**
| Operation | Time (µs) | Memory (KB) | Notes |
|-----------|-----------|-------------|-------|
| ML-KEM KeyGen | | | |
| ML-KEM Encap | | | |
| ML-KEM Decap | | | |
| ML-DSA KeyGen | | | |
| ML-DSA Sign | | | |
| ML-DSA Verify | | | |
| SecOC Authenticate (MAC) | | | Baseline |
| SecOC Authenticate (PQC) | | | Compare |
| End-to-End (MAC) | | | Baseline |
| End-to-End (PQC) | | | Compare |

**Create:** `PERFORMANCE_REPORT.md` with results and analysis

**Deliverables:**
- [ ] Performance benchmarks completed
- [ ] Comparison table (PQC vs. MAC)
- [ ] Analysis of real-time impact
- [ ] Recommendations for optimization

### Day 27-28: GUI Enhancements

**Modify:** `GUI/simple_gui.py`

**Changes:**

1. **Remove CAN options:**
```python
self.config_combo.addItems([
    "Ethernet SOAD IF",
    "Ethernet SOAD TP (PQC)"  # Only option for PQC
])
```

2. **Add Key Exchange Tab:**
```python
def create_key_exchange_tab(self):
    widget = QWidget()
    layout = QVBoxLayout(widget)

    # ML-KEM Key Generation
    keygen_btn = QPushButton("Generate ML-KEM KeyPair")
    keygen_btn.clicked.connect(self.on_generate_mlkem_key)
    layout.addWidget(keygen_btn)

    # Display Public Key (truncated)
    self.pubkey_display = QTextEdit()
    self.pubkey_display.setReadOnly(True)
    layout.addWidget(QLabel("Public Key (first 64 bytes):"))
    layout.addWidget(self.pubkey_display)

    # Key Exchange
    exchange_btn = QPushButton("Initiate Key Exchange")
    exchange_btn.clicked.connect(self.on_key_exchange)
    layout.addWidget(exchange_btn)

    # Shared Secret Status
    self.shared_secret_status = QLabel("Shared Secret: Not established")
    layout.addWidget(self.shared_secret_status)

    return widget
```

3. **Update Transmitter Tab:**
- Show signature size
- Display freshness as 64-bit hex value
- Add "Sign & Transmit" button

4. **Update Receiver Tab:**
- Show signature verification status
- Display public key used
- Show freshness comparison

**Add C functions to `GUIInterface.c`:**
```c
char* GUIInterface_GenerateMLKEMKey(uint8_t* publicKey, uint32* pubKeyLen);
char* GUIInterface_InitiateKeyExchange(uint8_t gatewayId);
char* GUIInterface_GetSharedSecretStatus(uint8_t* status);
```

**Deliverables:**
- [ ] GUI supports PQC operations
- [ ] Key exchange UI works
- [ ] Signature display functional
- [ ] User-friendly for PQC testing

---

## Week 5-6: Documentation, Optimization, and Finalization

### Week 5: Documentation

**Create:**
1. **`PQC_USER_GUIDE.md`** - How to use PQC-SecOC
2. **`PQC_API_REFERENCE.md`** - API documentation
3. **`PQC_TEST_REPORT.md`** - Test results
4. **`PERFORMANCE_REPORT.md`** - Benchmarks and analysis

**Update:**
- **`README.md`** - Add PQC sections
- **`CLAUDE.md`** - Document PQC architecture

### Week 6: Optimization & Polish

**Tasks:**
1. Code review and refactoring
2. Memory optimization (reduce key storage if possible)
3. Performance tuning (use SIMD if available)
4. Add compile-time options to switch PQC on/off
5. Final testing on target hardware (Raspberry Pi if available)

**Create:** `KNOWN_ISSUES.md` listing any limitations

---

## Success Criteria

✅ **Functional:**
- [ ] ML-KEM key exchange establishes shared secrets
- [ ] ML-DSA signatures generated and verified correctly
- [ ] Freshness prevents replay attacks
- [ ] Ethernet gateway communication works end-to-end
- [ ] Attack simulations fail as expected

✅ **Performance:**
- [ ] Sign operation < 500 µs on target hardware
- [ ] Verify operation < 300 µs on target hardware
- [ ] End-to-end latency < 1 ms for typical PDU

✅ **Quality:**
- [ ] All unit tests pass (>95% code coverage)
- [ ] All integration tests pass
- [ ] No memory leaks (valgrind clean)
- [ ] MISRA-compliant (as much as possible)
- [ ] Documentation complete

✅ **Usability:**
- [ ] GUI functional for PQC operations
- [ ] Clear error messages
- [ ] Easy to configure

---

## Risk Management

| Risk | Impact | Mitigation |
|------|--------|-----------|
| PQC libraries don't compile on Windows | High | Use pre-built binaries or Docker with Linux |
| Performance too slow for real-time | High | Use hardware acceleration or reduce signing frequency |
| Signature size exceeds Ethernet MTU | Medium | Already using TP mode with fragmentation |
| Memory constraints on embedded targets | Medium | Optimize key storage, use flash for keys |
| Complexity leads to bugs | High | Extensive testing, code reviews |

---

## Next Steps

**To begin implementation:**

1. **Review this roadmap** - Understand the full scope
2. **Set up development environment** - Ensure all tools ready
3. **Download PQC libraries** - Start with liboqs
4. **Create project branch** - `git checkout -b pqc-integration`
5. **Begin Week 1, Day 1** - Follow the roadmap step-by-step

**Questions to resolve:**
- Which security level? (Recommend ML-KEM-768 + ML-DSA-65)
- Hardware target specs? (CPU, RAM, Flash)
- Real-time constraints? (Max latency tolerance)
- Key storage mechanism? (RAM, Flash, TPM?)

---

## Version History

- **v1.0** (2025-11-13): Initial roadmap created

