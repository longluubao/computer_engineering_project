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

---

## PHASE 3 COMPLETE: ML-KEM + HKDF + SoAd_PQC Integration Diagrams

### 16. Phase 3 Complete Architecture Overview

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

---

### 17. ML-KEM-768 Key Exchange Flow (SoAd_PQC Integration)

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

---

### 18. HKDF Session Key Derivation (Detailed)

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

---

### 19. Complete AUTOSAR Signal Flow with Phase 3 Components

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

---

### 20. PQC Module Interaction Diagram (Phase 3 Complete)

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

---

### 21. Phase 3 Test Coverage Map

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

---

## Phase 3 Diagram Summary

| # | Diagram Name | Purpose | New Components Shown |
|---|-------------|---------|---------------------|
| 16 | Phase 3 Complete Architecture | Overall system view | PQC_KeyDerivation, SoAd_PQC, Phase3 Test |
| 17 | ML-KEM Key Exchange Flow | Detailed handshake sequence | SoAd_PQC, PQC_KeyExchange, HKDF integration |
| 18 | HKDF Session Key Derivation | Cryptographic key derivation | HKDF-Extract, HKDF-Expand, Session Keys |
| 19 | Complete AUTOSAR Signal Flow | Full stack with Phase 3 | All PQC modules integrated in AUTOSAR layers |
| 20 | PQC Module Interaction | Internal PQC architecture | PQC.c, PQC_KeyExchange.c, PQC_KeyDerivation.c |
| 21 | Phase 3 Test Coverage | Comprehensive test mapping | All 5 test suites with expected results |

**Total Diagrams: 21** (15 original + 6 Phase 3 diagrams)

---

# PART III: FINAL REPORT UPDATE — MODULES & FLOWS NOT PREVIOUSLY DEPICTED

> Sections 22–30 below were added during the final-report preparation. They reflect the
> current state of the source tree (~33 modules) and the latest hardware run on
> Raspberry Pi 4 (Total Secured PDU = 3343 bytes, all tests PASSED).
> Earlier diagrams (1–21) remain valid; these new diagrams fill the gaps that
> were identified when comparing the codebase to the existing documentation.

## 22. Complete Module Inventory (Current Codebase)

This diagram lists every module that physically exists under `source/`, grouped
by role. Modules introduced specifically for this PQC work are highlighted in
green; modules that exist but were absent from earlier diagrams are highlighted
in orange.

```mermaid
flowchart TB
    subgraph APP["Application & Bridge Layer"]
        APP_COM["Com<br/>signal/PDU pack"]
        APP_BRG["ApBridge<br/>health & state"]:::new
        APP_GUI["GUIInterface<br/>debug API"]:::new
    end

    subgraph SVC["Service Layer"]
        SVC_SEC["SecOC<br/>+ FVM"]:::touched
        SVC_CSM["Csm<br/>job queue"]:::touched
        SVC_DCM["Dcm"]:::orphan
        SVC_DEM["Dem"]:::orphan
        SVC_DET["Det"]:::orphan
        SVC_NVM["NvM"]:::orphan
        SVC_COMM["ComM"]:::orphan
        SVC_BSWM["BswM"]
        SVC_ECUM["EcuM"]
        SVC_OS["Os + Scheduler"]
    end

    subgraph CRY["Cryptography"]
        CRY_IF["CryIf<br/>adapter"]:::touched
        CRY_ENC["Encrypt<br/>AES legacy"]:::orphan
        CRY_PQC["PQC<br/>liboqs wrapper"]:::new
        CRY_KEX["PQC_KeyExchange<br/>ML-KEM-768"]:::new
        CRY_KDF["PQC_KeyDerivation<br/>HKDF-SHA256"]:::new
    end

    subgraph COMS["Communication Stack"]
        COM_PDUR["PduR"]
        COM_SOAD["SoAd"]:::touched
        COM_SOADP["SoAd_PQC<br/>handshake & rekey"]:::new
        COM_TCP["TcpIp"]
        COM_ETHIF["EthIf"]
        COM_ETH["Ethernet<br/>POSIX or Winsock"]
        COM_CAN["Can / CanIf / CanTp"]
        COM_CANNM["CanNm / CanSM"]:::orphan
        COM_UDPNM["UdpNm"]:::orphan
        COM_ETHSM["EthSM"]
    end

    subgraph MEM["Memory Stack"]
        MEM_MIF["MemIf"]:::orphan
        MEM_FEE["Fee"]:::orphan
        MEM_EA["Ea"]:::orphan
    end

    subgraph HW["Hardware Abstraction"]
        HW_MCAL["Mcal<br/>Pi4 drivers"]:::orphan
    end

    APP_COM --> COM_PDUR
    APP_BRG --> COM_SOAD
    APP_GUI --> SVC_SEC
    SVC_SEC --> SVC_CSM
    SVC_SEC --> COM_PDUR
    SVC_CSM --> CRY_IF
    CRY_IF --> CRY_PQC
    CRY_IF --> CRY_KEX
    CRY_IF --> CRY_KDF
    CRY_IF --> CRY_ENC
    COM_PDUR --> COM_SOAD
    COM_PDUR --> COM_CAN
    COM_SOAD --> COM_SOADP
    COM_SOADP --> SVC_CSM
    COM_SOAD --> COM_TCP
    COM_TCP --> COM_ETHIF
    COM_ETHIF --> COM_ETH
    COM_CAN --> HW_MCAL
    SVC_NVM --> MEM_MIF
    MEM_MIF --> MEM_FEE
    MEM_MIF --> MEM_EA

    classDef new fill:#a5d6a7,stroke:#1b5e20,color:#000,stroke-width:2px
    classDef touched fill:#fff59d,stroke:#f57f17,color:#000,stroke-width:2px
    classDef orphan fill:#ffcc80,stroke:#e65100,color:#000,stroke-width:1.5px
```

**Legend**

| Style | Meaning |
|-------|---------|
| Green (`new`) | Modules introduced for PQC work — `PQC`, `PQC_KeyExchange`, `PQC_KeyDerivation`, `SoAd_PQC`, `ApBridge`, `GUIInterface` |
| Yellow (`touched`) | Existing AUTOSAR modules modified to support PQC — `SecOC`, `Csm`, `CryIf`, `SoAd` |
| Orange (`orphan`) | Modules present in source but absent from previous diagrams |
| Default | Classic AUTOSAR modules already shown in earlier diagrams |

---

## 23. PQC Key File Provisioning at Boot

Captures the runtime sequence by which the ECU loads its long-term ML-DSA
keypair from the filesystem. This flow was previously missing from the
documentation.

```mermaid
sequenceDiagram
    autonumber
    participant Main as main.c
    participant EcuM
    participant SecOC
    participant Csm
    participant PQC as PQC core
    participant FS as /etc/secoc/keys/

    Main->>EcuM: EcuM_Init()
    EcuM->>EcuM: EcuM_StartupTwo()
    EcuM->>SecOC: SecOC_Init()
    SecOC->>Csm: Csm_Init()
    Csm->>PQC: PQC_Init()
    PQC->>PQC: OQS_init (liboqs)

    Note over Csm,FS: Bootstrap modes - STRICT fails if keys missing - DEMO auto-generates if absent

    Csm->>PQC: PQC_MLDSA_LoadKeys()
    PQC->>FS: open mldsa_secoc.pub (1952 B)
    PQC->>FS: open mldsa_secoc.key (4032 B)

    alt Files present
        FS-->>PQC: key bytes
        PQC-->>Csm: E_OK
    else STRICT and files missing
        PQC-->>Csm: E_NOT_OK
        Csm-->>SecOC: init failed
        SecOC-->>EcuM: SECOC_E_INIT_FAILED
    else DEMO and files missing
        Csm->>PQC: PQC_MLDSA_KeyGen()
        PQC->>FS: write mldsa_secoc.pub / .key (perms 0700)
        PQC-->>Csm: E_OK (auto-provisioned)
    end

    Csm-->>SecOC: ready
    SecOC-->>EcuM: ready
    EcuM->>Main: StartOS()
```

**Source references**

- `source/PQC/PQC.c:284-407` — `PQC_MLDSA_LoadKeys` / `SaveKeys`
- `include/Csm/Csm.h:74-84` — bootstrap mode enum and provisioning callback
- `include/SecOC/SecOC_PQC_Cfg.h:46-52` — `PQC_MLDSA_KEY_DIRECTORY` macro

---

## 24. Csm Job Queue and PQC Call Chain (Async)

The Csm layer queues crypto jobs and dispatches them to CryIf, which routes to
the PQC backend. The earlier Phase-3 diagram only showed the synchronous logical
chain; this diagram shows the actual job machinery.

```mermaid
flowchart LR
    subgraph SECOC["SecOC TX path"]
        AU["authenticate_PQC<br/>SecOC.c:1599"]
        DA["build DataToAuth<br/>DataId + AuthPdu + Freshness"]
    end

    subgraph CSM["Csm layer (Csm.c)"]
        Q["Job queue<br/>CSM_MAX_JOB_QUEUE = 16"]
        CTX["Job contexts<br/>CSM_MAX_JOB_CONTEXTS = 16"]
        SM["State<br/>IDLE -> ACTIVE -> QUEUED -> DONE"]
        SG["Csm_SignatureGenerate"]
        CB["completion callback"]
    end

    subgraph CRYIF["CryIf (CryIf.c)"]
        CIF_SG["CryIf_SignatureGenerate"]
    end

    subgraph PQC["PQC backend"]
        PSIGN["PQC_MLDSA_Sign<br/>liboqs OQS_SIG_ml_dsa_65"]
    end

    AU --> DA --> SG
    SG --> Q --> CTX --> SM
    SM --> CIF_SG --> PSIGN
    PSIGN -->|3309-byte signature| CB
    CB --> AU
    AU -->|build Secured I-PDU| SECOC

    style PSIGN fill:#a5d6a7,color:#000
    style Q fill:#fff59d,color:#000
    style SM fill:#fff59d,color:#000
```

The synchronous test path uses `CRYPTO_OPERATIONMODE_SINGLECALL`, which still
goes through the queue but completes within the same call.

---

## 25. ML-KEM Per-Peer Session State Machine

`PQC_KeyExchange` keeps an independent state machine for each of up to 8 peers.
Earlier diagrams showed the happy-path sequence but not the FSM with error and
rekey transitions.

```mermaid
stateDiagram-v2
    [*] --> IDLE
    IDLE --> INITIATED: PQC_KeyExchange_Initiate<br/>generates 1184-B public key
    INITIATED --> RESPONDED: PQC_KeyExchange_Respond<br/>receives publicKey,<br/>produces 1088-B ciphertext
    RESPONDED --> ESTABLISHED: PQC_KeyExchange_Complete<br/>recovers 32-B shared secret

    INITIATED --> IDLE: Reset or timeout
    RESPONDED --> IDLE: Reset
    ESTABLISHED --> IDLE: Reset (rekey)
    ESTABLISHED --> INITIATED: scheduled rekey<br/>SoAd_PQC every N cycles

    INITIATED --> ERROR: invalid peerId or NULL or liboqs failure
    RESPONDED --> ERROR
    ERROR --> IDLE: explicit Reset

    note right of ESTABLISHED
        Shared secret available
        via GetSharedSecret.
        Caller normally chains
        to PQC_DeriveSessionKeys.
    end note
```

**Source**: `source/PQC/PQC_KeyExchange.c`, peer slot array sized for 8 peers.

---

## 26. HKDF Internals — Extract + Expand

Phase-3 diagram 18 shows HKDF as a black box. The diagram below shows the
two-phase RFC 5869 construction as actually implemented.

```mermaid
flowchart TB
    SS["Shared secret 32 B<br/>from ML-KEM Decaps"]
    SALT["Salt = AUTOSAR-SecOC-PQC-v1.0"]
    PRK["PRK 32 B<br/>= HMAC-SHA256(salt, SS)"]
    SS --> EXTR["HKDF-Extract<br/>HMAC-SHA256"]
    SALT --> EXTR
    EXTR --> PRK

    PRK --> EXP1["HKDF-Expand<br/>info = Encryption-Key"]
    PRK --> EXP2["HKDF-Expand<br/>info = Authentication-Key"]
    EXP1 --> EK["EncryptionKey 32 B<br/>used for AES-256-GCM"]
    EXP2 --> AK["AuthenticationKey 32 B<br/>used for HMAC-SHA256"]

    EK --> SK["PQC_SessionKeysType<br/>IsValid = TRUE"]
    AK --> SK

    style PRK fill:#fff59d,color:#000
    style SK fill:#a5d6a7,color:#000
```

Properties guaranteed by the test suite (`KeyDerivationTests.cpp`):

- Deterministic: same shared secret produces same keys
- `EncryptionKey != AuthenticationKey`
- Different shared secrets produce different keys
- Up to 16 peer slots; sessions are independent

---

## 27. SoAd_PQC Rekey Scheduler

`SoAd_PQC_MainFunction` runs on the BSW main-cycle tick. Each peer has its own
rekey counter; when it reaches zero a new ML-KEM handshake is triggered.

```mermaid
sequenceDiagram
    participant Tick as BSW Main 10 ms
    participant SP as SoAd_PQC
    participant Csm
    participant Peer as Remote ECU

    loop every cycle
        Tick->>SP: SoAd_PQC_MainFunction
        loop for each peer
            SP->>SP: RekeyCycles[peer]--
            alt counter == 0
                SP->>Csm: Csm_KeyExchangeInitiate(peer)
                Csm->>Peer: send public key 1184 B (magic 0x5143)
                Peer-->>Csm: ciphertext 1088 B
                Csm->>Csm: Csm_KeyExchangeComplete(peer)
                Csm->>Csm: Csm_DeriveSessionKeys(peer)
                SP->>SP: RekeyCycles[peer] = REKEY_INTERVAL
            end
        end
    end
```

Default `SOAD_PQC_REKEY_INTERVAL_CYCLES = 360000` (= 1 hour at 10 ms tick).
Configurable in `include/SecOC/SecOC_PQC_Cfg.h:59`.

---

## 28. DataToAuth Construction (Tx side)

Explains the 64-byte authenticator input observed in the hardware run on Pi 4.

```mermaid
flowchart LR
    subgraph BUFFER["DataToAuth buffer 64 B observed on Pi 4"]
        DID["DataId 2 B"]
        APDU["AuthPdu 32 B (test payload 0x00..0x1F)"]
        FRESH["FreshnessFull 4 B counter"]
        PAD["alignment / padding to 64 B"]
    end

    subgraph WIRE["Secured I-PDU on the wire 3343 B"]
        H["Header 1 B"]
        APDU2["AuthPdu 32 B"]
        TF["TruncFreshness 1 B"]
        SIG["ML-DSA-65 Signature 3309 B"]
    end

    DID --> SIGN["ML-DSA-65 Sign<br/>via Csm + CryIf + PQC"]
    APDU --> SIGN
    FRESH --> SIGN
    SIGN --> SIG

    APDU --> APDU2

    style SIGN fill:#fff59d,color:#000
    style SIG fill:#a5d6a7,color:#000
```

Two distinct buffers:

- **DataToAuth** — only fed into the signature; never transmitted. Carries the
  *full* freshness counter plus DataId so the receiver can reconstruct identical
  input before verification.
- **Secured I-PDU** — what actually goes onto Ethernet. Contains the *truncated*
  freshness (1 byte by default) plus the 3309-byte ML-DSA-65 signature.

---

## 29. End-to-End Hardware Run on Raspberry Pi 4 (Latest)

Captured from the most recent successful run of `Phase3_Complete_Test` on the
real Pi 4 target.

```mermaid
flowchart LR
    subgraph BOOT["Boot"]
        B1["EcuM_Init"]
        B2["SecOC_Init<br/>load /etc/secoc/keys/"]
        B3["SoAd_PQC_Init<br/>TCP connect"]
    end

    subgraph TX["TX path measured"]
        T1["app payload 32 bytes"]
        T2["FVM_GetTxFreshness<br/>FreshnessLenBits = 32"]
        T3["build DataToAuth = 64 B"]
        T4["Csm_SignatureGenerate"]
        T5["PQC_MLDSA_Sign<br/>3309-B signature<br/>starts with E5 AB 90 96"]
        T6["assemble Secured I-PDU<br/>1 + 32 + 1 + 3309 = 3343 B"]
        T7["SoAd -> TcpIp -> Ethernet"]
    end

    subgraph RESULT["Reported result"]
        R1["All tests PASSED"]
        R2["ML-KEM-768 pk 1184 ct 1088"]
        R3["ML-DSA-65 pk 1952 sig 3309"]
        R4["Total Secured PDU = 3343 bytes"]
    end

    B1 --> B2 --> B3 --> T1 --> T2 --> T3 --> T4 --> T5 --> T6 --> T7 --> R1
    R1 --> R2 --> R3 --> R4

    style T5 fill:#a5d6a7,color:#000
    style R4 fill:#a5d6a7,color:#000
```

---

## 30. Module Classification Summary

```mermaid
pie title Source modules by category
    "Classic AUTOSAR (re-used as-is)" : 21
    "AUTOSAR modified for PQC" : 4
    "New PQC modules" : 4
    "New application / debug" : 2
    "Hardware abstraction (Mcal)" : 1
    "OS / scheduling" : 2
```

**Per-category list**

| Category | Modules |
|----------|---------|
| Classic AUTOSAR (re-used) | Com, PduR, TcpIp, EthIf, Ethernet, Can, CanIf, CanTp, CanNm, CanSM, EthSM, ComM, UdpNm, BswM, EcuM, Dcm, Dem, Det, NvM, Fee, Ea, MemIf, Encrypt |
| Modified for PQC | SecOC (+FVM), Csm, CryIf, SoAd |
| New PQC modules | PQC, PQC_KeyExchange, PQC_KeyDerivation, SoAd_PQC |
| New application / debug | ApBridge, GUIInterface |
| Hardware abstraction | Mcal (Pi4 drivers) |
| OS / scheduling | Os, Scheduler |

---

## Updated Diagram Index

| # | Section | Added in |
|---|---------|----------|
| 1-15 | Original gateway and security diagrams | Initial release |
| 16-21 | Phase 3 ML-KEM + HKDF + SoAd_PQC | Phase 3 |
| **22** | **Complete Module Inventory** | **Final report** |
| **23** | **PQC Key File Provisioning at Boot** | **Final report** |
| **24** | **Csm Job Queue and PQC Call Chain** | **Final report** |
| **25** | **ML-KEM Per-Peer Session FSM** | **Final report** |
| **26** | **HKDF Internals (Extract + Expand)** | **Final report** |
| **27** | **SoAd_PQC Rekey Scheduler** | **Final report** |
| **28** | **DataToAuth Construction (Tx)** | **Final report** |
| **29** | **End-to-End Hardware Run on Pi 4** | **Final report** |
| **30** | **Module Classification Summary** | **Final report** |

**Total Diagrams: 30**

---

*END OF DIAGRAMS*
