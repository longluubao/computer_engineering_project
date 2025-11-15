# PQC Test Enhancement Plan

## Based on Deep Research of Old Test Files

This document outlines the comprehensive enhancements to be added to both test files based on the research of the old liboqs test infrastructure.

---

## 1. ML-KEM-768 Enhancements (Standalone Test)

### A. Rejection Sampling Test
**Purpose**: Test ML-KEM specific rejection behavior when decapsulating corrupted ciphertexts
**Source**: `test_kem.c:mlkem_rej_testcase()`
**Implementation**:
- Generate valid keypair
- Encapsulate to get valid ciphertext + shared secret
- **Scenario 1**: Corrupt secret key, verify different shared secret generated (SHAKE256 fallback)
- **Scenario 2**: Corrupt ciphertext, verify different shared secret generated
**Expected**: Both scenarios should produce different shared secrets (not fail completely)

### B. Public Key Sanity Check (FIPS 203 Section 7.2)
**Purpose**: Validate ML-KEM public key encoding/decoding consistency
**Source**: `vectors_kem.c:sanityCheckPK()`
**Implementation**:
- Decode public key polynomial coefficients
- Perform Barrett reduction mod Q (Q=3329)
- Re-encode and compare with original
- Validate structure: ρ (seed, 32 bytes) + k=3 polynomials for ML-KEM-768

### C. Secret Key Sanity Check (FIPS 203 Section 7.3)
**Purpose**: Validate SHA3-256 hash of public key embedded in secret key
**Source**: `vectors_kem.c:sanityCheckSK()`
**Implementation**:
- Extract public key hash from secret key (at offset 384k)
- Compute SHA3-256 of actual public key
- Compare hashes to ensure consistency
- Validates secret key hasn't been corrupted

### D. Invalid Ciphertext Detection
**Purpose**: Test IND-CCA security by injecting random ciphertexts
**Source**: `test_kem.c:kem_test_correctness()`
**Implementation**:
- Generate random ciphertext (not from encapsulation)
- Attempt decapsulation
- Verify either failure or different shared secret
- Tests rejection sampling behavior

### E. Buffer Overflow Detection
**Purpose**: Detect out-of-bounds memory writes
**Source**: `test_kem.c:kem_test_correctness()` magic numbers
**Implementation**:
- Place 31-byte magic values before/after output buffers
- Run all ML-KEM operations (keygen, encaps, decaps)
- Verify magic values unchanged after operations
- 31 bytes (not 32) breaks alignment to catch alignment bugs

---

## 2. ML-DSA-65 Enhancements (Standalone Test)

### A. Bitflip Attack Resistance Testing
**Purpose**: Validate signature rejection when messages or signatures are corrupted
**Source**: `test_sig.c:test_sig_bitflip()` and `test_helpers.c:flip_bit()`
**Implementation**:

**EUF-CMA Test** (Existential Unforgeability):
- Generate keypair, sign message
- Flip 50 random bits in **message**
- Verify signature validation FAILS
- Tests message integrity

**SUF-CMA Test** (Strong Unforgeability):
- Generate keypair, sign message
- Flip 50 random bits in **signature**
- Verify signature validation FAILS
- Tests signature uniqueness

**Exhaustive Mode** (optional):
- Test ALL bit positions (not just 50 random)
- More thorough but slower

### B. Context String Testing
**Purpose**: Test ML-DSA context string support (0-255 bytes)
**Source**: `test_sig.c:sig_test_correctness_ctx()`
**Implementation**:
- Test with empty context (0 bytes)
- Test with small context (16 bytes): "PQC_AUTOSAR_TEST"
- Test with maximum context (255 bytes)
- Verify signatures with correct context validate
- Verify signatures fail with wrong context
- Validates context binding

### C. Message Size Variation Testing
**Purpose**: Test different message sizes for edge cases
**Current**: 5 sizes (8, 64, 256, 512, 1024 bytes)
**Enhancement**: Add edge cases
- 0 bytes (empty message)
- 1 byte (minimal message)
- 65535 bytes (large message)
- Tests scalability

### D. Stateless vs Stateful Signature Validation
**Purpose**: Ensure ML-DSA (stateless) behaves correctly
**Source**: `test_sig_stfl.c`
**Implementation**:
- Sign same message multiple times
- Verify signatures are DIFFERENT (non-deterministic)
- Unlike stateful SLH-DSA where signatures would be identical
- Validates proper randomness usage

---

## 3. AUTOSAR Integration Enhancements (Integration Test)

### A. ML-KEM via Csm Layer
**Current**: Not tested (Csm doesn't wrap ML-KEM)
**Enhancement Options**:
1. Create custom Csm wrapper for ML-KEM (requires Csm.c modifications)
2. Test ML-KEM standalone alongside Csm tests for comparison
3. Document why ML-KEM not integrated (SecOC uses signatures not KEMs)

**Recommended**: Option 3 - explain SecOC uses authentication (signatures) not encryption (KEMs)

### B. Enhanced Csm_Signature Testing
**Current**: Basic sign/verify with single message
**Enhancements**:
- Multiple message sizes (8, 64, 256, 512, 1024 bytes)
- Context string testing through Csm layer
- Bitflip testing via Csm (corrupt message, corrupt signature)
- Parallel signature generation (test thread safety if needed)

### C. SecOC Freshness Value Integration
**Purpose**: Test SecOC Data-to-Authenticator with freshness
**Implementation**:
- Construct Data-to-Authenticator: Message ID + Data + Freshness Value
- Sign with ML-DSA via Csm
- Verify freshness counter increments
- Test replay attack detection (reuse old freshness value)

### D. Performance Profiling Enhancements
**Current**: Basic mean time
**Enhancements**:
- Min/Max/StdDev timing
- Percentile analysis (50th, 90th, 99th percentile)
- Memory allocation tracking
- Cache performance analysis (if possible)
- CPU cycle counting (platform-dependent)

---

## 4. Common Test Enhancements (Both Files)

### A. Deterministic Testing Mode
**Purpose**: Reproducible test results with fixed seeds
**Source**: `kat_kem.c` and `kat_sig.c` NIST DRBG
**Implementation**:
- Option to use fixed entropy seed (48 bytes: 0x00 to 0x2F)
- Derandomized keygen (if PQC_MLKEM_KeyGen_seed available)
- Derandomized encaps (if PQC_MLKEM_Encaps_seed available)
- Verify results match expected KAT values

### B. Memory Safety Enhancements
**Current**: Basic allocation
**Enhancements**:
- Magic number guards (31-byte 0xAA...AA before, 0xBB...BB after)
- Alignment breaking detection
- Secure memory cleanup verification
- Stack overflow detection

### C. Comprehensive Metrics Collection
**Enhancements**:
- Wall-clock time (microseconds)
- CPU cycles (platform-specific via RDTSC or PMU)
- Memory footprint (total allocation for keys + ciphertexts/signatures)
- Throughput (operations per second)
- Bandwidth usage (bytes transmitted per operation)

### D. CSV Output Enhancements
**Current**: Basic metrics
**Enhancements**:
- Separate files for ML-KEM and ML-DSA
- Histogram data (for distribution analysis)
- Percentile data (50th, 90th, 95th, 99th)
- Comparison with classical algorithms
- FIPS compliance verification results

---

## 5. File Structure After Enhancement

```
test_pqc_standalone.c (estimated ~1500 lines)
├── Phase 1: ML-KEM-768 Testing
│   ├── 1.1 Basic Correctness (existing)
│   ├── 1.2 Rejection Sampling Test (NEW)
│   ├── 1.3 Public Key Sanity Check (NEW)
│   ├── 1.4 Secret Key Sanity Check (NEW)
│   ├── 1.5 Invalid Ciphertext Test (NEW)
│   ├── 1.6 Buffer Overflow Detection (NEW)
│   └── 1.7 Performance Metrics (enhanced)
│
├── Phase 2: ML-DSA-65 Testing
│   ├── 2.1 Basic Correctness (existing)
│   ├── 2.2 Bitflip EUF-CMA Test (NEW)
│   ├── 2.3 Bitflip SUF-CMA Test (NEW)
│   ├── 2.4 Context String Testing (NEW)
│   ├── 2.5 Message Size Variation (enhanced)
│   ├── 2.6 Signature Non-Determinism Test (NEW)
│   └── 2.7 Performance Metrics (enhanced)
│
└── Phase 3: Results & Reporting
    ├── Comprehensive CSV output
    ├── FIPS compliance summary
    └── Security validation summary

test_pqc_secoc_integration.c (estimated ~900 lines)
├── Phase 1: Csm PQC Signature (ML-DSA)
│   ├── 1.1 Basic Sign/Verify (existing)
│   ├── 1.2 Multiple Message Sizes (NEW)
│   ├── 1.3 Context String via Csm (NEW)
│   ├── 1.4 Bitflip via Csm (NEW)
│   └── 1.5 Enhanced Metrics (NEW)
│
├── Phase 2: Csm Classical MAC
│   ├── 2.1 Basic MAC Generate/Verify (existing)
│   └── 2.2 Enhanced Comparison (existing)
│
├── Phase 3: SecOC Integration
│   ├── 3.1 Freshness Value Testing (NEW)
│   ├── 3.2 Replay Attack Detection (NEW)
│   ├── 3.3 Data-to-Authenticator Construction (NEW)
│   └── 3.4 End-to-End SecOC Flow (NEW)
│
└── Phase 4: Security & Performance
    ├── Tampering detection (existing)
    ├── Performance comparison (enhanced)
    └── Security test summary
```

---

## 6. Implementation Priority

### High Priority (Must Have)
1. ML-KEM rejection sampling test
2. ML-DSA bitflip testing (EUF-CMA and SUF-CMA)
3. ML-KEM sanity checks (public key and secret key)
4. Buffer overflow detection with magic numbers
5. Enhanced performance metrics (min/max/stddev)

### Medium Priority (Should Have)
1. Context string testing for ML-DSA
2. Invalid ciphertext detection for ML-KEM
3. SecOC freshness value integration
4. Deterministic testing mode

### Low Priority (Nice to Have)
1. Exhaustive bitflip mode (all bits)
2. CPU cycle counting
3. Cache performance analysis
4. Histogram generation for timing distribution

---

## 7. Testing Validation Criteria

### ML-KEM-768 Tests Pass If:
- ✅ Rejection sampling produces different shared secrets (not crashes)
- ✅ Public key sanity check: re-encoded PK matches original
- ✅ Secret key sanity check: embedded PK hash matches computed hash
- ✅ Invalid ciphertext either fails or produces different shared secret
- ✅ Buffer magic numbers unchanged after all operations
- ✅ 100% correctness rate for valid operations

### ML-DSA-65 Tests Pass If:
- ✅ Bitflip EUF-CMA: 100% of corrupted messages rejected
- ✅ Bitflip SUF-CMA: 100% of corrupted signatures rejected
- ✅ Context binding: signatures fail with wrong context
- ✅ Multiple signatures on same message are different (non-deterministic)
- ✅ All valid signatures verify successfully
- ✅ 100% correctness rate for valid operations

### AUTOSAR Integration Tests Pass If:
- ✅ Csm_SignatureGenerate succeeds 100% of time
- ✅ Csm_SignatureVerify validates all valid signatures
- ✅ Tampering detection: 100% of tampered messages/signatures rejected
- ✅ Freshness value increments correctly
- ✅ Replay attacks detected (old freshness values rejected)
- ✅ Performance overhead documented and acceptable

---

## 8. Documentation Updates Needed

1. **PQC_TEST_README.md**: Update with new test descriptions
2. **BUILD_GUIDE.md**: Update build instructions if needed
3. **TECHNICAL_REPORT.md**: Add section on test coverage and validation
4. **Code comments**: Comprehensive inline documentation for all new tests

---

## Next Steps

1. Implement ML-KEM enhancements first (simpler, fewer dependencies)
2. Implement ML-DSA enhancements second (more complex)
3. Implement AUTOSAR integration enhancements third (depends on both)
4. Test all enhancements individually
5. Test integrated suite
6. Update documentation
7. Generate final test reports

---

**End of Enhancement Plan**
