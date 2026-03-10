---
name: autosar-com-stack-expert
description: Explains AUTOSAR Classic COM stack architecture and communication flows across COM, PduR, bus interfaces, transport/network layers, and SPI driver integration boundaries. Use when the user asks about signal-to-PDU mapping, routing paths, CAN/Ethernet/SPI communication design, TP segmentation, interface design, or debugging AUTOSAR communication issues.
---

# AUTOSAR COM Stack Expert

## Quick Start

When handling COM stack questions:

1. Identify the path first:
   - Tx or Rx
   - Interface (IF) or Transport Protocol (TP)
   - Bus type (CAN, Ethernet, FlexRay)
2. Clarify abstraction level:
   - Signal level (COM)
   - PDU level (PduR/routing)
   - Frame/socket transport (CanIf/CanTp/SoAd/TcpIp)
3. Explain both data flow and control flow (callbacks, confirmations, indications).

## Bus-Specific Deep Dive Workflow

When the user asks for deep dives on CAN, SPI, or Ethernet, always structure the answer in this order:

1. Placement:
   - State exactly where the bus sits in AUTOSAR layering.
2. Path:
   - Show canonical Tx/Rx module chain for that bus.
3. Constraints:
   - Frame/MTU size, timing model, buffering, and state-machine implications.
4. Integration:
   - Explain which module owns routing, segmentation, and error/report handling.
5. Verification:
   - List concrete traces/callbacks/counters to inspect.

Use these architecture anchors:

- CAN:
  - `COM -> PduR -> CanIf` for IF traffic.
  - `COM -> PduR -> CanTp -> CanIf` for segmented traffic.
- Ethernet:
  - `COM -> PduR -> SoAd -> TcpIp -> EthIf -> Eth Driver`.
- SPI:
  - Usually not a COM/PduR route.
  - Treated as MCAL peripheral communication via `Spi` driver (channels/jobs/sequences).
  - Explain bridge points if upper BSW modules consume SPI-backed device data.

## Response Pattern

Use this format:

```markdown
## Scope
[Platform, bus, Tx/Rx path, IF/TP]

## Module Flow
[Ordered module chain with key APIs/callbacks]

## Critical Config
- [Main config items that govern behavior]
- [Buffer/length/timing constraints]

## Failure Modes
- [Likely issue 1 and symptom]
- [Likely issue 2 and symptom]

## Validation Steps
- [Trace/log/checkpoint 1]
- [Trace/log/checkpoint 2]
```

## Canonical Module Paths

### Tx (IF path, typical CAN)

`SWC -> RTE -> COM -> PduR -> CanIf -> CAN Driver`

Use this when payload fits a single lower-layer frame and no TP segmentation is needed.

### Tx (TP path, larger payload)

`SWC -> RTE -> COM -> PduR -> CanTp/SoAd -> bus/network stack`

Use this when payload size exceeds IF limits or when transport segmentation/reassembly is required.

### Rx (generic)

`Bus/Network -> CanIf/CanTp/SoAd -> PduR -> COM -> RTE -> SWC`

Include indication/confirmation callback chain when debugging timing or data loss.

## Core Responsibilities by Module

- COM:
  - Signal packing/unpacking
  - I-PDU lifecycle handling
  - Deadline monitoring and update behavior (project-dependent)
- PduR:
  - PDU routing between upper/lower modules
  - IF/TP route selection
- CanIf/SoAd:
  - Bus or socket interface adaptation
  - Driver-facing send/receive handoff
- CanTp (or other TP):
  - Segmentation/reassembly
  - Flow control and transport state handling

## Design Heuristics

- Keep COM focused on signal semantics; avoid transport-specific logic in COM configuration.
- Treat PduR as the routing hub; centralize route decisions there.
- Use TP only where needed; IF is simpler and lower overhead for short payloads.
- Plan buffer sizes from worst-case burst + segmentation overhead.

## Common Debug Checklist

- PDU IDs consistent across COM/PduR/lower stack tables.
- Length assumptions aligned at every layer.
- IF vs TP mode matches actual payload sizes.
- Confirmation/indication callbacks wired and invoked as expected.
- Route path exists and is enabled for the specific PDU direction.
- For SPI-backed data flows, verify channel/job/sequence configuration and async/sync level usage.

## Additional Resource

- Deep reference: [reference.md](reference.md)
- Practical examples: [examples.md](examples.md)
