# Pi 4 HPC Gateway Deployment Context

**Purpose:** This document is context for AI assistants deploying and extending this AUTOSAR gateway stack on a Raspberry Pi 4. It describes what is implemented, what works, and what remains to be done.

**Last updated:** 2026-04-08
**Branch:** `claude/autosar-gateway-pi-abstraction-646tf`
**Build command:** `cd Autosar_SecOC/build && cmake -G "Unix Makefiles" -DMCAL_TARGET=PI4 .. && make -j4`

---

## 1. Architecture Overview

The Pi 4 acts as a **High Performance Computer (HPC) / Central Gateway** in a zonal E/E architecture. It routes traffic between CAN and Ethernet domains, applies SecOC authentication (classic HMAC or post-quantum ML-DSA), and runs diagnostics.

```
 ┌─────────────────────────────────────────────────────────────┐
 │  APPLICATION LAYER                                          │
 │  Com, GUIInterface                                          │
 ├─────────────────────────────────────────────────────────────┤
 │  SERVICES LAYER                                             │
 │  SecOC, Csm, PQC, BswM, ComM, Dcm, Dem, Det, NvM, Os      │
 ├─────────────────────────────────────────────────────────────┤
 │  ECU ABSTRACTION LAYER                                      │
 │  PduR, CanIF, CanTP, SoAd, TcpIp, EthIf, EthSM, CanSM     │
 ├─────────────────────────────────────────────────────────────┤
 │  MCAL (Microcontroller Abstraction Layer) ← NEW             │
 │  Mcu_Pi4, Gpt_Pi4, Wdg_Pi4, Can_Pi4 (SocketCAN), Dio_Pi4  │
 └─────────────────────────────────────────────────────────────┘
         │              │              │
    SocketCAN       POSIX TCP/UDP    sysfs GPIO
    (can0/vcan0)    (eth0)           (/sys/class/gpio)
```

---

## 2. What Is Implemented (DONE)

### 2.1 MCAL Layer (source/Mcal/Pi4/)

| Module | File | What It Does | Hardware |
|--------|------|-------------|----------|
| **Mcu** | `Mcu_Pi4.c` | Reads CPU info from `/proc/cpuinfo`, RAM from `/proc/meminfo`, clock from sysfs. Init/DeInit/GetHwInfo/PerformReset. | /proc, sysfs |
| **Gpt** | `Gpt_Pi4.c` | 4-channel timer via Linux `timerfd_create()`. One-shot and continuous modes. Microsecond timestamps via `CLOCK_MONOTONIC`. | timerfd |
| **Wdg** | `Wdg_Pi4.c` | Hardware watchdog via `/dev/watchdog`. SLOW/FAST/OFF modes. Graceful fallback if device unavailable. Magic-close disable. | BCM2835 WDT |
| **Can** | `Can_Pi4.c` | Full AUTOSAR Can driver API backed by Linux SocketCAN (`AF_CAN`, `SOCK_RAW`). Non-blocking Rx polling. Auto-fallback to `vcan0` if `can0` unavailable. | MCP2515/USB-CAN |
| **Dio** | `Dio_Pi4.c` | GPIO via sysfs (`/sys/class/gpio`). Export/unexport, direction, read/write/flip. | BCM2711 GPIO |

**Status:** All compile and link. Can_Pi4 tested with vcan0. Others need on-hardware validation.

### 2.2 BSW Stack (Fully Implemented)

| Layer | Module | Status | Notes |
|-------|--------|--------|-------|
| **OS** | Os | Complete | Cooperative non-preemptive OSEK. Tasks, alarms, counters, resources, schedule tables. Uses `clock_gettime` directly (not Gpt yet). |
| **EcuM** | EcuM | Complete | Full startup/shutdown sequence. MCAL Phase 0 init integrated. NvM persistence. Gateway health tracking. |
| **BswM** | BswM | Complete | Mode management, gateway health profiles (NORMAL/DEGRADED/DIAG_ONLY), rule engine, fault counters. |
| **SecOC** | SecOC | Complete | AUTOSAR R21-11 compliant. Tx authentication, Rx verification, freshness management. Both IF and TP modes. |
| **PQC** | PQC, PQC_KeyExchange, PQC_KeyDerivation | Complete | ML-KEM-768 key exchange, ML-DSA-65 signatures, HKDF session key derivation. Uses liboqs. |
| **Csm** | Csm | Complete | Crypto Service Manager. Routes to HMAC (classic) or ML-DSA (PQC) based on config. |
| **CryIf** | CryIf | Complete | Crypto interface abstraction. |
| **PduR** | PduR_CanIf, PduR_CanTP, PduR_SoAd, PduR_Com, PduR_SecOC, PduR_UdpNm | Complete | Central PDU router. Routes between all bus interfaces and SecOC. |
| **Com** | Com | Complete | Signal-level communication. I-PDU groups, Tx/Rx main functions. |
| **CanIF** | CanIF | Complete | CAN Interface. Registers Tx/Rx callbacks with Can driver. |
| **CanTP** | CanTP | Complete | CAN Transport Protocol. Segmentation/reassembly for multi-frame messages. |
| **SoAd** | SoAd, SoAd_PQC | Complete | Socket Adapter. UDP/TCP connections via TcpIp. PQC key exchange integration. Routing groups. ApBridge state awareness. |
| **TcpIp** | TcpIp | Complete | Real POSIX sockets (TCP/UDP). `socket()`, `bind()`, `connect()`, `send()`, `recv()`. Non-blocking. |
| **EthIf** | EthIf | Complete | Ethernet Interface abstraction. Tx buffer pool, Rx/Tx callbacks. Wraps platform ethernet driver. |
| **EthSM** | EthSM | Complete | Ethernet State Manager. |
| **CanSM** | CanSM | Complete | CAN State Manager. |
| **CanNm** | CanNm | Complete | CAN Network Management. |
| **UdpNm** | UdpNm | Complete | UDP Network Management (AUTOSAR SWS compliant). |
| **ComM** | ComM | Complete | Communication Manager. FULL/SILENT/NO modes. |
| **Dcm** | Dcm | Complete | Diagnostic Communication Manager. |
| **Dem** | Dem | Complete | Diagnostic Event Manager. DTC storage, event status. |
| **Det** | Det | Complete | Default Error Tracer. Development error reporting. |
| **NvM** | NvM | Complete | Non-Volatile Memory manager. Block read/write, dataset support. |
| **MemIf** | MemIf | Complete | Memory interface abstraction. |
| **Fee** | Fee | Complete | Flash EEPROM Emulation. |
| **Ea** | Ea | Complete | EEPROM Abstraction. |
| **ApBridge** | ApBridge | Complete | Application bridge. Heartbeat monitoring, forced state, service health. |
| **Encrypt** | encrypt | Complete | OpenSSL-based HMAC. |
| **Ethernet** | ethernet.c | Complete (development) | Linux raw TCP sockets. Reads IP from file. Works but see section 3.3. |

### 2.3 Build System

- **CMake target selection:** `-DMCAL_TARGET=PI4` (default on Linux) or `-DMCAL_TARGET=SIM`
- **PI4 mode:** Swaps `Can.c` (simulation) for `Can_Pi4.c` (SocketCAN), adds all MCAL drivers
- **SIM mode:** Uses simulation CAN driver, excludes MCAL Pi4 files
- **Libraries built:** `SecOC` (executable), `SecOCLib` (static), `SecOCLibShared` (shared)
- **Dependencies:** OpenSSL, liboqs (in `external/liboqs/`), pthreads

### 2.4 Tests

| Test | What It Covers |
|------|---------------|
| `SecOCTests` | Integration: init, authenticate, verify round-trip |
| `AuthenticationTests` | MAC/signature generation |
| `VerificationTests` | MAC/signature verification |
| `FreshnessTests` | Freshness value management, replay prevention |
| `DirectTxTests` | Direct IF-mode transmission |
| `DirectRxTests` | Direct IF-mode reception |
| `startOfReceptionTests` | TP reception initiation |
| `PQC_ComparisonTests` | ML-KEM, ML-DSA, classic vs PQC comparison |
| `UdpNmTests` | UDP Network Management |
| `Phase3_Complete_Test` | End-to-end Ethernet gateway + PQC |

---

## 3. What Needs To Be Done (NEXT STEPS)

### 3.1 Critical for First Boot on Pi 4

| Task | Priority | Details |
|------|----------|---------|
| **Set up SocketCAN hardware** | P0 | Connect MCP2515 SPI-to-CAN module. Load kernel modules: `mcp251x`, `can`, `can_raw`. Configure: `sudo ip link set can0 up type can bitrate 500000`. The Can_Pi4.c driver is ready; it just needs a live interface. |
| **Build liboqs for ARM** | P0 | Current liboqs is built for x86_64. On the Pi: `cd external/liboqs/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j4`. Or cross-compile with `aarch64-linux-gnu-gcc`. |
| **Cross-compilation toolchain** | P0 | Add CMake toolchain file for `aarch64-linux-gnu-gcc` so you can build on x86 desktop for Pi 4 ARM target. Currently only native compilation is supported. |
| **Validate /dev/watchdog** | P1 | Enable in kernel: `dtparam=watchdog=on` in `/boot/config.txt`. Verify `Wdg_Pi4.c` opens the device. The driver already has software-only fallback if unavailable. |
| **Test end-to-end CAN-to-Ethernet routing** | P1 | Send CAN frame on `can0` -> verify it routes through CanIF -> PduR -> SecOC -> SoAd -> TcpIp -> out on Ethernet. This is the core gateway function. |

### 3.2 Os Timer Integration with MCAL Gpt

**Current state:** The Os module (`Os.c`) uses `clock_gettime(CLOCK_MONOTONIC)` directly in `Os_GetSystemTimeMs()`. The MCAL Gpt driver exists but is **not yet wired into the Os module**.

**What to do:**
- Option A (simple): Leave as-is. `clock_gettime` works fine on Pi 4 and is precise.
- Option B (AUTOSAR-pure): Replace `Os_GetSystemTimeMs()` internals to call `Gpt_GetTimestampUs() / 1000`. This adds proper MCAL layering but no functional difference on Linux.

**Recommendation:** Option A for now. The Gpt driver is useful for application-level timers, not necessarily the OS tick source on Linux.

### 3.3 Ethernet Driver Hardening

**Current state:** `source/Ethernet/ethernet.c` uses raw TCP sockets with IP address read from a text file (`./source/Ethernet/ip_address.txt`). It creates a new socket per send/receive call.

**What to do:**
- Persistent socket connections instead of connect-per-send
- Configurable port numbers (currently hardcoded)
- UDP support for low-latency gateway traffic
- Error recovery and reconnection logic
- Consider replacing with the EthIf/TcpIp/SoAd path entirely (which already uses proper POSIX sockets)

### 3.4 SPI Driver for MCP2515

**Current state:** No SPI MCAL driver. The MCP2515 CAN controller is accessed via the kernel's `mcp251x` driver, which exposes a SocketCAN interface. So our `Can_Pi4.c` works without a userspace SPI driver.

**What to do if you need raw SPI access** (e.g., for a second SPI peripheral):
- Add `source/Mcal/Pi4/Spi_Pi4.c` using Linux `spidev` (`/dev/spidev0.0`)
- Add `include/Mcal/Spi.h` with AUTOSAR Spi driver API
- This is NOT needed for CAN — the kernel handles SPI-to-CAN internally

### 3.5 Multi-Device Distribution

**Current state:** The stack runs on a single Pi 4. The code is structured to support distributing to other devices but no multi-node communication is implemented yet.

**What to do:**
- Define node roles (HPC, Zone Controller, Sensor Node)
- Implement SOME/IP or a lightweight service discovery mechanism
- Use SoAd_PQC for secure inter-node key exchange over Ethernet
- Add per-node configuration profiles (separate `SecOC_PBcfg.c` per node)
- Consider adding a second MCAL target (e.g., `MCAL_TARGET_PI_ZERO` or `MCAL_TARGET_STM32`)

### 3.6 Production Hardening

| Area | Current State | What To Do |
|------|--------------|------------|
| **Logging** | `printf` statements | Replace with structured logging (syslog or journald) |
| **Error handling** | DET reports to Det module | Add persistent error log to NvM, watchdog escalation |
| **Security** | PQC keys in memory | Add secure key storage (consider TPM or /dev/urandom seeding) |
| **Startup** | Manual `./SecOC` execution | Create systemd service file for auto-start |
| **Config** | Compile-time `#define` | Move critical params to runtime JSON/YAML config |
| **Performance** | Not profiled on ARM | Benchmark PQC ops on Cortex-A72 (ML-DSA sign ~10x slower than x86) |

### 3.7 ML-KEM Key Management (KeyM) — Detailed Status

The ML-KEM key management is the most feature-complete area of the PQC implementation.
Below is an audit of what exists and what is still needed for Pi 4 deployment.

#### What's DONE

| Component | File | What It Does |
|-----------|------|-------------|
| **ML-KEM-768 KeyGen/Encaps/Decaps** | `source/PQC/PQC.c` | Full liboqs wrapper. Calls `OQS_KEM_keypair()`, `OQS_KEM_encaps()`, `OQS_KEM_decaps()`. Key sizes validated against constants. |
| **3-Way Key Exchange Protocol** | `source/PQC/PQC_KeyExchange.c` | Session manager for up to 8 peers. State machine: IDLE → INITIATED → ESTABLISHED (or FAILED). Alice/Bob roles. Secret key zeroized after decapsulation (line 211). |
| **HKDF Session Key Derivation** | `source/PQC/PQC_KeyDerivation.c` | Derives AES-256-GCM encryption key + HMAC-SHA256 auth key from ML-KEM shared secret. Uses SHA-256 from liboqs. Up to 16 session key slots. |
| **Csm Key Exchange API** | `source/Csm/Csm.c` | Full API: `Csm_KeyExchangeInitiate/Respond/Complete/Reset/GetSharedSecret`, `Csm_DeriveSessionKeys`, `Csm_GetSessionKeys`, `Csm_ClearSessionKeys`. Routes through CryIf to PQC layer. |
| **CryIf Routing** | `source/CryIf/CryIf.c` | Routes Csm key exchange calls to PQC_KeyExchange functions. |
| **SoAd PQC Integration** | `source/SoAd/SoAd_PQC.c` | ML-KEM key exchange over Ethernet. Control message handling for in-band negotiation. Main function for periodic processing. |
| **ML-DSA-65 Key File I/O** | `source/PQC/PQC.c` | `PQC_MLDSA_SaveKeys()` / `PQC_MLDSA_LoadKeys()` — writes `.pub` and `.key` binary files. |

**Key exchange flow (working end-to-end):**
```
Alice (HPC Pi4)                          Bob (Peer ECU)
      │                                        │
      │──── PQC_KeyExchange_Initiate() ────────>│  (sends 1184-byte public key)
      │                                        │
      │<──── PQC_KeyExchange_Respond() ─────────│  (sends 1088-byte ciphertext)
      │                                        │
      │──── PQC_KeyExchange_Complete() ────────>│  (decapsulates → 32-byte shared secret)
      │                                        │
      │──── PQC_DeriveSessionKeys() ───────────>│  (HKDF → AES-256 + HMAC-SHA256 keys)
      │                                        │
      ├─── Session ESTABLISHED ─────────────────┤
```

#### What's MISSING — Gaps for Pi 4 Deployment

| Gap | Priority | Details | Suggested Fix |
|-----|----------|---------|---------------|
| **HKDF uses simplified extract** | P0 | `PQC_KeyDerivation.c:47` uses `SHA-256(salt \|\| IKM)` instead of proper `HMAC-SHA256(salt, IKM)`. Not a true RFC 5869 HKDF-Extract. | Replace with `HMAC-SHA256` using OpenSSL `HMAC()` or liboqs. ~2 hours. |
| **Key file paths are CWD-relative** | P0 | `PQC_MLDSA_SaveKeys()` writes to `%s.pub` in current directory. Breaks if service launched from different directory. | Use absolute path like `/etc/secoc/keys/` or make configurable. ~1 hour. |
| **No automatic key rotation** | P1 | Session keys stay until manual `PQC_KeyExchange_Reset()`. No timer-based rekeying. NIST recommends rekeying periodically. | Add rekeying counter or timer in `SoAd_PQC_MainFunction()`. Trigger re-exchange after N messages or T seconds. ~4 hours. |
| **Secret keys in plaintext RAM** | P1 | `PQC_Sessions[].SharedSecret` and `LocalKeyPair.SecretKey` sit in static memory with no protection. | On Pi 4: use `mlock()` to prevent swapping. Consider `sodium_mprotect_noaccess()` pattern when keys not in use. ~3 hours. |
| **No persistent ML-KEM key storage** | P1 | ML-KEM keys are ephemeral (regenerated per session). If the gateway restarts, all sessions must re-negotiate. | This is by design (forward secrecy). For faster restart: cache session keys in NvM with expiry timestamp. ~6 hours. |
| **No trust anchor / certificate chain** | P2 | Keys exchanged in-band via SoAd_PQC. No pre-shared trust anchors, no PKI, no certificate verification. Vulnerable to MITM on first exchange. | Add pre-provisioned peer public key list. Or implement a simple certificate format with ML-DSA signatures. ~8-16 hours. |
| **No formal AUTOSAR KeyM module** | P2 | Key management is split across PQC/Csm/CryIf. No `KeyM_Init()`, no key slot abstraction, no AUTOSAR SWS_KeyManager API. Functionally complete but not spec-compliant. | Wrap existing functions in a `source/KeyM/KeyM.c` module with AUTOSAR API. ~4 hours. |
| **No key usage counters** | P3 | No tracking of how many sign/verify operations used a key. | Add counter in `Csm_SessionKeys[]` struct. ~1 hour. |
| **No key export restrictions** | P3 | `PQC_KeyExchange_GetSharedSecret()` freely returns raw shared secret bytes to any caller. | Add access control or restrict to Csm layer only. ~2 hours. |

#### Configuration Reference (ML-KEM related)

| Parameter | File | Value | Meaning |
|-----------|------|-------|---------|
| `PQC_MLKEM_PUBLIC_KEY_BYTES` | `PQC.h` | 1184 | ML-KEM-768 public key size |
| `PQC_MLKEM_SECRET_KEY_BYTES` | `PQC.h` | 2400 | ML-KEM-768 secret key size |
| `PQC_MLKEM_CIPHERTEXT_BYTES` | `PQC.h` | 1088 | ML-KEM-768 ciphertext size |
| `PQC_MLKEM_SHARED_SECRET_BYTES` | `PQC.h` | 32 | Shared secret (256 bits) |
| `PQC_MAX_PEERS` | `PQC_KeyExchange.h` | 8 | Max concurrent key exchange peers |
| `PQC_SESSION_KEYS_MAX` | `PQC_KeyDerivation.h` | 16 | Max derived session key slots |
| `PQC_DERIVED_KEY_LENGTH` | `PQC_KeyDerivation.h` | 32 | AES-256 key length (bytes) |
| `CSM_MAX_PEERS` | `Csm.h` | 8 | Csm peer session limit |
| `SOAD_PQC_MAX_PEERS` | `SoAd_PQC.h` | 8 | SoAd PQC session limit |

### 3.8 Other Missing AUTOSAR Features

These SWS features are defined in the AUTOSAR spec but not yet implemented:

- **RTE (Runtime Environment)** — no auto-generated RTE layer; direct function calls used
- **SchM (Schedule Manager)** — BSW scheduling is done via Os alarms, not a formal SchM
- **FrIf / FlexRay** — FlexRay interface headers exist but no driver implementation
- **Lin / LIN stack** — not implemented (not typical for HPC gateway)
- **J1939** — not implemented
- **DoIP (Diagnostics over IP)** — not implemented, would extend Dcm over Ethernet

---

## 4. File Map for AI Reference

```
Autosar_SecOC/
├── source/
│   ├── Mcal/Pi4/           ← MCAL: Pi 4 hardware drivers
│   │   ├── Mcu_Pi4.c       ← System info, clock, reset
│   │   ├── Gpt_Pi4.c       ← timerfd timers
│   │   ├── Wdg_Pi4.c       ← /dev/watchdog
│   │   ├── Can_Pi4.c       ← SocketCAN (AF_CAN raw sockets)
│   │   └── Dio_Pi4.c       ← GPIO sysfs
│   ├── SecOC/              ← Core SecOC module
│   ├── PQC/                ← Post-Quantum Crypto wrappers
│   ├── Csm/                ← Crypto Service Manager
│   ├── PduR/               ← PDU Router (6 interface files)
│   ├── Can/                ← Can.c (sim), CanIF.c, CanTP.c
│   ├── SoAd/               ← Socket Adapter + PQC integration
│   ├── TcpIp/              ← POSIX socket wrapper
│   ├── EthIf/              ← Ethernet Interface
│   ├── Ethernet/           ← Platform ethernet (Linux/Windows)
│   ├── Os/                 ← OSEK cooperative OS
│   ├── EcuM/               ← ECU Manager (startup/shutdown)
│   ├── BswM/               ← Mode Manager (gateway profiles)
│   ├── Com/                ← Communication module
│   ├── ComM/               ← Communication Manager
│   ├── CanSM/              ← CAN State Manager
│   ├── CanNm/              ← CAN Network Management
│   ├── EthSM/              ← Ethernet State Manager
│   ├── UdpNm/              ← UDP Network Management
│   ├── Dcm/                ← Diagnostic Communication
│   ├── Dem/                ← Diagnostic Event Manager
│   ├── Det/                ← Default Error Tracer
│   ├── NvM/                ← Non-Volatile Memory
│   ├── MemIf/Fee/Ea/       ← Memory stack
│   ├── ApBridge/           ← Application bridge (heartbeat)
│   ├── CryIf/              ← Crypto interface
│   ├── Encrypt/            ← OpenSSL HMAC
│   ├── GUIInterface/       ← GUI bridge (Python interop)
│   └── main.c              ← Entry point
├── include/                ← Headers (mirrors source structure)
│   └── Mcal/               ← MCAL headers (Mcal_Cfg.h, Mcu.h, etc.)
├── test/                   ← Google Test + C test files
├── external/liboqs/        ← Open Quantum Safe library
├── GUI/                    ← Python PySide2 GUI
├── CMakeLists.txt          ← Build config (MCAL_TARGET selection)
└── docs/                   ← Architecture, compliance, deployment docs
```

---

## 5. Quick Commands for Pi 4 Deployment

```bash
# 1. Build on Pi 4 (native)
cd Autosar_SecOC
mkdir -p build && cd build
cmake -G "Unix Makefiles" -DMCAL_TARGET=PI4 ..
make -j4

# 2. Set up virtual CAN (for testing without hardware)
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link set up vcan0

# 3. Set up real CAN (MCP2515 via SPI)
# Add to /boot/config.txt:
#   dtoverlay=mcp2515-can0,oscillator=8000000,interrupt=25
sudo ip link set can0 up type can bitrate 500000

# 4. Enable hardware watchdog
# Add to /boot/config.txt:
#   dtparam=watchdog=on
sudo modprobe bcm2835_wdt

# 5. Run the gateway
./SecOC

# 6. Monitor CAN traffic
candump can0

# 7. Send test CAN frame
cansend can0 123#DEADBEEF

# 8. Run tests
ctest --timeout 30
```

---

## 6. Configuration Switches

| Switch | File | Values | Effect |
|--------|------|--------|--------|
| `MCAL_TARGET` | CMake / `Mcal_Cfg.h` | `PI4`, `SIM` | Selects MCAL driver set |
| `SECOC_USE_PQC_MODE` | `SecOC_PQC_Cfg.h` | `TRUE`, `FALSE` | PQC (ML-DSA) vs classic (HMAC) |
| `SECOC_NUM_OF_TX_PDU_PROCESSING` | `SecOC_Cfg.h` | integer | Number of Tx PDU channels |
| `SECOC_NUM_OF_RX_PDU_PROCESSING` | `SecOC_Cfg.h` | integer | Number of Rx PDU channels |
| `MCAL_CAN_INTERFACE_NAME` | `Mcal_Cfg.h` | string | SocketCAN interface name |
| `MCAL_WDG_TIMEOUT_SEC` | `Mcal_Cfg.h` | integer | Watchdog timeout (seconds) |
| `MCAL_DIO_MAX_CHANNELS` | `Mcal_Cfg.h` | integer | Max GPIO pins |
| `OS_TICK_DURATION_MS` | `Os_Cfg.h` | integer | OS tick period |
| `OS_MAX_TASKS` | `Os_Cfg.h` | integer | Max concurrent OS tasks |
