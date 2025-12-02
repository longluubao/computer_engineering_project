# AUTOSAR SecOC with Post-Quantum Cryptography
## Thesis Presentation Diagrams - Storytelling Flow

**Author:** LLU2HC
**Project:** Bachelor's Graduation Project - Ethernet Gateway with PQC
**Purpose:** Diagrams organized for thesis defense presentation
**Total Diagrams:** 21 Mermaid diagrams arranged in narrative flow

---

## How to Use This Document

This document reorganizes all 21 diagrams from `DIAGRAMS.md` into a **thesis storytelling flow**. Each chapter builds on the previous one to create a compelling narrative:

1. **Introduction** - The big picture
2. **Communication Paths** - How data flows through the system
3. **Hardware Platform** - Physical implementation
4. **Core Cryptography** - PQC building blocks
5. **Software Architecture** - AUTOSAR integration
6. **Security Properties** - Attack prevention mechanisms
7. **Engineering Challenges** - Critical problems solved
8. **Performance Analysis** - Trade-offs and metrics
9. **Testing and Validation** - Proof of correctness

---

# Chapter 1: Introduction - The Big Picture

## 1.1 Ethernet Gateway System Overview

**Thesis Context:** "This is our complete system - a Raspberry Pi 4 Ethernet Gateway that bridges CAN bus networks with Ethernet backend systems using Post-Quantum Cryptography."

**Key Points to Highlight:**
- Classical MAC on CAN bus (existing vehicle network)
- PQC signatures on Ethernet (quantum-resistant backend)
- AUTOSAR SecOC stack in the middle (standard-compliant implementation)
- Raspberry Pi 4 as gateway hardware platform

```mermaid
graph TB
    subgraph "Vehicle CAN Network"
        ENGINE[Engine ECU<br/>Speed, RPM]
        BRAKE[Brake ECU<br/>ABS Status]
        STEERING[Steering ECU<br/>Angle]

        ENGINE --> CAN_BUS[CAN Bus<br/>500 kbps<br/>Classical MAC]
        BRAKE --> CAN_BUS
        STEERING --> CAN_BUS
    end

    subgraph "Raspberry Pi 4 - Ethernet Gateway"
        CAN_CTRL[MCP2515<br/>CAN Controller<br/>SPI Interface]

        subgraph "AUTOSAR SecOC Stack"
            COM[COM Manager]
            SECOC[SecOC Engine<br/>Freshness Mgmt]
            CSM[Crypto Service Mgr]
            PQC[PQC Module<br/>ML-DSA-65 Signatures<br/>ML-KEM-768 Key Exchange]
            PDUR[PduR Router]
        end

        ETH_STACK[Ethernet Stack<br/>TCP/IP<br/>Port 12345]
    end

    subgraph "Backend Network"
        CENTRAL[Central ECU<br/>192.168.1.200]
        TELEMATICS[Telematics Unit<br/>Cloud Gateway]
        DIAGNOSTIC[Diagnostic Tool<br/>OBD-II Interface]
    end

    CAN_BUS --> CAN_CTRL
    CAN_CTRL --> COM
    COM --> SECOC
    SECOC --> CSM
    CSM --> PQC
    PQC --> CSM
    CSM --> SECOC
    SECOC --> PDUR
    PDUR --> ETH_STACK

    ETH_STACK --> ETH_NET[Ethernet<br/>100 Mbps]
    ETH_NET --> CENTRAL
    ETH_NET --> TELEMATICS
    ETH_NET --> DIAGNOSTIC

    style CAN_BUS fill:#8bc34a
    style CAN_CTRL fill:#ffc107
    style SECOC fill:#ff9800
    style PQC fill:#f57c00
    style ETH_NET fill:#4caf50
    style CENTRAL fill:#2196f3
```

**Talking Points:**
- Vehicle ECUs communicate via CAN bus (industry standard)
- Gateway receives CAN messages, transforms them for Ethernet
- PQC module provides quantum-resistant security for backend communication
- Backend systems (Central ECU, Telematics, Diagnostics) access vehicle data securely

---

# Chapter 2: Communication Paths - How Data Flows

## 2.1 CAN to Ethernet Transformation (TX Path)

**Thesis Context:** "When a vehicle ECU sends data to the backend, the gateway performs this transformation: small CAN message with classical MAC becomes large Ethernet PDU with PQC signature."

**Key Points to Highlight:**
- Input: 24 bytes (8 data + 16 MAC) from CAN
- Process: Verify classical MAC → Extract data → Sign with ML-DSA-65
- Output: 3,320 bytes (8 data + 3 freshness + 3,309 signature) to Ethernet
- Quantum-resistant signature generated in ~8.1ms

```mermaid
sequenceDiagram
    participant ECU as Vehicle ECU<br/>(CAN)
    participant CAN as CAN Bus<br/>500 kbps
    participant GW as Ethernet Gateway<br/>(Raspberry Pi 4)
    participant SecOC as SecOC Engine
    participant PQC as PQC Module
    participant ETH as Ethernet<br/>100 Mbps
    participant Backend as Central ECU

    Note over ECU,CAN: Classical MAC (16 bytes)
    ECU->>CAN: CAN Frame (8 bytes + MAC)
    Note over CAN: Total: 24 bytes

    CAN->>GW: MCP2515 SPI Interface
    GW->>SecOC: Receive CAN Message
    SecOC->>SecOC: Verify Classical MAC

    alt MAC Valid
        SecOC->>SecOC: Extract Authentic PDU (8 bytes)
        SecOC->>SecOC: Increment Freshness Counter
        Note over SecOC: Freshness = Counter++

        SecOC->>PQC: Sign with ML-DSA-65
        Note over PQC: Sign [MessageID + Data + Freshness]<br/>Time: ~8.1ms

        PQC-->>SecOC: PQC Signature (3,309 bytes)
        SecOC->>SecOC: Build Secured PDU<br/>[Data(8) + Freshness(3) + Sig(3309)]

        Note over SecOC,ETH: Total: 3,320 bytes (PQC)
        SecOC->>ETH: Ethernet Transmission
        ETH->>Backend: TCP/IP Port 12345

        Backend->>Backend: Verify PQC Signature
        Note over Backend: Quantum-Resistant!<br/>Time: ~4.9ms

        Backend-->>Backend: Authentic Message<br/>Accepted
    else MAC Invalid
        SecOC->>SecOC: Drop Message
        Note over SecOC: Attack Detected!
    end
```

**Talking Points:**
- Gateway acts as protocol translator AND security upgrader
- Classical MAC ensures CAN message integrity
- PQC signature ensures quantum-resistant backend integrity
- 8.1ms signing time acceptable for non-real-time backend communication

---

## 2.2 Ethernet to CAN Transformation (RX Path)

**Thesis Context:** "The reverse path: large PQC-signed Ethernet PDU is verified and transformed back to small CAN message with classical MAC."

**Key Points to Highlight:**
- Input: 3,320 bytes from Ethernet
- Process: Check freshness → Verify PQC signature → Extract data → Generate classical MAC
- Output: 24 bytes to CAN
- Two security checks: replay detection (freshness) and tampering detection (signature)

```mermaid
sequenceDiagram
    participant Backend as Central ECU
    participant ETH as Ethernet<br/>100 Mbps
    participant GW as Ethernet Gateway<br/>(Raspberry Pi 4)
    participant SecOC as SecOC Engine
    participant PQC as PQC Module
    participant CAN as CAN Bus
    participant ECU as Vehicle ECU

    Backend->>ETH: Secured PDU (PQC Signed)
    Note over ETH: [Data + Freshness + Signature(3309B)]

    ETH->>GW: TCP/IP Receive
    GW->>SecOC: Process Secured PDU

    SecOC->>SecOC: Check Freshness Value
    alt Freshness > Last
        SecOC->>PQC: Verify ML-DSA-65 Signature
        Note over PQC: Verify [MessageID + Data + Freshness]<br/>Time: ~4.9ms

        PQC-->>SecOC: Verification Result

        alt Signature Valid
            SecOC->>SecOC: Update Freshness Counter
            SecOC->>SecOC: Extract Authentic PDU
            SecOC->>SecOC: Generate Classical MAC
            Note over SecOC: MAC for CAN (16 bytes)

            SecOC->>CAN: CAN Frame + MAC
            Note over CAN: Total: 24 bytes
            CAN->>ECU: Deliver Message
            ECU->>ECU: Authentic Message
        else Signature Invalid
            SecOC->>SecOC: Drop Message
            Note over SecOC: Tampering Detected!
        end
    else Freshness <= Last
        SecOC->>SecOC: Drop Message
        Note over SecOC: Replay Attack Detected!
    end
```

**Talking Points:**
- Two-layer security: freshness check first (fast), then signature verification
- Freshness counter prevents replay attacks (attacker can't reuse old messages)
- Invalid signature indicates tampering (data was modified in transit)
- Successfully verified messages converted back to CAN format

---

## 2.3 Message Format Comparison

**Thesis Context:** "This shows the dramatic size difference between CAN and Ethernet secured PDUs - a necessary trade-off for quantum resistance."

**Key Points to Highlight:**
- CAN: 28-84 bytes total (compact for bandwidth-limited bus)
- Ethernet: 3,320-4,337 bytes total (138x larger due to PQC signature)
- Gateway performs transformation between these formats
- Ethernet bandwidth (100 Mbps) easily accommodates larger PDUs

```mermaid
graph TB
    subgraph "CAN Bus PDU (Classical MAC)"
        direction LR
        CAN_HEADER[Header<br/>1 byte]
        CAN_DATA[Authentic PDU<br/>8-64 bytes]
        CAN_FV[Freshness<br/>3 bytes]
        CAN_MAC[Classical MAC<br/>16 bytes]

        CAN_HEADER --> CAN_DATA
        CAN_DATA --> CAN_FV
        CAN_FV --> CAN_MAC

        CAN_TOTAL[Total Size:<br/>28-84 bytes]
    end

    subgraph "Ethernet PDU (PQC Signature)"
        direction LR
        ETH_HEADER[Header<br/>1 byte]
        ETH_DATA[Authentic PDU<br/>8-1024 bytes]
        ETH_FV[Freshness<br/>3 bytes]
        ETH_SIG[PQC Signature<br/>3,309 bytes]

        ETH_HEADER --> ETH_DATA
        ETH_DATA --> ETH_FV
        ETH_FV --> ETH_SIG

        ETH_TOTAL[Total Size:<br/>3,320-4,337 bytes]
    end

    CAN_TOTAL -.->|Gateway Transformation| ETH_TOTAL

    style CAN_MAC fill:#4caf50
    style ETH_SIG fill:#ff9800
    style CAN_TOTAL fill:#81c784
    style ETH_TOTAL fill:#ffb74d
```

**Talking Points:**
- Classical MAC: 16 bytes (vulnerable to quantum attacks)
- PQC Signature: 3,309 bytes (quantum-resistant, 206x larger)
- Size increase justified: Ethernet has 200x more bandwidth than CAN
- Hybrid approach: classical for CAN (resource-constrained), PQC for Ethernet (quantum-threatened)

---

# Chapter 3: Hardware Platform - Physical Implementation

## 3.1 Raspberry Pi 4 Gateway Hardware

**Thesis Context:** "Our gateway runs on affordable, commercially available hardware - a Raspberry Pi 4 with external CAN controller."

**Key Points to Highlight:**
- Raspberry Pi 4: Quad-core ARM Cortex-A72 @ 1.5GHz (sufficient for PQC operations)
- 4GB RAM (adequate for AUTOSAR stack + liboqs library)
- Gigabit Ethernet (100x more bandwidth than required)
- MCP2515 CAN controller via SPI (standard automotive interface)
- Total cost: ~$50-60 (very affordable for automotive gateway)

```mermaid
graph TB
    subgraph "Raspberry Pi 4 Hardware"
        CPU[ARM Cortex-A72<br/>Quad-Core @ 1.5GHz<br/>ARMv8-A 64-bit]
        RAM[4GB LPDDR4-3200]
        GPIO[40-Pin GPIO Header]
        ETH_HW[Gigabit Ethernet<br/>RJ45 Port]
        SD[microSD Card<br/>32GB]

        CPU --- RAM
        CPU --- GPIO
        CPU --- ETH_HW
        CPU --- SD
    end

    subgraph "External Hardware"
        MCP2515[MCP2515<br/>CAN Controller<br/>SPI @ 8MHz]
        CAN_PHY[TJA1050<br/>CAN Transceiver]

        GPIO -.->|SPI| MCP2515
        MCP2515 --- CAN_PHY
    end

    subgraph "Software Stack"
        LINUX[Linux Kernel 6.1<br/>64-bit ARM]
        LIBOQS[liboqs Library<br/>ARM-Optimized Build]
        AUTOSAR[AUTOSAR SecOC<br/>+ PQC Integration]

        SD --> LINUX
        LINUX --> LIBOQS
        LIBOQS --> AUTOSAR
    end

    CAN_PHY --> CAN_BUS[CAN Bus<br/>Vehicle Network]
    ETH_HW --> ETH_NET[Ethernet<br/>Backend Network]

    AUTOSAR -.->|Control| MCP2515
    AUTOSAR -.->|Transmit| ETH_HW

    style CPU fill:#ff9800
    style LIBOQS fill:#f57c00
    style AUTOSAR fill:#2196f3
    style MCP2515 fill:#8bc34a
    style CAN_BUS fill:#8bc34a
    style ETH_NET fill:#4caf50
```

**Talking Points:**
- ARM Cortex-A72 powerful enough for PQC operations (ML-DSA-65 sign ~8ms)
- Linux kernel provides POSIX sockets for Ethernet communication
- liboqs ARM-optimized build leverages NEON SIMD instructions
- MCP2515 industry-standard CAN controller, compatible with automotive networks
- Total BOM cost < $100 (including enclosure, power supply)

---

## 3.2 Dual-Platform Build System

**Thesis Context:** "We developed on Windows for convenience, then deployed on Raspberry Pi for production - CMake handles platform differences."

**Key Points to Highlight:**
- Windows (MSYS2/MinGW64): Development and testing environment
- Raspberry Pi (GCC ARM): Production deployment platform
- Common AUTOSAR codebase (platform-independent)
- Platform-specific Ethernet implementation (Winsock2 vs BSD sockets)
- CMake detects platform and selects appropriate source files

```mermaid
graph TB
    subgraph "Common AUTOSAR SecOC Application"
        APP[AUTOSAR SecOC + PQC<br/>Common Codebase]
    end

    subgraph "Platform Abstraction Layer"
        direction LR

        subgraph "Windows Development"
            WIN_SOCK[Winsock2 API]
            WIN_IMPL[ethernet_windows.c]
            WIN_TOOLS[MSYS2/MinGW64<br/>x86_64]
            WIN_FLAGS[-DWINDOWS]
            WIN_LIBOQS[liboqs<br/>x86_64 build]

            WIN_SOCK --> WIN_IMPL
            WIN_IMPL --> WIN_TOOLS
            WIN_TOOLS --> WIN_FLAGS
            WIN_FLAGS --> WIN_LIBOQS
        end

        subgraph "Raspberry Pi Deployment"
            RPI_SOCK[BSD Sockets API]
            RPI_IMPL[ethernet.c]
            RPI_HW[Raspberry Pi 4<br/>ARM Cortex-A72]
            RPI_FLAGS[-DRASPBERRY_PI<br/>-mcpu=cortex-a72]
            RPI_LIBOQS[liboqs<br/>ARM-optimized build]

            RPI_SOCK --> RPI_IMPL
            RPI_IMPL --> RPI_HW
            RPI_HW --> RPI_FLAGS
            RPI_FLAGS --> RPI_LIBOQS
        end
    end

    subgraph "Build System"
        CMAKE[CMake Platform<br/>Detection]
        BUILD[build_and_run.sh]

        CMAKE --> BUILD
    end

    APP --> WIN_IMPL
    APP --> RPI_IMPL
    BUILD --> WIN_FLAGS
    BUILD --> RPI_FLAGS

    style APP fill:#2196f3
    style WIN_IMPL fill:#ff9800
    style RPI_IMPL fill:#4caf50
    style CMAKE fill:#9c27b0
```

**Talking Points:**
- Winsock2 (Windows) vs BSD sockets (Linux) - different APIs, same functionality
- ethernet_windows.c vs ethernet.c - platform-specific implementations
- liboqs builds for both x86_64 (development) and ARM (deployment)
- CMake `WIN32` vs `UNIX` detection selects correct source files
- This approach enables rapid development on Windows, production deployment on RPi

---

# Chapter 4: Core Cryptography - PQC Building Blocks

## 4.1 PQC Algorithms Overview

**Thesis Context:** "Our system uses two NIST-standardized Post-Quantum algorithms: ML-KEM-768 for key exchange and ML-DSA-65 for message authentication."

**Key Points to Highlight:**
- ML-KEM-768: NIST FIPS 203 (key encapsulation mechanism, NIST Security Category 3)
- ML-DSA-65: NIST FIPS 204 (digital signature algorithm, NIST Security Category 3)
- liboqs: Open Quantum Safe reference implementation (NIST-approved)
- Use cases: ML-KEM for session establishment (one-time), ML-DSA for per-message authentication

```mermaid
graph TB
    subgraph "Ethernet Gateway - PQC Integration"
        subgraph "ML-KEM-768 (Session Establishment)"
            KEM_PURPOSE[Use Case:<br/>Gateway-to-Backend<br/>Secure Channel]
            KEM_KEYGEN[KeyGen: 2.85ms]
            KEM_ENCAPS[Encapsulation: 3.12ms]
            KEM_DECAPS[Decapsulation: 3.89ms]
            KEM_SIZES[Sizes:<br/>PK: 1,184B<br/>SK: 2,400B<br/>CT: 1,088B<br/>SS: 32B]

            KEM_PURPOSE --> KEM_KEYGEN
            KEM_KEYGEN --> KEM_ENCAPS
            KEM_ENCAPS --> KEM_DECAPS
            KEM_DECAPS --> KEM_SIZES
        end

        subgraph "ML-DSA-65 (Message Authentication)"
            DSA_PURPOSE[Use Case:<br/>Per-Message<br/>Authentication]
            DSA_KEYGEN[KeyGen: 5.23ms]
            DSA_SIGN[Sign: 8.13ms]
            DSA_VERIFY[Verify: 4.89ms]
            DSA_SIZES[Sizes:<br/>PK: 1,952B<br/>SK: 4,032B<br/>Signature: 3,309B]

            DSA_PURPOSE --> DSA_KEYGEN
            DSA_KEYGEN --> DSA_SIGN
            DSA_SIGN --> DSA_VERIFY
            DSA_VERIFY --> DSA_SIZES
        end

        subgraph "liboqs Library"
            OQS_CORE[NIST Reference<br/>Implementation<br/>ARM-Optimized]
            OQS_KEM_API[OQS_KEM_ml_kem_768]
            OQS_SIG_API[OQS_SIG_ml_dsa_65]

            OQS_CORE --> OQS_KEM_API
            OQS_CORE --> OQS_SIG_API
        end
    end

    KEM_KEYGEN --> OQS_KEM_API
    KEM_ENCAPS --> OQS_KEM_API
    KEM_DECAPS --> OQS_KEM_API

    DSA_KEYGEN --> OQS_SIG_API
    DSA_SIGN --> OQS_SIG_API
    DSA_VERIFY --> OQS_SIG_API

    style KEM_PURPOSE fill:#4caf50,color:#fff
    style DSA_PURPOSE fill:#ff9800,color:#fff
    style OQS_CORE fill:#1565c0,color:#fff
    style DSA_SIGN fill:#f57c00,color:#fff
```

**Talking Points:**
- ML-KEM-768: Based on Module-LWE hardness (quantum-resistant)
- ML-DSA-65: Based on Module-SIS hardness (quantum-resistant)
- NIST Category 3: Security equivalent to AES-192 against quantum attacks
- Performance: KeyGen/Encaps/Decaps < 4ms, Sign ~8ms, Verify ~5ms
- liboqs provides reference implementations verified by NIST

---

## 4.2 ML-KEM-768 Key Exchange (Session Establishment)

**Thesis Context:** "Before exchanging messages, gateway and backend perform ML-KEM key exchange to establish a shared secret, then derive session keys using HKDF."

**Key Points to Highlight:**
- One-time handshake per session (~10ms total)
- Initiator generates keypair, sends public key (1,184 bytes)
- Responder encapsulates shared secret, sends ciphertext (1,088 bytes)
- Initiator decapsulates to recover shared secret (32 bytes)
- HKDF derives 32-byte encryption key + 32-byte authentication key

```mermaid
sequenceDiagram
    participant Gateway as Ethernet Gateway<br/>(Raspberry Pi)
    participant SoAd as SoAd_PQC Module
    participant PQC_KE as PQC_KeyExchange
    participant PQC_KD as PQC_KeyDerivation
    participant ETH as Ethernet Network
    participant Backend as Central ECU

    Note over Gateway,Backend: ML-KEM-768 SESSION ESTABLISHMENT

    Gateway->>SoAd: SoAd_PQC_KeyExchange(PeerId=0, IsInitiator=TRUE)
    activate SoAd

    SoAd->>SoAd: State = KEY_EXCHANGE_INITIATED

    SoAd->>PQC_KE: PQC_KeyExchange_Initiate(PeerId=0)
    activate PQC_KE

    PQC_KE->>PQC_KE: OQS_KEM_keypair()<br/>Time: 2.85ms
    Note over PQC_KE: Generate ML-KEM-768 keypair<br/>Public: 1184 bytes<br/>Secret: 2400 bytes

    PQC_KE-->>SoAd: Public Key (1184 bytes)
    deactivate PQC_KE

    SoAd->>ETH: ethernet_send(PeerId=0, PublicKey, 1184)
    ETH->>Backend: TCP/IP Port 12345

    Note over Backend: Responder Flow

    Backend->>Backend: ethernet_receive()<br/>Receive Public Key
    Backend->>Backend: PQC_KeyExchange_Respond()
    Backend->>Backend: OQS_KEM_encaps(PublicKey)<br/>Time: 3.12ms
    Note over Backend: Encapsulate<br/>Ciphertext: 1088 bytes<br/>Shared Secret: 32 bytes

    Backend->>ETH: ethernet_send(Ciphertext, 1088)
    ETH->>SoAd: TCP/IP

    SoAd->>SoAd: ethernet_receive()<br/>Ciphertext (1088 bytes)
    SoAd->>SoAd: State = KEY_EXCHANGE_COMPLETED

    SoAd->>PQC_KE: PQC_KeyExchange_Complete(PeerId=0, Ciphertext)
    activate PQC_KE

    PQC_KE->>PQC_KE: OQS_KEM_decaps(Ciphertext)<br/>Time: 3.89ms
    Note over PQC_KE: Decapsulate<br/>Recover Shared Secret: 32 bytes

    PQC_KE-->>SoAd: Shared Secret (32 bytes)
    deactivate PQC_KE

    SoAd->>PQC_KD: PQC_DeriveSessionKeys(SharedSecret, PeerId=0)
    activate PQC_KD

    PQC_KD->>PQC_KD: HKDF-Extract<br/>PRK = HMAC-SHA256(salt, SharedSecret)
    PQC_KD->>PQC_KD: HKDF-Expand<br/>EncryptionKey = HMAC-SHA256(PRK, "Encryption-Key" || 0x01)
    PQC_KD->>PQC_KD: HKDF-Expand<br/>AuthenticationKey = HMAC-SHA256(PRK, "Authentication-Key" || 0x01)
    Note over PQC_KD: Time: 0.3ms

    PQC_KD-->>SoAd: SessionKeys<br/>32B Encryption + 32B Auth
    deactivate PQC_KD

    SoAd->>SoAd: Store PQC_SessionKeys[0]<br/>State = SESSION_ESTABLISHED
    deactivate SoAd

    Note over Gateway,Backend: SESSION READY - Both peers have identical session keys<br/>Total Handshake Time: ~10.16ms (one-time cost)

    style SoAd fill:#ff9800,color:#fff
    style PQC_KE fill:#f57c00,color:#fff
    style PQC_KD fill:#f57c00,color:#fff
```

**Talking Points:**
- Three-step handshake: KeyGen → Encaps → Decaps
- Total time: 2.85 + 3.12 + 3.89 = 9.86ms (one-time cost per session)
- Bandwidth: 1,184 bytes (public key) + 1,088 bytes (ciphertext) = 2,272 bytes
- Output: 32-byte shared secret (same on both sides)
- HKDF adds 0.3ms to derive encryption + authentication keys

---

## 4.3 HKDF Session Key Derivation

**Thesis Context:** "The ML-KEM shared secret is not used directly - we use HKDF to derive two independent keys for encryption and authentication."

**Key Points to Highlight:**
- HKDF-Extract: Derive pseudorandom key (PRK) from shared secret
- HKDF-Expand: Derive encryption key and authentication key from PRK
- Key independence: Encryption key ≠ Authentication key ≠ Shared secret
- Deterministic: Same shared secret → Same session keys (both peers)
- Forward secrecy: Different shared secret → Different session keys

```mermaid
graph TB
    subgraph "INPUT"
        SS[ML-KEM Shared Secret<br/>32 bytes<br/>From OQS_KEM_decaps]
        SALT[HKDF Salt<br/>AUTOSAR-SecOC-PQC-v1.0]
        INFO_ENC[Info String: Encryption-Key]
        INFO_AUTH[Info String: Authentication-Key]
    end

    subgraph "HKDF-Extract Phase"
        CONCAT1[Concatenate: Salt || SharedSecret]
        SHA256_1[SHA-256 Hash]
        PRK[Pseudorandom Key PRK<br/>32 bytes]

        SS --> CONCAT1
        SALT --> CONCAT1
        CONCAT1 --> SHA256_1
        SHA256_1 --> PRK
    end

    subgraph "HKDF-Expand Phase Encryption Key"
        CONCAT2[Concatenate: PRK || Info_Enc || 0x01]
        SHA256_2[SHA-256 Hash]
        ENC_KEY[Encryption Key<br/>32 bytes<br/>For AES-256-GCM]

        PRK --> CONCAT2
        INFO_ENC --> CONCAT2
        CONCAT2 --> SHA256_2
        SHA256_2 --> ENC_KEY
    end

    subgraph "HKDF-Expand Phase Authentication Key"
        CONCAT3[Concatenate: PRK || Info_Auth || 0x01]
        SHA256_3[SHA-256 Hash]
        AUTH_KEY[Authentication Key<br/>32 bytes<br/>For HMAC-SHA256]

        PRK --> CONCAT3
        INFO_AUTH --> CONCAT3
        CONCAT3 --> SHA256_3
        SHA256_3 --> AUTH_KEY
    end

    subgraph "OUTPUT: PQC_SessionKeysType"
        SESSION[Session Keys Stored<br/>PQC_SessionKeys PeerId]
        ENC_FINAL[EncryptionKey 32B]
        AUTH_FINAL[AuthenticationKey 32B]
        VALID[IsValid = TRUE]

        ENC_KEY --> ENC_FINAL
        AUTH_KEY --> AUTH_FINAL
        ENC_FINAL --> SESSION
        AUTH_FINAL --> SESSION
        VALID --> SESSION
    end

    subgraph "Security Properties"
        PROP1[Key Independence:<br/>EncKey != AuthKey]
        PROP2[Forward Secrecy:<br/>Different SS -> Different Keys]
        PROP3[Deterministic:<br/>Same SS -> Same Keys]
    end

    SESSION --> PROP1
    SESSION --> PROP2
    SESSION --> PROP3

    style PRK fill:#ff9800
    style ENC_KEY fill:#4caf50
    style AUTH_KEY fill:#2196f3
    style SESSION fill:#f57c00,color:#fff
```

**Talking Points:**
- HKDF-Extract: PRK = HMAC-SHA256(salt, shared_secret)
- HKDF-Expand (Encryption): EncKey = HMAC-SHA256(PRK, "Encryption-Key" || 0x01)
- HKDF-Expand (Authentication): AuthKey = HMAC-SHA256(PRK, "Authentication-Key" || 0x01)
- Info strings ensure key independence (different contexts)
- PRK never used directly (defense in depth)

---

# Chapter 5: Software Architecture - AUTOSAR Integration

## 5.1 Phase 3 Complete Architecture

**Thesis Context:** "Phase 3 completed our PQC implementation by adding ML-KEM key exchange, HKDF key derivation, and SoAd_PQC integration."

**Key Points to Highlight:**
- New components: PQC_KeyDerivation.c (122 lines), SoAd_PQC.c (367 lines)
- Complete PQC module: ML-KEM + ML-DSA + HKDF
- Test coverage: 5 comprehensive tests (700+ lines test code)
- Full integration: COM → PduR → SecOC → Csm → PQC → SoAd → Ethernet

```mermaid
graph TB
    subgraph "PHASE 3 COMPLETE: ETHERNET GATEWAY WITH ML-KEM + ML-DSA"
        subgraph "New Components Implemented"
            PQC_KD[PQC_KeyDerivation.c<br/>HKDF Session Key Derivation<br/>122 lines]
            SOAD_PQC[SoAd_PQC.c<br/>ML-KEM Key Exchange<br/>367 lines]
            PHASE3_TEST[test_phase3_complete.c<br/>Comprehensive Integration Test<br/>700+ lines]
        end

        subgraph "AUTOSAR Stack with PQC"
            COM[COM Manager]
            PDUR[PduR Router]
            SECOC[SecOC Engine]
            CSM[Crypto Service Manager]

            subgraph "PQC Module Complete"
                PQC_MLKEM[ML-KEM-768<br/>Key Exchange<br/>PQC_KeyExchange.c]
                PQC_MLDSA[ML-DSA-65<br/>Signatures<br/>PQC.c]
                PQC_HKDF[HKDF<br/>Key Derivation<br/>PQC_KeyDerivation.c]
            end

            SOAD[Socket Adapter<br/>SoAd]
            ETH[Ethernet<br/>TCP/IP]
        end

        subgraph "Test Coverage"
            TEST1[Test 1:<br/>ML-KEM KeyGen/Encaps/Decaps<br/>1000 iterations each]
            TEST2[Test 2:<br/>HKDF Extract/Expand<br/>32B encryption + 32B auth keys]
            TEST3[Test 3:<br/>ML-DSA Sign/Verify<br/>100 iterations]
            TEST4[Test 4:<br/>Combined Performance<br/>Handshake + Per-Message]
            TEST5[Test 5:<br/>Security Validation<br/>Replay, Tampering, Quantum]
        end
    end

    COM --> PDUR
    PDUR --> SECOC
    SECOC --> CSM
    CSM --> PQC_MLDSA
    CSM --> PQC_MLKEM
    PQC_MLKEM --> PQC_HKDF
    PQC_HKDF --> SOAD_PQC
    SECOC --> SOAD
    SOAD --> SOAD_PQC
    SOAD --> ETH

    PHASE3_TEST --> TEST1
    PHASE3_TEST --> TEST2
    PHASE3_TEST --> TEST3
    PHASE3_TEST --> TEST4
    PHASE3_TEST --> TEST5

    style PQC_KD fill:#ff9800,color:#fff
    style SOAD_PQC fill:#ff9800,color:#fff
    style PHASE3_TEST fill:#2196f3,color:#fff
    style PQC_HKDF fill:#f57c00,color:#fff
```

**Talking Points:**
- Phase 3 added 489 lines of production code (PQC_KeyDerivation + SoAd_PQC)
- 700+ lines comprehensive test suite validates all functionality
- ML-KEM + ML-DSA + HKDF = complete PQC solution
- AUTOSAR-compliant integration (no modifications to SecOC core)

---

## 5.2 Complete AUTOSAR Signal Flow

**Thesis Context:** "This shows every layer from application to Ethernet, with all PQC components integrated into the AUTOSAR stack."

**Key Points to Highlight:**
- 9 AUTOSAR layers: APP → COM → PduR → SecOC → Csm → PQC → SoAd → Ethernet
- Bidirectional flow: TX (down) and RX (up)
- PQC module contains 3 sub-modules: ML-DSA, ML-KEM, HKDF
- SoAd_PQC integrates ML-KEM into Socket Adapter layer
- Complete separation of concerns (each layer has specific responsibility)

```mermaid
graph TB
    subgraph "APPLICATION LAYER"
        APP[Vehicle Application<br/>8-byte CAN Message]
    end

    subgraph "COM LAYER"
        COM[COM Manager<br/>Message Handling]
    end

    subgraph "PDUR LAYER"
        PDUR[PduR Router<br/>Central Routing Hub]
    end

    subgraph "SECOC LAYER"
        SECOC_TX[SecOC TX Processing]
        FVM[Freshness Value Manager<br/>64-bit Counter]
        SECOC_RX[SecOC RX Processing]
    end

    subgraph "CSM LAYER Crypto Service Manager"
        CSM_SIGN[Csm_SignatureGenerate]
        CSM_VERIFY[Csm_SignatureVerify]
    end

    subgraph "PQC MODULE Complete Integration"
        direction TB

        subgraph "ML-DSA-65 Signatures"
            MLDSA_SIGN[PQC_MLDSA_Sign<br/>Generate 3309-byte signature<br/>Time: 8.13ms]
            MLDSA_VERIFY[PQC_MLDSA_Verify<br/>Verify signature<br/>Time: 4.89ms]
        end

        subgraph "ML-KEM-768 Key Exchange"
            MLKEM_INIT[PQC_KeyExchange_Initiate<br/>Generate keypair<br/>Send 1184B public key]
            MLKEM_RESP[PQC_KeyExchange_Respond<br/>Encapsulate<br/>Send 1088B ciphertext]
            MLKEM_COMP[PQC_KeyExchange_Complete<br/>Decapsulate<br/>Recover 32B shared secret]
        end

        subgraph "HKDF Key Derivation"
            HKDF_EXT[HKDF-Extract<br/>Derive PRK from shared secret]
            HKDF_EXP[HKDF-Expand<br/>32B encryption + 32B auth keys]
        end
    end

    subgraph "SOAD LAYER Socket Adapter"
        SOAD_TX[SoAdTp_Transmit<br/>TP Mode for Large PDUs]
        SOAD_PQC_MOD[SoAd_PQC Module<br/>ML-KEM Integration]
        SOAD_RX[SoAdTp_RxIndication]
    end

    subgraph "ETHERNET LAYER"
        ETH_SEND[ethernet_send<br/>TCP Port 12345<br/>Max 4096 bytes]
        ETH_RECV[ethernet_receive<br/>TCP Port 12345]
    end

    subgraph "BACKEND SYSTEM"
        BACKEND[Central ECU<br/>Quantum-Resistant<br/>Verification]
    end

    APP --> COM
    COM --> PDUR
    PDUR --> SECOC_TX
    SECOC_TX --> FVM
    FVM --> SECOC_TX
    SECOC_TX --> CSM_SIGN
    CSM_SIGN --> MLDSA_SIGN
    MLDSA_SIGN --> CSM_SIGN
    CSM_SIGN --> SECOC_TX
    SECOC_TX --> PDUR
    PDUR --> SOAD_TX

    SOAD_TX --> SOAD_PQC_MOD
    SOAD_PQC_MOD --> MLKEM_INIT
    MLKEM_INIT --> MLKEM_COMP
    MLKEM_COMP --> HKDF_EXT
    HKDF_EXT --> HKDF_EXP
    HKDF_EXP --> SOAD_PQC_MOD

    SOAD_TX --> ETH_SEND
    ETH_SEND --> BACKEND
    BACKEND --> ETH_RECV
    ETH_RECV --> SOAD_RX
    SOAD_RX --> PDUR
    PDUR --> SECOC_RX
    SECOC_RX --> CSM_VERIFY
    CSM_VERIFY --> MLDSA_VERIFY
    MLDSA_VERIFY --> CSM_VERIFY
    CSM_VERIFY --> SECOC_RX
    SECOC_RX --> PDUR
    PDUR --> COM
    COM --> APP

    style MLDSA_SIGN fill:#f57c00,color:#fff
    style MLDSA_VERIFY fill:#f57c00,color:#fff
    style MLKEM_INIT fill:#ff9800,color:#fff
    style MLKEM_COMP fill:#ff9800,color:#fff
    style HKDF_EXT fill:#4caf50,color:#fff
    style HKDF_EXP fill:#4caf50,color:#fff
    style SOAD_PQC_MOD fill:#2196f3,color:#fff
```

**Talking Points:**
- Layered architecture enables separation of concerns
- SecOC layer handles authentication logic (algorithm-agnostic)
- Csm layer provides crypto abstraction (can switch algorithms)
- PQC module encapsulates liboqs complexity
- SoAd layer manages Ethernet communication + ML-KEM key exchange

---

## 5.3 PQC Module Internal Architecture

**Thesis Context:** "The PQC module is internally divided into three sub-modules, each with specific responsibility."

**Key Points to Highlight:**
- PQC.c: ML-DSA signatures (sign/verify)
- PQC_KeyExchange.c: ML-KEM key exchange (initiate/respond/complete)
- PQC_KeyDerivation.c: HKDF key derivation (extract/expand)
- liboqs integration: All modules call OQS APIs
- Session management: 8 peer slots for concurrent sessions

```mermaid
graph TB
    subgraph "EXTERNAL INTERFACE"
        CSM[Csm Layer]
        SOAD[SoAd Layer]
        ETH[Ethernet ethernet_send/receive]
    end

    subgraph "PQC MODULE ARCHITECTURE"
        subgraph "PQC.c ML-DSA Module"
            MLDSA_INIT[PQC_Init<br/>Initialize liboqs]
            MLDSA_KEYGEN[PQC_MLDSA_KeyGen<br/>1952B public + 4032B secret]
            MLDSA_SIGN[PQC_MLDSA_Sign<br/>3309B signature<br/>8.13ms]
            MLDSA_VERIFY[PQC_MLDSA_Verify<br/>Signature check<br/>4.89ms]
        end

        subgraph "PQC_KeyExchange.c ML-KEM Module"
            KE_INIT_FUNC[PQC_KeyExchange_Init<br/>Initialize 8 peer slots]
            KE_INITIATE[PQC_KeyExchange_Initiate<br/>KeyGen 2.85ms<br/>Store keypair]
            KE_RESPOND[PQC_KeyExchange_Respond<br/>Encaps 3.12ms<br/>Create ciphertext]
            KE_COMPLETE[PQC_KeyExchange_Complete<br/>Decaps 3.89ms<br/>Recover shared secret]
            KE_GET_SS[PQC_KeyExchange_GetSharedSecret<br/>Retrieve 32B shared secret]
        end

        subgraph "PQC_KeyDerivation.c HKDF Module"
            KD_INIT[PQC_KeyDerivation_Init<br/>Initialize session storage]
            KD_DERIVE[PQC_DeriveSessionKeys<br/>HKDF Extract + Expand<br/>0.3ms]
            KD_GET[PQC_GetSessionKeys<br/>Retrieve stored keys]
            KD_CLEAR[PQC_ClearSessionKeys<br/>Clear sensitive data]
        end

        subgraph "liboqs Integration"
            LIBOQS[Open Quantum Safe Library<br/>NIST FIPS 203 + 204]
            OQS_KEM[OQS_KEM_ml_kem_768<br/>KeyGen, Encaps, Decaps]
            OQS_SIG[OQS_SIG_ml_dsa_65<br/>KeyGen, Sign, Verify]
            OQS_SHA[OQS_SHA2_sha256<br/>For HKDF]
        end
    end

    subgraph "SESSION MANAGEMENT"
        SESSION_KEYS[PQC_SessionKeys 8<br/>32B encryption + 32B auth per peer]
        KE_SESSIONS[KE_Sessions 8<br/>Keypairs + Shared Secrets]
    end

    CSM --> MLDSA_SIGN
    CSM --> MLDSA_VERIFY
    SOAD --> KE_INITIATE
    SOAD --> KE_RESPOND
    SOAD --> KE_COMPLETE
    ETH --> SOAD

    MLDSA_INIT --> LIBOQS
    MLDSA_KEYGEN --> OQS_SIG
    MLDSA_SIGN --> OQS_SIG
    MLDSA_VERIFY --> OQS_SIG

    KE_INIT_FUNC --> LIBOQS
    KE_INITIATE --> OQS_KEM
    KE_RESPOND --> OQS_KEM
    KE_COMPLETE --> OQS_KEM
    KE_GET_SS --> KE_SESSIONS

    KE_COMPLETE --> KD_DERIVE
    KD_INIT --> OQS_SHA
    KD_DERIVE --> OQS_SHA
    KD_DERIVE --> SESSION_KEYS
    KD_GET --> SESSION_KEYS

    style MLDSA_SIGN fill:#f57c00,color:#fff
    style KE_COMPLETE fill:#ff9800,color:#fff
    style KD_DERIVE fill:#4caf50,color:#fff
    style LIBOQS fill:#2196f3,color:#fff
    style SESSION_KEYS fill:#9c27b0,color:#fff
```

**Talking Points:**
- Clear separation: Signatures (PQC.c), Key Exchange (PQC_KeyExchange.c), Key Derivation (PQC_KeyDerivation.c)
- liboqs provides all cryptographic primitives
- Session management supports up to 8 concurrent peers
- Stateful key exchange: Stores keypairs and shared secrets per peer

---

## 5.4 Gateway TX Flow (Detailed Implementation)

**Thesis Context:** "This flowchart shows every step the gateway takes when transmitting a message from CAN to Ethernet."

**Key Points to Highlight:**
- Input: CAN message with classical MAC
- Step 1: Verify classical MAC (ensures CAN integrity)
- Step 2: Extract authentic data
- Step 3: Increment freshness counter (prevents replay)
- Step 4: PQC signature generation (~8.1ms)
- Output: Ethernet PDU with PQC signature

```mermaid
flowchart TD
    START([CAN Message Arrival]) --> RX_CAN[Receive via MCP2515]
    RX_CAN --> VERIFY_MAC{Verify<br/>Classical MAC}

    VERIFY_MAC -->|Invalid| DROP1[Drop Message]
    DROP1 --> END1([END])

    VERIFY_MAC -->|Valid| EXTRACT[Extract Authentic PDU]
    EXTRACT --> INC_FV[Increment Freshness Counter]
    INC_FV --> BUILD_DTA[Build Data-to-Authenticator<br/>MessageID + Data + Freshness]

    BUILD_DTA --> PQC_SIGN[PQC Signature Generation<br/>ML-DSA-65 Sign]
    Note right of PQC_SIGN: Time: ~8.1ms<br/>Signature: 3,309 bytes

    PQC_SIGN --> BUILD_PDU[Build Secured PDU<br/>Data + Freshness + Signature]
    BUILD_PDU --> ROUTE[Route to Ethernet via PduR]
    ROUTE --> ETH_TX[Ethernet Transmission<br/>TCP Port 12345]
    ETH_TX --> SUCCESS([Message Sent])

    style VERIFY_MAC fill:#ffc107
    style PQC_SIGN fill:#ff9800
    style SUCCESS fill:#4caf50,color:#fff
    style DROP1 fill:#f44336,color:#fff
```

**Talking Points:**
- Classical MAC verification first (fast check, filters invalid CAN messages)
- Freshness counter incremented before signing (ensures uniqueness)
- Data-to-Authenticator: MessageID || Data || Freshness (all signed)
- Signature generation: 8.1ms (dominant latency)
- Total TX latency: ~8.5ms (MAC verify + sign + Ethernet send)

---

## 5.5 Gateway RX Flow (Security Checks)

**Thesis Context:** "The reception path has two security checks: freshness validation (replay detection) and signature verification (tampering detection)."

**Key Points to Highlight:**
- Input: Ethernet PDU with PQC signature
- Step 1: Parse PDU (extract data, freshness, signature)
- Step 2: Freshness check (must be > last value)
- Step 3: PQC signature verification (~4.9ms)
- Output: Authentic data forwarded to application (or dropped if invalid)

```mermaid
flowchart TD
    START([Ethernet Message Arrival]) --> RX_ETH[Receive via TCP Socket]
    RX_ETH --> PARSE[Parse Secured PDU<br/>Extract Data, Freshness, Signature]

    PARSE --> CHECK_FV{Freshness Value<br/>> Last Value?}

    CHECK_FV -->|NO| REPLAY[Replay Attack<br/>Detected!]
    REPLAY --> LOG1[Log Security Event]
    LOG1 --> DROP1[Drop PDU]
    DROP1 --> END1([BLOCKED])

    CHECK_FV -->|YES| REBUILD[Rebuild Data-to-Authenticator<br/>MessageID + Data + Freshness]

    REBUILD --> PQC_VERIFY[PQC Signature Verification<br/>ML-DSA-65 Verify]
    Note right of PQC_VERIFY: Time: ~4.9ms

    PQC_VERIFY --> CHECK_SIG{Signature<br/>Valid?}

    CHECK_SIG -->|NO| TAMPER[Tampering<br/>Detected!]
    TAMPER --> LOG2[Log Security Event]
    LOG2 --> DROP2[Drop PDU]
    DROP2 --> END2([BLOCKED])

    CHECK_SIG -->|YES| UPDATE_FV[Update Freshness Counter]
    UPDATE_FV --> EXTRACT[Extract Authentic PDU]
    EXTRACT --> GEN_MAC[Generate Classical MAC<br/>for CAN]
    GEN_MAC --> CAN_TX[Transmit to CAN Bus]
    CAN_TX --> SUCCESS([Message Delivered])

    style CHECK_FV fill:#ffc107
    style PQC_VERIFY fill:#ff9800
    style SUCCESS fill:#4caf50,color:#fff
    style REPLAY fill:#f44336,color:#fff
    style TAMPER fill:#f44336,color:#fff
    style END1 fill:#f44336,color:#fff
    style END2 fill:#f44336,color:#fff
```

**Talking Points:**
- Two-layer security: Freshness first (cheap), signature second (expensive)
- Replay detection: Freshness ≤ last value → drop (attacker replaying old message)
- Tampering detection: Invalid signature → drop (attacker modified data)
- Security events logged for forensic analysis
- Total RX latency: ~5.2ms (parse + verify + CAN transmit)

---

# Chapter 6: Security Properties - Attack Prevention

## 6.1 Security Attack Detection State Machine

**Thesis Context:** "The gateway operates as a state machine with three possible outcomes: authenticated (message delivered), replay detected (old message), or tampering detected (modified message)."

**Key Points to Highlight:**
- State machine ensures all messages undergo security checks
- Three attack detection mechanisms: Freshness, Signature, Logging
- No message bypasses security (fail-safe design)
- Security events logged for audit trail

```mermaid
stateDiagram-v2
    [*] --> IDLE

    IDLE --> RX_PDU: Ethernet Message<br/>Arrival

    RX_PDU --> PARSE_PDU: Extract Components

    PARSE_PDU --> CHECK_FRESHNESS: Freshness<br/>Validation

    CHECK_FRESHNESS --> REPLAY_DETECTED: Freshness <= Last
    CHECK_FRESHNESS --> REBUILD_DTA: Freshness > Last

    REPLAY_DETECTED --> LOG_REPLAY: Log Security Event<br/>(Replay Attack)
    LOG_REPLAY --> DROP_PDU_1: Drop PDU
    DROP_PDU_1 --> IDLE

    REBUILD_DTA --> VERIFY_SIG: PQC Signature<br/>Verification

    VERIFY_SIG --> TAMPER_DETECTED: Signature Invalid
    VERIFY_SIG --> AUTHENTICATED: Signature Valid

    TAMPER_DETECTED --> LOG_TAMPER: Log Security Event<br/>(Tampering)
    LOG_TAMPER --> DROP_PDU_2: Drop PDU
    DROP_PDU_2 --> IDLE

    AUTHENTICATED --> UPDATE_FV: Update Freshness<br/>Counter
    UPDATE_FV --> FORWARD_PDU: Extract Authentic<br/>PDU
    FORWARD_PDU --> DELIVER: Deliver to<br/>Application
    DELIVER --> IDLE

    note right of CHECK_FRESHNESS
        Anti-Replay
        Protection
    end note

    note right of VERIFY_SIG
        Quantum-Resistant
        ML-DSA-65 Verification
    end note

    note right of AUTHENTICATED
        Security Checks
        Passed
    end note
```

**Talking Points:**
- IDLE: Gateway waiting for message
- PARSE_PDU: Extract data, freshness, signature
- CHECK_FRESHNESS: Anti-replay mechanism (freshness must increase)
- VERIFY_SIG: Quantum-resistant signature verification
- Three outcomes: AUTHENTICATED, REPLAY_DETECTED, TAMPER_DETECTED
- Return to IDLE after processing (ready for next message)

---

## 6.2 Freshness Value Management (Anti-Replay)

**Thesis Context:** "Freshness values are monotonically increasing counters that prevent replay attacks - an attacker cannot reuse old messages."

**Key Points to Highlight:**
- TX freshness: Counter incremented on each transmission
- RX freshness: Stored last-received value, compared against incoming
- Replay attack: Attacker captures PDU (FV=1), later replays it when FV=2
- Detection: Incoming FV=1 < stored FV=2 → Drop
- 100% replay detection rate in tests

```mermaid
sequenceDiagram
    participant GW as Gateway
    participant TX as Transmitter
    participant RX as Receiver
    participant FVM as Freshness Value<br/>Manager

    Note over GW,FVM: Normal Communication Flow

    TX->>FVM: Request TX Freshness
    FVM->>FVM: Increment Counter<br/>Counter = 1
    FVM-->>TX: Return Freshness = 1

    TX->>TX: Build Data-to-Authenticator<br/>[MessageID + Data + FV=1]
    TX->>TX: Generate PQC Signature
    TX->>RX: Send Secured PDU (FV=1)

    RX->>RX: Extract Freshness = 1
    RX->>RX: Check: 1 > 0 (last FV)?
    Note over RX: YES - Accept
    RX->>RX: Verify PQC Signature
    RX->>FVM: Update RX Freshness = 1

    Note over GW,FVM: Replay Attack Scenario

    TX->>FVM: Request TX Freshness
    FVM->>FVM: Increment Counter<br/>Counter = 2
    FVM-->>TX: Return Freshness = 2

    TX->>TX: Build Data-to-Authenticator<br/>[MessageID + Data + FV=2]
    TX->>TX: Generate PQC Signature

    Note over TX: Attacker captures PDU

    TX->>RX: Send Secured PDU (FV=2)
    RX->>RX: Check: 2 > 1?
    Note over RX: YES - Accept
    RX->>FVM: Update RX Freshness = 2

    Note over TX,RX: Attacker replays old PDU

    TX->>RX: [ATTACK] Replay PDU (FV=1)
    RX->>RX: Extract Freshness = 1
    RX->>RX: Check: 1 > 2 (last FV)?
    Note over RX: NO - REJECT!
    RX->>RX: Drop PDU
    Note over RX: Replay Attack Detected!

    style RX fill:#f44336,color:#fff
```

**Talking Points:**
- Freshness Value Manager maintains TX and RX counters
- TX counter: Increments on each transmission (monotonically increasing)
- RX counter: Stores last-received freshness value
- Normal flow: FV increases (1 → 2), messages accepted
- Replay attack: Old FV (1) < current FV (2), message dropped
- Signature alone is insufficient (attacker can replay valid signed messages)

---

# Chapter 7: Engineering Challenges - Critical Problems Solved

## 7.1 Message Size Evolution Through System

**Thesis Context:** "This shows how a tiny 8-byte vehicle message grows to 3,321 bytes by the time it reaches Ethernet - necessary for quantum resistance."

**Key Points to Highlight:**
- Stage 1: 8 bytes raw vehicle data
- Stage 2: 24 bytes CAN (8 data + 16 MAC)
- Stage 3: Gateway transforms to 3,321 bytes (8 data + 4 freshness + 3,309 signature)
- Stage 4: Ethernet bandwidth (100 Mbps) easily handles 3,321-byte PDUs
- Size explosion: 414x increase from raw data to Ethernet PDU

```mermaid
graph LR
    subgraph "Stage 1: Vehicle ECU"
        VEH_DATA[Vehicle Data<br/>8 bytes]
    end

    subgraph "Stage 2: CAN Bus"
        CAN_DATA[8 bytes]
        CAN_MAC[+MAC 16B]
        CAN_TOTAL[Total: 24 bytes]

        CAN_DATA --> CAN_MAC
        CAN_MAC --> CAN_TOTAL
    end

    subgraph "Stage 3: Gateway Processing"
        GW_EXTRACT[Extract 8 bytes]
        GW_FV[+Freshness 4B]
        GW_SIGN[+PQC Sig 3309B]
        GW_TOTAL[Total: 3,321 bytes]

        GW_EXTRACT --> GW_FV
        GW_FV --> GW_SIGN
        GW_SIGN --> GW_TOTAL
    end

    subgraph "Stage 4: Ethernet"
        ETH_PDU[Secured PDU<br/>3,321 bytes]
        ETH_BW[Bandwidth: 100 Mbps]

        ETH_PDU --> ETH_BW
    end

    VEH_DATA -->|CAN Tx| CAN_DATA
    CAN_TOTAL -->|Gateway Rx| GW_EXTRACT
    GW_TOTAL -->|Gateway Tx| ETH_PDU

    style CAN_TOTAL fill:#8bc34a
    style GW_TOTAL fill:#ff9800
    style ETH_BW fill:#4caf50
```

**Talking Points:**
- 8 bytes → 24 bytes (CAN): 3x increase (classical MAC overhead)
- 24 bytes → 3,321 bytes (Ethernet): 138x increase (PQC signature overhead)
- Total increase: 414x (raw data to Ethernet PDU)
- Bandwidth utilization: 3,321 bytes @ 100 msg/sec = 2.7 Mbps (2.7% of 100 Mbps)
- Trade-off justified: Quantum resistance worth 414x size increase

---

## 7.2 Buffer Overflow Fix (Critical Vulnerability)

**Thesis Context:** "During development, we discovered a critical buffer overflow vulnerability when PQC signatures (3,309 bytes) were copied into 10-byte buffers. This shows the before and after fix."

**Key Points to Highlight:**
- BEFORE: BUS_LENGTH_RECEIVE = 8 (10-byte buffer total)
- Problem 1: memcpy 3,309 bytes → 10-byte buffer = buffer overflow (memory corruption)
- Problem 2: send only 10 bytes → 99.7% signature truncation (security failure)
- AFTER: BUS_LENGTH_RECEIVE = 4096 (4,098-byte buffer total)
- Result: Safe buffer, complete signature transmission

```mermaid
graph TB
    subgraph "BEFORE - Vulnerable Implementation"
        OLD_DEF["BUS_LENGTH_RECEIVE = 8"]
        OLD_BUF[Buffer Size: 10 bytes<br/>8 data + 2 ID]
        OLD_COPY[memcpy 3,309 bytes into 10-byte buffer]
        OLD_SEND[send only 10 bytes]

        OLD_DEF --> OLD_BUF
        OLD_BUF --> OLD_COPY
        OLD_COPY --> OLD_SEND

        OLD_ISSUE1[Buffer Overflow<br/>Memory Corruption]
        OLD_ISSUE2[Data Truncation<br/>99.7% of signature lost]

        OLD_COPY -.->|DANGER| OLD_ISSUE1
        OLD_SEND -.->|DANGER| OLD_ISSUE2
    end

    subgraph "AFTER - Fixed Implementation"
        NEW_DEF["BUS_LENGTH_RECEIVE = 4096"]
        NEW_BUF[Buffer Size: 4098 bytes<br/>4096 data + 2 ID]
        NEW_COPY[memcpy up to 4096 bytes safely]
        NEW_SEND[send actual length<br/>dataLen + sizeof ID]

        NEW_DEF --> NEW_BUF
        NEW_BUF --> NEW_COPY
        NEW_COPY --> NEW_SEND

        NEW_SAFE1[Safe Buffer<br/>Fits PQC Signature]
        NEW_SAFE2[Complete Transmission<br/>Full signature sent]

        NEW_BUF -.->|SAFE| NEW_SAFE1
        NEW_SEND -.->|SAFE| NEW_SAFE2
    end

    style OLD_ISSUE1 fill:#f44336,color:#fff
    style OLD_ISSUE2 fill:#f44336,color:#fff
    style NEW_SAFE1 fill:#4caf50,color:#fff
    style NEW_SAFE2 fill:#4caf50,color:#fff
```

**Talking Points:**
- Original code designed for 8-byte CAN messages
- PQC signatures (3,309 bytes) exceeded buffer capacity by 330x
- Buffer overflow consequences: Memory corruption, potential RCE vulnerability
- Truncation consequences: Only 10/3,309 bytes sent, signature verification impossible
- Fix: Increased buffer to 4,096 bytes (accommodates any AUTOSAR PDU)
- Lesson: Legacy code assumptions (small messages) break with PQC

---

# Chapter 8: Performance Analysis - Trade-offs

## 8.1 CAN vs Ethernet Performance Comparison

**Thesis Context:** "This summarizes the performance trade-offs: CAN with classical MAC is fast but quantum-vulnerable, Ethernet with PQC is slower but quantum-resistant."

**Key Points to Highlight:**
- CAN: 20,000 msg/sec (0.05ms MAC), suitable for real-time control
- Ethernet: 123 msg/sec (13ms sign+verify), suitable for telemetry/diagnostics
- Overhead: 162x slower, 206x larger
- Benefit: Quantum-resistant security for 20+ years
- Verdict: PQC suitable for Ethernet gateway (non-real-time backend communication)

```mermaid
graph TB
    subgraph "CAN Bus Performance (Classical MAC)"
        CAN_TITLE[CAN Bus<br/>500 kbps]
        CAN_MAC_TIME[MAC Time: 0.05ms]
        CAN_MAC_SIZE[MAC Size: 16 bytes]
        CAN_TOTAL[Total PDU: 24 bytes]
        CAN_THROUGHPUT[Throughput: 20,000 msg/sec]
        CAN_SUITABLE[Suitable for:<br/>Real-time control<br/>High-frequency messaging]

        CAN_TITLE --> CAN_MAC_TIME
        CAN_MAC_TIME --> CAN_MAC_SIZE
        CAN_MAC_SIZE --> CAN_TOTAL
        CAN_TOTAL --> CAN_THROUGHPUT
        CAN_THROUGHPUT --> CAN_SUITABLE
    end

    subgraph "Ethernet Performance (PQC)"
        ETH_TITLE[Ethernet<br/>100 Mbps]
        ETH_SIG_TIME[Sign Time: 8.13ms<br/>Verify Time: 4.89ms]
        ETH_SIG_SIZE[Signature Size: 3,309 bytes]
        ETH_TOTAL[Total PDU: 3,320 bytes]
        ETH_THROUGHPUT[Throughput: 123 msg/sec]
        ETH_SUITABLE[Suitable for:<br/>Telemetry data<br/>Firmware updates<br/>Diagnostics]

        ETH_TITLE --> ETH_SIG_TIME
        ETH_SIG_TIME --> ETH_SIG_SIZE
        ETH_SIG_SIZE --> ETH_TOTAL
        ETH_TOTAL --> ETH_THROUGHPUT
        ETH_THROUGHPUT --> ETH_SUITABLE
    end

    subgraph "Trade-off Analysis"
        OVERHEAD[Overhead:<br/>162x slower<br/>206x larger]
        BENEFIT[Benefit:<br/>Quantum-Resistant<br/>20+ year security]
        VERDICT[Gateway Verdict:<br/>PQC SUITABLE<br/>for Ethernet]

        OVERHEAD --> BENEFIT
        BENEFIT --> VERDICT
    end

    CAN_THROUGHPUT -.->|vs| ETH_THROUGHPUT
    ETH_THROUGHPUT --> OVERHEAD

    style CAN_SUITABLE fill:#4caf50
    style ETH_SUITABLE fill:#ff9800
    style VERDICT fill:#2196f3,color:#fff
    style BENEFIT fill:#4caf50
```

**Talking Points:**
- CAN: Classical MAC fast (0.05ms), 20,000 msg/sec throughput
- Ethernet: PQC slow (8.13ms sign + 4.89ms verify = 13.02ms), 123 msg/sec throughput
- Slowdown: 13.02 / 0.05 = 260x (signing), 162x (combined sign+verify)
- Size: 3,309 / 16 = 206x larger signature
- Use case differentiation: CAN for real-time control, Ethernet for backend telemetry
- Verdict: PQC acceptable for Ethernet gateway (non-critical latency requirements)

---

# Chapter 9: Testing and Validation - Proof of Correctness

## 9.1 Test Suite Architecture (Two-Tier)

**Thesis Context:** "We have two comprehensive test suites: standalone PQC testing (no AUTOSAR) and integrated SecOC testing (full stack)."

**Key Points to Highlight:**
- Test 1: Standalone PQC (ML-KEM + ML-DSA, 1000 iterations each)
- Test 2: AUTOSAR SecOC Integration (Csm layer, gateway flows, attack detection)
- Coverage: Functional correctness + Performance metrics + Security validation
- CSV output: Detailed metrics (time min/max/avg/stddev, throughput, success rate)

```mermaid
graph TB
    subgraph "Comprehensive PQC Test Suite"
        subgraph "Test 1: test_pqc_standalone.c"
            STANDALONE_TITLE[Standalone PQC Testing<br/>NO AUTOSAR Integration]

            ML_KEM_TESTS[ML-KEM-768 Tests:<br/>• KeyGen 1000 iter<br/>• Encaps 1000 iter<br/>• Decaps 1000 iter<br/>• Rejection sampling<br/>• Buffer overflow detection]

            ML_DSA_TESTS[ML-DSA-65 Tests:<br/>• KeyGen 1000 iter<br/>• Sign 5 sizes × 1000 iter<br/>• Verify 5 sizes × 1000 iter<br/>• Bitflip EUF-CMA 50 bitflips<br/>• Bitflip SUF-CMA 50 bitflips<br/>• Context string testing]

            CSV1[Output:<br/>pqc_standalone_results.csv]

            STANDALONE_TITLE --> ML_KEM_TESTS
            STANDALONE_TITLE --> ML_DSA_TESTS
            ML_KEM_TESTS --> CSV1
            ML_DSA_TESTS --> CSV1
        end

        subgraph "Test 2: test_pqc_secoc_integration.c"
            INTEGRATION_TITLE[AUTOSAR SecOC Integration<br/>Ethernet Gateway Context]

            CSM_TESTS[Csm Layer Tests:<br/>• Csm_SignatureGenerate PQC<br/>• Csm_SignatureVerify PQC<br/>• Csm_MacGenerate Classical<br/>• Csm_MacVerify Classical<br/>• Performance comparison]

            GATEWAY_TESTS[Ethernet Gateway Tests:<br/>• TX flow MessageID+Data+FV<br/>• RX flow with freshness check<br/>• Replay attack detection<br/>• Message tampering detection<br/>• End-to-end SecOC flow]

            CSV2[Output:<br/>pqc_secoc_integration_results.csv]

            INTEGRATION_TITLE --> CSM_TESTS
            INTEGRATION_TITLE --> GATEWAY_TESTS
            CSM_TESTS --> CSV2
            GATEWAY_TESTS --> CSV2
        end
    end

    subgraph "Test Results"
        METRICS[Performance Metrics:<br/>• Time min/max/avg/stddev<br/>• Throughput ops/sec<br/>• Size bytes<br/>• Success rate %]

        SECURITY[Security Validation:<br/>• 100% replay detection<br/>• 100% tampering detection<br/>• 100% bitflip detection]

        CSV1 --> METRICS
        CSV2 --> METRICS
        CSV1 --> SECURITY
        CSV2 --> SECURITY
    end

    style STANDALONE_TITLE fill:#4caf50,color:#fff
    style INTEGRATION_TITLE fill:#ff9800,color:#fff
    style SECURITY fill:#2196f3,color:#fff
```

**Talking Points:**
- Two-tier approach: Standalone (PQC primitives) + Integration (AUTOSAR stack)
- Standalone tests verify liboqs correctness (1000+ iterations for statistical confidence)
- Integration tests verify SecOC flow (TX, RX, attack detection)
- Security validation: 100% detection rate for replay, tampering, bitflip attacks
- Performance metrics: Min/max/avg/stddev times for all operations
- CSV output enables automated regression testing

---

## 9.2 Phase 3 Complete Test Coverage

**Thesis Context:** "Phase 3 test suite covers all components end-to-end: ML-KEM key exchange, HKDF key derivation, ML-DSA signatures, combined performance, and security validation."

**Key Points to Highlight:**
- Test 1: ML-KEM-768 (KeyGen, Encaps, Decaps, Multi-peer)
- Test 2: HKDF (Extract, Expand, Key Independence, Deterministic)
- Test 3: ML-DSA-65 via Csm layer (Sign, Verify, Invalid signatures)
- Test 4: Combined Performance (Handshake overhead, Per-message overhead, Amortized cost)
- Test 5: Security (Replay, Tampering, Signature corruption, Quantum resistance)

```mermaid
graph TB
    subgraph "TEST SUITE: test_phase3_complete_ethernet_gateway.c"
        TEST_MAIN[Phase 3 Complete Test Suite<br/>700+ lines comprehensive testing]

        subgraph "TEST 1: ML-KEM-768 Standalone"
            T1_1[1.1: KeyGen 1000 iterations<br/>Verify 1184B public + 2400B secret]
            T1_2[1.2: Encapsulation 1000 iterations<br/>Verify 1088B ciphertext + 32B secret]
            T1_3[1.3: Decapsulation 1000 iterations<br/>Verify shared secret recovery]
            T1_4[1.4: Multi-peer 8 concurrent sessions<br/>Session isolation test]
        end

        subgraph "TEST 2: HKDF Key Derivation"
            T2_1[2.1: HKDF-Extract<br/>PRK from shared secret]
            T2_2[2.2: HKDF-Expand Encryption<br/>32B encryption key]
            T2_3[2.3: HKDF-Expand Authentication<br/>32B authentication key]
            T2_4[2.4: Key Independence<br/>Verify EncKey != AuthKey != SS]
            T2_5[2.5: Deterministic Derivation<br/>Same SS -> Same keys]
        end

        subgraph "TEST 3: ML-DSA-65 Integrated"
            T3_1[3.1: Csm_SignatureGenerate<br/>100 iterations via Csm layer]
            T3_2[3.2: Csm_SignatureVerify<br/>100 iterations]
            T3_3[3.3: Invalid Signature Detection<br/>100 corrupted signatures]
            T3_4[3.4: Performance Measurement<br/>Sign time + Verify time]
        end

        subgraph "TEST 4: Combined Performance"
            T4_1[4.1: Handshake Total Time<br/>KeyGen + Encaps + Decaps]
            T4_2[4.2: HKDF Overhead<br/>Extract + 2x Expand]
            T4_3[4.3: Per-Message Overhead<br/>Sign + Verify]
            T4_4[4.4: Amortized Cost Analysis<br/>Handshake / N messages]
        end

        subgraph "TEST 5: Security Validation"
            T5_1[5.1: Replay Attack Detection<br/>Stale freshness rejection]
            T5_2[5.2: Message Tampering Detection<br/>Modified data rejection]
            T5_3[5.3: Signature Tampering Detection<br/>Corrupted signature rejection]
            T5_4[5.4: Quantum Resistance Validation<br/>NIST FIPS compliance check]
        end
    end

    subgraph "EXPECTED RESULTS"
        PASS_CRITERIA[All Tests Must PASS:<br/>100% Success Rate]
        PERF_METRICS[Performance Metrics:<br/>< 10ms handshake<br/>< 15ms per-message<br/>> 77 msg/sec throughput]
        SEC_METRICS[Security Metrics:<br/>100% attack detection<br/>NIST Category 3 security]
    end

    TEST_MAIN --> T1_1
    TEST_MAIN --> T1_2
    TEST_MAIN --> T1_3
    TEST_MAIN --> T1_4
    TEST_MAIN --> T2_1
    TEST_MAIN --> T2_2
    TEST_MAIN --> T2_3
    TEST_MAIN --> T2_4
    TEST_MAIN --> T2_5
    TEST_MAIN --> T3_1
    TEST_MAIN --> T3_2
    TEST_MAIN --> T3_3
    TEST_MAIN --> T3_4
    TEST_MAIN --> T4_1
    TEST_MAIN --> T4_2
    TEST_MAIN --> T4_3
    TEST_MAIN --> T4_4
    TEST_MAIN --> T5_1
    TEST_MAIN --> T5_2
    TEST_MAIN --> T5_3
    TEST_MAIN --> T5_4

    T1_4 --> PASS_CRITERIA
    T2_5 --> PASS_CRITERIA
    T3_4 --> PERF_METRICS
    T4_4 --> PERF_METRICS
    T5_4 --> SEC_METRICS

    style TEST_MAIN fill:#2196f3,color:#fff
    style PASS_CRITERIA fill:#4caf50,color:#fff
    style PERF_METRICS fill:#ff9800,color:#fff
    style SEC_METRICS fill:#f44336,color:#fff
```

**Talking Points:**
- 700+ lines of test code (comprehensive coverage)
- Test 1: ML-KEM correctness (1000 iterations, multi-peer isolation)
- Test 2: HKDF properties (key independence, determinism)
- Test 3: ML-DSA integration (Csm layer, invalid signature detection)
- Test 4: Performance (handshake < 10ms, per-message < 15ms, throughput > 77 msg/sec)
- Test 5: Security (100% attack detection, NIST compliance)

---

# Summary: Diagram Organization for Thesis Defense

This document reorganizes all 21 diagrams into 9 chapters for effective thesis storytelling:

| Chapter | Focus | Diagrams | Purpose |
|---------|-------|----------|---------|
| 1 | Introduction | 1 diagram | Big picture overview |
| 2 | Communication Paths | 3 diagrams | Data flow understanding |
| 3 | Hardware Platform | 2 diagrams | Physical implementation |
| 4 | Core Cryptography | 3 diagrams | PQC building blocks |
| 5 | Software Architecture | 5 diagrams | AUTOSAR integration |
| 6 | Security Properties | 2 diagrams | Attack prevention |
| 7 | Engineering Challenges | 2 diagrams | Problems solved |
| 8 | Performance Analysis | 1 diagram | Trade-offs |
| 9 | Testing and Validation | 2 diagrams | Proof of correctness |

**Total: 21 diagrams organized in narrative flow**

---

## Suggested Presentation Flow

**Introduction (5 minutes):**
- Start with Chapter 1 diagram (system overview)
- Explain the problem: Quantum computers threaten classical cryptography
- Solution: Ethernet gateway with PQC

**Technical Deep Dive (15 minutes):**
- Chapters 2-5: Walk through data flows, hardware, cryptography, architecture
- Emphasize AUTOSAR compliance and PQC integration

**Critical Discussions (10 minutes):**
- Chapters 6-8: Security properties, engineering challenges, performance
- Address questions about overhead, buffer overflow fix, trade-offs

**Validation (5 minutes):**
- Chapter 9: Show comprehensive testing, 100% attack detection
- Emphasize NIST compliance, security metrics

**Conclusion (5 minutes):**
- Summarize achievements
- Future work: Raspberry Pi deployment, optimization
- Questions from committee

**Total Presentation Time: ~40 minutes**

---

## How to Export Diagrams for Presentation

### Method 1: Mermaid Live Editor (Online)
1. Visit https://mermaid.live/
2. Copy diagram code
3. Export as PNG/SVG/PDF
4. Insert into PowerPoint/LaTeX

### Method 2: Mermaid CLI (Offline)
```bash
npm install -g @mermaid-js/mermaid-cli
mmdc -i DIAGRAMS_THESIS.md -o diagrams_thesis.pdf
```

### Method 3: VS Code Extension
1. Install "Markdown Preview Mermaid Support"
2. View diagrams in preview
3. Right-click → Save image

---

**End of Thesis Diagrams Document**

*This document was created specifically for thesis defense storytelling. All diagrams are also available in `DIAGRAMS.md` in original order.*
