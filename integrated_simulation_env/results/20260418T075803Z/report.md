# Integrated Simulation Environment — Thesis Report

Generated automatically by `generate_thesis_report.py`.

Number of scenarios collected: **20**.


## 1. Latency and overhead

Raw numbers in `summary/latency_stats.csv`. The table below is copy-pasteable into LaTeX via `pgfplotstable`.

| scenario | samples | e2e_mean_us | e2e_p50_us | e2e_p95_us | e2e_p99_us | e2e_p999_us | auth_mean_us | verify_mean_us | cantp_mean_us | bus_mean_us |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| attacks_aggregate | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_dos_flood | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 4.55 | 10.76 | 10.94 | 0.00 |
| attacks_downgrade_hmac | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 4.65 | 11.15 | 10.44 | 0.00 |
| attacks_freshness_rollback | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 11.41 | 0.00 | 10.62 | 0.00 |
| attacks_harvest_now | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 4.90 | 11.20 | 10.89 | 0.00 |
| attacks_mitm_key_confuse | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 4.88 | 11.69 | 9.86 | 0.00 |
| attacks_replay | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 16.61 | 7.52 | 9.88 | 0.00 |
| attacks_sig_fuzz | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 4.85 | 10.94 | 9.23 | 0.00 |
| attacks_tamper_auth | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 4.87 | 11.33 | 10.51 | 0.00 |
| attacks_tamper_payload | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 4.76 | 11.25 | 10.55 | 0.00 |
| attacks_timing_probe | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 4.57 | 11.33 | 10.63 | 0.00 |
| baseline_hmac_r1 | 2800 | 1948.97 | 2294.26 | 2404.07 | 2509.53 | 3435.50 | 6.80 | 12.74 | 13.10 | 0.00 |
| baseline_hybrid_r1 | 2800 | 2149.89 | 2210.28 | 3291.54 | 3374.28 | 3798.37 | 178.67 | 83.32 | 50.94 | 0.00 |
| baseline_pqc_r1 | 2800 | 2114.04 | 2213.75 | 3254.29 | 3355.07 | 3590.82 | 188.86 | 77.60 | 55.89 | 0.00 |
| mixed_bus_can | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 185.71 | 75.93 | 127.41 | 0.00 |
| mixed_bus_eth | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 167.15 | 0.00 | 22.45 | 0.00 |
| rekey_r1 | 200 | 76.72 | 73.51 | 98.92 | 121.52 | 126.17 | 30.04 | 28.65 | 17.65 | 0.00 |
| tput_hmac_canfd_r1 | 938 | 2133.82 | 2291.56 | 2361.82 | 2407.64 | 4087.17 | 7.23 | 12.01 | 10.39 | 0.00 |
| tput_pqc_eth1000_r1 | 1432 | 1396.39 | 1392.49 | 1732.15 | 2344.32 | 3401.20 | 173.74 | 80.21 | 21.01 | 0.00 |
| tput_pqc_eth100_r1 | 890 | 2249.08 | 2241.04 | 2532.90 | 2603.05 | 2741.24 | 171.77 | 78.71 | 23.10 | 0.00 |


## 2. Attack detection (UN R155 Annex 5 mapping)

Source: `summary/attack_detection.csv`.

| scenario | attacks_injected | attacks_detected | attacks_delivered | detection_rate_pct |
| --- | --- | --- | --- | --- |
| attacks_aggregate | 1462 | 662 | 1338 | 45.28 |
| attacks_dos_flood | 200 | 0 | 200 | 0.00 |
| attacks_downgrade_hmac | 200 | 0 | 200 | 0.00 |
| attacks_freshness_rollback | 200 | 200 | 0 | 100.00 |
| attacks_harvest_now | 200 | 0 | 200 | 0.00 |
| attacks_mitm_key_confuse | 0 | 0 | 200 | 0.00 |
| attacks_replay | 62 | 62 | 138 | 100.00 |
| attacks_sig_fuzz | 0 | 0 | 200 | 0.00 |
| attacks_tamper_auth | 200 | 200 | 0 | 100.00 |
| attacks_tamper_payload | 200 | 200 | 0 | 100.00 |
| attacks_timing_probe | 200 | 0 | 200 | 0.00 |
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
| attacks_aggregate | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_dos_flood | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_downgrade_hmac | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_freshness_rollback | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_harvest_now | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_mitm_key_confuse | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_replay | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_sig_fuzz | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_tamper_auth | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_tamper_payload | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_timing_probe | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| baseline_hmac_r1 | 0.17 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| baseline_hybrid_r1 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| baseline_pqc_r1 | 0.17 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
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
| attacks_aggregate | 0.0 | 0 | 0 | 0.00 | 0 |
| attacks_dos_flood | 34.0 | 34 | 34 | 1.00 | 1 |
| attacks_downgrade_hmac | 34.0 | 34 | 34 | 1.00 | 1 |
| attacks_freshness_rollback | 34.0 | 34 | 34 | 1.00 | 1 |
| attacks_harvest_now | 34.0 | 34 | 34 | 1.00 | 1 |
| attacks_mitm_key_confuse | 34.0 | 34 | 34 | 1.00 | 1 |
| attacks_replay | 34.0 | 34 | 34 | 1.00 | 1 |
| attacks_sig_fuzz | 34.0 | 34 | 34 | 1.00 | 1 |
| attacks_tamper_auth | 34.0 | 34 | 34 | 1.00 | 1 |
| attacks_tamper_payload | 34.0 | 34 | 34 | 1.00 | 1 |
| attacks_timing_probe | 34.0 | 34 | 34 | 1.00 | 1 |
| baseline_hmac_r1 | 173.1 | 1050 | 1050 | 1.79 | 5 |
| baseline_hybrid_r1 | 3482.1 | 4359 | 4359 | 114.14 | 418 |
| baseline_pqc_r1 | 3466.1 | 4343 | 4343 | 113.57 | 416 |
| mixed_bus_can | 3327.0 | 3327 | 3327 | 52.00 | 52 |
| mixed_bus_eth | 3327.0 | 3327 | 3327 | 3.00 | 3 |
| rekey_r1 | 0.0 | 0 | 0 | 0.00 | 0 |
| tput_hmac_canfd_r1 | 34.0 | 34 | 34 | 1.00 | 1 |
| tput_pqc_eth1000_r1 | 4343.0 | 4343 | 4343 | 3.00 | 3 |
| tput_pqc_eth100_r1 | 4343.0 | 4343 | 4343 | 3.00 | 3 |


## 5. Compliance matrix

See [`compliance_matrix.md`](compliance_matrix.md) for the mapping to AUTOSAR, FIPS, ISO/SAE 21434 and UN R155.

