# Hướng Dẫn Nhanh — Chạy PQC (Post-Quantum Crypto) Trên Raspberry Pi 4

> **Dành cho:** Kỹ sư phần cứng không cần biết AUTOSAR  
> **Mục tiêu:** Chứng minh mã hóa hậu lượng tử (ML-DSA-65) hoạt động trên Pi4  
> **Thời gian setup:** ~30 phút

---

## 1. Vấn Đề Giải Quyết (TL;DR)

Xe hơi hiện đại có hàng chục ECU (bộ điều khiển) nói chuyện với nhau qua CAN bus và Ethernet. Hacker có thể **giả mạo** hoặc **phát lại** (replay) các tin nhắn để điều khiển phanh, động cơ, v.v.

**Giải pháp:** Mỗi tin nhắn được **ký số** trước khi gửi → bên nhận **xác thực chữ ký** → nếu sai → từ chối.

```
ESP32 (ECU giả lập)  ──CAN──►  Pi4 #2 (HPC)  ──Ethernet──►  Pi4 #1 (ZC)
     gửi RPM=3500              KÝ chữ ký PQC              XÁC THỰC chữ ký
                               (3,309 byte)                OK → hiển thị RPM
                                                           FAIL → từ chối
```

### Tại sao cần "Post-Quantum"?

| | Ký số truyền thống (HMAC) | Ký số hậu lượng tử (ML-DSA-65) |
|---|---|---|
| Chữ ký | 4 byte | 3,309 byte |
| Bị máy tính lượng tử phá? | ✅ Có | ❌ Không |
| Tiêu chuẩn | Cũ | NIST FIPS 204 (2024) |
| Tốc độ ký | ~2 µs | ~250 µs |

> **Nói đơn giản:** Chúng ta thay thế chữ ký 4 byte cũ bằng chữ ký 3,309 byte mới mà máy tính lượng tử không thể phá được. Cái giá là chữ ký lớn hơn ~825 lần → cần Ethernet thay vì CAN để truyền.

---

## 2. Bạn Cần Gì? (Phần Cứng)

### Phương án A — Tối giản (chỉ cần 1 Pi4)

Chạy test PQC **ngay trên 1 Pi4 duy nhất**, không cần ESP32, không cần CAN, không cần 2 Pi4.

| # | Linh kiện | Số lượng |
|---|-----------|----------|
| 1 | Raspberry Pi 4 (4GB+) | 1 |
| 2 | Thẻ microSD 32GB+ (Class 10) | 1 |
| 3 | Nguồn USB-C 5V/3A | 1 |
| 4 | Màn hình HDMI hoặc SSH từ laptop | — |

> **Đây là đủ** để chứng minh PQC ký + xác thực hoạt động.

### Phương án B — Demo đầy đủ (2 Pi4 + ESP32)

Nếu muốn demo end-to-end (ESP32 → CAN → Pi4 → Ethernet → Pi4), xem [wrapup.md](wrapup.md) mục 2-3 để lấy BOM đầy đủ.

---

## 3. Cài Đặt Pi4 (Từng Bước)

### Bước 1: Cài Raspberry Pi OS

Flash **Raspberry Pi OS Lite 64-bit** lên thẻ SD bằng [Raspberry Pi Imager](https://www.raspberrypi.com/software/).

### Bước 2: Cài đặt công cụ

```bash
sudo apt update && sudo apt upgrade -y
sudo apt install -y build-essential cmake git ninja-build libssl-dev
```

### Bước 3: Clone repo

```bash
git clone <repo-url> ~/secoc
cd ~/secoc/Autosar_SecOC
```

### Bước 4: Build thư viện PQC (liboqs)

```bash
cd external/liboqs
mkdir -p build && cd build

cmake -GNinja \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=OFF \
    -DOQS_USE_OPENSSL=ON \
    -DOQS_ENABLE_KEM_ml_kem_768=ON \
    -DOQS_ENABLE_SIG_ml_dsa_65=ON \
    -DCMAKE_C_FLAGS="-mcpu=cortex-a72 -O3 -fPIC" \
    ..

ninja
```

> Bước này build **liboqs** — thư viện mật mã hậu lượng tử từ [Open Quantum Safe](https://openquantumsafe.org/). Chỉ cần chạy **1 lần**.

### Bước 5: Build project SecOC

```bash
cd ~/secoc/Autosar_SecOC
mkdir -p build && cd build
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug -DMCAL_TARGET=PI4 ..
make -j4
```

---

## 4. Chạy Test PQC (Phương án A — 1 Pi4)

Sau khi build xong, bạn có thể chạy test **ngay lập tức** mà không cần bất kỳ phần cứng nào khác.

### 4.1 Chạy toàn bộ test

```bash
cd ~/secoc/Autosar_SecOC/build
ctest --output-on-failure
```

### 4.2 Chạy riêng test PQC quan trọng

```bash
# Test ký + xác thực PQC (ML-DSA-65)
./AuthenticationTests

# Test so sánh PQC vs Classic
./PQC_ComparisonTests

# Test trao đổi khóa ML-KEM-768
./KeyExchangeTests

# Test tạo session key từ shared secret
./KeyDerivationTests

# Test chống replay (freshness counter)
./FreshnessTests
```

### 4.3 Kết quả mong đợi

```
[==========] Running 3 tests from 1 test suite.
[----------] 3 tests from AuthenticationTests
[ RUN      ] AuthenticationTests.authenticate1
  Mode: PQC (ML-DSA-65 Signature)
  Expected: Large PDU with ML-DSA-65 signature (>3000 bytes)
  Actual SecPdu.SduLength: 3320 bytes
  PASS: PQC signature detected
[       OK ] AuthenticationTests.authenticate1 (45 ms)
...
[==========] 3 tests passed.
```

> **Nếu thấy "3 tests passed"** → PQC đã hoạt động trên Pi4 của bạn. ✅

### 4.4 Chạy script test tự động (có báo cáo HTML)

```bash
cd ~/secoc/Autosar_SecOC
bash run_tests.sh
# Báo cáo tại: build/report/SecOC_Test_Report.html
```

---

## 5. Hiểu Kết Quả — PQC Ký & Xác Thực Là Gì?

### Quy trình ký (Tx — bên gửi)

```
Dữ liệu gốc: [RPM = 3500]  (2 byte: 0x0D, 0xAC)
         │
         ▼
    ┌─────────────────────────────┐
    │  1. Lấy freshness counter   │  ← Bộ đếm tăng dần (chống replay)
    │     counter = 0x00000042    │
    │                             │
    │  2. Ghép dữ liệu cần ký:   │
    │     [ID][RPM data][counter] │
    │     = 8 byte                │
    │                             │
    │  3. Ký bằng ML-DSA-65:     │  ← Dùng private key (4,032 byte)
    │     → chữ ký 3,309 byte    │
    │                             │
    │  4. Đóng gói:              │
    │     [Header][Data][FV][Sig] │
    │     = ~3,320 byte           │
    └─────────────────────────────┘
         │
         ▼
    Gửi qua Ethernet (3,320 byte)
```

### Quy trình xác thực (Rx — bên nhận)

```
Nhận 3,320 byte từ Ethernet
         │
         ▼
    ┌─────────────────────────────┐
    │  1. Tách gói:               │
    │     Data, Counter, Chữ ký   │
    │                             │
    │  2. Kiểm tra counter:       │
    │     0x42 > 0x41 (lần trước)?│  ← Nếu ≤ → REPLAY ATTACK → từ chối
    │     ✅ OK                   │
    │                             │
    │  3. Xác thực chữ ký:       │  ← Dùng public key (1,952 byte)
    │     ML-DSA-65 verify        │
    │     ✅ Chữ ký hợp lệ       │
    │                             │
    │  4. Trả dữ liệu gốc:      │
    │     RPM = 3500              │
    └─────────────────────────────┘
```

### Các kịch bản tấn công (test sẵn trong code)

| Tấn công | Cách thực hiện | Kết quả |
|----------|----------------|---------|
| **Giả mạo dữ liệu** | Sửa 1 byte payload | `VERIFY FAIL` — chữ ký không khớp |
| **Phát lại (Replay)** | Gửi lại gói cũ | `VERIFY FAIL` — freshness counter cũ |
| **Sửa chữ ký** | Thay đổi signature | `VERIFY FAIL` — chữ ký sai |

---

## 6. Kiến Trúc Code (Đơn Giản Hóa)

Bạn **không cần hiểu AUTOSAR** — chỉ cần biết 3 lớp:

```
╔══════════════════════════════════════════════════════╗
║  LỚP 3: AUTOSAR SecOC  (bạn KHÔNG cần quan tâm)    ║
║  Quản lý PDU, routing, freshness counter            ║
║  File: source/SecOC/SecOC.c                         ║
╠══════════════════════════════════════════════════════╣
║  LỚP 2: Crypto (Csm → CryIf → PQC)                ║
║  Chuyển tiếp lệnh ký/xác thực xuống PQC            ║
║  File: source/Csm/Csm.c, source/CryIf/CryIf.c     ║
╠══════════════════════════════════════════════════════╣
║  LỚP 1: PQC Module  ← BẠN CHỈ CẦN HIỂU LỚP NÀY  ║
║                                                      ║
║  PQC_Init()          → Khởi tạo liboqs              ║
║  PQC_MLDSA_KeyGen()  → Tạo cặp khóa (pub + secret) ║
║  PQC_MLDSA_Sign()    → Ký tin nhắn → 3,309 byte    ║
║  PQC_MLDSA_Verify()  → Xác thực chữ ký             ║
║                                                      ║
║  File: source/PQC/PQC.c, include/PQC/PQC.h          ║
╠══════════════════════════════════════════════════════╣
║  LỚP 0: liboqs (thư viện bên ngoài)                ║
║  Thuật toán ML-DSA-65, ML-KEM-768 từ NIST           ║
║  Folder: external/liboqs/                            ║
╚══════════════════════════════════════════════════════╝
```

### Tổng quan file quan trọng nhất

| File | Làm gì |
|------|--------|
| `source/PQC/PQC.c` | Code ký + xác thực ML-DSA-65 |
| `source/PQC/PQC_KeyExchange.c` | Trao đổi khóa ML-KEM-768 (handshake) |
| `source/PQC/PQC_KeyDerivation.c` | Tạo session key từ shared secret (HKDF) |
| `source/SecOC/SecOC.c` | Hàm `authenticate_PQC()` và `verify_PQC()` |
| `source/SecOC/FVM.c` | Quản lý freshness counter (chống replay) |
| `source/Csm/Csm.c` | Điều phối crypto: chọn PQC hay Classic |
| `source/CryIf/CryIf.c` | Cầu nối giữa Csm và PQC module |
| `source/SoAd/SoAd_PQC.c` | ML-KEM handshake qua Ethernet (UDP) |
| `source/GUIInterface/GUIInterface.c` | API cho GUI Python (demo) |
| `include/SecOC/SecOC_PQC_Cfg.h` | Bật/tắt chế độ PQC |

---

## 7. Chi Tiết Source Code Theo Từng Lớp

> Mục này liệt kê **tất cả hàm quan trọng** trong từng file — dùng để tra cứu khi đọc code hoặc viết luận văn.

### 7.1 LỚP 1 — PQC Module (Lõi mật mã hậu lượng tử)

Đây là lớp **quan trọng nhất** — nơi thực sự gọi thuật toán ML-DSA-65 và ML-KEM-768 từ liboqs.

#### `source/PQC/PQC.c` + `include/PQC/PQC.h` — Ký số & Xác thực

| Hàm | Mô tả | Input/Output |
|-----|-------|-------------|
| `PQC_Init()` | Khởi tạo, kiểm tra liboqs có bật ML-KEM-768 & ML-DSA-65 | → `PQC_E_OK` nếu sẵn sàng |
| `PQC_MLDSA_KeyGen(KeyPair)` | Tạo cặp khóa ML-DSA-65 | → public 1,952B + secret 4,032B |
| `PQC_MLDSA_Sign(msg, len, secretKey, sig, sigLen)` | **Ký tin nhắn** bằng ML-DSA-65 | → chữ ký ~3,309 byte |
| `PQC_MLDSA_Verify(msg, len, sig, sigLen, publicKey)` | **Xác thực chữ ký** | → `PQC_E_OK` hoặc `PQC_E_VERIFY_FAILED` |
| `PQC_MLDSA_SaveKeys(KeyPair, prefix)` | Lưu khóa ra file (VD: `/etc/secoc/keys/`) | → 2 file: `_public.key`, `_secret.key` |
| `PQC_MLDSA_LoadKeys(KeyPair, prefix)` | Đọc khóa từ file | → nạp vào struct KeyPair |
| `PQC_MLKEM_KeyGen(KeyPair)` | Tạo cặp khóa ML-KEM-768 | → public 1,184B + secret 2,400B |
| `PQC_MLKEM_Encapsulate(pubKey, SharedSecret)` | KEM encapsulate (Bob) | → ciphertext 1,088B + shared secret 32B |
| `PQC_MLKEM_Decapsulate(ct, secretKey, ss)` | KEM decapsulate (Alice) | → shared secret 32B |

**Hằng số kích thước (define trong PQC.h):**

```c
PQC_MLDSA_PUBLIC_KEY_BYTES   = 1952   // Public key ML-DSA-65
PQC_MLDSA_SECRET_KEY_BYTES   = 4032   // Secret key ML-DSA-65
PQC_MLDSA_SIGNATURE_BYTES    = 3309   // Chữ ký ML-DSA-65

PQC_MLKEM_PUBLIC_KEY_BYTES   = 1184   // Public key ML-KEM-768
PQC_MLKEM_SECRET_KEY_BYTES   = 2400   // Secret key ML-KEM-768
PQC_MLKEM_CIPHERTEXT_BYTES   = 1088   // Ciphertext ML-KEM-768
PQC_MLKEM_SHARED_SECRET_BYTES = 32    // Shared secret
```

**Cách dùng đơn giản nhất (không cần AUTOSAR):**

```c
PQC_MLDSA_KeyPairType key;
uint8_t signature[3309];
uint32_t sigLen;
uint8_t message[] = {0x0D, 0xAC};  // RPM = 3500

PQC_Init();
PQC_MLDSA_KeyGen(&key);
PQC_MLDSA_Sign(message, 2, key.SecretKey, signature, &sigLen);
// sigLen ≈ 3309

Std_ReturnType ok = PQC_MLDSA_Verify(message, 2, signature, sigLen, key.PublicKey);
// ok == PQC_E_OK → chữ ký hợp lệ ✅
```

---

#### `source/PQC/PQC_KeyExchange.c` + `include/PQC/PQC_KeyExchange.h` — Trao đổi khóa ML-KEM

Mô phỏng Diffie-Hellman nhưng dùng lattice-based → kháng lượng tử.

| Hàm | Mô tả | Vai trò |
|-----|-------|---------|
| `PQC_KeyExchange_Init()` | Khởi tạo 8 slot peer, tất cả IDLE | Startup |
| `PQC_KeyExchange_Initiate(peerId, pubKey)` | Alice: tạo keypair, xuất public key | Initiator |
| `PQC_KeyExchange_Respond(peerId, peerPubKey, ct)` | Bob: encapsulate → shared secret + ciphertext | Responder |
| `PQC_KeyExchange_Complete(peerId, ct)` | Alice: decapsulate → shared secret | Initiator |
| `PQC_KeyExchange_GetSharedSecret(peerId, ss)` | Lấy shared secret 32B sau handshake | Cả 2 bên |
| `PQC_KeyExchange_GetState(peerId)` | Truy vấn trạng thái | Debug |
| `PQC_KeyExchange_Reset(peerId)` | Xóa session | Cleanup / Rekey |

**State machine:**

```
IDLE → INITIATED → ESTABLISHED
  │                    ↑
  └──── RESPONDED ─────┘
            ↓
          FAILED
```

**Luồng handshake 3 bước:**

```
Alice (Initiator)                        Bob (Responder)
──────────────────                       ─────────────────
1. KeyGen() → pk_A, sk_A
   Gửi pk_A (1,184B) ──────────────────►
                                         2. Encapsulate(pk_A)
                                            → ct (1,088B) + ss_B (32B)
                              ◄──────────── Gửi ct (1,088B)
3. Decapsulate(ct, sk_A)
   → ss_A (32B)
   
   ss_A == ss_B ✅ (cả 2 bên có shared secret giống nhau)
```

---

#### `source/PQC/PQC_KeyDerivation.c` + `include/PQC/PQC_KeyDerivation.h` — Tạo Session Key

Dùng **HKDF-SHA256 (RFC 5869)** để biến shared secret 32B thành 2 session key.

| Hàm | Mô tả |
|-----|-------|
| `PQC_KeyDerivation_Init()` | Khởi tạo 16 slot session key |
| `PQC_DeriveSessionKeys(ss, peerId, keys)` | HKDF: shared secret → 2 key × 32B |
| `PQC_GetSessionKeys(peerId, keys)` | Lấy session key đã lưu |
| `PQC_ClearSessionKeys(peerId)` | Xóa an toàn (memset) |
| `HKDF_Extract(salt, ikm, prk)` | RFC 5869 §2.2: trích xuất entropy |
| `HKDF_Expand(prk, info, okm)` | RFC 5869 §2.3: mở rộng thành key |

**Output từ HKDF:**

```
Shared Secret (32B) ──HKDF──→ EncryptionKey (32B)    ← AES-256
                              AuthenticationKey (32B) ← HMAC key
```

**Salt & Info dùng trong code:**

```c
HKDF_SALT            = "AUTOSAR-SecOC-PQC-v1.0"
HKDF_INFO_ENCRYPTION = "Encryption-Key"
HKDF_INFO_AUTHENTICATION = "Authentication-Key"
```

---

#### `include/PQC/PQC_Visualization.h` — In log đẹp (debug)

File header-only chứa các macro in hex dump, thời gian, so sánh kích thước — dùng cho demo/debug.

---

### 7.2 LỚP 2 — SecOC Module (Đóng gói & Xác thực PDU)

Lớp này **gọi PQC** thông qua Csm/CryIf, đồng thời quản lý freshness counter.

#### `source/SecOC/SecOC.c` — Hàm chính

| Hàm | Mô tả | Gọi bởi |
|-----|-------|---------|
| `SecOC_Init(config)` | Khởi tạo toàn bộ SecOC module | `EcuM_Init()` |
| `SecOC_MainFunctionTx()` | **Vòng lặp ký** — gọi `authenticate_PQC()` cho mỗi PDU Tx | Os scheduler |
| `SecOC_MainFunctionRx()` | **Vòng lặp xác thực** — gọi `verify_PQC()` cho mỗi PDU Rx | Os scheduler |
| `authenticate_PQC(txId, authPdu, secPdu)` | Ký 1 PDU bằng ML-DSA-65 | `MainFunctionTx` |
| `verify_PQC(rxId, secPdu, result)` | Xác thực 1 PDU bằng ML-DSA-65 | `MainFunctionRx` |
| `authenticate(txId, authPdu, secPdu)` | Ký bằng MAC truyền thống (fallback) | Khi PQC tắt |
| `verify(rxId, secPdu, result)` | Xác thực bằng MAC truyền thống | Khi PQC tắt |
| `prepareFreshnessTx(txId, intermediate)` | Lấy freshness counter từ FVM | `authenticate*` |
| `constructDataToAuthenticatorTx(...)` | Ghép `[DataID \| AuthPdu \| Freshness]` | `authenticate*` |
| `parseSecuredPdu(...)` | Tách gói: Header, Data, Freshness, Signature | `verify*` |

**Chuyển đổi PQC/Classic — biên dịch có điều kiện:**

```c
#if (SECOC_USE_PQC_MODE == TRUE)
    result = authenticate_PQC(idx, authPdu, securedPdu);
#else
    result = authenticate(idx, authPdu, securedPdu);
#endif
```

---

#### `source/SecOC/FVM.c` + `include/SecOC/FVM.h` — Freshness Value Manager

Quản lý bộ đếm chống replay attack. Hỗ trợ 100 freshness counter ID.

| Hàm | Mô tả |
|-----|-------|
| `FVM_GetTxFreshness(id, value, length)` | Lấy counter hiện tại cho bên gửi |
| `FVM_GetTxFreshnessTruncData(id, full, fullLen, trunc, truncLen)` | Lấy counter đầy đủ + cắt ngắn |
| `FVM_GetRxFreshness(id, truncValue, truncLen, attempts, fullValue, fullLen)` | **Khôi phục** counter đầy đủ từ bản cắt ngắn |
| `FVM_IncreaseCounter(id)` | Tăng counter (sau khi gửi thành công) |
| `FVM_UpdateCounter(id, value, length)` | Cập nhật counter (sau khi nhận thành công) |

**Logic chống replay:**

```
Bên gửi: counter = 42 → gửi kèm PDU
Bên nhận: counter nhận = 42, counter lưu = 41
  → 42 > 41 → OK ✅ → cập nhật counter = 42
  
Hacker phát lại: counter = 42 (gói cũ)
  → 42 ≤ 42 (counter lưu) → REPLAY → từ chối ❌
```

---

#### `include/SecOC/SecOC_PQC_Cfg.h` — Cấu hình chế độ PQC

```c
#define SECOC_USE_PQC_MODE              TRUE     // TRUE=ML-DSA, FALSE=HMAC
#define SECOC_USE_MLKEM_KEY_EXCHANGE    TRUE     // Bật ML-KEM handshake
#define SECOC_ETHERNET_GATEWAY_MODE     TRUE     // Chế độ Ethernet (bỏ CAN)
#define SECOC_PQC_MAX_PDU_SIZE          8192U    // Buffer cho chữ ký 3,309B
#define PQC_MLDSA_KEY_DIRECTORY         "/etc/secoc/keys/"  // Thư mục khóa
#define SOAD_PQC_REKEY_INTERVAL_CYCLES  360000   // Tự đổi khóa mỗi ~1 giờ
```

---

### 7.3 LỚP 2.5 — Crypto Service Manager + Crypto Interface

Đây là các lớp **trung gian theo chuẩn AUTOSAR** — chuyển tiếp lệnh crypto từ SecOC xuống PQC.

#### `source/Csm/Csm.c` + `include/Csm/Csm.h` — Crypto Service Manager

| Hàm | Mô tả | Gọi xuống |
|-----|-------|-----------|
| `Csm_Init(config)` | Khởi tạo CSM, bootstrap khóa ML-DSA | CryIf_Init() |
| `Csm_SignatureGenerate(job, mode, data, len, sig, sigLen)` | **Ký PQC** (chuyển tiếp) | → CryIf → PQC_MLDSA_Sign |
| `Csm_SignatureVerify(job, mode, data, len, sig, sigLen, result)` | **Xác thực PQC** (chuyển tiếp) | → CryIf → PQC_MLDSA_Verify |
| `Csm_MacGenerate(...)` | Ký MAC truyền thống | → CryIf → AES-CMAC |
| `Csm_MacVerify(...)` | Xác thực MAC truyền thống | → CryIf → AES-CMAC |
| `Csm_KeyExchangeInitiate(job, peer, pk, pkLen)` | ML-KEM bước 1 | → CryIf → PQC_KeyExchange |
| `Csm_KeyExchangeRespond(job, peer, peerPk, ct, ctLen)` | ML-KEM bước 2 | → CryIf → PQC_KeyExchange |
| `Csm_KeyExchangeComplete(job, peer, ct, ctLen)` | ML-KEM bước 3 | → CryIf → PQC_KeyExchange |
| `Csm_DeriveSessionKeys(peer, ss, ssLen)` | HKDF tạo session key | → CryIf → PQC_KeyDerivation |
| `Csm_MldsaBootstrap()` | Tải hoặc tạo cặp khóa ML-DSA | → PQC_MLDSA_LoadKeys/KeyGen |

**Logic chọn provider:**

```
MAC operation        → CRYIF_PROVIDER_CLASSIC  (AES-CMAC 4 byte)
SIGNATURE operation  → CRYIF_PROVIDER_PQC      (ML-DSA-65 3,309 byte)
KEY_EXCHANGE         → CRYIF_PROVIDER_PQC      (ML-KEM-768)
```

---

#### `source/CryIf/CryIf.c` + `include/CryIf/CryIf.h` — Crypto Interface

Lớp trung gian mỏng — route 1:1 xuống PQC module hoặc AES.

| Hàm | → Gọi xuống |
|-----|-------------|
| `CryIf_Init()` | → `PQC_Init()` + `PQC_KeyExchange_Init()` + `PQC_KeyDerivation_Init()` |
| `CryIf_SignatureGenerate(provider, data, ..., sig)` | → `PQC_MLDSA_Sign()` |
| `CryIf_SignatureVerify(provider, data, ..., sig)` | → `PQC_MLDSA_Verify()` |
| `CryIf_MacGenerate(provider, data, ..., mac)` | → `startEncryption()` (AES) |
| `CryIf_KeyExchangeInitiate(peer, pk)` | → `PQC_KeyExchange_Initiate()` |
| `CryIf_KeyExchangeRespond(peer, peerPk, ct)` | → `PQC_KeyExchange_Respond()` |
| `CryIf_KeyExchangeComplete(peer, ct)` | → `PQC_KeyExchange_Complete()` |
| `CryIf_DeriveSessionKeys(ss, peer, keys)` | → `PQC_DeriveSessionKeys()` |

---

### 7.4 LỚP 3 — SoAd PQC (Trao đổi khóa qua mạng)

#### `source/SoAd/SoAd_PQC.c` + `include/SoAd/SoAd_PQC.h`

Quản lý **handshake ML-KEM qua UDP** giữa 2 Pi4.

| Hàm | Mô tả |
|-----|-------|
| `SoAd_PQC_Init()` | Khởi tạo 8 peer, state = IDLE |
| `SoAd_PQC_KeyExchange(peerId, isInitiator)` | Bắt đầu handshake ML-KEM |
| `SoAd_PQC_HandleControlMessage(buf, len)` | **Xử lý gói control** nhận từ UDP |
| `SoAd_PQC_MainFunction()` | Kiểm tra auto-rekey (mỗi 360,000 cycle ≈ 1 giờ) |
| `SoAd_PQC_GetState(peerId)` | Truy vấn trạng thái handshake |
| `SoAd_PQC_ResetSession(peerId)` | Reset session |

**Cấu trúc gói control (UDP):**

```
Byte [0]   : Magic0 = 0x51
Byte [1]   : Magic1 = 0x43
Byte [2]   : Version = 0x01
Byte [3]   : Type (0x01=PUBKEY, 0x02=CIPHERTEXT)
Byte [4]   : PeerId
Byte [5:6] : PayloadLength (little-endian)
Byte [7+]  : Payload (public key 1,184B hoặc ciphertext 1,088B)
```

**State machine:**

```
IDLE → KEY_EXCHANGE_INITIATED → SESSION_ESTABLISHED
                                       │
                           (sau 1 giờ) ↓
                              auto-rekey → IDLE → lặp lại
```

---

### 7.5 LỚP 4 — GUIInterface (API cho Demo)

#### `source/GUIInterface/GUIInterface.c` + `include/GUIInterface/GUIInterface.h`

12 hàm **DLL export** cho phép Python GUI gọi vào C stack:

| Hàm | Mô tả |
|-----|-------|
| `GUIInterface_init()` | Khởi tạo EcuM + SecOC (full AUTOSAR stack) |
| `GUIInterface_authenticate_PQC(id, data, len)` | Ký PQC → trả hex string |
| `GUIInterface_verify_PQC(id)` | Xác thực PQC → trả kết quả |
| `GUIInterface_authenticate(id, data, len)` | Ký MAC truyền thống |
| `GUIInterface_verify(id)` | Xác thực MAC |
| `GUIInterface_getSecuredPDU(id, len)` | Xem gói đã ký (hex) |
| `GUIInterface_getAuthPdu(id, len)` | Xem dữ liệu gốc |
| `GUIInterface_transmit(id)` | Gửi qua Ethernet/CAN |
| `GUIInterface_receive(rxId, rxLen)` | Nhận gói từ mạng |
| `GUIInterface_alterFreshness(id)` | **Giả lập replay attack** |
| `GUIInterface_alterAuthenticator(id)` | **Giả lập tamper attack** |

---

### 7.6 LỚP 0 — Classical Crypto (Fallback)

#### `source/Encrypt/encrypt.c` + `include/Encrypt/encrypt.h`

AES-128-ECB dùng cho **chế độ Classic** (khi `SECOC_USE_PQC_MODE = FALSE`).

| Hàm | Mô tả |
|-----|-------|
| `startEncryption(msg, msgLen, mac, macLen)` | Tạo MAC 4 byte bằng AES |
| `AESEncrypt(msg, expandedKey, encrypted)` | AES-ECB 1 block |
| `KeyExpansion(inputKey, expandedKeys)` | AES key schedule |

> **Không dùng trong chế độ PQC** — chỉ tồn tại để so sánh.

---

### 7.7 Test Files — Kiểm Thử

| File test | Kiểm thử gì | Test cases |
|-----------|-------------|------------|
| `test/AuthenticationTests.cpp` | Ký `authenticate_PQC()`, so sánh kích thước PDU (>3000B vs 8B) | 3 tests |
| `test/PQC_ComparisonTests.cpp` | So sánh PQC vs Classic: config, auth, verify, tamper, MLKEM, timing | ~8 tests |
| `test/KeyExchangeTests.cpp` | Handshake ML-KEM-768 đầy đủ, state machine, multi-peer, error | ~6 tests |
| `test/KeyDerivationTests.cpp` | HKDF RFC 5869, key separation, session storage | ~5 tests |
| `test/FreshnessTests.cpp` | Counter rollover, big-endian compare, replay rejection | ~5 tests |
| `test/DirectTxTests.cpp` | Ký + gửi qua Ethernet (PduR → SoAd) | ~3 tests |
| `test/DirectRxTests.cpp` | Nhận + xác thực từ Ethernet | ~2 tests |
| `test/SoAdPqcTests.cpp` | SoAd PQC control plane, rekeying timer, message parsing | ~4 tests |

---

### 7.8 Sơ Đồ Gọi Hàm Toàn Bộ (Call Graph)

```
                        ┌─────────────────────┐
                        │   SecOC_MainFunctionTx()  │
                        └──────────┬──────────┘
                                   │
                     ┌─────────────┴───────────────┐
                     ▼                             ▼
            authenticate_PQC()              authenticate()
                     │                        (Classic MAC)
         ┌───────────┼─────────────┐
         ▼           ▼             ▼
  prepareFreshness  construct    Csm_SignatureGenerate()
  Tx()              DataToAuth       │
    │               Tx()             ▼
    ▼                          CryIf_SignatureGenerate()
  FVM_GetTx                        │
  Freshness()                      ▼
                              PQC_MLDSA_Sign()
                                   │
                                   ▼
                              OQS_SIG_sign()   ← liboqs
                              (ML-DSA-65)
                              → 3,309 byte chữ ký


                        ┌─────────────────────┐
                        │   SecOC_MainFunctionRx()  │
                        └──────────┬──────────┘
                                   │
                     ┌─────────────┴───────────────┐
                     ▼                             ▼
              verify_PQC()                   verify()
                     │                      (Classic MAC)
         ┌───────────┼─────────────┐
         ▼           ▼             ▼
  parseSecured    construct    Csm_SignatureVerify()
  Pdu()           DataToAuth       │
    │             Rx()             ▼
    ▼                          CryIf_SignatureVerify()
  FVM_GetRx                        │
  Freshness()                      ▼
    │                         PQC_MLDSA_Verify()
    ▼                              │
  (chống replay)                   ▼
                              OQS_SIG_verify()  ← liboqs
                              → PQC_E_OK hoặc PQC_E_VERIFY_FAILED
```

---

## 8. Bảng Kích Thước Khóa & Chữ Ký

| Thành phần | Kích thước | Sử dụng |
|------------|-----------|---------|
| ML-DSA-65 Public Key | 1,952 byte | Phân phối cho bên xác thực |
| ML-DSA-65 Secret Key | 4,032 byte | Giữ bí mật ở bên ký |
| **ML-DSA-65 Chữ ký** | **3,309 byte** | **Đính kèm mỗi tin nhắn** |
| ML-KEM-768 Public Key | 1,184 byte | Trao đổi khóa (handshake) |
| ML-KEM-768 Ciphertext | 1,088 byte | Đóng gói shared secret |
| Shared Secret | 32 byte | Tạo session key |

---

## 9. Thiết Lập Demo 2 Pi4 (Phương án B)

> Chỉ làm bước này **sau khi Phương án A đã chạy thành công**.

### 9.1 Sơ đồ kết nối

```
┌──────────┐   Cáp Ethernet RJ45   ┌──────────┐
│ Pi4 #1   │◄══════════════════════►│ Pi4 #2   │
│ 10.0.0.10│   (nối trực tiếp)     │ 10.0.0.20│
│ Zone Ctrl│                        │ HPC GW   │
│ (nhận &  │                        │ (ký PQC  │
│  xác     │                        │  & gửi)  │
│  thực)   │                        │          │
└──────────┘                        └──────────┘
```

### 9.2 Cấu hình IP tĩnh

**Pi4 #1** — sửa `/etc/dhcpcd.conf`:
```
interface eth0
static ip_address=10.0.0.10/24
```

**Pi4 #2** — sửa `/etc/dhcpcd.conf`:
```
interface eth0
static ip_address=10.0.0.20/24
```

Reboot cả 2 Pi, kiểm tra: `ping 10.0.0.20` (từ Pi4 #1).

### 9.3 Tạo và trao đổi khóa PQC

```bash
# Trên mỗi Pi:
mkdir -p /etc/secoc/keys

# Build xong, chạy tool tạo khóa (nếu có):
cd ~/secoc/Autosar_SecOC/build
# Khóa sẽ được tạo tự động khi chạy lần đầu
# hoặc trao đổi qua ML-KEM handshake khi 2 Pi kết nối
```

### 9.4 File cấu hình role

**Pi4 #1** — tạo `/etc/secoc/zone-controller.env`:
```
SECOC_ROLE=ZC
SECOC_PEER_IP=10.0.0.20
SECOC_PORT=12345
SECOC_USE_PQC_MODE=TRUE
```

**Pi4 #2** — tạo `/etc/secoc/hpc-gateway.env`:
```
SECOC_ROLE=HPC
SECOC_PEER_IP=10.0.0.10
SECOC_PORT=12345
SECOC_CAN_IF=can0
SECOC_USE_PQC_MODE=TRUE
```

### 9.5 Chạy

```bash
# Pi4 #2 (HPC — chạy trước):
source /etc/secoc/hpc-gateway.env
cd ~/secoc/Autosar_SecOC/build
./SecOC

# Pi4 #1 (ZC — chạy sau):
source /etc/secoc/zone-controller.env
cd ~/secoc/Autosar_SecOC/build
./SecOC
```

---

## 10. Thêm CAN Bus + ESP32 (Tùy Chọn)

> Chỉ cần nếu muốn demo **đầy đủ luồng** ESP32 → CAN → Pi4 → Ethernet → Pi4.

### 10.1 Phần cứng thêm

| Linh kiện | Dùng cho |
|-----------|----------|
| Module MCP2515 CAN | Gắn trên Pi4 #2 (SPI) |
| SN65HVD230 CAN Transceiver | Mỗi ESP32 cần 1 cái |
| ESP32-WROOM-32 | ECU giả lập (gửi CAN frame) |
| Điện trở 120Ω × 2 | 2 đầu CAN bus |

### 10.2 Đấu nối MCP2515 → Pi4 #2

| MCP2515 | Pi4 GPIO |
|---------|---------|
| VCC | 3.3V |
| GND | GND |
| CS | GPIO 8 |
| SO | GPIO 9 |
| SI | GPIO 10 |
| SCK | GPIO 11 |
| INT | GPIO 25 |

### 10.3 Bật CAN trên Pi4 #2

Thêm vào `/boot/config.txt`:
```
dtparam=spi=on
dtoverlay=mcp2515-can0,oscillator=8000000,interrupt=25
```

```bash
sudo reboot
sudo ip link set can0 up type can bitrate 500000
candump can0   # Kiểm tra nhận được CAN frame
```

### 10.4 ESP32 firmware (đơn giản)

```c
#include "driver/twai.h"

// Cấu hình CAN 500kbit/s
twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
    GPIO_NUM_21, GPIO_NUM_22, TWAI_MODE_NORMAL);
twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

void app_main() {
    twai_driver_install(&g_config, &t_config, &f_config);
    twai_start();

    twai_message_t msg = {
        .identifier = 0x100,
        .data_length_code = 2,
        .data = {0x0D, 0xAC}  // RPM = 3500
    };

    while (1) {
        twai_transmit(&msg, pdMS_TO_TICKS(100));
        vTaskDelay(pdMS_TO_TICKS(100));  // Gửi mỗi 100ms
    }
}
```

> ESP32 chỉ gửi CAN frame thô — **Pi4 #2 lo toàn bộ việc ký PQC**.

---

## 11. Kịch Bản Demo Cho Bảo Vệ Luận Văn

### Demo 1: PQC hoạt động (2 phút)

1. Chạy `./AuthenticationTests` trên Pi4
2. Cho thấy chữ ký 3,309 byte được tạo thành công
3. Xác thực thành công → `PASS`

### Demo 2: Chống giả mạo (1 phút)

1. Chạy `./PQC_ComparisonTests` — test sẵn trong code sẽ:
   - Ký 1 gói tin
   - Sửa 1 byte trong payload
   - Xác thực → `FAIL` (chữ ký không khớp)

### Demo 3: So sánh PQC vs Classic (1 phút)

1. Trong code có test so sánh sẵn:
   - Classic: chữ ký 4 byte, xử lý ~2 µs
   - PQC: chữ ký 3,309 byte, xử lý ~250 µs
   - Kết luận: PQC lớn hơn nhưng **kháng lượng tử**

### Demo 4 (nâng cao): End-to-end nếu có 2 Pi4

1. ESP32 gửi CAN frame RPM=3500
2. Pi4 #2 ký ML-DSA-65 → gửi 3,320 byte qua Ethernet
3. Pi4 #1 xác thực → hiển thị RPM

---

## 12. Xử Lý Sự Cố

| Vấn đề | Giải pháp |
|--------|-----------|
| `liboqs not found` khi cmake | Chạy lại bước 4 (build liboqs) |
| Test fail `PQC keys may not be available` | Bình thường nếu chưa tạo key file — test vẫn PASS |
| `libssl` not found | `sudo apt install libssl-dev` |
| Build fail trên Pi4 | Kiểm tra: `cmake --version` ≥ 3.16, `gcc --version` ≥ 9 |
| CAN không nhận frame | Oscillator MCP2515 phải 8MHz, có điện trở 120Ω ở 2 đầu bus |
| Pi4 hết RAM khi build | `make -j2` thay vì `make -j4` |

---

## 13. Tóm Tắt

```
Scope TỐI THIỂU để chứng minh PQC hoạt động:
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

   1 × Raspberry Pi 4
 + Build project (30 phút)
 + Chạy: ./AuthenticationTests
 + Kết quả: "PQC signature detected — 3,320 bytes — PASS"

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   → Đủ để chứng minh ML-DSA-65 ký & xác thực
     thành công trên embedded ARM (Pi4)
```

---

*Chi tiết đầy đủ cho demo end-to-end, xem: [wrapup.md](wrapup.md)*
