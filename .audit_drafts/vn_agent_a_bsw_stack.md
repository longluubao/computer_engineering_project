# Phản biện đồ án — AUTOSAR Classic BSW Stack (Examiner A)

**Đề tài:** "AUTOSAR SecOC with Post-Quantum Cryptography on a Raspberry Pi 4 Ethernet Gateway"
**Ngày:** 2026-05-02. Tổng số câu: 20.

---

### Câu A.1: Vi phạm kiến trúc phân lớp — khái niệm "Extension Layer"

**Câu hỏi của giáo sư:** "Em vẽ kiến trúc 5 lớp gồm Services / ECU Abstraction / MCAL / Microcontroller cộng thêm một 'Extension Layer' chứa ApBridge, GUIInterface, Scheduler. Em hãy chỉ ra trong tài liệu AUTOSAR_EXP_LayeredSoftwareArchitecture R21-11 xem AUTOSAR có công nhận lớp thứ năm nào tên là 'Extension' hay không? Nếu không, vì sao em đưa nó vào kiến trúc tổng thể như một lớp ngang hàng với MCAL và Services?"

**Tại sao câu hỏi này quan trọng:** Đây là câu hỏi gốc rễ về tính tuân thủ chuẩn. AUTOSAR Classic chỉ định nghĩa bốn lớp BSW. Việc thêm một lớp ngoài chuẩn cho thấy thí sinh đang trộn lẫn nhu cầu kỹ thuật của Pi 4 Linux với chuẩn AUTOSAR thuần.

**Câu trả lời gợi ý cho thí sinh:**
- Em xin thừa nhận rằng AUTOSAR Classic Platform R21-11 chỉ chuẩn hoá bốn lớp BSW: Services Layer, ECU Abstraction Layer, MCAL và Microcontroller. Không có lớp gọi là "Extension".
- Trong code (`source/`), những module phi-AUTOSAR là `ApBridge/`, `GUIInterface/`, `Scheduler/`. `source/ApBridge/ApBridge.c:25-32` cho thấy ApBridge tự định nghĩa state machine `SoAd_ApBridgeStateType` — đây là logic ứng dụng (Adaptive↔Classic gateway) chứ không phải BSW chuẩn.
- Phân loại đúng:
  - **Scheduler** nên thuộc OS module hoặc Complex Driver.
  - **GUIInterface** thuần ngoài AUTOSAR — chỉ phục vụ demo.
  - **ApBridge** thực chất là một **Complex Device Driver (CDD)** theo SWS_BSWGeneral.
- Hướng khắc phục: vẽ lại biểu đồ với chú thích "Complex Device Drivers (CDD) — non-standard project-specific" thay vì "Extension Layer".

**Citations:**
- [Layered Software Architecture R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_EXP_LayeredSoftwareArchitecture.pdf) — Section 3 (retrieved 2026-05-02)
- [General Specification of Basic Software Modules R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_BSWGeneral.pdf) (retrieved 2026-05-02)

---

### Câu A.2: Bảy module chỉ là Stub — thesis có thật sự "đầy đủ BSW stack"?

**Câu hỏi của giáo sư:** "Em tuyên bố thesis có 'đầy đủ BSW stack' nhưng các module Det, Dem, Dcm, NvM, MemIf, Ea, Fee, CanSM, CanNm chỉ là Stub. Em chỉ rõ ranh giới giữa 'Stub' và 'tuân thủ AUTOSAR' ở đâu? Nếu ngày mai phải chứng nhận ASPICE Level 2, sẽ rớt ở module nào?"

**Câu trả lời gợi ý cho thí sinh:**
- "Stub" trong context của em: module có header chuẩn AUTOSAR, có log, nhưng không thực hiện đầy đủ behavior trong SWS. "Min." là functional cho đường dữ liệu chính nhưng thiếu API tuỳ chọn.
- Cụ thể:
  - **Det** (`source/Det/Det.c:52-90`): Min. — circular log buffer 32 entries, có `Det_ReportError`/`Det_ReportRuntimeError`. Thiếu callback notification và filter API.
  - **Dem** (`source/Dem/Dem.c:1-60`): Min. — DTC storage, NvM persistence pending. Thiếu Operation Cycle, Aging, OBD-II.
  - **Dcm**: chỉ forward `Dcm_TpRxIndication`/`Dcm_TpTxConfirmation` — chưa có UDS service handler.
  - **NvM/MemIf/Ea/Fee**: wrapper, chưa có block redundancy/CRC.
  - **CanSM/CanNm**: stub — không có state machine NetworkStartUp/BusOff Recovery.
- Nếu chứng nhận ASPICE Level 2, sẽ rớt ở Dcm, NvM, CanSM.
- Em xin sửa wording từ "complete BSW stack" thành **"complete authentication path with stub interfaces for non-critical BSW modules"**.

**Citations:**
- [SWS Default Error Tracer R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_DefaultErrorTracer.pdf)
- [BSW Module List R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_TR_BSWModuleList.pdf)

---

### Câu A.3: Thiếu RTE — vi phạm Methodology?

**Câu hỏi của giáo sư:** "AUTOSAR Classic định nghĩa RTE là tầng bắt buộc. Em không có module RTE trong source/. `source/SecOC/SecOC.c:13` em include 'Rte_SecOC.h' nhưng không có `source/Rte/`. Ai đảm nhận role của RTE? Đó có phải vi phạm AUTOSAR Methodology không?"

**Câu trả lời gợi ý cho thí sinh:**
- Em xin thừa nhận: trong scope thesis, em không hiện thực RTE đầy đủ. File `include/SecOC/Rte_SecOC.h` là header rỗng để giữ tương thích, không phải auto-generated từ ARXML.
- Lý do: RTE Generator yêu cầu tooling commercial (Vector DaVinci, EB tresos, ETAS ISOLAR-AB).
- "Vai trò RTE" được lap-ghép bằng:
  - Application code (GUI/test driver) gọi trực tiếp vào `Com_*`.
  - Com gọi vào `PduR_ComTransmit`, đóng vai cầu nối RTE-BSW.
- Đây là deviation so với AUTOSAR Methodology vì:
  - SWC không tồn tại như entity riêng (không có `*.arxml` với `RPort/PPort`).
  - Không có port-mapping qua `Rte_Write_*` / `Rte_Read_*`.
- Hướng khắc phục: ghi rõ "RTE Bypass mode" như design decision; Future Work tích hợp auto-generated RTE.

**Citations:**
- [Methodology for Classic Platform R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_TR_Methodology.pdf)
- [SRS RTE R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SRS_RTE.pdf)
- [SWS RTE R22-11](https://www.autosar.org/fileadmin/standards/R22-11/CP/AUTOSAR_SWS_RTE.pdf)

---

### Câu A.4: Mối quan hệ giữa các module — vẽ Tx/Rx Flow

**Câu hỏi của giáo sư:** "Em hãy vẽ chính xác đường đi Tx và Rx của một I-PDU bảo vệ bằng SecOC. Tại sao SecOC phải nằm 'kẹp' giữa hai lần routing của PduR?"

**Câu trả lời gợi ý cho thí sinh:**
- **Tx flow:** App → Com → `PduR_ComTransmit` → `SecOC_IfTransmit` → (SecOC `authenticate_PQC`) → `PduR_SecOCTransmit` → SoAd/CanIf/CanTp → Driver.
  - `source/Com/Com.c:13` Com gọi `PduR_ComTransmit` (Authentic I-PDU).
  - PduR routing lần 1: `PduR_ComTransmit` → SecOC.
  - SecOC tạo Secured I-PDU qua `authenticate()` (`SecOC.c:371-494`) hoặc `authenticate_PQC()`.
  - PduR routing lần 2: `PduR_SecOCTransmit` (`source/PduR/Pdur_SecOC.c:40-67`) định tuyến Secured I-PDU dựa trên `PdusCollections[TxPduId].Type`.
- **Rx flow:** Driver → CanIf/SoAd Rx → `PduR_SoAdIfRxIndication` → `SecOC_RxIndication`/`SecOC_StartOfReception` → SecOC verify → `PduR_SecOCIfRxIndication` → `Com_RxIndication` → app.
- **Tại sao SecOC kẹp giữa hai lần PduR?** SWS_SecOC R21-11 §7.1: SecOC nằm ở Service Layer, không trực tiếp connect physical bus. Nó nhận Authentic I-PDU và sản xuất Secured I-PDU. PduR là module routing duy nhất được phép connect các module thuộc các sub-stack khác nhau.

**Citations:**
- [SWS SecOC R21-11 §7](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_SecureOnboardCommunication.pdf)
- [SWS PDU Router R20-11 §7.1](https://www.autosar.org/fileadmin/standards/R20-11/CP/AUTOSAR_SWS_PDURouter.pdf)

---

### Câu A.5: Tách lớp Csm/CryIf/Crypto Driver — tại sao 3 lớp?

**Câu hỏi của giáo sư:** "AUTOSAR yêu cầu Csm là Service Provider, CryIf là router, Crypto Driver bind cụ thể tới HW. Em chứng minh PQC.c là Crypto Driver hợp lệ và CryIf đang routing chứ không bypass."

**Câu trả lời gợi ý cho thí sinh:**
- 3 lớp:
  - **Csm** (Service): API generic `Csm_MacGenerate`, `Csm_SignatureVerify`.
  - **CryIf** (Interface): chọn provider, dispatch tới Crypto Driver.
  - **Crypto Driver**: SW/HW-specific, implement primitive thực sự.
- Trong code:
  - `Csm_SignatureGenerate` (`Csm.c:806-879`) gọi `Csm_SelectProvider` rồi `CryIf_SignatureGenerate`.
  - `CryIf_SignatureGenerate` (`CryIf.c:107-130`) check provider type rồi delegate `PQC_MLDSA_Sign`.
  - PQC module bind tới liboqs — tương đương "SW Crypto Driver".
- **Điểm yếu:** CryIf của em không có Provider-Routing-Table generic. Hard-code provider check (`if (provider != CRYIF_PROVIDER_PQC)` ở `CryIf.c:115`). Đó là deviation so với SWS_CryptoInterface.
- Hướng khắc phục: thay điều kiện hard-code bằng bảng `CryIf_ProviderConfig[]` truyền vào tại runtime.

**Citations:**
- [SWS Crypto Service Manager R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_CryptoServiceManager.pdf)
- [SWS Crypto Driver R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_CryptoDriver.pdf)

---

### Câu A.6: AUTOSAR Methodology / ARXML — không có ARXML?

**Câu hỏi của giáo sư:** "AUTOSAR Methodology yêu cầu mọi cấu hình BSW phải mô tả qua ARXML và được generate bằng tool. Em có ARXML nào không?"

**Câu trả lời gợi ý cho thí sinh:**
- Em xin thẳng thắn: project **không có file ARXML nào**.
- Theo `AUTOSAR_TR_Methodology R21-11`: ECU Extract (*.arxml) → BSW Configuration (DaVinci/Tresos) → Generate `*_Cfg.h`, `*_PBcfg.c`.
- Project của em, ba loại file Cfg đều **viết tay**:
  - `include/SecOC/SecOC_Cfg.h` (482 lines): pre-compile time defines.
  - `source/SecOC/SecOC_Lcfg.c`: link-time configs — `PdusCollections[]` (`SecOC_Lcfg.c:72-94`).
  - `source/SecOC/SecOC_PBcfg.c`: post-build configs.
- Lý do: tooling AUTOSAR commercial license đắt; muốn debug được trực tiếp; project là academic.
- Hệ quả deviation: không có traceability từ requirement → ARXML → code; đổi config phải sửa cả 3 file → dễ inconsistency.
- Future Work: dùng EB tresos Studio Free, hoặc tự viết ARXML schema-validating script.

**Citations:**
- [Methodology for Classic Platform R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_TR_Methodology.pdf)
- [TPS ECU Configuration R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_TPS_ECUConfiguration.pdf)

---

### Câu A.7: BswM Min. — Mode arbitration thiếu

**Câu hỏi của giáo sư:** "Em ghi BswM là Min. Mode requests từ ComM, EcuM, Dcm... được điều phối thế nào?"

**Câu trả lời gợi ý cho thí sinh:**
- "Min." cho BswM nghĩa là: có rule engine cơ bản nhưng không full configurable qua ARXML rule list.
- `source/BswM/BswM.c:93-100` có 5 rules cứng: `EcuMStartup`, `EcuMShutdownPath`, `ComMFullReq`, `ComMNoReq`, `GatewayPathDown`.
- Mode requests vào qua `BswM_RequestMode(RequesterId, Mode)` → lưu trong `BswM_ModeRequests[BSWM_MAX_MODE_REQUESTS]` (`BswM.c:59`). `BswM_MainFunction` evaluate cả 5 rules.
- Điểm thiếu so với SWS:
  - Rules không loaded từ config.
  - Không có Logical Expression composer (AND/OR rules được hard-code).
  - Không support mode arbitration cho hơn ~10 sources.
- Hướng khắc phục: refactor thành `BswM_RuleEvaluator` với `BswM_RuleConfigType*` từ post-build file.

**Citations:**
- [SWS BSW Mode Manager R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_BSWModeManager.pdf)

---

### Câu A.8: EcuM Min. — State machine STARTUP/RUN/POST_RUN/SHUTDOWN

**Câu hỏi của giáo sư:** "EcuM phải có state machine OFF → STARTUP → STARTUP_TWO → UP/RUN → POST_RUN → SHUTDOWN → SLEEP. Em hiện thực hết chưa?"

**Câu trả lời gợi ý cho thí sinh:**
- `source/EcuM/EcuM.c:41-54` em khai báo: `EcuM_State`, `EcuM_ShutdownTarget`, `EcuM_BootTarget`, `EcuM_SleepMode`, các wakeup flags.
- States có: UNINIT, INIT, RUN, POST_RUN, PREP_SHUTDOWN, SHUTDOWN, SLEEP — **không có STARTUP_TWO riêng** (gộp vào STARTUP).
- Có:
  - Wakeup validation, Run/Post-run request counters.
  - Reset/Off callouts (`EcuM.c:57-61`).
  - NvM persistent state (`EcuM_LoadPersistentStateFromNvM`).
- Điểm thiếu:
  - **Không phân biệt STARTUP_ONE vs STARTUP_TWO**. SWS yêu cầu STARTUP_ONE chạy trước OS init, STARTUP_TWO sau OS init.
  - SLEEP mode chỉ là placeholder; Pi 4 Linux không có MCU sleep registers.
  - Multi-core EcuM Master/Slave không support.
- Future Work: thêm STARTUP_TWO cho post-OS init, wakeup source mapping qua ARXML.

**Citations:**
- [SWS ECU State Manager R20-11 §7.3](https://www.autosar.org/fileadmin/standards/R20-11/CP/AUTOSAR_SWS_ECUStateManager.pdf)

---

### Câu A.9: Os Min. — không phải OSEK

**Câu hỏi của giáo sư:** "Code em (`source/Os/Os.c:1-7`) ghi 'Cooperative non-preemptive OS, no pthread/ucontext'. Task priority, preemption, resource locking hoạt động thế nào? Tại sao không dùng pthread khi target là Linux?"

**Câu trả lời gợi ý cho thí sinh:**
- `source/Os/Os.c` là **cooperative non-preemptive scheduler** (line 4), gần với BCC1 nhưng KHÔNG đầy đủ vì OSEK BCC1 là preemptive.
- Lý do thiết kế:
  - Pi 4 Linux user-space đã có preemptive scheduler (CFS/SCHED_FIFO). Hai scheduler chồng nhau dễ deadlock.
  - Chọn cooperative để portable Windows/Linux/Pi 4 cùng codebase.
- Trong code:
  - `Os_TaskControlType` (`Os.c:26-36`) có `Priority`, `EventsSet`, `EventsWaiting`, `ResourceCount` — giống OSEK.
  - `Os_AlarmControlType` (`Os.c:38-45`) — alarm với cycle time.
  - `Os_ResourceControlType` (`Os.c:72-78`) — Priority Ceiling Protocol.
- Tuy có data structures OSEK, scheduler thì cooperative: tasks phải tự `TerminateTask()` hoặc `WaitEvent()`.
- Hệ quả: không thể đo WCET đúng nghĩa AUTOSAR; resource locking dùng ceiling priority "logical" chứ không thực sự suspend interrupt.
- Future Work: chuyển sang FreeRTOS hoặc ARINC-653, refactor `Os_Schedule()` thành interrupt-driven preemptive.

**Citations:**
- [SWS Operating System R21-11 §8.4](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_OS.pdf)

---

### Câu A.10: PduR routing tables — chỗ nào trong code?

**Câu hỏi của giáo sư:** "Em chỉ chính xác trong source code chỗ định nghĩa hai bảng routing (Authentic và Secured). Bảng có configurable runtime hay hard-coded?"

**Câu trả lời gợi ý cho thí sinh:**
- Em xin trả lời thẳng: **không có hai bảng riêng biệt** đúng theo SWS_PDURouter. Chỉ có **một bảng** `PdusCollections[]` ở `source/SecOC/SecOC_Lcfg.c:72-94`:
  ```c
  SecOC_PduCollection PdusCollections[] = {
      {SECOC_SECURED_PDU_CANIF,0,0,0,0},
      {SECOC_SECURED_PDU_CANTP,0,0,0,0},
      {SECOC_SECURED_PDU_SOADTP,0,0,0,0},
      ...
  };
  ```
- "Routing table 1" (Authentic → SecOC): không có bảng config, PduR_ComTransmit gọi thẳng SecOC. Đây là **hard-coded routing** dạng switch-case trong `Pdur_SecOC.c:40-67`.
- "Routing table 2" (Secured → If/Tp): chính là `PdusCollections[]`. `PduR_SecOCTransmit` (`Pdur_SecOC.c:51`) switch trên `Type` để chọn `CanIf_Transmit/CanTp_Transmit/SoAd_TpTransmit/SoAd_IfTransmit`.
- Đây là deviation: SWS_PDURouter yêu cầu một `PduR_RoutingPathType` array với source/destination, gating, queueing config.
- Future Work: refactor thành `PduR_RoutingTable_t PduR_RoutingPaths[]` chứa src/dst PduId, callback function pointer, queue config.

**Citations:**
- [SWS PDU Router R20-11 §7.2](https://www.autosar.org/fileadmin/standards/R20-11/CP/AUTOSAR_SWS_PDURouter.pdf)

---

### Câu A.11: ApBridge "Extension" — Tier-1 sẽ làm gì?

**Câu hỏi của giáo sư:** "Module `source/ApBridge/ApBridge.c` không có trong AUTOSAR Classic standard. Nếu submit cho Bosch hoặc Continental, sẽ bị xử lý ra sao?"

**Câu trả lời gợi ý cho thí sinh:**
- ApBridge (`ApBridge.c:1-203`) quản lý heartbeat và bridging với Adaptive Platform, với states `SOAD_AP_BRIDGE_NOT_READY/READY/DEGRADED`.
- Đây là **Complex Device Driver (CDD)** theo phân loại AUTOSAR Classic. CDD được chấp nhận nhưng phải khai báo trong ECU Configuration.
- Code đã có pattern AUTOSAR đúng: `ApBridge_Init`, `ApBridge_DeInit`, `ApBridge_MainFunction`, report functions.
- Nếu submit cho Tier-1, ApBridge sẽ bị split:
  - Phần Heartbeat tracking (`ApBridge_ReportHeartbeat`, line 120) — CDD độc lập.
  - Phần State arbitration với SoAd: merge vào BswM rule.
  - Phần Forced state override: route qua Diagnostic Service (Dcm).
- ApBridge gánh 3 vai trò không đồng nhất — vi phạm Single-Responsibility ở mức module.

**Citations:**
- [SWS BSW General R21-11 — Complex Driver](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_BSWGeneral.pdf)
- [Layered Software Architecture R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_EXP_LayeredSoftwareArchitecture.pdf)

---

### Câu A.12: MCAL chỉ có Pi4 — SocketCAN không phải MCAL chuẩn

**Câu hỏi của giáo sư:** "MCAL theo AUTOSAR phải truy cập trực tiếp register. `Can_Pi4.c:39-43` dùng SocketCAN (Linux kernel API). Em chứng minh đây vẫn là MCAL hợp lệ?"

**Câu trả lời gợi ý cho thí sinh:**
- Em xin thẳng thắn: theo nghĩa nghiêm túc, MCAL phải truy cập trực tiếp register thông qua memory-mapped I/O.
- `source/Mcal/Pi4/Can_Pi4.c`:
  - Line 39-43 include `<sys/socket.h>`, `<linux/can.h>`, `<linux/can/raw.h>` — Linux kernel API ở user-space.
  - Header comment (line 12-13): "Pi 4 không có CAN controller built-in, dùng MCP2515 SPI hoặc USB-CAN hoặc vcan".
- "MCAL" của em thực chất là **wrapper trên SocketCAN**. SocketCAN chính nó là kernel-level driver — phần em viết là application-level wrapper map AUTOSAR Can API → SocketCAN syscalls.
- Có thể chấp nhận như "Virtual MCAL" hoặc "Simulation MCAL" cho academic vì:
  - Pi 4 SoC (BCM2711) không có CAN controller built-in → không có register thật.
  - AUTOSAR cho phép Crypto SW Driver chạy trên CPU mà không có HSM thật — tương tự, "Software CAN Driver" trên Linux có precedent.
- Em xin gọi đúng tên: **"Linux/SocketCAN-backed CAN Driver — functional substitute for AUTOSAR Can MCAL on Pi 4 platform"**.
- Hướng khắc phục: viết một SPI-MCAL (`Spi_Pi4.c` truy cập `/dev/spidev0.0` và bit-bang MCP2515 registers theo datasheet).

**Citations:**
- [SWS CAN Driver R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_CANDriver.pdf)
- [Layered Software Architecture R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_EXP_LayeredSoftwareArchitecture.pdf)

---

### Câu A.13: COM/CanIf/EthIf — vai trò chính xác

**Câu hỏi của giáo sư:** "Em phân biệt COM, CanIf, EthIf như thế nào? Tại sao COM Min.?"

**Câu trả lời gợi ý cho thí sinh:**
- **COM** (`source/Com/Com.c`): Service Layer. Quản lý signal-to-IPDU packing/unpacking, deadline monitoring, signal gateway.
- **CanIf** (`source/Can/CanIF.c`): ECU Abstraction Layer. Map giữa logical PDU và physical CAN frame (CAN ID, DLC).
- **EthIf** (`source/EthIf/`): ECU Abstraction Layer. Map giữa logical PDU và Ethernet frame (MAC, VLAN, EthType).
- COM Min. có:
  - Signal table (`Com_SignalRuntime[COM_NUM_OF_SIGNALS]` ở `Com.c:78`) với Updated/Invalid flags.
  - Tx/Rx IPDU deadline counters.
  - IPDU group.
- Thiếu:
  - Signal **byte-order** conversion — assume host endian.
  - Signal **data filtering** (`ComFilter` types).
  - Signal **packing across PDU boundaries**.
  - **Notification callouts** (`Com_TimeoutNotification`).
  - **Group signal**.
- Trong context Pi 4 gateway PQC, em chỉ cần signal pass-through nguyên byte.

**Citations:**
- [SWS COM R21-11 §7](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_COM.pdf)
- [SWS CAN Interface R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_CANInterface.pdf)

---

### Câu A.14: Std_Types.h — chứng minh tuân thủ

**Câu hỏi của giáo sư:** "Tất cả module AUTOSAR phải include Std_Types.h. Em định nghĩa thêm value `E_BUSY = 0x02`, đó có phải extension hợp lệ không?"

**Câu trả lời gợi ý cho thí sinh:**
- `include/Std_Types.h:55-59`:
  ```c
  typedef uint8 Std_ReturnType;
  #define E_OK            ((Std_ReturnType)0x00)
  #define E_NOT_OK        ((Std_ReturnType)0x01)
  #define E_BUSY          ((Std_ReturnType)0x02)
  #define QUEUE_FULL      ((Std_ReturnType)0x03)
  ```
- `E_OK = 0x00` và `E_NOT_OK = 0x01` đúng theo AUTOSAR_SWS_StandardTypes.
- `boolean = unsigned char`, `uint8/uint16/uint32` — đúng (line 33-39).
- `Std_VersionInfoType` struct với `vendorID`, `moduleID`, `sw_major/minor/patch_version` — đúng (line 62-69).
- `STD_ON = 0x01u`, `STD_OFF = 0x00u` — đúng.
- **Về `E_BUSY` và `QUEUE_FULL`:** không phải standard values. AUTOSAR chỉ chuẩn hoá E_OK và E_NOT_OK. Các value khác phải module-specific (vd `CSM_E_BUSY`). Em định nghĩa `E_BUSY` chung trong Std_Types là **deviation**.
- Hướng khắc phục: di chuyển `E_BUSY` xuống module riêng dưới prefix, hoặc dùng `CRYPTO_E_BUSY` đã có.

**Citations:**
- [SWS Standard Types R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_StandardTypes.pdf)
- [SWS BSW General R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_BSWGeneral.pdf)

---

### Câu A.15: Encrypt module — vì sao ở ECU Abstraction và Min.?

**Câu hỏi của giáo sư:** "`source/Encrypt/encrypt.c` chứa AES S-box. Em xếp ở ECU Abstraction Layer và Min. Vì sao? AES thuộc về Crypto Driver chứ?"

**Câu trả lời gợi ý cho thí sinh:**
- Em xin thừa nhận: việc xếp Encrypt ở "ECU Abstraction Layer" là **không chuẩn**.
- Theo SWS_CryptoDriver R21-11: AES, SHA, ECC primitives đều thuộc **Crypto Driver** layer.
- File `encrypt.c:1-100`:
  - S-box, mul2, mul3, rcon (Rijndael tables).
  - `startEncryption()` (`encrypt.c:23`) là entry point để compute MAC.
  - Được CryIf gọi qua `extern void startEncryption(...)` ở `CryIf.c:51`.
- Đúng layer phải là: Encrypt module nằm dưới CryIf, ngang hàng với PQC.c. Cả hai là Crypto Driver (SW). CryIf route giữa hai providers (CLASSIC=AES; PQC=ML-DSA).
- Đường gọi đã đúng: chỉ là vấn đề **labelling layer** sai trên kiến trúc.
- "Min." nghĩa là chỉ implement AES-128 ECB MAC mode, không có CMAC, GCM, AES-256.
- Hướng khắc phục: rename `Encrypt/` thành `Crypto_Aes_Sw/`, đặt cùng tier với `PQC/`.

**Citations:**
- [SWS Crypto Driver R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_CryptoDriver.pdf)
- [Utilization of Crypto Services R20-11](https://www.autosar.org/fileadmin/standards/R20-11/CP/AUTOSAR_EXP_UtilizationOfCryptoServices.pdf)

---

### Câu A.16: PQC module — vị trí trong layer nào?

**Câu hỏi của giáo sư:** "Báo cáo ghi PQC module ở 'Services* (project-specific)'. Bản chất nó là Service Layer module hay Crypto Driver?"

**Câu trả lời gợi ý cho thí sinh:**
- Em xin sửa lại phân loại: **PQC module là Crypto Driver (SW)**, không phải Service Layer module.
- Bằng chứng:
  - `source/PQC/PQC.c` chứa wrapper cho liboqs (ML-KEM-768, ML-DSA-65) — cryptographic primitives.
  - Được gọi bởi CryIf (`source/CryIf/CryIf.c:47-50` extern declarations).
  - CryIf gọi PQC khi `provider == CRYIF_PROVIDER_PQC`.
- Chuỗi đúng: SecOC (Service) → Csm (Service) → CryIf (Service-router) → PQC (Crypto Driver SW).
- "Project-specific" vì:
  - liboqs không phải standard AUTOSAR library.
  - Algorithm IDs (`CRYPTO_ALGOFAM_ML_KEM`, `CRYPTO_ALGOFAM_ML_DSA`) chưa có trong AUTOSAR `Crypto_AlgorithmFamilyType` enum R21-11.
  - Key sizes lớn (ML-DSA-65 sig 3309 B, public key 1952 B).
- Đây không phải hack vì AUTOSAR cho phép **vendor-specific algorithm extensions** (SWS Crypto Driver §7.1). PQC mode là chiến lược kế thừa hậu lượng tử mà NIST đã chuẩn hoá.

**Citations:**
- [SWS Crypto Driver R21-11 §7.1](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_CryptoDriver.pdf)
- [SWS Crypto Service Manager R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_CryptoServiceManager.pdf)
- [NIST FIPS 204 ML-DSA](https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.204.pdf)
- [NIST FIPS 203 ML-KEM](https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.203.pdf)

---

### Câu A.17: Det/Dem coupling — persistence qua NvM?

**Câu hỏi của giáo sư:** "Em có chuỗi Det → Dem → NvM để persist DTC qua reset không?"

**Câu trả lời gợi ý cho thí sinh:**
- Có chuỗi nhưng còn thiếu hoàn thiện:
  - `Det_ReportError` (`Det.c:72-90`) log vào circular buffer, gọi `Dem_ReportDetError` (line 80).
  - `Dem.c:1-60` có `Dem_DtcStorage[]`, `Dem_NvMWritePending`, `Dem_NvMJobPending` flags.
  - `Dem.c:11-13` extern `NvM_GetErrorStatus`, `NvM_WriteBlock`.
- Flow:
  1. Module gọi `Det_ReportError(ModuleId, InstanceId, ApiId, ErrorId)`.
  2. Det log vào circular buffer + forward sang `Dem_ReportDetError`.
  3. Dem find/allocate DTC slot → set `Dem_NvMWritePending = TRUE`.
  4. `Dem_MainFunction` flush DTCs qua `NvM_WriteBlock`.
- Điểm thiếu:
  - NvM thực sự (block redundancy, CRC, write-buffer) chưa hiện thực đầy đủ.
  - Aging counter, Operation Cycle reset chưa hỗ trợ.
  - OBD-II (J1979) DTC mapping không có.
- Rủi ro: Pi 4 reset đột ngột giữa lúc `Dem_MainFunction` đang flush, có thể mất DTC.
- Hướng khắc phục: thêm `NvM` write-after-each-set strategy.

**Citations:**
- [SWS Default Error Tracer R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_DefaultErrorTracer.pdf)
- [SWS NVRAM Manager R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_NVRAMManager.pdf)

---

### Câu A.18: SecOC bypass và Forced-Pass-Override

**Câu hỏi của giáo sư:** "Theo SWS_SecOC_00253 và config `SECOC_ENABLE_FORCED_PASS_OVERRIDE` ở `SecOC_Cfg.h:94`, SecOC có thể bypass verification. Em set FALSE. Có path nào trong code bypass không?"

**Câu trả lời gợi ý cho thí sinh:**
- `include/SecOC/SecOC_Cfg.h:94`: `#define SECOC_ENABLE_FORCED_PASS_OVERRIDE ((boolean)FALSE)` — an toàn.
- Khi FALSE, SecOC luôn enforce verify.
- `source/SecOC/SecOC.c:189-235` `SecOC_IfTransmit` không có path bypass authenticate. Verification chain (`verify`/`verify_PQC` ở line 107-108) luôn được call trong `SecOC_MainFunctionRx`.
- Hai chỗ phải audit kỹ:
  1. `SECOC_IGNORE_VERIFICATION_RESULT` (`SecOC_Cfg.h:106` set FALSE) — đảm bảo verify fail thì PDU bị drop.
  2. `verify_PQC` đường thoát error: nếu `Csm_SignatureVerify` trả `CRYPTO_E_KEY_NOT_VALID`, SecOC phải mark PDU invalid chứ không silently pass.
- `SECOC_PROPAGATE_ONLY_FINAL_VERIFICATION_STATUS = FALSE` — mỗi lần verify đều report status.
- Em xin thừa nhận một nguy cơ: nếu key chưa load (Pi 4 boot trước khi NvM ready), `Csm_PQC_EnsureReady` (`Csm.c:825`) trả `CRYPTO_E_KEY_NOT_VALID` → SecOC trả E_NOT_OK. Em sẽ thêm check `EcuM_State == ECUM_RUN` trước khi enable SecOC verify.

**Citations:**
- [SWS SecOC R21-11 — SWS_SecOC_00253](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_SecureOnboardCommunication.pdf)

---

### Câu A.19: Buffer sizing cho PQC — SECPDU_MAX_LENGTH đủ chưa?

**Câu hỏi của giáo sư:** "ML-DSA-65 signature là 3309 bytes. `source/SecOC/SecOC_Lcfg.c:22` em khai báo `SecPdu0BufferTx[SECOC_SECPDU_MAX_LENGTH]`. Giá trị bao nhiêu, đủ cho PQC chưa?"

**Câu trả lời gợi ý cho thí sinh:**
- `include/SecOC/SecOC_PQC_Cfg.h:38` em định nghĩa `SECOC_PQC_MAX_PDU_SIZE = 8192U` — đủ cho ML-DSA signature (3309) + payload nhỏ + header + freshness 8 bytes.
- `SECOC_SECPDU_MAX_LENGTH` được set theo PQC mode hay classic mode tuỳ build flag.
- Lcfg buffers (`SecOC_Lcfg.c:22-69`) là **static array** — không resize runtime. Nếu compile với SECOC_SECPDU_MAX_LENGTH = 100 (classic) rồi switch PQC mode runtime, sẽ overflow.
- Hướng khắc phục đã tích hợp:
  - Pre-build flag chặn: nếu `SECOC_USE_PQC_MODE = TRUE` thì `SECOC_SECPDU_MAX_LENGTH` được override.
  - Verify trong `SoAdTp_RxIndication` (`SoAd.c:1205`): `if (PduInfoPtr->SduLength > SECOC_SECPDU_MAX_LENGTH)` thì reject.
- Điểm yếu: nếu future thay ML-DSA-65 sang ML-DSA-87 (sig ~4595 bytes), phải recompile.

**Citations:**
- [NIST FIPS 204 — Table 1](https://nvlpubs.nist.gov/nistpubs/fips/nist.fips.204.pdf)
- [SWS SecOC R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_SecureOnboardCommunication.pdf)

---

### Câu A.20: Tổng kết — có thực sự là AUTOSAR Classic ECU?

**Câu hỏi của giáo sư:** "Sau tất cả những thừa nhận về Stub, không có RTE, không có ARXML, MCAL không phải register-level, OS không preemptive — em vẫn khẳng định project này là 'AUTOSAR Classic ECU' chứ?"

**Câu trả lời gợi ý cho thí sinh:**
- Em xin định danh chính xác: **"AUTOSAR Classic-style BSW reference implementation for SecOC + PQC, hosted on Linux/Pi 4 user-space"**, không phải "AUTOSAR Classic ECU production-grade".
- Các đặc tính tuân thủ AUTOSAR Classic em đã chứng minh:
  - Naming conventions theo BSWGeneral.
  - Layered separation Service/ECU Abstraction/MCAL về thư mục và call direction.
  - Std_Types với `Std_ReturnType`, `E_OK/E_NOT_OK`.
  - SWS_SecOC mapping (comment cụ thể `[SWS_SecOC_00031]`, `[SWS_SecOC_00037]`...).
  - PduR routing pattern (dù simplified).
  - Csm/CryIf/Crypto Driver layer separation đúng.
- Các điểm deviation em sẽ ghi rõ:
  1. Không có RTE.
  2. Không có ARXML.
  3. MCAL là SocketCAN wrapper, không register-level.
  4. OS là cooperative.
  5. 7 modules ở mức Stub/Min.
  6. ApBridge và GUIInterface là project-specific (CDD).
  7. PQC và Encrypt là vendor-specific Crypto Drivers.
- Đóng góp khoa học vẫn nguyên vẹn:
  - Demo end-to-end PQC integration vào SecOC trên Linux/Pi 4.
  - Performance benchmark ML-KEM/ML-DSA so với HMAC.
  - Cấu trúc code follow AUTOSAR pattern — dễ port sang RTOS/MCU thật.
- Future Work: tích hợp tooling, port lên Infineon AURIX TC275, thay Os cooperative bằng FreeRTOS.

**Citations:**
- [Classic Platform Release Overview R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_TR_ClassicPlatformReleaseOverview.pdf)
- [SWS BSW General R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_BSWGeneral.pdf)
- [Layered Software Architecture R21-11](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_EXP_LayeredSoftwareArchitecture.pdf)
