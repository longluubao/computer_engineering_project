# Compliance Constraints — Pass/Fail Matrix

Evaluates every scenario against three constraint families:

1. **Time** — end-to-end latency p99 vs. the ASIL deadline class of the exercised signal.
2. **Signal** — PDU size vs. the bus MTU; fragmentation count; authenticator overhead ratio.
3. **Security** — authenticator presence, freshness monotonicity (replay detection), and SecOC SWS conformance.

## 1. Time constraints (ASIL deadline vs. e2e p99)

| Scenario | e2e p99 (µs) | Deadline class | Budget (µs) | Pass |
|----------|--------------|----------------|-------------|------|
| attacks_aggregate_hmac | 2439.36 | D2 | ≤ 5000 | PASS |
| attacks_aggregate_hybrid | 3330.29 | D2 | ≤ 5000 | PASS |
| attacks_aggregate_pqc | 3252.70 | D2 | ≤ 5000 | PASS |
| attacks_downgrade_hmac_hmac | 2391.23 | D2 | ≤ 5000 | PASS |
| attacks_downgrade_hmac_hybrid | 2287.68 | D2 | ≤ 5000 | PASS |
| attacks_downgrade_hmac_pqc | 3268.84 | D2 | ≤ 5000 | PASS |
| attacks_freshness_rollback_hmac | 2439.36 | D2 | ≤ 5000 | PASS |
| attacks_freshness_rollback_hybrid | 3337.47 | D2 | ≤ 5000 | PASS |
| attacks_freshness_rollback_pqc | 3190.07 | D2 | ≤ 5000 | PASS |
| attacks_mitm_key_confuse_hmac | 2362.14 | D2 | ≤ 5000 | PASS |
| attacks_mitm_key_confuse_hybrid | 2524.75 | D2 | ≤ 5000 | PASS |
| attacks_mitm_key_confuse_pqc | 3252.70 | D2 | ≤ 5000 | PASS |
| attacks_replay_hmac | 2545.54 | D2 | ≤ 5000 | PASS |
| attacks_replay_hybrid | 3342.07 | D2 | ≤ 5000 | PASS |
| attacks_replay_pqc | 3221.74 | D2 | ≤ 5000 | PASS |
| attacks_sig_fuzz_hmac | 2343.82 | D2 | ≤ 5000 | PASS |
| attacks_sig_fuzz_hybrid | 3209.68 | D2 | ≤ 5000 | PASS |
| attacks_sig_fuzz_pqc | 2441.52 | D2 | ≤ 5000 | PASS |
| attacks_tamper_auth_hmac | 2345.72 | D2 | ≤ 5000 | PASS |
| attacks_tamper_auth_hybrid | 2317.55 | D2 | ≤ 5000 | PASS |
| attacks_tamper_auth_pqc | 2550.22 | D2 | ≤ 5000 | PASS |
| attacks_tamper_payload_hmac | 2379.86 | D2 | ≤ 5000 | PASS |
| attacks_tamper_payload_hybrid | 3220.78 | D2 | ≤ 5000 | PASS |
| attacks_tamper_payload_pqc | 3224.49 | D2 | ≤ 5000 | PASS |
| baseline_hmac_r1 | 2423.33 | D2 | ≤ 5000 | PASS |
| baseline_hybrid_r1 | 3360.64 | D2 | ≤ 5000 | PASS |
| baseline_pqc_r1 | 3329.67 | D2 | ≤ 5000 | PASS |
| bus_failure_aggregate | 0.00 | D2 | ≤ 5000 | N/A |
| bus_failure_clean | 2455.39 | D2 | ≤ 5000 | PASS |
| bus_failure_heavy | 2341.79 | D2 | ≤ 5000 | PASS |
| bus_failure_noisy | 2373.68 | D2 | ≤ 5000 | PASS |
| deadline_relaxed | 1204.03 | D2 | ≤ 5000 | PASS |
| deadline_tight | 51245.99 | D2 | ≤ 5000 | FAIL |
| keymismatch_aggregate | 0.00 | D2 | ≤ 5000 | N/A |
| keymismatch_mismatch | 0.00 | D2 | ≤ 5000 | N/A |
| keymismatch_shared | 2368.52 | D2 | ≤ 5000 | PASS |
| mixed_bus_can | 2621.88 | D2 | ≤ 5000 | PASS |
| mixed_bus_eth | 4379.63 | D2 | ≤ 5000 | PASS |
| multi_ecu_aggregate | 0.00 | D2 | ≤ 5000 | N/A |
| multi_ecu_rx0 | 2415.88 | D2 | ≤ 5000 | PASS |
| multi_ecu_rx1 | 2410.91 | D2 | ≤ 5000 | PASS |
| multi_ecu_rx2 | 2384.15 | D2 | ≤ 5000 | PASS |
| multi_ecu_tx | 0.00 | D2 | ≤ 5000 | N/A |
| persistence_aggregate | 0.00 | D2 | ≤ 5000 | N/A |
| persistence_no_nvm | 2399.96 | D2 | ≤ 5000 | PASS |
| persistence_with_nvm | 2389.48 | D2 | ≤ 5000 | PASS |
| rekey_r1 | 106.89 | D2 | ≤ 5000 | PASS |
| rollover_hmac_r1 | 0.00 | D2 | ≤ 5000 | N/A |
| rollover_pqc_r1 | 0.00 | D2 | ≤ 5000 | N/A |
| tput_hmac_canfd_r1 | 2410.50 | D2 | ≤ 5000 | PASS |
| tput_pqc_eth1000_r1 | 2254.67 | D2 | ≤ 5000 | PASS |
| tput_pqc_eth100_r1 | 2583.68 | D2 | ≤ 5000 | PASS |

## 2. Signal / PDU constraints

| Scenario | PDU mean (B) | PDU max (B) | Fragments max | Auth ratio (PQC ≈ 95 %) |
|----------|--------------|-------------|----------------|-------------------------|
| attacks_aggregate_hmac | 34.0 | 34 | 1 | — |
| attacks_aggregate_hybrid | 3343.0 | 3343 | 53 | — |
| attacks_aggregate_pqc | 3327.0 | 3327 | 52 | — |
| attacks_downgrade_hmac_hmac | 34.0 | 34 | 1 | — |
| attacks_downgrade_hmac_hybrid | 3343.0 | 3343 | 53 | — |
| attacks_downgrade_hmac_pqc | 3327.0 | 3327 | 52 | — |
| attacks_freshness_rollback_hmac | 34.0 | 34 | 1 | — |
| attacks_freshness_rollback_hybrid | 3343.0 | 3343 | 53 | — |
| attacks_freshness_rollback_pqc | 3327.0 | 3327 | 52 | — |
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
| baseline_hmac_r1 | 173.1 | 1050 | 5 | — |
| baseline_hybrid_r1 | 3482.1 | 4359 | 418 | — |
| baseline_pqc_r1 | 3466.1 | 4343 | 416 | — |
| bus_failure_aggregate | 0.0 | 0 | 0 | — |
| bus_failure_clean | 34.0 | 34 | 1 | — |
| bus_failure_heavy | 34.0 | 34 | 1 | — |
| bus_failure_noisy | 34.0 | 34 | 1 | — |
| deadline_relaxed | 42.0 | 42 | 1 | — |
| deadline_tight | 34.0 | 34 | 0 | — |
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
| rollover_hmac_r1 | 34.0 | 34 | 1 | — |
| rollover_pqc_r1 | 3327.0 | 3327 | 52 | — |
| tput_hmac_canfd_r1 | 34.0 | 34 | 1 | — |
| tput_pqc_eth1000_r1 | 4343.0 | 4343 | 3 | — |
| tput_pqc_eth100_r1 | 4343.0 | 4343 | 3 | — |

## 3. Security constraints (SecOC + PQC)

| Scenario | SecOC verify-fail count | Replay attempts delivered | Detection rate (%) | Pass |
|----------|------------------------|----------------------------|--------------------|------|
| attacks_aggregate_hmac | 253 | 97 | 72.29 | FAIL |
| attacks_aggregate_hybrid | 347 | 3 | 99.14 | PASS |
| attacks_aggregate_pqc | 347 | 3 | 99.14 | PASS |
| attacks_downgrade_hmac_hmac | 0 | 50 | 0.00 | FAIL |
| attacks_downgrade_hmac_hybrid | 50 | 0 | 100.00 | PASS |
| attacks_downgrade_hmac_pqc | 50 | 0 | 100.00 | PASS |
| attacks_freshness_rollback_hmac | 50 | 0 | 100.00 | PASS |
| attacks_freshness_rollback_hybrid | 50 | 0 | 100.00 | PASS |
| attacks_freshness_rollback_pqc | 50 | 0 | 100.00 | PASS |
| attacks_mitm_key_confuse_hmac | 50 | 0 | 100.00 | PASS |
| attacks_mitm_key_confuse_hybrid | 50 | 0 | 100.00 | PASS |
| attacks_mitm_key_confuse_pqc | 50 | 0 | 100.00 | PASS |
| attacks_replay_hmac | 3 | 47 | 6.00 | FAIL |
| attacks_replay_hybrid | 47 | 3 | 94.00 | FAIL |
| attacks_replay_pqc | 47 | 3 | 94.00 | FAIL |
| attacks_sig_fuzz_hmac | 50 | 0 | 100.00 | PASS |
| attacks_sig_fuzz_hybrid | 50 | 0 | 100.00 | PASS |
| attacks_sig_fuzz_pqc | 50 | 0 | 100.00 | PASS |
| attacks_tamper_auth_hmac | 50 | 0 | 100.00 | PASS |
| attacks_tamper_auth_hybrid | 50 | 0 | 100.00 | PASS |
| attacks_tamper_auth_pqc | 50 | 0 | 100.00 | PASS |
| attacks_tamper_payload_hmac | 50 | 0 | 100.00 | PASS |
| attacks_tamper_payload_hybrid | 50 | 0 | 100.00 | PASS |
| attacks_tamper_payload_pqc | 50 | 0 | 100.00 | PASS |
| baseline_hmac_r1 | 0 | 0 | — | PASS |
| baseline_hybrid_r1 | 0 | 0 | — | PASS |
| baseline_pqc_r1 | 0 | 0 | — | PASS |
| bus_failure_aggregate | 0 | 0 | — | PASS |
| bus_failure_clean | 0 | 0 | — | PASS |
| bus_failure_heavy | 0 | 0 | — | PASS |
| bus_failure_noisy | 0 | 0 | — | PASS |
| deadline_relaxed | 0 | 0 | — | PASS |
| deadline_tight | 0 | 0 | — | PASS |
| keymismatch_aggregate | 50 | 0 | — | FAIL |
| keymismatch_mismatch | 50 | 0 | — | FAIL |
| keymismatch_shared | 0 | 0 | — | PASS |
| mixed_bus_can | 0 | 0 | — | PASS |
| mixed_bus_eth | 0 | 0 | — | PASS |
| multi_ecu_aggregate | 0 | 0 | — | PASS |
| multi_ecu_rx0 | 0 | 0 | — | PASS |
| multi_ecu_rx1 | 0 | 0 | — | PASS |
| multi_ecu_rx2 | 0 | 0 | — | PASS |
| multi_ecu_tx | 0 | 0 | — | PASS |
| persistence_aggregate | 25 | 25 | — | FAIL |
| persistence_no_nvm | 0 | 25 | — | PASS |
| persistence_with_nvm | 25 | 0 | — | FAIL |
| rekey_r1 | 0 | 0 | — | PASS |
| rollover_hmac_r1 | 8 | 0 | — | FAIL |
| rollover_pqc_r1 | 8 | 0 | — | FAIL |
| tput_hmac_canfd_r1 | 0 | 0 | — | PASS |
| tput_pqc_eth1000_r1 | 0 | 0 | — | PASS |
| tput_pqc_eth100_r1 | 0 | 0 | — | PASS |

## 4. AUTOSAR SWS_SecOC clauses

| Clause | Requirement | Evidence |
|--------|-------------|----------|
| SWS_SecOC_00106 | `SecOC_Init` initialises module state | `baseline_*_summary.json` (session_duration > 0) |
| SWS_SecOC_00112 | `SecOC_IfTransmit` transmits secured PDU | `baseline_*_frames.csv` (non-zero `secoc_auth_ns`) |
| SWS_SecOC_00177 | `SecOC_TpTransmit` handles large PDUs | `mixed_bus_*` + `tput_pqc_eth*` (fragments > 1) |
| SWS_SecOC_00209 | Freshness management rejects stale PDUs | `attacks_replay_*` (detection ≥ 95 %) |
| SWS_SecOC_00221 | Authenticator generation via Csm | `secoc_auth` histogram > 0 in every summary |
| SWS_SecOC_00230 | Drop PDU on verify failure | `attacks_*_summary.json` (verify_fail_count > 0) |

Deadline budgets come from SAE J3061 / ISO 26262 informative bands (D1 ≤ 1 ms, D2 ≤ 5 ms, …, D6 ≤ 100 ms). The scenario mapping above uses D2 as a generic worst-case class — see `raw/*_frames.csv` for per-signal deadline classes.
