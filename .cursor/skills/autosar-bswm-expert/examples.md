# AUTOSAR BswM Examples

## Example 1: Allow Communication After Wakeup

Input:

"After valid wakeup, I want network communication enabled automatically."

Expected response style:

- Define rule condition from EcuM wakeup indication.
- Map true-action list to `BswMComMAllowCom`.
- Specify validation for one-shot vs repeated execution behavior.

## Example 2: Conflicting Mode Requests

Input:

"ComM asks for full communication while power management asks no communication."

Expected response style:

- Explain arbitration precedence and rule layering.
- Propose explicit priority strategy and fallback behavior.
- Show how to avoid oscillation/chattering.

## Example 3: Immediate vs Deferred Choice

Input:

"Should this BswM transition be immediate or deferred?"

Expected response style:

- Compare latency requirement vs runtime jitter impact.
- Recommend immediate for urgent safety/availability transitions.
- Recommend deferred for high-frequency non-urgent requests.
