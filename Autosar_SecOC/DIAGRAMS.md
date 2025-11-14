# AUTOSAR SecOC with PQC - Architecture Diagrams
## Mermaid Diagrams for Technical Report

All diagrams can be rendered in GitHub, GitLab, VS Code, or any Mermaid-compatible viewer.

---

## 1. High-Level System Architecture

```mermaid
graph TB
    subgraph "Application Layer"
        APP[Application<br/>Send/Receive Messages]
    end

    subgraph "AUTOSAR Communication Stack"
        COM[COM Manager<br/>Communication Interface]

        subgraph "SecOC - Secure Onboard Communication"
            FVM[Freshness Value<br/>Manager]
            CSM[Crypto Service<br/>Manager]

            subgraph "PQC Integration"
                MLKEM[ML-KEM-768<br/>Key Exchange]
                MLDSA[ML-DSA-65<br/>Digital Signature]
                LIBOQS[liboqs<br/>Open Quantum Safe]
            end
        end

        PDUR[PduR<br/>PDU Router]

        subgraph "Transport Layer"
            CANIF[CanIf<br/>CAN Interface]
            CANTP[CanTP<br/>CAN Transport]
            SOADTP[SoAdTP<br/>Socket Adaptor TP]
            SOADIF[SoAdIf<br/>Socket Adaptor IF]
        end
    end

    subgraph "Physical Layer"
        CAN[CAN Bus<br/>500 kbps]
        ETH[Ethernet TCP/IP<br/>100 Mbps<br/>Port 12345]
    end

    APP <--> COM
    COM <--> FVM
    COM <--> CSM
    FVM <--> SecOC_Main[SecOC Main]
    CSM <--> MLKEM
    CSM <--> MLDSA
    MLKEM <--> LIBOQS
    MLDSA <--> LIBOQS
    SecOC_Main <--> PDUR
    PDUR <--> CANIF
    PDUR <--> CANTP
    PDUR <--> SOADTP
    PDUR <--> SOADIF
    CANIF <--> CAN
    CANTP <--> CAN
    SOADTP <--> ETH
    SOADIF <--> ETH

    style APP fill:#e1f5ff
    style COM fill:#b3e5fc
    style FVM fill:#81d4fa
    style CSM fill:#4fc3f7
    style MLKEM fill:#ff9800
    style MLDSA fill:#ff9800
    style LIBOQS fill:#f57c00
    style PDUR fill:#4fc3f7
    style ETH fill:#4caf50
    style CAN fill:#8bc34a
```

---

## 2. Dual-Platform Architecture

```mermaid
graph TB
    subgraph "AUTOSAR SecOC Application"
        APP[Common Codebase<br/>SecOC + PQC]
    end

    subgraph "Platform Abstraction Layer"
        direction LR

        subgraph "Windows Development"
            WIN_API[Winsock2 API]
            WIN_IMPL[ethernet_windows.c]
            WIN_TOOLS[MSYS2/MinGW64<br/>x86_64]
            WIN_FLAGS[-DWINDOWS]

            WIN_API --> WIN_IMPL
            WIN_IMPL --> WIN_TOOLS
            WIN_TOOLS --> WIN_FLAGS
        end

        subgraph "Linux Deployment"
            LIN_API[BSD Sockets API]
            LIN_IMPL[ethernet.c]
            LIN_HW[Raspberry Pi 4<br/>ARM Cortex-A72]
            LIN_FLAGS[-DRASPBERRY_PI<br/>-mcpu=cortex-a72]

            LIN_API --> LIN_IMPL
            LIN_IMPL --> LIN_HW
            LIN_HW --> LIN_FLAGS
        end
    end

    subgraph "Build System"
        CMAKE[CMake Platform Detection]
        BUILD[build_and_run.sh]

        CMAKE --> BUILD
    end

    APP --> WIN_IMPL
    APP --> LIN_IMPL
    BUILD --> WIN_FLAGS
    BUILD --> LIN_FLAGS

    style APP fill:#e1f5ff
    style WIN_API fill:#ff9800
    style LIN_API fill:#4caf50
    style CMAKE fill:#9c27b0
    style BUILD fill:#9c27b0
```

---

## 3. Complete Transmission Path (Tx)

```mermaid
sequenceDiagram
    participant APP as Application
    participant COM as COM Manager
    participant SecOC as SecOC Engine
    participant FVM as Freshness Manager
    participant CSM as Crypto Service
    participant PQC as PQC Module
    participant PduR as PduR Router
    participant ETH as Ethernet Layer
    participant BUS as Physical Bus

    APP->>COM: sendMessage(data)
    Note over COM: Pack into PDU

    COM->>SecOC: PduR_ComTransmit(pduId, PduInfo)

    SecOC->>FVM: GetTxFreshness(pduId)
    FVM-->>SecOC: freshnessValue (32-bit counter)

    Note over SecOC: Build Authenticator:<br/>[PDU Data][Freshness Value]

    SecOC->>CSM: Generate MAC/Signature
    CSM->>PQC: PQC_Sign(data, freshness)

    Note over PQC: ML-DSA-65 Sign<br/>~8.1ms
    PQC-->>CSM: signature (3,309 bytes)
    CSM-->>SecOC: authenticator

    Note over SecOC: Build Secured PDU:<br/>[Data][FV_truncated][Signature]

    SecOC->>PduR: PduR_SecOCTransmit(securedPdu)
    PduR->>ETH: Route to Ethernet

    ETH->>BUS: ethernet_send(id, data, 3380 bytes)
    Note over BUS: TCP Socket<br/>Port 12345

    Note over APP,BUS: Total Tx Time: ~8.1ms (PQC signing)
```

---

## 4. Complete Reception Path (Rx)

```mermaid
sequenceDiagram
    participant BUS as Physical Bus
    participant ETH as Ethernet Layer
    participant PduR as PduR Router
    participant SecOC as SecOC Engine
    participant FVM as Freshness Manager
    participant CSM as Crypto Service
    participant PQC as PQC Module
    participant COM as COM Manager
    participant APP as Application

    BUS->>ETH: Packet Arrival
    Note over ETH: ethernet_receive()<br/>4096 byte buffer

    ETH->>PduR: ethernet_RecieveMainFunction()
    PduR->>SecOC: SecOC_RxIndication(pduId, PduInfo)

    Note over SecOC: Parse Secured PDU:<br/>Extract Data, FV, Signature

    SecOC->>FVM: GetRxFreshness(truncatedFV)
    FVM-->>SecOC: fullFreshnessValue (reconstructed)

    Note over FVM: Replay Check:<br/>rxFV > lastRxFV?

    alt Freshness Value Invalid
        FVM-->>SecOC: E_NOT_OK
        Note over SecOC: DROP PDU<br/>Replay Attack!
    else Freshness Value Valid
        SecOC->>CSM: Verify MAC/Signature
        CSM->>PQC: PQC_Verify(data, freshness, signature)

        Note over PQC: ML-DSA-65 Verify<br/>~4.9ms

        alt Signature Invalid
            PQC-->>CSM: VERIFICATION_FAILED
            CSM-->>SecOC: E_NOT_OK
            Note over SecOC: DROP PDU<br/>Tampering Detected!
        else Signature Valid
            PQC-->>CSM: VERIFICATION_SUCCESS
            CSM-->>SecOC: E_OK

            SecOC->>PduR: PduR_SecOCIfRxIndication(authenticPdu)
            PduR->>COM: Com_RxIndication(pduId, data)
            COM->>APP: receiveMessage(data)

            Note over APP: Authentic Message<br/>Delivered ✓
        end
    end

    Note over BUS,APP: Total Rx Time: ~4.9ms (PQC verify)
```

---

## 5. SecOC State Machine

```mermaid
stateDiagram-v2
    [*] --> IDLE

    IDLE --> VERIFY_START: Rx PDU Arrival
    VERIFY_START --> GET_FRESHNESS: SecOC_RxIndication()

    GET_FRESHNESS --> FRESHNESS_CHECK: GetRxFreshness()

    FRESHNESS_CHECK --> DROP_REPLAY: Freshness ≤ Last
    FRESHNESS_CHECK --> VERIFICATION: Freshness > Last (OK)

    DROP_REPLAY --> LOG_SECURITY_EVENT: Replay Attack
    LOG_SECURITY_EVENT --> IDLE

    VERIFICATION --> CRYPTO_VERIFY: Build Authenticator
    CRYPTO_VERIFY --> DROP_TAMPER: Verify FAILED
    CRYPTO_VERIFY --> AUTHENTICATED: Verify SUCCESS

    DROP_TAMPER --> LOG_SECURITY_EVENT: Tampering Detected

    AUTHENTICATED --> FORWARD_PDU: Extract Original PDU
    FORWARD_PDU --> DELIVER_TO_COM: PduR_SecOCIfRxIndication()
    DELIVER_TO_COM --> IDLE: Message Delivered

    note right of VERIFY_START
        Entry point for
        received PDU
    end note

    note right of FRESHNESS_CHECK
        Anti-replay
        protection
    end note

    note right of CRYPTO_VERIFY
        PQC Signature
        Verification
        (ML-DSA-65)
    end note

    note right of AUTHENTICATED
        Security checks
        passed ✓
    end note
```

---

## 6. PQC Integration Architecture

```mermaid
graph TB
    subgraph "Crypto Service Manager (CSM)"
        CSM_IFACE[CSM Interface]
        CSM_MAC[MacGenerate/Verify<br/>Classical CMAC]
        CSM_PQC[PQC Integration Layer]
    end

    subgraph "PQC Module"
        subgraph "PQC_KeyExchange.c"
            KEM_KEYGEN[ML-KEM-768<br/>KeyPair Generation]
            KEM_ENCAPS[ML-KEM-768<br/>Encapsulation]
            KEM_DECAPS[ML-KEM-768<br/>Decapsulation]

            KEM_KEYGEN --> KEM_SIZE1[Public: 1,184 bytes<br/>Secret: 2,400 bytes]
            KEM_ENCAPS --> KEM_SIZE2[Ciphertext: 1,088 bytes]
            KEM_DECAPS --> KEM_SIZE3[Shared Secret: 32 bytes]
        end

        subgraph "PQC.c"
            DSA_KEYGEN[ML-DSA-65<br/>KeyPair Generation]
            DSA_SIGN[ML-DSA-65<br/>Sign]
            DSA_VERIFY[ML-DSA-65<br/>Verify]

            DSA_KEYGEN --> DSA_SIZE1[Public: 1,952 bytes<br/>Secret: 4,032 bytes]
            DSA_SIGN --> DSA_SIZE2[Signature: 3,309 bytes<br/>⚠️ Large!]
            DSA_VERIFY --> DSA_SIZE3[Result: PASS/FAIL]
        end
    end

    subgraph "liboqs Library"
        OQS_KEM[OQS_KEM_ml_kem_768]
        OQS_SIG[OQS_SIG_ml_dsa_65]
        OQS_COMMON[NIST Reference<br/>Implementations]
    end

    CSM_IFACE --> CSM_MAC
    CSM_IFACE --> CSM_PQC

    CSM_PQC --> KEM_KEYGEN
    CSM_PQC --> KEM_ENCAPS
    CSM_PQC --> KEM_DECAPS
    CSM_PQC --> DSA_SIGN
    CSM_PQC --> DSA_VERIFY

    KEM_KEYGEN --> OQS_KEM
    KEM_ENCAPS --> OQS_KEM
    KEM_DECAPS --> OQS_KEM
    DSA_KEYGEN --> OQS_SIG
    DSA_SIGN --> OQS_SIG
    DSA_VERIFY --> OQS_SIG

    OQS_KEM --> OQS_COMMON
    OQS_SIG --> OQS_COMMON

    style CSM_PQC fill:#ff9800
    style OQS_KEM fill:#f57c00
    style OQS_SIG fill:#f57c00
    style DSA_SIZE2 fill:#ff5722
```

---

## 7. Ethernet Gateway Use Case

```mermaid
graph TB
    subgraph "Vehicle Network"
        subgraph "CAN Network (500 kbps)"
            ENGINE[Engine ECU<br/>Speed, RPM]
            BRAKE[Brake ECU<br/>ABS Status]
            STEERING[Steering ECU<br/>Angle]

            ENGINE --- CAN_BUS[CAN Bus]
            BRAKE --- CAN_BUS
            STEERING --- CAN_BUS
        end

        subgraph "Gateway ECU (Raspberry Pi 4)"
            CAN_CTRL[CAN Controller<br/>MCP2515<br/>SPI Interface]

            subgraph "AUTOSAR SecOC Stack"
                SECOC_GW[SecOC Engine]
                PQC_GW[PQC Module<br/>ML-DSA-65]
                ETH_TX[Ethernet Tx]
            end

            CAN_CTRL --> SECOC_GW
            SECOC_GW --> PQC_GW
            PQC_GW --> ETH_TX
        end

        CAN_BUS --> CAN_CTRL

        subgraph "Ethernet Network (100 Mbps)"
            ETH_NET[Ethernet<br/>TCP/IP<br/>192.168.1.x]
        end

        ETH_TX --> ETH_NET

        subgraph "Backend Systems"
            CENTRAL[Central ECU<br/>192.168.1.200]
            TELEM[Telematics Unit<br/>Cloud Gateway]
            DIAG[Diagnostic Tool<br/>OBD-II Interface]

            ETH_NET --> CENTRAL
            ETH_NET --> TELEM
            ETH_NET --> DIAG
        end
    end

    style CAN_BUS fill:#8bc34a
    style SECOC_GW fill:#ff9800
    style PQC_GW fill:#f57c00
    style ETH_NET fill:#4caf50
    style CENTRAL fill:#2196f3
```

---

## 8. Message Format - Secured PDU

```mermaid
graph LR
    subgraph "Secured PDU Structure (Total: ~3,380 bytes)"
        subgraph "Header"
            PDU_ID[PDU ID<br/>2 bytes]
        end

        subgraph "Payload"
            ORIG_DATA[Original PDU Data<br/>8-1024 bytes<br/>CAN/Ethernet Message]

            FV_TRUNC[Truncated Freshness<br/>3 bytes<br/>24-bit Counter]

            subgraph "Authenticator"
                direction TB
                AUTH_TYPE{Type?}

                AUTH_TYPE -->|Classical| CMAC[CMAC<br/>16 bytes<br/>AES-128]
                AUTH_TYPE -->|PQC| SIG[ML-DSA-65 Signature<br/>3,309 bytes<br/>⚠️ Quantum-Resistant]
            end
        end
    end

    PDU_ID --> ORIG_DATA
    ORIG_DATA --> FV_TRUNC
    FV_TRUNC --> AUTH_TYPE

    style ORIG_DATA fill:#b3e5fc
    style FV_TRUNC fill:#81d4fa
    style CMAC fill:#4fc3f7
    style SIG fill:#ff9800

    classDef critical fill:#ff5722,color:#fff
    class SIG critical
```

---

## 9. Buffer Overflow Fix Visualization

```mermaid
graph TB
    subgraph "BEFORE - Vulnerable Implementation"
        OLD_DEF["#define BUS_LENGTH_RECEIVE 8"]
        OLD_BUF[Buffer: uint8 sendData[10]<br/>BUS_LENGTH_RECEIVE + sizeof(id)]
        OLD_COPY[memcpy(sendData, data, dataLen)]
        OLD_SEND[send(socket, sendData, 10, 0)]

        OLD_DEF --> OLD_BUF
        OLD_BUF --> OLD_COPY
        OLD_COPY --> OLD_SEND

        OLD_ISSUE1[❌ Buffer Overflow<br/>Writing 3,309 bytes<br/>into 10-byte buffer]
        OLD_ISSUE2[❌ Data Truncation<br/>Sending only 10 bytes<br/>Losing 99.7% of signature]

        OLD_COPY -.->|dataLen = 3,319| OLD_ISSUE1
        OLD_SEND -.->|Hardcoded 10| OLD_ISSUE2
    end

    subgraph "AFTER - Fixed Implementation"
        NEW_DEF["#define BUS_LENGTH_RECEIVE 4096"]
        NEW_BUF[Buffer: uint8 sendData[4098]<br/>BUS_LENGTH_RECEIVE + sizeof(id)]
        NEW_COPY[memcpy(sendData, data, dataLen)]
        NEW_ID[sendData[dataLen + indx] = id bytes]
        NEW_SEND[send(socket, sendData,<br/>dataLen + sizeof(id), 0)]

        NEW_DEF --> NEW_BUF
        NEW_BUF --> NEW_COPY
        NEW_COPY --> NEW_ID
        NEW_ID --> NEW_SEND

        NEW_SAFE1[✅ Safe Buffer<br/>4,098 bytes<br/>Fits PQC signature]
        NEW_SAFE2[✅ Dynamic Length<br/>Sends actual data size<br/>Full signature transmitted]

        NEW_BUF -.-> NEW_SAFE1
        NEW_SEND -.-> NEW_SAFE2
    end

    style OLD_ISSUE1 fill:#ff5722,color:#fff
    style OLD_ISSUE2 fill:#ff5722,color:#fff
    style NEW_SAFE1 fill:#4caf50,color:#fff
    style NEW_SAFE2 fill:#4caf50,color:#fff
```

---

## 10. Performance Comparison: Classical vs PQC

```mermaid
graph TB
    subgraph "Cryptographic Operations Comparison"
        subgraph "Classical (CMAC-AES128)"
            CLASS_KEYGEN[Key Generation<br/>~0.01 ms]
            CLASS_MAC[MAC Generate<br/>~0.05 ms]
            CLASS_VERIFY[MAC Verify<br/>~0.05 ms]
            CLASS_SIZE[Authenticator<br/>16 bytes]

            CLASS_PERF[Total: ~0.10 ms<br/>10,000 msg/sec]
        end

        subgraph "PQC (ML-DSA-65)"
            PQC_KEYGEN[Key Generation<br/>~5.2 ms]
            PQC_SIGN[Sign<br/>~8.1 ms]
            PQC_VERIFY[Verify<br/>~4.9 ms]
            PQC_SIZE[Signature<br/>3,309 bytes]

            PQC_PERF[Total: ~13.0 ms<br/>123 msg/sec]
        end

        subgraph "Trade-off Analysis"
            OVERHEAD[Performance Overhead<br/>❌ 130x slower<br/>❌ 207x larger signatures]
            BENEFIT[Security Benefit<br/>✅ Quantum-resistant<br/>✅ 20+ year protection<br/>✅ Non-repudiation]
            VERDICT[Verdict: Acceptable for<br/>non-real-time data]
        end
    end

    CLASS_PERF --> OVERHEAD
    PQC_PERF --> OVERHEAD
    OVERHEAD --> BENEFIT
    BENEFIT --> VERDICT

    style CLASS_PERF fill:#4caf50
    style PQC_PERF fill:#ff9800
    style OVERHEAD fill:#ff5722,color:#fff
    style BENEFIT fill:#2196f3,color:#fff
    style VERDICT fill:#9c27b0,color:#fff
```

---

## 11. Security Attack Defense Flow

```mermaid
flowchart TD
    START([PDU Received]) --> EXTRACT[Extract Components:<br/>Data, Freshness, Signature]

    EXTRACT --> FRESH_CHECK{Freshness Value<br/>Valid?}

    FRESH_CHECK -->|rxFV ≤ lastFV| REPLAY[Replay Attack<br/>Detected!]
    FRESH_CHECK -->|rxFV > lastFV| UPDATE_FV[Update lastFV<br/>rxFV is new]

    REPLAY --> LOG1[Log Security Event]
    LOG1 --> DROP1[Drop PDU]
    DROP1 --> END1([BLOCKED])

    UPDATE_FV --> BUILD_AUTH[Build Authenticator:<br/>[Data][Freshness]]
    BUILD_AUTH --> VERIFY{PQC Signature<br/>Verification}

    VERIFY -->|FAIL| TAMPER[Tampering or<br/>Spoofing Detected!]
    VERIFY -->|SUCCESS| AUTH_OK[Signature Valid]

    TAMPER --> LOG2[Log Security Event]
    LOG2 --> DROP2[Drop PDU]
    DROP2 --> END2([BLOCKED])

    AUTH_OK --> DELIVER[Forward Authentic PDU<br/>to COM Layer]
    DELIVER --> APP[Deliver to<br/>Application]
    APP --> END3([SUCCESS])

    style START fill:#b3e5fc
    style REPLAY fill:#ff5722,color:#fff
    style TAMPER fill:#ff5722,color:#fff
    style AUTH_OK fill:#4caf50,color:#fff
    style END1 fill:#ff5722,color:#fff
    style END2 fill:#ff5722,color:#fff
    style END3 fill:#4caf50,color:#fff
```

---

## 12. Raspberry Pi 4 Deployment Architecture

```mermaid
graph TB
    subgraph "Raspberry Pi 4 Hardware"
        CPU[ARM Cortex-A72<br/>Quad-Core @ 1.5GHz<br/>ARMv8-A 64-bit]
        RAM[4GB LPDDR4-3200<br/>Shared Memory]
        ETH_HW[Gigabit Ethernet<br/>RJ45 Port]
        GPIO[40-Pin GPIO Header]
        SD[microSD Card<br/>32GB Storage]

        CPU --- RAM
        CPU --- ETH_HW
        CPU --- GPIO
        CPU --- SD
    end

    subgraph "Operating System"
        KERNEL[Linux Kernel 6.1<br/>64-bit ARM]
        OS[Raspberry Pi OS<br/>Debian Bookworm]

        KERNEL --- OS
    end

    subgraph "Software Stack"
        LIBOQS_ARM[liboqs<br/>ARM-Optimized Build<br/>-mcpu=cortex-a72]
        AUTOSAR_APP[AUTOSAR SecOC<br/>+ PQC Integration]
        SOCKET_API[BSD Sockets API<br/>ethernet.c]

        LIBOQS_ARM --> AUTOSAR_APP
        AUTOSAR_APP --> SOCKET_API
    end

    subgraph "External Interfaces"
        CAN_MODULE[MCP2515<br/>CAN Controller<br/>SPI Connection]
        ETH_NET[Ethernet Network<br/>192.168.1.x]

        GPIO -.->|SPI| CAN_MODULE
        ETH_HW --> ETH_NET
    end

    SD --> OS
    OS --> LIBOQS_ARM
    SOCKET_API --> ETH_HW
    AUTOSAR_APP -.->|Control| CAN_MODULE

    style CPU fill:#ff9800
    style LIBOQS_ARM fill:#f57c00
    style AUTOSAR_APP fill:#2196f3
    style CAN_MODULE fill:#8bc34a
    style ETH_NET fill:#4caf50
```

---

## 13. Build System Flow

```mermaid
flowchart TD
    START([User: bash build_and_run.sh all]) --> DETECT{Detect Platform}

    DETECT -->|uname -m = x86_64| WIN_PLATFORM[Platform: X86_64<br/>Flags: -DWINDOWS]
    DETECT -->|uname -m = aarch64| RPI_PLATFORM[Platform: RASPBERRY_PI<br/>Flags: -DRASPBERRY_PI<br/>-mcpu=cortex-a72]

    WIN_PLATFORM --> CHECK_LIBOQS{liboqs<br/>exists?}
    RPI_PLATFORM --> CHECK_LIBOQS

    CHECK_LIBOQS -->|No| BUILD_LIBOQS[Build liboqs<br/>1. cmake -DOQS_ENABLE_KEM_ml_kem_768=ON<br/>2. cmake -DOQS_ENABLE_SIG_ml_dsa_65=ON<br/>3. ninja]
    CHECK_LIBOQS -->|Yes| SKIP_LIBOQS[Skip rebuild<br/>liboqs.a exists]

    BUILD_LIBOQS --> BUILD_INTEGRATION
    SKIP_LIBOQS --> BUILD_INTEGRATION

    BUILD_INTEGRATION[Build Integration Test<br/>gcc test_autosar_integration.c<br/>+ SecOC + PQC + liboqs<br/>→ test_autosar_integration.exe]

    BUILD_INTEGRATION --> BUILD_ADVANCED[Build Advanced Test<br/>gcc test_pqc_advanced.c<br/>→ test_pqc_advanced.exe]

    BUILD_ADVANCED --> SUCCESS([Build Complete ✓])

    style START fill:#b3e5fc
    style WIN_PLATFORM fill:#ff9800
    style RPI_PLATFORM fill:#4caf50
    style SUCCESS fill:#4caf50,color:#fff
```

---

## 14. Test Flow - Comprehensive Integration

```mermaid
flowchart TD
    START([test_autosar_integration.exe]) --> INIT[Initialize<br/>• SecOC<br/>• PQC Keys<br/>• Ethernet]

    INIT --> TEST1[Test 1:<br/>Transmission Path]

    TEST1 --> TX1[Send 8-byte CAN message]
    TX1 --> TX2[Send 64-byte CAN-FD message]
    TX2 --> TX3[Send 1024-byte Ethernet message]
    TX3 --> TX_RESULT{All Tx<br/>Pass?}

    TX_RESULT -->|No| FAIL1[❌ Test Failed]
    TX_RESULT -->|Yes| TEST2[Test 2:<br/>Reception Path]

    TEST2 --> RX1[Receive & Verify<br/>Authentic Message]
    RX1 --> RX_RESULT{Rx<br/>Pass?}

    RX_RESULT -->|No| FAIL2[❌ Test Failed]
    RX_RESULT -->|Yes| TEST3[Test 3:<br/>Security Attacks]

    TEST3 --> ATK1[Attack 1: Replay<br/>Send old message]
    ATK1 --> ATK1_CHK{Blocked?}
    ATK1_CHK -->|No| FAIL3[❌ Replay not detected!]
    ATK1_CHK -->|Yes| ATK2[Attack 2: Tampering<br/>Modify data]

    ATK2 --> ATK2_CHK{Blocked?}
    ATK2_CHK -->|No| FAIL4[❌ Tampering not detected!]
    ATK2_CHK -->|Yes| ATK3[Attack 3: Payload Mod<br/>Change payload]

    ATK3 --> ATK3_CHK{Blocked?}
    ATK3_CHK -->|No| FAIL5[❌ Attack not detected!]
    ATK3_CHK -->|Yes| TEST4[Test 4:<br/>Performance Benchmark]

    TEST4 --> BENCH[100 iterations<br/>5 message sizes<br/>Measure: sign, verify, total]
    BENCH --> CSV[Export Results<br/>autosar_integration_results.csv]
    CSV --> SUCCESS([✅ All Tests Passed])

    FAIL1 --> END_FAIL([Test Aborted])
    FAIL2 --> END_FAIL
    FAIL3 --> END_FAIL
    FAIL4 --> END_FAIL
    FAIL5 --> END_FAIL

    style SUCCESS fill:#4caf50,color:#fff
    style END_FAIL fill:#ff5722,color:#fff
    style ATK1_CHK fill:#ff9800
    style ATK2_CHK fill:#ff9800
    style ATK3_CHK fill:#ff9800
```

---

## 15. Real-World Use Case: Vehicle Data Flow

```mermaid
sequenceDiagram
    participant ENGINE as Engine ECU<br/>(CAN)
    participant GATEWAY as Gateway<br/>Raspberry Pi 4
    participant SECOC as SecOC Engine
    participant PQC as PQC Module
    participant CLOUD as Cloud Backend

    Note over ENGINE: Engine RPM: 3000<br/>Timestamp: T0

    ENGINE->>GATEWAY: CAN Frame<br/>[RPM=3000]
    Note over GATEWAY: MCP2515<br/>CAN → SPI

    GATEWAY->>SECOC: Process CAN Data
    SECOC->>SECOC: Get Freshness: 12345

    SECOC->>PQC: Sign([RPM=3000][FV=12345])
    Note over PQC: ML-DSA-65<br/>8.1ms
    PQC-->>SECOC: Signature (3,309 bytes)

    Note over SECOC: Build Secured PDU:<br/>[RPM][FV_trunc][Signature]

    SECOC->>GATEWAY: Secured PDU Ready
    GATEWAY->>CLOUD: Ethernet Tx<br/>TCP Port 12345<br/>3,380 bytes

    Note over CLOUD: Receive & Verify
    CLOUD->>CLOUD: Verify Signature
    Note over CLOUD: ML-DSA-65<br/>4.9ms

    alt Signature Valid
        CLOUD->>CLOUD: Store Data<br/>✅ Authentic
        Note over CLOUD: RPM=3000 from ENGINE<br/>Quantum-proof integrity
    else Signature Invalid
        CLOUD->>CLOUD: Reject Data<br/>❌ Attack!
        Note over CLOUD: Security Alert<br/>Possible tampering
    end
```

---

## How to Use These Diagrams

### In GitHub/GitLab
Simply paste the markdown file - Mermaid renders automatically.

### In VS Code
Install extension: "Markdown Preview Mermaid Support"

### Export to Images
Use Mermaid CLI:
```bash
npm install -g @mermaid-js/mermaid-cli
mmdc -i DIAGRAMS.md -o diagrams.pdf
```

### In LaTeX Reports
Convert to SVG/PNG and include:
```bash
mmdc -i diagram.mmd -o diagram.png
```

---

## Diagram Summary

| # | Diagram Name | Type | Purpose |
|---|-------------|------|---------|
| 1 | High-Level Architecture | Component | System overview |
| 2 | Dual-Platform | Deployment | Windows/Linux abstraction |
| 3 | Transmission Path | Sequence | Tx signal flow |
| 4 | Reception Path | Sequence | Rx signal flow |
| 5 | State Machine | State | SecOC states |
| 6 | PQC Integration | Component | Crypto modules |
| 7 | Ethernet Gateway | Topology | Vehicle network |
| 8 | Message Format | Structure | PDU layout |
| 9 | Buffer Fix | Comparison | Before/after fix |
| 10 | Performance | Comparison | Classical vs PQC |
| 11 | Security Flow | Flowchart | Attack defense |
| 12 | RPi Deployment | Deployment | Hardware stack |
| 13 | Build System | Flowchart | Build process |
| 14 | Test Flow | Flowchart | Testing process |
| 15 | Use Case | Sequence | Real-world example |

All diagrams are professional, scalable, and ready for academic presentations.
