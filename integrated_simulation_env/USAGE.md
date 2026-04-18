# ISE — Concepts & Usage Guide

This document explains the concepts behind the Integrated Simulation
Environment (ISE) and shows how to run the full thesis campaign on
Linux (x86_64), Raspberry Pi 4 (aarch64) and Windows (MinGW).

---

## 1. Architecture

```
┌─────────────────────────────────────────────────────────┐
│  Scenario (sc_baseline / sc_throughput / sc_attacks…)   │  ← test kịch bản
└───────────────────┬─────────────────────────────────────┘
                    │
┌───────────────────▼────────────┐   ┌──────────────────┐
│   SimEcu (mô phỏng 1 ECU)      │◄──┤  SimAttacker     │  ← chèn attack
│   - AUTOSAR stack:             │   │  (hook vào bus)  │
│     Com → PduR → SecOC → Csm   │   └──────────────────┘
│     → PQC → SoAd / CanIf       │
└───────────────────┬────────────┘
                    │ send/recv
┌───────────────────▼────────────┐
│   SimBus (CAN / CAN-FD /       │   ← bus ảo: tính bitrate,
│   FlexRay / Ethernet 100/1000) │     propagation, MTU, drop
└───────────────────┬────────────┘
                    │
          ┌─────────▼─────────┐
          │   SimMetrics      │   ← histogram + percentile
          │   SimLogger       │   ← CSV frames + JSON summary
          └───────────────────┘
```

The real C sources of the AUTOSAR BSW stack (`Autosar_SecOC/source/`) are
compiled directly into `ise_runner`; only the MCAL / physical bus is
replaced by a deterministic software model. This lets the same code
that will run on the Pi 4 be exercised end-to-end before deployment.

---

## 2. Attack catalogue

The `attacks` scenario evaluates the 7 active attacks below — each one
tries to get a malicious or stale PDU accepted by the receiver.
`detection_rate_pct = detected / (detected + delivered)` is the share
of attacked messages the receiver rejected.

| Attack | Mô tả | SecOC + PQC xử lý bằng |
|--------|-------|------------------------|
| **replay** | Ghi lại frame cũ, phát lại sau | Freshness counter (8 B, phải tăng dần) |
| **tamper_payload** | Flip bit trong data | ML-DSA signature bao toàn bộ payload |
| **tamper_auth** | Flip bit trong signature | ML-DSA verify fail |
| **freshness_rollback** | Reset counter về 0 | FVM reject `freshness ≤ last_seen` |
| **mitm_key_confuse** | MITM swap khóa session | Signature verify fail vì sai public key |
| **sig_fuzz** | Ngẫu nhiên hoá bytes signature | ML-DSA verify fail |
| **downgrade_hmac** | Ép protection mode về HMAC | Signature fail vì header byte đổi |

### Attacks excluded from the default loop

Three attacks do **not** fit a detection-rate metric. They remain in the
enum and can still be invoked individually via `--attack <kind>`, but
they are not part of the aggregated thesis numbers:

- **dos_flood** — impact = throughput / drop, not verify-bypass. Needs a
  dedicated flood thread and a throughput metric.
- **harvest_now** — passive eavesdropping mitigated by the PQC algorithm
  choice itself. No runtime detection event exists.
- **timing_probe** — side-channel, addressed by liboqs constant-time
  implementations rather than the SecOC verify path.

---

## 3. Building

```bash
cd integrated_simulation_env
bash build.sh
```

`build.sh` auto-detects the platform and picks the right generator and
flags:

| Host | Detected via | Extra flags |
|------|--------------|-------------|
| Raspberry Pi 4 (aarch64) | `uname -m` | `-mcpu=cortex-a72 -O3 -fPIC` |
| Raspberry Pi 3 (armv7l)  | `uname -m` | `-O3 -fPIC` |
| x86_64 Linux             | `uname -m` | `-O3` |
| Windows (MinGW/MSYS)     | `uname -s` | generator `MinGW Makefiles` |

`liboqs` is built automatically if it is not already present at
`Autosar_SecOC/external/liboqs/build/`.

---

## 4. Running the full campaign

```bash
bash run_all_scenarios.sh              # defaults: 200 iterations, 1 repeat
bash run_all_scenarios.sh 1000 3       # 1000 iterations, 3 repeats
```

Output lands in `results/<UTC-timestamp>/`:

```
results/20260418T090020Z/
├── raw/                            # per-iteration traces
│   ├── baseline_pqc_r1_frames.csv
│   ├── attacks_pqc_r1_attacks.csv
│   └── …
├── summary/                        # thesis-ready tables
│   ├── latency_stats.csv
│   ├── attack_detection.csv
│   ├── deadline_miss.csv
│   ├── pdu_overhead.csv
│   ├── cross_env_comparison.md
│   └── compliance_matrix.md
└── report.md                       # main thesis report
```

---

## 5. Running individual scenarios

```bash
# baseline latency / overhead (HMAC vs PQC vs HYBRID)
./build/ise_runner --scenario baseline   --protection pqc    --iterations 500
./build/ise_runner --scenario baseline   --protection hmac   --iterations 500
./build/ise_runner --scenario baseline   --protection hybrid --iterations 500

# throughput on a specific bus
./build/ise_runner --scenario throughput --protection pqc  --bus eth1000
./build/ise_runner --scenario throughput --protection pqc  --bus eth100
./build/ise_runner --scenario throughput --protection hmac --bus canfd

# gateway CAN-FD ↔ Ethernet (double verify/sign overhead)
./build/ise_runner --scenario mixed_bus  --protection pqc

# all 7 active attacks
./build/ise_runner --scenario attacks    --protection pqc

# single attack (also supports dos_flood / harvest_now / timing_probe)
./build/ise_runner --scenario attacks    --protection pqc --attack replay
./build/ise_runner --scenario attacks    --protection pqc --attack tamper_payload

# ML-KEM rekey (keygen + encaps + decaps + HKDF)
./build/ise_runner --scenario rekey --iterations 200

# NvM freshness persistence across reboot (SWS_SecOC_00194)
# Two phases: no_nvm (breach) vs with_nvm (safe) — replay accepted/rejected
./build/ise_runner --scenario persistence      --protection pqc  --iterations 200

# Physical-layer BER sweep (CanSM / EthSM surface)
# Three phases: clean, noisy (1e-6), heavy (1e-4)
./build/ise_runner --scenario bus_failure      --protection pqc  --iterations 100

# Deadline miss per ASIL class (D1=5ms .. D6=500ms)
# Tight: BrakeCmd (ASIL-D, D1) on CAN 2.0 → 100% miss with PQC
# Relaxed: Speedometer (ASIL-B, D4) on Eth 100 → 0% miss
./build/ise_runner --scenario deadline_stress  --protection pqc  --iterations 100

# Multi-ECU broadcast: 1 TX × N RX with shared freshness / signer key
./build/ise_runner --scenario multi_ecu        --protection pqc  --iterations 80

# Wrong-key rejection (SWS_SecOC_00046 cryptographic binding)
# Phase A shared keys → 100% accept; Phase B independent keys → 100% reject
./build/ise_runner --scenario keymismatch      --protection pqc  --iterations 50

# Freshness counter wrap-around + rekey recovery (SWS_SecOC_00033)
# approach UINT64_MAX → wrap (all rejected) → rekey epoch (channel restored)
./build/ise_runner --scenario rollover         --protection pqc  --iterations 20
```

### Common flags

| Flag | Meaning |
|------|---------|
| `--iterations N` | Number of messages / handshakes |
| `--protection {hmac,pqc,hybrid}` | Authenticator mode |
| `--bus {can,canfd,flexray,eth100,eth1000}` | Physical bus for throughput / mixed_bus |
| `--attack <kind>` | Run a single attack kind (see enum `SimAttackKind`) |
| `--seed N` | Deterministic RNG seed (reproducible runs) |
| `--out <dir>` | Output directory (default: `results/<UTC>/raw`) |

---

## 6. Platform-specific notes

### Raspberry Pi 4 (target)

```bash
git clone <repo>
cd computer_engineering_project/integrated_simulation_env
bash build.sh         # auto-detects aarch64, applies -mcpu=cortex-a72 -O3
bash run_all_scenarios.sh
```

### Windows (MinGW / MSYS)

```bash
cd integrated_simulation_env
bash build.sh         # auto-picks "MinGW Makefiles"
./build/ise_runner.exe --scenario baseline --protection pqc --iterations 200
```

No special environment setup is needed beyond MinGW, CMake and a POSIX
shell (Git Bash / MSYS2). The scenarios, metrics and reports work
identically to Linux.

---

## 7. Reading the results

### `summary/latency_stats.csv`
`samples, e2e_mean_us, e2e_p50_us, e2e_p95_us, e2e_p99_us, e2e_p999_us,
auth_mean_us, verify_mean_us, cantp_mean_us, bus_mean_us` — per scenario.
Percentiles come from a 16 384-entry reservoir (exact when `samples ≤
reservoir`, log2 bucket fallback otherwise).

### `summary/attack_detection.csv`
`attacks_injected, attacks_detected, attacks_delivered,
detection_rate_pct`. `injected` counts fragment-level hook activity and
is informational; the thesis metric is `detection_rate_pct`, computed
as `detected / (detected + delivered)` (message-level).

### `summary/cross_env_comparison.md`
Side-by-side table of ISE (full stack) vs Pi 4 unit-only vs x86_64
unit-only. ISE numbers come from the JSON summaries; Pi/x86 numbers are
parsed from the historical log files in `PiTest/` and
`Autosar_SecOC/test_logs/`.

### `raw/*_frames.csv`
One row per PDU with per-layer timestamps: `seq, tx_ns, rx_ns,
secoc_auth_ns, cantp_ns, pdu_bytes, auth_bytes, fragments, signal_id,
asil, deadline_class, protected_mode, outcome`. Useful for the thesis's
detailed latency-breakdown figures.

### `raw/*_attacks.csv`
One row per attacker event with `sim_ns, bus_id, attack_kind,
signal_id, note`. Used to align attack bursts with verify-fail events
in the time-series plots.

---

## 8. Key thesis numbers (PQC, full stack)

From `results/20260418T090020Z/`:

```
ML-DSA Sign:       164 µs       End-to-end latency (p50):  2153 µs
ML-DSA Verify:     107 µs       End-to-end latency (p99):  3258 µs
ML-KEM KeyGen:      23 µs       PDU size (PQC):            3466 B
                                PDU size (HMAC):            173 B
Detection rate (PQC aggregate, 7 active attacks):   99.79 %
Deadline miss rate (all ASIL classes D1–D6):        0.00 %
```

Cross-environment (real measurements, not documentation placeholders):

| Environment | Sign [µs] | Verify [µs] | KeyGen [µs] |
|-------------|-----------|-------------|-------------|
| ISE (full AUTOSAR stack) | 164 | 107 | 23 |
| Pi 4 unit-only | 10 581 | 518 | n/a |
| x86_64 unit-only | 2 048 | 129 | n/a |

The ISE numbers are faster than the bare-metal unit-only measurements
because `liboqs` in the ISE build is compiled with `-O3`, while the
historical Pi / x86 archives used the default optimisation level.
