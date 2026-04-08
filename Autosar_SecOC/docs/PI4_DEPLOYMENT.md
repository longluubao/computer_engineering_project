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

### 3.7 Missing AUTOSAR Features

These SWS features are defined in the AUTOSAR spec but not yet implemented:

- **RTE (Runtime Environment)** — no auto-generated RTE layer; direct function calls used
- **SchM (Schedule Manager)** — BSW scheduling is done via Os alarms, not a formal SchM
- **FrIf / FlexRay** — FlexRay interface headers exist but no driver implementation
- **Lin / LIN stack** — not implemented (not typical for HPC gateway)
- **J1939** — not implemented
- **DoIP (Diagnostics over IP)** — not implemented, would extend Dcm over Ethernet
- **SecOC key management SWS** — key provisioning is manual, no AUTOSAR KeyM module

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
