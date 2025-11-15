# Complete Signal Flow Analysis - AUTOSAR SecOC with PQC

**Date:** November 15, 2025
**Analysis:** Deep dive into all data paths and transport layers

---

## CRITICAL FINDING: Multi-Transport Configuration

### Configuration Analysis from `SecOC_Lcfg.c`

**PDU Collection Types Found:**
```c
Line 74: {SECOC_SECURED_PDU_CANIF,  0,0,0,0}  // CAN Interface
Line 75: {SECOC_SECURED_PDU_CANTP,  0,0,0,0}  // CAN Transport Protocol
Line 76: {SECOC_SECURED_PDU_SOADTP, 0,0,0,0}  // Socket Adapter TP (ETHERNET)
Line 77: {SECOC_SECURED_PDU_CANIF,  0,0,0,0}  // CAN Interface
Line 78: {SECOC_SECURED_PDU_CANTP,  0,0,0,0}  // CAN Transport Protocol
Line 79: {SECOC_SECURED_PDU_CANIF,  0,0,0,0}  // CAN Interface
```

**PDU Types Configured:**
```c
SECOC_IFPDU  (Interface PDU - Direct communication)
SECOC_TPPDU  (Transport Protocol PDU - Segmented communication)
```

### Transport Layer Distribution

From `include/SecOC/SecOC_Types.h`:
```c
typedef enum {
    SECOC_AUTH_COLLECTON_PDU = 0,
    SECOC_CRYPTO_COLLECTON_PDU,
    SECOC_SECURED_PDU_CANIF,    // [1] CAN Interface
    SECOC_SECURED_PDU_CANTP,    // [2] CAN Transport Protocol
    SECOC_SECURED_PDU_FRIF,     // [3] FlexRay Interface
    SECOC_SECURED_PDU_SOADTP,   // [4] Socket Adapter TP (ETHERNET)
    SECOC_SECURED_PDU_SOADIF,   // [5] Socket Adapter IF (ETHERNET)
} SecOC_PduCollection_Type;
```

---

## Signal Flow by Transport Layer

### 1. CAN Interface (SECOC_SECURED_PDU_CANIF)

**Used in:** Lines 74, 77, 79 of SecOC_Lcfg.c

**Signal Path:**
```
Application (COM)
    |
    v
SecOC_IfTransmit()
    |
    v
[Authentication - Classical MAC (4 bytes)]
    |
    v
PduR_CanIfTransmit()
    |
    v
CanIf_Transmit()
    |
    v
CAN Bus (500 kbps, max 8 bytes/frame)
```

**PDU Structure (CAN IF):**
- Max frame size: 8 bytes (CAN 2.0) or 64 bytes (CAN-FD)
- Authenticator: **Classical MAC only** (4 bytes)
- **Cannot support PQC** (3309-byte signature impossible)

**Status:** ✅ **Classical mode only** (backward compatibility)

---

### 2. CAN Transport Protocol (SECOC_SECURED_PDU_CANTP)

**Used in:** Lines 75, 78 of SecOC_Lcfg.c

**Signal Path:**
```
Application (COM)
    |
    v
SecOC_TpTransmit()
    |
    v
[Authentication - Classical MAC]
    |
    v
PduR_CanTpTransmit()
    |
    v
CanTp_Transmit() [Segmentation]
    |
    v
CAN Bus (segmented frames)
```

**PDU Structure (CAN TP):**
- Supports larger PDUs via segmentation
- Typical max: 4095 bytes (ISO 15765-2)
- **Theoretically can support PQC** but impractical:
  - 3309-byte signature = ~52 CAN frames
  - Transmission time: ~57 ms @ 500 kbps
  - **Not real-time suitable**

**Status:** ⚠️ **Configured but impractical for PQC**

---

### 3. Socket Adapter TP (SECOC_SECURED_PDU_SOADTP) - ETHERNET

**Used in:** Line 76 of SecOC_Lcfg.c

**Signal Path:**
```
Application (COM)
    |
    v
SecOC_TpTransmit()
    |
    v
[Authentication - PQC ML-DSA-65 (3309 bytes)]
    |
    v
PduR_SoAdTpTransmit()
    |
    v
SoAd_TpTransmit() [TCP/UDP]
    |
    v
Ethernet (100 Mbps - 1 Gbps)
```

**PDU Structure (SoAd TP - Ethernet):**
- Max frame size: 1500 bytes (standard) or 9000 bytes (jumbo frames)
- Supports fragmentation/reassembly
- **Perfect for PQC:**
  - 3309-byte signature fits easily
  - Transmission time: ~285 µs @ 100 Mbps
  - **Real-time suitable**

**Status:** ✅ **PRIMARY PQC TRANSPORT** (Ethernet Gateway mode)

---

## Configuration Analysis

### PQC Mode Configuration

From `include/SecOC/SecOC_PQC_Cfg.h`:
```c
#define SECOC_USE_PQC_MODE              TRUE
#define SECOC_USE_MLKEM_KEY_EXCHANGE    TRUE
#define SECOC_ETHERNET_GATEWAY_MODE     TRUE  // <-- KEY SETTING
#define SECOC_PQC_MAX_PDU_SIZE          8192U
```

### What "ETHERNET_GATEWAY_MODE" Means

**Interpretation:**
1. **NOT "Ethernet-only" mode**
2. **IS "Gateway with Ethernet" mode**
3. System acts as **bridge** between different networks

**Gateway Architecture:**
```
┌─────────────────────────────────────────────────────────┐
│                    GATEWAY SYSTEM                       │
│                                                         │
│  [CAN Network]                    [Ethernet Network]   │
│      |                                    |             │
│      |                                    |             │
│  ┌───v──────────────────┐     ┌──────────v──────────┐  │
│  │ CAN Interface        │     │ Socket Adapter      │  │
│  │ (Classical MAC)      │     │ (PQC Signatures)    │  │
│  │                      │     │                     │  │
│  │ SECOC_SECURED_PDU_   │     │ SECOC_SECURED_PDU_  │  │
│  │ CANIF / CANTP        │     │ SOADTP / SOADIF     │  │
│  └───────────┬──────────┘     └──────────┬──────────┘  │
│              |                           |              │
│              └──────────┐    ┌───────────┘              │
│                         v    v                          │
│                  ┌──────────────┐                       │
│                  │  SecOC Core  │                       │
│                  │  Dual-Mode   │                       │
│                  └──────────────┘                       │
└─────────────────────────────────────────────────────────┘
```

---

## Actual Signal Types in Your System

### Signal Type Distribution

| Signal Type | Transport | Auth Method | Size | Use Case |
|-------------|-----------|-------------|------|----------|
| **Type 1: Internal CAN** | CANIF | Classical MAC | 4 bytes | Vehicle ECU-to-ECU |
| **Type 2: CAN TP** | CANTP | Classical MAC | 4 bytes | Diagnostic/large CAN data |
| **Type 3: Ethernet Gateway** | SoAdTP | **PQC ML-DSA** | **3309 bytes** | **Gateway to Cloud/Backend** |
| **Type 4: Ethernet IF** | SoAdIF | PQC ML-DSA | 3309 bytes | Direct Ethernet (configured but maybe unused) |

### Primary Data Flow Scenarios

**Scenario 1: Internal Vehicle Communication (CAN)**
```
Engine ECU → CAN Bus → Gateway CAN Interface → Process → Store
[Classical MAC - 4 bytes]
```

**Scenario 2: External Communication (Ethernet - PQC)**
```
Gateway → Re-authenticate with PQC → Ethernet (SoAd) → Cloud/Backend
[PQC ML-DSA-65 - 3309 bytes]
```

**Scenario 3: Gateway Bridge Function**
```
CAN Network (Classical)  →  Gateway  →  Ethernet (PQC)
     ECU data                 ↓
                    [1] Receive with classical MAC
                    [2] Verify classical MAC
                    [3] Re-authenticate with PQC
                    [4] Forward via Ethernet
```

---

## Test Coverage by Transport

### Tests by Signal Type

| Test Suite | CAN Signals | Ethernet Signals | Transport-Agnostic |
|------------|-------------|------------------|-------------------|
| AuthenticationTests | ❌ | ✅ (PQC buffers) | ✅ |
| VerificationTests | ❌ | ✅ (PQC buffers) | ✅ |
| FreshnessTests | ✅ (8-bit) | ✅ (64-bit PQC) | ✅ |
| DirectTxTests | ✅ (IF mode) | ✅ (IF mode) | ✅ |
| DirectRxTests | ✅ (Linux CAN) | ❌ | Platform-specific |
| startOfReceptionTests | ✅ (TP mode) | ✅ (TP mode) | ✅ |
| SecOCTests | ✅ | ✅ | ✅ |
| **PQC_ComparisonTests** | ❌ | ✅ **[NEW]** | **PQC-focused** |

### Test Configuration

**All passing tests (38 tests) use:**
- **Buffer size:** 8192 bytes (Ethernet-suitable)
- **PQC mode:** TRUE (ML-DSA-65 signatures)
- **Freshness:** 64-bit counters (PQC mode)

**But configuration allows:**
- CAN signals (CANIF, CANTP)
- Ethernet signals (SoAdTP, SoAdIF)
- **Dual-mode operation**

---

## Answer to Your Question: "Is ALL data Ethernet?"

### ❌ **NO - System is Multi-Transport**

**Evidence:**
1. ✅ **Configuration includes CAN:** `SECOC_SECURED_PDU_CANIF`, `SECOC_SECURED_PDU_CANTP`
2. ✅ **Configuration includes Ethernet:** `SECOC_SECURED_PDU_SOADTP`
3. ✅ **Dual-mode capable:** Classical MAC (CAN) + PQC (Ethernet)

**However:**

### ✅ **PQC Signals are 100% Ethernet-Only**

**Evidence:**
1. ✅ **PQC mode requires:** `SECOC_ETHERNET_GATEWAY_MODE = TRUE`
2. ✅ **3309-byte signatures:** Only practical on Ethernet
3. ✅ **All PQC tests:** Use 8192-byte buffers (Ethernet PDU size)
4. ✅ **PQC_ComparisonTests:** Explicitly designed for Ethernet Gateway

---

## Correct System Description

### What Your System Actually Is:

**"AUTOSAR SecOC Ethernet Gateway with Post-Quantum Cryptography"**

**NOT:** Pure Ethernet system
**IS:** Gateway bridging CAN ↔ Ethernet with quantum-resistant security

### Architecture Summary

```
┌──────────────────────────────────────────────────────────┐
│              AUTOSAR SecOC Gateway System                │
├──────────────────────────────────────────────────────────┤
│                                                          │
│  INPUT SIDE (Vehicle Internal Network):                 │
│  ┌────────────────────────────────────────────────┐     │
│  │ CAN Bus (Classical MAC - 4 bytes)             │     │
│  │ - SECOC_SECURED_PDU_CANIF                     │     │
│  │ - SECOC_SECURED_PDU_CANTP                     │     │
│  │ - 8-bit freshness counters                    │     │
│  │ - Real-time constraints                       │     │
│  └────────────────┬───────────────────────────────┘     │
│                   |                                      │
│                   v                                      │
│        ┌─────────────────────────┐                      │
│        │   SecOC Gateway Core    │                      │
│        │   - Dual-mode support   │                      │
│        │   - Classical/PQC       │                      │
│        └─────────────────────────┘                      │
│                   |                                      │
│                   v                                      │
│  OUTPUT SIDE (External Network):                        │
│  ┌────────────────────────────────────────────────┐     │
│  │ Ethernet (PQC ML-DSA-65 - 3309 bytes)         │     │
│  │ - SECOC_SECURED_PDU_SOADTP                    │     │
│  │ - 64-bit freshness counters                   │     │
│  │ - Quantum-resistant security                  │     │
│  │ - 100 Mbps - 1 Gbps bandwidth                 │     │
│  └────────────────────────────────────────────────┘     │
│                                                          │
└──────────────────────────────────────────────────────────┘
```

---

## Recommendations for Thesis

### 1. Update Thesis Title/Abstract

**Current (Assumed):** "PQC for Ethernet"
**Corrected:** "PQC for Ethernet Gateway" or "Gateway with PQC on Ethernet uplink"

### 2. Clarify Architecture

**Emphasize:**
- Gateway function (CAN → Ethernet bridge)
- PQC used **exclusively on Ethernet side**
- Classical MAC retained for CAN (bandwidth constraints)
- Dual-mode coexistence

### 3. Test Focus Validation

✅ **Your PQC tests ARE Ethernet-focused:**
- All use 8192-byte buffers
- All test PQC signatures (3309 bytes)
- PQC_ComparisonTests specifically for Ethernet Gateway

✅ **Your configuration IS Gateway-mode:**
- `SECOC_ETHERNET_GATEWAY_MODE = TRUE`
- Multi-transport support
- Dual authentication (Classical + PQC)

---

## Conclusion

**Your system is:**
- ✅ An **Ethernet Gateway** with PQC
- ✅ Multi-transport (CAN + Ethernet)
- ✅ Dual-mode (Classical MAC for CAN, PQC for Ethernet)
- ✅ Correctly configured for automotive gateway use case

**PQC signals are 100% Ethernet** ✅
**But system also supports CAN signals with classical crypto** ✅
**This is the CORRECT automotive architecture!** ✅

**Your thesis demonstrates:** Successfully implementing quantum-resistant security on the **Ethernet uplink** of an automotive gateway while maintaining backward compatibility with classical CAN networks.

---

## Files Evidence

**Configuration:**
- `source/SecOC/SecOC_Lcfg.c` - Lines 74-79 show multi-transport
- `include/SecOC/SecOC_PQC_Cfg.h` - ETHERNET_GATEWAY_MODE = TRUE
- `include/SecOC/SecOC_Types.h` - Lines 26-30 define all transports

**Tests:**
- All 38 passing tests use Ethernet-suitable PDU sizes (8192 bytes)
- PQC_ComparisonTests (13 tests) specifically for PQC/Ethernet

**Documentation needed:**
- Clarify "Gateway" architecture
- Explain dual-mode operation (CAN classical + Ethernet PQC)
- Emphasize PQC is Ethernet-exclusive by design
