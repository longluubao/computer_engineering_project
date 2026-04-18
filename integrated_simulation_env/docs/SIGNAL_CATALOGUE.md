# Signal Catalogue

The ISE uses a fixed catalogue of 14 representative automotive signals.
They cover the full ASIL spectrum and the four bus families discussed
in the thesis. The same catalogue is compiled into `src/sim_signals.c`
and described in `config/signals.json` for external tooling.

## Legend

- **ASIL** — Automotive Safety Integrity Level (ISO 26262). `QM` =
  quality-managed, `A` < `B` < `C` < `D` in ascending rigour.
- **Deadline class** — the end-to-end deadline we enforce; see
  `THESIS_RESEARCH.md §1.2`:

  | Class | End-to-end deadline |
  |-------|---------------------|
  | D1    |   5 ms              |
  | D2    |  10 ms              |
  | D3    |  20 ms              |
  | D4    |  50 ms              |
  | D5    | 100 ms              |
  | D6    | 500 ms              |

- **TP mode** — `true` forces SecOC_TpTransmit / CAN-TP fragmentation;
  this is required on CAN/CAN-FD when the secured PDU exceeds the raw
  bus MTU.

## Table

| id   | Name                  | ASIL | Deadline | Cycle  | Payload | Preferred bus  | TP mode | Why this signal exists                                       |
|------|-----------------------|------|----------|--------|---------|----------------|---------|---------------------------------------------------------------|
| 0x01 | BrakeCmd              | D    | D1       |   5 ms |   8 B   | CAN-FD         | no      | Canonical safety-critical short command                       |
| 0x02 | SteeringTorque        | D    | D1       |   5 ms |  16 B   | CAN-FD         | no      | Steer-by-wire setpoint; tests 16-byte payload on short cycle  |
| 0x03 | AirbagTrigger         | D    | D1       |   5 ms |   4 B   | CAN 2.0 B      | no      | Legacy CAN — stresses 8-byte MTU vs. auth overhead            |
| 0x04 | Throttle              | C    | D2       |  10 ms |   8 B   | CAN-FD         | no      | Typical powertrain ASIL-C signal                              |
| 0x05 | AbsWheelSpeed         | C    | D2       |  10 ms |  32 B   | CAN-FD         | no      | Medium payload — tests 1-2 CAN-FD fragments                   |
| 0x06 | InverterTorque        | C    | D2       |  10 ms |  16 B   | FlexRay        | no      | Time-triggered backbone signal                                |
| 0x07 | BodyMotion            | B    | D3       |  20 ms |  24 B   | CAN-FD         | no      | Suspension damping                                           |
| 0x08 | DoorLockState         | B    | D3       |  20 ms |   8 B   | CAN 2.0 B      | no      | Body CAN control signal                                       |
| 0x09 | Speedometer           | B    | D4       |  50 ms |  16 B   | CAN-FD         | no      | HMI display with safety relevance                             |
| 0x0A | HVAC                  | QM   | D4       |  50 ms |   8 B   | CAN 2.0 B      | no      | Comfort — QM baseline                                         |
| 0x0B | InfotainmentGateway   | QM   | D5       | 100 ms | 512 B   | 100BASE-T1     | yes     | Large Ethernet PDU — exercises SoAd TP path                   |
| 0x0C | ADAS_Camera_Meta      | C    | D3       |  20 ms | 256 B   | 100BASE-T1     | yes     | ADAS metadata — realistic ASIL-C/Eth signal                   |
| 0x0D | V2X_AuthenticatedMsg  | B    | D5       | 100 ms | 1024 B  | 1000BASE-T1    | yes     | V2X — main hero for PQC (signature-dominated PDU)             |
| 0x0E | OBD_Diagnostic        | QM   | D6       | 500 ms | 128 B   | 100BASE-T1     | yes     | Low-priority diagnostic traffic                               |

## Rationale for signal selection

- **Short cycle, short payload (0x01, 0x03):** stress the auth overhead
  relative to the bus capacity. With PQC enabled, a 3309-byte signature
  cannot fit these CAN frames — the scenario thus also documents the
  *infeasibility* of PQC on bare CAN and motivates the gateway design.
- **Medium payload on FlexRay (0x06):** shows the time-triggered slot
  constraint on gateway re-authentication.
- **Ethernet TP mode (0x0B..0x0E):** exercises the SoAd/CanTP paths
  that are required to carry a PQC signature. These are the scenarios
  where PQC is realistic today.
- **ASIL-D/C/B/QM mix:** ensures each deadline class appears at least
  once, producing a well-populated `deadline_miss.csv`.

## Modifying the catalogue

The catalogue is compiled into the binary for determinism. To add a
signal:

1. Append a row to the array in `src/sim_signals.c`.
2. Mirror the change in `config/signals.json` and this table.
3. Rebuild (`bash build.sh`).

Scenarios iterate over the full catalogue — no scenario code needs to
be touched unless the new signal requires a new bus kind.
