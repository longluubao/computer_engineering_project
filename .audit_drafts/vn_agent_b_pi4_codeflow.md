# Phản biện đồ án — Pi 4 deployment, build system, code flow (Examiner B)

**Đề tài:** "AUTOSAR SecOC with Post-Quantum Cryptography on a Raspberry Pi 4 Ethernet Gateway"
**Ngày:** 2026-05-02. Tổng số câu: 22.

---

## A. Tính chính đáng của việc triển khai trên Pi 4

### Câu B.1 — Pi 4 có ý nghĩa gì? Lập luận "đại diện gateway/zonal HPC" có chính đáng không?

**Câu hỏi của giáo sư:** "Pi 4 không phải target chuẩn của AUTOSAR Classic. Tại sao chọn Pi 4 dù biết Cortex-A72 + Linux không phải ECU AUTOSAR? Lập luận 'Pi 4 đại diện cho gateway/zonal HPC / Adaptive AUTOSAR' có chính đáng không?"

**Câu trả lời gợi ý cho thí sinh:**

Em xin trả lời thẳng: Pi 4 KHÔNG phải target chuẩn của AUTOSAR Classic Platform, và mã nguồn chạy như một *Linux user-space process* (xem `source/main.c:34` `for(;;) { Os_MainFunction(); }`). Tuy vậy, lựa chọn Pi 4 vẫn có cơ sở:

1. **Phạm vi đồ án là demonstrator.** Em không tuyên bố thiết bị này có thể đưa vào xe sản xuất. Em chỉ chứng minh tính khả thi của tích hợp ML-DSA-65 vào kiến trúc layered theo SWS_SecOC R21-11. Mã nguồn lõi SecOC trong `source/SecOC/SecOC.c` byte-identical giữa Windows/Pi 4 — chỉ MCAL bị thay bằng Linux SocketCAN trong `source/Mcal/Pi4/Can_Pi4.c`.
2. **Industry trajectory:** SDV với HPC zonal/centralized đang dùng Cortex-A nodes chạy Linux/QNX/Adaptive AUTOSAR. Pi 4 với Cortex-A72 quad-core 1.5 GHz là proxy hợp lý cho lớp HPC compute domain controller (KHÔNG phải proxy cho ECU sensor/actuator).
3. **Khả thi cho sinh viên đại học:** AURIX TC4Dx EVB + Tasking compiler license vượt ngân sách. Pi 4 + liboqs miễn phí.

**Phải thừa nhận:** Không có jitter bound real-time, không có HSM tách biệt, không ASIL-D, ML-DSA private key trong filesystem.

**Citations:**
- [Infineon AURIX TC4Dx](https://news.europawire.eu/infineon-unveils-high-performance-aurix-tc4dx-microcontroller-for-next-gen-software-defined-vehicles/eu-press-release/2024/11/06/15/07/28/143544/) (retrieved 2026-05-02)
- [AUTOSAR Classic R21-11 SWS SecOC](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_SecureOnboardCommunication.pdf) (retrieved 2026-05-02)

---

### Câu B.2 — MCP2515 CAN HAT specs và giới hạn

**Câu hỏi của giáo sư:** "Tại sao chọn MCP2515? Specs (8 MHz SPI, 1 Mbps CAN). Có giới hạn gì không?"

**Câu trả lời gợi ý:**

Em chọn MCP2515 vì kernel Linux mainline có driver `mcp251x` sẵn:
```
dtoverlay=mcp2515-can0,oscillator=8000000,interrupt=25,spimaxfrequency=1000000
sudo ip link set can0 up type can bitrate 500000
```

Trong `source/Mcal/Pi4/Can_Pi4.c:117` em mở socket `PF_CAN, SOCK_RAW, CAN_RAW`, bind `ifr_name = "can0"` (line 188-191), dùng `write()/read()` trực tiếp trên `struct can_frame` (line 422-427, 482-484).

**Giới hạn nghiêm trọng:**
1. **MCP2515 chỉ hỗ trợ CAN 2.0B (max 1 Mbps), KHÔNG hỗ trợ CAN-FD.** ML-DSA-65 signature 3309 byte chia ~414 frame × 8 byte, ở 500 kbps mất ≥60-80 ms. Đây là lý do em buộc phải dùng Ethernet (`SECOC_ETHERNET_GATEWAY_MODE = TRUE` trong `SecOC_PQC_Cfg.h:32`) cho PQC PDU.
2. **SPI 1 MHz là bottleneck** cho CAN bus saturation.
3. **Crystal 8 MHz vs 16 MHz** khác nhau giữa các revision HAT.
4. **Latency interrupt:** GPIO 25 IRQ + Linux soft-IRQ scheduling → không real-time.

Mitigation: `Can_Pi4.c:259-263` fallback sang `vcan0` virtual CAN khi không có hardware.

**Citations:**
- [MCP2515 SocketCAN setup](https://www.pragmaticlinux.com/2021/10/can-communication-on-the-raspberry-pi-with-socketcan/) (retrieved 2026-05-02)
- [Beyondlogic MCP2515 Pi](https://www.beyondlogic.org/adding-can-controller-area-network-to-the-raspberry-pi/) (retrieved 2026-05-02)

---

### Câu B.3 — Linux 6.1 không real-time. Tại sao không PREEMPT_RT?

**Câu hỏi của giáo sư:** "Linux 6.1 mainline KHÔNG real-time. Tại sao không dùng PREEMPT_RT? Latency jitter ảnh hưởng SecOC?"

**Câu trả lời gợi ý:**

Em thừa nhận đây là điểm yếu lớn. Pi 4 OS chuẩn dùng `CONFIG_PREEMPT_NONE`/`CONFIG_PREEMPT` (voluntary), không phải `CONFIG_PREEMPT_RT`. `Os_MainFunction()` (`main.c:36`) bị preempt bất kỳ lúc nào.

**Số liệu thực nghiệm tham chiếu:**
- Mainline non-RT Pi 4: max latency 200-300 µs, worst-case spike vài ms.
- PREEMPT_RT 6.6 + isolcpus: max latency ~17-40 µs.

**Tại sao KHÔNG dùng PREEMPT_RT:** Đây là *correctness demonstrator*, không phải *timing demonstrator*. liboqs ML-DSA-65 verify ~120 µs mean trên Pi 4, nhưng Linux scheduler có thể delay 200+ µs → PQC mode latency budget bị chi phối bởi *OS jitter*. PREEMPT_RT đòi hỏi rebuild kernel + tinh chỉnh isolcpus, IRQ affinity — không trong scope.

**Tác động lên SecOC:**
- Freshness counter 64-bit không nhạy cảm jitter.
- Rekey window ~1h: jitter 1ms không ảnh hưởng.
- Verify result delivery: delay tăng tail-latency.

**Citations:**
- [Pi 4B PREEMPT-RT performance test](https://lemariva.com/blog/2019/09/raspberry-pi-4b-preempt-rt-kernel-419y-performance-test) (retrieved 2026-05-02)

---

### Câu B.4 — Pi 4 vs AURIX TC3xx/TC4Dx — performance gap

**Câu hỏi của giáo sư:** "Pi 4 vs AURIX TC3xx/TC4Dx — performance gap thực tế?"

**Câu trả lời gợi ý:**

| Chỉ tiêu | Pi 4 (Cortex-A72 1.5 GHz) | AURIX TC3xx (TriCore 300 MHz) | AURIX TC4Dx (TriCore 500 MHz + CSRM) |
|---|---|---|---|
| ML-DSA-65 Sign | ~250 µs (liboqs) | 5-10 ms ước tính (SW thuần) | µs-range qua CSRM hardware |
| ML-DSA-65 Verify | ~120 µs | 2-5 ms ước tính | µs-range hardware |
| ASIL safety | Không | ASIL-D capable | ASIL-D, lock-step |
| Real-time guarantee | Không | Có | Có |

Pi 4 thực ra **NHANH hơn** TriCore TC3xx về raw compute (3× clock + 64-bit + NEON), nhưng *thua hoàn toàn về determinism*. AURIX TC4Dx có Crypto Satellites cho PQC mới đạt µs-class.

Em thừa nhận: kết quả benchmark Pi 4 KHÔNG phải proxy hợp lệ cho timing trên ECU thực. Số liệu chỉ chứng minh "PQC khả thi về raw cycles", không "PQC đáp ứng deadline ECU".

**Citations:**
- [Infineon AURIX TC4x — CSRM 10x perf](https://www.infineon.com/assets/row/public/documents/10/156/infineon-tc4x-overview-productpresentation-en.pdf) (retrieved 2026-05-02)
- [Electronics Weekly — PQC in automotive MCUs](https://www.electronicsweekly.com/news/products/micros/post-quantum-cryptography-support-in-automotive-mcus-2024-11/) (retrieved 2026-05-02)

---

### Câu B.5 — NEON SIMD trong liboqs

**Câu hỏi của giáo sư:** "Cross-build aarch64 với `-mcpu=cortex-a72`, NEON SIMD. Đoạn nào của liboqs hưởng lợi từ NEON? Có bench-compare không?"

**Câu trả lời gợi ý:**

liboqs từ 0.13 hỗ trợ chính thức ba variant: portable C, AVX2 (x86), và **AArch64 NEON**. AArch64 ML-KEM được port từ project `mlkem-native` với formal verification.

**Đoạn được hưởng lợi từ NEON:**
- **NTT (Number Theoretic Transform)** — bottleneck của ML-KEM/ML-DSA. Four-lane Montgomery multiplication chạy trên NEON 128-bit registers.
- **Polynomial multiplication/addition** — vectorize 8 × int16 trong một instruction.

**Em CHƯA bench-compare portable C vs NEON — gap thừa nhận.** Số liệu (~250 µs sign, ~120 µs verify) là kết quả với liboqs default (sẽ chọn AArch64 NEON nếu CMake detect được). Cần thêm bench với `-DOQS_DIST_BUILD=OFF -DOQS_OPT_TARGET=generic` để loại NEON optimization.

Cortex-A72 có crypto extensions (`+crypto`): AES, SHA1, SHA2 instructions. Nhưng SHAKE/Keccak (dùng trong ML-DSA) không có hardware ARMv8 instruction.

**Citations:**
- [liboqs releases — formally verified AArch64 ML-KEM](https://github.com/open-quantum-safe/liboqs/releases) (retrieved 2026-05-02)
- [MDPI — ARMv8/NEON Optimization on Cortex-A72](https://www.mdpi.com/2079-9292/15/7/1456) (retrieved 2026-05-02)

---

### Câu B.6 — Linux user-space thay vì bare-metal

**Câu hỏi của giáo sư:** "Tại sao chạy Linux user-space process thay vì bare-metal? Có thể bị 'đập tan' không?"

**Câu trả lời gợi ý:**

Em không phòng vệ điểm này — em chấp nhận đây là giới hạn căn bản và xếp vào Future Work.

**Sự thật:**
1. `source/main.c:34` chạy `for(;;) { Os_MainFunction(); }` trong một process Linux. Đây không phải bare-metal AUTOSAR OS.
2. Toàn bộ BSW từ `EcuM_Init()` → `EcuM_StartupTwo()` → `Os_Init()` chỉ là *static init pattern* mô phỏng AUTOSAR startup.
3. ML-DSA private key đọc từ filesystem.
4. Không có MPU/MMU enforcement giữa AUTOSAR partitions.

**Tại sao CHẤP NHẬN ĐƯỢC trong scope đồ án:**
1. Đồ án validate SOFTWARE ARCHITECTURE — chứng minh layered design tuân thủ, PQC integration không phá vỡ API contract.
2. Bare-metal port là engineering effort, không phải research effort. SecOC core (`SecOC.c`) **không có Linux-specific code**. Linux dependency tập trung trong `Mcal/Pi4/Can_Pi4.c`, `Ethernet/ethernet.c`, `PQC.c:280-297`.
3. Adaptive AUTOSAR (R20-11+) cũng chạy trên Linux user-space (POSIX OS). Pi 4 demonstrator gần với Adaptive spirit hơn Classic SecOC.

**Citations:**
- [AUTOSAR Adaptive Platform R23-11](https://www.autosar.org/standards/adaptive-platform) (retrieved 2026-05-02)

---

## B. Hệ build & dual-platform

### Câu B.7 — CMake `MCAL_TARGET=PI4` tách build

**Câu hỏi của giáo sư:** "CMake `MCAL_TARGET=PI4` chính xác tách build path như thế nào? Defend SecOC core source byte-identical giữa Windows và Pi 4."

**Câu trả lời gợi ý:**

`CMakeLists.txt`:

1. **Default selection** (line 17-23): UNIX → "PI4", else "SIM".
2. **Source filtering trên Linux khi PI4** (line 60-71):
   ```cmake
   if(MCAL_TARGET STREQUAL "PI4")
       list(REMOVE_ITEM SCR_FILES "source/Can/Can.c")
   else()
       file(GLOB_RECURSE MCAL_PI4_FILES "source/Mcal/Pi4/*.c")
       foreach(_f ${MCAL_PI4_FILES}) list(REMOVE_ITEM SCR_FILES "${_f}") endforeach()
   endif()
   ```
3. **Compile-time define** (line 119-123): `MCAL_TARGET=MCAL_TARGET_PI4`.
4. **Windows always SIM** (line 26-44): remove `ethernet.c`/`scheduler.c`, replace bằng `ethernet_windows.c`.

**Defense byte-identical:**
`source/SecOC/SecOC.c`, `source/Csm/Csm.c`, `source/CryIf/CryIf.c`, `source/PQC/PQC.c` — KHÔNG chứa `#ifdef LINUX`/`#ifdef WIN32`/`#ifdef MCAL_TARGET_PI4` cho SecOC logic. Chỉ có `#if defined(LINUX)` trong `PQC.c:21,282` cho `mkdir/stat`. Điều này chứng minh **SecOC layer thực sự hardware-agnostic**, đúng tinh thần AUTOSAR layered.

**Citations:**
- [CMake `target_compile_definitions`](https://cmake.org/cmake/help/latest/command/target_compile_definitions.html) (retrieved 2026-05-02)

---

### Câu B.8 — Tại sao chỉ enable ML-KEM-768 và ML-DSA-65?

**Câu hỏi của giáo sư:** "liboqs build — tại sao chỉ enable hai thuật toán?"

**Câu trả lời gợi ý:**

liboqs default enable TẤT CẢ NIST PQC candidates → tổng .a file ~30+ MB.

**Lý do em chỉ enable hai:**
1. **NIST FIPS 203 (Aug 2024)** standardize ML-KEM, **FIPS 204** standardize ML-DSA. Đây là hai thuật toán *production-grade*.
2. **Code size:** Build cho future-portability sang AURIX TC4Dx (4-8 MB Flash). Enable hết liboqs ~25 MB → KHÔNG vừa.
3. **Attack surface:** Mỗi thuật toán enable tăng CVE history. Disable không dùng = principle of least privilege.
4. **Build time:** Compile liboqs đầy đủ ~15 phút trên Pi 4, subset ~2 phút.

Configuration: `OQS_KEM_alg_ml_kem_768` (`PQC.c:68`), `OQS_SIG_alg_ml_dsa_65` (`PQC.c:74`).

**Citations:**
- [NIST FIPS 204](https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.204.pdf) (retrieved 2026-05-02)
- [Open Quantum Safe — ML-KEM](https://openquantumsafe.org/liboqs/algorithms/kem/ml-kem.html) (retrieved 2026-05-02)

---

### Câu B.9 — Hai implementation Ethernet — design pattern

**Câu hỏi của giáo sư:** "Tại sao 2 implementation Ethernet (`ethernet.c` Linux + `ethernet_windows.c`)? Có ifdef không?"

**Câu trả lời gợi ý:**

Em chọn pattern "platform-specific source file selection" — tách hai file riêng thay vì `#ifdef WIN32`/`#ifdef LINUX` trong file chung.

`CMakeLists.txt`:
- Windows (line 30-39): remove `ethernet.c`, include `ethernet_windows.c`.
- Linux (line 49-50): remove `ethernet_windows.c`, giữ `ethernet.c`.

`include/Ethernet/ethernet.h` chỉ một header chung, expose 4 function: `EthDrv_Init()`, `EthDrv_Send()`, `EthDrv_Receive()`, `EthDrv_ReceiveMainFunction()`.

Hai implementation cùng tên function nhưng API platform khác:
- `ethernet.c:78-80,95` — POSIX `socket()`, `connect()`, `send()`.
- `ethernet_windows.c` — Winsock2 `WSAStartup()`.

**Lý do:**
1. Readability: mỗi file thuần một platform.
2. MISRA-friendly: Rule 20.9 khuyến nghị tránh conditional compilation phức tạp.
3. Comment trong `ethernet.c:40`: `/* cppcheck-suppress misra-c2012-8.6 ; platform-specific, only one file compiled per target */`.

**Citations:**
- [POSIX vs Winsock](https://learn.microsoft.com/en-us/windows/win32/winsock/porting-socket-applications-to-winsock) (retrieved 2026-05-02)

---

## C. Luồng mã — TX path (PQC)

### Câu B.10 — CODE-TRACE: TX flow PQC chi tiết line-by-line

**Câu hỏi của giáo sư:** "Walkthrough Tx flow PQC từ Application gọi `SecOC_IfTransmit` đến bytes ra socket Ethernet. Trace từng function call với line number."

**Câu trả lời gợi ý:**

**Bước 1. Application enqueue PDU**
```
SecOC_IfTransmit(TxPduId, PduInfoPtr)         @ source/SecOC/SecOC.c:189
├── kiểm tra SecOCState != SECOC_INIT          @ line 196
├── kiểm tra PduInfoPtr non-NULL               @ line 202
├── kiểm tra SecOC_IsValidTxPduId              @ line 208
├── memcpy(authpdu->SduDataPtr, PduInfoPtr->SduDataPtr, SduLength)  @ line 218
├── authpdu->SduLength = PduInfoPtr->SduLength @ line 220
└── return E_OK                                 @ line 234
```

**Bước 2. MainFunction gọi authenticate_PQC**
```
SecOC_MainFunctionTx() iterates over each TxPduProcessing
└── #if (SECOC_USE_PQC_MODE == TRUE) → authenticate_PQC(idx, authPdu, secPdu)

authenticate_PQC(TxPduId, AuthPdu, SecPdu)    @ source/SecOC/SecOC.c:1429
├── prepareFreshnessTx(TxPduId, &SecOCIntermediate)        @ line 1439
├── constructDataToAuthenticatorTx(...)                    @ line 1446
├── Csm_SignatureGenerate(jobId, 0, DataToAuth, DataToAuthLen, AuthenticatorPtr, &signatureLen)  @ line 1451
├── Memory layout: Header | AuthPDU | Freshness | Signature
│   - memcpy header                            @ line 1479
│   - memcpy AuthPdu payload                   @ line 1484
│   - memcpy MsgFreshness                      @ line 1492
│   - memcpy Authenticator (signature)         @ line 1496
└── return result                               @ line 1549
```

**Bước 3. prepareFreshnessTx — lookup FVM**
```
prepareFreshnessTx(TxPduId, SecOCIntermediate) @ source/SecOC/SecOC.c:312
├── memset(Freshness, 0, ...)                 @ line 320 (SWS_SecOC_00220)
├── FreshnessLenBits = SecOCFreshnessValueLength @ line 323
├── if SECOC_CFUNC && SecOCProvideTxTruncatedFreshnessValue == TRUE:
│       SecOC_GetTxFreshnessTruncData(...)    @ line 333 (SWS_SecOC_00094)
│       └── FVM_GetTxFreshnessTruncData(...) → 64-bit counter ở PQC mode
└── return result                              @ line 355
```

**Bước 4. constructDataToAuthenticatorTx — concat layout**
```
@ source/SecOC/SecOC.c:251
├── DataToAuthLen = 0                         @ line 260
├── memcpy(DataToAuth[0], SecOCDataId, sizeof(SecOCDataId))  @ line 263
├── memcpy(DataToAuth+DataToAuthLen, AuthPdu->SduDataPtr, AuthPdu->SduLength)  @ line 267
└── memcpy(DataToAuth+DataToAuthLen, Freshness, FreshnesslenBytes)  @ line 271
```
Layout: **DataId(2) || Payload(N) || Freshness(8 byte ở PQC)**.

**Bước 5. Csm_SignatureGenerate → CryIf → liboqs**
```
Csm_SignatureGenerate(jobId, mode, dataPtr, dataLength, sigPtr, sigLengthPtr)  @ source/Csm/Csm.c:806
├── Csm_PQC_EnsureReady() → load ML-DSA keys  @ line 825
│   ├── CSM_MLDSA_BOOTSTRAP_DEMO_FILE_AUTO   @ line 453
│   ├── CSM_MLDSA_BOOTSTRAP_FILE_STRICT      @ line 468
│   └── CSM_MLDSA_BOOTSTRAP_PROVISIONED/HSM  @ line 478
├── Csm_GetJobContext(jobId)                  @ line 830
├── provider = Csm_SelectProvider             @ line 868
├── CryIf_SignatureGenerate(...)               @ source/CryIf/CryIf.c:107
│   └── PQC_MLDSA_Sign(dataPtr, dataLength, secretKey, sig, &sigLen)  @ CryIf.c:124
│       └── source/PQC/PQC.c:415
│           ├── OQS_SIG_new(OQS_SIG_alg_ml_dsa_65)  @ line 436
│           ├── OQS_SIG_sign(...)              @ line 443
│           └── return PQC_E_OK                @ line 458
```

**Bước 6. Quay về SecOC, ghép Secured PDU** (line 1472-1497):
```
[byte 0..headerLen-1]    Header (PduLength field)
[+authLen bytes]         Authentic PDU payload
[+freshLen bytes]        Truncated Freshness Value
[+3309 bytes]            ML-DSA-65 Signature
```

**Bước 7-8. PduR routes → SoAd**
```
PduR_SecOCTransmit(securedPduId, securedInfo)
└── SoAd_IfTransmit(TxPduId, PduInfoPtr)        @ source/SoAd/SoAd.c:654
    ├── kiểm tra SoAd_Initialized               @ line 663-689
    ├── SoAd_FindOrCreateSoCon(TxPduId)         @ line 693
    ├── SoAd_GetPduRouteConfig(TxPduId, &RouteCfgPtr)  @ line 701
    └── SoAd_TcpIpTransmit(...)                 @ line 727
```

**Bước 9. Linux socket send()**
```
EthDrv_Send(id, data, dataLen)                  @ source/Ethernet/ethernet.c:74
├── socket(AF_INET, SOCK_STREAM, 0)             @ line 80
├── connect(network_socket, server_address)     @ line 95
├── sendData = data || id_bytes (big-endian id) @ line 106-111
└── send(network_socket, sendData, totalLen, 0) → kernel
```

**Tổng kết flow:** App → SecOC enqueue (189) → MainFunction → authenticate_PQC (1429) → prepareFreshnessTx (312) → constructDataToAuthenticatorTx (251) → Csm_SignatureGenerate (Csm.c:806) → CryIf_SignatureGenerate (CryIf.c:107) → PQC_MLDSA_Sign (PQC.c:415) → OQS_SIG_sign (liboqs) → ghép PDU (1472-1497) → PduR_SecOCTransmit → SoAd_IfTransmit (SoAd.c:654) → SoAd_TcpIpTransmit → EthDrv_Send (ethernet.c:74) → Linux `send()`.

**Citations:**
- [NIST FIPS 204 — sign procedure](https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.204.pdf) (retrieved 2026-05-02)
- [AUTOSAR R21-11 SWS_SecOC_00031](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_SecureOnboardCommunication.pdf) (retrieved 2026-05-02)

---

## D. Luồng mã — RX path

### Câu B.11 — CODE-TRACE: RX flow PQC chi tiết

**Câu hỏi của giáo sư:** "Walkthrough Rx flow PQC. Linux socket nhận → SoAd_RxIndication → parseSecuredPdu → freshness check → verify_PQC → PduR."

**Câu trả lời gợi ý:**

**Bước 1-2. Linux receive → SoAdTp_RxIndication**
```
EthDrv_ReceiveMainFunction() (called periodically)
└── recv(sockfd, buffer, BUS_LENGTH_RECEIVE=4096, 0) → callback đến SoAd

SoAdTp_RxIndication(RxPduId, PduInfoPtr)        @ source/SoAd/SoAd.c:1178
├── PduInfoPtr non-NULL, RxPduId valid          @ line 1184
├── SduLength <= SECOC_SECPDU_MAX_LENGTH        @ line 1205
├── enqueue vào SoAdTp_RxQueueData              @ line 1219
├── #if SECOC_USE_PQC_MODE == TRUE:
│       authInfoLength = PQC_MLDSA_SIGNATURE_BYTES (3309)  @ line 1260
│   else:
│       authInfoLength = BIT_TO_BYTES(SoAdAuthInfoLengthBits)  @ line 1262
└── SoAdTp_secureLength_Recieve = AuthHeadlen + authenticLength
                                + truncFreshness + authInfoLength  @ line 1265
```

**Bước 3-4. SecOC_MainFunctionRx → verify_PQC**
```
SecOC_MainFunctionRx()                          @ source/SecOC/SecOC.c:1663
└── for each RxPduId:
    ├── #if SECOC_USE_PQC_MODE == TRUE:
    │       authInfoLength = PQC_MLDSA_SIGNATURE_BYTES   @ line 1688
    ├── if securedPdu->SduLength >= securePduLength:     @ line 1696
    │   #if SECOC_USE_PQC_MODE == TRUE:
    │       result = verify_PQC(idx, securedPdu, &result_ver)  @ line 1700
    └── nếu E_OK + IsTpAuthenticPdu: PduR_SecOCTpStartOfReception  @ line 1716
```

**Bước 5. verify_PQC**
```
verify_PQC(RxPduId, SecPdu, verification_result) @ source/SecOC/SecOC.c:1556
├── parseSecuredPdu(RxPduId, SecPdu, &SecOCIntermediate)  @ line 1565
├── *verification_result = SECOC_NO_VERIFICATION  @ line 1567
├── if SecOCSecuredRxPduVerification == TRUE:
│   ├── /* FRESHNESS CHECK FIRST */
│   │   if freshnessResult == E_BUSY || E_NOT_OK:
│   │       *verification_result = SECOC_FRESHNESSFAILURE  @ line 1587
│   │       return                                         @ line 1589
│   ├── constructDataToAuthenticatorRx(...)     @ line 1593
│   └── Csm_SignatureVerify(jobId, SINGLECALL, DataToAuth, DataToAuthLen,
│                           SecOCIntermediate.mac, signatureLen, &verify_var)  @ line 1604
├── if Sig_verify == E_OK: SECOC_VERIFICATIONSUCCESS  @ line 1645
├── memcpy(authPdu, SecOCIntermediate.authenticPdu, ...)  @ line 1650
└── FVM_UpdateCounter(...)                       @ line 1657 — advance freshness
```

**Bước 6. parseSecuredPdu — phân tách [Header | Auth | Freshness | Signature]**
```
parseSecuredPdu(RxPduId, SecPdu, SecOCIntermediate)  @ source/SecOC/SecOC.c:1163
├── SecCursor = 0                              @ line 1169
├── headerLen = SecOCAuthPduHeaderLength       @ line 1172
├── memcpy(authenticPduLen, SecPdu+SecCursor, headerLen)  @ line 1177
├── memcpy(authenticPdu, SecPdu+SecCursor, authenticPduLen)  @ line 1187
├── /* FRESHNESS LOOKUP */
│   freshnessResult = SecOC_GetRxFreshness(...)  @ line 1199 (SWS_SecOC_00250)
└── #if SECOC_USE_PQC_MODE == TRUE:
        actualSignatureLen = SecPdu->SduLength - SecCursor  @ line 1213
        memcpy(SecOCIntermediate->mac, SecPdu+SecCursor, actualSignatureLen)  @ line 1215
```

**Bước 8. constructDataToAuthenticatorRx — REBUILD layout giống TX**
```
@ source/SecOC/SecOC.c:1287
├── DataToAuthLen = 0                          @ line 1296
├── memcpy(DataToAuth, SecOCDataId, sizeof(SecOCDataId))  @ line 1298
├── memcpy(DataToAuth+DataToAuthLen, authenticPdu, authenticPduLen)  @ line 1303
└── memcpy(DataToAuth+DataToAuthLen, freshness, BIT_TO_BYTES(freshnessLenBits))  @ line 1307
```

**Bước 9. Csm_SignatureVerify → CryIf → liboqs**
```
Csm_SignatureVerify(...)                       @ source/Csm/Csm.c:881
├── Csm_PQC_EnsureReady()                     @ line 901
├── CryIf_SignatureVerify(...)                @ source/CryIf/CryIf.c:132
│   └── PQC_MLDSA_Verify(...)                  @ source/PQC/PQC.c:466
│       ├── OQS_SIG_new(OQS_SIG_alg_ml_dsa_65)  @ line 486
│       ├── OQS_SIG_verify(...)                 @ line 493
│       └── return PQC_E_OK / PQC_E_VERIFY_FAILED  @ line 502/505
└── *verifyResultPtr = CRYPTO_E_VER_OK / CRYPTO_E_VER_NOT_OK
```

**Citations:**
- [SWS_SecOC R21-11 — Reception flow](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_SecureOnboardCommunication.pdf) (retrieved 2026-05-02)

---

### Câu B.12 — Tại sao FRESHNESS CHECK chạy TRƯỚC SIGNATURE VERIFY?

**Câu hỏi của giáo sư:** "Tại sao freshness check chạy trước verify? Defend defense-in-depth."

**Câu trả lời gợi ý:**

`source/SecOC/SecOC.c:1583-1590`:
```c
if((SecOCIntermediate.freshnessResult == E_BUSY) ||
   (SecOCIntermediate.freshnessResult == E_NOT_OK))
{
    if(SecOCIntermediate.freshnessResult == E_NOT_OK) {
        *verification_result = SECOC_FRESHNESSFAILURE;
    }
    return SecOCIntermediate.freshnessResult;
}
/* SAU đó mới: */
constructDataToAuthenticatorRx(RxPduId, &SecOCIntermediate);
Std_ReturnType Sig_verify = Csm_SignatureVerify(...);
```

**Rationale (defense-in-depth):**
1. **Cost asymmetry:**
   - Freshness check: O(1) — so sánh counter với window. Nanoseconds.
   - Signature verify ML-DSA-65: ~120 µs trên Pi 4. **6 orders of magnitude đắt hơn**.
2. **DoS prevention:** Attacker spam network với PDU chứa signature bậy. Verify trước → CPU verify rồi mới reject vì freshness sai → easy DoS. Verify freshness trước = early rejection cheap.
3. **Replay attack defense:** Replay PDU đã capture (signature vẫn valid). Freshness counter monotonic → out-of-window → reject ngay.
4. **AUTOSAR SWS_SecOC_00250** chỉ định freshness check là một bước riêng biệt.

**Constant-time concern:** Code dùng `Csm_ConstantTimeMemcmp` (`Csm.c:114-124`) cho MAC comparison, ngăn timing side-channel.

**Citations:**
- [ISO/SAE 21434](https://www.iso.org/standard/70918.html) (retrieved 2026-05-02)

---

## E. Luồng mã — ML-KEM key exchange

### Câu B.13 — CODE-TRACE: ML-KEM handshake và architectural smell

**Câu hỏi của giáo sư:** "Trace ML-KEM handshake. Tại sao logic key exchange nằm ở SoAd? (architectural smell)"

**Câu trả lời gợi ý:**

**Architectural critique tự thừa nhận:** ML-KEM key exchange **nên** nằm ở Csm hoặc Key Management module riêng, KHÔNG ở SoAd. Việc đặt ở `source/SoAd/SoAd_PQC.c` vi phạm separation of concerns. Em đặt ở đây vì SoAd có sẵn TcpIp socket framing.

**Trace handshake:**

**Bước 1. Initiator rekey trigger**
```
SoAd_PQC_MainFunction()                       @ source/SoAd/SoAd_PQC.c:175
├── #if SOAD_PQC_REKEY_INTERVAL_CYCLES > 0:
│   for each peer:
│     if state == SOAD_PQC_STATE_SESSION_ESTABLISHED:
│       SoAd_PQC_RekeyCycles[peerId]++       @ line 189
│       if RekeyCycles >= 360000U:           @ line 191
│         Csm_KeyExchangeReset(peerId)       @ line 197
│         SoAd_PQC_KeyExchange_Initiator(peerId)  @ line 202

SoAd_PQC_KeyExchange_Initiator(PeerId)        @ source/SoAd/SoAd_PQC.c:221
├── Csm_KeyExchangeInitiate(0U, PeerId, publicKey, &actualSize)  @ line 228
├── SoAd_PQC_SendControlMessage(PeerId, SOAD_PQC_CTRL_TYPE_PUBKEY, ...)  @ line 235
│   └── đóng gói magic 0x51 0x43, version, type=0x01, gửi UDP
└── State → SOAD_PQC_STATE_KEY_EXCHANGE_INITIATED  @ line 227
```

**Bước 2. Csm → CryIf → PQC_KeyExchange_Initiate**
```
CryIf_KeyExchangeInitiate(peerId, publicValuePtr)      @ source/CryIf/CryIf.c:156
└── PQC_KeyExchange_Initiate(peerId, publicKey)        @ source/PQC/PQC_KeyExchange.c:62
    ├── PQC_MLKEM_KeyGen(&session->LocalKeyPair)        @ line 88
    │   └── OQS_KEM_keypair(kem, PublicKey, SecretKey)  (PQC.c)
    ├── memcpy(PublicKey, session->LocalKeyPair.PublicKey, 1184 byte)  @ line 97
    └── session->State = PQC_KE_STATE_INITIATED         @ line 100
```

**Bước 3. Responder nhận**
```
SoAd_RxIndication() → SoAd_PQC_HandleControlMessage(BufPtr, Length)  @ source/SoAd/SoAd_PQC.c:283
├── parse magic + version                     @ line 296
├── messageType = 0x01 (PUBKEY)               @ line 303
└── Csm_KeyExchangeRespond(...)               @ line 326
    └── PQC_KeyExchange_Respond(peerId, PeerPublicKey, Ciphertext)  @ source/PQC/PQC_KeyExchange.c:112
        ├── PQC_MLKEM_Encapsulate(PeerPublicKey, &shared_secret_data)  @ line 136
        │   └── OQS_KEM_encaps(kem, ciphertext, sharedSecret, peerPublicKey)
        ├── memcpy(session->SharedSecret, sharedSecret, 32 byte)  @ line 145
        └── session->State = PQC_KE_STATE_ESTABLISHED   @ line 150
```

**Bước 4. Initiator decapsulate**
```
PQC_KeyExchange_Complete(peerId, ciphertext)  @ source/PQC/PQC_KeyExchange.c:162
├── PQC_MLKEM_Decapsulate(Ciphertext, secretKey, sharedSecret)  @ line 188
│   └── OQS_KEM_decaps(kem, sharedSecret, ciphertext, secretKey)  @ source/PQC/PQC.c:211
├── session->State = PQC_KE_STATE_ESTABLISHED  @ line 205
└── memset(session->LocalKeyPair.SecretKey, 0, ...)  @ line 211 /* zeroize */
```

**Architectural smell tự thừa nhận:**
1. SoAd biết về PQC — vi phạm Single Responsibility Principle.
2. Magic numbers (0x51 0x43) — control message protocol tự định nghĩa, không có spec. Production cần TLS 1.3 hybrid.
3. `peerId` là `uint8` — giới hạn 256 peers.

**Citations:**
- [NIST FIPS 203 — ML-KEM](https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.203.pdf) (retrieved 2026-05-02)
- [Open Quantum Safe — ML-KEM API](https://openquantumsafe.org/liboqs/algorithms/kem/ml-kem.html) (retrieved 2026-05-02)

---

### Câu B.14 — HKDF-SHA256: salt và info có domain-separated không?

**Câu hỏi của giáo sư:** "HKDF-SHA256 derivation. Salt và info có domain-separated không? RFC 5869 compliance?"

**Câu trả lời gợi ý:**

`source/PQC/PQC_KeyDerivation.c` implement HKDF theo RFC 5869.

**Constants tại line 30-32:**
```c
static const char* HKDF_SALT = "AUTOSAR-SecOC-PQC-v1.0";
static const char* HKDF_INFO_ENCRYPTION = "Encryption-Key";
static const char* HKDF_INFO_AUTHENTICATION = "Authentication-Key";
```

**Domain separation:**
1. **Salt cố định** "AUTOSAR-SecOC-PQC-v1.0" — versioned domain separator.
2. **Info string khác nhau** cho hai loại key — đúng RFC 5869 §3.1.

**Implementation:**
- **HKDF-Extract** (line 71-82): `PRK = HMAC-SHA256(salt, IKM)` với IKM = ML-KEM-768 shared secret 32 byte.
- **HKDF-Expand** (line 91-148): `T(i) = HMAC-SHA256(PRK, T(i-1) || info || counter)`. Counter byte 1-indexed.

**HMAC** dùng OpenSSL 3.0 EVP_MAC API (line 47-65) thay vì legacy.

**Critique:**
1. **Salt nên KHÔNG cố định** — RFC 5869 §3.1 khuyến nghị salt random. Em dùng cố định để hai bên derive cùng PRK mà không cần truyền salt — trade-off phổ biến trong protocol nhúng.
2. **Không có per-session nonce trong info** — hai session khác nhau cùng peerId sẽ derive cùng key nếu shared secret giống. Tuy nhiên ML-KEM tạo shared secret random mỗi lần encap → collision negligible.
3. **Versioning** "v1.0" cho phép cryptographic agility.

**Citations:**
- [RFC 5869 HKDF](https://www.rfc-editor.org/rfc/rfc5869) (retrieved 2026-05-02)
- [OpenSSL 3.0 EVP_MAC](https://www.openssl.org/docs/man3.0/man3/EVP_MAC.html) (retrieved 2026-05-02)

---

## F. Cấu hình compile-time

### Câu B.15 — Ba flags `SECOC_USE_PQC_MODE`, `SECOC_USE_MLKEM_KEY_EXCHANGE`, `SECOC_ETHERNET_GATEWAY_MODE`

**Câu hỏi của giáo sư:** "Mối quan hệ giữa 3 flags?"

**Câu trả lời gợi ý:**

`include/SecOC/SecOC_PQC_Cfg.h:21-32`:
```c
#define SECOC_USE_PQC_MODE              TRUE   /* line 21 */
#define SECOC_USE_MLKEM_KEY_EXCHANGE    TRUE   /* line 27 */
#define SECOC_ETHERNET_GATEWAY_MODE     TRUE   /* line 32 */
```

**Phân cấp:**
```
SECOC_USE_PQC_MODE = TRUE
  ├── BẮT BUỘC: SecOC dùng Csm_SignatureGenerate/Verify thay Csm_MacGenerate/Verify
  │   (verify_PQC ở line 1556 thay vì verify ở line 1314)
  ├── KHUYẾN NGHỊ: SECOC_USE_MLKEM_KEY_EXCHANGE = TRUE → forward secrecy
  └── KHUYẾN NGHỊ: SECOC_ETHERNET_GATEWAY_MODE = TRUE
      └── ML-DSA signature 3309 byte → CAN không khả thi (414 frame)
```

**Trong code các flag được dùng ở:**
- `SECOC_USE_PQC_MODE`: `SecOC.c:1598, 1687-1691, 1699-1703, 1211-1226`; `SoAd.c:1259-1263`.
- `SECOC_USE_MLKEM_KEY_EXCHANGE`: kích hoạt SoAd_PQC + PQC_KeyExchange + rekey.
- `SECOC_ETHERNET_GATEWAY_MODE`: tắt CAN/FlexRay dependencies, focus Ethernet.

Cấu hình hiện tại (TRUE/TRUE/TRUE): "PQC Ethernet Gateway full".

**Citations:**
- [Internal: include/SecOC/SecOC_PQC_Cfg.h]

---

### Câu B.16 — `SOAD_PQC_REKEY_INTERVAL_CYCLES = 360000` ~1 giờ

**Câu hỏi của giáo sư:** "Tại sao chọn 1 giờ? Tradeoff?"

**Câu trả lời gợi ý:**

Code (line 56-58): `#define SOAD_PQC_REKEY_INTERVAL_CYCLES 360000U /* At 10 ms MainFunction period: 360000 cycles = 1 hour */`. Dùng tại `SoAd_PQC.c:191`.

**Lý do:**
1. **ML-DSA-65 không có giới hạn cứng số signature/key** (~2^192 an toàn). Best-practice rotate keys định kỳ giảm exposure window.
2. **Driving session typical** ~30-60 phút.
3. **TLS industry convention:** TLS 1.3 session ticket TTL 24h, automotive ngắn hơn.
4. **Cost rekey:** ML-KEM-768 KeyGen + Encap + Decap + HKDF ~100 µs total → negligible.

**Trade-off:**
| Interval | Lợi | Hại |
|---|---|---|
| 1 phút | Forward secrecy mạnh | Rekey thrashing |
| **1 giờ** | Cân bằng | OK với daily driving |
| 24 giờ | Ít overhead | Window compromise dài |
| Không rekey | Đơn giản | Mất forward secrecy |

**Critique:** 1 giờ là arbitrary cứng. Production cần dynamic interval.

**Citations:**
- [TLS 1.3 RFC 8446](https://www.rfc-editor.org/rfc/rfc8446) (retrieved 2026-05-02)

---

### Câu B.17 — `BUS_LENGTH_RECEIVE = 4096` — tại sao 4096 mà không 3500?

**Câu hỏi của giáo sư:** "BUS_LENGTH_RECEIVE = 4096 — đủ lớn cho 3343 byte secured PDU + headroom. Tại sao chọn 4096 mà không 3500?"

**Câu trả lời gợi ý:**

Code: `#define BUS_LENGTH_RECEIVE 4096  // Increased for PQC signatures (3309 bytes)`.

**Tính toán secured PDU PQC tối đa:**
| Field | Size (byte) |
|---|---|
| Header (PduLength) | 2 |
| Authentic PDU payload | 8-32 |
| Truncated Freshness | 8 |
| ML-DSA-65 Signature | 3309 |
| **Total** | **~3327-3351** |

**Tại sao 4096 thay vì 3500:**
1. **Power-of-two alignment** — 4096 = 2^12, page size Linux. Cache-friendly.
2. **Headroom future schemes:** ML-DSA-87 (4627 byte) tràn 4096 nhưng FALCON-1024 (1280 byte) fit. Optional fields tương lai (timestamp, MAC fallback).
3. **Off-by-one safety:** Buffer 3500 → headroom 159 byte (4.5%). Buffer 4096 → headroom 755 byte (22%).
4. **Linux MTU:** Ethernet MTU 1500, secured PDU 3343 fragment thành 3 IP packets.

**Trade-off:** Bộ nhớ +4088 byte per buffer instance. OK trên Pi 4 (4 GB RAM), nhưng AURIX (1 MB SRAM) phải tinh chỉnh.

**Citations:**
- [Linux `getpagesize(2)` — typical 4096](https://man7.org/linux/man-pages/man2/getpagesize.2.html) (retrieved 2026-05-02)
- [NIST FIPS 204](https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.204.pdf) (retrieved 2026-05-02)

---

## G. Buffer overflow fix narrative

### Câu B.18 — Buffer 8 → 4096: files affected, bài học PQC migration

**Câu hỏi của giáo sư:** "Buffer 8 bytes → 4096 — chính xác commit nào fix? Files nào affected?"

**Câu trả lời gợi ý:**

Buffer overflow là bug điển hình khi migrate MAC (4 byte) → ML-DSA signature (3309 byte). Original codebase (HosamAboabla fork) thiết kế cho MAC PDU ngắn → buffers `BUS_LENGTH = 8` byte cho CAN-style frame. PQC PDU 3309+ byte → tràn buffer.

**Files affected:**
1. `include/Ethernet/ethernet.h:30` — `BUS_LENGTH_RECEIVE` từ 8 → 4096.
2. `include/SecOC/SecOC_Cfg.h` — `SECOC_SECPDU_MAX_LENGTH` phải nâng ≥4096.
3. `include/SecOC/SecOC_PQC_Cfg.h:38` — `SECOC_PQC_MAX_PDU_SIZE = 8192U`.
4. `source/SoAd/SoAd.c:89-91` — `SoAdTp_Buffer_RxData[][SECOC_SECPDU_MAX_LENGTH]`.
5. `source/SecOC/SecOC.c` — `SecOC_TxIntermediateType.DataToAuth[]`, `SecOC_RxIntermediateType.mac[]`/`authenticPdu[]`.
6. `source/Mcal/Pi4/Can_Pi4.c:64,73` — vẫn 8 byte vì CAN frame physical constraint, KHÔNG phải overflow.

**Bài học PQC migration:**
1. **PQC migration KHÔNG chỉ là thay thuật toán** — cần audit TẤT CẢ buffer/array hard-coded.
2. **Stack overflow nguy hiểm hơn heap overflow** trong embedded. Linux stack 8 MB nhưng AURIX RTOS task chỉ 4-16 KB.
3. **Cần `static_assert(SECOC_SECPDU_MAX_LENGTH >= PQC_MLDSA_SIGNATURE_BYTES + ...)`** — em chưa thêm.
4. **Defense-in-depth:** parametric `#define MAX_AUTH_INFO_BYTES`, không hardcode.
5. **Test coverage cho large PDU** — đã thêm `Phase3_Complete_Test`.

**Citations:**
- [CWE-119 — Buffer overflow](https://cwe.mitre.org/data/definitions/119.html) (retrieved 2026-05-02)

---

## H. Boot-time key provisioning

### Câu B.19 — Bốn modes (DEMO_FILE_AUTO, FILE_STRICT, PROVISIONED, HSM_HANDLE)

**Câu hỏi của giáo sư:** "4 modes ở `Csm.c:453-492`. Default cho production = ?"

**Câu trả lời gợi ý:**

`source/Csm/Csm.c:453-495`:
```c
if (CsmMldsaBootstrapMode == CSM_MLDSA_BOOTSTRAP_DEMO_FILE_AUTO) {  /* line 453 */
    result = CryIf_MldsaLoadKeys(...);
    if (result != E_OK) {
        result = CryIf_MldsaGenerateKeyPair(...);   /* line 458 */
        (void)CryIf_MldsaSaveKeys(...);             /* line 465 */
    }
}
else if (CSM_MLDSA_BOOTSTRAP_FILE_STRICT) {          /* line 468 */
    result = CryIf_MldsaLoadKeys(...);
    if (result != E_OK) return E_NOT_OK;             /* line 474 */
}
else {                                                /* line 477 — PROVISIONED/HSM */
    if (CsmLoadProvisionedMldsaKeysFct == NULL) return E_NOT_OK;  /* line 482 */
    result = CsmLoadProvisionedMldsaKeysFct(...);    /* line 485 */
}
```

**Phân loại:**
| Mode | Trust model | Phù hợp |
|---|---|---|
| `DEMO_FILE_AUTO` | Auto-generate nếu thiếu | Development, demo (Pi 4) |
| `FILE_STRICT` | Phải load từ file | Pre-production tooling |
| `PROVISIONED` | Callback OEM-provision | Pilot deployment |
| `HSM_HANDLE` | Callback HSM/secure element | **Production ECU** |

**Default Pi 4 demonstrator:** `DEMO_FILE_AUTO`.
**Default production:** `HSM_HANDLE` — KHÔNG BAO GIỜ filesystem.

Pi 4 deployment **không thể chạy HSM_HANDLE** vì Pi 4 không có secure element built-in. Có thể bổ sung Microchip ATECC608 qua I2C HAT để partial mitigate.

**Citations:**
- [TPM 2.0 Library](https://trustedcomputinggroup.org/resource/tpm-library-specification/) (retrieved 2026-05-02)
- [Microchip ATECC608B](https://www.microchip.com/en-us/product/atecc608b) (retrieved 2026-05-02)

---

### Câu B.20 — Filesystem keys 4032 byte 0600 — defend M21 PARTIAL

**Câu hỏi của giáo sư:** "ML-DSA private key 4032 byte trong `/etc/secoc/keys/mldsa_secoc.key` 0600. Defend M21 PARTIAL."

**Câu trả lời gợi ý:**

Code thực tế:
- `source/PQC/PQC.c:280-297` — `PQC_EnsureKeyDirectory()`: `mkdir(PQC_MLDSA_KEY_DIRECTORY, 0700)` (line 287).
- `source/PQC/PQC.c:303-355` — `PQC_MLDSA_SaveKeys()`: `fopen("wb")` rồi `fwrite(SecretKey, 1, 4032, fp)` (line 343). **KHÔNG có `chmod(0600)` explicit** — file inherit umask. Em chưa làm — gap.

**ML-DSA-65 secret key size:** 4032 byte chính xác theo NIST FIPS 204.

**Permission 0600 cho owner-only:**
- Bảo vệ khỏi other Linux users.
- KHÔNG bảo vệ khỏi root, physical access (tháo SD), backup leak, container escape.

**Tại sao M21 chỉ PARTIAL:**
1. **Không encryption-at-rest** — key plaintext on disk.
2. **Không integrity check** — file có thể bị tamper.
3. **Không key rotation policy** — một file một key, không versioning.
4. **Không access logging** — audit trail thiếu.
5. **Không atomic write** — power-loss giữa fwrite → corrupt.
6. **Không anti-rollback** — restore old key file.

**Path FULL compliance Future Work:**
- Linux `keyctl(2)` API (kernel keyring).
- TPM 2.0 NVRAM storage.
- AURIX HSE/CYRES module hoặc Crypto Satellites.

**Citations:**
- [NIST FIPS 204 — secret key 4032 bytes](https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.204.pdf) (retrieved 2026-05-02)
- [Linux keyctl(2)](https://man7.org/linux/man-pages/man2/keyctl.2.html) (retrieved 2026-05-02)
- [ISO/SAE 21434 §10](https://www.iso.org/standard/70918.html) (retrieved 2026-05-02)

---

## I. Endianness và byte alignment

### Câu B.21 — DataToAuthenticator byte order: violation phát hiện

**Câu hỏi của giáo sư:** "DataToAuthenticator big-endian (network byte order). Chứng minh từ code."

**Câu trả lời gợi ý — đây là điểm phải critique:**

**Spec AUTOSAR R21-11:** SWS_SecOC_00031 quy định DataToAuthenticator concatenation. **Data Identifier và Freshness Value phải mã hoá big-endian (network byte order)**.

**Code thực tế:** `source/SecOC/SecOC.c:263`:
```c
(void)memcpy((void*)&DataToAuth[*DataToAuthLen],
             (const void*)&SecOCTxPduProcessing[TxPduId].SecOCDataId,
             sizeof(SecOCTxPduProcessing[TxPduId].SecOCDataId));
```

**Vấn đề:** `memcpy(&dst, &SecOCDataId, 2)` copy raw bytes của uint16. Trên **little-endian platform** (Pi 4 aarch64, x86_64), uint16 `0x1234` được store trong memory là `0x34 0x12`. Vậy DataToAuthenticator là `0x34 0x12 ...` — **little-endian**, KHÔNG phải big-endian như SWS yêu cầu.

**Tại sao VẪN HOẠT ĐỘNG ĐÚNG trong demonstrator:**
Khi cả TX và RX đều dùng cùng kiến trúc (Pi 4 ↔ Pi 4), endianness là **internal convention** chứ không observable. `constructDataToAuthenticatorTx` (line 251) và `constructDataToAuthenticatorRx` (line 1287) đều dùng cùng pattern memcpy → TX byte string == RX byte string → signature valid.

**Vấn đề LỘ ra khi:**
1. Interop với ECU big-endian (PowerPC, classic AUTOSAR target).
2. Interop với spec-conformant implementation từ vendor khác.
3. Hội đồng kiểm tra wire-format có đúng SWS không.

**Em phải thừa nhận:** Code hiện tại **vi phạm SWS_SecOC_00031 về byte order conformance**. Để fix:
```c
uint16 dataIdBE = htons(SecOCTxPduProcessing[TxPduId].SecOCDataId);
memcpy(&DataToAuth[*DataToAuthLen], &dataIdBE, sizeof(dataIdBE));
```

**Citations:**
- [SWS_SecOC R21-11 §7.3](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_SecureOnboardCommunication.pdf) (retrieved 2026-05-02)
- [POSIX htons(3)](https://man7.org/linux/man-pages/man3/htons.3.html) (retrieved 2026-05-02)
- [Munich Dissecto — SecOC byte order](https://munich.dissec.to/kb/chapters/can/can-secoc.html) (retrieved 2026-05-02)

---

### Câu B.22 — Pi 4 aarch64 little-endian native — có byte-swap?

**Câu hỏi của giáo sư:** "Pi 4 aarch64 little-endian native. Có byte-swap khi build DataToAuthenticator?"

**Câu trả lời gợi ý:**

**Phần cứng:** ARMv8-A AArch64 hỗ trợ cả BE và LE, nhưng Pi 4 Linux 6.1 mainline chạy **little-endian native** (`CONFIG_CPU_LITTLE_ENDIAN=y`). x86_64 và MinGW/Windows cũng little-endian.

**Hệ quả:** Cả ba target build đều LE. Code **KHÔNG có byte-swap nào** trong:
- `constructDataToAuthenticatorTx` (`SecOC.c:251-273`)
- `constructDataToAuthenticatorRx` (`SecOC.c:1287-1310`)
- `parseSecuredPdu` header read (`SecOC.c:1177`)
- `SoAdTp_RxIndication` length parse (`SoAd.c:1247`)

Tất cả dùng raw `memcpy` → host byte order leak vào wire.

**Liên hệ với Câu B.21:** Code đang vi phạm SWS yêu cầu BE, nhưng KHÔNG GÂY LỖI TEST vì:
1. Cả TX và RX cùng little-endian.
2. ML-DSA signature là byte-string opaque → không nhạy cảm endianness.
3. ML-DSA verify chỉ kiểm tra `H(message)` — message là byte array, không parse cấu trúc.

**Sẽ phát hiện lỗi khi:**
1. Interop test với ECU thật BE (PowerPC, MPC57xx automotive).
2. CodeQL/Coverity static analysis flag inconsistent endianness usage.
3. AUTOSAR conformance test sẽ flag.

**Fix strategy proper:**
```c
static void be_write_u16(uint8* dst, uint16 v) {
    dst[0] = (uint8)((v >> 8) & 0xFF);
    dst[1] = (uint8)(v & 0xFF);
}
static void be_write_u64(uint8* dst, uint64 v) {
    for (int i = 0; i < 8; i++)
        dst[i] = (uint8)((v >> (56 - 8*i)) & 0xFF);
}
```
Thay tất cả `memcpy(buffer, &uint16/uint64, sizeof)` bằng `be_write_*`. Đây là **task cho Future Work**.

**Tóm tắt phòng thủ:**
- Em **THỪA NHẬN** code không byte-swap, vi phạm strict SWS conformance.
- Em **JUSTIFY** nó hoạt động đúng trong demonstrator vì đồng nhất LE.
- Em **PROPOSE** fix rõ ràng cho future work.
- Đây là honest engineering: prototype-first, conformance-iteration.

**Citations:**
- [ARMv8-A Architecture Reference Manual](https://developer.arm.com/documentation/ddi0487/latest) (retrieved 2026-05-02)
- [SWS_SecOC R21-11 byte order](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_SecureOnboardCommunication.pdf) (retrieved 2026-05-02)

---

## Tổng kết tự vệ chiến lược

3 nguyên tắc phản biện:

1. **Phạm vi đồ án rõ ràng:** *PQC demonstrator/prototype trong Linux user-space trên Pi 4*, KHÔNG phải production ECU. Đóng góp chính: chứng minh kiến trúc layered AUTOSAR vẫn ổn định khi swap MAC↔ML-DSA, và identify các challenge buffer/endianness/key-mgmt.

2. **Honest engineering — thừa nhận limitations:** Pi 4 không real-time, key trên filesystem, byte order vi phạm strict SWS, SoAd architectural smell. Mỗi điểm propose path khắc phục cụ thể.

3. **Industry trajectory:** AURIX TC4Dx (2025 mass production) đã có CSRM cho PQC. NIST FIPS 203/204 standardize Aug 2024. Đồ án là **bridging research** giữa NIST standardization và automotive deployment.

**Discovered architectural issues:**
1. **Byte order non-conformance** ở `SecOC.c:263, 271, 1177, 1298` — vi phạm SWS_SecOC_00031.
2. **Architectural smell**: ML-KEM key exchange ở `SoAd_PQC.c` thay vì Csm/KeyManager.
3. **Key storage gap**: `PQC_MLDSA_SaveKeys` không gọi `chmod(0600)` explicit.
4. **Missing `static_assert`** cho buffer size.
