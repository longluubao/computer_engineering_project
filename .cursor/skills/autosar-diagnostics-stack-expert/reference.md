# AUTOSAR Diagnostics Stack Research Notes

## 1) Scope of the Diagnostics Stack

In AUTOSAR Classic projects, the diagnostics stack usually centers on:

- DCM (Diagnostic Communication Manager)
- DEM (Diagnostic Event Manager)
- DET (Development Error Tracer)
- FiM (Function Inhibition Manager)

It is integrated with transport and routing modules (for example PduR, CanTp, CanIf, SoAd, TcpIp) depending on channel (CAN or Ethernet DoIP).

## 2) Layered Architecture View

Typical diagnostic request path (high-level):

1. External tester sends diagnostic request.
2. Transport/network stack receives frames.
3. PduR/transport callbacks pass message to DCM.
4. DCM dispatches service handling to DEM/application/security/session logic.
5. Response is generated and returned through the reverse path.

Diagnostics is not only a communication problem; it is also event-memory and control-policy logic (DEM + FiM).

## 3) DCM Deep Dive (UDS Processing)

### Functional role

DCM handles UDS/OBD requests, validates preconditions, dispatches services, and manages response generation.

### Typical processing structure

Common implementation structure splits behavior into:

- Session/timing handling layer
- Service dispatcher layer
- Service processor layer

### Timing parameters to always review

- P2: normal server response timing budget.
- P2*: extended timing budget for long operations (response pending path).
- S3: non-default session timeout and fallback handling.

These timing parameters strongly impact tester-observed reliability.

## 4) DEM Deep Dive (Events and DTCs)

### Functional role

DEM ingests diagnostic events from SWCs/BSW monitors, applies debouncing/maturation logic, and manages DTC states and diagnostic data.

### Key data areas

- DTC status tracking (ISO 14229 style status byte semantics)
- Event memory (primary/mirror/user-defined or project-specific partitions)
- Freeze frame/snapshot and extended data storage

### Integration responsibilities

- Provide diagnostic status/data to DCM services (for example read DTC info services).
- Coordinate with NvM or persistence strategy for retained diagnostic data.

## 5) DET Deep Dive (Development-Time Diagnostics)

DET is development-focused and reports module/API/parameter misuse at runtime. It is useful for integration bring-up and API contract validation, but typically not treated as production fault memory like DEM.

Use DET to quickly localize:

- Invalid API usage sequence
- Null/invalid pointer misuse in BSW calls
- Out-of-range IDs/config access

## 6) FiM Deep Dive (Function Inhibition)

FiM maps diagnostic conditions to function permissions.

- It typically uses configured mappings between DEM event status and function identifiers (FIDs).
- Application logic queries permission before executing sensitive or safety-relevant features.

Common issue pattern: DEM event transitions are correct, but FiM inhibition masks or FID mapping cause unexpected persistent disablement.

## 7) Diagnostic Transport Paths

### A) UDS over CAN

Typical path:

`Tester -> CanIf/CanTp -> PduR -> DCM -> DEM/FiM/App -> response path back`

Important points:

- CanTp segmentation/reassembly for multi-frame services
- Timeout alignment between transport and DCM timing
- PDU mapping consistency across transport/routing/DCM

### B) UDS over Ethernet DoIP

Typical path:

`Tester -> EthIf/TcpIp/SoAd/DoIP integration -> DCM -> DEM/FiM/App -> response path back`

Important points:

- Routing activation and connection lifecycle
- Socket/session state interactions with diagnostic timing behavior
- Higher throughput can expose backend processing bottlenecks instead of transport bottlenecks

## 8) Common Failure Modes

### Request reception OK, response missing

Likely causes:

- Session/security preconditions not met
- Service handler rejected with NRC path not propagated correctly
- P2/P2* handling mismatch leading to tester timeout

### DTC expected but absent or unstable

Likely causes:

- Debounce configuration too strict or wrong operation-cycle semantics
- Event not stored due to memory/displacement policy
- Incorrect mapping between monitor event and DEM configuration

### Function remains inhibited unexpectedly

Likely causes:

- FiM condition masks do not match intended DEM status interpretation
- Event clear path incomplete, leaving inhibition condition true

### Transport-specific intermittency

Likely causes:

- CAN path: CanTp state/timeout/buffer constraints
- DoIP path: connection lifecycle and routing activation inconsistencies

## 9) Bring-Up and Debug Strategy

1. Validate communication channel path with one known service (for example ReadDataByIdentifier).
2. Validate DCM session/security transitions and timing behavior.
3. Inject one controlled fault event and trace DEM state transitions.
4. Verify DTC visibility and snapshot/extended data retrieval.
5. Verify FiM permission transitions for mapped functions.
6. Stress timing and burst scenarios for both request and response paths.

## 10) External Research Links Used

- Diagnostics stack overview:
  - [Diagnostic Stack in AUTOSAR](https://automotivevehicletesting.com/autosar/diagnostic-stack-in-autosar/)
  - [What are DEM, DLT, DCM and DET](https://www.autosartoday.com/posts/what_are_dem_dlt_dcm_and_det)
- Official AUTOSAR specs (public links):
  - [AUTOSAR SWS DCM](https://www.autosar.org/fileadmin/standards/R20-11/CP/AUTOSAR_SWS_DiagnosticCommunicationManager.pdf)
  - [AUTOSAR SWS DiagnosticOverIP](https://www.autosar.org/fileadmin/standards/R20-11/CP/AUTOSAR_SWS_DiagnosticOverIP.pdf)
- DCM timing/flow explainer references:
  - [AUTOSAR DCM Module](https://www.embeddedtutor.com/2019/09/autosar-dcm-module.html)
  - [Dcm configuration notes](https://autosar.dev/AUTOSAR/Classic_AUTOSAR/Configure_Parameters/Dcm)
- DEM and event memory references:
  - [AUTOSAR DEM Module](https://www.embeddedtutor.com/2019/09/autosar-dem-module.html)
  - [AUTOSAR DEM Event Memory notes](https://www.programmersought.com/article/67033576433/)
- FiM references:
  - [FiM Overview](https://www.autosartoday.com/posts/fim_overview)
  - [Function Inhibition Manager wiki](https://automotive.wiki/index.php/Function_Inhibition_Manager)
