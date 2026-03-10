# AUTOSAR OS Examples

## Example 1: Missed Deadline in Control Task

Input:

"A 5 ms control task sometimes misses deadline after enabling diagnostics."

Expected response style:

- Analyze task/ISR interaction and resource blocking.
- Check alarm/schedule-table timing offsets.
- Propose priority and critical-section adjustments.

## Example 2: Event-Driven Task Stuck

Input:

"Extended task stays in waiting state forever."

Expected response style:

- Verify event set/wait logic and activation path.
- Validate alarm or ISR source that should set the event.
- Provide minimal trace points to confirm state transitions.

## Example 3: Multicore Latency Spike

Input:

"Latency spikes appear only when both cores are active."

Expected response style:

- Check spinlock contention and hold durations.
- Identify cross-core shared-resource hotspots.
- Recommend partitioning or lock-scope reduction.
