# AUTOSAR COM Stack Examples

## Example 1: IF vs TP Decision

Input:

"My message is 56 bytes on CAN. Should this stay IF?"

Expected response style:

- Identify payload exceeds single-frame IF limits.
- Recommend TP route: `COM -> PduR -> CanTp -> CanIf`.
- List required checks: PDU length, TP config, timeout/flow control.

## Example 2: Routing Failure Debug

Input:

"`Com_SendSignal` returns OK but frame never appears on bus."

Expected response style:

- Trace path: COM -> PduR -> lower module.
- Verify PDU ID mapping consistency and route activation.
- Verify lower-layer transmit confirmations and callback chain.
- Provide likely root causes ordered by probability.

## Example 3: Ethernet Path Design

Input:

"How do I route AUTOSAR I-PDU to Ethernet service path?"

Expected response style:

- Show chain: `COM -> PduR -> SoAd -> TcpIp -> EthIf`.
- Clarify static PDU vs socket lifecycle concerns.
- Define validation points (socket state, route group, indication callbacks).
