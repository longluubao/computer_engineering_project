# Compliance Constraints — Pass/Fail Matrix

Evaluates every scenario against three constraint families:

1. **Time** — end-to-end latency p99 vs. the ASIL deadline class of the exercised signal.
2. **Signal** — PDU size vs. the bus MTU; fragmentation count; authenticator overhead ratio.
3. **Security** — authenticator presence, freshness monotonicity (replay detection), and SecOC SWS conformance.

## 1. Time constraints (ASIL deadline vs. e2e p99)

| Scenario | e2e p99 (µs) | Deadline class | Budget (µs) | Pass |
|----------|--------------|----------------|-------------|------|
| attacks_aggregate_hmac | 915.40 | D2 | ≤ 5000 | PASS |
| attacks_aggregate_hybrid | 1344.02 | D2 | ≤ 5000 | PASS |
| attacks_aggregate_pqc | 1299.30 | D2 | ≤ 5000 | PASS |
| attacks_dos_flood_hmac | 913.55 | D2 | ≤ 5000 | PASS |
| attacks_dos_flood_hybrid | 1965.48 | D2 | ≤ 5000 | PASS |
| attacks_dos_flood_pqc | 2016.07 | D2 | ≤ 5000 | PASS |
| attacks_downgrade_hmac_hmac | 916.31 | D2 | ≤ 5000 | PASS |
| attacks_downgrade_hmac_hybrid | 1272.59 | D2 | ≤ 5000 | PASS |
| attacks_downgrade_hmac_pqc | 2004.25 | D2 | ≤ 5000 | PASS |
| attacks_freshness_rollback_hmac | 944.35 | D2 | ≤ 5000 | PASS |
| attacks_freshness_rollback_hybrid | 2000.49 | D2 | ≤ 5000 | PASS |
| attacks_freshness_rollback_pqc | 1222.76 | D2 | ≤ 5000 | PASS |
| attacks_harvest_now_hmac | 903.59 | D2 | ≤ 5000 | PASS |
| attacks_harvest_now_hybrid | 2039.74 | D2 | ≤ 5000 | PASS |
| attacks_harvest_now_pqc | 1958.74 | D2 | ≤ 5000 | PASS |
| attacks_mitm_key_confuse_hmac | 894.58 | D2 | ≤ 5000 | PASS |
| attacks_mitm_key_confuse_hybrid | 1890.62 | D2 | ≤ 5000 | PASS |
| attacks_mitm_key_confuse_pqc | 1958.37 | D2 | ≤ 5000 | PASS |
| attacks_replay_hmac | 937.07 | D2 | ≤ 5000 | PASS |
| attacks_replay_hybrid | 1197.82 | D2 | ≤ 5000 | PASS |
| attacks_replay_pqc | 1240.53 | D2 | ≤ 5000 | PASS |
| attacks_sig_fuzz_hmac | 921.61 | D2 | ≤ 5000 | PASS |
| attacks_sig_fuzz_hybrid | 1260.37 | D2 | ≤ 5000 | PASS |
| attacks_sig_fuzz_pqc | 1197.21 | D2 | ≤ 5000 | PASS |
| attacks_tamper_auth_hmac | 899.75 | D2 | ≤ 5000 | PASS |
| attacks_tamper_auth_hybrid | 1170.63 | D2 | ≤ 5000 | PASS |
| attacks_tamper_auth_pqc | 1313.86 | D2 | ≤ 5000 | PASS |
| attacks_tamper_payload_hmac | 903.42 | D2 | ≤ 5000 | PASS |
| attacks_tamper_payload_hybrid | 1143.64 | D2 | ≤ 5000 | PASS |
| attacks_tamper_payload_pqc | 1166.31 | D2 | ≤ 5000 | PASS |
| attacks_timing_probe_hmac | 909.68 | D2 | ≤ 5000 | PASS |
| attacks_timing_probe_hybrid | 1338.94 | D2 | ≤ 5000 | PASS |
| attacks_timing_probe_pqc | 1281.93 | D2 | ≤ 5000 | PASS |
| baseline_hmac_r1 | 963.85 | D2 | ≤ 5000 | PASS |
| baseline_hybrid_r1 | 2149.82 | D2 | ≤ 5000 | PASS |
| baseline_pqc_r1 | 2162.74 | D2 | ≤ 5000 | PASS |
| bus_failure_aggregate | 0.00 | D2 | ≤ 5000 | N/A |
| bus_failure_clean | 908.62 | D2 | ≤ 5000 | PASS |
| bus_failure_heavy | 931.45 | D2 | ≤ 5000 | PASS |
| bus_failure_noisy | 935.45 | D2 | ≤ 5000 | PASS |
| deadline_relaxed | 880.77 | D2 | ≤ 5000 | PASS |
| deadline_tight | 50318.90 | D2 | ≤ 5000 | FAIL |
| flexray_hmac_r1 | 877.67 | D2 | ≤ 5000 | PASS |
| flexray_hybrid_r1 | 1262.48 | D2 | ≤ 5000 | PASS |
| flexray_pqc_r1 | 1179.14 | D2 | ≤ 5000 | PASS |
| freshness_overflow_hmac_r1 | 0.00 | D2 | ≤ 5000 | N/A |
| freshness_overflow_pqc_r1 | 0.00 | D2 | ≤ 5000 | N/A |
| keymismatch_aggregate | 0.00 | D2 | ≤ 5000 | N/A |
| keymismatch_mismatch | 0.00 | D2 | ≤ 5000 | N/A |
| keymismatch_shared | 885.03 | D2 | ≤ 5000 | PASS |
| mixed_bus_can | 1206.20 | D2 | ≤ 5000 | PASS |
| mixed_bus_eth | 2194.66 | D2 | ≤ 5000 | PASS |
| multi_ecu_aggregate | 0.00 | D2 | ≤ 5000 | N/A |
| multi_ecu_rx0 | 919.94 | D2 | ≤ 5000 | PASS |
| multi_ecu_rx1 | 883.59 | D2 | ≤ 5000 | PASS |
| multi_ecu_rx2 | 914.44 | D2 | ≤ 5000 | PASS |
| multi_ecu_tx | 0.00 | D2 | ≤ 5000 | N/A |
| persistence_aggregate | 0.00 | D2 | ≤ 5000 | N/A |
| persistence_no_nvm | 853.40 | D2 | ≤ 5000 | PASS |
| persistence_with_nvm | 841.17 | D2 | ≤ 5000 | PASS |
| rekey_r1 | 76.59 | D2 | ≤ 5000 | PASS |
| replay_boundary_hmac_r1 | 0.00 | D2 | ≤ 5000 | N/A |
| replay_boundary_pqc_r1 | 0.00 | D2 | ≤ 5000 | N/A |
| rollover_hmac_r1 | 0.00 | D2 | ≤ 5000 | N/A |
| rollover_pqc_r1 | 0.00 | D2 | ≤ 5000 | N/A |
| tput_hmac_canfd_r1 | 933.15 | D2 | ≤ 5000 | PASS |
| tput_pqc_eth1000_r1 | 1122.34 | D2 | ≤ 5000 | PASS |
| tput_pqc_eth100_r1 | 1243.40 | D2 | ≤ 5000 | PASS |
| verify_disabled_r1 | 866.35 | D2 | ≤ 5000 | PASS |

## 2. Signal / PDU constraints

| Scenario | PDU mean (B) | PDU max (B) | Fragments max | Auth ratio (PQC ≈ 95 %) |
|----------|--------------|-------------|----------------|-------------------------|
| attacks_aggregate_hmac | 34.0 | 34 | 1 | — |
| attacks_aggregate_hybrid | 3343.0 | 3343 | 53 | — |
| attacks_aggregate_pqc | 3327.0 | 3327 | 52 | — |
| attacks_dos_flood_hmac | 34.0 | 34 | 1 | — |
| attacks_dos_flood_hybrid | 3343.0 | 3343 | 53 | — |
| attacks_dos_flood_pqc | 3327.0 | 3327 | 52 | — |
| attacks_downgrade_hmac_hmac | 34.0 | 34 | 1 | — |
| attacks_downgrade_hmac_hybrid | 3343.0 | 3343 | 53 | — |
| attacks_downgrade_hmac_pqc | 3327.0 | 3327 | 52 | — |
| attacks_freshness_rollback_hmac | 34.0 | 34 | 1 | — |
| attacks_freshness_rollback_hybrid | 3343.0 | 3343 | 53 | — |
| attacks_freshness_rollback_pqc | 3327.0 | 3327 | 52 | — |
| attacks_harvest_now_hmac | 34.0 | 34 | 1 | — |
| attacks_harvest_now_hybrid | 3343.0 | 3343 | 53 | — |
| attacks_harvest_now_pqc | 3327.0 | 3327 | 52 | — |
| attacks_mitm_key_confuse_hmac | 34.0 | 34 | 1 | — |
| attacks_mitm_key_confuse_hybrid | 3343.0 | 3343 | 53 | — |
| attacks_mitm_key_confuse_pqc | 3327.0 | 3327 | 52 | — |
| attacks_replay_hmac | 34.0 | 34 | 1 | — |
| attacks_replay_hybrid | 3343.0 | 3343 | 53 | — |
| attacks_replay_pqc | 3327.0 | 3327 | 52 | — |
| attacks_sig_fuzz_hmac | 34.0 | 34 | 1 | — |
| attacks_sig_fuzz_hybrid | 3343.0 | 3343 | 53 | — |
| attacks_sig_fuzz_pqc | 3327.0 | 3327 | 52 | — |
| attacks_tamper_auth_hmac | 34.0 | 34 | 1 | — |
| attacks_tamper_auth_hybrid | 3343.0 | 3343 | 53 | — |
| attacks_tamper_auth_pqc | 3327.0 | 3327 | 52 | — |
| attacks_tamper_payload_hmac | 34.0 | 34 | 1 | — |
| attacks_tamper_payload_hybrid | 3343.0 | 3343 | 53 | — |
| attacks_tamper_payload_pqc | 3327.0 | 3327 | 52 | — |
| attacks_timing_probe_hmac | 34.0 | 34 | 1 | — |
| attacks_timing_probe_hybrid | 3343.0 | 3343 | 53 | — |
| attacks_timing_probe_pqc | 3327.0 | 3327 | 52 | — |
| baseline_hmac_r1 | 173.1 | 1050 | 5 | — |
| baseline_hybrid_r1 | 3482.1 | 4359 | 418 | — |
| baseline_pqc_r1 | 3466.1 | 4343 | 416 | — |
| bus_failure_aggregate | 0.0 | 0 | 0 | — |
| bus_failure_clean | 34.0 | 34 | 1 | — |
| bus_failure_heavy | 34.0 | 34 | 1 | — |
| bus_failure_noisy | 34.0 | 34 | 1 | — |
| deadline_relaxed | 42.0 | 42 | 1 | — |
| deadline_tight | 34.0 | 34 | 0 | — |
| flexray_hmac_r1 | 99.5 | 280 | 1 | — |
| flexray_hybrid_r1 | 3408.5 | 3589 | 53 | — |
| flexray_pqc_r1 | 3392.5 | 3573 | 53 | — |
| freshness_overflow_hmac_r1 | 34.0 | 34 | 1 | — |
| freshness_overflow_pqc_r1 | 3327.0 | 3327 | 52 | — |
| keymismatch_aggregate | 0.0 | 0 | 0 | — |
| keymismatch_mismatch | 34.0 | 34 | 1 | — |
| keymismatch_shared | 34.0 | 34 | 1 | — |
| mixed_bus_can | 3327.0 | 3327 | 52 | — |
| mixed_bus_eth | 3327.0 | 3327 | 52 | — |
| multi_ecu_aggregate | 0.0 | 0 | 0 | — |
| multi_ecu_rx0 | 0.0 | 0 | 0 | — |
| multi_ecu_rx1 | 0.0 | 0 | 0 | — |
| multi_ecu_rx2 | 0.0 | 0 | 0 | — |
| multi_ecu_tx | 34.0 | 34 | 1 | — |
| persistence_aggregate | 0.0 | 0 | 0 | — |
| persistence_no_nvm | 34.0 | 34 | 1 | — |
| persistence_with_nvm | 34.0 | 34 | 1 | — |
| rekey_r1 | 0.0 | 0 | 0 | — |
| replay_boundary_hmac_r1 | 34.0 | 34 | 1 | — |
| replay_boundary_pqc_r1 | 3327.0 | 3327 | 52 | — |
| rollover_hmac_r1 | 34.0 | 34 | 1 | — |
| rollover_pqc_r1 | 3327.0 | 3327 | 52 | — |
| tput_hmac_canfd_r1 | 34.0 | 34 | 1 | — |
| tput_pqc_eth1000_r1 | 4343.0 | 4343 | 3 | — |
| tput_pqc_eth100_r1 | 4343.0 | 4343 | 3 | — |
| verify_disabled_r1 | 18.0 | 18 | 1 | — |

## 3. Security constraints (SecOC + PQC)

| Scenario | SecOC verify-fail count | Replay attempts delivered | Detection rate (%) | Pass |
|----------|------------------------|----------------------------|--------------------|------|
| attacks_aggregate_hmac | 1197 | 203 | 85.50 | FAIL |
| attacks_aggregate_hybrid | 1397 | 3 | 99.79 | PASS |
| attacks_aggregate_pqc | 1397 | 3 | 99.79 | PASS |
| attacks_dos_flood_hmac | 0 | 200 | 0.00 | FAIL |
| attacks_dos_flood_hybrid | 0 | 200 | 0.00 | FAIL |
| attacks_dos_flood_pqc | 0 | 200 | 0.00 | FAIL |
| attacks_downgrade_hmac_hmac | 0 | 200 | 0.00 | FAIL |
| attacks_downgrade_hmac_hybrid | 200 | 0 | 100.00 | PASS |
| attacks_downgrade_hmac_pqc | 200 | 0 | 100.00 | PASS |
| attacks_freshness_rollback_hmac | 200 | 0 | 100.00 | PASS |
| attacks_freshness_rollback_hybrid | 200 | 0 | 100.00 | PASS |
| attacks_freshness_rollback_pqc | 200 | 0 | 100.00 | PASS |
| attacks_harvest_now_hmac | 0 | 200 | 0.00 | FAIL |
| attacks_harvest_now_hybrid | 0 | 200 | 0.00 | FAIL |
| attacks_harvest_now_pqc | 0 | 200 | 0.00 | FAIL |
| attacks_mitm_key_confuse_hmac | 200 | 0 | 100.00 | PASS |
| attacks_mitm_key_confuse_hybrid | 200 | 0 | 100.00 | PASS |
| attacks_mitm_key_confuse_pqc | 200 | 0 | 100.00 | PASS |
| attacks_replay_hmac | 197 | 3 | 98.50 | PASS |
| attacks_replay_hybrid | 197 | 3 | 98.50 | PASS |
| attacks_replay_pqc | 197 | 3 | 98.50 | PASS |
| attacks_sig_fuzz_hmac | 200 | 0 | 100.00 | PASS |
| attacks_sig_fuzz_hybrid | 200 | 0 | 100.00 | PASS |
| attacks_sig_fuzz_pqc | 200 | 0 | 100.00 | PASS |
| attacks_tamper_auth_hmac | 200 | 0 | 100.00 | PASS |
| attacks_tamper_auth_hybrid | 200 | 0 | 100.00 | PASS |
| attacks_tamper_auth_pqc | 200 | 0 | 100.00 | PASS |
| attacks_tamper_payload_hmac | 200 | 0 | 100.00 | PASS |
| attacks_tamper_payload_hybrid | 200 | 0 | 100.00 | PASS |
| attacks_tamper_payload_pqc | 200 | 0 | 100.00 | PASS |
| attacks_timing_probe_hmac | 0 | 200 | 0.00 | FAIL |
| attacks_timing_probe_hybrid | 0 | 200 | 0.00 | FAIL |
| attacks_timing_probe_pqc | 0 | 200 | 0.00 | FAIL |
| baseline_hmac_r1 | 0 | 0 | — | PASS |
| baseline_hybrid_r1 | 0 | 0 | — | PASS |
| baseline_pqc_r1 | 0 | 0 | — | PASS |
| bus_failure_aggregate | 5 | 0 | — | FAIL |
| bus_failure_clean | 0 | 0 | — | PASS |
| bus_failure_heavy | 5 | 0 | — | FAIL |
| bus_failure_noisy | 0 | 0 | — | PASS |
| deadline_relaxed | 0 | 0 | — | PASS |
| deadline_tight | 0 | 0 | — | PASS |
| flexray_hmac_r1 | 0 | 0 | — | PASS |
| flexray_hybrid_r1 | 0 | 0 | — | PASS |
| flexray_pqc_r1 | 0 | 0 | — | PASS |
| freshness_overflow_hmac_r1 | 8 | 0 | — | FAIL |
| freshness_overflow_pqc_r1 | 8 | 0 | — | FAIL |
| keymismatch_aggregate | 200 | 0 | — | FAIL |
| keymismatch_mismatch | 200 | 0 | — | FAIL |
| keymismatch_shared | 0 | 0 | — | PASS |
| mixed_bus_can | 0 | 0 | — | PASS |
| mixed_bus_eth | 0 | 0 | — | PASS |
| multi_ecu_aggregate | 0 | 0 | — | PASS |
| multi_ecu_rx0 | 0 | 0 | — | PASS |
| multi_ecu_rx1 | 0 | 0 | — | PASS |
| multi_ecu_rx2 | 0 | 0 | — | PASS |
| multi_ecu_tx | 0 | 0 | — | PASS |
| persistence_aggregate | 100 | 100 | — | FAIL |
| persistence_no_nvm | 0 | 100 | — | PASS |
| persistence_with_nvm | 100 | 0 | — | FAIL |
| rekey_r1 | 0 | 0 | — | PASS |
| replay_boundary_hmac_r1 | 47 | 0 | — | FAIL |
| replay_boundary_pqc_r1 | 47 | 0 | — | FAIL |
| rollover_hmac_r1 | 8 | 0 | — | FAIL |
| rollover_pqc_r1 | 8 | 0 | — | FAIL |
| tput_hmac_canfd_r1 | 0 | 0 | — | PASS |
| tput_pqc_eth1000_r1 | 0 | 0 | — | PASS |
| tput_pqc_eth100_r1 | 0 | 0 | — | PASS |
| verify_disabled_r1 | 0 | 50 | — | PASS |

## 4. AUTOSAR SWS_SecOC clauses

| Clause | Requirement | Evidence |
|--------|-------------|----------|
| SWS_SecOC_00033 | Freshness counter strict monotonicity | `rollover_*_summary.json` (wrap rejected, rekey recovers) |
| SWS_SecOC_00046 | Verifier uses the configured SecOCKeyId | `keymismatch_mismatch_summary.json` (100 % rejection) |
| SWS_SecOC_00106 | `SecOC_Init` initialises module state | `baseline_*_summary.json` (session_duration > 0) |
| SWS_SecOC_00112 | `SecOC_IfTransmit` transmits secured PDU | `baseline_*_frames.csv` (non-zero `secoc_auth_ns`) |
| SWS_SecOC_00177 | `SecOC_TpTransmit` handles large PDUs | `mixed_bus_*` + `tput_pqc_eth*` (fragments > 1) |
| SWS_SecOC_00194 | Persist freshness across reboot via NvM | `persistence_no_nvm` breach vs `persistence_with_nvm` 0 delivered |
| SWS_SecOC_00209 | Freshness management rejects stale PDUs | `attacks_replay_*` (detection ≥ 95 %) |
| SWS_SecOC_00221 | Authenticator generation via Csm | `secoc_auth` histogram > 0 in every summary |
| SWS_SecOC_00230 | Drop PDU on verify failure | `attacks_*_summary.json` (verify_fail_count > 0) |

Deadline budgets come from SAE J3061 / ISO 26262 informative bands (D1 ≤ 1 ms, D2 ≤ 5 ms, …, D6 ≤ 100 ms). The scenario mapping above uses D2 as a generic worst-case class — see `raw/*_frames.csv` for per-signal deadline classes.
