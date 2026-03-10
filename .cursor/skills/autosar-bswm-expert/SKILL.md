---
name: autosar-bswm-expert
description: Explains AUTOSAR Basic Software Mode Manager (BswM) architecture, rule arbitration, and action list design across EcuM, ComM, CanSM, Nm, and related BSW modules. Use when the user asks about mode-management design, BswM configuration, immediate vs deferred arbitration, or debugging mode-transition behavior.
---

# AUTOSAR BswM Expert

## Quick Start

When handling BswM questions:

1. Identify request source and target:
   - Source: SWC, EcuM, ComM, CanSM, Nm, Dcm, etc.
   - Target: communication allowance, network mode, ECU state, shutdown behavior
2. Classify arbitration style:
   - Immediate operation
   - Deferred operation (`BswM_MainFunction`)
3. Trace rule-to-action path:
   - Condition(s) -> logical rule -> action list -> BSW API calls

## Response Pattern

Use this structure:

```markdown
## Scope
[Vehicle function, involved mode source, impacted BSW modules]

## Arbitration Model
[Immediate/deferred, rule condition logic]

## Action Execution Path
[Rule -> action list -> called module APIs]

## Risks and Edge Cases
- [Race or ordering risk]
- [Fallback/degraded behavior]

## Validation
- [Trace points/counters]
- [Expected mode transitions]
```

## Core BswM Concepts

- Rule-based mode arbitration using logical conditions.
- Action lists as ordered side effects for mode control.
- Immediate vs deferred execution strategy.
- Integration hub behavior across mode-related BSW modules.

## Typical Integration Paths

- ECU lifecycle:
  - `Mode indication -> BswM rule -> BswMEcuM* actions`
- Communication gating:
  - `ComM/CanSM status -> BswM rule -> BswMComMAllowCom / ModeLimitation`
- Network management:
  - `Diagnostic or state trigger -> BswM rule -> BswMNMControl`

## Design Heuristics

- Keep rules simple, explicit, and auditable.
- Prefer small reusable action lists over one large monolithic list.
- Separate safety-critical transitions from convenience transitions.
- Use deferred arbitration for non-urgent mode updates to reduce jitter.
- Document precedence where multiple requests can conflict.

## Common Debug Checklist

- Requested mode really reaches the expected BswM request port.
- Rule result changes as expected for each condition transition.
- Correct action list is triggered (and only once where intended).
- Called module APIs confirm expected state/mode transitions.
- Immediate vs deferred configuration matches latency requirements.

## Additional Resource

- Deep reference: [reference.md](reference.md)
- Practical examples: [examples.md](examples.md)
