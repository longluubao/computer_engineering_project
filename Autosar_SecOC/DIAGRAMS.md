# AUTOSAR SecOC Ethereum Gateway with PQC - Architecture Diagrams
## Mermaid Diagrams for Technical Report

**Context:** All diagrams emphasize the **Ethernet Gateway** use case with Post-Quantum Cryptography.

---

## 1. Ethernet Gateway System Overview

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

---

## 2. Ethernet Gateway Data Flow (CAN → Ethernet)

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

---

## 3. Ethernet Gateway Data Flow (Ethernet → CAN)

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

---

## 4. Secured PDU Format Comparison (CAN vs Ethernet)

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

---

## 5. Gateway TX Flow (Detailed Steps)

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

---

## 6. Gateway RX Flow with Security Checks

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

---

## 7. Raspberry Pi 4 Gateway Hardware Architecture

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

---

## 8. PQC Integration in Gateway

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

---

## 9. Performance Comparison (Gateway Context)

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

---

## 10. Security Attack Detection Flow

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

---

## 11. Ethernet Gateway Message Size Evolution

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

---

## 12. Dual-Platform Architecture

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

---

## 13. Buffer Overflow Fix Visualization

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

---

## 14. Freshness Value Management (Anti-Replay)

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

---

## 15. Test Suite Architecture

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

---

## Diagram Summary

| # | Diagram Name | Purpose | Gateway Context |
|---|-------------|---------|----------------|
| 1 | Ethernet Gateway System Overview | High-level architecture | Core gateway topology |
| 2 | Data Flow (CAN → Ethernet) | TX path with PQC signing | CAN to backend transformation |
| 3 | Data Flow (Ethernet → CAN) | RX path with PQC verification | Backend to CAN transformation |
| 4 | Secured PDU Format Comparison | Message structure | Size comparison CAN vs Ethernet |
| 5 | Gateway TX Flow | Detailed transmission steps | PQC signature generation |
| 6 | Gateway RX Flow | Detailed reception with security | Replay & tampering detection |
| 7 | Raspberry Pi 4 Architecture | Hardware deployment | Physical gateway platform |
| 8 | PQC Integration | ML-KEM-768 & ML-DSA-65 | Cryptographic modules |
| 9 | Performance Comparison | CAN vs Ethernet metrics | Suitability analysis |
| 10 | Security Attack Detection | State machine | Attack prevention flow |
| 11 | Message Size Evolution | Size transformation | 8 bytes → 3,321 bytes |
| 12 | Dual-Platform Architecture | Windows & Raspberry Pi | Build system abstraction |
| 13 | Buffer Overflow Fix | Before/after comparison | Critical vulnerability fix |
| 14 | Freshness Management | Anti-replay mechanism | Replay attack prevention |
| 15 | Test Suite Architecture | Comprehensive testing | Both test files coverage |

**Total: 15 diagrams - All focused on Ethernet Gateway with Post-Quantum Cryptography**

---

## How to Use These Diagrams

### In GitHub/GitLab
Simply view the markdown file - Mermaid renders automatically.

### In VS Code
Install extension: "Markdown Preview Mermaid Support"

### Export to Images
Use Mermaid CLI:
```bash
npm install -g @mermaid-js/mermaid-cli
mmdc -i DIAGRAMS.md -o diagrams.pdf
```

### In LaTeX Reports
Convert to SVG/PNG:
```bash
mmdc -i DIAGRAMS.md -o diagram.png
```

---

**Key Features:**
- All diagrams emphasize **Ethernet Gateway** context
- Clear visualization of **CAN ↔ Ethernet** transformation
- **PQC integration** shown in gateway operations
- **Security mechanisms** (replay detection, tampering detection) highlighted
- **Performance trade-offs** between CAN and Ethernet clearly shown
- **Dual-platform** deployment (Windows development → Raspberry Pi deployment)

*END OF DIAGRAMS*
