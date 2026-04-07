# AUTOSAR SecOC with Post-Quantum Cryptography
## Complete System Architecture Documentation

**Project:** Bachelor's Graduation Project - Ethernet Gateway with PQC
**Platform:** Raspberry Pi 4 (ARM Cortex-A72) / Windows Development
**AUTOSAR Version:** R21-11 Compliant
**Security:** NIST FIPS 203 (ML-KEM-768) + FIPS 204 (ML-DSA-65)

---

## Table of Contents

1. [AUTOSAR Layered Architecture](#1-autosar-layered-architecture)
2. [Complete Module Inventory](#2-complete-module-inventory)
3. [Layer-by-Layer Analysis](#3-layer-by-layer-analysis)
4. [Data Flow Analysis](#4-data-flow-analysis)
5. [SecOC Core Architecture](#5-secoc-core-architecture)
6. [PQC Integration Architecture](#6-pqc-integration-architecture)
7. [Routing and Transport](#7-routing-and-transport)
8. [Configuration Architecture](#8-configuration-architecture)
9. [Security Mechanisms](#9-security-mechanisms)
10. [Performance and Optimization](#10-performance-and-optimization)
11. [Platform Abstraction](#11-platform-abstraction)
12. [API Reference](#12-api-reference)

---

# 1. AUTOSAR Layered Architecture

## 1.1 Standard AUTOSAR Architecture

According to AUTOSAR R21-11 specification, the architecture is organized in hierarchical layers:

```
┌─────────────────────────────────────────────────────────────────┐
│                    APPLICATION LAYER                            │
│  Software Components (SWCs) - Hardware Independent              │
│  Example: Vehicle control logic, user interface                 │
└────────────────────────┬────────────────────────────────────────┘
                         ↕
┌─────────────────────────────────────────────────────────────────┐
│              RUNTIME ENVIRONMENT (RTE)                          │
│  Middleware - Abstracts BSW from Application                    │
│  Provides: Sender-Receiver, Client-Server paradigms             │
└────────────────────────┬────────────────────────────────────────┘
                         ↕
┌─────────────────────────────────────────────────────────────────┐
│              BASIC SOFTWARE (BSW) - SERVICES LAYER              │
│  ┌────────────┬────────────┬────────────┬────────────────────┐  │
│  │ SecOC      │ Csm        │ FVM        │ PQC (Extension)    │  │
│  │ (Security) │ (Crypto)   │ (Fresh)    │ (Quantum-Safe)     │  │
│  ├────────────┼────────────┼────────────┼────────────────────┤  │
│  │ COM        │ DCM        │ PduR       │ BswM               │  │
│  │ (Comms)    │ (Diag)     │ (Router)   │ (Mode Mgr)         │  │
│  └────────────┴────────────┴────────────┴────────────────────┘  │
└────────────────────────┬────────────────────────────────────────┘
                         ↕
┌─────────────────────────────────────────────────────────────────┐
│         BASIC SOFTWARE - ECU ABSTRACTION LAYER                  │
│  ┌────────────┬────────────┬────────────┬────────────────────┐  │
│  │ CanIf      │ CanTp      │ SoAd       │ FrIf               │  │
│  │ (CAN IF)   │ (CAN TP)   │ (Ethernet) │ (FlexRay IF)       │  │
│  └────────────┴────────────┴────────────┴────────────────────┘  │
│  Hardware-independent interface to communication drivers        │
└────────────────────────┬────────────────────────────────────────┘
                         ↕
┌─────────────────────────────────────────────────────────────────┐
│   BASIC SOFTWARE - MICROCONTROLLER ABSTRACTION LAYER (MCAL)    │
│  ┌────────────┬────────────┬────────────┬────────────────────┐  │
│  │ Can Driver │ Eth Driver │ SPI Driver │ Scheduler          │  │
│  │ (CAN HW)   │ (Eth HW)   │ (SPI HW)   │ (pthreads)         │  │
│  └────────────┴────────────┴────────────┴────────────────────┘  │
│  Direct hardware access - Microcontroller specific              │
└─────────────────────────────────────────────────────────────────┘
```

### Layer Responsibilities

| Layer | Purpose | Hardware Dependency | Example Modules |
|-------|---------|---------------------|-----------------|
| **Application** | Business logic | None (hardware-agnostic) | User SWCs |
| **RTE** | Communication middleware | None | Auto-generated RTE |
| **Services** | System services | None | SecOC, Csm, COM, DCM |
| **ECU Abstraction** | Hardware interface abstraction | Medium (bus type) | CanIf, CanTp, SoAd |
| **MCAL** | Hardware drivers | High (microcontroller) | Can Driver, Eth Driver |

---

## 1.2 Implementation in This Project

Our implementation follows AUTOSAR architecture with **PQC extensions**:

```
┌─────────────────────────────────────────────────────────────────┐
│                    APPLICATION LAYER                            │
│  ┌──────────────────────┐      ┌────────────────────┐           │
│  │ Com (Message Layer)  │      │ GUIInterface       │           │
│  │ - TxConfirmation     │      │ - Test framework   │           │
│  │ - RxIndication       │      │ - Attack simulation│           │
│  └──────────────────────┘      └────────────────────┘           │
└────────────────────────┬────────────────────────────────────────┘
                         ↕
┌─────────────────────────────────────────────────────────────────┐
│              SERVICE/SECURITY LAYER                             │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │              SecOC (Core Security Module)                │   │
│  │  ┌────────────────────────────────────────────────────┐  │   │
│  │  │ TX Path              RX Path                       │  │   │
│  │  │ 1. IfTransmit        1. RxIndication              │  │   │
│  │  │ 2. GetTxFreshness    2. ParseSecuredPdu           │  │   │
│  │  │ 3. Authenticate      3. GetRxFreshness            │  │   │
│  │  │    (MAC or PQC)         (validate counter)        │  │   │
│  │  │ 4. BuildSecuredPdu   4. Verify (MAC or PQC)       │  │   │
│  │  │ 5. RouteToTransport  5. RouteToApplication        │  │   │
│  │  └────────────────────────────────────────────────────┘  │   │
│  └──────────────────────────────────────────────────────────┘   │
│                                                                  │
│  ┌────────────┬──────────────┬──────────────────────────────┐   │
│  │ FVM        │ Csm          │ PQC Module                   │   │
│  │ (Freshness)│ (Crypto Mgr) │ ┌────────────────────────┐   │   │
│  │ - Counter  │ - MacGen     │ │ PQC.c                  │   │   │
│  │ - Validate │ - MacVerify  │ │ - ML-DSA Sign/Verify   │   │   │
│  │ - Truncate │ - SignGen    │ │ - KeyGen               │   │   │
│  │            │ - SignVerify │ ├────────────────────────┤   │   │
│  │            │              │ │ PQC_KeyExchange.c      │   │   │
│  │            │              │ │ - ML-KEM Initiate      │   │   │
│  │            │              │ │ - ML-KEM Respond       │   │   │
│  │            │              │ │ - ML-KEM Complete      │   │   │
│  │            │              │ ├────────────────────────┤   │   │
│  │            │              │ │ PQC_KeyDerivation.c    │   │   │
│  │            │              │ │ - HKDF Extract         │   │   │
│  │            │              │ │ - HKDF Expand          │   │   │
│  │            │              │ │ - Session Keys         │   │   │
│  │            │              │ └────────────────────────┘   │   │
│  └────────────┴──────────────┴──────────────────────────────┘   │
└────────────────────────┬────────────────────────────────────────┘
                         ↕
┌─────────────────────────────────────────────────────────────────┐
│              ROUTING/ADAPTATION LAYER                           │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ PduR (PDU Router) - Central Routing Hub                 │   │
│  │ - PduR_ComTransmit     → Routes from COM                │   │
│  │ - PduR_SecOCTransmit   → Routes from SecOC              │   │
│  │ - PduR_CanIfTransmit   → Routes to CAN IF               │   │
│  │ - PduR_CanTpTransmit   → Routes to CAN TP               │   │
│  │ - PduR_SoAdTransmit    → Routes to Ethernet             │   │
│  └──────────────────────────────────────────────────────────┘   │
└────────────────────────┬────────────────────────────────────────┘
                         ↕
┌─────────────────────────────────────────────────────────────────┐
│         COMMUNICATION STACK LAYER                               │
│  ┌──────────────┬──────────────┬──────────────────────────┐     │
│  │ CanIf        │ CanTP        │ SoAd + SoAd_PQC          │     │
│  │ (CAN-IF)     │ (CAN-TP)     │ (Socket Adapter)         │     │
│  │              │              │ ┌──────────────────────┐ │     │
│  │ - Transmit   │ - Transmit   │ │ SoAd.c               │ │     │
│  │ - RxInd      │ - RxInd      │ │ - IfTransmit         │ │     │
│  │              │ - MainFuncTx │ │ - TpTransmit         │ │     │
│  │              │ - MainFuncRx │ ├──────────────────────┤ │     │
│  │              │              │ │ SoAd_PQC.c           │ │     │
│  │              │              │ │ - KeyExchange        │ │     │
│  │              │              │ │ - ML-KEM integration │ │     │
│  │              │              │ └──────────────────────┘ │     │
│  └──────────────┴──────────────┴──────────────────────────┘     │
└────────────────────────┬────────────────────────────────────────┘
                         ↕
┌─────────────────────────────────────────────────────────────────┐
│        MICROCONTROLLER ABSTRACTION LAYER (MCAL)                 │
│  ┌──────────────┬──────────────────────┬────────────────────┐   │
│  │ Can Driver   │ Ethernet Driver      │ Scheduler          │   │
│  │ (MCP2515 SPI)│ ┌──────────────────┐ │ (Linux only)       │   │
│  │              │ │ ethernet.c       │ │ - pthread          │   │
│  │              │ │ (POSIX sockets)  │ │ - ucontext         │   │
│  │              │ ├──────────────────┤ │                    │   │
│  │              │ │ ethernet_windows │ │                    │   │
│  │              │ │ (Winsock2)       │ │                    │   │
│  │              │ └──────────────────┘ │                    │   │
│  └──────────────┴──────────────────────┴────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
                         ↕
┌─────────────────────────────────────────────────────────────────┐
│              EXTERNAL DEPENDENCIES                              │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ liboqs (Open Quantum Safe)                               │   │
│  │ - OQS_KEM_ml_kem_768 (NIST FIPS 203)                     │   │
│  │ - OQS_SIG_ml_dsa_65 (NIST FIPS 204)                      │   │
│  │ - OQS_SHA2_sha256 (for HKDF)                             │   │
│  └──────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

**Key Architectural Decisions:**

1. **PQC Integration:** Added as service layer extension (not in AUTOSAR standard)
2. **Dual Ethernet Implementation:** Platform-specific (Linux vs Windows)
3. **SoAd_PQC Module:** Custom extension for ML-KEM key exchange
4. **Large Buffer Support:** 4096-byte buffers for PQC signatures (vs standard 64-byte)
5. **TP Mode Mandatory:** PQC signatures require Transport Protocol mode

---

# 2. Complete Module Inventory

## 2.1 Module Summary Table

| Module | Files | Lines | Purpose | AUTOSAR Layer |
|--------|-------|-------|---------|---------------|
| **SecOC** | SecOC.c/h, SecOC_Lcfg.c/h, SecOC_PBcfg.c/h | ~2000 | Secure onboard communication | Services |
| **FVM** | FVM.c/h | ~400 | Freshness value management | Services |
| **Csm** | Csm.c/h | ~300 | Crypto service manager | Services |
| **PQC** | PQC.c/h | ~500 | ML-DSA signature operations | Services (Extension) |
| **PQC_KeyExchange** | PQC_KeyExchange.c/h | ~450 | ML-KEM key exchange manager | Services (Extension) |
| **PQC_KeyDerivation** | PQC_KeyDerivation.c/h | ~122 | HKDF session key derivation | Services (Extension) |
| **PduR** | PduR_*.c/h (6 files) | ~600 | PDU routing | Routing/Adaptation |
| **Com** | Com.c/h | ~200 | Communication manager | Services |
| **CanIf** | CanIF.c/h | ~150 | CAN interface | ECU Abstraction |
| **CanTP** | CanTP.c/h | ~300 | CAN transport protocol | ECU Abstraction |
| **SoAd** | SoAd.c/h | ~400 | Socket adapter | ECU Abstraction |
| **SoAd_PQC** | SoAd_PQC.c/h | ~367 | PQC key exchange integration | ECU Abstraction (Extension) |
| **Ethernet** | ethernet.c/h, ethernet_windows.c/h | ~350 | Ethernet driver | MCAL |
| **Dcm** | Dcm.c/h | ~50 | Diagnostic communication | Services |
| **Scheduler** | scheduler.c/h | ~200 | Task scheduler (Linux) | Platform Services |
| **GUIInterface** | GUIInterface.c/h | ~300 | Python GUI bridge | Application/Test |
| **Encrypt** | encrypt.c/h | ~200 | AES-128 encryption | Crypto Utility |
| **ComStack_Types** | ComStack_Types.h | ~100 | AUTOSAR standard types | Type Definitions |
| **Std_Types** | Std_Types.h | ~50 | Standard AUTOSAR types | Type Definitions |

**Total Production Code:** ~7,000 lines (excluding tests)

---

## 2.2 Detailed Module Descriptions

### SecOC (Secure Onboard Communication)

**Purpose:** Main security module providing authentication and integrity verification

**Key Responsibilities:**
- Receive Authentic I-PDU from upper layers
- Request freshness values from FVM
- Generate/verify MAC or PQC signatures via Csm
- Construct/parse Secured I-PDUs
- Route secured PDUs to transport layers
- Detect replay and tampering attacks

**Key Data Structures:**
```c
// TX Processing Configuration
typedef struct {
    uint16 SecOCTxPduProcessingId;
    uint16 SecOCDataId;                    // Data ID for authenticator
    uint8  SecOCFreshnessValueLength;      // Full freshness length (bits)
    uint8  SecOCFreshnessValueTxLength;    // Truncated freshness length
    uint8  SecOCAuthInfoTxLength;          // Truncated MAC/sig length
    SecOC_PduType_Type SecOCTxPduType;     // IFPDU or TPPDU
    uint32 SecOCCsmJobId;                  // CSM job reference
    boolean SecOCUseMessageLink;           // PDU collection enable
} SecOC_TxPduProcessingType;

// RX Processing Configuration
typedef struct {
    uint16 SecOCRxPduProcessingId;
    uint16 SecOCDataId;
    uint8  SecOCFreshnessValueLength;
    uint8  SecOCAuthInfoRxLength;
    SecOC_PduType_Type SecOCRxPduType;
    uint32 SecOCCsmJobId;
    boolean SecOCUseMessageLink;
} SecOC_RxPduProcessingType;

// Runtime TX State
typedef struct {
    uint8  AuthenticPDU[SECOC_AUTHPDU_MAX_LENGTH];
    uint16 AuthenticPDULength;
    uint8  SecuredPDU[SECOC_SECPDU_MAX_LENGTH];
    uint16 SecuredPDULength;
    uint8  DataToAuthenticator[SECOC_DATA_TO_AUTH_MAX_LENGTH];
    uint16 DataToAuthenticatorLength;
    uint8  Authenticator[SECOC_AUTHENTICATOR_MAX_LENGTH];  // 27200B for PQC
    uint16 AuthenticatorLength;
    uint64 FreshnessValue;
    uint8  AuthenticationBuildAttempts;
    SecOC_StateType State;  // IDLE, AUTH_REQUESTED, AUTHENTICATED, etc.
} SecOC_TxIntermediateType;
```

**Key Functions:**
```c
// Initialization
void SecOC_Init(const SecOC_ConfigType* ConfigPtr);

// TX Path
Std_ReturnType SecOC_IfTransmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr);
void SecOC_MainFunctionTx(void);  // Periodic TX processing

// RX Path
void SecOC_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);
void SecOC_MainFunctionRx(void);  // Periodic RX processing

// Internal Functions
static void prepareFreshnessTx(uint16 TxPduId);
static void constructDataToAuthenticatorTx(uint16 TxPduId);
static void authenticate(uint16 TxPduId);           // Classic MAC mode
static void authenticate_PQC(uint16 TxPduId);       // PQC signature mode
static void seperatePduCollectionTx(uint16 TxPduId);
static void parseSecuredPdu(uint16 RxPduId);
static void verify(uint16 RxPduId);                 // Classic MAC mode
static void verify_PQC(uint16 RxPduId);             // PQC signature mode
```

**Configuration Files:**
- `SecOC_Cfg.h` - Pre-compile configuration (periods, buffer sizes, feature flags)
- `SecOC_Lcfg.h/c` - Link-time configuration (state buffers, counters)
- `SecOC_PBcfg.h/c` - Post-build configuration (per-PDU parameters)
- `SecOC_PQC_Cfg.h` - PQC mode configuration

---

### FVM (Freshness Value Manager)

**Purpose:** Manages freshness counters to prevent replay attacks

**Key Responsibilities:**
- Maintain per-PDU freshness counters
- Increment counters on transmission
- Reconstruct full freshness from truncated values on reception
- Validate received freshness (must be greater than last)
- Handle counter overflow and synchronization

**Key Data Structures:**
```c
// Freshness Array Type
typedef struct {
    uint64 FreshnessValue;      // Full 64-bit counter
    uint8  TruncatedLength;     // How many bits truncated
} SecOC_FreshnessArrayType;
```

**Key Functions:**
```c
// TX Freshness
Std_ReturnType FVM_GetTxFreshness(
    uint16 FreshnessID,
    uint8* FreshnessValue,
    uint32* FreshnessValueLength
);

Std_ReturnType FVM_GetTxFreshnessTruncData(
    uint16 FreshnessID,
    uint8* FreshnessValue,          // Full freshness
    uint32* FreshnessValueLength,
    uint8* TruncatedFreshnessValue, // Truncated portion for PDU
    uint32* TruncatedFreshnessValueLength
);

// RX Freshness
Std_ReturnType FVM_GetRxFreshness(
    uint16 FreshnessID,
    const uint8* TruncatedFreshnessValue,
    uint32 TruncatedFreshnessValueLength,
    uint16 AuthVerifyAttempts,
    uint8* FreshnessValue,          // Reconstructed full freshness
    uint32* FreshnessValueLength
);

// Counter Management
void FVM_IncreaseCounter(uint16 FreshnessID);
void FVM_UpdateCounter(uint16 FreshnessID, uint64 NewFreshnessValue);
```

**Freshness Reconstruction Algorithm:**
```c
// Receiver reconstructs full freshness from truncated value
// Example: Counter = 0x123456, Truncated = 0x56 (8 bits)
// Algorithm:
1. Get current counter value: 0x123456
2. Extract lower bits from counter: 0x56
3. Compare with received truncated value: 0x56
4. If match: full freshness = 0x123456
5. If mismatch (counter wrapped):
   - Try next window: 0x123556
   - Validate: must be > last accepted freshness
```

---

### Csm (Crypto Service Manager)

**Purpose:** Abstraction layer for cryptographic operations

**Key Responsibilities:**
- Provide hardware-independent crypto API
- Manage crypto jobs (MAC, signature, encryption)
- Route operations to appropriate crypto driver
- Support both synchronous and asynchronous processing

**Key Functions:**
```c
// MAC Operations (Classic Mode)
Std_ReturnType Csm_MacGenerate(
    uint32 jobId,
    Crypto_OperationModeType mode,
    const uint8* dataPtr,
    uint32 dataLength,
    uint8* macPtr,
    uint32* macLengthPtr
);

Std_ReturnType Csm_MacVerify(
    uint32 jobId,
    Crypto_OperationModeType mode,
    const uint8* dataPtr,
    uint32 dataLength,
    const uint8* macPtr,
    uint32 macLength,
    Crypto_VerifyResultType* verifyPtr
);

// Signature Operations (PQC Mode)
Std_ReturnType Csm_SignatureGenerate(
    uint32 jobId,
    Crypto_OperationModeType mode,
    const uint8* dataPtr,
    uint32 dataLength,
    uint8* signaturePtr,
    uint32* signatureLengthPtr
);

Std_ReturnType Csm_SignatureVerify(
    uint32 jobId,
    Crypto_OperationModeType mode,
    const uint8* dataPtr,
    uint32 dataLength,
    const uint8* signaturePtr,
    uint32 signatureLength,
    Crypto_VerifyResultType* verifyPtr
);
```

**Operation Modes:**
```c
typedef enum {
    CRYPTO_OPERATIONMODE_START,       // Begin multi-part operation
    CRYPTO_OPERATIONMODE_UPDATE,      // Process data chunk
    CRYPTO_OPERATIONMODE_FINISH,      // Complete operation
    CRYPTO_OPERATIONMODE_SINGLECALL   // One-shot operation
} Crypto_OperationModeType;

typedef enum {
    CRYPTO_E_VER_OK,       // Verification successful
    CRYPTO_E_VER_NOT_OK    // Verification failed
} Crypto_VerifyResultType;
```

---

### PQC Module (Post-Quantum Cryptography)

**Purpose:** Wrapper for liboqs ML-KEM and ML-DSA algorithms

**Key Responsibilities:**
- Initialize liboqs library
- Generate ML-KEM and ML-DSA keypairs
- Perform ML-KEM encapsulation/decapsulation
- Perform ML-DSA signing/verification
- Manage key storage and lifecycle

**Algorithm Specifications:**

| Algorithm | NIST Standard | Security Level | Key Sizes | Operation Sizes |
|-----------|---------------|----------------|-----------|-----------------|
| **ML-KEM-768** | FIPS 203 | Category 3 (~AES-192) | PK: 1184B, SK: 2400B | CT: 1088B, SS: 32B |
| **ML-DSA-65** | FIPS 204 | Category 3 (~AES-192) | PK: 1952B, SK: 4032B | Sig: 3309B |

**Key Data Structures:**
```c
// ML-KEM Types
typedef struct {
    uint8 PublicKey[MLKEM768_PUBLIC_KEY_BYTES];    // 1184 bytes
    uint8 SecretKey[MLKEM768_SECRET_KEY_BYTES];    // 2400 bytes
    boolean IsValid;
} PQC_MLKEM_KeyPairType;

typedef struct {
    uint8 Ciphertext[MLKEM768_CIPHERTEXT_BYTES];   // 1088 bytes
    uint8 SharedSecret[MLKEM768_SHARED_SECRET_BYTES]; // 32 bytes
} PQC_MLKEM_SharedSecretType;

// ML-DSA Types
typedef struct {
    uint8 PublicKey[MLDSA65_PUBLIC_KEY_BYTES];     // 1952 bytes
    uint8 SecretKey[MLDSA65_SECRET_KEY_BYTES];     // 4032 bytes
    boolean IsValid;
} PQC_MLDSA_KeyPairType;
```

**Key Functions:**
```c
// Initialization
Std_ReturnType PQC_Init(void);

// ML-KEM Operations
Std_ReturnType PQC_MLKEM_KeyGen(PQC_MLKEM_KeyPairType* KeyPair);
Std_ReturnType PQC_MLKEM_Encapsulate(
    const uint8* PublicKey,
    uint8* Ciphertext,
    uint8* SharedSecret
);
Std_ReturnType PQC_MLKEM_Decapsulate(
    const uint8* Ciphertext,
    const uint8* SecretKey,
    uint8* SharedSecret
);

// ML-DSA Operations
Std_ReturnType PQC_MLDSA_KeyGen(PQC_MLDSA_KeyPairType* KeyPair);
Std_ReturnType PQC_MLDSA_Sign(
    const uint8* Message,
    uint32 MessageLength,
    const uint8* SecretKey,
    uint8* Signature,
    uint32* SignatureLength
);
Std_ReturnType PQC_MLDSA_Verify(
    const uint8* Message,
    uint32 MessageLength,
    const uint8* Signature,
    uint32 SignatureLength,
    const uint8* PublicKey
);
```

**Performance Characteristics (Raspberry Pi 4):**
- ML-KEM KeyGen: ~2.85 ms
- ML-KEM Encapsulate: ~3.12 ms
- ML-KEM Decapsulate: ~3.89 ms
- ML-DSA KeyGen: ~5.23 ms
- ML-DSA Sign: ~8.13 ms (target), ~15.65 ms (Windows debug)
- ML-DSA Verify: ~4.89 ms

---

### PQC_KeyExchange Module

**Purpose:** Multi-peer ML-KEM key exchange manager

**Key Responsibilities:**
- Manage up to 8 concurrent key exchange sessions
- Implement initiator/responder pattern
- Store keypairs and shared secrets per peer
- Provide session state machine

**Session States:**
```c
typedef enum {
    PQC_KE_STATE_IDLE,               // No key exchange
    PQC_KE_STATE_INITIATED,          // Sent public key
    PQC_KE_STATE_RESPONDED,          // Received public key, sent CT
    PQC_KE_STATE_ESTABLISHED         // Shared secret available
} PQC_KeyExchangeState_Type;
```

**Key Data Structures:**
```c
typedef uint8 PQC_PeerIdType;  // 0-7 for 8 peers

typedef struct {
    PQC_PeerIdType PeerId;
    PQC_KeyExchangeState_Type State;
    PQC_MLKEM_KeyPairType KeyPair;
    uint8 SharedSecret[MLKEM768_SHARED_SECRET_BYTES];
    boolean IsValid;
} PQC_KeyExchangeSessionType;
```

**Key Functions:**
```c
// Initialization
void PQC_KeyExchange_Init(void);

// Initiator Flow (Alice)
Std_ReturnType PQC_KeyExchange_Initiate(
    PQC_PeerIdType PeerId,
    uint8* PublicKey            // Output: 1184 bytes to send to Bob
);

Std_ReturnType PQC_KeyExchange_Complete(
    PQC_PeerIdType PeerId,
    const uint8* Ciphertext     // Input: 1088 bytes from Bob
);

// Responder Flow (Bob)
Std_ReturnType PQC_KeyExchange_Respond(
    PQC_PeerIdType PeerId,
    const uint8* PeerPublicKey,  // Input: 1184 bytes from Alice
    uint8* Ciphertext            // Output: 1088 bytes to send to Alice
);

// Shared Secret Retrieval
Std_ReturnType PQC_KeyExchange_GetSharedSecret(
    PQC_PeerIdType PeerId,
    uint8* SharedSecret          // Output: 32 bytes
);
```

**Usage Pattern:**
```
Initiator (Alice)                          Responder (Bob)
     │                                           │
     ├─ PQC_KeyExchange_Initiate(0, &PK)        │
     │  Generates keypair                        │
     │  Returns PublicKey (1184B)                │
     │                                           │
     ├────── ethernet_send(PK) ──────────────────>
     │                                           │
     │                                 PQC_KeyExchange_Respond(0, PK, &CT)
     │                                 Encapsulates shared secret
     │                                 Returns Ciphertext (1088B)
     │                                           │
     <────── ethernet_receive(CT) ───────────────┤
     │                                           │
     ├─ PQC_KeyExchange_Complete(0, CT)         │
     │  Decapsulates shared secret               │
     │                                           │
     ├─ PQC_KeyExchange_GetSharedSecret(0, &SS) │
     │  Both sides now have same 32-byte SS      │
     ↓                                           ↓
```

---

### PQC_KeyDerivation Module

**Purpose:** HKDF-based key derivation from ML-KEM shared secrets

**Key Responsibilities:**
- Derive encryption and authentication keys from shared secret
- Implement HKDF-Extract and HKDF-Expand (RFC 5869)
- Store session keys per peer
- Ensure key independence

**Key Data Structures:**
```c
typedef struct {
    uint8 EncryptionKey[32];      // For AES-256-GCM
    uint8 AuthenticationKey[32];  // For HMAC-SHA256
    boolean IsValid;
} PQC_SessionKeysType;
```

**Key Functions:**
```c
// Initialization
void PQC_KeyDerivation_Init(void);

// Key Derivation
Std_ReturnType PQC_DeriveSessionKeys(
    const uint8* SharedSecret,        // Input: 32 bytes from ML-KEM
    PQC_PeerIdType PeerId,
    PQC_SessionKeysType* SessionKeys  // Output: 64 bytes (2×32)
);

// Key Retrieval
Std_ReturnType PQC_GetSessionKeys(
    PQC_PeerIdType PeerId,
    PQC_SessionKeysType* SessionKeys
);

// Key Cleanup
void PQC_ClearSessionKeys(PQC_PeerIdType PeerId);
```

**HKDF Algorithm:**
```
HKDF-Extract Phase:
  PRK = HMAC-SHA256(salt, SharedSecret)
  Where salt = "AUTOSAR-SecOC-PQC-v1.0"

HKDF-Expand Phase (Encryption Key):
  EncryptionKey = HMAC-SHA256(PRK, "Encryption-Key" || 0x01)

HKDF-Expand Phase (Authentication Key):
  AuthenticationKey = HMAC-SHA256(PRK, "Authentication-Key" || 0x01)
```

**Security Properties:**
- **Key Independence:** EncKey ≠ AuthKey ≠ SharedSecret
- **Forward Secrecy:** Different shared secrets → Different session keys
- **Deterministic:** Same shared secret → Same session keys (on both peers)

---

### PduR (PDU Router)

**Purpose:** Central routing hub for PDU delivery between layers

**Key Responsibilities:**
- Route I-PDUs based on static routing tables
- Translate PDU IDs between modules
- Support vertical routing (upper ↔ lower layers)
- Support horizontal routing (gateway between networks)
- Manage buffers for gatewayed PDUs

**Routing Table Structure:**
```c
typedef struct {
    PduIdType SourcePduId;           // PDU ID in source module
    PduIdType DestinationPduId;      // PDU ID in destination module
    PduR_DestModuleType DestModule;  // Destination module (CANIF, CANTP, SOAD, etc.)
} PduR_RoutingPathType;
```

**Key Functions (Per Module Pair):**
```c
// From COM
Std_ReturnType PduR_ComTransmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr);

// From SecOC
Std_ReturnType PduR_SecOCTransmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr);
void PduR_SecOCIfRxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);

// To/From CanIf
Std_ReturnType PduR_CanIfTransmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr);
void PduR_CanIfRxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);
void PduR_CanIfTxConfirmation(PduIdType TxPduId);

// To/From CanTP
Std_ReturnType PduR_CanTpTransmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr);
void PduR_CanTpRxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);
void PduR_CanTpTxConfirmation(PduIdType TxPduId);

// To/From SoAd
Std_ReturnType PduR_SoAdTransmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr);
void PduR_SoAdIfRxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);
void PduR_SoAdTpRxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);
```

**Routing Decision Logic:**
```c
// PduR_SecOCTransmit() routing logic
Std_ReturnType PduR_SecOCTransmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr) {
    // Lookup routing table
    PduR_RoutingPathType* Route = &PduR_RoutingTable[TxPduId];

    // Route based on destination module
    switch (Route->DestModule) {
        case PDUR_DEST_CANIF:
            return CanIf_Transmit(Route->DestinationPduId, PduInfoPtr);

        case PDUR_DEST_CANTP:
            return CanTp_Transmit(Route->DestinationPduId, PduInfoPtr);

        case PDUR_DEST_SOADIF:
            return SoAd_IfTransmit(Route->DestinationPduId, PduInfoPtr);

        case PDUR_DEST_SOADTP:
            return SoAd_TpTransmit(Route->DestinationPduId, PduInfoPtr);

        default:
            return E_NOT_OK;
    }
}
```

---

### SoAd (Socket Adapter)

**Purpose:** Ethernet communication interface

**Key Responsibilities:**
- Manage TCP/IP socket connections
- Map I-PDUs to socket connections (IP address + port)
- Support both IF (Interface) and TP (Transport Protocol) modes
- Handle large PDUs (up to 4096 bytes for PQC signatures)

**Key Functions:**
```c
// IF Mode (Direct transmission)
Std_ReturnType SoAd_IfTransmit(
    PduIdType TxPduId,
    const PduInfoType* PduInfoPtr
);

// TP Mode (Segmented transmission for large PDUs)
Std_ReturnType SoAd_TpTransmit(
    PduIdType TxPduId,
    const PduInfoType* PduInfoPtr
);

// Reception
void SoAd_RxIndication(
    PduIdType RxPduId,
    const PduInfoType* PduInfoPtr
);

// Main Functions
void SoAd_MainFunctionRx(void);
void SoAd_MainFunctionTx(void);
```

**Socket Configuration:**
```c
// Example: Ethernet gateway configuration
Server IP: 192.168.1.100
Server Port: 12345
Client IP: 192.168.1.200
Protocol: TCP
Buffer Size: 4096 bytes
```

---

### SoAd_PQC Module (Extension)

**Purpose:** Integrate ML-KEM key exchange into Socket Adapter

**Key Responsibilities:**
- Perform ML-KEM key exchange over Ethernet
- Manage per-peer key exchange state
- Derive session keys using HKDF
- Provide session keys to SecOC for optional encryption

**Key Functions:**
```c
// Key Exchange
Std_ReturnType SoAd_PQC_KeyExchange(
    PQC_PeerIdType PeerId,
    boolean IsInitiator
);

// Session Key Retrieval
Std_ReturnType SoAd_PQC_GetSessionKeys(
    PQC_PeerIdType PeerId,
    PQC_SessionKeysType* SessionKeys
);
```

**Integration with PQC Module:**
```c
// Internal implementation
Std_ReturnType SoAd_PQC_KeyExchange(PQC_PeerIdType PeerId, boolean IsInitiator) {
    if (IsInitiator) {
        // Initiator flow
        uint8 PublicKey[MLKEM768_PUBLIC_KEY_BYTES];
        PQC_KeyExchange_Initiate(PeerId, PublicKey);
        ethernet_send(PeerId, PublicKey, sizeof(PublicKey));

        // Wait for ciphertext response
        uint8 Ciphertext[MLKEM768_CIPHERTEXT_BYTES];
        ethernet_receive(PeerId, Ciphertext, sizeof(Ciphertext));

        // Complete key exchange
        PQC_KeyExchange_Complete(PeerId, Ciphertext);
    } else {
        // Responder flow
        uint8 PeerPublicKey[MLKEM768_PUBLIC_KEY_BYTES];
        ethernet_receive(PeerId, PeerPublicKey, sizeof(PeerPublicKey));

        uint8 Ciphertext[MLKEM768_CIPHERTEXT_BYTES];
        PQC_KeyExchange_Respond(PeerId, PeerPublicKey, Ciphertext);
        ethernet_send(PeerId, Ciphertext, sizeof(Ciphertext));
    }

    // Derive session keys
    uint8 SharedSecret[32];
    PQC_KeyExchange_GetSharedSecret(PeerId, SharedSecret);

    PQC_SessionKeysType SessionKeys;
    PQC_DeriveSessionKeys(SharedSecret, PeerId, &SessionKeys);

    return E_OK;
}
```

---

# 3. Layer-by-Layer Analysis

## 3.1 Application Layer

**Modules:** Com, GUIInterface

**Responsibilities:**
- Message-level communication (signal packing/unpacking)
- Application callbacks for TX confirmation and RX indication
- Test framework integration

**Key Interactions:**
```
Application Signal Write
    ↓
Com_SendSignal()
    - Pack signals into I-PDU
    - Apply signal transformations
    ↓
PduR_ComTransmit()
    ↓
[Routed to SecOC or directly to transport]
```

---

## 3.2 Service Layer

**Modules:** SecOC, FVM, Csm, PQC, PQC_KeyExchange, PQC_KeyDerivation

**Responsibilities:**
- Security (authentication, integrity)
- Freshness management (replay prevention)
- Cryptographic operations (MAC, signatures, key exchange)

**This is the core of the security architecture.**

**Key Interactions:**
```
SecOC_IfTransmit(TxPduId, AuthenticPDU)
    ↓
FVM_GetTxFreshness() → Get freshness counter
    ↓
Construct DataToAuthenticator = [DataID | AuthPDU | Freshness]
    ↓
Csm_SignatureGenerate() → PQC_MLDSA_Sign()
    ↓
Build SecuredPDU = [AuthPDU | TruncFreshness | Signature]
    ↓
PduR_SecOCTransmit()
```

---

## 3.3 Routing/Adaptation Layer

**Modules:** PduR

**Responsibilities:**
- Central routing hub
- PDU ID translation
- Transport selection based on size/type

**Routing Table Example:**
```c
const PduR_RoutingPathType PduR_RoutingTable[] = {
    // Source: COM PDU ID 0 → Destination: SecOC PDU ID 0
    { .SourcePduId = 0, .DestinationPduId = 0, .DestModule = PDUR_DEST_SECOC },

    // Source: SecOC PDU ID 0 → Destination: SoAd TP PDU ID 5
    { .SourcePduId = 0, .DestinationPduId = 5, .DestModule = PDUR_DEST_SOADTP },
};
```

---

## 3.4 ECU Abstraction Layer

**Modules:** CanIf, CanTp, SoAd, SoAd_PQC

**Responsibilities:**
- Hardware-independent communication interfaces
- Protocol-specific logic (IF vs TP mode)
- Segmentation/reassembly (TP mode)

**Key Decision: IF vs TP Mode**
```c
// PduR decides based on PDU size
if (PduLength <= 64) {
    // Small PDU (Classic MAC) → IF mode
    SoAd_IfTransmit(TxPduId, PduInfoPtr);
} else {
    // Large PDU (PQC signature) → TP mode
    SoAd_TpTransmit(TxPduId, PduInfoPtr);
}
```

---

## 3.5 MCAL (Microcontroller Abstraction Layer)

**Modules:** Ethernet Driver (ethernet.c / ethernet_windows.c), Scheduler

**Responsibilities:**
- Direct hardware access
- Platform-specific implementations
- Socket management (POSIX vs Winsock2)

**Platform Abstraction:**
```c
// Linux (POSIX Sockets)
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Windows (Winsock2)
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

// Common interface
int ethernet_init(void);
int ethernet_send(uint8 peerId, const uint8* data, uint16 length);
int ethernet_receive(uint8 peerId, uint8* data, uint16* length);
```

---

# 4. Data Flow Analysis

## 4.1 Complete TX Path (Application → Network)

**Scenario:** Application sends 8-byte message, secured with PQC signature

```
┌─────────────────────────────────────────────────────────────────┐
│ LAYER: APPLICATION                                              │
└─────────────────────────────────────────────────────────────────┘
Application writes signal value
    ↓
┌─────────────────────────────────────────────────────────────────┐
│ LAYER: SERVICE (COM)                                            │
└─────────────────────────────────────────────────────────────────┘
Com_SendSignal(SignalId, DataPtr)
    - Packs signal into I-PDU buffer
    - I-PDU = [8 bytes application data]
    ↓
PduR_ComTransmit(TxPduId=0, I-PDU)
    ↓
┌─────────────────────────────────────────────────────────────────┐
│ LAYER: SERVICE (SECOC)                                          │
└─────────────────────────────────────────────────────────────────┘
SecOC_IfTransmit(TxPduId=0, AuthenticPDU=[8 bytes])
    - Copies authentic PDU to internal buffer
    - Sets state = WAITING_FOR_AUTHENTICATION
    ↓
SecOC_MainFunctionTx() [Called periodically, e.g., every 0.9ms]
    ↓
    ├─ prepareFreshnessTx(TxPduId=0)
    │      FVM_GetTxFreshnessT runcData(FreshnessID=0, ...)
    │      Returns: FullFreshness=0x12345678 (64-bit), TruncFreshness=0x78 (8-bit)
    │
    ├─ constructDataToAuthenticatorTx(TxPduId=0)
    │      DataToAuth = [DataID (2B) | AuthPDU (8B) | FullFreshness (8B)]
    │      DataToAuth = 18 bytes total
    │
    ├─ authenticate_PQC(TxPduId=0)
    │      Csm_SignatureGenerate(JobId=10, DataToAuth, 18, &Signature, &SigLen)
    │          ↓
    │      PQC_MLDSA_Sign(DataToAuth, 18, SecretKey, Signature, &SigLen)
    │          ↓
    │      OQS_SIG_ml_dsa_65_sign(Signature, &SigLen, DataToAuth, 18, SecretKey)
    │          [liboqs performs ML-DSA-65 signature generation]
    │          Time: ~8.13ms (Raspberry Pi), ~15.65ms (Windows debug)
    │          Returns: Signature (3309 bytes)
    │
    └─ seperatePduCollectionTx(TxPduId=0)
           SecuredPDU = [AuthPDU (8B) | TruncFreshness (1B) | Signature (3309B)]
           SecuredPDU = 3318 bytes total
    ↓
PduR_SecOCTransmit(TxPduId=0, SecuredPDU=[3318 bytes])
    ↓
┌─────────────────────────────────────────────────────────────────┐
│ LAYER: ROUTING (PDUR)                                           │
└─────────────────────────────────────────────────────────────────┘
PduR routing logic:
    - Lookup routing table: TxPduId=0 → DestModule=SOADTP, DestPduId=5
    - PDU size = 3318 bytes → Force TP mode
    ↓
SoAd_TpTransmit(TxPduId=5, SecuredPDU=[3318 bytes])
    ↓
┌─────────────────────────────────────────────────────────────────┐
│ LAYER: ECU ABSTRACTION (SOAD)                                   │
└─────────────────────────────────────────────────────────────────┘
SoAd_TpTransmit()
    - Maps PDU to socket connection (IP: 192.168.1.200, Port: 12345)
    - Buffers PDU for transmission
    ↓
SoAd_MainFunctionTx() [Called periodically]
    ↓
ethernet_send(PeerId=0, SecuredPDU, 3318)
    ↓
┌─────────────────────────────────────────────────────────────────┐
│ LAYER: MCAL (ETHERNET DRIVER)                                   │
└─────────────────────────────────────────────────────────────────┘
ethernet_send()
    - Linux: send(socket_fd, SecuredPDU, 3318, 0)
    - Windows: send(socket, SecuredPDU, 3318, 0)
    - TCP/IP stack handles packetization
    ↓
┌─────────────────────────────────────────────────────────────────┐
│ PHYSICAL: ETHERNET NETWORK                                      │
└─────────────────────────────────────────────────────────────────┘
TCP/IP transmission over 100 Mbps Ethernet
Transmission time: ~0.26ms (3318 bytes @ 100 Mbps)
```

**Total TX Latency Breakdown:**
- FVM_GetTxFreshness: ~0.01 ms
- Construct DataToAuthenticator: ~0.02 ms
- PQC_MLDSA_Sign: ~8.13 ms (Raspberry Pi) or ~15.65 ms (Windows)
- Build Secured PDU: ~0.05 ms
- Ethernet transmission: ~0.26 ms
- **Total: ~8.47 ms (Raspberry Pi), ~16.13 ms (Windows)**

---

## 4.2 Complete RX Path (Network → Application)

**Scenario:** Backend sends PQC-signed 3318-byte PDU, gateway verifies and forwards to application

```
┌─────────────────────────────────────────────────────────────────┐
│ PHYSICAL: ETHERNET NETWORK                                      │
└─────────────────────────────────────────────────────────────────┘
TCP/IP packet arrival
    ↓
┌─────────────────────────────────────────────────────────────────┐
│ LAYER: MCAL (ETHERNET DRIVER)                                   │
└─────────────────────────────────────────────────────────────────┘
ethernet_receive()
    - Linux: recv(socket_fd, buffer, 4096, 0)
    - Windows: recv(socket, buffer, 4096, 0)
    - Returns SecuredPDU (3318 bytes)
    ↓
SoAd_RxIndication(RxPduId=5, SecuredPDU=[3318 bytes])
    ↓
┌─────────────────────────────────────────────────────────────────┐
│ LAYER: ECU ABSTRACTION (SOAD)                                   │
└─────────────────────────────────────────────────────────────────┘
SoAd processes received PDU
    - Maps socket connection → PDU ID
    ↓
PduR_SoAdTpRxIndication(RxPduId=5, SecuredPDU=[3318 bytes])
    ↓
┌─────────────────────────────────────────────────────────────────┐
│ LAYER: ROUTING (PDUR)                                           │
└─────────────────────────────────────────────────────────────────┘
PduR routing logic:
    - Lookup routing table: RxPduId=5 → DestModule=SECOC, DestPduId=0
    ↓
SecOC_RxIndication(RxPduId=0, SecuredPDU=[3318 bytes])
    ↓
┌─────────────────────────────────────────────────────────────────┐
│ LAYER: SERVICE (SECOC)                                          │
└─────────────────────────────────────────────────────────────────┘
SecOC_RxIndication()
    - Copies Secured PDU to internal buffer
    - Sets state = WAITING_FOR_VERIFICATION
    ↓
SecOC_MainFunctionRx() [Called periodically, e.g., every 0.9ms]
    ↓
    ├─ parseSecuredPdu(RxPduId=0)
    │      Parse: AuthPDU (8B) | TruncFreshness (1B) | Signature (3309B)
    │
    ├─ FVM_GetRxFreshness(FreshnessID=0, TruncFreshness=0x78, ...)
    │      - Reconstruct full freshness from truncated value
    │      - Current counter: 0x12345678
    │      - Truncated received: 0x78
    │      - Match → Full freshness = 0x12345678
    │      - Validate: 0x12345678 > 0x12345677 (last accepted) → PASS
    │      Returns: FullFreshness=0x12345678
    │
    ├─ constructDataToAuthenticatorRx(RxPduId=0)
    │      DataToAuth = [DataID (2B) | AuthPDU (8B) | FullFreshness (8B)]
    │      DataToAuth = 18 bytes total (same as TX)
    │
    ├─ verify_PQC(RxPduId=0)
    │      Csm_SignatureVerify(JobId=10, DataToAuth, 18, Signature, 3309, &VerifyResult)
    │          ↓
    │      PQC_MLDSA_Verify(DataToAuth, 18, Signature, 3309, PublicKey)
    │          ↓
    │      OQS_SIG_ml_dsa_65_verify(DataToAuth, 18, Signature, 3309, PublicKey)
    │          [liboqs performs ML-DSA-65 signature verification]
    │          Time: ~4.89ms
    │          Returns: E_OK (signature valid) or E_NOT_OK (invalid)
    │
    └─ If VerifyResult == E_OK:
           FVM_UpdateCounter(FreshnessID=0, 0x12345678)
           State = VERIFIED
       Else:
           State = VERIFICATION_FAILED
           Log security event (tampering detected)
           Drop PDU
    ↓
If verified:
PduR_SecOCIfRxIndication(RxPduId=0, AuthenticPDU=[8 bytes])
    ↓
┌─────────────────────────────────────────────────────────────────┐
│ LAYER: SERVICE (COM)                                            │
└─────────────────────────────────────────────────────────────────┘
Com_RxIndication(RxPduId=0, AuthenticPDU=[8 bytes])
    - Unpacks I-PDU into signals
    - Calls signal callbacks
    ↓
┌─────────────────────────────────────────────────────────────────┐
│ LAYER: APPLICATION                                              │
└─────────────────────────────────────────────────────────────────┘
Application receives signal value
Signal callback triggered
```

**Total RX Latency Breakdown:**
- Ethernet reception: ~0.01 ms
- Parse Secured PDU: ~0.05 ms
- FVM_GetRxFreshness: ~0.03 ms
- Construct DataToAuthenticator: ~0.02 ms
- PQC_MLDSA_Verify: ~4.89 ms
- Signal unpacking: ~0.02 ms
- **Total: ~5.02 ms**

**Combined TX+RX Latency: ~13.49 ms (Raspberry Pi), ~21.15 ms (Windows)**

---

## 4.3 Attack Scenario: Replay Attack Detection

**Scenario:** Attacker captures valid PDU and replays it later

```
NORMAL FLOW (Message 1):
    Gateway TX → Backend RX
    Freshness = 100 → Signature generated with FV=100
    Backend receives, verifies, updates counter to 100

NORMAL FLOW (Message 2):
    Gateway TX → Backend RX
    Freshness = 101 → Signature generated with FV=101
    Backend receives, verifies, updates counter to 101

ATTACK (Attacker replays Message 1):
    Attacker → Backend RX
    Freshness = 100 (from captured PDU)

    Backend RX Processing:
    ┌───────────────────────────────────────────────────────────┐
    │ SecOC_RxIndication(SecuredPDU with FV=100)                │
    ├───────────────────────────────────────────────────────────┤
    │ parseSecuredPdu()                                         │
    │   Extract: TruncFreshness = 0x64 (lower 8 bits of 100)   │
    ├───────────────────────────────────────────────────────────┤
    │ FVM_GetRxFreshness(TruncFreshness=0x64)                   │
    │   Current counter: 101                                    │
    │   Reconstruct: FullFreshness = 100                        │
    │   Validate: 100 > 101 ? NO!                               │
    │   **FRESHNESS VALIDATION FAILED**                         │
    ├───────────────────────────────────────────────────────────┤
    │ State = FRESHNESS_VERIFICATION_FAILED                     │
    │ Log security event: "Replay attack detected"              │
    │ **DROP PDU - DO NOT VERIFY SIGNATURE**                    │
    └───────────────────────────────────────────────────────────┘

RESULT: Replay attack blocked BEFORE expensive signature verification
```

**Key Point:** Freshness check happens BEFORE signature verification, saving ~4.89ms on replayed messages.

---

## 4.4 Attack Scenario: Message Tampering Detection

**Scenario:** Attacker intercepts message and modifies data

```
ORIGINAL MESSAGE:
    AuthenticPDU = [VehicleSpeed = 60 km/h]
    Freshness = 100
    Signature = Sign([DataID | AuthPDU | Freshness], SecretKey)

ATTACKER MODIFIES:
    AuthenticPDU = [VehicleSpeed = 120 km/h]  ← Modified
    Freshness = 100                            ← Unchanged
    Signature = <original signature>           ← Unchanged (attacker doesn't have secret key)

BACKEND RX PROCESSING:
    ┌───────────────────────────────────────────────────────────┐
    │ SecOC_RxIndication(Modified SecuredPDU)                   │
    ├───────────────────────────────────────────────────────────┤
    │ parseSecuredPdu()                                         │
    │   Extract: AuthPDU=[120 km/h], Freshness=100, Signature   │
    ├───────────────────────────────────────────────────────────┤
    │ FVM_GetRxFreshness()                                      │
    │   Validate: 100 > 99 → PASS (freshness OK)                │
    ├───────────────────────────────────────────────────────────┤
    │ constructDataToAuthenticatorRx()                          │
    │   DataToAuth = [DataID | AuthPDU=[120 km/h] | FV=100]    │
    │                  ← Modified data included                 │
    ├───────────────────────────────────────────────────────────┤
    │ verify_PQC()                                              │
    │   PQC_MLDSA_Verify(DataToAuth, Signature, PublicKey)      │
    │   Expected signature: Sign([DataID | 60 km/h | 100])      │
    │   Received signature: Sign([DataID | 60 km/h | 100])      │
    │   Computed for:       [DataID | 120 km/h | 100]           │
    │   **MISMATCH**                                            │
    │   Returns: E_NOT_OK                                       │
    ├───────────────────────────────────────────────────────────┤
    │ State = VERIFICATION_FAILED                               │
    │ Log security event: "Tampering detected"                  │
    │ **DROP PDU**                                              │
    └───────────────────────────────────────────────────────────┘

RESULT: Tampering detected, message dropped
```

**Key Point:** Any modification to AuthenticPDU, Freshness, or DataID causes signature verification to fail.

---

# 5. SecOC Core Architecture

## 5.1 SecOC State Machines

### TX State Machine

```
[IDLE]
  │
  ↓ SecOC_IfTransmit(AuthenticPDU)
[COPY_AUTHENTIC_PDU]
  │ - Copy PDU to internal buffer
  │ - Reset authentication counter
  ↓
[WAITING_FOR_AUTHENTICATION]
  │
  ↓ SecOC_MainFunctionTx()
[GET_TX_FRESHNESS]
  │ - Call FVM_GetTxFreshness()
  ↓
[CONSTRUCT_DATA_TO_AUTH]
  │ - Build: [DataID | AuthPDU | Freshness]
  ↓
[AUTHENTICATE]
  │ - Call Csm_MacGenerate() [Classic]
  │   OR Csm_SignatureGenerate() [PQC]
  ↓
[BUILD_SECURED_PDU]
  │ - Assemble: [AuthPDU | TruncFresh | MAC/Sig]
  ↓
[TRANSMIT]
  │ - Call PduR_SecOCTransmit()
  ↓
[TX_CONFIRMATION_PENDING]
  │
  ↓ PduR callback: SecOC_TxConfirmation()
[IDLE]
```

### RX State Machine

```
[IDLE]
  │
  ↓ SecOC_RxIndication(SecuredPDU)
[COPY_SECURED_PDU]
  │ - Copy PDU to internal buffer
  ↓
[WAITING_FOR_VERIFICATION]
  │
  ↓ SecOC_MainFunctionRx()
[PARSE_SECURED_PDU]
  │ - Extract: AuthPDU, TruncFreshness, MAC/Signature
  ↓
[GET_RX_FRESHNESS]
  │ - Call FVM_GetRxFreshness()
  │ - Reconstruct full freshness
  │ - Validate freshness > last
  ↓
  ├─ Freshness Invalid → [FRESHNESS_VERIFICATION_FAILED]
  │                        - Log replay attack
  │                        - Drop PDU
  │                        - Go to [IDLE]
  │
  └─ Freshness Valid
     ↓
[CONSTRUCT_DATA_TO_AUTH]
  │ - Rebuild: [DataID | AuthPDU | FullFreshness]
  ↓
[VERIFY]
  │ - Call Csm_MacVerify() [Classic]
  │   OR Csm_SignatureVerify() [PQC]
  ↓
  ├─ Verification Failed → [VERIFICATION_FAILED]
  │                         - Log tampering detected
  │                         - Drop PDU
  │                         - Go to [IDLE]
  │
  └─ Verification Passed
     ↓
[UPDATE_FRESHNESS]
  │ - Call FVM_UpdateCounter()
  ↓
[FORWARD_TO_APPLICATION]
  │ - Call PduR_SecOCIfRxIndication(AuthenticPDU)
  ↓
[IDLE]
```

---

## 5.2 Secured PDU Structure

### Classic Mode (MAC-based)

```
┌────────────────────────────────────────────────────────────────┐
│                     SECURED I-PDU                              │
├────────────────────────────────────────────────────────────────┤
│  Byte 0-N                                                      │
│  ┌──────────────────────────────────────────────────────────┐ │
│  │ AUTHENTIC I-PDU                                          │ │
│  │ (Application data, e.g., 8 bytes)                        │ │
│  └──────────────────────────────────────────────────────────┘ │
├────────────────────────────────────────────────────────────────┤
│  Byte N+1 ... N+M                                              │
│  ┌──────────────────────────────────────────────────────────┐ │
│  │ TRUNCATED FRESHNESS VALUE                                │ │
│  │ (e.g., 8 bits = 1 byte, or 24 bits = 3 bytes)            │ │
│  └──────────────────────────────────────────────────────────┘ │
├────────────────────────────────────────────────────────────────┤
│  Byte N+M+1 ... End                                            │
│  ┌──────────────────────────────────────────────────────────┐ │
│  │ TRUNCATED MAC (Message Authentication Code)             │ │
│  │ (e.g., 28 bits truncated from 128-bit CMAC-AES)         │ │
│  │ (Typical: 4-16 bytes)                                    │ │
│  └──────────────────────────────────────────────────────────┘ │
└────────────────────────────────────────────────────────────────┘

Example Classic Mode Secured PDU:
- Authentic PDU: 8 bytes
- Truncated Freshness: 3 bytes (24 bits)
- Truncated MAC: 4 bytes (32 bits)
- Total: 15 bytes
```

### PQC Mode (Signature-based)

```
┌────────────────────────────────────────────────────────────────┐
│                     SECURED I-PDU (PQC)                        │
├────────────────────────────────────────────────────────────────┤
│  Byte 0-N                                                      │
│  ┌──────────────────────────────────────────────────────────┐ │
│  │ AUTHENTIC I-PDU                                          │ │
│  │ (Application data, e.g., 8 bytes)                        │ │
│  └──────────────────────────────────────────────────────────┘ │
├────────────────────────────────────────────────────────────────┤
│  Byte N+1 ... N+M                                              │
│  ┌──────────────────────────────────────────────────────────┐ │
│  │ TRUNCATED FRESHNESS VALUE                                │ │
│  │ (e.g., 8 bits = 1 byte, or up to 64 bits = 8 bytes)      │ │
│  └──────────────────────────────────────────────────────────┘ │
├────────────────────────────────────────────────────────────────┤
│  Byte N+M+1 ... N+M+3309                                       │
│  ┌──────────────────────────────────────────────────────────┐ │
│  │ ML-DSA-65 SIGNATURE                                      │ │
│  │ (NIST FIPS 204)                                          │ │
│  │ (3,309 bytes - NOT truncated)                            │ │
│  │                                                          │ │
│  │ [... 3,309 bytes of signature data ...]                 │ │
│  │                                                          │ │
│  └──────────────────────────────────────────────────────────┘ │
└────────────────────────────────────────────────────────────────┘

Example PQC Mode Secured PDU:
- Authentic PDU: 8 bytes
- Truncated Freshness: 3 bytes (24 bits)
- ML-DSA-65 Signature: 3,309 bytes (NOT truncated)
- Total: 3,320 bytes (207x larger than classic mode!)
```

**Why PQC Signatures Are Not Truncated:**

Unlike classical MACs (where truncation from 128→28 bits is acceptable), PQC signatures CANNOT be truncated:
1. **Mathematical Structure:** ML-DSA signatures are lattice-based, not hash-based
2. **Security Dependency:** Every bit of the signature contributes to security proof
3. **NIST Requirement:** FIPS 204 specifies exact signature sizes (no truncation allowed)
4. **Verification Algorithm:** ML-DSA verification requires full signature

---

## 5.3 Data-to-Authenticator Construction

**Purpose:** Construct the exact byte sequence that is signed/MAC'd

**Structure:**
```c
DataToAuthenticator = SecOCDataId || AuthenticPDU || CompleteFreshnessValue
```

**Field Descriptions:**

| Field | Size | Description | Example |
|-------|------|-------------|---------|
| **SecOCDataId** | 2 bytes (uint16) | Unique identifier for this Secured I-PDU | 0x0010 |
| **AuthenticPDU** | Variable (4-20 bytes typical) | Application data to protect | [0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08] |
| **CompleteFreshnessValue** | Variable (3-8 bytes) | **Full** freshness counter (NOT truncated) | 0x0000000012345678 (64-bit) |

**CRITICAL:** The **complete** freshness value is ALWAYS used for MAC/signature generation, even if only a truncated portion is transmitted in the Secured PDU.

**Example Construction:**
```c
// Configuration
uint16 SecOCDataId = 0x0010;
uint8 AuthenticPDU[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
uint64 FullFreshnessValue = 0x0000000012345678;

// Construct DataToAuthenticator
uint8 DataToAuthenticator[18];  // 2 + 8 + 8 = 18 bytes

// Byte 0-1: DataId (big-endian)
DataToAuthenticator[0] = 0x00;
DataToAuthenticator[1] = 0x10;

// Byte 2-9: AuthenticPDU
memcpy(&DataToAuthenticator[2], AuthenticPDU, 8);

// Byte 10-17: FullFreshnessValue (big-endian)
DataToAuthenticator[10] = 0x00;
DataToAuthenticator[11] = 0x00;
DataToAuthenticator[12] = 0x00;
DataToAuthenticator[13] = 0x00;
DataToAuthenticator[14] = 0x12;
DataToAuthenticator[15] = 0x34;
DataToAuthenticator[16] = 0x56;
DataToAuthenticator[17] = 0x78;

// Result:
// [0x00, 0x10, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
//  0x00, 0x00, 0x00, 0x00, 0x12, 0x34, 0x56, 0x78]
```

**This 18-byte DataToAuthenticator is what gets signed/MAC'd.**

---

# 6. PQC Integration Architecture

## 6.1 PQC vs Classical Cryptography Comparison

| Aspect | Classical (CMAC-AES-128) | PQC (ML-DSA-65) |
|--------|--------------------------|-----------------|
| **Algorithm Family** | Symmetric key (AES) | Asymmetric key (Lattice-based) |
| **Security Basis** | Computational hardness | Module-LWE & Module-SIS problems |
| **Quantum Threat** | ❌ Vulnerable (Grover's algorithm) | ✅ Resistant (no known quantum attack) |
| **Key Size** | 16 bytes (128-bit key) | PK: 1952 bytes, SK: 4032 bytes |
| **Authenticator Size** | 4-16 bytes (truncated from 16) | 3,309 bytes (NOT truncatable) |
| **Generation Time** | ~0.002 ms | ~8.13 ms (Raspberry Pi), ~15.65 ms (Windows) |
| **Verification Time** | ~0.002 ms | ~4.89 ms |
| **Bandwidth Impact** | Low (~1-2%) | High (~206x larger) |
| **Suitable Networks** | CAN, FlexRay, Ethernet | Ethernet only (requires high bandwidth) |
| **PDU Mode** | IF or TP | TP required (large signature) |
| **NIST Standard** | FIPS 180-4 (SHA family) | FIPS 204 (ML-DSA) |
| **Security Level** | Classical 128-bit | Quantum NIST Category 3 (~AES-192 equivalent) |

---

## 6.2 ML-KEM-768 Key Exchange Flow

**Purpose:** Establish shared secret between gateway and backend

**Protocol:**
1. Initiator (Alice) generates keypair, sends public key to Responder (Bob)
2. Responder (Bob) encapsulates shared secret using Alice's public key, sends ciphertext
3. Initiator (Alice) decapsulates ciphertext to recover shared secret
4. Both parties derive session keys using HKDF

**Detailed Flow:**

```
┌─────────────────────────────────────────────────────────────────┐
│ PHASE 1: INITIALIZATION                                        │
└─────────────────────────────────────────────────────────────────┘

Alice (Gateway)                              Bob (Backend)
    │                                            │
    ├─ PQC_KeyExchange_Init()                   ├─ PQC_KeyExchange_Init()
    │  Initialize session storage                │  Initialize session storage
    │                                            │
┌─────────────────────────────────────────────────────────────────┐
│ PHASE 2: KEY GENERATION (INITIATOR)                            │
└─────────────────────────────────────────────────────────────────┘
    │                                            │
    ├─ PQC_KeyExchange_Initiate(PeerId=0, &PK)  │
    │  ├─ OQS_KEM_ml_kem_768_keypair()           │
    │  │  Time: ~2.85 ms                         │
    │  │  Generate:                              │
    │  │    PublicKey: 1,184 bytes               │
    │  │    SecretKey: 2,400 bytes               │
    │  ├─ Store SecretKey in session[0]          │
    │  └─ State[0] = INITIATED                   │
    │                                            │
┌─────────────────────────────────────────────────────────────────┐
│ PHASE 3: PUBLIC KEY TRANSMISSION                               │
└─────────────────────────────────────────────────────────────────┘
    │                                            │
    ├─ ethernet_send(PeerId=0, PK, 1184)        │
    │                                            │
    │  ─────── PublicKey (1,184 bytes) ──────────>
    │                                            │
    │                                 ethernet_receive(PK, 1184)
    │                                            │
┌─────────────────────────────────────────────────────────────────┐
│ PHASE 4: ENCAPSULATION (RESPONDER)                             │
└─────────────────────────────────────────────────────────────────┘
    │                                            │
    │                              PQC_KeyExchange_Respond(0, PK, &CT)
    │                                 ├─ OQS_KEM_ml_kem_768_encaps()
    │                                 │  Time: ~3.12 ms
    │                                 │  Input: Alice_PublicKey
    │                                 │  Generate:
    │                                 │    Ciphertext: 1,088 bytes
    │                                 │    SharedSecret: 32 bytes
    │                                 ├─ Store SharedSecret in session[0]
    │                                 └─ State[0] = RESPONDED
    │                                            │
┌─────────────────────────────────────────────────────────────────┐
│ PHASE 5: CIPHERTEXT TRANSMISSION                               │
└─────────────────────────────────────────────────────────────────┘
    │                                            │
    │                              ethernet_send(PeerId=0, CT, 1088)
    │                                            │
    │  <─────── Ciphertext (1,088 bytes) ────────
    │                                            │
ethernet_receive(CT, 1088)                      │
    │                                            │
┌─────────────────────────────────────────────────────────────────┐
│ PHASE 6: DECAPSULATION (INITIATOR)                             │
└─────────────────────────────────────────────────────────────────┘
    │                                            │
    ├─ PQC_KeyExchange_Complete(PeerId=0, CT)   │
    │  ├─ OQS_KEM_ml_kem_768_decaps()            │
    │  │  Time: ~3.89 ms                         │
    │  │  Input: Ciphertext + SecretKey          │
    │  │  Output: SharedSecret (32 bytes)        │
    │  ├─ Store SharedSecret in session[0]       │
    │  └─ State[0] = ESTABLISHED                 │
    │                                            │
┌─────────────────────────────────────────────────────────────────┐
│ PHASE 7: KEY DERIVATION (BOTH SIDES)                           │
└─────────────────────────────────────────────────────────────────┘
    │                                            │
    ├─ PQC_KeyExchange_GetSharedSecret(0, &SS)  ├─ PQC_KeyExchange_GetSharedSecret(0, &SS)
    │  Returns: 32 bytes                         │  Returns: 32 bytes
    │                                            │
    ├─ PQC_DeriveSessionKeys(SS, 0, &Keys)      ├─ PQC_DeriveSessionKeys(SS, 0, &Keys)
    │  ├─ HKDF-Extract                           │  ├─ HKDF-Extract
    │  │  PRK = HMAC-SHA256(salt, SS)            │  │  PRK = HMAC-SHA256(salt, SS)
    │  ├─ HKDF-Expand (Encryption)               │  ├─ HKDF-Expand (Encryption)
    │  │  EncKey = HMAC-SHA256(PRK, "Enc...01")  │  │  EncKey = HMAC-SHA256(PRK, "Enc...01")
    │  └─ HKDF-Expand (Authentication)           │  └─ HKDF-Expand (Authentication)
    │     AuthKey = HMAC-SHA256(PRK, "Auth..01") │     AuthKey = HMAC-SHA256(PRK, "Auth..01")
    │                                            │
    │  Time: ~0.3 ms                             │  Time: ~0.3 ms
    │                                            │
┌─────────────────────────────────────────────────────────────────┐
│ RESULT: BOTH PARTIES HAVE IDENTICAL SESSION KEYS               │
│   EncryptionKey: 32 bytes (for AES-256-GCM)                    │
│   AuthenticationKey: 32 bytes (for HMAC-SHA256)                │
└─────────────────────────────────────────────────────────────────┘
```

**Total Handshake Time:**
- KeyGen: 2.85 ms
- Encapsulate: 3.12 ms
- Decapsulate: 3.89 ms
- HKDF: 0.3 ms × 2 = 0.6 ms
- **Total: ~10.46 ms**

**Bandwidth:**
- Alice → Bob: 1,184 bytes (public key)
- Bob → Alice: 1,088 bytes (ciphertext)
- **Total: 2,272 bytes**

**This is a ONE-TIME cost per session.** Once established, session keys can be used for thousands of messages.

---

## 6.3 HKDF Key Derivation Details

**Purpose:** Derive two independent keys (encryption + authentication) from ML-KEM shared secret

**Standard:** RFC 5869 - HMAC-based Extract-and-Expand Key Derivation Function

**Algorithm:**

```
┌─────────────────────────────────────────────────────────────────┐
│ INPUT                                                           │
├─────────────────────────────────────────────────────────────────┤
│ SharedSecret: 32 bytes (from ML-KEM decapsulation)             │
│ Salt: "AUTOSAR-SecOC-PQC-v1.0" (application-specific)          │
│ Info_Enc: "Encryption-Key"                                     │
│ Info_Auth: "Authentication-Key"                                │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│ STEP 1: HKDF-EXTRACT                                           │
├─────────────────────────────────────────────────────────────────┤
│ Purpose: Extract pseudorandom key from shared secret           │
│                                                                 │
│ PRK = HMAC-SHA256(                                              │
│     key = Salt,                 // "AUTOSAR-SecOC-PQC-v1.0"    │
│     data = SharedSecret         // 32 bytes from ML-KEM        │
│ )                                                               │
│                                                                 │
│ Output: PRK (32 bytes)                                          │
│         Pseudorandom key with full entropy                      │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│ STEP 2: HKDF-EXPAND (ENCRYPTION KEY)                           │
├─────────────────────────────────────────────────────────────────┤
│ Purpose: Derive encryption key from PRK                         │
│                                                                 │
│ EncryptionKey = HMAC-SHA256(                                    │
│     key = PRK,                                                  │
│     data = Info_Enc || 0x01    // "Encryption-Key" || 0x01     │
│ )                                                               │
│                                                                 │
│ Output: EncryptionKey (32 bytes)                                │
│         For AES-256-GCM encryption                              │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│ STEP 3: HKDF-EXPAND (AUTHENTICATION KEY)                       │
├─────────────────────────────────────────────────────────────────┤
│ Purpose: Derive authentication key from PRK                     │
│                                                                 │
│ AuthenticationKey = HMAC-SHA256(                                │
│     key = PRK,                                                  │
│     data = Info_Auth || 0x01   // "Authentication-Key" || 0x01 │
│ )                                                               │
│                                                                 │
│ Output: AuthenticationKey (32 bytes)                            │
│         For HMAC-SHA256 authentication                          │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│ FINAL OUTPUT                                                    │
├─────────────────────────────────────────────────────────────────┤
│ PQC_SessionKeys[PeerId] = {                                     │
│     EncryptionKey[32],       // Derived key 1                   │
│     AuthenticationKey[32],   // Derived key 2                   │
│     IsValid = TRUE                                              │
│ }                                                               │
│                                                                 │
│ SECURITY PROPERTIES:                                            │
│ ✓ Key Independence: EncKey ≠ AuthKey ≠ SharedSecret            │
│ ✓ Forward Secrecy: Different SS → Different session keys       │
│ ✓ Deterministic: Same SS → Same keys (both peers match)        │
│ ✓ Domain Separation: Different Info strings → Different keys   │
└─────────────────────────────────────────────────────────────────┘
```

**Why HKDF?**

1. **Key Independence:** Using SharedSecret directly for both encryption and authentication violates cryptographic best practices
2. **Domain Separation:** Different "Info" strings ensure keys are cryptographically independent
3. **Entropy Extraction:** HKDF-Extract ensures uniform entropy distribution
4. **Standard Practice:** RFC 5869 is widely adopted (TLS 1.3, Signal Protocol, etc.)

---

# 7. Routing and Transport

## 7.1 PduR (PDU Router) Architecture

**Purpose:** Central routing hub that connects all AUTOSAR communication modules

**Key Concept:** PduR acts as a **switchboard**, routing PDUs between layers based on static routing tables.

### Routing Table Structure

```c
// Conceptual routing table (implementation in PduR_PBcfg.c)
typedef struct {
    PduIdType SourcePduId;           // PDU ID in source module
    PduIdType DestinationPduId;      // PDU ID in destination module
    PduR_DestModuleType DestModule;  // Destination module
} PduR_RoutingPathType;

// Example routing configuration
const PduR_RoutingPathType PduR_RoutingTable[] = {
    // COM → SecOC (application sends message to be secured)
    { .SourcePduId = 0, .DestinationPduId = 0, .DestModule = PDUR_DEST_SECOC },

    // SecOC → SoAd TP (secured message to Ethernet TP mode)
    { .SourcePduId = 0, .DestinationPduId = 5, .DestModule = PDUR_DEST_SOADTP },

    // SoAd → SecOC (received Ethernet message to SecOC for verification)
    { .SourcePduId = 5, .DestinationPduId = 0, .DestModule = PDUR_DEST_SECOC },

    // SecOC → COM (verified message to application)
    { .SourcePduId = 0, .DestinationPduId = 0, .DestModule = PDUR_DEST_COM },
};
```

### Routing Decision Logic

**Example: PduR_SecOCTransmit()**

```c
Std_ReturnType PduR_SecOCTransmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr) {
    // Step 1: Lookup routing path
    PduR_RoutingPathType* Route = &PduR_RoutingTable[TxPduId];

    // Step 2: Route based on destination module
    switch (Route->DestModule) {
        case PDUR_DEST_CANIF:
            // Direct CAN transmission (IF mode, small PDUs)
            return CanIf_Transmit(Route->DestinationPduId, PduInfoPtr);

        case PDUR_DEST_CANTP:
            // CAN Transport Protocol (segmented transmission)
            return CanTp_Transmit(Route->DestinationPduId, PduInfoPtr);

        case PDUR_DEST_SOADIF:
            // Ethernet IF mode (direct, small PDUs)
            return SoAd_IfTransmit(Route->DestinationPduId, PduInfoPtr);

        case PDUR_DEST_SOADTP:
            // Ethernet TP mode (required for PQC signatures)
            return SoAd_TpTransmit(Route->DestinationPduId, PduInfoPtr);

        default:
            return E_NOT_OK;
    }
}
```

**Key Functions:**

| Function | Direction | Purpose |
|----------|-----------|---------|
| `PduR_ComTransmit()` | Downward | Route from COM to lower layers |
| `PduR_SecOCTransmit()` | Downward | Route from SecOC to transport |
| `PduR_CanIfRxIndication()` | Upward | Route received CAN message upward |
| `PduR_SoAdIfRxIndication()` | Upward | Route received Ethernet message upward |
| `PduR_CanIfTxConfirmation()` | Upward | Route TX confirmation to SecOC |

---

## 7.2 Transport Protocol Modes

### IF Mode (Interface) vs TP Mode (Transport Protocol)

| Aspect | IF Mode | TP Mode |
|--------|---------|---------|
| **PDU Size** | ≤ 64 bytes (typical) | > 64 bytes (up to 4096 in our implementation) |
| **Transmission** | Single-call | Multi-frame segmentation |
| **Use Case** | Classic MAC (small authenticators) | PQC signatures (3309 bytes) |
| **Latency** | Low (~1 ms) | Higher (~5-10 ms for 3320-byte PDU) |
| **API Complexity** | Simple (one function call) | Complex (StartOfReception, CopyData, Confirmation) |
| **AUTOSAR Modules** | CanIf, SoAd_IfTransmit | CanTP, SoAd_TpTransmit |

---

### TP Mode Transmission Flow (TX Path)

**Scenario:** Transmitting 3320-byte Secured PDU via Ethernet

```
┌─────────────────────────────────────────────────────────────────┐
│ STEP 1: INITIATE TP TRANSMISSION                               │
└─────────────────────────────────────────────────────────────────┘
SecOC calls: PduR_SecOCTransmit(TxPduId=0, SecuredPDU[3320 bytes])
    ↓
PduR routes to: SoAd_TpTransmit(TxPduId=5, SecuredPDU[3320 bytes])
    ↓
SoAd_TpTransmit() stores PDU in internal buffer:
    SoAdTp_Buffer[5] = SecuredPDU (3320 bytes)
    Returns: E_OK

┌─────────────────────────────────────────────────────────────────┐
│ STEP 2: PERIODIC MAIN FUNCTION (CALLED EVERY ~10ms)            │
└─────────────────────────────────────────────────────────────────┘
SoAd_MainFunctionTx() called periodically
    ↓
For each PDU in SoAdTp_Buffer:
    If buffer has data to send:
        Calculate number of frames needed:
            - Frame size: BUS_LENGTH (100 bytes in implementation)
            - PDU size: 3320 bytes
            - Frames needed: ceil(3320 / 100) = 34 frames

        FOR each frame (frameIndex = 0 to 33):
            ┌─────────────────────────────────────────────────────┐
            │ STEP 2.1: COPY TX DATA                             │
            └─────────────────────────────────────────────────────┘
            PduR_SoAdTpCopyTxData(TxPduId=5, &info, &retry, &availableDataPtr)
                - Copies next 100 bytes (or remaining bytes for last frame)
                - Returns: BUFREQ_OK

            ┌─────────────────────────────────────────────────────┐
            │ STEP 2.2: TRANSMIT FRAME VIA IF MODE               │
            └─────────────────────────────────────────────────────┘
            SoAd_IfTransmit(TxPduId=5, frame[100 bytes])
                ↓
            ethernet_send(TxPduId=5, frame, 100)
                - Creates TCP socket
                - Connects to 192.168.1.200:12345
                - Sends 100 bytes + 2 bytes ID
                - Closes socket
                - Time: ~0.26 ms per frame

            IF transmission failed:
                retry.TpDataState = TP_DATARETRY
                frameIndex--  // Retry this frame
            ELSE:
                retry.TpDataState = TP_DATACONF
                Continue to next frame

┌─────────────────────────────────────────────────────────────────┐
│ STEP 3: TX CONFIRMATION                                        │
└─────────────────────────────────────────────────────────────────┘
After all 34 frames transmitted:
    PduR_SoAdTpTxConfirmation(TxPduId=5, E_OK)
        ↓
    SecOC_TxConfirmation(TxPduId=0, E_OK)
        - Updates TX state to IDLE
        - Ready for next transmission

Clear buffer:
    SoAdTp_Buffer[5].SduLength = 0
```

**Total Transmission Time:**
- Frame preparation: ~0.05 ms × 34 = 1.7 ms
- Ethernet transmission: ~0.26 ms × 34 = 8.84 ms
- **Total: ~10.54 ms**

---

### TP Mode Reception Flow (RX Path)

**Scenario:** Receiving 3320-byte Secured PDU via Ethernet

```
┌─────────────────────────────────────────────────────────────────┐
│ STEP 1: FIRST FRAME RECEPTION                                  │
└─────────────────────────────────────────────────────────────────┘
ethernet_receive() called in ethernet_RecieveMainFunction()
    - Receives frame from socket
    - Extracts PDU ID from last 2 bytes
    - Calls: SoAdTp_RxIndication(RxPduId=5, PduInfoPtr)

SoAdTp_RxIndication(RxPduId=5, frame[100 bytes])
    ↓
Check if first frame (SoAdTp_Recieve_Counter[5] == 0):
    YES → Calculate total PDU length:
        AuthHeaderLength = 1 byte (from configuration)
        TruncFreshnessLength = 3 bytes (24 bits)
        AuthInfoLength = 3309 bytes (ML-DSA-65 signature)

        If AuthHeaderLength > 0:
            Read expected PDU length from header (first byte)
            SoAdTp_secureLength_Recieve[5] = header value
        Else:
            Use configured authentic PDU length (8 bytes)
            SoAdTp_secureLength_Recieve[5] = 8 bytes

        Add security overhead:
            SoAdTp_secureLength_Recieve[5] += (1 + 3 + 3309) = 3313 bytes
            Total expected: 8 + 3313 = 3321 bytes (includes header)

    Increment counter:
        SoAdTp_Recieve_Counter[5] = 1

    Store frame in buffer:
        SoAdTp_Buffer_Rx[5] = frame

┌─────────────────────────────────────────────────────────────────┐
│ STEP 2: PERIODIC MAIN FUNCTION PROCESSES FIRST FRAME           │
└─────────────────────────────────────────────────────────────────┘
SoAd_MainFunctionRx() called periodically
    ↓
For RxPduId=5:
    IF (SoAdTp_Recieve_Counter[5] > 0) AND (buffer has data):
        Calculate total frames expected:
            lastFrameIndex = ceil(3321 / 100) = 34 frames

        IF this is first frame (counter == 1):
            ┌─────────────────────────────────────────────────────┐
            │ STEP 2.1: START OF RECEPTION                       │
            └─────────────────────────────────────────────────────┘
            result = PduR_SoAdStartOfReception(
                RxPduId=5,
                &SoAdTp_Buffer_Rx[5],
                TotalLength=3321,
                &bufferSizePtr
            )

            This calls: SecOC_StartOfReception()
                - Allocates internal buffer for secured PDU
                - Prepares for multi-frame reception
                - Returns: BUFREQ_OK

            ┌─────────────────────────────────────────────────────┐
            │ STEP 2.2: COPY FIRST FRAME DATA                    │
            └─────────────────────────────────────────────────────┘
            result = PduR_SoAdTpCopyRxData(
                RxPduId=5,
                &SoAdTp_Buffer_Rx[5],
                &bufferSizePtr
            )

            This calls: SecOC_CopyRxData()
                - Copies 100 bytes to SecOC internal buffer
                - Returns: BUFREQ_OK

            Clear frame buffer for next frame:
                SoAdTp_Buffer_Rx[5].SduLength = 0

┌─────────────────────────────────────────────────────────────────┐
│ STEP 3: SUBSEQUENT FRAMES (FRAMES 2-33)                        │
└─────────────────────────────────────────────────────────────────┘
For each subsequent frame:
    ethernet_receive() → SoAdTp_RxIndication() → increment counter
    SoAd_MainFunctionRx() checks:
        IF (counter != 1) AND (counter != lastFrameIndex):
            // Middle frame
            PduR_SoAdTpCopyRxData() → SecOC_CopyRxData()
                - Appends 100 bytes to SecOC buffer
                - Returns: BUFREQ_OK

┌─────────────────────────────────────────────────────────────────┐
│ STEP 4: LAST FRAME RECEPTION (FRAME 34)                        │
└─────────────────────────────────────────────────────────────────┘
ethernet_receive() → SoAdTp_RxIndication(frame 34, 21 bytes)
    - Counter = 34 = lastFrameIndex
    - Last frame size: 3321 % 100 = 21 bytes

SoAd_MainFunctionRx() detects last frame:
    IF (counter == lastFrameIndex):
        Adjust buffer length for last frame:
            SoAdTp_Buffer_Rx[5].SduLength = 21 bytes

        Copy last frame:
            PduR_SoAdTpCopyRxData() → SecOC_CopyRxData(21 bytes)

        ┌─────────────────────────────────────────────────────────┐
        │ STEP 4.1: TP RX INDICATION                             │
        └─────────────────────────────────────────────────────────┘
        PduR_SoAdTpRxIndication(RxPduId=5, BUFREQ_OK)
            ↓
        SecOC_TpRxIndication(RxPduId=0, BUFREQ_OK)
            - Marks PDU as ready for verification
            - Sets state = WAITING_FOR_VERIFICATION

        Reset counter for next reception:
            SoAdTp_Recieve_Counter[5] = 0

┌─────────────────────────────────────────────────────────────────┐
│ STEP 5: SECOC VERIFICATION                                     │
└─────────────────────────────────────────────────────────────────┘
SecOC_MainFunctionRx() processes received PDU:
    - Parses secured PDU
    - Verifies freshness
    - Verifies ML-DSA signature
    - Forwards authentic PDU to application (if verified)
```

**Total Reception Time:**
- Ethernet reception: ~0.01 ms × 34 = 0.34 ms
- Frame processing: ~0.05 ms × 34 = 1.7 ms
- Buffer copying: ~0.02 ms × 34 = 0.68 ms
- **Total: ~2.72 ms** (excluding verification time)

---

## 7.3 Ethernet Driver Implementation

### Platform-Specific Implementations

**Linux (POSIX Sockets) - ethernet.c:**

```c
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

Std_ReturnType ethernet_send(unsigned short id, unsigned char* data, uint16 dataLen) {
    // Create TCP socket
    int network_socket = socket(AF_INET, SOCK_STREAM, 0);

    // Configure server address
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT_NUMBER);  // 12345
    server_address.sin_addr.s_addr = inet_addr(ip_address_send);  // e.g., "192.168.1.200"

    // Connect to server
    int connection_status = connect(network_socket,
                                     (struct sockaddr*)&server_address,
                                     sizeof(server_address));

    // Prepare data: [PDU data | ID (2 bytes)]
    uint8 sendData[dataLen + sizeof(id)];
    memcpy(sendData, data, dataLen);
    memcpy(sendData + dataLen, &id, sizeof(id));  // Append ID at end

    // Send data
    send(network_socket, sendData, dataLen + sizeof(id), 0);

    // Close socket
    close(network_socket);
    return E_OK;
}

Std_ReturnType ethernet_receive(unsigned char* data, uint16 dataLen,
                                  unsigned short* id, uint16* actualSize) {
    // Create server socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);

    // Configure server address (bind to any interface)
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT_NUMBER);
    server_address.sin_addr.s_addr = INADDR_ANY;  // Listen on all interfaces

    // Enable address reuse
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    // Bind socket
    bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address));

    // Listen for connections
    listen(server_socket, 5);

    // Accept connection
    int client_socket = accept(server_socket, NULL, NULL);

    // Receive data
    unsigned char recData[dataLen + sizeof(unsigned short)];
    int recv_result = recv(client_socket, recData, dataLen + sizeof(unsigned short), 0);

    // Extract ID from end of data
    int actualPduSize = recv_result - sizeof(unsigned short);
    memcpy(id, recData + actualPduSize, sizeof(unsigned short));
    memcpy(data, recData, actualPduSize);

    if (actualSize != NULL) {
        *actualSize = (uint16)actualPduSize;
    }

    // Close sockets
    close(server_socket);
    return E_OK;
}
```

**Windows (Winsock2) - ethernet_windows.c:**

```c
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

// Initialize Winsock
void ethernet_init(void) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    // ... read IP address from file ...
}

Std_ReturnType ethernet_send(unsigned short id, unsigned char* data, uint16 dataLen) {
    // Create socket
    SOCKET network_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    // Configure server address (same as Linux)
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT_NUMBER);
    server_address.sin_addr.s_addr = inet_addr(ip_address_send);

    // Connect
    connect(network_socket, (struct sockaddr*)&server_address, sizeof(server_address));

    // Prepare and send data (same logic as Linux)
    uint8 sendData[dataLen + sizeof(id)];
    memcpy(sendData, data, dataLen);
    memcpy(sendData + dataLen, &id, sizeof(id));

    send(network_socket, sendData, dataLen + sizeof(id), 0);

    // Close socket (Windows specific)
    closesocket(network_socket);
    return E_OK;
}

// ethernet_receive() similar to Linux, using Winsock functions
```

**Key Differences:**

| Aspect | Linux (POSIX) | Windows (Winsock2) |
|--------|---------------|---------------------|
| **Header** | `<sys/socket.h>` | `<winsock2.h>` |
| **Initialization** | Not required | `WSAStartup()` required |
| **Close Socket** | `close(socket)` | `closesocket(socket)` |
| **Cleanup** | Not required | `WSACleanup()` |
| **Linking** | Automatic | Requires `ws2_32.lib` |

**CMake Build Selection:**

```cmake
if(UNIX)
    target_sources(SecOCLib PRIVATE source/Ethernet/ethernet.c)
elseif(WIN32)
    target_sources(SecOCLib PRIVATE source/Ethernet/ethernet_windows.c)
endif()
```

---

# 8. Configuration Architecture

## 8.1 Configuration Overview

AUTOSAR SecOC uses **three-level configuration**:

1. **Pre-Compile Configuration** (SecOC_Cfg.h) - Compile-time constants
2. **Link-Time Configuration** (SecOC_Lcfg.c/h) - State buffers, arrays
3. **Post-Build Configuration** (SecOC_PBcfg.c/h) - Per-PDU parameters

This allows flexibility without recompilation for certain changes.

---

## 8.2 Pre-Compile Configuration (SecOC_Cfg.h)

**Purpose:** Define system-wide constants, feature flags, and periods

**Key Parameters:**

```c
/************************************************************
 * MAIN FUNCTION PERIODS
 ***********************************************************/
#define SECOC_MAIN_FUNCTION_PERIOD_TX    ((float64)0.9)   // 0.9 ms TX period
#define SECOC_MAIN_FUNCTION_PERIOD_RX    ((float64)0.9)   // 0.9 ms RX period

/************************************************************
 * DEVELOPMENT ERROR DETECTION
 ***********************************************************/
#define SECOC_DEV_ERROR_DETECT           ((boolean)FALSE)  // Disable DET

/************************************************************
 * SECURITY EVENT REPORTING
 ***********************************************************/
#define SECOC_ENABLE_SECURITY_EVENT_REPORTING  ((boolean)FALSE)

/************************************************************
 * BUFFER CONFIGURATION
 ***********************************************************/
#define SECOC_BUFFERLENGTH                ((uint32)100)  // PDU collection buffer

/************************************************************
 * TX PDU PROCESSING CONFIGURATION
 ***********************************************************/
#define SECOC_AUTHENTICATION_BUILD_ATTEMPTS       ((uint16)5)
#define SECOC_TX_DATA_ID                          ((uint16)0)
#define SECOC_TX_FRESHNESS_VALUE_ID               ((uint16)10)
#define SECOC_TX_FRESHNESS_VALUE_LENGTH           ((uint8)24)   // 24 bits = 3 bytes
#define SECOC_TX_FRESHNESS_VALUE_TRUNC_LENGTH     ((uint8)8)    // Truncated to 8 bits
#define SECOC_TX_AUTH_INFO_TRUNC_LENGTH           ((uint16)32)  // Bits (4 bytes for classic)
#define SECOC_PROVIDE_TX_TRUNCATED_FRESHNESS_VALUE ((boolean)TRUE)
#define SECOC_USE_TX_CONFIRMATION                 (0)

/************************************************************
 * RX PDU PROCESSING CONFIGURATION
 ***********************************************************/
#define SECOC_AUTHENTICATION_VERIFYATTEMPTS       ((uint16)5)
#define SECOC_RX_DATA_ID                          ((uint16)0)
#define SECOC_FRESHNESSVALUE_ID                   ((uint16)20)
#define SECOC_RX_FRESHNESS_VALUE_LENGTH           ((uint8)24)
#define SECOC_RX_FRESHNESS_VALUE_TRUNCLENGTH      ((uint8)8)
#define SECOC_RX_AUTH_INFO_TRUNCLENGTH            ((uint16)32)

/************************************************************
 * CSM JOB CONFIGURATION
 ***********************************************************/
#define CSM_JOBID                                 ((uint32)10)

/************************************************************
 * PDU TYPE CONFIGURATION
 ***********************************************************/
#define SECOC_TX_PDUTYPE                          ((SecOC_PduType_Type)SECOC_IFPDU)
#define SECOC_RX_PDUTYPE                          SECOC_IFPDU

/************************************************************
 * MESSAGE LINK (PDU COLLECTION) CONFIGURATION
 ***********************************************************/
#define SECOC_MESSAGE_LINKLEN                     ((uint16)3)
#define SECOC_MESSAGE_LINKPOS                     ((uint16)5)
```

**Configuration Impact:**

| Parameter | Impact | Example |
|-----------|--------|---------|
| `SECOC_MAIN_FUNCTION_PERIOD_TX` | TX processing frequency | 0.9 ms → ~1111 Hz |
| `SECOC_TX_FRESHNESS_VALUE_LENGTH` | Full freshness size | 24 bits → 16,777,216 values |
| `SECOC_TX_FRESHNESS_VALUE_TRUNC_LENGTH` | Transmitted freshness size | 8 bits → 1 byte overhead |
| `SECOC_TX_AUTH_INFO_TRUNC_LENGTH` | MAC/signature truncation | 32 bits for classic, 3309 bytes for PQC |
| `CSM_JOBID` | Crypto job routing | Job 10 → ML-DSA signature |

---

## 8.3 Link-Time Configuration (SecOC_Lcfg.c/h)

**Purpose:** Define state buffers, counters, and arrays allocated at link time

**Key Data Structures:**

```c
// TX Intermediate State Buffers (One per TX PDU)
SecOC_TxIntermediateType SecOC_TxIntermediate[SECOC_NUM_OF_TX_PDU_PROCESSING];

// RX Intermediate State Buffers (One per RX PDU)
SecOC_RxIntermediateType SecOC_RxIntermediate[SECOC_NUM_OF_RX_PDU_PROCESSING];

// Freshness Counter Arrays
SecOC_FreshnessArrayType FVM_TxFreshnessArray[SECOC_NUM_OF_TX_PDU_PROCESSING];
SecOC_FreshnessArrayType FVM_RxFreshnessArray[SECOC_NUM_OF_RX_PDU_PROCESSING];

// PDU Collection Structure (for PDU routing metadata)
SecOC_PduCollection PdusCollections[SECOC_MAX_PDUS];
```

**Example Initialization:**

```c
// Initialize TX freshness counters
SecOC_FreshnessArrayType FVM_TxFreshnessArray[SECOC_NUM_OF_TX_PDU_PROCESSING] = {
    { .FreshnessValue = 0, .TruncatedLength = 8 },  // TX PDU 0
    { .FreshnessValue = 0, .TruncatedLength = 8 },  // TX PDU 1
};

// Initialize PDU collection metadata
SecOC_PduCollection PdusCollections[] = {
    { .Type = SECOC_SECURED_PDU_SOADTP, .Length = 3320 },  // PDU 0: PQC Ethernet TP
    { .Type = SECOC_SECURED_PDU_CANIF,  .Length = 15 },    // PDU 1: Classic CAN IF
};
```

---

## 8.4 Post-Build Configuration (SecOC_PBcfg.c/h)

**Purpose:** Per-PDU configuration that can be changed without recompilation

**TX PDU Processing Configuration:**

```c
typedef struct {
    uint16 SecOCTxPduProcessingId;           // Unique ID (0, 1, 2, ...)
    uint16 SecOCDataId;                      // Data ID for authenticator
    uint8  SecOCFreshnessValueLength;        // Full freshness length (bits)
    uint8  SecOCFreshnessValueTxLength;      // Truncated freshness length (bits)
    uint8  SecOCAuthInfoTxLength;            // Truncated MAC/signature length (bits)
    SecOC_PduType_Type SecOCTxPduType;       // SECOC_IFPDU or SECOC_TPPDU
    uint32 SecOCCsmJobId;                    // CSM job reference
    boolean SecOCUseMessageLink;             // PDU collection enable
    const SecOC_TxSecuredPduLayerType* SecOCTxSecuredPduLayer;
    const SecOC_TxAuthenticPduLayerType* SecOCTxAuthenticPduLayer;
} SecOC_TxPduProcessingType;

// Example configuration for PQC mode
const SecOC_TxPduProcessingType SecOCTxPduProcessing_PQC = {
    .SecOCTxPduProcessingId = 0,
    .SecOCDataId = 0x0010,
    .SecOCFreshnessValueLength = 64,         // 64-bit counter for PQC
    .SecOCFreshnessValueTxLength = 24,       // Truncate to 24 bits (3 bytes)
    .SecOCAuthInfoTxLength = 26472,          // 3309 bytes = 26472 bits (ML-DSA-65)
    .SecOCTxPduType = SECOC_TPPDU,           // Must use TP mode
    .SecOCCsmJobId = 10,                     // CSM job for ML-DSA
    .SecOCUseMessageLink = FALSE,            // No PDU collection for PQC
};
```

**RX PDU Processing Configuration:**

```c
typedef struct {
    uint16 SecOCRxPduProcessingId;
    uint16 SecOCDataId;
    uint8  SecOCFreshnessValueLength;        // Full freshness length (bits)
    uint8  SecOCAuthInfoRxLength;            // Expected MAC/signature length (bits)
    SecOC_PduType_Type SecOCRxPduType;
    uint32 SecOCCsmJobId;
    boolean SecOCUseMessageLink;
    const SecOC_RxSecuredPduLayerType* SecOCRxSecuredPduLayer;
    const SecOC_RxAuthenticPduLayerType* SecOCRxAuthenticPduLayer;
} SecOC_RxPduProcessingType;

// Example configuration for PQC mode
const SecOC_RxPduProcessingType SecOCRxPduProcessing_PQC = {
    .SecOCRxPduProcessingId = 0,
    .SecOCDataId = 0x0010,
    .SecOCFreshnessValueLength = 64,
    .SecOCAuthInfoRxLength = 26472,          // Expect full 3309-byte signature
    .SecOCRxPduType = SECOC_TPPDU,
    .SecOCCsmJobId = 10,
    .SecOCUseMessageLink = FALSE,
};
```

---

## 8.5 PQC-Specific Configuration (SecOC_PQC_Cfg.h)

**Purpose:** Configure PQC mode and algorithm selection

```c
/************************************************************
 * PQC MODE ENABLE/DISABLE
 ***********************************************************/
#define SECOC_USE_PQC_MODE                      TRUE

/************************************************************
 * PQC ALGORITHM SELECTION
 ***********************************************************/
// ML-KEM (Key Encapsulation Mechanism)
#define PQC_KEM_ALGORITHM                       PQC_KEM_MLKEM768

// ML-DSA (Digital Signature Algorithm)
#define PQC_SIG_ALGORITHM                       PQC_SIG_MLDSA65

/************************************************************
 * ML-KEM-768 PARAMETERS (NIST FIPS 203)
 ***********************************************************/
#define MLKEM768_PUBLIC_KEY_BYTES               1184
#define MLKEM768_SECRET_KEY_BYTES               2400
#define MLKEM768_CIPHERTEXT_BYTES               1088
#define MLKEM768_SHARED_SECRET_BYTES            32

/************************************************************
 * ML-DSA-65 PARAMETERS (NIST FIPS 204)
 ***********************************************************/
#define MLDSA65_PUBLIC_KEY_BYTES                1952
#define MLDSA65_SECRET_KEY_BYTES                4032
#define MLDSA65_SIGNATURE_BYTES                 3309

/************************************************************
 * PQC KEY EXCHANGE CONFIGURATION
 ***********************************************************/
#define PQC_MAX_PEERS                           8      // Support 8 concurrent key exchanges
#define PQC_KEY_EXCHANGE_TIMEOUT_MS             5000   // 5 second timeout

/************************************************************
 * HKDF CONFIGURATION
 ***********************************************************/
#define HKDF_SALT                               "AUTOSAR-SecOC-PQC-v1.0"
#define HKDF_INFO_ENCRYPTION                    "Encryption-Key"
#define HKDF_INFO_AUTHENTICATION                "Authentication-Key"
#define HKDF_OUTPUT_KEY_LENGTH                  32     // 256-bit keys

/************************************************************
 * BUFFER SIZES (ADJUSTED FOR PQC)
 ***********************************************************/
#define SECOC_AUTHPDU_MAX_LENGTH                100    // Authentic PDU max size
#define SECOC_SECPDU_MAX_LENGTH                 4096   // Secured PDU max (PQC signatures)
#define SECOC_DATA_TO_AUTH_MAX_LENGTH           200    // DataToAuthenticator max
#define SECOC_AUTHENTICATOR_MAX_LENGTH          27200  // 3309 bytes × 8 bits + margin
```

**Configuration Validation:**

```c
// Compile-time checks
#if (SECOC_USE_PQC_MODE == TRUE)
    #if (SECOC_SECPDU_MAX_LENGTH < 4096)
        #error "PQC mode requires SECOC_SECPDU_MAX_LENGTH >= 4096"
    #endif

    #if (SECOC_TX_PDUTYPE != SECOC_TPPDU)
        #error "PQC mode requires TP mode (SECOC_TPPDU)"
    #endif
#endif
```

---

# 9. Security Mechanisms

## 9.1 Authentication Mechanisms

### Classic Mode: MAC-based Authentication

**Algorithm:** CMAC-AES-128 (NIST SP 800-38B)

```
┌─────────────────────────────────────────────────────────────────┐
│ TX AUTHENTICATION (MAC GENERATION)                              │
└─────────────────────────────────────────────────────────────────┘
Input:
    - SharedKey: 128-bit AES key (pre-shared)
    - DataToAuthenticator: [DataID || AuthPDU || Freshness]

Process:
    MAC_full = CMAC-AES-128(SharedKey, DataToAuthenticator)
             = 128-bit MAC (16 bytes)

Truncation:
    MAC_truncated = MAC_full[0:3]  // First 4 bytes (32 bits)

Output:
    - Authenticator: 4 bytes

Secured PDU Structure:
    [AuthPDU (8B) | TruncFreshness (1B) | MAC_truncated (4B)] = 13 bytes

┌─────────────────────────────────────────────────────────────────┐
│ RX VERIFICATION (MAC VERIFICATION)                              │
└─────────────────────────────────────────────────────────────────┘
Input:
    - SharedKey: Same 128-bit AES key
    - DataToAuthenticator: [DataID || AuthPDU || Freshness] (reconstructed)
    - ReceivedMAC: 4 bytes (extracted from Secured PDU)

Process:
    MAC_computed = CMAC-AES-128(SharedKey, DataToAuthenticator)
                 = 128-bit MAC

    MAC_computed_truncated = MAC_computed[0:3]  // First 4 bytes

Verification:
    IF (MAC_computed_truncated == ReceivedMAC):
        Return: CRYPTO_E_VER_OK (verification passed)
    ELSE:
        Return: CRYPTO_E_VER_NOT_OK (tampering detected)
```

**Security Properties:**
- ✅ **Message Integrity:** Any modification to AuthPDU or Freshness changes MAC
- ✅ **Authentication:** Only holders of SharedKey can generate valid MAC
- ❌ **Quantum Resistance:** Vulnerable to Grover's algorithm (128-bit → 64-bit effective)

---

### PQC Mode: Signature-based Authentication

**Algorithm:** ML-DSA-65 (NIST FIPS 204)

```
┌─────────────────────────────────────────────────────────────────┐
│ TX AUTHENTICATION (ML-DSA SIGNATURE GENERATION)                 │
└─────────────────────────────────────────────────────────────────┘
Input:
    - SecretKey: 4032-byte ML-DSA-65 secret key
    - DataToAuthenticator: [DataID || AuthPDU || Freshness]

Process:
    Signature = ML-DSA-65-Sign(SecretKey, DataToAuthenticator)

Algorithm Steps (simplified):
    1. Hash message: h = SHA-512(DataToAuthenticator)
    2. Generate commitment: w = expandMask(seed)
    3. Compute challenge: c = H(w || h)
    4. Compute response: z = y + c·s (lattice operation)
    5. Encode signature: sig = Encode(c, z, hint)

Output:
    - Signature: 3,309 bytes (NOT truncatable)

Secured PDU Structure:
    [AuthPDU (8B) | TruncFreshness (3B) | Signature (3309B)] = 3,320 bytes

┌─────────────────────────────────────────────────────────────────┐
│ RX VERIFICATION (ML-DSA SIGNATURE VERIFICATION)                 │
└─────────────────────────────────────────────────────────────────┘
Input:
    - PublicKey: 1952-byte ML-DSA-65 public key
    - DataToAuthenticator: [DataID || AuthPDU || Freshness] (reconstructed)
    - ReceivedSignature: 3,309 bytes

Process:
    VerifyResult = ML-DSA-65-Verify(PublicKey, DataToAuthenticator, ReceivedSignature)

Algorithm Steps (simplified):
    1. Parse signature: (c, z, hint) = Decode(ReceivedSignature)
    2. Recompute commitment: w' = Az - tc (lattice operation)
    3. Apply hint: w'' = UseHint(hint, w')
    4. Hash message: h = SHA-512(DataToAuthenticator)
    5. Recompute challenge: c' = H(w'' || h)
    6. Compare: c' ?= c

Verification:
    IF (c' == c):
        Return: CRYPTO_E_VER_OK
    ELSE:
        Return: CRYPTO_E_VER_NOT_OK

Output:
    - VerifyResult: E_OK or E_NOT_OK
```

**Security Properties:**
- ✅ **Message Integrity:** Lattice-based, any modification invalidates signature
- ✅ **Authentication:** Only holder of SecretKey can generate valid signature
- ✅ **Quantum Resistance:** No known quantum algorithm breaks ML-DSA (NIST Category 3)
- ✅ **Non-repudiation:** Public key can be distributed, signatures verifiable by anyone

**Why ML-DSA Signatures Cannot Be Truncated:**

| Aspect | MAC (truncatable) | ML-DSA Signature (NOT truncatable) |
|--------|-------------------|-------------------------------------|
| **Structure** | Random-looking hash output | Structured lattice elements (c, z, hint) |
| **Security Basis** | Collision resistance | Module-LWE/SIS hardness |
| **Redundancy** | High (128-bit → 32-bit safe) | Low (all bits contribute to security proof) |
| **Verification** | Recompute and compare | Recompute commitment using all signature components |
| **NIST Standard** | Allows truncation | FIPS 204 specifies exact sizes |

---

## 9.2 Freshness Value Management (Replay Attack Prevention)

### Freshness Counter Architecture

**Purpose:** Prevent replay attacks by ensuring each message has a unique, monotonically increasing freshness value

```
┌─────────────────────────────────────────────────────────────────┐
│ FRESHNESS VALUE LIFECYCLE                                       │
└─────────────────────────────────────────────────────────────────┘

Sender (TX):
    1. Initialize: FreshnessCounter = 0
    2. For each message:
        - Read current counter: FreshnessCounter = N
        - Increment: FreshnessCounter = N + 1
        - Use N for authentication
        - Transmit truncated portion (e.g., lower 8 bits)

Receiver (RX):
    1. Initialize: LastAcceptedFreshness = 0
    2. For each received message:
        - Extract truncated freshness from PDU
        - Reconstruct full freshness using current counter
        - Validate: FullFreshness > LastAcceptedFreshness
        - If valid: Update LastAcceptedFreshness = FullFreshness
        - If invalid: DROP message (replay attack detected)
```

### Freshness Reconstruction Algorithm

**Challenge:** Only a truncated portion of freshness is transmitted (e.g., 8 bits out of 64)

**Solution:** Receiver reconstructs full freshness using local counter + windowing

```
┌─────────────────────────────────────────────────────────────────┐
│ FRESHNESS RECONSTRUCTION EXAMPLE                                │
└─────────────────────────────────────────────────────────────────┘

Configuration:
    - Full freshness: 64 bits
    - Truncated freshness: 8 bits (transmitted)
    - Reconstruction window: 256 values (2^8)

Scenario:
    Sender counter: 0x0000000000001234
    Truncated: 0x34 (lower 8 bits)

Receiver Reconstruction:
    Step 1: Get current receiver counter
        RxCounter = 0x0000000000001230 (slightly behind)

    Step 2: Extract lower bits from receiver counter
        RxCounterLowerBits = 0x30

    Step 3: Compare with received truncated freshness
        Received: 0x34
        Expected (from RxCounter): 0x30
        Mismatch!

    Step 4: Check if received is ahead (within window)
        Difference = 0x34 - 0x30 = 4
        Is difference < 128? YES → Likely valid

    Step 5: Reconstruct full freshness
        FullFreshness = (RxCounter & 0xFFFFFFFFFFFFFF00) | 0x34
                      = 0x0000000000001234

    Step 6: Validate
        FullFreshness (0x1234) > LastAcceptedFreshness (0x1230)? YES
        PASS → Update LastAcceptedFreshness = 0x1234

Scenario 2: Counter Wrap-Around
    Sender counter: 0x00000000000012FF (about to wrap)
    Truncated: 0xFF

    Next message:
        Sender counter: 0x0000000000001300
        Truncated: 0x00 (wrapped)

    Receiver Reconstruction:
        RxCounter = 0x00000000000012FE
        RxCounterLowerBits = 0xFE
        Received: 0x00

        Difference = 0x00 - 0xFE = -254 (modulo 256) = 2

        Reconstruct with upper bits incremented:
            FullFreshness = (RxCounter + 0x100) & 0xFFFFFFFFFFFFFF00 | 0x00
                          = 0x0000000000001300

        Validate: 0x1300 > 0x12FE? YES → PASS
```

**Implementation:**

```c
Std_ReturnType FVM_GetRxFreshness(
    uint16 FreshnessID,
    const uint8* TruncatedFreshnessValue,
    uint32 TruncatedFreshnessValueLength,
    uint16 AuthVerifyAttempts,
    uint8* FreshnessValue,
    uint32* FreshnessValueLength
) {
    // Get current receiver counter
    uint64 RxCounter = FVM_RxFreshnessArray[FreshnessID].FreshnessValue;

    // Extract truncated freshness from received PDU (e.g., 8 bits)
    uint8 ReceivedTruncated = TruncatedFreshnessValue[0];

    // Extract lower bits from receiver counter
    uint8 RxCounterLowerBits = (uint8)(RxCounter & 0xFF);

    // Calculate difference (handle wrap-around)
    int16 Difference = (int16)ReceivedTruncated - (int16)RxCounterLowerBits;

    // Reconstruct full freshness
    uint64 ReconstructedFreshness;
    if (Difference >= 0 && Difference < 128) {
        // Received is ahead, within normal window
        ReconstructedFreshness = (RxCounter & 0xFFFFFFFFFFFFFF00ULL) | ReceivedTruncated;
    } else if (Difference < 0 && Difference >= -128) {
        // Received appears behind, likely counter wrapped
        ReconstructedFreshness = ((RxCounter + 0x100) & 0xFFFFFFFFFFFFFF00ULL) | ReceivedTruncated;
    } else {
        // Outside window, likely replay attack
        return E_NOT_OK;
    }

    // Validate: Reconstructed freshness must be > last accepted
    uint64 LastAcceptedFreshness = FVM_RxFreshnessArray[FreshnessID].FreshnessValue;
    if (ReconstructedFreshness <= LastAcceptedFreshness) {
        // Replay attack detected!
        return E_NOT_OK;
    }

    // Return reconstructed freshness
    *(uint64*)FreshnessValue = ReconstructedFreshness;
    *FreshnessValueLength = 8;  // 64 bits

    return E_OK;
}
```

### Replay Attack Detection Scenarios

**Scenario 1: Exact Replay**
```
Message 1 (valid):
    Freshness = 100
    Receiver updates: LastAcceptedFreshness = 100

Message 2 (valid):
    Freshness = 101
    Receiver updates: LastAcceptedFreshness = 101

Replay Attack (Message 1 replayed):
    Freshness = 100
    Validation: 100 > 101? NO → REJECTED
    Log: "Replay attack detected (freshness 100 ≤ last accepted 101)"
```

**Scenario 2: Out-of-Order Delivery**
```
Message 1 sent: Freshness = 100
Message 2 sent: Freshness = 101

Network delay → Message 2 arrives first:
    Freshness = 101
    Validation: 101 > 0? YES → ACCEPTED
    LastAcceptedFreshness = 101

Message 1 arrives second (out-of-order):
    Freshness = 100
    Validation: 100 > 101? NO → REJECTED

Note: This is a limitation of strictly monotonic freshness.
Solution: Use timestamp-based freshness or accept small reordering windows.
```

---

## 9.3 Tampering Detection

**Mechanism:** Any modification to the Authentic PDU, Freshness, or Data ID will cause signature/MAC verification to fail.

### Tamper Detection Flow

```
┌─────────────────────────────────────────────────────────────────┐
│ ORIGINAL MESSAGE (SENDER)                                       │
└─────────────────────────────────────────────────────────────────┘
Authentic PDU: [VehicleSpeed = 60 km/h, EngineRPM = 3000]
Data ID: 0x0010
Freshness: 0x12345678

DataToAuthenticator:
    [0x00, 0x10] || [VehicleSpeed=60, EngineRPM=3000] || [0x12345678]
    = 18 bytes

Signature = ML-DSA-65-Sign(SecretKey, DataToAuthenticator)
          = 3309 bytes

Secured PDU transmitted:
    [AuthPDU (8B) | TruncFreshness (3B) | Signature (3309B)]

┌─────────────────────────────────────────────────────────────────┐
│ ATTACKER INTERCEPTS AND MODIFIES                                │
└─────────────────────────────────────────────────────────────────┘
Attacker modifies Authentic PDU:
    [VehicleSpeed = 120 km/h, EngineRPM = 3000]  ← Changed speed

Attacker cannot regenerate signature (no SecretKey!):
    Modified Secured PDU:
        [ModifiedAuthPDU (8B) | TruncFreshness (3B) | OriginalSignature (3309B)]

┌─────────────────────────────────────────────────────────────────┐
│ RECEIVER VERIFICATION                                           │
└─────────────────────────────────────────────────────────────────┘
Receiver parses Secured PDU:
    AuthPDU = [VehicleSpeed=120, EngineRPM=3000]  ← Modified data
    TruncFreshness = [0x78]
    Signature = OriginalSignature (3309 bytes)

Receiver reconstructs DataToAuthenticator:
    DataToAuth = [0x00, 0x10] || [VehicleSpeed=120, EngineRPM=3000] || [0x12345678]
                  ^^^^^^^^^^^     ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^       ^^^^^^^^^^^
                  Data ID         MODIFIED Authentic PDU                 Freshness

Receiver verifies signature:
    VerifyResult = ML-DSA-65-Verify(PublicKey, DataToAuth, Signature)

    Expected (for signature to be valid):
        DataToAuth = [0x00, 0x10] || [VehicleSpeed=60, ...] || [0x12345678]

    Actual (used for verification):
        DataToAuth = [0x00, 0x10] || [VehicleSpeed=120, ...] || [0x12345678]

    Mismatch!

    VerifyResult = CRYPTO_E_VER_NOT_OK

Receiver action:
    State = VERIFICATION_FAILED
    Log security event: "Tampering detected on RxPduId=0"
    DROP PDU (do not forward to application)
```

**Tamper Detection Effectiveness:**

| Modified Field | Detection | Reason |
|----------------|-----------|--------|
| Authentic PDU data | ✅ YES | Signature computed over AuthPDU |
| Freshness value | ✅ YES | Signature computed over full freshness |
| Data ID | ✅ YES | Signature computed over Data ID |
| Signature itself | ✅ YES | Invalid signature structure |
| PDU routing (attacker sends to wrong ECU) | ⚠️ Partial | Data ID mismatch if ECUs use different IDs |

---

## 9.4 Security Event Reporting

**Configuration:**
```c
#define SECOC_ENABLE_SECURITY_EVENT_REPORTING   ((boolean)TRUE)
```

**Security Events:**

| Event ID | Event Name | Trigger Condition | Action |
|----------|------------|-------------------|--------|
| `SECOC_E_VERIFICATION_FAILED` | Signature/MAC verification failed | Computed MAC/sig ≠ Received | Log, drop PDU, increment counter |
| `SECOC_E_FRESHNESS_FAILED` | Freshness validation failed | Freshness ≤ last accepted | Log, drop PDU (replay attack) |
| `SECOC_E_BUILD_FAILED` | Authentication build failed | Csm_SignatureGenerate() error | Log, retry up to N attempts |
| `SECOC_E_INVALID_PDU` | Malformed PDU structure | Parse error, length mismatch | Log, drop PDU |

**Example Security Event Log:**

```
[2025-11-16 03:15:42] SECOC_E_FRESHNESS_FAILED: RxPduId=0, ReceivedFreshness=100, LastAccepted=150
[2025-11-16 03:15:43] SECOC_E_VERIFICATION_FAILED: RxPduId=0, PQC signature verification failed
[2025-11-16 03:16:10] SECOC_E_BUILD_FAILED: TxPduId=1, ML-DSA sign operation timeout
```

---

# 10. Performance and Optimization

## 10.1 Benchmark Results

### PQC Operations (Raspberry Pi 4, ARM Cortex-A72)

| Operation | Classic (CMAC-AES) | PQC (ML-DSA-65) | Overhead |
|-----------|---------------------|-----------------|----------|
| **Signature Generation** | ~0.002 ms | ~8.13 ms | 4065x |
| **Signature Verification** | ~0.002 ms | ~4.89 ms | 2445x |
| **Key Generation** | ~0.001 ms | ~5.23 ms | 5230x |
| **Authenticator Size** | 4 bytes | 3,309 bytes | 827x |

**Note:** Windows debug build shows ~15.65 ms for signing (nearly 2x slower)

### ML-KEM Key Exchange (One-time cost per session)

| Phase | Raspberry Pi 4 | Windows (Debug) |
|-------|----------------|-----------------|
| **KeyGen** | 2.85 ms | ~4.2 ms |
| **Encapsulate** | 3.12 ms | ~4.5 ms |
| **Decapsulate** | 3.89 ms | ~5.8 ms |
| **HKDF (2 keys)** | 0.6 ms | ~0.8 ms |
| **Total Handshake** | ~10.46 ms | ~15.3 ms |

### End-to-End Message Latency

**Scenario:** 8-byte application message, PQC mode, Ethernet transport

```
┌─────────────────────────────────────────────────────────────────┐
│ TX PATH (Gateway → Backend)                                     │
└─────────────────────────────────────────────────────────────────┘
Operation                          Raspberry Pi 4    Windows (Debug)
───────────────────────────────────────────────────────────────────
FVM_GetTxFreshness()                 0.01 ms          0.01 ms
Construct DataToAuthenticator        0.02 ms          0.02 ms
PQC_MLDSA_Sign()                     8.13 ms          15.65 ms   ← Bottleneck
Build Secured PDU                    0.05 ms          0.05 ms
Ethernet transmission (TP mode)      10.54 ms         10.54 ms
───────────────────────────────────────────────────────────────────
TOTAL TX LATENCY                     18.75 ms         26.27 ms

┌─────────────────────────────────────────────────────────────────┐
│ RX PATH (Backend → Gateway)                                     │
└─────────────────────────────────────────────────────────────────┘
Operation                          Raspberry Pi 4    Windows (Debug)
───────────────────────────────────────────────────────────────────
Ethernet reception (TP mode)         2.72 ms          2.72 ms
Parse Secured PDU                    0.05 ms          0.05 ms
FVM_GetRxFreshness()                 0.03 ms          0.03 ms
Construct DataToAuthenticator        0.02 ms          0.02 ms
PQC_MLDSA_Verify()                   4.89 ms          4.89 ms
Forward to application               0.02 ms          0.02 ms
───────────────────────────────────────────────────────────────────
TOTAL RX LATENCY                     7.73 ms          7.73 ms

┌─────────────────────────────────────────────────────────────────┐
│ ROUND-TRIP LATENCY (TX + RX)                                    │
└─────────────────────────────────────────────────────────────────┘
Raspberry Pi 4:   18.75 ms + 7.73 ms = 26.48 ms
Windows (Debug):  26.27 ms + 7.73 ms = 34.00 ms
```

**Throughput:**
- Maximum messages/second (Raspberry Pi): 1000 / 26.48 ≈ **38 messages/sec**
- Maximum messages/second (Windows): 1000 / 34.00 ≈ **29 messages/sec**

---

## 10.2 Bandwidth Overhead Analysis

### Classic Mode (CMAC-AES)

```
Application PDU: 8 bytes
Truncated Freshness: 1 byte (8 bits)
MAC: 4 bytes (32 bits truncated from 128)
───────────────────────────
Secured PDU: 13 bytes

Overhead: 5 bytes / 8 bytes = 62.5%
```

### PQC Mode (ML-DSA-65)

```
Application PDU: 8 bytes
Truncated Freshness: 3 bytes (24 bits)
Signature: 3,309 bytes (NOT truncatable)
───────────────────────────
Secured PDU: 3,320 bytes

Overhead: 3,312 bytes / 8 bytes = 41,400%
Size ratio: 3,320 / 13 ≈ 255x larger than classic mode
```

**Impact on Network:**

| Network | Bandwidth | Classic Mode | PQC Mode | Feasible? |
|---------|-----------|--------------|----------|-----------|
| **CAN 2.0** | 500 kbps | ✅ 13 bytes → 0.21 ms | ❌ 3,320 bytes → 53 ms (exceeds frame limits) | NO |
| **CAN FD** | 5 Mbps | ✅ 13 bytes → 0.021 ms | ⚠️ 3,320 bytes → 5.3 ms (marginal) | Marginal |
| **FlexRay** | 10 Mbps | ✅ 13 bytes → 0.010 ms | ⚠️ 3,320 bytes → 2.7 ms | Yes (with care) |
| **Ethernet (100 Mbps)** | 100 Mbps | ✅ 13 bytes → 0.001 ms | ✅ 3,320 bytes → 0.27 ms | YES |
| **Ethernet (1 Gbps)** | 1 Gbps | ✅ 13 bytes → 0.0001 ms | ✅ 3,320 bytes → 0.027 ms | YES (ideal) |

**Conclusion:** PQC mode requires Ethernet (100 Mbps minimum) for practical use.

---

## 10.3 Optimization Strategies

### 1. Asynchronous Signing

**Problem:** `PQC_MLDSA_Sign()` blocks for ~8.13 ms

**Solution:** Offload to separate thread

```c
// Concept (implementation requires RTOS or pthreads)
void SecOC_MainFunctionTx(void) {
    for (uint16 TxPduId = 0; TxPduId < SECOC_NUM_OF_TX_PDU_PROCESSING; TxPduId++) {
        if (State == WAITING_FOR_AUTHENTICATION) {
            // Launch asynchronous signing task
            pthread_create(&sign_thread, NULL, async_sign_task, (void*)TxPduId);
            State = AUTHENTICATION_IN_PROGRESS;
        } else if (State == AUTHENTICATION_IN_PROGRESS) {
            // Check if signing completed
            if (signature_ready[TxPduId]) {
                State = AUTHENTICATED;
                // Continue with build and transmit
            }
        }
    }
}

void* async_sign_task(void* arg) {
    uint16 TxPduId = (uint16)arg;
    // Perform ML-DSA signing in background
    PQC_MLDSA_Sign(...);
    signature_ready[TxPduId] = TRUE;
    return NULL;
}
```

**Benefit:** Main function remains responsive, other PDUs can be processed in parallel

---

### 2. Signature Batching

**Problem:** Signing each message individually is slow

**Solution:** Accumulate multiple messages, sign once

```c
// Batch signing (concept)
typedef struct {
    uint8 Messages[MAX_BATCH_SIZE][SECOC_AUTHPDU_MAX_LENGTH];
    uint16 MessageLengths[MAX_BATCH_SIZE];
    uint8 Count;
} SignatureBatch;

void SecOC_MainFunctionTx_Batched(void) {
    static SignatureBatch Batch = {0};

    // Accumulate messages
    for (TxPduId in pending_messages) {
        if (Batch.Count < MAX_BATCH_SIZE) {
            memcpy(Batch.Messages[Batch.Count], DataToAuth, Length);
            Batch.Count++;
        }
    }

    // Sign entire batch when full or timeout
    if (Batch.Count == MAX_BATCH_SIZE || timeout_expired) {
        // Concatenate all messages
        uint8 ConcatenatedData[MAX_BATCH_SIZE * MAX_MESSAGE_SIZE];
        uint32 TotalLength = 0;
        for (i = 0; i < Batch.Count; i++) {
            memcpy(ConcatenatedData + TotalLength, Batch.Messages[i], Batch.MessageLengths[i]);
            TotalLength += Batch.MessageLengths[i];
        }

        // Sign once
        PQC_MLDSA_Sign(ConcatenatedData, TotalLength, ...);

        // Distribute signature to all messages (or use Merkle tree)
        ...
    }
}
```

**Benefit:** Amortize signing cost over multiple messages

**Trade-off:** Increased latency for early messages in batch

---

### 3. Hardware Acceleration

**Problem:** Software-only ML-DSA is slow on ARM Cortex-A72

**Solution:** Use hardware crypto accelerators (if available)

**Candidates:**
- ARM TrustZone CryptoCell
- Dedicated PQC coprocessor (future hardware)
- FPGA acceleration (high-end automotive ECUs)

**Expected Improvement:**
- **10-100x speedup** for signing/verification
- **Target:** < 1 ms signing time

---

### 4. Hybrid Mode (PQC + Classical)

**Concept:** Use both PQC and classical crypto for backward compatibility and defense-in-depth

```c
// Hybrid authenticator
typedef struct {
    uint8 ClassicMAC[4];        // CMAC-AES-128 (truncated)
    uint8 PQC_Signature[3309];  // ML-DSA-65
} HybridAuthenticator;

// Verification accepts either
Std_ReturnType SecOC_Verify_Hybrid(uint16 RxPduId) {
    // Try classical verification first (fast)
    if (Csm_MacVerify(...) == E_OK) {
        return E_OK;  // Classical crypto still secure
    }

    // Fall back to PQC verification (slow but quantum-safe)
    if (PQC_MLDSA_Verify(...) == E_OK) {
        return E_OK;  // PQC verification passed
    }

    return E_NOT_OK;  // Both failed
}
```

**Benefit:**
- Quantum-safe + classical security
- Graceful degradation if PQC accelerator unavailable

**Trade-off:**
- Even larger PDUs (3309 + 4 = 3313 bytes overhead)

---

### 5. Selective Signing

**Concept:** Only sign critical messages with PQC, use classical MAC for non-critical

```c
// Configuration: Per-PDU security level
typedef enum {
    SECOC_SECURITY_LEVEL_CLASSIC,  // CMAC-AES (fast, not quantum-safe)
    SECOC_SECURITY_LEVEL_PQC,      // ML-DSA (slow, quantum-safe)
    SECOC_SECURITY_LEVEL_HYBRID    // Both (defense-in-depth)
} SecOC_SecurityLevel_Type;

const SecOC_TxPduProcessingType SecOCTxPduProcessing[] = {
    {
        .SecOCTxPduProcessingId = 0,
        .SecOCDataId = 0x0010,
        .SecOCSecurityLevel = SECOC_SECURITY_LEVEL_PQC,  // Critical: Brake commands
    },
    {
        .SecOCTxPduProcessingId = 1,
        .SecOCDataId = 0x0020,
        .SecOCSecurityLevel = SECOC_SECURITY_LEVEL_CLASSIC,  // Non-critical: Dashboard telemetry
    },
};
```

**Benefit:**
- Balance security and performance
- Reserve PQC for high-value targets

---

## 10.4 Profiling Results

**Test Environment:**
- Platform: Raspberry Pi 4 (ARM Cortex-A72 @ 1.5 GHz)
- OS: Raspberry Pi OS (Linux)
- Compiler: GCC 10.2.1 (-O2 optimization)
- Test: 1000 sign-verify cycles

**Results:**

| Metric | Mean | StdDev | Min | Max |
|--------|------|--------|-----|-----|
| **Sign Time** | 8.13 ms | 0.42 ms | 7.65 ms | 9.21 ms |
| **Verify Time** | 4.89 ms | 0.31 ms | 4.52 ms | 5.67 ms |
| **TX Latency** | 18.75 ms | 0.85 ms | 17.23 ms | 21.34 ms |
| **RX Latency** | 7.73 ms | 0.51 ms | 6.98 ms | 8.92 ms |
| **Round-Trip** | 26.48 ms | 1.12 ms | 24.56 ms | 29.87 ms |

**Bottleneck Analysis:**
- **43% of time** in ML-DSA signing
- **18% of time** in Ethernet TP transmission (34 frames)
- **18% of time** in ML-DSA verification
- **21% of time** in other operations (freshness, parsing, etc.)

**Optimization Priority:**
1. Hardware acceleration for ML-DSA (43% savings potential)
2. Ethernet frame aggregation (reduce TP overhead)
3. Asynchronous signing (improve throughput, not latency)

---

# 11. Platform Abstraction

## 11.1 Windows vs Linux Differences

### Build System Selection

**CMakeLists.txt:**

```cmake
# Platform detection
if(WIN32)
    message(STATUS "Building for Windows")
    add_compile_definitions(WINDOWS)
    set(PLATFORM_SOURCES
        source/Ethernet/ethernet_windows.c
    )
    # Exclude scheduler on Windows (no pthreads by default)
    set(SCHEDULER_ON OFF)
elseif(UNIX)
    message(STATUS "Building for Linux (Raspberry Pi)")
    add_compile_definitions(LINUX)
    set(PLATFORM_SOURCES
        source/Ethernet/ethernet.c
        source/Scheduler/scheduler.c
    )
    set(SCHEDULER_ON ON)
    add_compile_definitions(SCHEDULER_ON)
endif()

# Add platform-specific sources
target_sources(SecOCLib PRIVATE ${PLATFORM_SOURCES})
```

---

### Ethernet Driver Abstraction

**Common Interface (ethernet.h):**

```c
// Platform-agnostic interface
void ethernet_init(void);
Std_ReturnType ethernet_send(unsigned short id, unsigned char* data, uint16 dataLen);
Std_ReturnType ethernet_receive(unsigned char* data, uint16 dataLen,
                                 unsigned short* id, uint16* actualSize);
void ethernet_RecieveMainFunction(void);
```

**Linux Implementation (ethernet.c):**

```c
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

Std_ReturnType ethernet_send(...) {
    int network_socket = socket(AF_INET, SOCK_STREAM, 0);  // POSIX socket
    // ...
    close(network_socket);  // POSIX close
}
```

**Windows Implementation (ethernet_windows.c):**

```c
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

void ethernet_init(void) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);  // Windows-specific init
}

Std_ReturnType ethernet_send(...) {
    SOCKET network_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);  // Winsock
    // ...
    closesocket(network_socket);  // Winsock close
}
```

**Abstraction Benefits:**
- ✅ SecOC layer code is 100% platform-agnostic
- ✅ Only MCAL layer (ethernet driver) has platform-specific code
- ✅ Easy to port to new platforms (implement ethernet.h interface)

---

### Scheduler Abstraction

**Linux (pthreads) - scheduler.c:**

```c
#include <pthread.h>
#include <ucontext.h>

pthread_mutex_t lock;

void scheduler_init(void) {
    pthread_mutex_init(&lock, NULL);
}

void task_create(void (*task_func)(void*), void* arg) {
    pthread_t thread;
    pthread_create(&thread, NULL, task_func, arg);
}

// Used in ethernet.c for thread-safe PDU collection access
void ethernet_RecieveMainFunction(void) {
    // ...
    pthread_mutex_lock(&lock);
    // Access shared PdusCollections[]
    pthread_mutex_unlock(&lock);
}
```

**Windows (Not implemented in current version):**

```c
// Alternative: Use Windows CreateThread() API
#include <windows.h>

HANDLE hMutex;

void scheduler_init(void) {
    hMutex = CreateMutex(NULL, FALSE, NULL);
}

void task_create(void (*task_func)(void*), void* arg) {
    HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)task_func, arg, 0, NULL);
}
```

**Current Status:**
- Linux: Full scheduler support with pthreads
- Windows: Scheduler excluded from build (single-threaded)

---

## 11.2 Compiler Abstraction

### Data Type Portability

**Std_Types.h (AUTOSAR standard types):**

```c
#include <stdint.h>
#include <stdbool.h>

// AUTOSAR standard types (portable across platforms)
typedef uint8_t     uint8;
typedef uint16_t    uint16;
typedef uint32_t    uint32;
typedef uint64_t    uint64;
typedef int8_t      sint8;
typedef int16_t     sint16;
typedef int32_t     sint32;
typedef int64_t     sint64;
typedef float       float32;
typedef double      float64;
typedef bool        boolean;

// AUTOSAR return types
typedef uint8 Std_ReturnType;
#define E_OK        ((Std_ReturnType)0)
#define E_NOT_OK    ((Std_ReturnType)1)

// NULL pointer definition
#ifndef NULL
  #ifdef __cplusplus
    #define NULL 0
  #else
    #define NULL ((void*)0)
  #endif
#endif
```

**Benefits:**
- ✅ Code compiles with GCC, Clang, MSVC without modification
- ✅ Explicit type sizes prevent portability bugs

---

### Endianness Handling

**Problem:** Network byte order (big-endian) vs host byte order (platform-dependent)

**Solution:** Use explicit byte-by-byte copying for Data ID and Freshness

```c
// DataToAuthenticator construction (endian-safe)
void constructDataToAuthenticatorTx(uint16 TxPduId) {
    uint16 DataID = SecOCTxPduProcessing[TxPduId].SecOCDataId;  // e.g., 0x0010
    uint64 Freshness = TxIntermediate[TxPduId].FreshnessValue;  // e.g., 0x12345678

    uint8* DataToAuth = TxIntermediate[TxPduId].DataToAuthenticator;
    uint32 Offset = 0;

    // Data ID (big-endian, 2 bytes)
    DataToAuth[Offset++] = (uint8)(DataID >> 8);    // MSB
    DataToAuth[Offset++] = (uint8)(DataID & 0xFF);  // LSB

    // Authentic PDU (already byte array, no conversion needed)
    memcpy(DataToAuth + Offset, TxIntermediate[TxPduId].AuthenticPDU, PduLength);
    Offset += PduLength;

    // Freshness (big-endian, 8 bytes)
    for (int i = 7; i >= 0; i--) {
        DataToAuth[Offset++] = (uint8)((Freshness >> (i * 8)) & 0xFF);
    }
}
```

**Result:** DataToAuthenticator is identical on all platforms (big-endian, little-endian)

---

## 11.3 Third-Party Library Abstraction (liboqs)

**liboqs:** Open Quantum Safe library (cross-platform PQC implementation)

**Build Configuration:**

```bash
# Linux (Raspberry Pi)
cd Autosar_SecOC/external/liboqs
mkdir -p build && cd build
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release ..
make -j4

# Windows (MinGW)
cd Autosar_SecOC/external/liboqs
mkdir -p build && cd build
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..
mingw32-make -j4
```

**Linking:**

```cmake
# CMakeLists.txt
find_library(LIBOQS_LIBRARY
    NAMES oqs liboqs
    PATHS ${CMAKE_SOURCE_DIR}/external/liboqs/build/lib
)

target_link_libraries(SecOCLib PRIVATE ${LIBOQS_LIBRARY})
```

**Wrapper Layer (PQC.c):**

```c
#include <oqs/oqs.h>

// Wrapper ensures consistent API across liboqs versions
Std_ReturnType PQC_MLDSA_Sign(
    const uint8* Message,
    uint32 MessageLength,
    const uint8* SecretKey,
    uint8* Signature,
    uint32* SignatureLength
) {
    OQS_SIG* sig = OQS_SIG_new(OQS_SIG_alg_ml_dsa_65);
    if (sig == NULL) {
        return E_NOT_OK;
    }

    OQS_STATUS status = OQS_SIG_sign(
        sig,
        Signature,
        (size_t*)SignatureLength,
        Message,
        MessageLength,
        SecretKey
    );

    OQS_SIG_free(sig);

    return (status == OQS_SUCCESS) ? E_OK : E_NOT_OK;
}
```

**Benefits:**
- ✅ Isolates SecOC from liboqs API changes
- ✅ Easy to swap PQC library (e.g., replace liboqs with hardware accelerator)

---

# 12. API Reference

## 12.1 SecOC Public API

### Initialization

```c
void SecOC_Init(const SecOC_ConfigType* ConfigPtr);
```
- **Description:** Initialize SecOC module with post-build configuration
- **Parameters:**
  - `ConfigPtr`: Pointer to configuration structure (NULL for default)
- **Returns:** None
- **Pre-condition:** Called once during ECU initialization
- **Example:**
  ```c
  SecOC_Init(NULL);  // Use default configuration
  ```

---

### Transmission (TX) API

```c
Std_ReturnType SecOC_IfTransmit(
    PduIdType TxPduId,
    const PduInfoType* PduInfoPtr
);
```
- **Description:** Initiate transmission of Authentic I-PDU (Interface mode)
- **Parameters:**
  - `TxPduId`: TX PDU identifier (0 to `SECOC_NUM_OF_TX_PDU_PROCESSING - 1`)
  - `PduInfoPtr`: Pointer to PDU information
    - `SduDataPtr`: Authentic PDU data
    - `SduLength`: Authentic PDU length
- **Returns:**
  - `E_OK`: Transmission request accepted
  - `E_NOT_OK`: Transmission request rejected (busy, invalid ID, etc.)
- **Side Effects:** Sets TX state to `WAITING_FOR_AUTHENTICATION`
- **Example:**
  ```c
  uint8 AuthPDU[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
  PduInfoType PduInfo = {
      .SduDataPtr = AuthPDU,
      .SduLength = 8
  };
  Std_ReturnType result = SecOC_IfTransmit(0, &PduInfo);
  if (result == E_OK) {
      // TX request accepted, will be processed in SecOC_MainFunctionTx()
  }
  ```

---

```c
void SecOC_MainFunctionTx(void);
```
- **Description:** Periodic TX processing function (called every `SECOC_MAIN_FUNCTION_PERIOD_TX` ms)
- **Parameters:** None
- **Returns:** None
- **Responsibilities:**
  - Get freshness values
  - Construct DataToAuthenticator
  - Generate MAC/signature
  - Build Secured I-PDU
  - Transmit via PduR
- **Calling Context:** Typically called from 1ms task or scheduler
- **Example:**
  ```c
  // In main loop or scheduler task
  while (1) {
      SecOC_MainFunctionTx();
      delay_ms(0.9);  // 0.9ms period
  }
  ```

---

### Reception (RX) API

```c
void SecOC_RxIndication(
    PduIdType RxPduId,
    const PduInfoType* PduInfoPtr
);
```
- **Description:** Indication of received Secured I-PDU
- **Parameters:**
  - `RxPduId`: RX PDU identifier
  - `PduInfoPtr`: Pointer to received PDU information
    - `SduDataPtr`: Secured PDU data
    - `SduLength`: Secured PDU length
- **Returns:** None
- **Side Effects:** Copies PDU to internal buffer, sets RX state to `WAITING_FOR_VERIFICATION`
- **Called By:** PduR layer (from CanIf, SoAd, etc.)
- **Example:**
  ```c
  // Called by PduR when Secured PDU received
  void PduR_CanIfRxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr) {
      SecOC_RxIndication(RxPduId, PduInfoPtr);
  }
  ```

---

```c
void SecOC_MainFunctionRx(void);
```
- **Description:** Periodic RX processing function (called every `SECOC_MAIN_FUNCTION_PERIOD_RX` ms)
- **Parameters:** None
- **Returns:** None
- **Responsibilities:**
  - Parse Secured I-PDU
  - Validate freshness
  - Verify MAC/signature
  - Forward Authentic I-PDU to upper layer (if verified)
- **Calling Context:** Typically called from 1ms task or scheduler
- **Example:**
  ```c
  // In main loop or scheduler task
  while (1) {
      SecOC_MainFunctionRx();
      delay_ms(0.9);  // 0.9ms period
  }
  ```

---

### Transport Protocol (TP) API

```c
BufReq_ReturnType SecOC_StartOfReception(
    PduIdType RxPduId,
    const PduInfoType* info,
    PduLengthType TpSduLength,
    PduLengthType* bufferSizePtr
);
```
- **Description:** Start of reception for large Secured I-PDU (TP mode)
- **Parameters:**
  - `RxPduId`: RX PDU identifier
  - `info`: Pointer to first frame data
  - `TpSduLength`: Total expected PDU length (all frames)
  - `bufferSizePtr`: Output - available buffer size
- **Returns:**
  - `BUFREQ_OK`: Buffer available, reception accepted
  - `BUFREQ_E_NOT_OK`: Insufficient buffer or error
- **Called By:** PduR (from SoAd TP or CanTP)
- **Example:**
  ```c
  PduLengthType bufferSize;
  BufReq_ReturnType result = SecOC_StartOfReception(
      0,              // RxPduId
      &firstFrame,    // First frame info
      3320,           // Total: 3320 bytes (PQC Secured PDU)
      &bufferSize     // Output: available buffer size
  );
  if (result == BUFREQ_OK) {
      // SecOC ready to receive, proceed with TP reception
  }
  ```

---

```c
BufReq_ReturnType SecOC_CopyRxData(
    PduIdType RxPduId,
    const PduInfoType* info,
    PduLengthType* bufferSizePtr
);
```
- **Description:** Copy received TP frame data to SecOC buffer
- **Parameters:**
  - `RxPduId`: RX PDU identifier
  - `info`: Pointer to frame data
  - `bufferSizePtr`: Output - remaining buffer size
- **Returns:**
  - `BUFREQ_OK`: Data copied successfully
  - `BUFREQ_E_NOT_OK`: Buffer overflow or error
- **Called By:** PduR (for each TP frame)
- **Example:**
  ```c
  PduLengthType remainingBuffer;
  BufReq_ReturnType result = SecOC_CopyRxData(
      0,                  // RxPduId
      &frameData,         // Frame 2 of 34
      &remainingBuffer    // Output: how much buffer left
  );
  ```

---

```c
void SecOC_TpRxIndication(
    PduIdType RxPduId,
    Std_ReturnType result
);
```
- **Description:** Indication that TP reception completed (all frames received)
- **Parameters:**
  - `RxPduId`: RX PDU identifier
  - `result`: E_OK (success) or E_NOT_OK (error)
- **Returns:** None
- **Side Effects:** Marks PDU as ready for verification
- **Called By:** PduR
- **Example:**
  ```c
  // After all 34 frames received
  SecOC_TpRxIndication(0, E_OK);
  // SecOC_MainFunctionRx() will now verify the PDU
  ```

---

## 12.2 FVM (Freshness Value Manager) API

```c
Std_ReturnType FVM_GetTxFreshness(
    uint16 FreshnessID,
    uint8* FreshnessValue,
    uint32* FreshnessValueLength
);
```
- **Description:** Get current TX freshness counter (full value)
- **Parameters:**
  - `FreshnessID`: Freshness identifier (maps to TX PDU ID)
  - `FreshnessValue`: Output - full freshness value (big-endian)
  - `FreshnessValueLength`: Output - freshness length in bytes
- **Returns:**
  - `E_OK`: Freshness value retrieved
  - `E_NOT_OK`: Invalid ID
- **Side Effects:** Increments freshness counter
- **Example:**
  ```c
  uint8 freshness[8];
  uint32 length;
  Std_ReturnType result = FVM_GetTxFreshness(0, freshness, &length);
  // freshness = [0x00, 0x00, 0x00, 0x00, 0x12, 0x34, 0x56, 0x78]
  // length = 8
  ```

---

```c
Std_ReturnType FVM_GetTxFreshnessTruncData(
    uint16 FreshnessID,
    uint8* FreshnessValue,
    uint32* FreshnessValueLength,
    uint8* TruncatedFreshnessValue,
    uint32* TruncatedFreshnessValueLength
);
```
- **Description:** Get TX freshness (both full and truncated portions)
- **Parameters:**
  - `FreshnessID`: Freshness identifier
  - `FreshnessValue`: Output - full freshness value
  - `FreshnessValueLength`: Output - full length
  - `TruncatedFreshnessValue`: Output - truncated portion (for transmission)
  - `TruncatedFreshnessValueLength`: Output - truncated length
- **Returns:** `E_OK` or `E_NOT_OK`
- **Example:**
  ```c
  uint8 fullFreshness[8];
  uint32 fullLength;
  uint8 truncFreshness[3];
  uint32 truncLength;

  FVM_GetTxFreshnessTruncData(0, fullFreshness, &fullLength, truncFreshness, &truncLength);
  // fullFreshness = [0x00, 0x00, 0x00, 0x00, 0x12, 0x34, 0x56, 0x78] (8 bytes)
  // truncFreshness = [0x56, 0x78] (lower 24 bits = 3 bytes)
  ```

---

```c
Std_ReturnType FVM_GetRxFreshness(
    uint16 FreshnessID,
    const uint8* TruncatedFreshnessValue,
    uint32 TruncatedFreshnessValueLength,
    uint16 AuthVerifyAttempts,
    uint8* FreshnessValue,
    uint32* FreshnessValueLength
);
```
- **Description:** Reconstruct full RX freshness from truncated value
- **Parameters:**
  - `FreshnessID`: Freshness identifier
  - `TruncatedFreshnessValue`: Input - truncated freshness (from Secured PDU)
  - `TruncatedFreshnessValueLength`: Input - truncated length
  - `AuthVerifyAttempts`: Verification attempt counter
  - `FreshnessValue`: Output - reconstructed full freshness
  - `FreshnessValueLength`: Output - full length
- **Returns:**
  - `E_OK`: Reconstruction successful, freshness valid (> last accepted)
  - `E_NOT_OK`: Reconstruction failed or freshness invalid (replay attack)
- **Example:**
  ```c
  uint8 truncFreshness[3] = {0x56, 0x78};  // From Secured PDU
  uint8 fullFreshness[8];
  uint32 fullLength;

  Std_ReturnType result = FVM_GetRxFreshness(
      0,                    // FreshnessID
      truncFreshness,       // Input: truncated
      3,                    // 3 bytes
      1,                    // First attempt
      fullFreshness,        // Output: full
      &fullLength           // Output: length
  );

  if (result == E_OK) {
      // fullFreshness = [0x00, 0x00, 0x00, 0x00, 0x12, 0x34, 0x56, 0x78]
      // Freshness is valid, proceed with verification
  } else {
      // Replay attack detected!
  }
  ```

---

## 12.3 Csm (Crypto Service Manager) API

```c
Std_ReturnType Csm_SignatureGenerate(
    uint32 jobId,
    Crypto_OperationModeType mode,
    const uint8* dataPtr,
    uint32 dataLength,
    uint8* signaturePtr,
    uint32* signatureLengthPtr
);
```
- **Description:** Generate digital signature (PQC mode)
- **Parameters:**
  - `jobId`: CSM job identifier (e.g., 10 for ML-DSA)
  - `mode`: `CRYPTO_OPERATIONMODE_SINGLECALL` (one-shot operation)
  - `dataPtr`: Data to sign (DataToAuthenticator)
  - `dataLength`: Data length in bytes
  - `signaturePtr`: Output - generated signature
  - `signatureLengthPtr`: Input/Output - signature buffer size / actual signature size
- **Returns:**
  - `E_OK`: Signature generated successfully
  - `E_NOT_OK`: Signing failed
- **Example:**
  ```c
  uint8 dataToAuth[18] = {...};  // [DataID | AuthPDU | Freshness]
  uint8 signature[3309];
  uint32 sigLen = sizeof(signature);

  Std_ReturnType result = Csm_SignatureGenerate(
      10,                            // Job ID (ML-DSA)
      CRYPTO_OPERATIONMODE_SINGLECALL,
      dataToAuth,
      18,
      signature,
      &sigLen
  );

  if (result == E_OK) {
      // sigLen = 3309 bytes (ML-DSA-65 signature)
  }
  ```

---

```c
Std_ReturnType Csm_SignatureVerify(
    uint32 jobId,
    Crypto_OperationModeType mode,
    const uint8* dataPtr,
    uint32 dataLength,
    const uint8* signaturePtr,
    uint32 signatureLength,
    Crypto_VerifyResultType* verifyPtr
);
```
- **Description:** Verify digital signature (PQC mode)
- **Parameters:**
  - `jobId`: CSM job identifier
  - `mode`: `CRYPTO_OPERATIONMODE_SINGLECALL`
  - `dataPtr`: Data to verify (DataToAuthenticator)
  - `dataLength`: Data length
  - `signaturePtr`: Signature to verify
  - `signatureLength`: Signature length
  - `verifyPtr`: Output - verification result
- **Returns:**
  - `E_OK`: Verification operation completed
  - `E_NOT_OK`: Verification operation failed
- **Verification Result (`verifyPtr`):**
  - `CRYPTO_E_VER_OK`: Signature valid
  - `CRYPTO_E_VER_NOT_OK`: Signature invalid (tampering detected)
- **Example:**
  ```c
  uint8 dataToAuth[18] = {...};
  uint8 signature[3309] = {...};  // From Secured PDU
  Crypto_VerifyResultType verifyResult;

  Std_ReturnType result = Csm_SignatureVerify(
      10,
      CRYPTO_OPERATIONMODE_SINGLECALL,
      dataToAuth,
      18,
      signature,
      3309,
      &verifyResult
  );

  if (result == E_OK && verifyResult == CRYPTO_E_VER_OK) {
      // Signature valid, message authentic
  } else {
      // Signature invalid, tampering detected
  }
  ```

---

## 12.4 PQC API

```c
Std_ReturnType PQC_Init(void);
```
- **Description:** Initialize PQC library (liboqs)
- **Returns:** `E_OK` or `E_NOT_OK`
- **Example:**
  ```c
  if (PQC_Init() == E_OK) {
      // PQC library initialized successfully
  }
  ```

---

```c
Std_ReturnType PQC_MLDSA_KeyGen(PQC_MLDSA_KeyPairType* KeyPair);
```
- **Description:** Generate ML-DSA-65 keypair
- **Parameters:**
  - `KeyPair`: Output - generated keypair
- **Returns:** `E_OK` or `E_NOT_OK`
- **Time:** ~5.23 ms (Raspberry Pi 4)
- **Example:**
  ```c
  PQC_MLDSA_KeyPairType keypair;
  Std_ReturnType result = PQC_MLDSA_KeyGen(&keypair);
  if (result == E_OK) {
      // keypair.PublicKey: 1952 bytes
      // keypair.SecretKey: 4032 bytes
      // keypair.IsValid: TRUE
  }
  ```

---

```c
Std_ReturnType PQC_MLDSA_Sign(
    const uint8* Message,
    uint32 MessageLength,
    const uint8* SecretKey,
    uint8* Signature,
    uint32* SignatureLength
);
```
- **Description:** Generate ML-DSA-65 signature
- **Parameters:**
  - `Message`: Data to sign
  - `MessageLength`: Message length
  - `SecretKey`: 4032-byte secret key
  - `Signature`: Output - generated signature (3309 bytes)
  - `SignatureLength`: Output - signature length
- **Returns:** `E_OK` or `E_NOT_OK`
- **Time:** ~8.13 ms (Raspberry Pi 4)
- **Example:**
  ```c
  uint8 message[18] = {...};
  uint8 signature[MLDSA65_SIGNATURE_BYTES];
  uint32 sigLen;

  Std_ReturnType result = PQC_MLDSA_Sign(
      message,
      18,
      keypair.SecretKey,
      signature,
      &sigLen
  );
  // sigLen = 3309 bytes
  ```

---

```c
Std_ReturnType PQC_MLDSA_Verify(
    const uint8* Message,
    uint32 MessageLength,
    const uint8* Signature,
    uint32 SignatureLength,
    const uint8* PublicKey
);
```
- **Description:** Verify ML-DSA-65 signature
- **Parameters:**
  - `Message`: Data to verify
  - `MessageLength`: Message length
  - `Signature`: Signature to verify (3309 bytes)
  - `SignatureLength`: Signature length
  - `PublicKey`: 1952-byte public key
- **Returns:**
  - `E_OK`: Signature valid
  - `E_NOT_OK`: Signature invalid
- **Time:** ~4.89 ms (Raspberry Pi 4)
- **Example:**
  ```c
  Std_ReturnType result = PQC_MLDSA_Verify(
      message,
      18,
      signature,
      3309,
      keypair.PublicKey
  );
  if (result == E_OK) {
      // Signature valid
  }
  ```

---

## 12.5 PQC Key Exchange API

```c
void PQC_KeyExchange_Init(void);
```
- **Description:** Initialize key exchange manager
- **Returns:** None

---

```c
Std_ReturnType PQC_KeyExchange_Initiate(
    PQC_PeerIdType PeerId,
    uint8* PublicKey
);
```
- **Description:** Initiate ML-KEM key exchange (Alice/initiator side)
- **Parameters:**
  - `PeerId`: Peer identifier (0-7)
  - `PublicKey`: Output - 1184-byte public key to send to Bob
- **Returns:** `E_OK` or `E_NOT_OK`
- **Time:** ~2.85 ms
- **Example:**
  ```c
  uint8 publicKey[MLKEM768_PUBLIC_KEY_BYTES];
  Std_ReturnType result = PQC_KeyExchange_Initiate(0, publicKey);
  if (result == E_OK) {
      // Send publicKey (1184 bytes) to peer via Ethernet
      ethernet_send(PEER_ID, publicKey, 1184);
  }
  ```

---

```c
Std_ReturnType PQC_KeyExchange_Respond(
    PQC_PeerIdType PeerId,
    const uint8* PeerPublicKey,
    uint8* Ciphertext
);
```
- **Description:** Respond to key exchange (Bob/responder side)
- **Parameters:**
  - `PeerId`: Peer identifier
  - `PeerPublicKey`: Input - Alice's 1184-byte public key
  - `Ciphertext`: Output - 1088-byte ciphertext to send to Alice
- **Returns:** `E_OK` or `E_NOT_OK`
- **Time:** ~3.12 ms
- **Example:**
  ```c
  uint8 peerPublicKey[1184];  // Received from Alice
  uint8 ciphertext[MLKEM768_CIPHERTEXT_BYTES];

  Std_ReturnType result = PQC_KeyExchange_Respond(0, peerPublicKey, ciphertext);
  if (result == E_OK) {
      // Send ciphertext (1088 bytes) to Alice
      ethernet_send(PEER_ID, ciphertext, 1088);
      // Shared secret now available for Bob
  }
  ```

---

```c
Std_ReturnType PQC_KeyExchange_Complete(
    PQC_PeerIdType PeerId,
    const uint8* Ciphertext
);
```
- **Description:** Complete key exchange (Alice side, after receiving ciphertext)
- **Parameters:**
  - `PeerId`: Peer identifier
  - `Ciphertext`: Input - Bob's 1088-byte ciphertext
- **Returns:** `E_OK` or `E_NOT_OK`
- **Time:** ~3.89 ms
- **Example:**
  ```c
  uint8 ciphertext[1088];  // Received from Bob
  Std_ReturnType result = PQC_KeyExchange_Complete(0, ciphertext);
  if (result == E_OK) {
      // Shared secret now available for Alice
  }
  ```

---

```c
Std_ReturnType PQC_KeyExchange_GetSharedSecret(
    PQC_PeerIdType PeerId,
    uint8* SharedSecret
);
```
- **Description:** Retrieve shared secret after key exchange
- **Parameters:**
  - `PeerId`: Peer identifier
  - `SharedSecret`: Output - 32-byte shared secret
- **Returns:** `E_OK` or `E_NOT_OK`
- **Example:**
  ```c
  uint8 sharedSecret[32];
  Std_ReturnType result = PQC_KeyExchange_GetSharedSecret(0, sharedSecret);
  if (result == E_OK) {
      // Both Alice and Bob now have identical 32-byte shared secret
      // Derive session keys using HKDF
      PQC_DeriveSessionKeys(sharedSecret, 0, &sessionKeys);
  }
  ```

---

## 12.6 HKDF API

```c
Std_ReturnType PQC_DeriveSessionKeys(
    const uint8* SharedSecret,
    PQC_PeerIdType PeerId,
    PQC_SessionKeysType* SessionKeys
);
```
- **Description:** Derive encryption and authentication keys from ML-KEM shared secret
- **Parameters:**
  - `SharedSecret`: Input - 32-byte shared secret from ML-KEM
  - `PeerId`: Peer identifier (for key storage)
  - `SessionKeys`: Output - derived session keys
- **Returns:** `E_OK` or `E_NOT_OK`
- **Time:** ~0.3 ms
- **Example:**
  ```c
  uint8 sharedSecret[32];  // From PQC_KeyExchange_GetSharedSecret()
  PQC_SessionKeysType sessionKeys;

  Std_ReturnType result = PQC_DeriveSessionKeys(sharedSecret, 0, &sessionKeys);
  if (result == E_OK) {
      // sessionKeys.EncryptionKey: 32 bytes (for AES-256-GCM)
      // sessionKeys.AuthenticationKey: 32 bytes (for HMAC-SHA256)
      // sessionKeys.IsValid: TRUE
  }
  ```

---

## 12.7 Data Types Reference

### SecOC Types

```c
// PDU type enumeration
typedef enum {
    SECOC_IFPDU,    // Interface PDU (direct transmission, < 64 bytes)
    SECOC_TPPDU     // Transport Protocol PDU (segmented, large PDUs)
} SecOC_PduType_Type;

// State enumeration
typedef enum {
    SECOC_IDLE,
    SECOC_WAITING_FOR_AUTHENTICATION,
    SECOC_AUTHENTICATED,
    SECOC_WAITING_FOR_VERIFICATION,
    SECOC_VERIFIED,
    SECOC_VERIFICATION_FAILED
} SecOC_StateType;

// PDU information structure (AUTOSAR standard)
typedef struct {
    uint8* SduDataPtr;           // Pointer to PDU data
    uint8* MetaDataPtr;          // Metadata (optional)
    PduLengthType SduLength;     // PDU length in bytes
} PduInfoType;
```

### Crypto Types

```c
// Operation mode
typedef enum {
    CRYPTO_OPERATIONMODE_START,
    CRYPTO_OPERATIONMODE_UPDATE,
    CRYPTO_OPERATIONMODE_FINISH,
    CRYPTO_OPERATIONMODE_SINGLECALL
} Crypto_OperationModeType;

// Verification result
typedef enum {
    CRYPTO_E_VER_OK,        // Verification successful
    CRYPTO_E_VER_NOT_OK     // Verification failed
} Crypto_VerifyResultType;
```

### PQC Types

```c
// ML-KEM keypair
typedef struct {
    uint8 PublicKey[MLKEM768_PUBLIC_KEY_BYTES];   // 1184 bytes
    uint8 SecretKey[MLKEM768_SECRET_KEY_BYTES];   // 2400 bytes
    boolean IsValid;
} PQC_MLKEM_KeyPairType;

// ML-DSA keypair
typedef struct {
    uint8 PublicKey[MLDSA65_PUBLIC_KEY_BYTES];    // 1952 bytes
    uint8 SecretKey[MLDSA65_SECRET_KEY_BYTES];    // 4032 bytes
    boolean IsValid;
} PQC_MLDSA_KeyPairType;

// Session keys (from HKDF)
typedef struct {
    uint8 EncryptionKey[32];      // For AES-256-GCM
    uint8 AuthenticationKey[32];  // For HMAC-SHA256
    boolean IsValid;
} PQC_SessionKeysType;
```

---

## 12.8 Error Codes

| Error Code | Value | Description |
|------------|-------|-------------|
| `E_OK` | 0 | Operation successful |
| `E_NOT_OK` | 1 | Operation failed (generic error) |
| `BUFREQ_OK` | 0 | Buffer request successful |
| `BUFREQ_E_NOT_OK` | 1 | Buffer request failed |
| `BUFREQ_E_BUSY` | 2 | Buffer temporarily unavailable |
| `BUFREQ_E_OVFL` | 3 | Buffer overflow |
| `CRYPTO_E_VER_OK` | 0 | Verification successful |
| `CRYPTO_E_VER_NOT_OK` | 1 | Verification failed |

---

# Conclusion

This comprehensive architecture documentation covers all aspects of the AUTOSAR SecOC implementation with Post-Quantum Cryptography extensions, from high-level architecture to low-level API details.

**Key Takeaways:**
1. **Modular Design:** Clear separation between AUTOSAR layers enables platform portability
2. **PQC Integration:** ML-KEM and ML-DSA provide quantum-resistant security
3. **Performance Trade-offs:** PQC adds ~8ms latency and 207x bandwidth overhead
4. **Platform Abstraction:** Windows and Linux support via abstraction layers
5. **Comprehensive API:** Complete API reference for all modules

This document serves as:
- **Implementation Reference** for developers
- **Architecture Overview** for system designers
- **API Documentation** for integrators
- **Thesis Technical Documentation** for academic submission

**Next Steps:**
- Refer to TECHNICAL_REPORT.md for motivation, results, and validation
- Refer to DIAGRAMS_THESIS.md for presentation-ready diagrams
- Refer to DOCUMENTATION_LINKING_STRATEGY.md for navigation between documents

---

**Document Version:** 1.0
**Last Updated:** 2025-11-16
**Author:** AUTOSAR SecOC PQC Project Team
