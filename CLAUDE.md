# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Repository Overview

This is a bachelor's graduation project implementing the AUTOSAR SecOC (Secure On-Board Communication) module with Post-Quantum Cryptography (PQC) support. The project provides authentication and integrity verification for automotive in-vehicle network communications, with recent enhancements for quantum-resistant security.

**Main Directory:** All source code and development work is located in `Autosar_SecOC/`

**Key Features:**
- AUTOSAR R21-11 compliant SecOC implementation
- MISRA C:2012 coding standards
- Post-Quantum Cryptography support (ML-KEM-768 and ML-DSA-65)
- Support for CAN, CAN-TP, FlexRay, and Ethernet communications
- Python GUI for testing and demonstration
- Comprehensive test suite using Google Test

## Project Structure

```
computer_engineering_project/
├── Autosar_SecOC/           # Main project directory (work here)
│   ├── source/              # C source files
│   │   ├── SecOC/          # Core SecOC module
│   │   ├── PQC/            # Post-Quantum Cryptography wrappers
│   │   ├── Csm/            # Crypto Service Manager
│   │   ├── PduR/           # PDU Router
│   │   ├── Can/            # CAN interface/transport
│   │   ├── SoAd/           # Socket Adapter (Ethernet)
│   │   ├── Ethernet/       # Ethernet (Linux-specific)
│   │   └── GUIInterface/   # GUI bridge functions
│   ├── include/            # Header files (mirrors source structure)
│   ├── test/               # Google Test unit tests
│   ├── GUI/                # Python GUI application
│   ├── external/           # Third-party libraries
│   │   └── liboqs/         # Open Quantum Safe library
│   ├── build/              # CMake build directory
│   └── CMakeLists.txt      # Build configuration
└── CLAUDE.md               # This file
```

## Common Development Commands

### Building the Project

**Standard build (MinGW on Windows):**
```bash
cd Autosar_SecOC
bash rebuild_pqc.sh
```

**Manual build:**
```bash
cd Autosar_SecOC/build
cmake -G "MinGW Makefiles" ..
mingw32-make -j4
```

**On Linux (target platform - Raspberry Pi):**
```bash
cd Autosar_SecOC
mkdir -p build && cd build
cmake -G "Unix Makefiles" ..
make -j4
```

### Testing

**Run all tests:**
```bash
cd Autosar_SecOC/build
ctest
```

**Run specific test:**
```bash
cd Autosar_SecOC/build
./AuthenticationTests.exe      # Windows
./VerificationTests.exe
./FreshnessTests.exe
./SecOCTests.exe
# etc.
```

**PQC functionality test:**
```bash
cd Autosar_SecOC
bash build_test_pqc.sh
```

**Performance benchmark:**
```bash
cd Autosar_SecOC
bash build_perf.sh
```

### Running the GUI

```bash
cd Autosar_SecOC/GUI
python simple_gui.py           # Original GUI
python simple_gui_pqc.py       # PQC-enhanced GUI
```

**Requirements:** PySide2 (Qt for Python)

### Building liboqs (PQC library)

If liboqs needs to be rebuilt:
```bash
cd Autosar_SecOC
bash build_liboqs.sh
```

Manual build:
```bash
cd Autosar_SecOC/external/liboqs
mkdir -p build && cd build
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..
mingw32-make -j4
```

## High-Level Architecture

### AUTOSAR Layered Architecture

```
Application Layer (COM)
         ↕
    SecOC Module (Authentication/Verification Layer)
         ↕ [PQC Mode: ML-DSA signatures OR Classic Mode: HMAC]
  PduR (PDU Router) - Central routing hub
         ↕
  ┌──────┴────────┬────────────┐
  ↓               ↓             ↓
CAN Stack      Ethernet      FlexRay
(CanIF/CanTP)    (SoAd)       Stack
```

### SecOC Operation Flow

**Transmission (Tx):**
1. Application layer provides Authentic I-PDU to SecOC
2. SecOC requests freshness value from FVM (Freshness Value Manager)
3. **Classic Mode:** CSM generates MAC (Message Authentication Code)
   **PQC Mode:** CSM generates ML-DSA digital signature
4. SecOC constructs Secured I-PDU = Authentic PDU + Freshness + MAC/Signature
5. PduR routes Secured I-PDU to appropriate transport layer

**Reception (Rx):**
1. Transport layer delivers Secured I-PDU to SecOC via PduR
2. SecOC parses PDU, extracts MAC/Signature and Freshness
3. FVM validates freshness (prevents replay attacks)
4. **Classic Mode:** CSM verifies MAC
   **PQC Mode:** CSM verifies ML-DSA signature
5. If valid, SecOC forwards Authentic I-PDU to application layer

### Post-Quantum Cryptography Integration

**Algorithms Used:**
- **ML-KEM-768** (NIST FIPS 203): Key encapsulation for secure key exchange
- **ML-DSA-65** (NIST FIPS 204): Digital signatures for authentication

**Key Differences from Classic Mode:**

| Aspect | Classic Mode | PQC Mode |
|--------|--------------|----------|
| Authentication | HMAC (4 bytes) | ML-DSA Signature (~3,309 bytes) |
| Key Management | Static shared keys | ML-KEM key exchange |
| Freshness | 8-bit counter | 64-bit counter |
| Transmission | IF or TP mode | TP mode required (large signatures) |
| Security | Classical attacks | Quantum-resistant |

**PQC Module Components:**
- `source/PQC/PQC.c`: ML-KEM and ML-DSA wrapper functions
- `source/PQC/PQC_KeyExchange.c`: Multi-peer key exchange manager
- `include/PQC/SecOC_PQC_Cfg.h`: PQC configuration parameters
- `external/liboqs/`: Open Quantum Safe library (PQC implementations)

## Key Configuration Files

1. **SecOC_Cfg.h** - Pre-compile configuration
   - Main function periods
   - Number of Tx/Rx PDUs
   - Development error detection
   - PQC mode enable/disable

2. **SecOC_PBcfg.c/.h** - Post-build configuration
   - PDU processing configurations (Tx and Rx)
   - Freshness value lengths
   - Authenticator lengths
   - MAC/Signature algorithm selection

3. **SecOC_Lcfg.c/.h** - Link-time configuration
   - Per-PDU counters and state structures

4. **SecOC_PQC_Cfg.h** - PQC-specific configuration
   - Algorithm selection (ML-KEM-768, ML-DSA-65)
   - Key sizes
   - PQC mode toggles

## Development Workflow

### Working Directory
Always work from the `Autosar_SecOC/` directory, not the repository root.

### Making Changes to SecOC Logic

1. Edit source files in `source/SecOC/`
2. Rebuild: `cd build && mingw32-make`
3. Run relevant tests: `ctest -R <TestName>`
4. If tests pass, run GUI for integration testing

### Adding PQC Features

1. Modify PQC wrapper in `source/PQC/`
2. Update CSM layer if needed (`source/Csm/Csm.c`)
3. Test with `build_test_pqc.sh`
4. Run performance benchmark with `build_perf.sh`
5. Update GUI for new features

### Adding New Tests

1. Create `test/NewTests.cpp`
2. Add executable in `test/CMakeLists.txt`:
   ```cmake
   add_executable(NewTests NewTests.cpp)
   target_link_libraries(NewTests SecOCLib gtest gtest_main)
   add_test(NAME NewTests COMMAND NewTests)
   ```
3. Rebuild and run: `cd build && mingw32-make && ctest`

## Coding Standards

The project strictly follows **AUTOSAR BSWGeneral** naming conventions and **MISRA C:2012** rules.

### Naming Conventions

**Module Prefix:** `SecOC` (mixed case) or `SECOC` (uppercase)

| Element | Pattern | Example |
|---------|---------|---------|
| Functions | `SecOC_FunctionName()` | `SecOC_Init()` |
| Types | `SecOC_TypeNameType` | `SecOC_ConfigType` |
| Enums | `SecOC_EnumName_Type` | `SecOC_PduType_Type` |
| Enum Literals | `SECOC_LITERAL_NAME` | `SECOC_IFPDU` |
| Defines | `SECOC_CONSTANT_NAME` | `SECOC_BUFFER_LENGTH` |
| Global Variables | `SecOC_VariableName` | `SecOC_MessageBuffer` |
| Error Values | `SECOC_E_ERROR_NAME` | `SECOC_E_PARAM_POINTER` |
| Config Parameters | `SecOC<ParameterName>` | `SecOCTxPduProcessingId` |

**Example Structure:**
```c
// Type definition
typedef struct {
    uint16 SecOCMessageLinkLen;
    uint16 SecOCMessageLinkPos;
} SecOC_UseMessageLinkType;

// Enumeration
typedef enum {
    SECOC_IFPDU,    // Interface PDU
    SECOC_TPPDU     // Transport Protocol PDU
} SecOC_PduType_Type;

// Function
Std_ReturnType SecOC_IfTransmit(
    PduIdType TxPduId,
    const PduInfoType* PduInfoPtr
);
```

### MISRA Compliance

- Use CppCheck extension with MISRA C:2012 rules enabled
- CMake generates `build/cppcheck_includes.txt` for analysis
- Avoid dynamic memory allocation
- Use const for read-only data
- All functions must have explicit return types

## Platform Differences

### Windows vs Linux

**Windows (Development):**
- Uses MinGW for building
- Ethernet implementation: `source/Ethernet/ethernet_windows.c`
- Winsock2 for sockets
- Scheduler excluded from build

**Linux (Target - Raspberry Pi):**
- Uses GCC
- Ethernet implementation: `source/Ethernet/ethernet.c`
- POSIX sockets
- Scheduler included: `source/Scheduler/scheduler.c`
- pthreads for threading

**CMake automatically handles platform differences** based on `WIN32` or `UNIX` detection.

## Important Documentation

### In Repository
- `Autosar_SecOC/README.md` - Project overview, installation, features
- `Autosar_SecOC/CLAUDE.md` - Detailed architecture (more specific than this file)
- `Autosar_SecOC/QUICK_START.md` - PQC quick reference
- `Autosar_SecOC/PQC_RESEARCH.md` - PQC theory and design decisions
- `Autosar_SecOC/PQC_IMPLEMENTATION_ROADMAP.md` - Step-by-step PQC implementation guide
- `Autosar_SecOC/source/Code_Style.md` - AUTOSAR naming conventions
- `Autosar_SecOC/GUI/PQC_MODE_INSTRUCTIONS.md` - GUI PQC usage
- `Autosar_SecOC/GUI/PQC_TESTING_GUIDE.md` - PQC testing procedures

### External References
- AUTOSAR SecOC SWS R21-11: https://www.autosar.org/
- NIST FIPS 203 (ML-KEM): https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.203.pdf
- NIST FIPS 204 (ML-DSA): https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.204.pdf
- liboqs Documentation: https://github.com/open-quantum-safe/liboqs

## Troubleshooting

### Build Issues

**"liboqs not found":**
```bash
cd Autosar_SecOC
bash build_liboqs.sh
```

**CMake cache issues:**
```bash
cd Autosar_SecOC/build
rm -rf CMakeFiles CMakeCache.txt
cmake -G "MinGW Makefiles" ..
```

**Test compilation errors:**
- Ensure Google Test is fetched: CMake does this automatically via `FetchContent`
- Check that SecOCLib is built before tests

### Runtime Issues

**"Cannot find liboqs.dll" (Windows):**
- Ensure `external/liboqs/build/bin/liboqs.dll` exists
- Copy DLL to same directory as executable

**GUI not launching:**
```bash
pip install PySide2
```

**Large PDU transmission fails:**
- Ensure using TP mode, not IF mode
- Check PDU buffer sizes in configuration

### PQC-Specific Issues

**Signature verification fails:**
- Ensure public/private key pair matches
- Check freshness counter synchronization
- Verify signature length matches `MLDSA65_SIGNATURE_BYTES`

**Key exchange not completing:**
- Check network connectivity
- Verify both peers have correct public keys
- Ensure ciphertext is transmitted completely

## PQC Mode vs Classic Mode

The project supports both modes, selectable at compile time:

**Enable PQC Mode:**
```c
// In include/SecOC/SecOC_PQC_Cfg.h
#define SECOC_USE_PQC_MODE           TRUE
```

**Disable PQC Mode (Classic MAC-based):**
```c
#define SECOC_USE_PQC_MODE           FALSE
```

**When to use PQC Mode:**
- Quantum-resistant security required
- Ethernet-based communication (due to large signature sizes)
- Long-term security needs (quantum computers threat)
- Compliance with future NIST standards

**When to use Classic Mode:**
- CAN/CAN-TP/FlexRay (limited bandwidth)
- Resource-constrained ECUs
- Backward compatibility required
- Lower latency critical

## Performance Considerations

### PQC Performance (approximate on modern x86):
- ML-KEM-768 KeyGen: ~20 µs
- ML-KEM-768 Encap: ~30 µs
- ML-KEM-768 Decap: ~40 µs
- ML-DSA-65 Sign: ~250 µs (vs ~2 µs for HMAC)
- ML-DSA-65 Verify: ~120 µs

**Impact:** PQC adds ~370 µs latency per message (sign + verify)

**Mitigation Strategies:**
- Use asynchronous signing/verification
- Batch sign multiple messages
- Hardware acceleration (if available)
- Selective signing (only critical messages)

## Testing Strategy

### Unit Tests (`test/` directory)
- `AuthenticationTests.cpp`: MAC/signature generation
- `VerificationTests.cpp`: MAC/signature verification
- `FreshnessTests.cpp`: Freshness value management
- `DirectTxTests.cpp`: Direct transmission (IF mode)
- `DirectRxTests.cpp`: Direct reception (IF mode)
- `startOfReceptionTests.cpp`: TP reception initiation
- `SecOCTests.cpp`: Integration tests

### PQC Tests
- `test_pqc.c`: PQC functionality tests (ML-KEM, ML-DSA, key exchange)
- `test_performance.c`: Benchmarking PQC operations

### GUI Testing
- Interactive testing of different PDU configurations
- Attack simulations (replay, tampering)
- Real-time visualization of SecOC behavior

### Test Execution Flow
1. Unit tests → Verify individual components
2. Integration tests → Verify end-to-end flow
3. PQC tests → Verify quantum-resistant operations
4. Performance tests → Measure latency and throughput
5. GUI tests → Validate user-facing functionality
6. Attack tests → Ensure security properties hold

## Future Work

**Unimplemented AUTOSAR Features:** See README.md "Future Work" section for list of SWS requirements not yet implemented.

**PQC Enhancements:**
- Hardware acceleration support
- Hybrid mode (PQC + classical for backward compatibility)
- Multi-ECU key management system
- Formal verification of PQC integration
- SIMD/assembly optimizations
- Compressed signature schemes

## Contributors

This is a graduation project by six senior Computer and Automatic Controls students from Tanta University, Faculty of Engineering, under supervision of Prof. Dr. Wael Mohammed AlAwadi and Eng. Ahmed Elsaka.

## License

MIT License - See LICENSE file

## Important Notes for Claude Code

1. **Always work from `Autosar_SecOC/` directory** when making code changes
2. **Rebuild after any source changes:** `cd build && mingw32-make`
3. **Run tests after changes:** `ctest` or individual test executables
4. **Check MISRA compliance** using CppCheck extension
5. **Follow AUTOSAR naming conventions** strictly (see Code_Style.md)
6. **Document configuration changes** in relevant config files
7. **Test both PQC and Classic modes** if touching core SecOC logic
8. **Use build scripts** (`rebuild_pqc.sh`, `build_test_pqc.sh`) for consistency
9. **Update GUI** if changing PDU structure or adding new features
10. **Performance test** PQC changes with `build_perf.sh`

## Quick Reference

| Task | Command |
|------|---------|
| Full rebuild | `cd Autosar_SecOC && bash rebuild_pqc.sh` |
| Run all tests | `cd Autosar_SecOC/build && ctest` |
| Test PQC | `cd Autosar_SecOC && bash build_test_pqc.sh` |
| Benchmark | `cd Autosar_SecOC && bash build_perf.sh` |
| Launch GUI | `cd Autosar_SecOC/GUI && python simple_gui.py` |
| Rebuild liboqs | `cd Autosar_SecOC && bash build_liboqs.sh` |

**Main Documentation:** See `Autosar_SecOC/CLAUDE.md` for detailed architecture and API reference.
