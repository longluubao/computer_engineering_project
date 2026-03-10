# AUTOSAR Diagnostics Stack Examples

## Example 1: UDS Timeout Investigation

Input:

"Tester times out on routine control, but ECU eventually finishes."

Expected response style:

- Map DCM DSL/DSD/DSP flow.
- Check P2/P2* handling and NRC 0x78 usage.
- Verify transport and response timing alignment.

## Example 2: DTC Missing in 0x19

Input:

"Monitor reports failure, but `ReadDTCInformation (0x19)` shows nothing."

Expected response style:

- Inspect DEM debouncing and operation-cycle configuration.
- Check event memory/storage policy and DTC mapping.
- Confirm DCM service access/session/security preconditions.

## Example 3: DoIP Diagnostics Path

Input:

"UDS over Ethernet works after boot, then fails after reconnect."

Expected response style:

- Trace DoIP path through SoAd/TcpIp/DCM.
- Validate routing activation and socket lifecycle handling.
- Check session continuity and re-initialization behavior.
