# AUTOSAR Architecture Research Notes

## 1) AUTOSAR in One View

AUTOSAR defines standardized software architecture and interfaces for automotive ECUs to improve portability, reuse, and integration across suppliers. In practice, teams work with two major platforms:

- Classic Platform (CP): static, deterministic, resource-constrained ECU software.
- Adaptive Platform (AP): dynamic, service-oriented software on high-performance compute.

Many modern vehicles use both, connected through gateways/domain controllers.

## 2) Classic Platform Architecture (CP)

### Layering model

Typical vertical stack:

1. Application Layer (SWCs)
2. Runtime Environment (RTE)
3. Basic Software (BSW)
4. Microcontroller Abstraction Layer (MCAL)
5. Hardware

### Core role of each layer

- SWCs: application logic with standardized interfaces.
- RTE: generated middleware that maps SWC communication to BSW services.
- BSW Service Layer: OS, communication, diagnostics, memory, crypto, network management.
- ECU Abstraction + MCAL: hardware access isolation and peripheral drivers.

### CP communication path (signal/PDU style)

A common path for a transmitted application signal:

SWC -> RTE -> COM (signal packing) -> PduR (routing) -> CanIf/CanTp or SoAd -> bus driver/stack.

For secured communication (for example SecOC-based paths), authentication/freshness are inserted in the communication chain before final transport.

## 3) Adaptive Platform Architecture (AP)

### Architectural style

AP is service-oriented and process-based. Applications provide/consume services with dynamic discovery and runtime lifecycle control.

### Key concepts

- Execution and state lifecycle control across applications.
- Service interface contracts and discovery.
- `ara::com` style communication abstraction for services/events/fields.
- Platform services for diagnostics, logging, crypto, health and updates.

### Why AP exists

AP targets workloads not well served by strict CP assumptions: high compute demand, richer networking, frequent software updates, and cross-domain orchestration.

## 4) CP vs AP: Practical Comparison

- Determinism: CP stronger by default; AP requires careful scheduling and budgeting.
- Configuration: CP more static; AP more dynamic/deployable.
- Communication: CP commonly signal/PDU-centric; AP service-oriented.
- Resource profile: CP fits constrained MCUs; AP targets high-performance ECUs.
- Typical usage: CP for body/chassis/powertrain control; AP for ADAS/central compute/service-heavy functions.

## 5) Mixed Architecture Patterns (CP + AP)

### Domain and zonal systems

Vehicles often combine:

- Zonal/domain ECUs (CP-heavy real-time control),
- Central compute nodes (AP-heavy service orchestration),
- Bridges/gateways for data and control exchange.

### Integration recommendations

- Define explicit interface contracts:
  - Data models, units, validity and timing semantics.
  - Fault handling and degraded modes.
- Budget end-to-end latency across:
  - serialization/deserialization,
  - routing and transport,
  - security checks and diagnostics.
- Keep safety-critical loops on deterministic execution paths.
- Isolate non-critical service features from safety-relevant control.

## 6) Security and Safety Architecture Placement

### Security

- Place cybersecurity controls along communication boundaries and platform services.
- Typical concerns: authenticity/integrity, key management, secure diagnostics, secure updates.
- In mixed systems, preserve trust boundaries when crossing CP/AP or domain gateways.

### Safety

- Ensure safety goals are traceable to software architecture partitioning.
- Avoid unnecessary complexity in ASIL-relevant control paths.
- Use diagnostics and health monitoring with clear recovery behavior.

## 7) Performance and Verification Guidance

### Performance checkpoints

- Communication latency per layer (COM/PduR/transport/service middleware).
- Buffer usage and peak traffic behavior under burst load.
- Crypto overhead and impact on deadlines.

### Verification checklist

- Unit tests per module/service.
- Integration tests for end-to-end communication paths.
- Fault injection (timing loss, dropped messages, invalid payload, stale freshness).
- Diagnostics observability (event visibility, traceability, health status transitions).

## 8) Typical Architecture Questions to Ask Early

- Which functions truly require hard real-time determinism?
- Where should CP/AP boundary sit for this vehicle domain?
- What are the latency and availability budgets end-to-end?
- Which interfaces are safety-critical vs convenience-oriented?
- How are updates, diagnostics, and cybersecurity handled over lifecycle?

## 9) Short Design Recipe

1. Classify function criticality and timing class.
2. Allocate to CP, AP, or split architecture.
3. Define interface contracts (signals/PDUs/services).
4. Map communication and security controls end-to-end.
5. Validate with timing, fault-injection, and diagnostics-driven tests.
