# AUTOSAR SecOC with Post-Quantum Cryptography
## Ethernet Gateway - Computer Engineering Project

**Institution:** Ho Chi Minh City University of Technology (HCMUT)
**Project Type:** Bachelor's Thesis - Computer Engineering
**Date:** November 2025

---

## Quick Start

### For Thesis Reporting (Recommended)

```bash
cd Autosar_SecOC
bash build_and_run.sh report
```

**This generates:**
- `test_summary.txt` - Complete test results summary
- `pqc_standalone_results.csv` - PQC performance metrics
- `pqc_advanced_results.csv` - Integration test performance
- Terminal output with all test results

---

## Documentation

### 📄 TECHNICAL_REPORT.md
**Complete technical documentation for thesis**
- System architecture
- PQC integration details
- Performance analysis
- Security validation
- Implementation guide

### 📊 DIAGRAMS.md
**All architecture diagrams (Mermaid format)**
- Ethernet Gateway system overview
- Signal flow diagrams
- PQC integration architecture
- Test coverage diagrams

---

## Project Overview

This project implements **Post-Quantum Cryptography (PQC)** in an AUTOSAR SecOC Ethernet Gateway to protect automotive communication against future quantum computer threats.

### Key Features

✅ **NIST-standardized PQC algorithms:**
- ML-KEM-768 (FIPS 203): Quantum-resistant key exchange
- ML-DSA-65 (FIPS 204): Post-quantum digital signatures

✅ **Ethernet Gateway Architecture:**
- Bridges CAN bus (vehicle ECUs) to Ethernet network
- PQC signatures for Ethernet communication (3309 bytes)
- Classical MAC for CAN bus (backward compatibility)

✅ **Complete AUTOSAR implementation:**
- SecOC module with freshness management
- Crypto Service Manager (Csm) integration
- PDU Router for multi-transport support

✅ **Comprehensive testing:**
- 38 unit tests (Google Test framework)
- PQC algorithm validation (6000+ iterations)
- Performance benchmarking
- Security validation (tampering detection)

---

## System Architecture

```
CAN Network          Ethernet Gateway (Raspberry Pi 4)           Backend Network
┌─────────┐          ┌──────────────────────────────┐          ┌──────────────┐
│ Engine  │          │  AUTOSAR SecOC Stack         │          │ Central ECU  │
│ Brake   │──CAN──>  │  - SecOC (PQC signatures)    │──ETH──>  │ Telematics   │
│ Steering│          │  - Csm (ML-DSA-65)           │          │ Cloud Gateway│
└─────────┘          │  - PduR (Multi-transport)    │          └──────────────┘
                     └──────────────────────────────┘
Classical MAC        Dual-mode authentication         PQC Signatures
(4 bytes)            (CAN: Classical, ETH: PQC)       (3309 bytes)
```

---

## Test Results Summary

### Google Test Suite: 100% Pass Rate
- **8 test suites** (38 individual tests)
- AuthenticationTests, VerificationTests, FreshnessTests
- **PQC_ComparisonTests (13 tests)** - Thesis contribution

### PQC Standalone Tests: 100% Success
- **ML-KEM-768:** 1000 iterations (KeyGen, Encap, Decap)
- **ML-DSA-65:** 5000 iterations (Sign, Verify)

### Integration Tests: 100% Pass
- Csm layer PQC integration validated
- Classical vs PQC comparison successful
- Security tests: 100% tampering detection (2/2)

---

## Performance Metrics

| Algorithm | Operation | Avg Time | Throughput |
|-----------|-----------|----------|------------|
| ML-KEM-768 | KeyGen | ~81 µs | 12,323 ops/sec |
| ML-KEM-768 | Encapsulate | ~81 µs | 12,410 ops/sec |
| ML-KEM-768 | Decapsulate | ~27 µs | 36,775 ops/sec |
| ML-DSA-65 | Sign | ~377 µs | 2,652 ops/sec |
| ML-DSA-65 | Verify | ~85 µs | 11,783 ops/sec |

**Signature Size:** ML-DSA-65 = 3309 bytes (vs Classical MAC = 4 bytes)

---

## Build & Test

### Prerequisites
- **Windows (Development):** MinGW-w64, CMake, Python 3
- **Linux (Deployment):** GCC, CMake, liboqs

### Build System
```bash
cd Autosar_SecOC
bash build_and_run.sh report  # Run all tests + generate report
```

### Test Options
```bash
bash build_and_run.sh thesis      # Interactive validation (phase-by-phase)
bash build_and_run.sh googletest  # Run only Google Test suite
bash build_and_run.sh report      # Complete validation + report (RECOMMENDED)
```

---

## Project Structure

```
Autosar_SecOC/
├── source/                    # C source files
│   ├── SecOC/                # Core SecOC module
│   ├── PQC/                  # Post-Quantum Crypto wrappers
│   ├── Csm/                  # Crypto Service Manager
│   ├── PduR/                 # PDU Router
│   └── Ethernet/             # Ethernet stack
├── include/                   # Header files
├── test/                      # Google Test unit tests
├── external/liboqs/           # Open Quantum Safe library
├── build_and_run.sh           # Main build & test script
├── TECHNICAL_REPORT.md        # Complete technical documentation
├── DIAGRAMS.md                # Architecture diagrams
└── README.md                  # This file
```

---

## Key Implementation Files

### Core SecOC Module
- `source/SecOC/SecOC.c` - Main SecOC logic
- `source/SecOC/FVM.c` - Freshness Value Manager
- `source/SecOC/SecOC_Lcfg.c` - Transport configuration

### PQC Integration
- `source/PQC/PQC.c` - ML-KEM & ML-DSA wrappers
- `source/PQC/PQC_KeyExchange.c` - Multi-peer key exchange
- `source/Csm/Csm.c` - Cryptographic Service Manager

### Configuration
- `include/SecOC/SecOC_PQC_Cfg.h` - PQC configuration
- `include/SecOC/SecOC_PBcfg.h` - Post-build configuration

### Tests
- `test/PQC_ComparisonTests.cpp` - **Thesis contribution** (13 tests)
- `test_pqc_standalone.c` - PQC algorithm validation
- `test_pqc_secoc_integration.c` - Csm layer integration

---

## Configuration

### Ethernet Gateway Mode (Enabled)
```c
// include/SecOC/SecOC_PQC_Cfg.h
#define SECOC_ETHERNET_GATEWAY_MODE     TRUE
#define SECOC_USE_PQC_MODE              TRUE
#define SECOC_PQC_MAX_PDU_SIZE          8192U
```

### Multi-Transport Support
- **CAN Interface:** Classical MAC (4 bytes)
- **CAN Transport:** Classical MAC (segmented)
- **Socket Adapter (Ethernet):** PQC ML-DSA-65 (3309 bytes)

---

## Security Features

✅ **Post-quantum resistance:** ML-DSA-65 digital signatures
✅ **Replay attack prevention:** 64-bit freshness counters
✅ **Tampering detection:** 100% detection rate (validated)
✅ **Backward compatibility:** Classical MAC for CAN
✅ **Key exchange security:** ML-KEM-768 encapsulation

---

## Research Question

**"Can Post-Quantum Cryptography be successfully integrated into AUTOSAR SecOC module for quantum-resistant automotive security?"**

### Answer: ✅ YES

**Validation Results:**
- ✅ Phase 1: PQC Fundamentals - PASSED
- ✅ Phase 2: Classical vs PQC Comparison - PASSED (38/38 tests)
- ✅ Phase 3: AUTOSAR Integration - PASSED
- ✅ Phase 4: System Validation - PASSED

---

## License

MIT License - See LICENSE file

---

## Contact

**Project Team:** Computer Engineering Students
**Institution:** HCMUT (Ho Chi Minh City University of Technology)
**Supervisor:** Prof. Dr. [Supervisor Name]

---

## References

1. NIST FIPS 203 (ML-KEM): [https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.203.pdf](https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.203.pdf)
2. NIST FIPS 204 (ML-DSA): [https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.204.pdf](https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.204.pdf)
3. AUTOSAR SecOC SWS R21-11: [https://www.autosar.org/](https://www.autosar.org/)
4. Open Quantum Safe (liboqs): [https://github.com/open-quantum-safe/liboqs](https://github.com/open-quantum-safe/liboqs)

---

**Last Updated:** November 15, 2025
**Version:** 2.0 (Thesis Submission)
