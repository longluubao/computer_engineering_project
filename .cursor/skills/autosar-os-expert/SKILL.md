---
name: autosar-os-expert
description: Explains AUTOSAR Classic OS architecture, scheduling, tasks/ISRs, alarms/counters/schedule tables, and protection mechanisms for safety-critical ECUs. Use when the user asks about OS configuration, task design, timing behavior, multicore synchronization, memory/timing protection, or AUTOSAR OS debugging.
---

# AUTOSAR OS Expert

## Quick Start

When answering AUTOSAR OS questions:

1. Classify the issue first:
   - Functional design (task/event/alarm/resource model)
   - Timing behavior (latency, jitter, deadline misses)
   - Protection and safety (timing/memory/application isolation)
   - Multicore synchronization (spinlocks/shared resources)
2. Identify runtime object chain:
   - Task type (basic/extended)
   - ISR category
   - Counter/Alarm/ScheduleTable ownership
   - Resource and hook usage
3. Provide both configuration guidance and runtime-debug checkpoints.

## Response Pattern

Use this structure:

```markdown
## Scope
[Single-core/multicore, target OS feature, symptoms]

## OS Model
[Tasks, ISRs, alarms/counters, resources/events involved]

## Timing and Protection
- [Scheduling and preemption implications]
- [Timing protection and memory/application isolation concerns]

## Design Guidance
- [Recommended configuration pattern]
- [Trade-off or risk]

## Validation
- [What to trace/log/measure]
- [Pass criteria]
```

## Core AUTOSAR OS Areas

- Task model:
  - Basic vs extended tasks
  - Activation behavior and priority assignment
- Interrupt model:
  - ISR integration and interaction with scheduler/resources
- Time-trigger model:
  - Counters, alarms, and schedule tables
- Synchronization:
  - Resources and (on multicore) spinlocks/shared access control
- Hook and error handling:
  - Startup/shutdown, error and context hooks

## Design Heuristics

- Keep high-rate control paths short and deterministic; avoid unnecessary blocking calls.
- Use extended tasks only when event waiting semantics are needed.
- Prefer explicit timing budgets per task/ISR before tuning priorities.
- Treat shared-resource design as a timing problem, not only a correctness problem.
- In multicore systems, minimize global lock duration and cross-core contention.

## Common Debug Checklist

- Ready/running/waiting transitions match expected task model.
- Alarm/counter periods and schedule-table offsets match requirement timing.
- Resource/spinlock usage does not create long blocking chains.
- Hook/error paths are visible and not masking root-cause timing faults.
- Protection violations (timing/memory/application) are captured and diagnosable.

## Additional Resource

- Deep reference: [reference.md](reference.md)
- Practical examples: [examples.md](examples.md)
