---
name: autosar-nvm-expert
description: Explains AUTOSAR NvM architecture, block management, and memory-stack integration with MemIf/Fee/Ea for reliable non-volatile data handling. Use when the user asks about NvM block design, startup ReadAll/WriteAll behavior, CRC/redundancy, queue priorities, or debugging persistence issues.
---

# AUTOSAR NvM Expert

## Quick Start

When handling NvM questions:

1. Classify the data path:
   - Read path, write path, ReadAll/WriteAll, or restore-default path
2. Identify block strategy:
   - Native, Redundant, or Dataset block
3. Identify stack route:
   - `NvM -> MemIf -> Fee/Ea -> Fls/Eep`

## Response Pattern

Use this structure:

```markdown
## Scope
[Affected block(s), operation, and failure symptom]

## Memory Stack Flow
[NvM request path and callbacks/state progression]

## Block and Integrity Model
- [Block type and RAM/NV/ROM relationships]
- [CRC/redundancy/write-verify behavior]

## Risks and Edge Cases
- [Queue/timing or endurance risk]
- [Recovery or default-data risk]

## Validation
- [Trace points and expected block states]
- [Pass criteria]
```

## Core NvM Concepts

- Block-centric persistence with Block IDs.
- RAM block as working copy; NV block as persistent copy.
- Optional ROM/default data path for recovery/factory defaults.
- Administrative metadata managed internally by NvM.

## Architecture Anchors

- Typical call chain:
  - `SWC/RTE -> NvM -> MemIf -> Fee/Ea -> memory driver`
- MemIf routes operations to configured memory abstraction.
- NvM sequencing and queue handling determine runtime persistence behavior.

## Design Heuristics

- Use Redundant blocks for safety/availability-critical data.
- Use CRC and write verification where integrity matters most.
- Keep immediate-priority writes for truly critical data only.
- Separate high-frequency mutable data from low-frequency calibration/state.
- Define explicit default-data and recovery behavior per critical block.

## Common Debug Checklist

- Correct Block ID and block type configuration.
- RAM/NV/ROM pointers and lengths are consistent.
- ReadAll/WriteAll ordering matches lifecycle requirements.
- Queue priorities do not starve normal block processing.
- Fee/Ea routing and lower-driver status are correct.

## Additional Resource

- Deep reference: [reference.md](reference.md)
- Practical examples: [examples.md](examples.md)
