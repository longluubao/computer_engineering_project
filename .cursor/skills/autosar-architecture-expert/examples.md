# AUTOSAR Architecture Examples

## Example 1: CP vs AP Partitioning

Input:

"We have ADAS perception and braking control on one ECU. Should we use Classic or Adaptive?"

Expected response style:

- Classify functions by timing criticality.
- Place braking control on Classic (deterministic path).
- Place perception and service-heavy analytics on Adaptive.
- Define CP/AP boundary contract (latency, data model, fault fallback).

## Example 2: Gateway-Centric Design

Input:

"How should we connect CAN body domain ECUs to an Ethernet central computer?"

Expected response style:

- Show gateway path (signal/PDU side to service-oriented side).
- Identify transformation points (units, freshness, integrity metadata).
- Budget end-to-end latency across routing + serialization + security.
- Define diagnostics and observability requirements at boundary.

## Example 3: Migration Planning

Input:

"We want to migrate infotainment functions from Classic to Adaptive without breaking legacy CAN nodes."

Expected response style:

- Keep legacy control loops unchanged on Classic.
- Introduce AP services behind stable contracts.
- Use phased migration with compatibility adapters.
- Include rollback/degraded mode strategy for mixed deployments.
