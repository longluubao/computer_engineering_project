# Tài Liệu Bàn Giao Phần Cứng — AUTOSAR SecOC + PQC

> **Ngày:** 14/04/2026  
> **Mục đích:** Hướng dẫn kỹ sư phần cứng triển khai hệ thống SecOC trên nền tảng Raspberry Pi 4 + ESP32  
> **Trạng thái:** Phần mềm SecOC/PQC đã hoàn thiện — cần tích hợp phần cứng

---

## 1. Tổng Quan Hệ Thống

```
┌──────────────┐  Ethernet (PQC, ML-DSA-65)    ┌──────────────┐  CAN 500k  ┌─────────┐
│  Pi4 #1      │ ◄───────────────────────────► │  Pi4 #2      │ ◄─────────► │ ESP32-1 │
│  Zone Ctrl   │   Chữ ký 3,309 byte           │  HPC Gateway │ ◄──────────│ ESP32-2 │
│  (subscriber)│                               │              │ ◄─────────│ ESP32-N │
└──────────────┘                               └──────────────┘            └─────────┘
```

| Thiết bị | Vai trò | Giao tiếp |
|----------|---------|-----------|
| Raspberry Pi 4 #1 | Zone Controller (ZC) — nhận và xác thực PDU | Ethernet với Pi4 #2 |
| Raspberry Pi 4 #2 | HPC Gateway — nhận CAN từ ECU, ký PQC, chuyển tiếp | Ethernet với Pi4 #1 + CAN với ESP32 |
| ESP32 × N | ECU giả lập (động cơ, phanh, v.v.) | CAN bus với Pi4 #2 |

---

## 2. Danh Sách Linh Kiện (BOM)

| # | Linh kiện | Số lượng | Ghi chú |
|---|-----------|----------|---------|
| 1 | Raspberry Pi 4 (4GB+) | 2 | Cài Raspberry Pi OS Lite 64-bit |
| 2 | Thẻ microSD 32GB+ | 2 | Class 10 trở lên |
| 3 | Module MCP2515 CAN | 1 | Gắn trên Pi4 #2 qua SPI |
| 4 | CAN Transceiver SN65HVD230 | N | Mỗi ESP32 cần 1 module |
| 5 | ESP32-WROOM-32 | N (tối thiểu 2) | Dùng TWAI controller tích hợp |
| 6 | Cáp Ethernet RJ45 | 1 | Nối trực tiếp Pi4 #1 ↔ Pi4 #2 |
| 7 | Dây jumper, breadboard | — | Kết nối CAN bus |
| 8 | Nguồn 5V/3A cho Pi4 | 2 | USB-C |
| 9 | Điện trở terminal 120Ω | 2 | Đầu và cuối CAN bus |

---

## 3. Sơ Đồ Kết Nối

### 3.1 Ethernet (Pi4 #1 ↔ Pi4 #2)

- Nối trực tiếp cáp RJ45 giữa 2 Pi
- IP tĩnh:
  - Pi4 #1 (ZC): `10.0.0.10`
  - Pi4 #2 (HPC): `10.0.0.20`

### 3.2 CAN Bus (Pi4 #2 ↔ ESP32)

**Pi4 #2 — Module MCP2515 (SPI):**

| MCP2515 Pin | Pi4 GPIO |
|-------------|----------|
| VCC | 3.3V |
| GND | GND |
| CS | GPIO 8 (SPI0 CE0) |
| SO | GPIO 9 (SPI0 MISO) |
| SI | GPIO 10 (SPI0 MOSI) |
| SCK | GPIO 11 (SPI0 SCLK) |
| INT | GPIO 25 |

**ESP32 — SN65HVD230:**

| ESP32 Pin | SN65HVD230 Pin |
|-----------|----------------|
| GPIO 21 (CAN TX) | D (Driver Input) |
| GPIO 22 (CAN RX) | R (Receiver Output) |
| 3.3V | VCC |
| GND | GND |

**CAN Bus:**
- CAN_H và CAN_L nối song song giữa MCP2515 và tất cả SN65HVD230
- Điện trở 120Ω ở 2 đầu bus
- Tốc độ: **500 kbit/s**

---

## 4. Cấu Hình Pi4

### 4.1 Cài đặt cơ bản (cả 2 Pi)

```bash
# Cập nhật hệ thống
sudo apt update && sudo apt upgrade -y

# Cài đặt công cụ build
sudo apt install -y build-essential cmake git can-utils net-tools

# Clone repo
git clone <repo-url> /home/pi/secoc
cd /home/pi/secoc/Autosar_SecOC

# Build liboqs (thư viện PQC)
bash build_liboqs.sh

# Build SecOC
mkdir -p build && cd build
cmake -G "Unix Makefiles" ..
make -j4
```

### 4.2 Cấu hình MCP2515 trên Pi4 #2 (HPC)

Thêm vào `/boot/config.txt`:
```
dtparam=spi=on
dtoverlay=mcp2515-can0,oscillator=8000000,interrupt=25
```

Khởi động CAN interface:
```bash
sudo ip link set can0 up type can bitrate 500000
# Kiểm tra:
candump can0
```

### 4.3 Cấu hình IP tĩnh

Tạo file `/etc/secoc/<role>.env`:

**Pi4 #1 (Zone Controller):**
```
SECOC_ROLE=ZC
SECOC_PEER_IP=10.0.0.20
SECOC_PORT=12345
SECOC_KEY_DIR=/etc/secoc/keys
SECOC_USE_PQC_MODE=TRUE
```

**Pi4 #2 (HPC Gateway):**
```
SECOC_ROLE=HPC
SECOC_PEER_IP=10.0.0.10
SECOC_PORT=12345
SECOC_CAN_IF=can0
SECOC_KEY_DIR=/etc/secoc/keys
SECOC_USE_PQC_MODE=TRUE
```

### 4.4 Tạo và phân phối khóa PQC

```bash
# Trên mỗi Pi, tạo cặp khóa ML-DSA-65:
mkdir -p /etc/secoc/keys
cd /home/pi/secoc/Autosar_SecOC/build
./bin/secoc_keygen   # Tạo public + private key

# Copy public key sang Pi kia:
scp /etc/secoc/keys/mldsa65_public.key pi@<PEER_IP>:/etc/secoc/keys/peer_public.key
```

---

## 5. Cách Chạy AUTOSAR Stack & Cấu Hình Cho Từng Thiết Bị

### 5.1 Kiến Trúc Phần Mềm AUTOSAR Trên Mỗi Pi4

```
┌─────────────────────────────────────────────┐
│  Application Layer (Com, ApBridge)          │
├─────────────────────────────────────────────┤
│  SecOC Module  (ký/xác thực ML-DSA-65)      │
├─────────────────────────────────────────────┤
│  PduR  (PDU Router — định tuyến giữa các    │
│         bus: CAN ↔ Ethernet)                │
├──────────────┬──────────────────────────────┤
│  CAN Stack   │  Ethernet Stack              │
│  CanIf→CanTp │  TcpIp → SoAd → SoAd_PQC     │
├──────────────┴──────────────────────────────┤
│  MCAL (Pi4): SocketCAN, GPIO, Watchdog      │
└─────────────────────────────────────────────┘
```

### 5.2 Trình Tự Khởi Động (Boot Sequence)

Khi binary chạy trên Pi4, `main()` thực hiện 3 giai đoạn:

```
main()
  ├── EcuM_Init()          ← Giai đoạn 0+1
  │     ├── Mcu_Init()           # MCAL: CPU, clock
  │     ├── Wdg_Init()           # Watchdog 15s
  │     ├── Dio_Init()           # GPIO
  │     ├── Can_Init()           # SocketCAN (can0)
  │     ├── Csm_Init()           # Crypto Service Manager
  │     └── Det_Init(), Dem_Init(), Dcm_Init()
  │ 
  ├── EcuM_StartupTwo()    ← Giai đoạn 2
  │     ├── TcpIp_Init()         # TCP/IP stack
  │     ├── SoAd_Init()          # Socket Adapter
  │     ├── CanIf_Init()         # CAN Interface
  │     ├── CanTp_Init()         # CAN Transport Protocol
  │     ├── SecOC_Init()         # ★ SecOC module
  │     ├── Com_Init()           # Communication
  │     └── SoAd_PQC_Init()     # ★ PQC key exchange (ML-KEM)
  │
  └── Os_MainFunction()    ← Vòng lặp chính (cooperative OS)
        ├── SecOC_MainFunctionTx()   # Ký & gửi PDU
        ├── SecOC_MainFunctionRx()   # Nhận & xác thực PDU
        ├── SoAd_MainFunctionTx/Rx() # Ethernet I/O
        ├── CanTp_MainFunctionTx/Rx()# CAN I/O
        └── Com_MainTx()            # Application data
```

### 5.3 Build & Chạy Trên Pi4

```bash
cd /home/pi/secoc/Autosar_SecOC

# 1. Build liboqs (chỉ lần đầu)
bash build_liboqs.sh

# 2. Build AUTOSAR stack
mkdir -p build && cd build
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug -DMCAL_TARGET=PI4 ..
make -j4

# 3. Chạy
source /etc/secoc/zone-controller.env   # hoặc hpc-gateway.env
./bin/secoc_gateway
```

### 5.4 Cấu Hình Khác Nhau Giữa 2 Pi4

Cả 2 Pi4 chạy **cùng một binary**. Sự khác biệt nằm ở file env:

| Tham số | Pi4 #1 — Zone Controller | Pi4 #2 — HPC Gateway |
|---------|--------------------------|----------------------|
| `SECOC_ROLE` | `ZC` | `HPC` |
| `SECOC_PEER_IP` | `10.0.0.20` | `10.0.0.10` |
| CAN interface | Không dùng | `can0` (SocketCAN + MCP2515) |
| Luồng dữ liệu | Nhận Ethernet → xác thực → dashboard | Nhận CAN → ký PQC → gửi Ethernet |

### 5.5 Cấu Hình PDU (đã cài sẵn trong code)

Stack có **6 cặp Tx/Rx PDU** được cấu hình tĩnh:

| PDU ID | Loại | Transport | Mô tả |
|--------|------|-----------|-------|
| 0 | `SECOC_SECURED_PDU_CANIF` | CAN IF | Frame CAN trực tiếp (≤8 byte) |
| 1 | `SECOC_SECURED_PDU_CANTP` | CAN TP | Frame CAN lớn (segmented) |
| 2 | `SECOC_SECURED_PDU_SOADTP` | SoAd TP | **★ Đường Ethernet — dùng cho PQC** |
| 3 | `SECOC_SECURED_PDU_CANIF` | CAN IF | CAN IF không header |
| 4 | `SECOC_SECURED_PDU_CANTP` | CAN TP | CAN TP không header |
| 5 | Collection | Auth + Crypto | PDU chia tách auth/crypto |

> **Quan trọng:** Ở chế độ PQC, chữ ký ML-DSA-65 = 3,309 byte → **bắt buộc dùng TP mode** (PDU ID 1, 2, hoặc 4). CAN IF (PDU 0, 3) chỉ dùng cho classic HMAC 4-byte.

### 5.6 Các Tham Số Cấu Hình Quan Trọng

**File `include/SecOC/SecOC_PQC_Cfg.h`:**

| Define | Giá trị | Ý nghĩa |
|--------|---------|---------|
| `SECOC_USE_PQC_MODE` | `TRUE` | Bật chế độ PQC (ML-DSA-65) |
| `SECOC_USE_MLKEM_KEY_EXCHANGE` | `TRUE` | Bật trao đổi khóa ML-KEM-768 |
| `SECOC_ETHERNET_GATEWAY_MODE` | `TRUE` | Chế độ gateway Ethernet |
| `SECOC_PQC_MAX_PDU_SIZE` | `8192` | Kích thước PDU tối đa (byte) |
| `PQC_MLDSA_KEY_DIRECTORY` | `"/etc/secoc/keys/"` | Thư mục chứa khóa |
| `SOAD_PQC_REKEY_INTERVAL_CYCLES` | `360000` | Tự đổi khóa mỗi ~1 giờ |

**File `include/SecOC/SecOC_Cfg.h`:**

| Define | Giá trị | Ý nghĩa |
|--------|---------|---------|
| `SECOC_MAIN_FUNCTION_PERIOD_TX` | `0.9` s | Chu kỳ gửi |
| `SECOC_MAIN_FUNCTION_PERIOD_RX` | `0.9` s | Chu kỳ nhận |
| `SECOC_NUM_OF_TX_PDU_PROCESSING` | `6` | Số PDU Tx |
| `SECOC_NUM_OF_RX_PDU_PROCESSING` | `6` | Số PDU Rx |

**File `include/Mcal/Mcal_Cfg.h` (Pi4 MCAL):**

| Define | Giá trị | Ý nghĩa |
|--------|---------|---------|
| `MCAL_CAN_INTERFACE_NAME` | `"can0"` | Tên SocketCAN interface |
| `MCAL_WDG_TIMEOUT_SEC` | `15` | Watchdog timeout |
| `MCAL_DIO_GPIO_CHIP` | `"/dev/gpiochip0"` | GPIO device |

### 5.7 Khởi Tạo CAN Trên Pi4 #2

Pi4 #2 dùng **Linux SocketCAN** — không cần driver riêng trong code:

```bash
# Bật CAN thật (MCP2515 qua SPI):
sudo ip link set can0 up type can bitrate 500000

# Hoặc CAN ảo để test không cần phần cứng:
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link set up vcan0
```

Trong code, `Can_Pi4.c` mở socket `AF_CAN` trên interface `can0`, đọc/ghi frame qua `Can_MainFunction_Read()` và `Can_MainFunction_Write()`.

### 5.8 Luồng Dữ Liệu End-to-End

```
ESP32 (CAN frame 0x100, rpm=3500)
  │
  ▼ CAN bus 500kbit/s
  │
Pi4 #2 (HPC):
  Can_MainFunction_Read()  →  CanIf  →  CanTp  →  PduR
  →  SecOC_MainFunctionTx()  [ký ML-DSA-65, thêm freshness]
  →  PduR  →  SoAd_MainFunctionTx()
  │
  ▼ Ethernet (secured PDU 3,316 byte)
  │
Pi4 #1 (ZC):
  SoAd_MainFunctionRx()  →  PduR
  →  SecOC_MainFunctionRx()  [xác thực chữ ký + freshness]
  →  PduR  →  Com  →  Dashboard (hiển thị RPM)
```

---

## 6. Firmware ESP32

### 6.1 Yêu cầu

- Arduino IDE hoặc PlatformIO
- Thư viện: `driver/twai.h` (TWAI = CAN controller tích hợp ESP32)

### 6.2 Chức năng cần lập trình

Mỗi ESP32 giả lập một ECU, gửi CAN frame định kỳ:

| ESP32 | ECU giả lập | CAN ID | Payload mẫu | Chu kỳ |
|-------|-------------|--------|-------------|--------|
| ESP32-1 | Engine ECU | 0x100 | `{rpm: uint16}` | 100 ms |
| ESP32-2 | Brake ECU | 0x200 | `{pressure: uint16}` | 50 ms |

### 6.3 Cấu hình TWAI trên ESP32

```c
#include "driver/twai.h"

twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
    GPIO_NUM_21,  // TX
    GPIO_NUM_22,  // RX
    TWAI_MODE_NORMAL
);
twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
```

> **Lưu ý:** ESP32 chỉ gửi CAN frame thô — việc ký PQC do Pi4 #2 (HPC) xử lý.

---

## 7. Kịch Bản Demo (4 kịch bản)

### Kịch bản 1 — Happy Path (5 phút)

1. ESP32 gửi CAN frame `rpm=3500` mỗi 100ms
2. Pi4 #2 nhận qua SocketCAN → ký ML-DSA-65 → gửi secured PDU (3,316 byte) qua Ethernet
3. Pi4 #1 nhận → xác thực chữ ký → hiển thị RPM trên dashboard
4. **Kết quả mong đợi:** Dashboard hiện `Verified OK: xxxx / Failed: 0`

### Kịch bản 2 — Tấn công giả mạo (3 phút)

1. Thay đổi 1 byte trong payload trên đường truyền
2. **Kết quả:** Pi4 #1 log `verify_PQC FAIL — signature mismatch`, dashboard hiện `Failed: +1`

### Kịch bản 3 — Tấn công phát lại (2 phút)

1. Bắt 1 PDU hợp lệ, phát lại sau 5 giây
2. **Kết quả:** `verify_PQC FAIL — freshness stale` (bộ đếm freshness từ chối)

### Kịch bản 4 — So sánh PQC vs Classic (5 phút)

| Tiêu chí | Classic (AES-CMAC) | PQC (ML-DSA-65) |
|----------|-------------------|-----------------|
| Kích thước chữ ký | 4 byte | 3,309 byte |
| Độ trễ | ~0.05 ms | ~4 ms |
| Kháng lượng tử | ❌ | ✅ NIST FIPS 204 |

---

## 8. Thiết Kế Hộp Demo & Bố Trí Phần Cứng

### 8.1 Ý Tưởng Thiết Kế Tổng Thể

Mục tiêu: Tạo một bộ demo **di động, chuyên nghiệp** mà có thể mang đến phòng bảo vệ luận văn hoặc triển lãm. Toàn bộ hệ thống nằm gọn trong **1 hộp acrylic/gỗ** kích thước khoảng **50cm × 35cm × 12cm**.

```
┌─────────────────────────────────────────────────────────────┐
│                    MẶT TRÊN (NẮP TRONG SUỐT)                │
│                                                             │
│  ┌────────────┐    Cáp RJ45     ┌────────────┐              │
│  │  Pi4 #1    │◄═══════════════►│  Pi4 #2    │              │
│  │  HPC       │   (Ethernet)    │  ZC        │              │
│  │            │                 │            │              │
│  │            │                 │  ┌──────┐  │              │
│  │  LED xanh: │                 │  │MCP2515│ │              │
│  │  Verify OK │                 │  └──┬───┘  │              │
│  │  LED đỏ:   │                 │     │SPI   │              │
│  │  Verify FAIL│                │     │      │              │
│  └────────────┘                 └─────┼──────┘              │
│                                       │                      │
│                                  CAN bus (dây xoắn)          │
│                                      │                      │
│  ┌──────────┐  ┌──────────┐  ┌───────┴──┐                   │
│  │ ESP32 #1 │  │ ESP32 #2 │  │ ESP32 #3 │   ← ECU giả lập  │
│  │ Engine   │  │ Brake    │  │ Steering │                    │
│  │ 0x100    │  │ 0x200    │  │ 0x300    │                    │
│  └──────────┘  └──────────┘  └──────────┘                    │
│                                                              │
│  ┌──────────────────────────────────────────────┐            │
│  │       BẢNG NHÃN (Label Board)                │            │
│  │  "AUTOSAR SecOC + Post-Quantum Cryptography" │            │
│  │  "ML-DSA-65 | ML-KEM-768 | NIST FIPS 204"   │            │
│  └──────────────────────────────────────────────┘            │
└─────────────────────────────────────────────────────────────┘
     ↑ nguồn USB-C (ổ cắm ở mặt sau)
```

### 8.2 Chi Tiết Vật Liệu Hộp

| Thành phần | Vật liệu | Kích thước | Ghi chú |
|------------|----------|-----------|---------|
| Đáy hộp | Mica đen 5mm hoặc gỗ MDF | 50×35cm | Khoan lỗ cố định Pi4, ESP32 |
| Nắp trên | Mica trong suốt 3mm | 50×35cm | Để nhìn thấy linh kiện bên trong |
| Thành hộp | Mica đen 5mm | Cao 10-12cm | Có lỗ thoáng khí + cổng USB-C |
| Standoff | Đồng M2.5 × 11mm | 8 cái | Cố định Pi4 (4 lỗ/board) |
| Giá đỡ ESP32 | In 3D hoặc keo nóng | — | Gắn ESP32 lên đáy |

### 8.3 Bố Trí Bên Trong Hộp

```
Phân vùng theo chức năng:

╔═══════════════════╦═══════════════════╗
║   VÙNG ETHERNET   ║   VÙNG CAN BUS   ║
║                   ║                   ║
║   Pi4 #1 (ZC)     ║   Pi4 #2 (HPC)   ║
║   ↕ HDMI ra màn   ║   + MCP2515      ║
║                   ║   ↕               ║
║                   ║  ┌───┬───┬───┐   ║
║                   ║  │E1 │E2 │E3 │   ║
║                   ║  └───┴───┴───┘   ║
║                   ║  ESP32 cluster   ║
╠═══════════════════╩═══════════════════╣
║  LED indicator bar + Bảng label       ║
╚═══════════════════════════════════════╝
```

**Nguyên tắc bố trí:**
- **Trái:** Pi4 #1 (ZC) — cổng HDMI hướng ra ngoài hộp để nối màn hình
- **Phải:** Pi4 #2 (HPC) + module MCP2515 + cluster ESP32
- **Dưới cùng:** Dải LED indicator + bảng nhãn thông tin
- **Mặt sau:** Lỗ cho cáp nguồn USB-C × 2, cổng HDMI × 1

### 8.4 Hệ Thống Hiển Thị (Display)

#### Phương án A: Màn hình HDMI rời (khuyên dùng cho bảo vệ)

```
Hộp demo ──HDMI──→ Màn hình 15" hoặc TV
                   ┌─────────────────────────────────┐
                   │  ┌─────────────────────────┐    │
                   │  │  AUTOSAR SecOC Dashboard │    │
                   │  ├─────────────────────────┤    │
                   │  │ ● Auth: ML-DSA-65       │    │
                   │  │ ● Key:  ML-KEM-768      │    │
                   │  │                         │    │
                   │  │ RPM: ████████ 3500      │    │
                   │  │ Brake: ████ 42 bar      │    │
                   │  │                         │    │
                   │  │ ✅ Verified: 12,847     │    │
                   │  │ ❌ Failed:   3          │    │
                   │  │ ⏱ Latency:  0.38 ms    │    │
                   │  │ 📦 Sig size: 3,309 B   │    │
                   │  └─────────────────────────┘    │
                   └─────────────────────────────────┘
```

- Pi4 #1 (ZC) xuất HDMI ra màn hình
- Chạy GUI PySide6 (`python GUI/main.py`) hiển thị dashboard
- Dashboard đọc dữ liệu từ shared library SecOC qua ctypes

#### Phương án B: Màn hình 7" touchscreen gắn trên hộp

- Màn hình Waveshare 7" (800×480) gắn vào nắp hộp hoặc đứng cạnh
- Compact hơn, tự chứa hoàn toàn
- Thêm vào BOM: ~350k VND

#### Phương án C: Laptop qua SSH/VNC (backup)

```bash
# Từ laptop:
ssh pi@10.0.0.10 -X   # X11 forwarding
cd /home/pi/secoc/GUI && python main.py
```

### 8.5 Hệ Thống LED Chỉ Thị (GPIO)

Thêm trực quan cho demo — các LED gắn trên mặt hộp:

| LED | GPIO Pin (Pi4 #1) | Ý nghĩa | Trạng thái |
|-----|-------------------|---------|-----------|
| 🟢 Xanh lá | GPIO 17 | Verify OK | Nhấp nháy mỗi lần xác thực thành công |
| 🔴 Đỏ | GPIO 27 | Verify FAIL | Sáng khi phát hiện tấn công |
| 🔵 Xanh dương | GPIO 22 | ML-KEM handshake | Sáng trong lúc trao đổi khóa |
| 🟡 Vàng | GPIO 23 | CAN Active | Nhấp nháy khi nhận CAN frame |

```
Mặt trước hộp:
┌──────────────────────────────────────┐
│  🟢 AUTH OK   🔴 FAIL   🔵 KEY   🟡 CAN │
│  ○            ○          ○         ○      │
└──────────────────────────────────────┘
```

### 8.6 Quản Lý Nguồn Điện

```
Ổ cắm 220V → Bộ chia USB-C (hub) → Pi4 #1 (5V/3A)
                                   → Pi4 #2 (5V/3A)
                                   → ESP32 (5V từ USB hoặc 3.3V từ Pi4)
```

**Khuyến nghị:** Dùng 1 bộ nguồn USB-C PD 65W với hub chia 3 cổng — chỉ cần 1 dây nguồn cho toàn bộ hộp.

ESP32 có thể lấy nguồn 5V từ chân 5V của Pi4 #2 qua header — giảm số dây nguồn.

---

## 9. Giải Thích Chi Tiết AUTOSAR SecOC — Dành Cho Kỹ Sư Phần Cứng

### 9.1 AUTOSAR Là Gì?

**AUTOSAR** (AUTomotive Open System ARchitecture) là tiêu chuẩn kiến trúc phần mềm cho ECU ô tô, được hầu hết OEM & Tier-1 áp dụng (BMW, Toyota, Continental, Bosch, v.v.).

Trong dự án này, chúng ta implement **lớp BSW** (Basic Software) theo AUTOSAR R21-11, tập trung vào module **SecOC** (Secure Onboard Communication).

### 9.2 Các Module AUTOSAR Trong Hệ Thống

```
┌─────────────────────────────────────────────────────────────────┐
│                    APPLICATION LAYER                            │
│  Com_SendSignal() ← ESP32 gửi RPM/brake data                    │
│  Com_ReceiveSignal() → Đọc dữ liệu đã xác thực                  │
├─────────────────────────────────────────────────────────────────┤
│                    COM (Communication)                          │
│  Quản lý 6 Tx + 6 Rx I-PDU, signal packing/unpacking            │
│  Com_MainFunctionTx() → PduR_ComTransmit()                      │
├─────────────────────────────────────────────────────────────────┤
│              ★ SecOC (Secure Onboard Communication) ★          │
│                                                                 │
│  TX: authenticate_PQC()                                         │
│    1. Lấy freshness counter (FVM)                               │
│    2. Tạo DataToAuth = [DataId|AuthPdu|Freshness]               │
│    3. Csm_SignatureGenerate() → ML-DSA-65 sign → 3,309 byte     │
│    4. Ghép: [Header][AuthPdu][TruncFreshness][Signature]        │
│                                                                 │
│  RX: verify_PQC()                                               │
│    1. Tách SecuredPDU → AuthPdu + Freshness + Signature         │
│    2. Khôi phục full freshness từ truncated bits                │
│    3. Csm_SignatureVerify() → ML-DSA-65 verify                  │
│    4. Nếu OK → chuyển AuthPdu lên COM                           │
│    5. Nếu FAIL → drop PDU, log lỗi                              │
├─────────────────────────────────────────────────────────────────┤
│                    PduR (PDU Router)                            │
│  Định tuyến dựa trên PDU ID:                                    │
│    ID 0,3 → CanIF    (CAN trực tiếp, PDU ≤ 8 byte)              │
│    ID 1,4 → CanTP    (CAN phân đoạn, PDU lớn)                   │
│    ID 2   → SoAd TP  (Ethernet — đường chính cho PQC)           │
├──────────────────────┬──────────────────────────────────────────┤
│   CAN Stack          │   Ethernet Stack                          │
│                      │                                           │
│   CanIF              │   SoAd (Socket Adapter)                   │
│    ↕                 │    + SoAd_PQC (ML-KEM handshake)         │
│   CanTP              │    ↕                                      │
│    ↕                 │   TcpIp                                   │
│   Can_Pi4.c          │    ↕                                      │
│   (SocketCAN)        │   EthDrv (TCP socket, port 50002)        │
├──────────────────────┴──────────────────────────────────────────┤
│                    CSM (Crypto Service Manager)                   │
│                                                                  │
│  Classic mode: HMAC-SHA256 hoặc AES-CMAC (4 byte MAC)          │
│  PQC mode:                                                       │
│    ├── ML-DSA-65 (NIST FIPS 204): ký/xác thực                  │
│    │   Secret key: 4,032 byte | Public key: 1,952 byte          │
│    │   Signature: 3,309 byte                                     │
│    └── ML-KEM-768 (NIST FIPS 203): trao đổi khóa               │
│        Public key: 1,184 byte | Ciphertext: 1,088 byte          │
│        Shared secret: 32 byte                                    │
├─────────────────────────────────────────────────────────────────┤
│                    EcuM (ECU State Manager)                       │
│  Khởi tạo toàn bộ hệ thống theo đúng thứ tự AUTOSAR:           │
│  MCAL → Det → Dem → NvM → Can → Csm → SecOC → Com → Eth → PQC │
├─────────────────────────────────────────────────────────────────┤
│                    Os (OSEK Cooperative OS)                       │
│  Vòng lặp chính: gọi MainFunction của từng module theo chu kỳ  │
│  SecOC_MainFunctionTx/Rx chạy mỗi 900ms                        │
├─────────────────────────────────────────────────────────────────┤
│                    MCAL (Pi4 Hardware Abstraction)                │
│  Can_Pi4.c: SocketCAN driver (can0)                              │
│  Dio_Pi4.c: GPIO qua /dev/gpiochip0 (LED, input)               │
│  Wdg_Pi4.c: Watchdog /dev/watchdog (timeout 15s)                │
│  Mcu_Pi4.c: CPU info                                             │
│  Gpt_Pi4.c: Timer (clock_gettime)                                │
└─────────────────────────────────────────────────────────────────┘
```

### 9.3 Luồng Dữ Liệu Chi Tiết: ESP32 → Dashboard

Đây là **toàn bộ hành trình** của 1 CAN frame từ ESP32 đến màn hình:

```
╔══════════════════════════════════════════════════════════════╗
║ BƯỚC 1: ESP32 gửi CAN frame                                 ║
║                                                              ║
║   twai_transmit({id=0x100, data=[0x0D,0xAC], len=2})       ║
║   → CAN bus 500kbit/s → dây CAN_H / CAN_L                  ║
╚══════════════════════════════════════════════════════════════╝
                              │
                              ▼
╔══════════════════════════════════════════════════════════════╗
║ BƯỚC 2: Pi4 #2 (HPC) nhận CAN frame                        ║
║                                                              ║
║   MCP2515 (SPI) → Linux kernel → SocketCAN driver           ║
║   Can_Pi4.c::Can_MainFunction_Read()                        ║
║     → read(can_socket) → Can_PduType {id=0x100, data, len} ║
║   CanIf_RxIndication(ControllerId=0, &pdu)                  ║
║     → kiểm tra ID, chuyển tiếp lên PduR                     ║
║   PduR_CanIfRxIndication(RxPduId=0, &pduInfo)               ║
║     → SecOC_RxIndication() → lưu vào buffer nội bộ          ║
╚══════════════════════════════════════════════════════════════╝
                              │
                              ▼
╔══════════════════════════════════════════════════════════════╗
║ BƯỚC 3: Pi4 #2 ký PQC (trong SecOC_MainFunctionTx)         ║
║                                                              ║
║   1. AuthPdu = [0x0D, 0xAC] (2 byte RPM data)              ║
║   2. FVM_GetTxFreshnessTruncData(FreshnessId)               ║
║      → fullFreshness = 0x00000042 (counter tăng mỗi lần)   ║
║      → truncFreshness = 0x42 (LSB truncated)                ║
║   3. DataToAuth = [DataId(2B)] | [AuthPdu(2B)] | [FV(4B)]  ║
║      = [0x00,0x02, 0x0D,0xAC, 0x00,0x00,0x00,0x42]        ║
║   4. Csm_SignatureGenerate(DataToAuth, 8 bytes)             ║
║      → CryIf → PQC_MLDSA_Sign() → OQS_SIG_sign()          ║
║      → signature[3309 byte] ← ML-DSA-65                     ║
║   5. SecuredPDU = [Header(4B)][AuthPdu(2B)][FV(1B)][Sig]   ║
║      = tổng ~3,316 byte                                     ║
║   6. FVM_IncreaseCounter() → counter = 0x43                 ║
╚══════════════════════════════════════════════════════════════╝
                              │
                              ▼
╔══════════════════════════════════════════════════════════════╗
║ BƯỚC 4: Pi4 #2 gửi qua Ethernet                            ║
║                                                              ║
║   PduR_SecOCTransmit(PduId=2)  → SECOC_SECURED_PDU_SOADTP  ║
║   SoAd_TpTransmit() → TcpIp_TcpTransmit()                 ║
║   EthDrv: TCP socket → 10.0.0.10:50002                     ║
║   Wire payload: [SecuredPDU(3316B)] [PduId(2B)] = 3,318 B  ║
╚══════════════════════════════════════════════════════════════╝
                              │
                     Cáp Ethernet RJ45
                              │
                              ▼
╔══════════════════════════════════════════════════════════════╗
║ BƯỚC 5: Pi4 #1 (ZC) nhận và xác thực                       ║
║                                                              ║
║   EthDrv_Receive() → TCP accept → read 3,318 byte          ║
║   Tách PduId từ 2 byte cuối → PduId = 2                    ║
║   SoAd_RxIndication()                                        ║
║     → kiểm tra PQC control message? Không → chuyển SecOC   ║
║   SecOC_StartOfReception() → cấp buffer Rx                   ║
║   SecOC_MainFunctionRx():                                    ║
║     1. parseSecuredPdu()                                     ║
║        → AuthPdu=[0x0D,0xAC], TruncFV=0x42, Sig[3309B]     ║
║     2. FVM_GetRxFreshness(truncFV=0x42)                     ║
║        → reconstruct fullFV = 0x00000042                    ║
║        → kiểm tra: 0x42 > lastVerified(0x41)? ✓             ║
║     3. Xây lại DataToAuth (giống bước 3.3)                  ║
║     4. Csm_SignatureVerify(DataToAuth, Signature)           ║
║        → PQC_MLDSA_Verify() → OQS_SIG_verify()              ║
║        → kết quả: CRYPTO_E_VER_OK ✓                         ║
║     5. Cập nhật counter, chuyển AuthPdu lên COM              ║
╚══════════════════════════════════════════════════════════════╝
                              │
                              ▼
╔══════════════════════════════════════════════════════════════╗
║ BƯỚC 6: Hiển thị trên Dashboard                            ║
║                                                              ║
║   PduR_SecOCIfRxIndication() → Com_RxIndication()           ║
║   Com_ReceiveSignal(SignalId) → [0x0D, 0xAC] = 3500 RPM    ║
║                                                              ║
║   GUI (PySide6) gọi qua ctypes:                              ║
║     bridge.verify(pdu_id=2, mode="PQC")                      ║
║     bridge.get_auth_pdu(pdu_id=2) → "0DAC"                   ║
║                                                              ║
║   Dashboard hiện:                                            ║
║     RPM: 3500 | Verified: ✅ | Latency: 0.38ms              ║
║     Sig size: 3,309 B | Freshness: 0x42                      ║
╚══════════════════════════════════════════════════════════════╝
```
 
### 9.4 Quy Trình Trao Đổi Khóa ML-KEM (Khi Khởi Động)

Trước khi truyền dữ liệu, 2 Pi4 phải hoàn thành **handshake ML-KEM-768**:

```
Pi4 #2 (HPC - Initiator)                Pi4 #1 (ZC - Responder)
═══════════════════════                  ═══════════════════════
SoAd_PQC_Init()                          SoAd_PQC_Init()
  state = IDLE                             state = IDLE

SoAd_PQC_KeyExchange(peer=0, init=TRUE)
  │
  ├─ PQC_MLKEM_KeyGen()
  │  → pk_alice[1184B], sk_alice[2400B]
  │
  ▼ UDP: [0x51][0x43][0x01][0x01][peerId][pk 1184B]
  ─────────────────────────────────────────────────→
                                           Nhận pk_alice
                                           PQC_MLKEM_Encapsulate(pk_alice)
                                             → ct[1088B] + ss[32B]
                                           Lưu shared_secret
  ←─────────────────────────────────────────────────
  UDP: [0x51][0x43][0x01][0x02][peerId][ct 1088B] ◄┘

  Nhận ciphertext
  PQC_MLKEM_Decapsulate(ct, sk_alice)
    → ss[32B] (giống shared_secret bên kia)
  Xóa sk_alice (bảo mật)

  state = SESSION_ESTABLISHED              state = SESSION_ESTABLISHED
  ══════════════════════════════════════════════════════════════
  Bây giờ cả 2 bên có shared_secret 32 byte
  → Csm_DeriveSessionKeys() tạo session key
  → Sẵn sàng truyền SecOC PDU

  Auto-rekey: mỗi 360,000 chu kỳ (~1 giờ) lặp lại handshake
```

### 9.5 Cấu Trúc Secured PDU Trên Dây

```
Classic Mode (CAN IF — 4 byte MAC):
┌────────┬──────────┬──────────┬────────────┐
│ Header │ AuthPdu  │ TruncFV  │ HMAC (4B)  │
│ 0-4B   │ ≤20B     │ 1-4B     │            │
└────────┴──────────┴──────────┴────────────┘
Tổng: ≤ 28 byte → vừa CAN frame

PQC Mode (Ethernet — 3,309 byte chữ ký):
┌────────┬──────────┬──────────┬─────────────────────┐
│ Header │ AuthPdu  │ TruncFV  │ ML-DSA-65 Sig       │
│ 4B     │ ≤20B     │ 4B       │ 3,309 byte          │
└────────┴──────────┴──────────┴─────────────────────┘
Tổng: ~3,337 byte → bắt buộc TP mode qua Ethernet

Trên Ethernet wire (thêm PduId):
[SecuredPDU ~3,337B] [PduId 2B little-endian] → TCP payload ~3,339B
```

### 9.6 Bảng So Sánh Tổng Hợp: Classic vs PQC

| Tiêu chí | Classic (HMAC/CMAC) | PQC (ML-DSA-65 + ML-KEM-768) |
|----------|--------------------|-----------------------------|
| **Thuật toán ký** | HMAC-SHA256 / AES-CMAC | ML-DSA-65 (NIST FIPS 204) |
| **Kích thước chữ ký** | 4 byte | 3,309 byte |
| **Thuật toán trao đổi khóa** | Không (static key) | ML-KEM-768 (NIST FIPS 203) |
| **Thời gian ký** | ~2 µs | ~250 µs |
| **Thời gian xác thực** | ~2 µs | ~120 µs |
| **Transport** | CAN IF (trực tiếp) | Ethernet TP (bắt buộc) |
| **Kháng máy tính lượng tử** | ❌ Bị phá bởi Shor/Grover | ✅ An toàn lượng tử |
| **Tiêu chuẩn** | Truyền thống | NIST FIPS 203 + 204 (2024) |
| **Khi nào dùng** | CAN bus, ECU tài nguyên thấp | Ethernet backbone, bảo mật dài hạn |

### 9.7 Dashboard GUI — Cách Kết Nối Với AUTOSAR Stack

GUI chạy trên Pi4 #1, giao tiếp với C backend qua **shared library** (`.so`):

```
┌─────────────────────────────────┐
│  Python GUI (PySide6)           │
│  main.py → main_window.py      │
│  ┌───────────────────────────┐  │
│  │ pages/secoc_config.py     │  │ ← Cấu hình SecOC, chọn PQC/Classic
│  │ pages/dashboard.py        │  │ ← Hiển thị topology, trạng thái
│  │ pages/diagnostics.py      │  │ ← Monitor network health
│  │ pages/routing.py          │  │ ← Bảng routing PDU/Signal
│  │ pages/hardware_config.py  │  │ ← Thông tin Pi4 hardware
│  └───────────────────────────┘  │
│           │ ctypes FFI          │
│           ▼                     │
│  core/backend_bridge.py         │
│    load("libSecOCLibShared.so") │
├─────────────────────────────────┤
│  C Shared Library               │
│  GUIInterface.c (DLL exports)   │
│    ├─ init()      → EcuM_Init() + EcuM_StartupTwo()
│    ├─ authenticate(id, data, "PQC") → authenticate_PQC()
│    ├─ verify(id, "PQC")            → verify_PQC()
│    ├─ get_secured_pdu(id)          → hex string kết quả ký
│    ├─ transmit(id)                 → PduR → SoAd → Ethernet
│    ├─ receive()                    → EthDrv_Receive() → SecOC Rx
│    ├─ alter_freshness(id)          → Giả lập replay attack
│    └─ alter_authenticator(id)      → Giả lập tamper attack
└─────────────────────────────────┘
```

**Cách chạy GUI trên Pi4 #1:**
```bash
# Cài dependencies
pip install PySide6

# Chạy
cd /home/pi/secoc/GUI
python main.py
```

GUI sẽ tự load shared library, gọi `init()` khởi tạo toàn bộ AUTOSAR stack, và sẵn sàng demo.

---

## 10. Checklist Nghiệm Thu

Kỹ sư phần cứng hoàn thành khi **tất cả** các mục sau đạt:

- [ ] Cài `install.sh` trên Pi4 mới → `can0` lên và `secoc-gateway.service` tự khởi động
- [ ] Cả 2 Pi4 chạy cùng binary, chỉ khác file `/etc/secoc/<role>.env`
- [ ] 2 ESP32 gửi CAN frame → `journalctl -u secoc-gateway -f` trên Pi4 #1 hiện `verify_pqc result=E_OK`
- [ ] Tắt/bật nguồn Pi4 bất kỳ → ML-KEM handshake hoàn thành trong 5 giây, traffic tự phục hồi
- [ ] Kịch bản tấn công giả mạo (Kịch bản 2) → log `E_NOT_OK`, frame bị drop, **không crash**
- [ ] `bash tests/hw_smoke_test.sh` chạy tự động < 60 giây, exit code = 0

---

## 11. Các Vấn Đề Cần Lưu Ý

| Vấn đề | Giải pháp |
|--------|-----------|
| `liboqs.so` không tìm thấy khi chạy | `sudo ldconfig` hoặc copy vào `/usr/local/lib` |
| CAN không nhận frame | Kiểm tra: oscillator MCP2515 (8MHz vs 16MHz), điện trở 120Ω, `dmesg | grep can` |
| ML-KEM handshake thất bại | Kiểm tra kết nối Ethernet, public key đúng peer, firewall tắt |
| PDU lớn bị mất | Đảm bảo dùng TP mode (không phải IF mode) cho PQC — signature 3,309 byte |
| ESP32 không gửi CAN | Kiểm tra chân TX/RX, transceiver SN65HVD230, `twai_start()` đã gọi |

---

## 12. Liên Hệ Hỗ Trợ

Mọi vấn đề liên quan đến **SecOC/PQC software** → liên hệ nhóm phần mềm.  
Kỹ sư phần cứng chịu trách nhiệm: **đấu nối, MCP2515 overlay, first-boot, firmware ESP32**.

---

*Tài liệu này là bản tóm tắt bàn giao. Chi tiết kỹ thuật đầy đủ xem tại `docs/ARCHITECTURE.md` và `CLAUDE.md`.*
