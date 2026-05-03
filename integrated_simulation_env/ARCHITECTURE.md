# ARCHITECTURE.md — Integrated Simulation Environment

## 1. Goals

1. Link the **real PQC modules** (`Autosar_SecOC/source/PQC/*.c` →
   liboqs) and exercise them end-to-end on simulated automotive buses
   without flashing an ECU.
2. Reproduce the **AUTOSAR SecOC protocol** (frame layout, freshness
   monotonicity, authenticator placement) in a deterministic, single
   process driver so attacks can be injected, replayed and measured.
3. Provide **repeatable** (seeded) runs with **thesis-grade logs**.
4. Allow arbitrary **attack injection** between Tx and Rx.

> **Scope note (read this before §5).** The ISE is a *focused PQC +
> SecOC-protocol harness*, not a full BSW integration test. Module-level
> AUTOSAR conformance is asserted by the 40-file gtest suite under
> `Autosar_SecOC/test/` which links the real `SecOCLib` static library.
> See `Autosar_SecOC/test/SecOCIntegrationTests.cpp` and the
> per-module `*Tests.cpp` files for that layer of the evidence.

## 2. Logical topology

```
┌──────────────────────────────────┐      Virtual bus      ┌──────────────────────────────────┐
│            ECU Tx                │◀─────────────────────▶│            ECU Rx                │
│  (Com → SecOC → PduR → CanIf /   │   (CAN/FD/FR/ENET)    │  (CanIf/SoAd → PduR → SecOC →    │
│   SoAd → sim_bus)                │                       │   Com) → application signal sink │
│                                  │          ▲            │                                  │
└──────────────────────────────────┘          │            └──────────────────────────────────┘
                                              │
                                     ┌────────┴────────┐
                                     │   Attacker       │   replay / tamper / MITM / DoS /
                                     │  (sim_attacker)  │   downgrade / fuzz / quantum probe
                                     └──────────────────┘
```

For gateway scenarios, a third ECU (`gw`) is instantiated that routes
between two virtual buses (CAN-FD ↔ 100BASE-T1) exercising the SoAd / PduR
gateway paths.

## 3. Threading model

Every virtual ECU runs in its own pthread. The simulator guarantees a
global monotonic clock (`sim_clock`) so latency numbers are meaningful
even when the host is under load. Optionally, a **deterministic mode**
serialises stack activity to the clock tick (no jitter from scheduling)
which is used for CI regression and replayable bug reproduction.

| Thread           | Responsibility                                      |
|------------------|-----------------------------------------------------|
| `sim_clock`      | Provide `sim_now_ns()` based on `CLOCK_MONOTONIC`   |
| `bus::can_fd`    | Transport CAN/CAN-FD frames FIFO w/ BER & delay     |
| `bus::flexray`   | Time-triggered TDMA slot scheduler                  |
| `bus::ethernet`  | Point-to-point Ethernet w/ TSN profile support      |
| `ecu::tx`        | Runs `SecOCMainFunctionTx`, application cycle       |
| `ecu::rx`        | Runs `SecOCMainFunctionRx`, consumes Com signals    |
| `ecu::gw`        | Forwards between `bus::can_fd` and `bus::ethernet`  |
| `attacker`       | Active injection (only during attack scenarios)     |
| `logger`         | Drains lock-free ring-buffers → CSV/JSON on disk    |

## 4. Bus model details

`sim_bus` simulates each bus with three knobs:

1. **Bandwidth** — bit-rate in bps; payload delay is `frame_bits / bps`.
2. **BER** — bit error rate; corrupted frames are dropped / flipped to
   emulate real cable conditions.
3. **Propagation delay** — per-hop latency in ns (configurable per bus).

```c
typedef struct {
    SimBusKind       kind;     /* CAN_2, CAN_FD, FLEXRAY, ETH_100, ETH_1000 */
    uint32_t         bitrate_bps;
    uint32_t         mtu_bytes;
    uint64_t         propagation_ns;
    double           bit_error_rate;
    /* TDMA params for FlexRay */
    uint32_t         slot_count;
    uint64_t         slot_duration_ns;
    /* TSN params for Ethernet */
    bool             tsn_enabled;
    uint8_t          priority_queue_count;
} SimBusCfg;
```

Messages carry a `tx_ns` (enqueue), `rx_ns` (deliver), and optional
`attack_tag` so the logger can correlate both sides.

## 5. Stack binding — what is actually compiled into `ise_runner`

The honest, machine-checkable truth is in `CMakeLists.txt:35-39`:

```cmake
set(AUTOSAR_SOURCES
    ${AUTOSAR_ROOT}/source/PQC/PQC.c
    ${AUTOSAR_ROOT}/source/PQC/PQC_KeyDerivation.c
    ${AUTOSAR_ROOT}/source/PQC/PQC_KeyExchange.c
)
```

That is the entire AUTOSAR-source set linked into `ise_runner`. The
PQC modules in turn pull in `liboqs.a` (NIST FIPS 203/204) and
OpenSSL (HMAC-SHA-256 + AES). Everything else under
`Autosar_SecOC/source/` (SecOC.c, PduR/*, Csm.c, CryIf.c, CanTp.c,
SoAd/*, Com/*, NvM/*, …) is **not** compiled by this CMake project.

### 5.1 What `sim_ecu.c` does instead

`integrated_simulation_env/src/sim_ecu.c` provides a *deterministic
re-implementation of the SecOC frame protocol* that calls the real
PQC modules:

```
            ┌────────────────────────────────────────────┐
            │ sim_ecu.c (this directory)                  │
            │   build_secured_pdu()                        │
            │     [hdr 2B | freshness 8B | payload | auth] │
            │     auth ← PQC_MLDSA_Sign() (liboqs)         │
            │           or HMAC-SHA-256 / 16 (OpenSSL)     │
            │   verify_secured_pdu()                       │
            │     freshness > last_rx ?                    │
            │     auth ← PQC_MLDSA_Verify() (liboqs)       │
            └────────────────────────────────────────────┘
                              ▲
                              │ exercised by 11 scenario drivers in
                              │ scenarios/sc_*.c, attack injection
                              │ via sim_attacker.c, deterministic
                              │ buses via sim_bus.c
```

This means ISE latency numbers measure: PQC sign/verify (real) +
freshness handling (re-implemented) + bus transit (modelled). They do
**not** include real Com/PduR/SecOC.c/Csm/CryIf/CanTp/SoAd overhead.

### 5.2 Where AUTOSAR module conformance is actually asserted

The **40-file gtest suite** under `Autosar_SecOC/test/` links the
real `SecOCLib` static library (which globs every `Autosar_SecOC/source/**/*.c`
file) and asserts each module's AUTOSAR API contract:

| Test executable                       | Module under test          |
|---------------------------------------|----------------------------|
| `SecOCTests`, `SecOCExtTests`         | `source/SecOC/SecOC.c`     |
| `SecOCIntegrationTests`               | full Tx→Rx round-trip       |
| `AuthenticationTests`                 | `Authenticate()`           |
| `VerificationTests`                   | `verify()` / `verify_PQC()` |
| `FreshnessTests` (495 LOC)            | FVM + counter math         |
| `DirectTxTests`, `DirectRxTests`      | `IfTransmit` / `RxIndication` |
| `startOfReceptionTests`               | TP `StartOfReception` API  |
| `CsmTests`, `CryIfTests`, `EncryptTests` | crypto stack            |
| `PduRTests`, `CanIfTests`, `CanTpTests`, `SoAdTests`, `SoAdPqcTests`, `EthIfTests`, `TcpIpTests` | PDU routing + TP |
| `ComTests`, `EcuMTests`, `BswMTests`, `ComMTests`, `OsTests` | upper BSW |
| `DemTests`, `DcmTests`, `DetTests`    | diagnostics                |
| `NvMTests`, `MemIfTests`, `FeeTests`, `EaTests` | NV memory       |
| `CanSMTests`, `EthSMTests`, `CanNmTests`, `UdpNmTests` | bus state |
| `KeyDerivationTests`, `KeyExchangeTests`, `PQC_ComparisonTests` | PQC |
| `McalTests`, `ApBridgeTests`          | leaf driver + bridge       |

41 ctest executables, **678 individual test cases**, 100 % pass rate
(see `THESIS_COVERAGE_AUDIT.md` for the latest run).

The thesis evidence base is therefore *two-layered*:

| Layer                    | Linker view                       | Asserted by              |
|--------------------------|-----------------------------------|--------------------------|
| AUTOSAR API conformance  | `SecOCLib` (real BSW C sources)  | `Autosar_SecOC/test/*`   |
| PQC + protocol semantics | `PQC.c` + `liboqs.a` + sim_ecu.c | `integrated_simulation_env/scenarios/sc_*.c` |

## 6. Deterministic time & seeding

Every scenario accepts `--seed <u64>`. The seed drives:

- the BER RNG (`xoshiro256**`) so frame-drop patterns are reproducible,
- the attacker's tamper offset choice,
- the signal-generator (payload contents and jitter).

Crypto material (`liboqs`) remains non-deterministic by design to reflect
real entropy sources, but the logger records the digest of every key
pair so a thesis reviewer can correlate runs.

## 7. Metric collection

`sim_metrics` keeps per-signal fixed-size histograms (log-scale buckets)
for:

- Application-to-application latency `app_tx→app_rx` (ns)
- Per-layer contributions: Com, SecOC (auth), Csm/PQC (sign/verify),
  CanTP fragmentation, bus transit
- Jitter (stddev of inter-arrival time)
- Deadline miss count per ASIL class
- Bytes in flight per second per bus

At the end of the scenario, `sim_logger_finalize()` writes four files:

| File                                | Content                                  |
|-------------------------------------|------------------------------------------|
| `<scenario>_raw.csv`                | every frame (tx_ns, rx_ns, layers, outcome)|
| `<scenario>_summary.json`           | aggregate stats + percentiles            |
| `<scenario>_attacks.csv`            | attack events + detection outcome        |
| `<scenario>_bus.csv`                | bus utilisation per 100 ms window        |

## 8. Extension points

Adding a new scenario is 3 steps:

1. Drop a `sc_<name>.c` in `scenarios/` exposing
   `int sc_<name>_run(const SimConfig*);`
2. Register it in `scenarios/scenario_runner.c`.
3. Add metadata (iterations, bus, attackers) in `config/scenarios.json`.

Adding a new attack is similarly simple — implement the `SimAttackerOps`
interface in `sim_attacker.h`.

## 9. Why this is suitable for a thesis

- **Separation of concerns** — the stack is unmodified, so results
  reflect the real implementation; only the hardware is abstracted.
- **Reproducibility** — seeded RNG + deterministic mode means a reviewer
  can re-run the very same experiment.
- **Traceability** — every log row carries the seed, scenario name,
  build git hash, and timestamp; the `compliance_matrix.md` maps rows
  back to SWS requirements and ISO 21434 clauses.
- **Scalable** — scenarios range from single-ECU benchmarks to multi-bus
  gateway topologies, covering the full thesis demonstration envelope.
