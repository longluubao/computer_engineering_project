# AUTOSAR EcuM Examples

## Example 1: Startup Phase Clarification

Input:

"Where should this module init happen: before or after OS start?"

Expected response style:

- Place init in pre-OS vs post-OS based on dependency chain.
- Show startup sequence checkpoints.
- Explain effect on BswM and application bring-up.

## Example 2: Wakeup Validation Failure

Input:

"ECU wakes up, then goes back to sleep unexpectedly."

Expected response style:

- Separate wakeup detection vs wakeup validation logic.
- Verify source enable/disable ordering and validation result path.
- Check mode indications forwarded to BswM/ComM flow.

## Example 3: Graceful Shutdown

Input:

"How do I guarantee critical data is saved before ECU off?"

Expected response style:

- Define shutdown order with NvM/diagnostic finalization.
- Confirm transition target (`OFF`/`RESET`/`SLEEP`) handling.
- Provide trace-based pass criteria for shutdown completeness.
