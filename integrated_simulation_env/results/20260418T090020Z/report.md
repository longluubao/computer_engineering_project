# Integrated Simulation Environment — Thesis Report

Generated automatically by `generate_thesis_report.py`.

Number of scenarios collected: **25**.


## 1. Latency and overhead

Raw numbers in `summary/latency_stats.csv`. The table below is copy-pasteable into LaTeX via `pgfplotstable`.

| scenario | samples | e2e_mean_us | e2e_p50_us | e2e_p95_us | e2e_p99_us | e2e_p999_us | auth_mean_us | verify_mean_us | cantp_mean_us | bus_mean_us |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| attacks_aggregate_hmac | 1400 | 2146.84 | 2259.89 | 2322.42 | 2404.96 | 2978.53 | 6.86 | 9.66 | 8.79 | 0.00 |
| attacks_aggregate_pqc | 1400 | 2112.18 | 2177.73 | 2586.98 | 3256.17 | 3356.35 | 194.19 | 67.99 | 116.21 | 0.00 |
| attacks_downgrade_hmac_hmac | 200 | 2153.21 | 2262.47 | 2331.24 | 2452.93 | 2483.81 | 4.77 | 10.83 | 8.44 | 0.00 |
| attacks_downgrade_hmac_pqc | 200 | 2150.63 | 2181.33 | 2584.41 | 3239.12 | 3242.64 | 162.41 | 110.04 | 110.47 | 0.00 |
| attacks_freshness_rollback_hmac | 200 | 2200.01 | 2255.43 | 2311.10 | 2349.31 | 2354.54 | 10.99 | 0.00 | 8.95 | 0.00 |
| attacks_freshness_rollback_pqc | 200 | 2185.36 | 2188.34 | 2624.43 | 3274.01 | 3302.66 | 247.90 | 0.00 | 117.27 | 0.00 |
| attacks_mitm_key_confuse_hmac | 200 | 2163.30 | 2262.86 | 2322.75 | 2359.31 | 2367.89 | 5.03 | 10.80 | 8.20 | 0.00 |
| attacks_mitm_key_confuse_pqc | 200 | 2099.04 | 2171.49 | 2600.45 | 3241.32 | 3243.24 | 201.37 | 42.30 | 149.93 | 0.00 |
| attacks_replay_hmac | 200 | 2180.95 | 2267.35 | 2320.26 | 2515.57 | 2758.78 | 12.61 | 11.30 | 8.75 | 0.00 |
| attacks_replay_pqc | 200 | 1997.99 | 2167.52 | 2566.59 | 3244.68 | 3250.56 | 174.51 | 104.40 | 81.01 | 0.00 |
| attacks_sig_fuzz_hmac | 200 | 2068.88 | 2258.01 | 2310.40 | 2400.55 | 2418.31 | 4.77 | 11.29 | 9.15 | 0.00 |
| attacks_sig_fuzz_pqc | 200 | 2146.01 | 2178.02 | 2611.38 | 3280.72 | 3285.04 | 212.65 | 44.56 | 125.18 | 0.00 |
| attacks_tamper_auth_hmac | 200 | 2082.45 | 2249.78 | 2318.06 | 2427.91 | 2457.82 | 4.76 | 10.99 | 8.25 | 0.00 |
| attacks_tamper_auth_pqc | 200 | 2066.51 | 2171.98 | 2584.11 | 3253.53 | 3254.12 | 187.33 | 80.74 | 116.99 | 0.00 |
| attacks_tamper_payload_hmac | 200 | 2179.10 | 2262.46 | 2336.37 | 2391.53 | 2401.08 | 5.09 | 12.39 | 9.77 | 0.00 |
| attacks_tamper_payload_pqc | 200 | 2139.73 | 2177.66 | 2573.24 | 3204.22 | 3263.01 | 173.13 | 93.89 | 112.58 | 0.00 |
| baseline_hmac_r1 | 2800 | 1994.09 | 2248.88 | 2316.54 | 2386.33 | 2998.01 | 6.20 | 12.05 | 9.63 | 0.00 |
| baseline_hybrid_r1 | 2800 | 2094.06 | 2188.36 | 2549.38 | 3191.80 | 3306.09 | 165.50 | 118.47 | 32.50 | 0.00 |
| baseline_pqc_r1 | 2800 | 2117.81 | 2193.60 | 2564.22 | 3291.13 | 3416.74 | 163.30 | 109.54 | 32.86 | 0.00 |
| mixed_bus_can | 200 | 2077.66 | 2186.75 | 2503.95 | 2602.88 | 3232.93 | 164.90 | 109.76 | 55.89 | 0.00 |
| mixed_bus_eth | 200 | 3440.59 | 3304.95 | 5462.75 | 5731.48 | 5760.50 | 156.53 | 110.02 | 55.08 | 0.00 |
| rekey_r1 | 200 | 49.68 | 45.57 | 66.61 | 75.75 | 83.57 | 21.22 | 15.37 | 12.80 | 0.00 |
| tput_hmac_canfd_r1 | 923 | 2166.82 | 2262.79 | 2332.79 | 2427.56 | 2617.98 | 6.65 | 10.79 | 7.08 | 0.00 |
| tput_pqc_eth1000_r1 | 1438 | 1390.90 | 1393.26 | 2141.30 | 2226.61 | 3192.41 | 170.68 | 113.14 | 13.76 | 0.00 |
| tput_pqc_eth100_r1 | 911 | 2196.78 | 2186.89 | 2504.77 | 2588.27 | 2970.09 | 169.69 | 117.33 | 14.62 | 0.00 |


## 2. Attack detection (UN R155 Annex 5 mapping)

Source: `summary/attack_detection.csv`.

| scenario | attacks_injected | attacks_detected | attacks_delivered | detection_rate_pct |
| --- | --- | --- | --- | --- |
| attacks_aggregate_hmac | 1397 | 1003 | 397 | 71.64 |
| attacks_aggregate_pqc | 62608 | 1397 | 3 | 99.79 |
| attacks_downgrade_hmac_hmac | 200 | 0 | 200 | 0.00 |
| attacks_downgrade_hmac_pqc | 10400 | 200 | 0 | 100.00 |
| attacks_freshness_rollback_hmac | 200 | 200 | 0 | 100.00 |
| attacks_freshness_rollback_pqc | 10400 | 200 | 0 | 100.00 |
| attacks_mitm_key_confuse_hmac | 200 | 200 | 0 | 100.00 |
| attacks_mitm_key_confuse_pqc | 10400 | 200 | 0 | 100.00 |
| attacks_replay_hmac | 197 | 3 | 197 | 1.50 |
| attacks_replay_pqc | 208 | 197 | 3 | 98.50 |
| attacks_sig_fuzz_hmac | 200 | 200 | 0 | 100.00 |
| attacks_sig_fuzz_pqc | 10400 | 200 | 0 | 100.00 |
| attacks_tamper_auth_hmac | 200 | 200 | 0 | 100.00 |
| attacks_tamper_auth_pqc | 10400 | 200 | 0 | 100.00 |
| attacks_tamper_payload_hmac | 200 | 200 | 0 | 100.00 |
| attacks_tamper_payload_pqc | 10400 | 200 | 0 | 100.00 |
| baseline_hmac_r1 | 0 | 0 | 0 | 0.00 |
| baseline_hybrid_r1 | 0 | 0 | 0 | 0.00 |
| baseline_pqc_r1 | 0 | 0 | 0 | 0.00 |
| mixed_bus_can | 0 | 0 | 0 | 0.00 |
| mixed_bus_eth | 0 | 0 | 0 | 0.00 |
| rekey_r1 | 0 | 0 | 0 | 0.00 |
| tput_hmac_canfd_r1 | 0 | 0 | 0 | 0.00 |
| tput_pqc_eth1000_r1 | 0 | 0 | 0 | 0.00 |
| tput_pqc_eth100_r1 | 0 | 0 | 0 | 0.00 |


## 3. Deadline-miss rate per ASIL/deadline class

Source: `summary/deadline_miss.csv`.

| scenario | D1_miss_pct | D2_miss_pct | D3_miss_pct | D4_miss_pct | D5_miss_pct | D6_miss_pct |
| --- | --- | --- | --- | --- | --- | --- |
| attacks_aggregate_hmac | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_aggregate_pqc | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_downgrade_hmac_hmac | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_downgrade_hmac_pqc | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_freshness_rollback_hmac | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_freshness_rollback_pqc | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_mitm_key_confuse_hmac | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_mitm_key_confuse_pqc | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_replay_hmac | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_replay_pqc | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_sig_fuzz_hmac | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_sig_fuzz_pqc | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_tamper_auth_hmac | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_tamper_auth_pqc | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_tamper_payload_hmac | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_tamper_payload_pqc | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| baseline_hmac_r1 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| baseline_hybrid_r1 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| baseline_pqc_r1 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| mixed_bus_can | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| mixed_bus_eth | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| rekey_r1 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| tput_hmac_canfd_r1 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| tput_pqc_eth1000_r1 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| tput_pqc_eth100_r1 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |


## 4. PDU size and fragmentation

Source: `summary/pdu_overhead.csv`.

| scenario | pdu_mean_bytes | pdu_p95_bytes | pdu_max_bytes | fragments_mean | fragments_max |
| --- | --- | --- | --- | --- | --- |
| attacks_aggregate_hmac | 34.0 | 34 | 34 | 1.00 | 1 |
| attacks_aggregate_pqc | 3327.0 | 3327 | 3327 | 52.00 | 52 |
| attacks_downgrade_hmac_hmac | 34.0 | 34 | 34 | 1.00 | 1 |
| attacks_downgrade_hmac_pqc | 3327.0 | 3327 | 3327 | 52.00 | 52 |
| attacks_freshness_rollback_hmac | 34.0 | 34 | 34 | 1.00 | 1 |
| attacks_freshness_rollback_pqc | 3327.0 | 3327 | 3327 | 52.00 | 52 |
| attacks_mitm_key_confuse_hmac | 34.0 | 34 | 34 | 1.00 | 1 |
| attacks_mitm_key_confuse_pqc | 3327.0 | 3327 | 3327 | 52.00 | 52 |
| attacks_replay_hmac | 34.0 | 34 | 34 | 1.00 | 1 |
| attacks_replay_pqc | 3327.0 | 3327 | 3327 | 52.00 | 52 |
| attacks_sig_fuzz_hmac | 34.0 | 34 | 34 | 1.00 | 1 |
| attacks_sig_fuzz_pqc | 3327.0 | 3327 | 3327 | 52.00 | 52 |
| attacks_tamper_auth_hmac | 34.0 | 34 | 34 | 1.00 | 1 |
| attacks_tamper_auth_pqc | 3327.0 | 3327 | 3327 | 52.00 | 52 |
| attacks_tamper_payload_hmac | 34.0 | 34 | 34 | 1.00 | 1 |
| attacks_tamper_payload_pqc | 3327.0 | 3327 | 3327 | 52.00 | 52 |
| baseline_hmac_r1 | 173.1 | 1050 | 1050 | 1.79 | 5 |
| baseline_hybrid_r1 | 3482.1 | 4359 | 4359 | 114.14 | 418 |
| baseline_pqc_r1 | 3466.1 | 4343 | 4343 | 113.57 | 416 |
| mixed_bus_can | 3327.0 | 3327 | 3327 | 52.00 | 52 |
| mixed_bus_eth | 3327.0 | 3327 | 3327 | 52.00 | 52 |
| rekey_r1 | 0.0 | 0 | 0 | 0.00 | 0 |
| tput_hmac_canfd_r1 | 34.0 | 34 | 34 | 1.00 | 1 |
| tput_pqc_eth1000_r1 | 4343.0 | 4343 | 4343 | 3.00 | 3 |
| tput_pqc_eth100_r1 | 4343.0 | 4343 | 4343 | 3.00 | 3 |


## 5. Compliance matrix

See [`compliance_matrix.md`](compliance_matrix.md) for the mapping to AUTOSAR, FIPS, ISO/SAE 21434 and UN R155.

