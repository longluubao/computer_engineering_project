---
name: autosar-ecum-expert
description: Explains AUTOSAR EcuM architecture for ECU lifecycle control, including startup/shutdown, sleep and wakeup handling, and integration with BswM/ComM/NvM/OS. Use when the user asks about ECU state transitions, EcuM Fixed vs Flex behavior, wakeup validation flow, or debugging boot and power-mode issues.
---

# AUTOSAR EcuM Expert

## Quick Start

When handling EcuM questions:

1. Identify lifecycle phase:
   - Startup (pre-OS / post-OS)
   - Run / post-run
   - Sleep / wakeup
   - Shutdown / reset target
2. Identify variant:
   - EcuM Fixed
   - EcuM Flex
3. Trace ownership:
   - What EcuM controls directly
   - What is delegated to BswM and other mode managers

## Response Pattern

Use this structure:

```markdown
## Scope
[ECU state issue, variant, and affected modules]

## Lifecycle Flow
[Ordered phase transitions and key API/callback points]

## Module Interaction
- [EcuM with BswM/ComM/NvM/OS/etc.]
- [Who owns each transition decision]

## Risks and Failure Modes
- [Timing/order hazard]
- [Wakeup or shutdown edge case]

## Validation
- [What to trace and expected sequence]
- [Pass criteria]
```

## Core EcuM Responsibilities

- Initialize/de-initialize core BSW and OS-related startup sequence.
- Manage high-level ECU states and lifecycle transitions.
- Coordinate wakeup-source handling and validation.
- Control shutdown target selection and orderly power-down path.

## Fixed vs Flex Guidance

- Fixed:
  - More fixed state-transition behavior inside EcuM logic.
- Flex:
  - Broader mode orchestration through BswM rule/action model.
  - EcuM focuses strongly on startup/shutdown framework and wakeup handling.

When unsure, explicitly state assumptions and map them to configured project behavior.

## Design Heuristics

- Keep startup chain deterministic and minimal before OS is fully up.
- Separate wakeup detection, wakeup validation, and communication enable steps.
- Align shutdown ordering with persistence and diagnostics requirements.
- Treat EcuM-BswM boundaries as explicit contracts, not implicit behavior.

## Common Debug Checklist

- Startup order matches expected pre-OS and post-OS sequencing.
- Wakeup sources are enabled, reported, and validated as configured.
- BswM receives the expected EcuM indications for mode arbitration.
- ComM/NvM actions occur in the intended lifecycle phase.
- Shutdown path flushes critical state before final transition.

## Additional Resource

- Deep reference: [reference.md](reference.md)
- Practical examples: [examples.md](examples.md)
