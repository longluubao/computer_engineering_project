# AUTOSAR COM Stack Research Notes

## 1) What "COM Stack" Usually Means

In AUTOSAR Classic discussions, "COM stack" usually covers communication modules from application-facing signal handling down to bus/network transmission:

- COM (signal and I-PDU handling)
- PduR (routing hub)
- Bus interface and transport modules (for example CanIf, CanTp, SoAd)
- Lower driver/network stack beneath those modules

The exact chain depends on bus and payload size.

## 2) Layered View (Practical)

Top to bottom for a common ECU data path:

1. SWC logic (through RTE)
2. COM (signals <-> I-PDU payload)
3. PduR (module-to-module routing)
4. Interface/Transport module
5. Driver/network stack and physical medium

The major architecture decision is often IF path vs TP path.

## 3) Tx Data Flow Patterns

### A) IF path (short payload)

`SWC -> RTE -> COM -> PduR -> CanIf -> CAN Driver`

Typical characteristics:

- Lower latency and less state overhead.
- Limited by frame payload constraints.
- Best for short cyclic/status signals.

### B) TP path (long payload)

`SWC -> RTE -> COM -> PduR -> CanTp (or SoAd for Ethernet-based TP) -> network stack`

Typical characteristics:

- Handles larger payload with segmentation.
- Adds state machine complexity and timing dependencies.
- Needed for diagnostic or large secured payloads.

## 4) Rx Data Flow Patterns

Incoming path is the mirror of Tx:

`Driver/network -> Interface/TP -> PduR -> COM -> RTE -> SWC`

Important debugging point: verify callback chains and timing at each boundary, not only payload values.

## 5) Module Responsibilities

### COM

- Packs/unpacks signals into/from I-PDU payload.
- Applies signal-level handling rules and update semantics.
- Interacts with upper layer via RTE-facing APIs.

### PduR

- Central route dispatcher between modules.
- Maintains IF and TP route logic.
- Handles multi-destination or gateway-like routing patterns where configured.

### CanIf / SoAd

- Provides adaptation between AUTOSAR modules and bus/network-specific lower layers.
- Converts module-level send/receive requests to driver/socket operations.

### CanTp (or another TP module)

- Segments and reassembles payloads.
- Manages flow-control related transport behavior.
- Handles transport-level state and timeout behavior.

## 6) High-Impact Configuration Hotspots

Most integration issues cluster in these areas:

- PDU IDs and mapping consistency across COM, PduR, and lower modules.
- PDU length assumptions and buffer sizes across all layers.
- Correct IF/TP selection per message profile.
- Route activation direction (Tx vs Rx) and destination validity.
- Timing and timeout values that interact with TP state.

## 7) Security-Aware COM Stack Design

When authenticated/secured payloads are added in communication chains:

- Authenticator and freshness bytes increase effective payload size.
- IF routes may become invalid due to length growth; TP may be required.
- Routing and buffering must account for expanded secured payload.
- Verification failure behavior should be visible in diagnostics and trace points.

## 8) Typical Failure Modes and Symptoms

### Route mismatch

Symptom: send API returns error or lower module never sees request.

Likely root cause: wrong PDU ID mapping or missing/disabled PduR route.

### IF/TP mismatch

Symptom: truncation, rejection, or intermittent failures for larger messages.

Likely root cause: payload exceeds IF limits but route remains configured as IF.

### Length inconsistency

Symptom: corrupted payload interpretation or rejected reception.

Likely root cause: COM/PduR/lower stack disagree on expected lengths.

### Callback chain break

Symptom: transmitted data appears on bus but upper confirmations/indications do not occur.

Likely root cause: missing callback binding or wrong module integration direction.

## 9) Bring-Up and Debug Strategy

1. Start with one minimal Tx and one minimal Rx path.
2. Confirm PDU ID mapping end-to-end before signal-level tuning.
3. Validate IF path first for short payloads.
4. Introduce TP path and verify segmentation behavior under load.
5. Add security/diagnostic features after base path is stable.
6. Stress with burst traffic and timing disturbance to validate robustness.

## 10) Practical Review Questions

- Is each message correctly classified as IF or TP?
- Is the PduR route explicit and unique for the intended direction?
- Are all length/buffer assumptions documented and validated?
- Are confirmations/indications monitored for every critical PDU?
- Does the system degrade safely when TP or verification fails?

## 11) Deep Dive: CAN Stack

### Canonical AUTOSAR CAN communication chain

- IF data path (single-frame style):
  - `COM -> PduR -> CanIf -> Can Driver`
- TP data path (multi-frame style):
  - `COM -> PduR -> CanTp -> CanIf -> Can Driver`

### Where complexity usually appears

- CanTp segmentation/reassembly and flow control for larger payloads.
- Buffer coordination between COM/PduR/CanTp during burst traffic.
- Callback ordering (`StartOfReception`, `CopyRxData`, `RxIndication`, confirmations).

### Design and debug guidance

- Keep short periodic signals on IF when possible for lower overhead.
- Move large payloads (diagnostics, secured payload growth) to TP routes early.
- Validate PDU size and timeout assumptions under worst-case bus load.

## 12) Deep Dive: Ethernet Stack

### Canonical AUTOSAR Ethernet chain

`COM -> PduR -> SoAd -> TcpIp -> EthIf -> Eth Driver`

Related state management typically includes `EthSM` (and network management modules depending on design).

### Key architecture points

- `SoAd` bridges static AUTOSAR PDU communication and dynamic socket communication.
- `TcpIp` provides network/transport protocol handling.
- `EthIf` abstracts Ethernet hardware and unifies upper module access.

### Service-oriented implications

- Ethernet-heavy systems often combine SOME/IP style service communication with AUTOSAR routing behavior.
- Design must account for socket lifecycle, dynamic connection states, and routing-group control.

## 13) Deep Dive: SPI in AUTOSAR Context

### Important boundary clarification

SPI is usually not part of the COM/PduR network communication route. In AUTOSAR Classic, SPI is an MCAL driver used for peripheral communication (sensors, external ICs, memories, watchdogs).

### SPI architecture model

- Hierarchy:
  - Channel -> Job -> Sequence
- Driver implementation levels:
  - Level 0: synchronous
  - Level 1: asynchronous
  - Level 2: combined sync/async

### Why SPI still matters in COM-stack discussions

- Application data sent through COM may originate from SPI peripherals.
- End-to-end timing budgets must include SPI acquisition latency before COM transmit.
- Integration issues can appear at the boundary between peripheral data producer and COM signal update timing.

## 14) External Research Links Used

- AUTOSAR CAN Transport Layer (official spec landing):
  - [AUTOSAR SWS CANTransportLayer](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SWS_CANTransportLayer.pdf)
- AUTOSAR Ethernet requirements/spec references:
  - [AUTOSAR SRS Ethernet](https://www.autosar.org/fileadmin/standards/R21-11/CP/AUTOSAR_SRS_Ethernet.pdf)
- SoAd overview article:
  - [SoAd - Socket Adaptor Overview](https://www.autosartoday.com/posts/soad_-_socket_adaptor_overview)
- SPI AUTOSAR implementation overview:
  - [SPI Overview - AutosarToday](https://www.autosartoday.com/posts/spi_overview)
- Vendor AUTOSAR SPI design/API references:
  - [TI MCUSW SPI Design](https://software-dl.ti.com/jacinto7/esd/processor-sdk-rtos-j7200/07_03_00_07/exports/docs/mcusw/mcal_drv/docs/drv_docs/design_spi_top.html)
  - [TI MCUSW SPI Handler and Driver API](https://software-dl.ti.com/jacinto7/esd/processor-sdk-rtos-jacinto7/07_03_00_07/exports/docs/mcusw/mcal_drv/docs/drv_docs/group__MCAL__SPIHANDLER__API.html)
