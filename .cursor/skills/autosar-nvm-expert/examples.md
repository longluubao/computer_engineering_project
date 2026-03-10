# AUTOSAR NvM Examples

## Example 1: Startup Data Not Restored

Input:

"Values reset to default after reboot even though write API returned OK."

Expected response style:

- Inspect `WriteAll` timing and completion before shutdown.
- Verify block configuration (RAM/NV/default linkage).
- Check backend routing (`MemIf -> Fee/Ea`) and job status.

## Example 2: Integrity Failure Recovery

Input:

"How should we recover when CRC check fails for a critical block?"

Expected response style:

- Recommend redundant block strategy for critical data.
- Define fallback order (primary -> redundant -> default data).
- Include required verification and fault-injection tests.

## Example 3: Queue Starvation

Input:

"Normal NvM writes are delayed indefinitely under load."

Expected response style:

- Analyze immediate-priority block usage and queue policy.
- Identify starvation source and rebalance priorities.
- Propose monitoring metrics for queue latency and completion.
