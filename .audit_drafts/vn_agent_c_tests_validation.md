# Phản biện đồ án — Test methodology & validation (Examiner C)

**Đề tài:** "AUTOSAR SecOC with Post-Quantum Cryptography on a Raspberry Pi 4 Ethernet Gateway"
**Phạm vi:** Phương pháp kiểm thử, độ phủ, tái lập, validation các tuyên bố thực nghiệm.
**Tài liệu căn cứ đã đọc:** `Autosar_SecOC/test/CMakeLists.txt`, `SecOCTests.cpp`, `AuthenticationTests.cpp`, `VerificationTests.cpp`, `FreshnessTests.cpp`, `CsmTests.cpp`, `CryIfTests.cpp`, `SecOCIntegrationTests.cpp`, `PQC_ComparisonTests.cpp`, `SoAdPqcTests.cpp`, `KeyExchangeTests.cpp`, `KeyDerivationTests.cpp`, `test_phase3_complete_ethernet_gateway.c`, `test_phase4_integration_report.c`, `test_pqc_secoc_integration.c`, `test_pqc_standalone.c`, `test/pqc_standalone_results.csv`, `integrated_simulation_env/README.md`, `ARCHITECTURE.md`, `docs/ATTACK_PLAYBOOK.md`, `config/scenarios.json`.
**Tổng số câu hỏi:** 18.

---

## A. Sử dụng Google Test framework

### Câu C.1 — GoogleTest C++ wrap pure C — rủi ro linker name-mangling

**Câu hỏi:** "GoogleTest là framework C++ — code SecOC/Csm là pure C. Em đã dùng cơ chế nào để wrap, và rủi ro khi linker xử lý các symbol có C++ name-mangling là gì?"

**Bằng chứng:** Trong `test/AuthenticationTests.cpp` (dòng 3-30) và mọi file `*Tests.cpp` em dùng:
```cpp
extern "C" {
#include "SecOC.h"
#include "Csm.h"
extern Std_ReturnType authenticate(const PduIdType TxPduId, ...);
extern Std_ReturnType authenticate_PQC(...);
}
```

**Đáp án mong đợi:**
- `extern "C"` block bao quanh tất cả `#include` của header C — đúng, ngăn C++ name-mangling.
- Tuy nhiên `extern Std_ReturnType authenticate(...)` cho hàm vốn được khai báo `STATIC` — `SecOCIntegrationTests.cpp` dòng 51-52 chú thích "STATIC is empty in non-RELEASE builds so they are linkable from tests" → vi phạm encapsulation và **MISRA C:2012 Rule 8.7/8.8 (internal linkage)**. Test build phải compile với macro `STATIC=` rỗng → **khác production build** → rủi ro test pass nhưng release build optimize khác đi.
- Em **không dùng Google Mock** (`SecOCTests.cpp` dòng 2: `// #include<gmock/gmock.h>` đã comment) → không thể inject mock cho `Csm`/`PduR`/`FVM` → tất cả "unit test" thực chất là integration test theo định nghĩa Fowler.

**Citations:**
- [GoogleTest primer](https://google.github.io/googletest/primer.html) (retrieved 2026-05-02)
- [C++ language linkage](https://en.cppreference.com/w/cpp/language/language_linkage) (retrieved 2026-05-02)
- [MISRA C:2012 Rule 8.7/8.8](https://misra.org.uk/product/misra-c2012-third-edition-first-revision/) (retrieved 2026-05-02)

### Câu C.2 — 669+ test cases phân bố thế nào? Module nào không có test?

**Câu hỏi:** "Em tuyên bố 669+ test cases trên 40 file. Phân bố thế nào? Module nào không có test?"

**Bằng chứng:** `test/CMakeLists.txt` dòng 5: `file(GLOB_RECURSE Sources CONFIGURE_DEPENDS "*.cpp")`. `CsmTests.cpp` ~32 `TEST_F`. `SecOCTests.cpp` chỉ còn **2 TEST hoạt động** — phần lớn (`SecOCCopyTXData1-4`, `example1-2`) bị comment-out (dòng 29-325) với lý do "require internal TP state management".

**Đáp án mong đợi:**
- 669+ chưa verify được trong source — em phải báo cáo bảng `<file>: <số TEST_F + TEST>`.
- **Lỗ hổng rõ:** module `BswM`, `EcuM`, `NvM`, `Os`, `Com` (liệt kê trong `ARCHITECTURE.md` mục 5) **không có file `*Tests.cpp` riêng**.
- Comment-out test thay vì xoá là **anti-pattern test rot**.

**Citations:**
- [Test rot — Martin Fowler](https://martinfowler.com/bliki/TestRot.html) (retrieved 2026-05-02)

### Câu C.3 — TDD hay test-after?

**Câu hỏi:** "Em làm TDD hay test-after?"

**Bằng chứng:** `AuthenticationTests.cpp` dòng 79-83, 152-156, 220-224 chứa `if (Result != E_OK) { SUCCEED(); return; }` — pattern điển hình test-after với behaviour-conditional.

**Đáp án mong đợi:**
- Pattern `SUCCEED()` khi function trả `E_NOT_OK` với lý do "PQC keys may not be available" → **test có thể pass kể cả khi feature hỏng** trong CI sandbox không có `/etc/secoc/keys/`. Đây là **red flag nghiêm trọng cho validity**.
- Báo cáo nên tách: trong 669 test, bao nhiêu thực sự chạy nhánh PQC end-to-end trên CI, bao nhiêu chỉ chạm code-path rồi `SKIP`/`SUCCEED`. Nếu phần lớn skip, tuyên bố "100% pass" mất ý nghĩa.

**Citations:**
- [Conditional asserts considered harmful (Google SWE Book)](https://abseil.io/resources/swe-book/html/ch11.html) (retrieved 2026-05-02)
- [GTEST_SKIP semantics](https://google.github.io/googletest/advanced.html#skipping-test-execution) (retrieved 2026-05-02)

---

## B. Coverage analysis

### Câu C.4 — 55% line coverage cho thesis claim CAL-2

**Câu hỏi:** "55% line coverage cho thesis claim CAL-2 — defend hoặc nhận khiếm khuyết."

**Đáp án mong đợi:**
- Nhận khiếm khuyết. ISO 21434 không quy định số cứng nhưng industry expect **≥80% statement** cho CAL-2; ISO 26262 ASIL-D đòi MC/DC ≥95%. 55% nghĩa là **45% code chưa được executed** → tuyên bố "100% pass" chỉ áp lên 55% codebase.
- Hành động: liệt kê function chưa cover; đặc biệt báo cáo error-handling paths có cover không (đây là phần thesis security claim phụ thuộc nhất); hạ tone từ "comprehensive" xuống "core path coverage".

**Citations:**
- [ISO/SAE 21434:2021 §10.4](https://www.iso.org/standard/70918.html) (retrieved 2026-05-02)
- [ISO 26262-6:2018 Table 12](https://www.iso.org/standard/68388.html) (retrieved 2026-05-02)
- [gcovr documentation](https://gcovr.com/en/stable/) (retrieved 2026-05-02)

### Câu C.5 — Loại trừ Mcal/Scheduler/GUIInterface/liboqs khỏi coverage — cherry-pick?

**Câu hỏi:** "Loại trừ Mcal/Scheduler/GUIInterface/liboqs khỏi coverage — hợp lý hay cherry-pick?"

**Bằng chứng:** `test/CMakeLists.txt` dòng 7-13 loại `McalTests.cpp` khi không phải PI4 — hợp lý. `Scheduler` và `GUIInterface` không có cơ chế tương tự.

**Đáp án mong đợi:**
- `liboqs/` exclude **hợp lý** — third-party đã có NIST KAT, ACVTS upstream.
- `Mcal/` exclude **chấp nhận được** với SIM build, nhưng phải báo "55% của ~70% codebase ngoài MCAL".
- `Scheduler` và `GUIInterface` exclude là **cherry-pick**: cả hai đều là code em viết, thuộc scope CAL-2.
- Đề xuất: tính 2 con số — "core SecOC/Csm/PQC coverage" và "full project coverage" — và defend trên con số cao hơn nhưng minh bạch.

**Citations:**
- [Goodhart's Law](https://en.wikipedia.org/wiki/Goodhart%27s_law) (retrieved 2026-05-02)
- [gcovr exclusions](https://gcovr.com/en/stable/cookbook.html#excluding-code-from-coverage) (retrieved 2026-05-02)

### Câu C.6 — Branch coverage 60% — branch nào không cover được?

**Câu hỏi:** "Branch coverage 60% — chỗ nào branch không cover được?"

**Bằng chứng:** `CsmTests.cpp` dòng 156-162: `if (result == E_OK) { EXPECT_GT(...) }` không có nhánh `else` → nếu function luôn `E_NOT_OK` test vẫn pass nhưng cả 2 branch chưa kiểm chứng.

**Đáp án mong đợi:** Branch khó cover điển hình:
1. Defensive null-check trên public API — một số có (`Csm_MacGenerate` NULL data, dòng 70-81), nhiều khác không.
2. Error return từ liboqs — `OQS_KEM_keypair` rất hiếm fail → branch error không test.
3. Buffer-overflow guard `SecOC_CopyRxData` (`SecOCTests.cpp:230-277` đã commented).
4. DET callout khi `SECOC_DEV_ERROR_DETECT == TRUE` — DET log branch không kiểm vì test build có thể tắt.
- Để cải thiện cần **fault-injection mocking** — em chưa có.

**Citations:**
- [gcov branches](https://gcc.gnu.org/onlinedocs/gcc/Invoking-Gcov.html) (retrieved 2026-05-02)
- [Mutation testing — pitest](https://pitest.org/quickstart/basic_concepts/) (retrieved 2026-05-02)

---

## C. Unit tests vs Integration tests

### Câu C.7 — Phase-1/2/3/4 nghĩa là gì?

**Câu hỏi:** "Phase-1/2/3/4 nghĩa là gì?"

**Bằng chứng:** `test_pqc_standalone.c:9-17` "Phase 1: ML-KEM-768", "Phase 2: ML-DSA-65"; `test_phase3_complete_ethernet_gateway.c:4-20` integration; `test_phase4_integration_report.c:4-20` chỉ là **report generator** in ASCII art architecture.

**Đáp án mong đợi:**
- Phase-4 **không phải test**, là tool sinh báo cáo — không nên đếm vào "test cases".
- Đặt tên Phase-X là ad-hoc, không ánh xạ với taxonomy chuẩn ISTQB/IEEE 829/29119 (unit/integration/system/acceptance).

**Citations:**
- [ISO/IEC/IEEE 29119-1:2022](https://www.iso.org/standard/81291.html) (retrieved 2026-05-02)
- [ISTQB Glossary](https://glossary.istqb.org/) (retrieved 2026-05-02)

### Câu C.8 — Csm test có mock không?

**Câu hỏi:** "CsmTests có mock CryIf/liboqs không? Nếu real, đây là integration trá hình unit."

**Bằng chứng:** `CsmTests.cpp` dòng 24-32 SetUp gọi `PQC_Init()` rồi `Csm_Init` với `CSM_MLDSA_BOOTSTRAP_DEMO_FILE_AUTO` → đọc/sinh keypair thực tế filesystem.

**Đáp án mong đợi:**
- **Không có mock**. Mọi "unit" của Csm gọi xuống PQC → liboqs → OS RNG → file I/O.
- Tradeoff: real crypto cho gold-standard behaviour và số liệu performance đáng tin, nhưng (i) chậm; (ii) flaky với filesystem (`SECOC_IT_SKIP_IF_NO_PQC_KEYS` macro `SecOCIntegrationTests.cpp:90-97` thừa nhận); (iii) không thể test fault-injection.
- Đề xuất chuẩn: GMock cho `Csm` mock vào SecOC test, `CryIf` mock vào Csm test — test pyramid Cohn.

**Citations:**
- [Test pyramid — Martin Fowler](https://martinfowler.com/articles/practical-test-pyramid.html) (retrieved 2026-05-02)
- [GMock for C](https://google.github.io/googletest/gmock_for_dummies.html) (retrieved 2026-05-02)

### Câu C.9 — Standalone .c programs chạy thế nào? Có nằm trong CTest không?

**Câu hỏi:** "Standalone .c programs chạy thế nào? Có nằm trong CTest không?"

**Bằng chứng:** `test/CMakeLists.txt` dòng 5: `file(GLOB_RECURSE Sources CONFIGURE_DEPENDS "*.cpp")` — **chỉ glob `.cpp`** → các file `.c` (`test_pqc_standalone.c`, `test_phase3_*.c`, `test_phase4_*.c`, `test_pqc_secoc_integration.c`) **không nằm trong CTest**.

**Đáp án mong đợi:**
- Các file `.c` là standalone, không link gtest, không chạy `ctest`. Build qua `bash build_test_pqc.sh`, `bash build_perf.sh`.
- Hệ quả: nếu CI chỉ chạy `ctest`, **4 standalone không bao giờ kiểm tra trên Pi4** → regression có thể trượt qua. Em phải thêm shell-level integration test trong pipeline.
- `test_phase4_integration_report.c` chỉ in văn bản → không nên đếm vào test count.

**Citations:**
- [CTest manual](https://cmake.org/cmake/help/latest/manual/ctest.1.html) (retrieved 2026-05-02)
- [CMake GLOB_RECURSE](https://cmake.org/cmake/help/latest/command/file.html#glob-recurse) (retrieved 2026-05-02)

---

## D. ISE (Integrated Simulation Environment)

### Câu C.10 — ISE là self-built? Có cross-validate hardware không?

**Câu hỏi:** "ISE là self-built? Bao nhiêu SLOC? Có cross-validate với hardware không?"

**Bằng chứng:** `integrated_simulation_env/README.md:1-46` — ISE tự phát triển; `ARCHITECTURE.md:81-98` re-use SecOC source và chỉ thay MCAL bằng sim shim.

**Đáp án mong đợi:**
- ISE **tự code** — không phải Vector CANoe, không phải Synopsys VCS. Open-source equivalent: OpenDavinci, CarMaker open. Em phải defend lý do không adopt.
- **Validity threat critical:** Nếu simulator có bug, mọi kết quả 52-scenarios/8316-frames **mất giá trị external**. Cần ít nhất một **cross-check co-simulation** — chạy 1 scenario trên ISE và Pi4 hardware, so latency.
- Self-reference risk: nếu em dùng ISE để debug bug ISE chính nó.

**Citations:**
- [Vector CANoe](https://www.vector.com/int/en/products/products-a-z/software/canoe/) (retrieved 2026-05-02)
- [Co-simulation validation IEEE](https://ieeexplore.ieee.org/document/8703088) (retrieved 2026-05-02)
- [OpenDavinci](https://github.com/se-research/OpenDaVINCI) (retrieved 2026-05-02)

### Câu C.11 — 5 virtual buses — model chính xác đến đâu?

**Câu hỏi:** "5 virtual buses — model chính xác đến đâu?"

**Bằng chứng:** `ARCHITECTURE.md` mục 4 (dòng 53-75) `SimBusCfg` chỉ có `bitrate_bps`, `mtu_bytes`, `propagation_ns`, `bit_error_rate`, FlexRay `slot_count`/`slot_duration_ns`. **Không có CAN priority arbitration**, error frame, bus-off recovery, TSN gating cho Ethernet (chỉ flag `tsn_enabled`).

**Đáp án mong đợi:** Model là bandwidth + delay + BER + (optional) TDMA — cấp thấp. Thiếu so với ISO 11898-1:
- Bit-stuffing dynamic frame size.
- CRC/ACK field timing.
- **Arbitration multiple senders** — CAN thực tế lower-priority lùi → ảnh hưởng latency. ISE chỉ FIFO theo `tx_ns`.
- Bus-off recovery (Tec/Rec 8-bit counter).
Cho Ethernet: thiếu PCP queuing, CBS/TAS scheduler TSN — chỉ flag boolean.
- Hệ quả: ISE latency là **upper bound nhẹ**, không phải worst-case real → phải khai báo trong threats-to-validity.

**Citations:**
- [ISO 11898-1:2015](https://www.iso.org/standard/63648.html) (retrieved 2026-05-02)
- [IEEE 802.1Qbv TSN](https://standards.ieee.org/standard/802_1Qbv-2015.html) (retrieved 2026-05-02)
- [FlexRay v3.0.1](https://www.iso.org/standard/63102.html) (retrieved 2026-05-02)

### Câu C.12 — 3 ECU + 10 attacker — design rationale

**Câu hỏi:** "3 ECU + 10 attacker — design rationale."

**Bằng chứng:** `README.md:24-25` "3 ECUs (Tx, Rx, Gateway)"; `ARCHITECTURE.md` mục 2 confirm topology Tx ↔ Gateway ↔ Rx.

**Đáp án mong đợi:**
- 3-ECU đủ cho gateway routing demo (CAN-FD ↔ Ethernet) — core thesis.
- Không cover được: multi-ECU broadcast với verify outcome khác nhau; bus contention arbitration; key management scale (xe thực tế 50-100 ECU).
- Code có infrastructure multi-peer (`PQC_KeyExchange.c`, `Csm_MAX_PEERS`) nhưng **scenario test không khai thác hết** — gap giữa code khả năng và test khả năng.

**Citations:**
- [E/E architecture trends — Bosch](https://www.bosch-mobility.com/en/mobility-topics/e-e-architecture/) (retrieved 2026-05-02)
- [AUTOSAR Methodology R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_TR_Methodology.pdf) (retrieved 2026-05-02)

---

## E. Attack injection

### Câu C.13 — 7 vector hay 10? Mỗi vector inject gì?

**Câu hỏi:** "7 vector hay 10? Mỗi vector inject gì?"

**Bằng chứng:** `ATTACK_PLAYBOOK.md` mô tả **10** attacker; `config/scenarios.json:31-42` cũng list 10. Tuyên bố thesis "7 vectors, 99.14%".

**Đáp án mong đợi:**
- Discrepancy 7 vs 10. Có 2 khả năng: (a) em chỉ count active attack — bỏ harvest_now, timing_probe (passive), dos_flood (availability) khỏi "detection rate" vì không có boolean detected/missed; (b) `sim_attacker.c` chưa implement đủ.
- Em phải làm rõ 7 vector cụ thể nào và defend denominator.
- Replay 47/47 = 100% claim riêng, nhưng aggregate 99.14% có thể skew do tamper_payload hoặc khác — phải break-down từng vector.

**Citations:**
- [UN R155 Annex 5](https://unece.org/sites/default/files/2021-03/R155e.pdf) (retrieved 2026-05-02)
- [ISO/SAE 21434:2021](https://www.iso.org/standard/70918.html) (retrieved 2026-05-02)

### Câu C.14 — Replay 47/47 nhưng aggregate 94% — denominator pollution

**Câu hỏi:** "Replay 47/47 (100%) nhưng aggregate 94% — denominator pollution là gì và bạn defend thế nào?"

**Đáp án mong đợi:**
- "Denominator pollution" là vấn đề **đo lường**, không phải bug security. Nếu log gồm 47 replay + 3 legitimate frame chen vào → Total=50, detected_as_attack=47 → "94% detection rate" sai về định nghĩa.
- Cách tính đúng: **detection rate = TP/(TP+FN)** chỉ với attack frames; **false positive rate = FP/(FP+TN)** với legitimate. Trộn 2 con số là **methodological flaw**.
- Phải tách 2 metric, không quote "99.14% aggregate" như một số duy nhất.

**Citations:**
- [IDS confusion matrix — NIST](https://csrc.nist.gov/glossary/term/intrusion_detection_system) (retrieved 2026-05-02)
- [Precision/Recall](https://en.wikipedia.org/wiki/Precision_and_recall) (retrieved 2026-05-02)

### Câu C.15 — Downgrade attack 0% trong HMAC mode — n/a thay vì fail

**Câu hỏi:** "Downgrade attack 0% trong HMAC mode — defend 'n/a' thay vì 'fail'."

**Bằng chứng:** `ATTACK_PLAYBOOK.md:67-75` "claims to be HMAC-protected when both ECUs are in PQC mode". HMAC mode không có gì để "downgrade xuống".

**Đáp án mong đợi:**
- Đúng — `downgrade_hmac` chỉ applicable cho PQC mode. Trong configuration HMAC, attack vô nghĩa → "n/a" chính xác.
- Tuy nhiên phải report rõ: nếu count vector này như "passed by default" trong HMAC → inflate aggregate.
- Đề xuất: 2 bảng riêng — HMAC mode (8 vector applicable) và PQC mode (10 applicable) — KHÔNG aggregate cross-mode.

**Citations:**
- [POODLE downgrade precedent](https://en.wikipedia.org/wiki/POODLE) (retrieved 2026-05-02)
- [ISO 26262-6:2018](https://www.iso.org/standard/68388.html) (retrieved 2026-05-02)

### Câu C.16 — Coverage attack space — class bị bỏ qua

**Câu hỏi:** "Coverage attack space — class nào bị bỏ qua?"

**Bằng chứng:** `ATTACK_PLAYBOOK.md` chỉ có timing_probe (passive observation, không phải DPA). Không có fault injection.

**Đáp án mong đợi:**
- Phải khai báo rõ: ISE là **logical-layer simulator** → không thể mô phỏng:
  - Side-channel power analysis (cần osciloscope GHz).
  - Fault injection EMFI/laser (cần hardware vật lý).
  - Supply chain (compromised liboqs binary).
- Trong threat model thesis: "ISE chỉ cover logical attack surface theo UN R155 Annex 5 §4.3; physical/supply-chain ngoài phạm vi".
- Attack AUTOSAR-specific bị bỏ: GW spoofing, firmware rollback, UDS 0x27 SecurityAccess bypass.

**Citations:**
- [ISO/SAE 21434 §15](https://www.iso.org/standard/70918.html) (retrieved 2026-05-02)
- [Side-channel ML-KEM/ML-DSA 2024 IACR ePrint 2024/1626](https://eprint.iacr.org/2024/1626) (retrieved 2026-05-02)
- [AUTOSAR Security Extensions R21-11](https://www.autosar.org/fileadmin/standards/R21-11/FO/AUTOSAR_TR_SecurityExtensions.pdf) (retrieved 2026-05-02)

---

## F. Performance benchmarks methodology

### Câu C.17 — 1000 iterations đủ cho p99.9 không?

**Câu hỏi:** "1000 iterations đủ cho p99.9 không?"

**Bằng chứng:** `test_pqc_standalone.c:53` `#define NUM_ITERATIONS 1000`. CSV `pqc_standalone_results.csv` báo Avg/Min/Max/Stddev — **không có cột p50/p95/p99/p99.9**.

**Đáp án mong đợi:**
- 1000 mẫu là **mức tối thiểu** cho p99.9 — quy tắc 1/(1−0.999) = 1000 cho 1 quan sát ở quantile 99.9 → standard error rất lớn (Wilson interval ±15-25%).
- Để claim p99.9 reliably cần **≥10⁵ mẫu** (HdrHistogram convention).
- CSV không báo percentile, chỉ Min/Max/Stddev. `Encapsulate Max=28536.60µs` (dòng 4) cho thấy outlier 130x median — có thể OS scheduler preemption, không phải crypto. Cần loại outlier hoặc dùng **median + MAD** thay mean + stddev (latency long-tail, không Gaussian).
- Đề xuất: HdrHistogram + p50/p95/p99/p99.9/max riêng; tăng iter lên 10⁵.

**Citations:**
- [HdrHistogram (Gil Tene)](http://www.hdrhistogram.org/) (retrieved 2026-05-02)
- [Latency percentile sampling — Brendan Gregg](https://www.brendangregg.com/blog/2018-02-09/kpti-kaiser-meltdown-performance.html) (retrieved 2026-05-02)
- [Wilson score interval](https://en.wikipedia.org/wiki/Binomial_proportion_confidence_interval#Wilson_score_interval) (retrieved 2026-05-02)

### Câu C.18 — x86_64 vs Pi 4 — vi-kiến-trúc giải thích?

**Câu hỏi:** "x86_64 vs Pi 4 — sự khác biệt 0.36x đến 1.68x. Tại sao Encap nhanh hơn trên Pi 4 mà Decap chậm hơn?"

**Bằng chứng:** CSV `pqc_standalone_results.csv` chỉ có 1 dataset, **không có cột platform** — em cần xác nhận nguồn số Pi vs x86 từ đâu.

**Đáp án mong đợi:**
- Cortex-A72 Pi 4 là ARMv8.0-A — **không có ARMv8.4 SHA3 extension**. ML-DSA dùng SHAKE-256/Keccak nội bộ → trên Pi không có HW acceleration → chậm hơn x86.
- Encap chậm hơn trên Pi (1.5-2x) hợp lý vì bottleneck Keccak.
- Decap nhanh hơn trên Pi nghe **kỳ lạ** — cần kiểm tra: noise đo (1000 sample không đủ); cache effect (Pi L2 1MB shared 4-core, x86 desktop L2 256KB per-core + L3 nhiều MB → behavior khác cho working set Decap); compile flag (`-O2` vs `-O3` vs `-march=native`).
- CSV hiện không có CPU model, frequency, compiler flags → kết quả **non-reproducible**. Khuyến nghị dùng liboqs upstream benchmark suite để có baseline.

**Citations:**
- [Cortex-A72 TRM (no SHA3)](https://developer.arm.com/documentation/100095/0003) (retrieved 2026-05-02)
- [ARMv8.4-A SHA3](https://developer.arm.com/architectures/learn-the-architecture/armv8-a-instruction-set-architecture) (retrieved 2026-05-02)
- [liboqs ML-KEM bench](https://github.com/open-quantum-safe/liboqs/blob/main/docs/algorithms/kem/ml_kem.md) (retrieved 2026-05-02)
- [Dilithium reference perf](https://pq-crystals.org/dilithium/) (retrieved 2026-05-02)

---

## G. Reproducibility

### Câu C.19 — Artifact dating — re-run hôm nay có khớp?

**Câu hỏi:** "Artifact dating — re-run hôm nay có khớp không?"

**Bằng chứng:** `pqc_standalone_results.csv` không có cột timestamp/seed/git-hash. `ARCHITECTURE.md:100-110` ghi "Every scenario accepts `--seed`" cho ISE nhưng "Crypto material remains non-deterministic by design".

**Đáp án mong đợi:**
- ISE: deterministic mode khả thi cho bus/attack/signal nhưng crypto luôn random → 2 run khác signature bytes nhưng performance distribution **nên trong ±5-10% mean**.
- Standalone CSV: không seed → không reproducible.
- Hành động cần: (i) embed git-hash + timestamp + platform info + compiler flags vào mỗi CSV row; (ii) re-run sát ngày bảo vệ trên cùng platform; (iii) báo cáo % drift — nếu >10% → flakiness chưa kiểm soát.

**Citations:**
- [Reproducibility crisis — Reproducible Builds](https://reproducible-builds.org/docs/reproducible-research/) (retrieved 2026-05-02)
- [ACM Reproducibility Badging](https://www.acm.org/publications/policies/artifact-review-and-badging-current) (retrieved 2026-05-02)

### Câu C.20 — CI pipeline + compliance report

**Câu hỏi:** "CI pipeline + compliance report — chạy on-demand hay continuous?"

**Bằng chứng:** HEAD `8fe3080 Remove MISRA compliance reports`. Không thấy `.github/workflows/` hay `.gitlab-ci.yml` được mô tả trong các file đã đọc.

**Đáp án mong đợi:**
- Em phải answer: (a) CI có chạy mỗi push không? Provider gì? (b) `generate_compliance_report.py` trigger thế nào? (c) Commit `2a031b9` ở đâu — có rebase mất rồi không (HEAD là `8fe3080`)?
- 35 PASS / 1 WARN / 0 FAIL — phải giải thích **WARN nào** và tại sao không fix trước bảo vệ.
- Nếu CI báo cáo dùng artifact stale → mọi tuyên bố compliance cần re-validate.

**Citations:**
- [GitHub Actions C/C++](https://docs.github.com/en/actions/automating-builds-and-tests/building-and-testing-c-or-c) (retrieved 2026-05-02)
- [ISO/SAE 21434:2021 §6.4.4 traceability](https://www.iso.org/standard/70918.html) (retrieved 2026-05-02)

---

## Tổng kết phản biện

### Điểm mạnh thực sự
1. **Cấu trúc test có tổ chức:** 40 file `.cpp` phân theo module (Csm, CryIf, Freshness, KeyExchange, KeyDerivation, SoAdPqc, SecOC, ...).
2. **Có integration test thực sự** (`SecOCIntegrationTests.cpp`) trace tới SWS requirements (`SWS_SecOC_00031`, `00079`, `00094`, `00112`, `00177`) — điểm sáng.
3. **ISE có deterministic mode + seeded RNG** — khả thi cho reproducibility nếu hoàn thiện.
4. **Phân biệt PQC vs Classical mode** trong cùng codebase (`#if SECOC_USE_PQC_MODE`) — test cả hai.
5. **Đo metrics định lượng** (CSV với min/max/avg/stddev), không chỉ "test pass".

### Điểm yếu nghiêm trọng cần defend
1. **55% line coverage thấp dưới chuẩn CAL-2** (≥80% expected). Cherry-pick exclusion `Scheduler`, `GUIInterface` làm con số càng yếu khi tính lại.
2. **Không có Mock infrastructure** — mọi "unit" thực chất là integration → không thể fault-inject.
3. **Conditional `if (Result == E_OK) ... else SUCCEED()`** trong `AuthenticationTests.cpp:79-83`, `PQC_ComparisonTests.cpp:130-138`, `CsmTests.cpp:156-162` cho phép test pass khi feature hỏng → **"100% pass" mất ý nghĩa** trong CI thiếu PQC keys.
4. **669+ chưa verify** và `test_phase4_integration_report.c` không phải test (chỉ in báo cáo).
5. **4 standalone `.c` programs ngoài CTest** (`file(GLOB_RECURSE *.cpp)` không bắt `.c`) → CI có thể bỏ qua.
6. **ISE simulator self-built** không có cross-validation hardware → internal validity OK, external validity questionable.
7. **Bus model thiếu** arbitration CAN, TSN gating Ethernet, bus-off recovery → latency là estimate.
8. **1000 iterations chưa đủ p99.9**; CSV không report percentile; outlier 130x median chưa giải thích.
9. **Aggregate 99.14% detection** trộn vector applicable/non-applicable, mode HMAC/PQC, attack/legitimate denominators → metrics flawed.
10. **CSV không có git-hash, timestamp, seed, platform info** → non-reproducible.
11. **Test rot:** >200 dòng `TEST` comment-out trong `SecOCTests.cpp` thay vì xoá hoặc fix.
12. **Không có mutation testing** — không biết test có thực sự catch bug.

### Câu hỏi quyết định cho hội đồng
> Em tuyên bố **100% pass rate trên 669+ test với ~55% coverage**. Trong 45% code chưa cover, có bao nhiêu là **error-handling path** (DET callout, fail-safe transitions)? Đây chính là phần code mà thesis security claim **phụ thuộc nhất** — error handling sai → silent verification failure → an toàn giả tạo. Nếu em chưa biết, đó là **gap critical** cho CAL-2 evidence.
