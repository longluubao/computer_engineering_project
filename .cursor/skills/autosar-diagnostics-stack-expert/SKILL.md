---
name: autosar-diagnostics-stack-expert
description: Explains AUTOSAR Classic Diagnostics Stack architecture across DCM, DEM, DET, FiM, and transport integration over CAN or Ethernet DoIP. Use when the user asks about UDS request handling, DTC/event management, diagnostic timing (P2/P2*/S3), diagnostics routing, or troubleshooting ECU diagnostic behavior.
---

# AUTOSAR Diagnostics Stack Expert

## Quick Start

When answering diagnostics stack questions:

1. Identify diagnostic channel:
   - CAN-based diagnostics (UDS on CAN / via CanTp)
   - Ethernet diagnostics (DoIP)
2. Identify concern type:
   - Communication/session/timing issue (DCM path)
   - Fault memory/event status issue (DEM path)
   - Development-time API misuse (DET path)
   - Safety/feature inhibition behavior (FiM path)
3. Explain both protocol flow and module interaction flow.

## Response Pattern

Use this output structure:

```markdown
## Scope
[Bus/channel, ECU role, affected module]

## Stack Flow
[Ordered module chain and key callbacks]

## Timing and State
- [P2/P2*/S3 or operation-cycle implications]
- [Session/security preconditions]

## Data and Fault Handling
- [DTC/event/snapshot behavior]
- [Negative response or inhibition behavior]

## Validation Steps
- [Trace points/logs/counters to inspect]
- [Repro and pass criteria]
```

## Canonical Paths

### UDS over CAN (Classic)

`Tester -> CanIf/CanTp -> PduR -> DCM -> DEM/FiM/Application -> DCM -> PduR -> CanTp/CanIf -> Tester`

### UDS over DoIP (Ethernet)

`Tester -> Eth Driver/EthIf -> TcpIp -> SoAd -> DoIP/DCM path -> DEM/FiM/Application -> response path back`

Use the DoIP path when diagnostics run over Ethernet and routing activation/session establishment are relevant.

## Module Responsibilities

- DCM:
  - UDS/OBD request reception and dispatch
  - Session/security/timing control (including response timing behavior)
  - Positive/negative response orchestration
- DEM:
  - Event reporting, debouncing, DTC status management
  - Snapshot/freeze-frame and extended-data handling
- DET:
  - Development-time API/error tracing for BSW modules
- FiM:
  - Function permission decision based on diagnostic status
  - Inhibit functionality when configured failure conditions are met

## Diagnostics Timing Focus

Always check:

- P2 server response window
- P2* extended timing handling (response pending path)
- S3 session timeout behavior
- Operation cycle influence on event maturation and DTC status

## Common Failure Patterns

- Request reaches DCM but fails due to wrong session/security precondition.
- DTC exists in monitor logic but never matures in DEM due to debounce/operation-cycle setup.
- Tester times out because P2/P2* timing and long operation behavior are inconsistent.
- Function remains disabled because FiM inhibition mask and DEM status mapping are mismatched.
- Diagnostic routing breaks at PduR/transport mapping layer (wrong PDU/channel IDs).

## Additional Resource

- Deep reference: [reference.md](reference.md)
- Practical examples: [examples.md](examples.md)
