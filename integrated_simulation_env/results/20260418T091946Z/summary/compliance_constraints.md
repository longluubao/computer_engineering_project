# Compliance Constraints — Pass/Fail Matrix

Evaluates every scenario against three constraint families:

1. **Time** — end-to-end latency p99 vs. the ASIL deadline class of the exercised signal.
2. **Signal** — PDU size vs. the bus MTU; fragmentation count; authenticator overhead ratio.
3. **Security** — authenticator presence, freshness monotonicity (replay detection), and SecOC SWS conformance.

## 1. Time constraints (ASIL deadline vs. e2e p99)

| Scenario | e2e p99 (µs) | Deadline class | Budget (µs) | Pass |
|----------|--------------|----------------|-------------|------|
| attacks_aggregate_hmac | 2474.41 | D2 | ≤ 5000 | PASS |
| attacks_aggregate_pqc | 3261.16 | D2 | ≤ 5000 | PASS |
| attacks_downgrade_hmac_hmac | 2398.21 | D2 | ≤ 5000 | PASS |
| attacks_downgrade_hmac_pqc | 3203.82 | D2 | ≤ 5000 | PASS |
| attacks_freshness_rollback_hmac | 2439.23 | D2 | ≤ 5000 | PASS |
| attacks_freshness_rollback_pqc | 3199.32 | D2 | ≤ 5000 | PASS |
| attacks_mitm_key_confuse_hmac | 2381.79 | D2 | ≤ 5000 | PASS |
| attacks_mitm_key_confuse_pqc | 2654.36 | D2 | ≤ 5000 | PASS |
| attacks_replay_hmac | 3611.84 | D2 | ≤ 5000 | PASS |
| attacks_replay_pqc | 3288.77 | D2 | ≤ 5000 | PASS |
| attacks_sig_fuzz_hmac | 2474.27 | D2 | ≤ 5000 | PASS |
| attacks_sig_fuzz_pqc | 3321.76 | D2 | ≤ 5000 | PASS |
| attacks_tamper_auth_hmac | 2474.41 | D2 | ≤ 5000 | PASS |
| attacks_tamper_auth_pqc | 3254.78 | D2 | ≤ 5000 | PASS |
| attacks_tamper_payload_hmac | 2407.57 | D2 | ≤ 5000 | PASS |
| attacks_tamper_payload_pqc | 2799.56 | D2 | ≤ 5000 | PASS |
| baseline_hmac_r1 | 2397.51 | D2 | ≤ 5000 | PASS |
| baseline_hybrid_r1 | 3378.71 | D2 | ≤ 5000 | PASS |
| baseline_pqc_r1 | 3354.82 | D2 | ≤ 5000 | PASS |
| mixed_bus_can | 2659.91 | D2 | ≤ 5000 | PASS |
| mixed_bus_eth | 4254.74 | D2 | ≤ 5000 | PASS |
| rekey_r1 | 129.60 | D2 | ≤ 5000 | PASS |
| tput_hmac_canfd_r1 | 2437.93 | D2 | ≤ 5000 | PASS |
| tput_pqc_eth1000_r1 | 2190.72 | D2 | ≤ 5000 | PASS |
| tput_pqc_eth100_r1 | 2549.00 | D2 | ≤ 5000 | PASS |

## 2. Signal / PDU constraints

| Scenario | PDU mean (B) | PDU max (B) | Fragments max | Auth ratio (PQC ≈ 95 %) |
|----------|--------------|-------------|----------------|-------------------------|
| attacks_aggregate_hmac | 34.0 | 34 | 1 | — |
| attacks_aggregate_pqc | 3327.0 | 3327 | 52 | — |
| attacks_downgrade_hmac_hmac | 34.0 | 34 | 1 | — |
| attacks_downgrade_hmac_pqc | 3327.0 | 3327 | 52 | — |
| attacks_freshness_rollback_hmac | 34.0 | 34 | 1 | — |
| attacks_freshness_rollback_pqc | 3327.0 | 3327 | 52 | — |
| attacks_mitm_key_confuse_hmac | 34.0 | 34 | 1 | — |
| attacks_mitm_key_confuse_pqc | 3327.0 | 3327 | 52 | — |
| attacks_replay_hmac | 34.0 | 34 | 1 | — |
| attacks_replay_pqc | 3327.0 | 3327 | 52 | — |
| attacks_sig_fuzz_hmac | 34.0 | 34 | 1 | — |
| attacks_sig_fuzz_pqc | 3327.0 | 3327 | 52 | — |
| attacks_tamper_auth_hmac | 34.0 | 34 | 1 | — |
| attacks_tamper_auth_pqc | 3327.0 | 3327 | 52 | — |
| attacks_tamper_payload_hmac | 34.0 | 34 | 1 | — |
| attacks_tamper_payload_pqc | 3327.0 | 3327 | 52 | — |
| baseline_hmac_r1 | 173.1 | 1050 | 5 | — |
| baseline_hybrid_r1 | 3482.1 | 4359 | 418 | — |
| baseline_pqc_r1 | 3466.1 | 4343 | 416 | — |
| mixed_bus_can | 3327.0 | 3327 | 52 | — |
| mixed_bus_eth | 3327.0 | 3327 | 52 | — |
| rekey_r1 | 0.0 | 0 | 0 | — |
| tput_hmac_canfd_r1 | 34.0 | 34 | 1 | — |
| tput_pqc_eth1000_r1 | 4343.0 | 4343 | 3 | — |
| tput_pqc_eth100_r1 | 4343.0 | 4343 | 3 | — |

## 3. Security constraints (SecOC + PQC)

| Scenario | SecOC verify-fail count | Replay attempts delivered | Detection rate (%) | Pass |
|----------|------------------------|----------------------------|--------------------|------|
| attacks_aggregate_hmac | 503 | 197 | 71.86 | FAIL |
| attacks_aggregate_pqc | 697 | 3 | 99.57 | PASS |
| attacks_downgrade_hmac_hmac | 0 | 100 | 0.00 | FAIL |
| attacks_downgrade_hmac_pqc | 100 | 0 | 100.00 | PASS |
| attacks_freshness_rollback_hmac | 100 | 0 | 100.00 | PASS |
| attacks_freshness_rollback_pqc | 100 | 0 | 100.00 | PASS |
| attacks_mitm_key_confuse_hmac | 100 | 0 | 100.00 | PASS |
| attacks_mitm_key_confuse_pqc | 100 | 0 | 100.00 | PASS |
| attacks_replay_hmac | 3 | 97 | 3.00 | FAIL |
| attacks_replay_pqc | 97 | 3 | 97.00 | PASS |
| attacks_sig_fuzz_hmac | 100 | 0 | 100.00 | PASS |
| attacks_sig_fuzz_pqc | 100 | 0 | 100.00 | PASS |
| attacks_tamper_auth_hmac | 100 | 0 | 100.00 | PASS |
| attacks_tamper_auth_pqc | 100 | 0 | 100.00 | PASS |
| attacks_tamper_payload_hmac | 100 | 0 | 100.00 | PASS |
| attacks_tamper_payload_pqc | 100 | 0 | 100.00 | PASS |
| baseline_hmac_r1 | 0 | 0 | — | PASS |
| baseline_hybrid_r1 | 0 | 0 | — | PASS |
| baseline_pqc_r1 | 0 | 0 | — | PASS |
| mixed_bus_can | 0 | 0 | — | PASS |
| mixed_bus_eth | 0 | 0 | — | PASS |
| rekey_r1 | 0 | 0 | — | PASS |
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
