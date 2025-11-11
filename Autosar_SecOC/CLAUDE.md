# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is an implementation of the AUTOSAR SecOC (Secure On-Board Communication) module, which provides authentication and integrity verification for in-vehicle network communications. The project follows AUTOSAR R21-11 specifications and MISRA C:2012 coding standards.

SecOC operates at the PDU (Protocol Data Unit) level, adding authentication information (MAC + optional Freshness Value) to outgoing messages and verifying incoming messages to protect against attacks like replay and man-in-the-middle.

## Build System

This project uses CMake (minimum version 3.20) with platform-specific configurations:

### Building on Windows
```bash
mkdir build
cd build
cmake -G "MinGW Makefiles" ..
make
```

### Building on Linux (Primary Target - Raspberry Pi)
```bash
mkdir build
cd build
cmake -G "Unix Makefiles" ..
make
```

Note: On Windows, Ethernet-related files (ethernet.c/h, scheduler.c/h) are excluded from the build via CMake configuration.

### Build Targets
- **SecOC**: Main executable
- **SecOCLib**: Static library for linking with Google Test
- **SecOCLibShared**: Shared library for linking with the GUI frontend

## Testing

The project uses Google Test (version 1.11.0) fetched automatically by CMake.

### Running Tests
```bash
cd build
ctest
# Or run individual test executables:
./AuthenticationTests
./VerificationTests
./FreshnessTests
./SecOCTests
```

### Test Files Location
All test files are in `test/` directory:
- `AuthenticationTests.cpp`: MAC generation tests
- `VerificationTests.cpp`: MAC verification tests
- `FreshnessTests.cpp`: Freshness value management tests
- `DirectTxTests.cpp`: Direct transmission tests
- `DirectRxTests.cpp`: Direct reception tests
- `startOfReceptionTests.cpp`: TP reception initiation tests
- `SecOCTests.cpp`: Integration tests

Each test file is compiled into a separate executable automatically by `test/CMakeLists.txt`.

## Architecture

### Module Hierarchy (AUTOSAR Layered Architecture)

```
Application Layer (COM)
         ↕
    SecOC Module (Authentication/Verification)
         ↕
  PduR (PDU Router) - Central routing component
         ↕
  ┌──────┴──────┐
  ↓             ↓
CAN Stack    Ethernet Stack
(CanIF/CanTP)   (SoAd)
```

### Key Components

**SecOC Module** (`source/SecOC/`, `include/SecOC/`)
- `SecOC.c/h`: Core authentication and verification logic
- `FVM.c/h`: Freshness Value Manager - manages counters for replay protection
- `SecOC_Cfg.h`: Static configuration (compile-time parameters)
- `SecOC_PBcfg.c/h`: Post-Build configuration (runtime PDU configurations)
- `SecOC_Lcfg.c/h`: Link-time configuration

**PduR** (`source/PduR/`, `include/PduR/`)
- Central routing layer connecting SecOC with transport protocols
- `Pdur_SecOC.c`: Routing between SecOC and lower layers
- `PduR_Com.c`: Routing to/from COM layer
- `PduR_CanIf.c`, `Pdur_CanTP.c`: CAN interface routing
- `PduR_SoAd.c`: Ethernet socket adapter routing

**CSM (Crypto Service Manager)** (`source/Csm/`, `include/Csm/`)
- Abstraction layer for cryptographic operations
- `Csm_MacGenerate()`: Generate Message Authentication Code
- `Csm_MacVerify()`: Verify MAC authenticity

**Transport Layers**
- `Can/`: CAN interface (CanIF) and transport protocol (CanTP)
- `SoAd/`: Socket Adapter for Ethernet communication
- `Ethernet/`: Ethernet-specific functionality (Linux only)

**Supporting Modules**
- `Com/`: COM layer for application data
- `Dcm/`: Diagnostic Communication Manager
- `Encrypt/`: Encryption utilities
- `Scheduler/`: Task scheduling for Linux
- `GUIInterface/`: Bridge to Python GUI

### Transmission Flow (Tx)
1. `COM` → `SecOC_IfTransmit()` with Authentic I-PDU
2. SecOC copies PDU, requests freshness value from FVM
3. SecOC constructs DataToAuthenticator (Authentic PDU + Freshness)
4. CSM generates MAC via `Csm_MacGenerate()`
5. SecOC builds Secured I-PDU (Authentic PDU + MAC + Freshness bits)
6. PduR routes to CanIF/CanTP/SoAd based on configuration

### Reception Flow (Rx)
1. CanIF/CanTP/SoAd → PduR → SecOC with Secured I-PDU
2. SecOC parses MAC and Freshness from Secured I-PDU
3. FVM provides complete freshness value for verification
4. SecOC reconstructs DataToAuthenticator
5. CSM verifies MAC via `Csm_MacVerify()`
6. If valid, SecOC forwards Authentic I-PDU to upper layer via PduR

### Supported Transmission Modes
- **Direct Mode (IF)**: Single-frame transmission without segmentation
- **TP Mode**: Multi-frame transmission with segmentation/reassembly for large PDUs

## Coding Standards

The project strictly follows AUTOSAR naming conventions and MISRA C:2012 rules. See `source/Code_Style.md` for detailed conventions:

### Naming Conventions
- **Module prefix**: `SecOC` (mixed case), `SECOC` (uppercase for defines/enums)
- **Functions**: `SecOC_FunctionName()` (CamelCase after prefix)
- **Types**: `SecOC_TypeNameType` (always ends with "Type")
- **Config Types**: `SecOC_ContainerNameType`
- **Enums**: `SecOC_EnumName_Type` with members like `SECOC_LITERAL_NAME`
- **Defines**: `SECOC_CONSTANT_NAME` (all caps with underscores)
- **Global Variables**: `SecOC_VariableName` (CamelCase after prefix)
- **Error Values**: `SECOC_E_ERROR_NAME`

### Example Patterns
```c
// Configuration struct
typedef struct {
    uint16 SecOCMessageLinkLen;
    uint16 SecOCMessageLinkPos;
} SecOC_UseMessageLinkType;

// Enumeration
typedef enum {
    SECOC_IFPDU,
    SECOC_TPPDU
} SecOC_PduType_Type;

// Preprocessor define
#define SECOC_AUTH_PDUHEADER_LENGTH ((uint8)0)
```

### MISRA Compliance
- Use CppCheck extension with MISRA C:2012 rules enabled
- CMake generates `build/cppcheck_includes.txt` with include paths for CppCheck analysis

## GUI Frontend

A Python-based GUI (`GUI/` directory) provides interactive testing capabilities using PySide2 (Qt):

**Running the GUI**:
```bash
cd GUI
python main.py
```

The GUI links against the SecOCLibShared library and allows:
- Configuring PDU parameters (header/no-header, IF/TP mode)
- Testing different attacks (altered MAC, replay attacks)
- Visualizing SecOC behavior in real-time

**GUI Files**:
- `main.py`: Entry point with dark theme setup
- `mydialog.py`: Main dialog window logic
- `connections.py`: Connects GUI to C library via ctypes
- `GUI.ui`, `ui.py`: Qt Designer interface

## Configuration System

SecOC uses AUTOSAR's three-tier configuration:

1. **Pre-compile (SecOC_Cfg.h)**: Compile-time constants
   - `SECOC_MAIN_FUNCTION_PERIOD_TX/RX`: Main function periods
   - `SECOC_DEV_ERROR_DETECT`: Enable DET reporting
   - `SECOC_NUM_OF_TX_PDU_PROCESSING`: Number of Tx PDUs

2. **Link-time (SecOC_Lcfg.c/h)**: PDU structures and counters
   - `SecOC_TxCountersType`, `SecOC_RxCountersType`: Per-PDU state
   - Linkable but not runtime-changeable

3. **Post-build (SecOC_PBcfg.c/h)**: Full PDU configurations
   - `SecOC_ConfigType`: Root configuration structure
   - `SecOC_TxPduProcessingType[]`: Tx PDU array
   - `SecOC_RxPduProcessingType[]`: Rx PDU array
   - Passed to `SecOC_Init()`

## Development Workflow

### Making Changes to SecOC Logic
1. Edit source files in `source/SecOC/`
2. Rebuild: `cd build && make`
3. Run relevant tests: `ctest -R <TestName>`
4. Check MISRA compliance via CppCheck extension

### Adding New PDU Configurations
1. Modify `source/SecOC/SecOC_PBcfg.c` with new PDU structures
2. Update `SECOC_NUM_OF_TX/RX_PDU_PROCESSING` in `SecOC_Cfg.h`
3. Add corresponding test cases

### Debugging
- Use `#define SECOC_DEBUG` to enable printf debugging
- Set `#ifdef RELEASE` to make internal functions `static` for production

## Documentation

- **AUTOSAR Specification**: [AUTOSAR SecOC SWS R21-11](https://www.autosar.org/)
- **Graduation Book**: Comprehensive project documentation available in README
- **Supported Features**: See README.md tables listing implemented SWS requirements
- **Future Work**: Unimplemented SWS requirements documented in README for potential enhancement

## Important Notes

- **Platform Differences**: Ethernet modules are only built on Linux
- **CMake Definitions**: `WINDOWS` or `LINUX` preprocessor symbols set automatically
- **Thread Safety**: Project uses pthreads (linked via CMake's `Threads` package)
- **Static Analysis**: Include paths auto-exported to `build/cppcheck_includes.txt`
- **Test Independence**: Each test .cpp file becomes a separate executable for isolation
