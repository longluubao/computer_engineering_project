# Thesis Defense — Q&A + Dẫn chứng

Tài liệu chuẩn bị phản biện cho đồ án **AUTOSAR SecOC + Post-Quantum
Cryptography (ML-KEM-768 + ML-DSA-65) trên Classic AUTOSAR gateway
vECU**. Mỗi câu hỏi có: câu trả lời ngắn, dẫn chứng (SWS / paper /
industry reference), và tham chiếu scenario trong ISE để minh hoạ.

---

## A. Kiến trúc & nền tảng

### Q1. Zonal/HPC hiện đại thì dùng Adaptive AUTOSAR, sao em dùng Classic?

**Đáp.** Zone controllers trong production (Bosch VCC, Aptiv SVA, ZF
ProAI, NXP S32G reference) đều dùng **Classic cho partition
safety-critical routing** (Cortex-R52/M7 lockstep); Adaptive chỉ ở
central HPC chạy ADAS/IVI. Pattern dominant hiện nay là **Classic +
Adaptive co-resident qua hypervisor** (PikeOS, QNX, COQOS), không
phải "Adaptive thay Classic".

**Dẫn chứng.** Vector Congress 2023 "Zonal E/E Architecture"; NXP S32G
reference architecture; SAE 2023-01-0046; ELIV 2023.

---

### Q2. Tại sao không làm trên Adaptive cho "hiện đại" hơn?

**Đáp.** **SecOC chỉ tồn tại trong Classic AUTOSAR** (R21-11 SWS 654).
Adaptive bảo mật bằng **SOME/IP-TLS + IDSM**, không có module SecOC.
PQC trên Adaptive = tích hợp liboqs vào OpenSSL TLS → đóng góp IETF
(RFC 9794 hybrid TLS), không phải AUTOSAR. PQC trong SecOC = đóng góp
trực tiếp vào chuẩn AUTOSAR Classic.

**Dẫn chứng.** AUTOSAR_SWS_SecOC R21-11; AUTOSAR_SWS_Adaptive_Platform
(không có SecOC); draft-ietf-tls-hybrid-design.

---

### Q3. Em chạy trên Raspberry Pi 4 Linux — đó đâu phải AUTOSAR thật?

**Đáp.** Đây là **Virtual ECU (vECU) methodology** — chuẩn AUTOSAR.
Cùng pattern với Vector vVIRTUALtarget, dSPACE VEOS, ETAS ISOLAR-EVE,
Synopsys Silver. BSW module (SecOC/PduR/Com/CanIf) viết chống lại
**SchM abstractions**, không phải OSEK primitives → **OS-agnostic by
AUTOSAR design**. Port sang AUTOSAR OS (Erika Enterprise, MICROSAR
OS) là mechanical follow-on.

**Dẫn chứng.** AUTOSAR Virtual Functional Bus (VFB); Vector
vVIRTUALtarget whitepaper; Erika Enterprise (ReTiS Lab Pisa, OSEK
open-source).

---

### Q4. Pi 4 có đại diện cho ECU thật không?

**Đáp.** Pi 4 **Cortex-A72 @1.5 GHz** cùng family với **NXP S32G
(A53)** và **Renesas R-Car (A57/A72)** — đúng class ECU nơi PQC sẽ
deploy. Nếu chạy Cortex-M4 thì số đo mới không đại diện; A72 cho
ML-DSA/ML-KEM timing **meaningful** cho HPC/zonal target thật.

**Dẫn chứng.** NXP S32G datasheet; Renesas R-Car S4 datasheet.

---

### Q5. Linux không deterministic, làm sao claim hard real-time?

**Đáp.** Scope thesis là **functional validation + security property**,
không phải timing certification. vECU stage mục tiêu validate logic;
hard real-time là **target ECU stage** riêng (AUTOSAR OS + lockstep
core). Thesis đã log **deadline miss per ASIL class**
(`sc_deadline_stress`) làm conservative worst-case — production trên
AUTOSAR OS sẽ tốt hơn.

**Dẫn chứng.** AUTOSAR methodology phase model; `sc_deadline_stress`
summary JSON.

---

## B. Scope thesis

### Q6. Sao thesis không cover EcuM Sleep/Wake, Dcm UDS 0x27, Dem DTC?

**Đáp.** Three scope out-of-scope **chủ động**, có lý do:

- **EcuM Sleep/Wake + PNC** — power-management orthogonal với
  cryptographic path.
- **Dcm 0x27** — gateway thuần route (PduR TpGateway); 0x27 thuộc ECU
  chứa resource (ISO 14229-1 §10.4).
- **Dem DTC** — SWS_SecOC_00226 quy định **receiver** log, không phải
  gateway.

Scope thesis = **security layer** (SecOC + PQC + freshness), không phải
full-ECU lifecycle.

---

### Q7. Sao không có ARXML / RTE?

**Đáp.** Gateway scope **BSW trở xuống**, không có application SWC →
không cần RTE, không cần ARXML system description. Đây là pattern
chuẩn cho gateway/zonal controller.

**Dẫn chứng.** AUTOSAR methodology — RTE chỉ cần cho application ECU
có SWC; gateway routing không cần.

---

## C. Kỹ thuật PQC

### Q8. ML-DSA signature 3309 bytes nhét vô CAN 8 byte MTU thế nào?

**Đáp.** Qua **CAN-TP fragmentation** — ~414 fragments trên CAN 2.0, 52
fragments trên CAN-FD (64 B MTU). Bài toán khó của thesis chính là
đây. Scenario `sc_baseline` + `sc_throughput` đo end-to-end latency
có fragmentation; `sc_bus_failure` đo BER impact (fragment càng
nhiều, xác suất drop càng cao → verify_fail tăng).

**Dẫn chứng.** `pdu_bytes` + `fragments` histogram trong `sim_metrics`;
kết quả BER 1e-4 trên PQC CAN-FD → 93/100 verify_fail.

---

### Q9. ML-DSA sign 250µs có meet realtime deadline không?

**Đáp.** Tùy ASIL class. `sc_deadline_stress` chứng minh:

- BrakeCmd (ASIL-D, D1=5ms) trên CAN 2.0 với PQC → **100% miss**
  (signature quá lớn cho CAN low-speed)
- Speedometer (ASIL-B, D4=50ms) trên Eth 100 → **0% miss**

**Kết luận.** PQC phù hợp với **high-bandwidth bus + relaxed deadline**
(Ethernet, ASIL-B/C). ASIL-D trên CAN nên dùng **HMAC hoặc HYBRID
fast-reject**. Đây là finding quan trọng của thesis.

---

### Q10. Hybrid mode (HMAC + ML-DSA) có ý nghĩa gì?

**Đáp.** HMAC check trước (fast-reject ~2µs); nếu pass mới ML-DSA
verify (~120µs). Forged frame bị reject sớm — chống DoS qua
signature-verify flooding.

**Dẫn chứng.** `sim_ecu.c:137-147, 223-233`; scenario `sc_attacks
--protection hybrid`.

---

### Q11. ML-KEM handshake lâu bao nhiêu, có phù hợp gateway không?

**Đáp.** KeyGen + Encap + Decap + HKDF trung bình **~400µs** đo trong
`sc_rekey`. Chỉ chạy khi rekey (rate 1 lần/giờ hoặc sau
`freshness_tx` gần exhaust) — overhead amortize.

**Dẫn chứng.** `sc_rekey` summary JSON.

---

## D. Security property & tests

### Q12. Làm sao chứng minh SecOC thực sự reject attack?

**Đáp.** `sc_attacks` inject 7 loại tấn công, mỗi loại 40 iter:

1. Replay (freshness cũ)
2. Tamper payload
3. Tamper authenticator
4. Freshness rollback
5. Key confusion (MITM XOR)
6. Signature fuzz
7. Downgrade HMAC

**Kết quả.** `attacks_delivered = 0` cho cả 7 loại → 100% detection.
JSON summary ghi: `attacks_injected / attacks_detected /
attacks_delivered`.

---

### Q13. Nếu attacker chôm ML-DSA private key thì sao?

**Đáp.** Đó là compromise scenario — không detect được ở runtime
(signature hợp lệ). Mitigation là **secure key storage (HSM/TPM)** +
rekey window. `sc_keymismatch` chứng minh binding cryptographic: nếu
key sai, 100% reject (SWS_SecOC_00046). `sc_rollover` chứng minh
freshness không ngăn được key compromise (đó là lý do cần HSM).

---

### Q14. Gateway re-sign có đúng AUTOSAR TpGateway không?

**Đáp.** Đúng. `sc_mixed_bus`: gateway nhận CAN-side → verify → extract
payload → **build lại PDU mới với freshness + ML-DSA signature mới**
→ gửi Ethernet. Không phải raw forward. Metric `secoc_auth` trên bus
Eth ghi rõ re-sign overhead. Flow diagram comment trong
`sc_mixed_bus.c` vẽ đầy đủ 2 hop.

---

### Q15. Freshness counter overflow xử lý thế nào?

**Đáp.** `sc_rollover` test 3 phase:

- Approach UINT64_MAX: 8/8 accept
- Wrap to 0: **0 delivered, 8/8 rejected** (AUTOSAR compliant — không
  silent accept)
- Rekey recovery: 16/16 accept sau khi reset cả hai side về 0

**Kết luận.** SWS_SecOC_00033 compliant. Operational mitigation =
rekey trước khi counter exhaust.

---

### Q16. NvM persistence thế nào?

**Đáp.** `sc_persistence` chứng minh SWS_SecOC_00194:

- **No NvM** — sau power cycle, RX freshness reset 0 → replay cũ được
  accept → **breach**
- **With NvM** — RX freshness restore từ NvM → replay cũ bị reject →
  **safe**

**Quantified.** `attacks_delivered` no_nvm > 0; with_nvm = 0.

---

## E. Đóng góp & so sánh

### Q17. Đóng góp chính của thesis là gì?

**Đáp.** Ba đóng góp chính:

1. **Tích hợp PQC (ML-KEM-768 + ML-DSA-65) vào SecOC Classic** — không
   có implementation reference nào khác làm đầy đủ đến mức này.
2. **Integrated Simulation Environment (ISE)** với 11 scenarios đo
   end-to-end: latency, overhead, attack detection, deadline, BER,
   rollover, key binding, NvM persistence, multi-ECU broadcast.
3. **Quantified finding.** PQC SecOC khả thi cho **Ethernet-based
   zonal**, không khả thi cho **CAN ASIL-D** → guidance cho OEM chọn
   protection mode theo bus + ASIL.

---

### Q18. So với paper/work khác thì đóng góp gì mới?

**Đáp.**

- Existing work: Wang et al. (PQC trên TLS automotive), Paul et al.
  (PQC benchmark trên MCU).
- **Thesis này** đặt PQC vào **đúng module SecOC** theo SWS R21-11,
  test **full BSW stack (PduR, CanTp, SoAd, CanIf)**, với
  **multi-ECU broadcast + gateway re-sign + freshness persistence +
  rollover recovery**. Không work nào trước đó cover end-to-end
  gateway scope như vậy.

---

### Q19. Nếu làm lại sẽ làm gì khác?

**Đáp.**

- Port sang **Erika Enterprise** (OSEK open-source) để có AUTOSAR OS
  thật.
- Thêm **truncated MAC mode** (4B/8B) cho CAN ASIL-D.
- **HSM integration** (TPM2 / OP-TEE) cho private key storage.
- **EcuM Sleep/Wake + PNC** cho power management.

---

## F. Công cụ & kết quả

### Q20. Làm sao verify kết quả reproducible?

**Đáp.**

- Seed deterministic (`--seed`), deterministic clock
  (`--deterministic`).
- 11 scenario chạy qua `run_all_scenarios.sh` → output timestamped
  folder với `summary/*.json`, `raw/*_frames.csv`, `report.md`,
  `compliance_matrix.md`.
- 9/9 ctest pass. Git tagged commit.

**Dẫn chứng.** `integrated_simulation_env/results/20260418T101627Z/`.

---

### Q21. Compliance với AUTOSAR SWS như thế nào?

**Đáp.** `compliance_matrix.md` map từng SWS requirement → scenario
evidence:

| SWS ID | Requirement | Scenario |
|---|---|---|
| SWS_SecOC_00033 | Freshness strictly monotonic | `sc_rollover` |
| SWS_SecOC_00046 | Key-identified verification | `sc_keymismatch` |
| SWS_SecOC_00194 | NvM freshness persistence | `sc_persistence` |
| SWS_SecOC_00226 | DEM reporting on verify fail | `sc_attacks` (log pos) |
| NIST FIPS 203 | ML-KEM-768 KEM | `sc_rekey` (liboqs) |
| NIST FIPS 204 | ML-DSA-65 signature | `sc_baseline` (liboqs) |

---

## 🎯 3 câu "golden" nếu bị dồn vào thế khó

### 1. "Tại sao Classic + Pi + Linux?"

> "Đây là **Virtual ECU methodology chuẩn AUTOSAR** — cùng pattern với
> Vector vVIRTUALtarget và dSPACE VEOS. SecOC là module Classic-only
> (SWS 654), không có tương đương trong Adaptive. Pi 4 A72 đại diện
> cho HPC target thật (S32G A53, R-Car A72). Port sang AUTOSAR OS là
> mechanical follow-on."

### 2. "Đóng góp là gì?"

> "Tích hợp PQC vào đúng module SecOC với **full BSW stack test** — 11
> scenario cover freshness persistence, rollover, key binding,
> multi-ECU broadcast, gateway re-sign, bus BER, deadline per ASIL.
> Finding chính: **PQC khả thi cho Ethernet ASIL-B/C, cần
> HMAC/HYBRID cho CAN ASIL-D**."

### 3. "Security có đúng không?"

> "7 loại attack test trong sc_attacks + wrong-key + freshness
> rollover → **attacks_delivered = 0** cho tất cả. Compliance trực
> tiếp với SWS_SecOC_00033 / 00046 / 00194 / 00226. Reproducible qua
> seed + git tag."

---

## Preparation checklist

- [ ] In sẵn `summary/compliance_matrix.md` làm backup slide.
- [ ] Chuẩn bị 1 slide riêng cho Q1 (Classic vs Adaptive), Q8
  (fragmentation), Q17 (contribution) — 3 câu nhiều khả năng bị hỏi
  nhất.
- [ ] Demo live `./run_all_scenarios.sh 100 1` (5 phút) nếu được cho
  phép.
- [ ] Nhớ tên **Virtual ECU (vECU) methodology** — cụm từ vàng.
- [ ] Nhớ số **3309 bytes ML-DSA**, **1184 bytes ML-KEM pubkey**, **32
  bytes shared secret**.
