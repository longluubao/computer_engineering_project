# PQC-SecOC Quick Start Guide

## What's Been Implemented

✅ **Core PQC Functions:**
- ML-KEM-768 (key exchange)
- ML-DSA-65 (digital signatures)
- CSM signature functions
- SecOC PQC authentication/verification

✅ **Key Exchange Manager:**
- Multi-peer key exchange support
- Initiator/Responder roles
- Session management

✅ **Testing & Benchmarking:**
- PQC functionality tests
- Performance benchmarks

## Quick Commands

### Test PQC Functions
```bash
cd ~/Documents/Long-Stuff/HCMUT/computer_engineering_project/Autosar_SecOC
bash build_test_pqc.sh
```

### Run Performance Benchmark
```bash
bash build_perf.sh
```

### Rebuild Full Project
```bash
bash rebuild_pqc.sh
```

### Run GUI (when ready)
```bash
cd GUI
python simple_gui.py
```

## File Structure

```
Autosar_SecOC/
├── source/PQC/
│   ├── PQC.c                    # ML-KEM & ML-DSA wrapper
│   └── PQC_KeyExchange.c        # Key exchange manager
├── include/PQC/
│   ├── PQC.h
│   ├── PQC_KeyExchange.h
│   └── SecOC_PQC_Cfg.h          # PQC configuration
├── source/Csm/Csm.c             # Updated with signatures
├── source/SecOC/SecOC.c         # PQC authenticate/verify
├── source/GUIInterface/         # PQC GUI functions ready
├── test_pqc.c                   # Functionality tests
├── test_performance.c           # Benchmarks
└── external/liboqs/             # PQC library
```

## Key Functions

### PQC Core
```c
PQC_Init()                       // Initialize PQC module
PQC_MLKEM_KeyGen()              // Generate ML-KEM keypair
PQC_MLKEM_Encapsulate()         // Create shared secret
PQC_MLKEM_Decapsulate()         // Extract shared secret
PQC_MLDSA_Sign()                // Generate signature
PQC_MLDSA_Verify()              // Verify signature
```

### CSM Layer
```c
Csm_SignatureGenerate()         // ML-DSA sign
Csm_SignatureVerify()           // ML-DSA verify
```

### SecOC
```c
authenticate_PQC()              // PQC authentication
verify_PQC()                    // PQC verification
```

### Key Exchange
```c
PQC_KeyExchange_Init()
PQC_KeyExchange_Initiate()      // Alice: send public key
PQC_KeyExchange_Respond()       // Bob: send ciphertext
PQC_KeyExchange_Complete()      // Alice: extract secret
PQC_KeyExchange_GetSharedSecret()
```

### GUI Interface
```c
GUIInterface_authenticate_PQC()
GUIInterface_verify_PQC()
```

## Configuration

Edit `include/SecOC/SecOC_PQC_Cfg.h`:

```c
#define SECOC_USE_PQC_MODE           TRUE   // Enable PQC
#define SECOC_USE_MLKEM_KEY_EXCHANGE TRUE   // Enable key exchange
#define SECOC_ETHERNET_GATEWAY_MODE  TRUE   // Ethernet focus
```

## Algorithm Details

**ML-KEM-768** (NIST Level 3):
- Public Key: 1,184 bytes
- Ciphertext: 1,088 bytes
- Shared Secret: 32 bytes

**ML-DSA-65** (NIST Level 3):
- Public Key: 1,952 bytes
- Signature: 3,309 bytes (vs 4 bytes MAC)
- Security: Quantum-resistant

## Next Steps

1. Run tests: `bash build_test_pqc.sh`
2. Benchmark: `bash build_perf.sh`
3. Update GUI for PQC toggle
4. Test end-to-end flow
5. Deploy to Ethernet Gateway hardware

## Status

- ✅ PQC library integration
- ✅ Signature operations
- ✅ Key exchange
- ✅ Tests passing
- ⏳ GUI integration
- ⏳ End-to-end testing
