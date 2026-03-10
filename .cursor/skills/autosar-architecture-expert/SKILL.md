---
name: autosar-architecture-expert
description: Explains AUTOSAR architecture, compares Classic and Adaptive platforms, and guides module-level design decisions for ECU software. Use when the user asks about AUTOSAR layered architecture, BSW/RTE/SWC interactions, communication stacks (CAN/Ethernet), service-oriented design, execution management, or migration between Classic and Adaptive AUTOSAR.
---

# AUTOSAR Architecture Expert

## Quick Start

When asked about AUTOSAR architecture:

1. Classify the scope first:
   - Classic Platform (CP)
   - Adaptive Platform (AP)
   - Mixed CP + AP system
2. Map the question to architecture level:
   - System architecture (vehicle/domain/zonal)
   - ECU software architecture (SWCs, RTE, BSW/services)
   - Communication path (signal/PDU/service)
   - Safety/security and lifecycle concerns
3. Respond with concrete module-level examples, not only theory.

## Response Pattern

Use this structure:

```markdown
## Context
[What platform and vehicle/software scope is assumed]

## Architecture View
[Layered or service-oriented view with key components]

## Data/Control Flow
[How data or control moves end-to-end]

## Design Guidance
- [Actionable recommendation 1]
- [Actionable recommendation 2]
- [Trade-off or risk]

## Validation
- [How to verify by tests, tracing, diagnostics, or timing]
```

## Architecture Baselines

### Classic Platform (CP)

- Layered architecture: Application SWCs -> RTE -> BSW -> MCAL.
- BSW typically split into:
  - Service Layer (OS, EcuM, ComM, NvM, Dem, Dcm, Csm, etc.)
  - ECU Abstraction Layer
  - Microcontroller Abstraction Layer (MCAL)
- Communication path usually signal/PDU based:
  - SWC ports/signals -> COM -> PduR -> bus interface/transport (CanIf/CanTp/SoAd)
- Deterministic, resource-constrained ECUs, strong static configuration.

### Adaptive Platform (AP)

- Service-oriented architecture for high-performance ECUs.
- Main concepts: applications/processes, services, discovery, execution lifecycle.
- Common platform concerns include:
  - Execution Management
  - State Management
  - Communication Management (`ara::com`)
  - Diagnostics, logging, crypto, update support
- Dynamic deployment and richer POSIX-like runtime environment.

### CP + AP Coexistence

- Use gateways/domain controllers to bridge signal/PDU-oriented and service-oriented domains.
- Define clear contracts for:
  - Timing budgets
  - Data transformation and serialization
  - Safety and cybersecurity boundaries

## Decision Heuristics

- Choose CP for hard real-time, low-resource, high-volume control ECUs.
- Choose AP for compute-intensive features (ADAS, central compute, service ecosystems).
- For mixed architectures, partition by criticality and latency sensitivity.
- Keep safety paths simple and deterministic; isolate non-critical service features.

## Common Pitfalls

- Mixing AP-style dynamic behavior into strict CP timing paths.
- Underestimating PDU routing and buffer sizing across bus boundaries.
- Weak interface contracts between COM signals, PDUs, and service APIs.
- Missing end-to-end observability (trace IDs, diagnostics events, health states).

## Additional Resource

- Deep reference: [reference.md](reference.md)
- Practical examples: [examples.md](examples.md)
