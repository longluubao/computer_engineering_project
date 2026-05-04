# Bảo vệ luận văn — Bộ câu hỏi & câu trả lời (Tiếng Việt)

**Đề tài:** "AUTOSAR SecOC with Post-Quantum Cryptography on a Raspberry Pi 4 Ethernet Gateway"
**Sinh viên:** Luu Bao Long và nhóm (đồ án tốt nghiệp)
**Hội đồng phản biện mô phỏng:** Ba giáo sư chuyên môn
1. **Giáo sư A** — AUTOSAR Classic Platform & Basic Software (BSW) Stack
2. **Giáo sư B** — Triển khai Raspberry Pi 4, hệ build, code flow (TX/RX)
3. **Giáo sư C** — Test methodology, ISE simulator, validation

**Ngày biên soạn:** 2026-05-02
**Tổng số câu hỏi:** 60 (A: 20 + B: 22 + C: 18). Mỗi câu kèm ≥1 citation đã verify.

---

## 0 · Tóm tắt điều hành

### 0.1 Đánh giá tổng quan

| Hạng mục | Kết luận |
|---|---|
| Phạm vi đồ án | **Demonstrator BSW software-architecture** trên Linux user-space — không phải ECU AUTOSAR Classic production-grade |
| Đóng góp chính | Tích hợp ML-KEM-768 + ML-DSA-65 vào AUTOSAR SecOC qua Csm/CryIf seam, end-to-end TX/RX trên Pi 4 |
| Điểm mạnh | Layered separation chuẩn, code byte-identical Windows/Pi 4, 669+ test cases, 22/23 PASS, ISE 8316 frames, code-trace có line numbers |
| Điểm yếu | Pi 4 ≠ AUTOSAR Classic target; không RTE; không ARXML; OS cooperative không OSEK; key trên filesystem; ML-KEM handshake không xác thực |

### 0.2 Năm điểm yếu lớn nhất cần phòng thủ

| Vấn đề | Vị trí | Mức độ |
|---|---|---|
| **Pi 4 + Linux KHÔNG phải target AUTOSAR Classic** — toàn bộ hệ thống | Cluster A & B | Critical (cần reframe là demonstrator) |
| **Không có RTE; cấu hình BSW viết tay không qua ARXML** | Toàn bộ source/ | Cao |
| **ML-KEM handshake không xác thực** → MitM phá toàn bộ chuỗi | `source/SoAd/SoAd_PQC.c` | Critical (đã thừa nhận) |
| **ML-DSA private key trong `/etc/secoc/keys/` (POSIX 0600)** — không HSM/TEE | `include/SecOC/SecOC_PQC_Cfg.h:48` | Cao (M21 PARTIAL đã thừa nhận) |
| **Byte order non-conformance** — code dùng raw memcpy LE, vi phạm SWS_SecOC_00031 BE | `SecOC.c:263, 271, 1177, 1298` | Trung bình (mới phát hiện) |
| **liboqs malloc/free** vi phạm MISRA Rule 21.3 trong build Classic | `external/liboqs/` | Cao (production blocker) |
| **SocketCAN ở `Mcal/Pi4/Can_Pi4.c`** không phải MCAL register-level | `source/Mcal/Pi4/` | Trung bình |
| **Os module cooperative**, không OSEK conformance class | `source/Os/Os.c` | Trung bình |
| **8 lệnh `Det_ReportError()` không gated** — vi phạm `[SWS_SecOC_00054]` | `source/SecOC/SecOC.c` | Cao (đã thừa nhận) |
| **55% line coverage thấp dưới chuẩn CAL-2** (≥80% expected) | gcovr report | Cao |

### 0.3 Chiến lược phòng thủ tổng thể

> Em xin định danh chính xác: **"AUTOSAR Classic-style BSW reference implementation for SecOC + PQC, hosted on Linux/Pi 4 user-space"** — không phải "AUTOSAR Classic ECU production-grade". Đây là demonstrator engineering chứng minh kiến trúc layered của AUTOSAR vẫn ổn định khi swap MAC ↔ ML-DSA, và identify các challenge buffer/endianness/key-management cần giải quyết khi scale lên production. Mỗi điểm yếu em đều thừa nhận thẳng và đề xuất hướng khắc phục cụ thể (HSM-backed key storage, sigma-style authenticated KEM, byte-order helpers, OSEK-conformant RTOS port).

---

## 0.4 Mục lục câu hỏi (60 câu)

### Phần I — Giáo sư A: AUTOSAR Classic BSW Stack (20 câu)
1. Câu A.1 — Vi phạm kiến trúc phân lớp ("Extension Layer")
2. Câu A.2 — 7 module chỉ là Stub — có thật sự "đầy đủ BSW stack"?
3. Câu A.3 — Thiếu RTE — vi phạm Methodology?
4. Câu A.4 — Mối quan hệ giữa các module — vẽ Tx/Rx Flow
5. Câu A.5 — Tách lớp Csm/CryIf/Crypto Driver — tại sao 3 lớp?
6. Câu A.6 — AUTOSAR Methodology / ARXML — không có ARXML?
7. Câu A.7 — BswM Min. — Mode arbitration thiếu
8. Câu A.8 — EcuM Min. — STARTUP/RUN/POST_RUN/SHUTDOWN state machine
9. Câu A.9 — Os Min. — không phải OSEK
10. Câu A.10 — PduR routing tables — chỗ nào trong code?
11. Câu A.11 — ApBridge "Extension" — Tier-1 sẽ làm gì?
12. Câu A.12 — MCAL chỉ có Pi4 — SocketCAN không phải MCAL chuẩn
13. Câu A.13 — COM/CanIf/EthIf — vai trò chính xác
14. Câu A.14 — Std_Types.h — chứng minh tuân thủ
15. Câu A.15 — Encrypt module — vì sao ở ECU Abstraction và Min.?
16. Câu A.16 — PQC module — vị trí trong layer nào?
17. Câu A.17 — Det/Dem coupling — persistence qua NvM?
18. Câu A.18 — SecOC bypass và Forced-Pass-Override
19. Câu A.19 — Buffer sizing cho PQC — SECPDU_MAX_LENGTH đủ chưa?
20. Câu A.20 — Tổng kết — có thực sự là AUTOSAR Classic ECU?

### Phần II — Giáo sư B: Pi 4 Deployment + Code Flow (22 câu)
21. Câu B.1 — Pi 4 có ý nghĩa gì? Lập luận "đại diện gateway/zonal HPC"
22. Câu B.2 — MCP2515 CAN HAT specs và giới hạn
23. Câu B.3 — Linux 6.1 không real-time. Tại sao không PREEMPT_RT?
24. Câu B.4 — Pi 4 vs AURIX TC3xx/TC4Dx — performance gap
25. Câu B.5 — NEON SIMD trong liboqs
26. Câu B.6 — Linux user-space thay vì bare-metal
27. Câu B.7 — CMake `MCAL_TARGET=PI4` tách build
28. Câu B.8 — Tại sao chỉ enable ML-KEM-768 và ML-DSA-65?
29. Câu B.9 — Hai implementation Ethernet — design pattern
30. **Câu B.10** — CODE-TRACE: TX flow PQC chi tiết line-by-line
31. **Câu B.11** — CODE-TRACE: RX flow PQC chi tiết
32. Câu B.12 — Tại sao FRESHNESS CHECK chạy TRƯỚC SIGNATURE VERIFY?
33. **Câu B.13** — CODE-TRACE: ML-KEM handshake và architectural smell
34. Câu B.14 — HKDF-SHA256: salt và info có domain-separated không?
35. Câu B.15 — Ba flags (PQC_MODE / MLKEM / ETHERNET_GATEWAY)
36. Câu B.16 — `SOAD_PQC_REKEY_INTERVAL_CYCLES = 360000` ~1 giờ
37. Câu B.17 — `BUS_LENGTH_RECEIVE = 4096` — tại sao 4096?
38. Câu B.18 — Buffer 8 → 4096: files affected, bài học PQC migration
39. Câu B.19 — Bốn bootstrap modes (DEMO/STRICT/PROVISIONED/HSM)
40. Câu B.20 — Filesystem keys 4032 byte — defend M21 PARTIAL
41. **Câu B.21** — DataToAuthenticator byte order: violation phát hiện
42. Câu B.22 — Pi 4 aarch64 little-endian — có byte-swap không?

### Phần III — Giáo sư C: Test methodology + ISE validation (18 câu)
43. Câu C.1 — GoogleTest C++ wrap pure C — rủi ro linker name-mangling
44. Câu C.2 — 669+ test cases phân bố thế nào?
45. Câu C.3 — TDD hay test-after?
46. Câu C.4 — 55% line coverage cho thesis claim CAL-2
47. Câu C.5 — Loại trừ Mcal/Scheduler/GUIInterface khỏi coverage — cherry-pick?
48. Câu C.6 — Branch coverage 60% — branch nào không cover được?
49. Câu C.7 — Phase-1/2/3/4 nghĩa là gì?
50. Câu C.8 — CsmTests có mock không? Integration trá hình unit?
51. Câu C.9 — Standalone .c programs có nằm trong CTest không?
52. Câu C.10 — ISE là self-built? Cross-validate hardware không?
53. Câu C.11 — 5 virtual buses — model chính xác đến đâu?
54. Câu C.12 — 3 ECU + 10 attacker — design rationale
55. Câu C.13 — 7 vector hay 10? Mỗi vector inject gì?
56. Câu C.14 — Replay 47/47 nhưng aggregate 94% — denominator pollution
57. Câu C.15 — Downgrade attack 0% trong HMAC mode — n/a thay vì fail
58. Câu C.16 — Coverage attack space — class bị bỏ qua
59. Câu C.17 — 1000 iterations đủ cho p99.9 không?
60. Câu C.18 — x86_64 vs Pi 4 — vi-kiến-trúc giải thích?

---

## 0.5 Cách đọc tài liệu này

Mỗi câu hỏi gồm bốn thành phần:
1. **Câu hỏi của giáo sư** — phong cách phỏng vấn nghiêm túc
2. **Tại sao câu hỏi này quan trọng** — bối cảnh
3. **Câu trả lời gợi ý** — viết theo ngôi "em", có dẫn chứng cụ thể từ code (`source/SecOC/SecOC.c:1429`)
4. **Citations** — link trực tiếp tới spec autosar.org / NIST FIPS / RFC / IEEE

**Nguyên tắc phòng thủ xuyên suốt:**
- Không tô hồng. Khi code thiếu/sai, thừa nhận thẳng và đưa hướng khắc phục.
- Mỗi citation phải verify được (đã chạy WebSearch).
- Ngôn ngữ: tiếng Việt học thuật. Citations giữ nguyên tiếng Anh để đối chiếu spec.

---

# PHẦN I — Giáo sư A: AUTOSAR Classic BSW Stack

> 20 câu hỏi về kiến trúc phân lớp, RTE, ARXML, các module Stub/Min, tách lớp Csm/CryIf/Crypto Driver, MCAL, OS conformance.

📄 **Nội dung chi tiết:** `.audit_drafts/vn_agent_a_bsw_stack.md`

**Các điểm nóng nhất trong Phần I:**

- **A.1 (Extension Layer):** Em thừa nhận AUTOSAR R21-11 chỉ có 4 lớp BSW (Services / ECU Abstraction / MCAL / Microcontroller). ApBridge/GUIInterface/Scheduler nên gọi là **Complex Device Drivers (CDD)**, không phải lớp 5.
- **A.3 (Thiếu RTE):** Project **không có file ARXML nào**. `Rte_SecOC.h` chỉ là header rỗng. Đây là deviation Methodology — em sẽ ghi rõ "RTE Bypass mode".
- **A.6 (Không ARXML):** `*_Cfg.h`, `*_Lcfg.c`, `*_PBcfg.c` đều **viết tay** thay vì auto-generated từ DaVinci/Tresos.
- **A.9 (Os cooperative):** `source/Os/Os.c` là cooperative scheduler, không OSEK preemptive. Không thể đo WCET đúng nghĩa AUTOSAR.
- **A.12 (SocketCAN ≠ MCAL):** `Can_Pi4.c` là wrapper trên Linux SocketCAN — KHÔNG truy cập register MCP2515 trực tiếp. Đây là "Virtual MCAL".
- **A.20 (Định danh):** Project là **"AUTOSAR Classic-style BSW reference implementation, hosted on Linux/Pi 4 user-space"**, không phải "AUTOSAR Classic ECU production-grade".

---

# PHẦN II — Giáo sư B: Pi 4 Deployment + Code Flow

> 22 câu hỏi về Pi 4 deployment justification, MCP2515, Linux non-real-time, build dual-platform, **code-trace TX/RX line-by-line**, ML-KEM handshake, HKDF, compile-time flags, key provisioning, byte-order violation phát hiện.

📄 **Nội dung chi tiết:** `.audit_drafts/vn_agent_b_pi4_codeflow.md`

**Code-trace highlights (ít nhất 6 câu có line numbers chi tiết):**

### B.10 — TX flow PQC (rút gọn)
```
SecOC_IfTransmit (SecOC.c:189)
  → enqueue PDU (line 218)
  → MainFunction
  → authenticate_PQC (SecOC.c:1429)
    → prepareFreshnessTx (SecOC.c:312)         [FVM lookup, 64-bit counter PQC]
    → constructDataToAuthenticatorTx (SecOC.c:251)  [Layout: DataId(2) || Payload || Freshness(8)]
    → Csm_SignatureGenerate (Csm.c:806)
      → Csm_PQC_EnsureReady (Csm.c:825)        [bootstrap key load]
      → CryIf_SignatureGenerate (CryIf.c:107)
        → PQC_MLDSA_Sign (PQC.c:415)
          → OQS_SIG_sign (liboqs)              [3309 byte signature]
    → ghép Secured PDU (SecOC.c:1472-1497)     [Header | AuthPDU | Freshness | Signature]
  → PduR_SecOCTransmit
  → SoAd_IfTransmit (SoAd.c:654)
  → SoAd_TcpIpTransmit (SoAd.c:727)
  → EthDrv_Send (ethernet.c:74)
    → Linux send() syscall
```

### B.11 — RX flow PQC (rút gọn)
```
Linux recv() → SoAdTp_RxIndication (SoAd.c:1178)
  → enqueue (line 1219)
  → SecOC_MainFunctionRx (SecOC.c:1663)
  → verify_PQC (SecOC.c:1556)
    → parseSecuredPdu (SecOC.c:1163)           [tách Header | Auth | Freshness | Signature]
    → /* FRESHNESS CHECK FIRST */ (line 1583-1590)   [defense-in-depth, cost asymmetry]
    → constructDataToAuthenticatorRx (SecOC.c:1287)
    → Csm_SignatureVerify (Csm.c:881)
      → CryIf_SignatureVerify (CryIf.c:132)
        → PQC_MLDSA_Verify (PQC.c:466)
          → OQS_SIG_verify (liboqs)
    → FVM_UpdateCounter (line 1657)
  → PduR_SecOCRxIndication → COM → App
```

### B.13 — ML-KEM handshake (architectural smell tự thừa nhận)
```
SoAd_PQC_MainFunction (SoAd_PQC.c:175) [TIMER 360000 cycles ≈ 1h]
  → SoAd_PQC_KeyExchange_Initiator (SoAd_PQC.c:221)
    → Csm_KeyExchangeInitiate
      → CryIf_KeyExchangeInitiate (CryIf.c:156)
        → PQC_KeyExchange_Initiate (PQC_KeyExchange.c:62)
          → PQC_MLKEM_KeyGen → OQS_KEM_keypair  [pk 1184 byte, sk 2400 byte]
  → Send PUBKEY control message [magic 0x51 0x43, type=0x01]
  → Responder nhận → PQC_KeyExchange_Respond (PQC_KeyExchange.c:112)
    → PQC_MLKEM_Encapsulate → OQS_KEM_encaps   [ciphertext 1088 byte, shared 32 byte]
  → Send CIPHERTEXT control message
  → Initiator nhận → PQC_KeyExchange_Complete (PQC_KeyExchange.c:162)
    → PQC_MLKEM_Decapsulate → OQS_KEM_decaps
    → memset SecretKey = 0 (zeroize, line 211)
  → HKDF-SHA256(shared_secret) → encryption + authentication keys
```

### B.21 — Endianness violation phát hiện
```c
// SecOC.c:263 — VI PHẠM SWS_SecOC_00031 (yêu cầu big-endian)
memcpy(&DataToAuth[*DataToAuthLen], &SecOCDataId, sizeof(SecOCDataId));
// Trên Pi 4 LE: uint16 0x1234 → bytes [0x34, 0x12] LE
// SWS yêu cầu BE: [0x12, 0x34]
```
Em thừa nhận violation. Hoạt động đúng trong demonstrator vì TX và RX cùng LE. Fix: dùng `htons()` hoặc `be_write_u16()` helper.

**Các điểm nóng khác trong Phần II:**

- **B.6 (Linux user-space):** Em không phòng vệ — chấp nhận đây là demonstrator, không bare-metal. Đóng góp ở mức software architecture validation.
- **B.13 (Architectural smell):** ML-KEM key exchange logic đặt ở SoAd vi phạm Single Responsibility Principle. Magic numbers (0x51 0x43) không có spec.
- **B.20 (Filesystem keys):** `PQC_MLDSA_SaveKeys` (`PQC.c:303-355`) **không gọi `chmod(0600)` explicit** — chỉ rely vào umask. Đây là gap em chưa làm.

---

# PHẦN III — Giáo sư C: Test methodology + ISE validation

> 18 câu hỏi về GoogleTest framework cho code C, 669+ tests, 55% coverage defend, mock vs real, ISE simulator design, 7 attack vectors, replay 47/47 vs aggregate 94%, performance benchmark methodology, reproducibility.

📄 **Nội dung chi tiết:** `.audit_drafts/vn_agent_c_tests_validation.md`

**Các điểm nóng nhất trong Phần III:**

- **C.1 (extern "C"):** `extern Std_ReturnType authenticate(...)` cho hàm `STATIC` — `SecOCIntegrationTests.cpp:51-52` thừa nhận "STATIC is empty in non-RELEASE builds so they are linkable from tests" → vi phạm MISRA Rule 8.7/8.8. Test build **khác production build**.
- **C.3 (test-after pattern nguy hiểm):** `AuthenticationTests.cpp:79-83`, `PQC_ComparisonTests.cpp:130-138`, `CsmTests.cpp:156-162` chứa `if (Result != E_OK) { SUCCEED(); return; }` → **test có thể pass kể cả khi feature hỏng** trong CI sandbox không có `/etc/secoc/keys/`. Đây là red flag nghiêm trọng cho validity.
- **C.4 (55% coverage):** Dưới chuẩn CAL-2 (≥80% expected). Cherry-pick exclude `Scheduler` và `GUIInterface` làm con số càng yếu khi tính lại.
- **C.9 (Standalone .c outside CTest):** `test/CMakeLists.txt` dùng `file(GLOB_RECURSE *.cpp)` — **chỉ glob `.cpp`** → 4 standalone `.c` programs (`test_pqc_standalone.c`, `test_phase3_*.c`, `test_phase4_*.c`, `test_pqc_secoc_integration.c`) **KHÔNG nằm trong CTest** → CI có thể bỏ qua.
- **C.13 (7 vs 10 vector):** `ATTACK_PLAYBOOK.md` mô tả **10** attacker; thesis tuyên bố "7 vectors, 99.14%". Em phải làm rõ 7 nào và defend denominator.
- **C.14 (Denominator pollution):** "94% aggregate" trộn detected_as_attack với legitimate frames → metric flawed. Cách đúng: tách precision/recall.
- **C.17 (1000 iterations cho p99.9):** Mức tối thiểu — Wilson interval ±15-25%. Cần ≥10⁵ mẫu cho HdrHistogram convention. CSV không báo percentile.
- **Câu hỏi quyết định:** *"Trong 45% code chưa cover, có bao nhiêu là error-handling path? Đây chính là phần code mà thesis security claim PHỤ THUỘC NHẤT."*

---

# PHẦN IV — Phát biểu kết luận (đã rehearse)

> "Đề tài là một demonstrator engineering có giá trị: nó chứng minh rằng kiến trúc layered của AUTOSAR vẫn ổn định khi swap MAC ↔ ML-DSA, và validate được tích hợp NIST FIPS 203/204 vào Csm/CryIf seam. Em xin định danh chính xác là **AUTOSAR Classic-style BSW reference implementation hosted on Linux/Pi 4 user-space** — không phải AUTOSAR Classic ECU production-grade. Em thẳng thắn liệt kê các deviation: không có RTE và ARXML, OS cooperative không OSEK, MCAL là SocketCAN wrapper, key trên filesystem, byte order chưa conform SWS_SecOC_00031, ML-KEM handshake chưa có authentication. Mỗi điểm em đều có hướng khắc phục cụ thể trong Future Work — port lên AURIX TC275/TC4Dx với HSM, dùng EB tresos cho ARXML toolchain, thay Os bằng FreeRTOS/AUTOSAR OS thật, thêm `htons()` cho byte-order conformance, integrate sigma-style authenticated KEM. Em sẵn sàng defend từng điểm trong câu hỏi tiếp theo."

---

# Tổng hợp Citation (Master Citation Index)

## Spec AUTOSAR — autosar.org
- [Classic Platform standards landing](https://www.autosar.org/standards/classic-platform)
- [Adaptive Platform standards](https://www.autosar.org/standards/adaptive-platform)
- [Classic Platform Release Overview R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_TR_ClassicPlatformReleaseOverview.pdf)
- [SWS Secure Onboard Communication R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_SecureOnboardCommunication.pdf)
- [SWS Crypto Service Manager R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_CryptoServiceManager.pdf)
- [SWS Crypto Interface R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_CryptoInterface.pdf)
- [SWS Crypto Driver R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_CryptoDriver.pdf)
- [SWS PDU Router R20-11](https://www.autosar.org/fileadmin/standards/R20-11/CP/AUTOSAR_SWS_PDURouter.pdf)
- [SWS Socket Adaptor R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_SocketAdaptor.pdf)
- [SWS CAN Driver R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_CANDriver.pdf)
- [SWS CAN Interface R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_CANInterface.pdf)
- [SWS Operating System R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_OS.pdf)
- [SWS Default Error Tracer R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_DefaultErrorTracer.pdf)
- [SWS BSW General R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_BSWGeneral.pdf)
- [SWS BSW Mode Manager R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_BSWModeManager.pdf)
- [SWS ECU State Manager R20-11](https://www.autosar.org/fileadmin/standards/R20-11/CP/AUTOSAR_SWS_ECUStateManager.pdf)
- [SWS COM R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_COM.pdf)
- [SWS Standard Types R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_StandardTypes.pdf)
- [SWS NVRAM Manager R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_NVRAMManager.pdf)
- [SWS RTE R22-11](https://www.autosar.org/fileadmin/standards/R22-11/CP/AUTOSAR_SWS_RTE.pdf)
- [SRS RTE R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SRS_RTE.pdf)
- [Methodology R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_TR_Methodology.pdf)
- [TPS ECU Configuration R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_TPS_ECUConfiguration.pdf)
- [Layered Software Architecture R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_EXP_LayeredSoftwareArchitecture.pdf)
- [BSW Module List R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_TR_BSWModuleList.pdf)
- [Utilization of Crypto Services R20-11](https://www.autosar.org/fileadmin/standards/R20-11/CP/AUTOSAR_EXP_UtilizationOfCryptoServices.pdf)

## NIST PQC Standards
- [FIPS 203 (ML-KEM Standard)](https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.203.pdf) — §3.3, §7.1-7.3, §8 Table 2
- [FIPS 204 (ML-DSA Standard)](https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.204.pdf) — §3.7, §4 Table 2, §5 Algorithm 7
- [Open Quantum Safe — ML-KEM](https://openquantumsafe.org/liboqs/algorithms/kem/ml-kem.html)
- [Open Quantum Safe — ML-DSA](https://openquantumsafe.org/liboqs/algorithms/sig/ml-dsa.html)
- [liboqs releases on GitHub](https://github.com/open-quantum-safe/liboqs/releases)

## Cryptography Foundations
- [RFC 5869 — HKDF](https://www.rfc-editor.org/rfc/rfc5869)
- [TLS 1.3 RFC 8446](https://www.rfc-editor.org/rfc/rfc8446)
- [OpenSSL 3.0 EVP_MAC API](https://www.openssl.org/docs/man3.0/man3/EVP_MAC.html)

## Phần cứng & Platform
- [Infineon AURIX TC4Dx — PQC support](https://news.europawire.eu/infineon-unveils-high-performance-aurix-tc4dx-microcontroller-for-next-gen-software-defined-vehicles/eu-press-release/2024/11/06/15/07/28/143544/)
- [Infineon AURIX TC4x CSRM](https://www.infineon.com/assets/row/public/documents/10/156/infineon-tc4x-overview-productpresentation-en.pdf)
- [Electronics Weekly — PQC in automotive MCUs](https://www.electronicsweekly.com/news/products/micros/post-quantum-cryptography-support-in-automotive-mcus-2024-11/)
- [ARMv8-A Architecture Reference Manual](https://developer.arm.com/documentation/ddi0487/latest)
- [MDPI — ARMv8/NEON on Cortex-A72](https://www.mdpi.com/2079-9292/15/7/1456)

## Linux & Build Tooling
- [Linux SocketCAN docs](https://www.kernel.org/doc/Documentation/networking/can.txt)
- [Linux getrandom(2)](https://man7.org/linux/man-pages/man2/getrandom.2.html)
- [Linux getpagesize(2)](https://man7.org/linux/man-pages/man2/getpagesize.2.html)
- [Linux keyctl(2)](https://man7.org/linux/man-pages/man2/keyctl.2.html)
- [POSIX htons(3)](https://man7.org/linux/man-pages/man3/htons.3.html)
- [POSIX vs Winsock porting](https://learn.microsoft.com/en-us/windows/win32/winsock/porting-socket-applications-to-winsock)
- [CMake target_compile_definitions](https://cmake.org/cmake/help/latest/command/target_compile_definitions.html)
- [CMake CTest](https://cmake.org/cmake/help/latest/manual/ctest.1.html)
- [CMake GLOB_RECURSE](https://cmake.org/cmake/help/latest/command/file.html#glob-recurse)
- [MCP2515 SocketCAN setup](https://www.pragmaticlinux.com/2021/10/can-communication-on-the-raspberry-pi-with-socketcan/)
- [Pi 4B PREEMPT-RT bench](https://lemariva.com/blog/2019/09/raspberry-pi-4b-preempt-rt-kernel-419y-performance-test)

## Test Methodology
- [GoogleTest primer](https://google.github.io/googletest/primer.html)
- [GMock for C](https://google.github.io/googletest/gmock_for_dummies.html)
- [GTEST_SKIP semantics](https://google.github.io/googletest/advanced.html#skipping-test-execution)
- [Test pyramid — Martin Fowler](https://martinfowler.com/articles/practical-test-pyramid.html)
- [Test rot — Martin Fowler](https://martinfowler.com/bliki/TestRot.html)
- [Conditional asserts considered harmful](https://abseil.io/resources/swe-book/html/ch11.html)
- [HdrHistogram](http://www.hdrhistogram.org/)
- [Latency percentile sampling — Brendan Gregg](https://www.brendangregg.com/blog/2018-02-09/kpti-kaiser-meltdown-performance.html)
- [Wilson score interval](https://en.wikipedia.org/wiki/Binomial_proportion_confidence_interval#Wilson_score_interval)
- [gcovr](https://gcovr.com/en/stable/)
- [gcov branches](https://gcc.gnu.org/onlinedocs/gcc/Invoking-Gcov.html)
- [Mutation testing — pitest](https://pitest.org/quickstart/basic_concepts/)
- [GitHub Actions C/C++](https://docs.github.com/en/actions/automating-builds-and-tests/building-and-testing-c-or-c)

## Compliance & Standards
- [ISO/SAE 21434:2021](https://www.iso.org/standard/70918.html)
- [ISO 26262-6:2018](https://www.iso.org/standard/68388.html)
- [ISO/IEC/IEEE 29119-1:2022](https://www.iso.org/standard/81291.html)
- [ISO 11898-1:2015](https://www.iso.org/standard/63648.html)
- [IEEE 802.1Qbv TSN](https://standards.ieee.org/standard/802_1Qbv-2015.html)
- [FlexRay v3.0.1](https://www.iso.org/standard/63102.html)
- [MISRA Compliance:2020](https://misra.org.uk/product/misra-c2012-third-edition-first-revision/)
- [UN R155 Annex 5](https://unece.org/sites/default/files/2021-03/R155e.pdf)
- [TPM 2.0 Library Specification](https://trustedcomputinggroup.org/resource/tpm-library-specification/)
- [Microchip ATECC608B](https://www.microchip.com/en-us/product/atecc608b)

## Cybersecurity & Side-Channel Research
- [CWE-119 — Buffer overflow](https://cwe.mitre.org/data/definitions/119.html)
- [Side-channel ML-KEM/ML-DSA IACR ePrint 2024/1626](https://eprint.iacr.org/2024/1626)
- [POODLE downgrade precedent](https://en.wikipedia.org/wiki/POODLE)
- [IDS confusion matrix — NIST](https://csrc.nist.gov/glossary/term/intrusion_detection_system)
- [Precision/Recall](https://en.wikipedia.org/wiki/Precision_and_recall)
- [Reproducible Builds](https://reproducible-builds.org/docs/reproducible-research/)
- [ACM Reproducibility Badging](https://www.acm.org/publications/policies/artifact-review-and-badging-current)
- [ISTQB Glossary](https://glossary.istqb.org/)
- [Goodhart's Law](https://en.wikipedia.org/wiki/Goodhart%27s_law)

## ISE & Vehicle E/E
- [Bosch Mobility — E/E architecture](https://www.bosch-mobility.com/en/mobility-topics/e-e-architecture/)
- [Vector CANoe](https://www.vector.com/int/en/products/products-a-z/software/canoe/)
- [OpenDavinci](https://github.com/se-research/OpenDaVINCI)
- [Munich Dissecto — SecOC byte order](https://munich.dissec.to/kb/chapters/can/can-secoc.html)
- [AUTOSAR Security Extensions R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_TR_SecurityExtensions.pdf)

---

# Hướng dẫn sử dụng cho buổi bảo vệ

1. **Trước buổi bảo vệ:** đọc kỹ Phần 0.2 (5 điểm yếu cần phòng thủ) và Phần IV (kết luận đã rehearse). Học thuộc các line number quan trọng (SecOC.c:1429, Csm.c:806, PQC.c:415).
2. **Mở 3 file draft chi tiết** trong tab tách riêng:
   - `.audit_drafts/vn_agent_a_bsw_stack.md` — 20 câu BSW
   - `.audit_drafts/vn_agent_b_pi4_codeflow.md` — 22 câu Pi 4 + code flow
   - `.audit_drafts/vn_agent_c_tests_validation.md` — 18 câu test
3. **Khi giáo sư hỏi câu nào:** xác định cluster (A/B/C), tra số câu, áp dụng câu trả lời gợi ý — nhưng **dùng ngôn ngữ tự nhiên**, không đọc thuộc lòng.
4. **Khi bị hỏi câu mới:** áp dụng nguyên tắc "Honest engineering — thừa nhận limitations" + chỉ dẫn chứng line number cụ thể trong code.
5. **Giữ tone phòng vệ:** mỗi điểm yếu phải kèm theo "Future Work để khắc phục" — không bao giờ chỉ thừa nhận mà không có hướng đi.

**Chúc em bảo vệ thành công! 🎓**
