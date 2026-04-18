# Integrated Simulation Environment — Thesis Report

Generated automatically by `generate_thesis_report.py`.

Number of scenarios collected: **31**.


## 1. Latency and overhead

Raw numbers in `summary/latency_stats.csv`. The table below is copy-pasteable into LaTeX via `pgfplotstable`.

| scenario | samples | e2e_mean_us | e2e_p50_us | e2e_p95_us | e2e_p99_us | e2e_p999_us | auth_mean_us | verify_mean_us | cantp_mean_us | bus_mean_us |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| attacks_aggregate_hmac | 2000 | 2114.03 | 2248.07 | 2311.86 | 2383.61 | 3450.13 | 5.87 | 7.93 | 6.77 | 0.00 |
| attacks_aggregate_pqc | 2000 | 2138.56 | 2189.82 | 2609.42 | 3257.15 | 3547.95 | 182.48 | 74.98 | 113.71 | 0.00 |
| attacks_dos_flood_hmac | 200 | 2157.69 | 2250.05 | 2314.06 | 2428.32 | 2462.13 | 4.78 | 9.11 | 6.79 | 0.00 |
| attacks_dos_flood_pqc | 200 | 2176.52 | 2187.60 | 2647.30 | 3256.88 | 3258.25 | 174.98 | 107.10 | 110.41 | 0.00 |
| attacks_downgrade_hmac_hmac | 200 | 2152.38 | 2246.01 | 2306.76 | 2354.54 | 2359.66 | 4.58 | 8.50 | 6.31 | 0.00 |
| attacks_downgrade_hmac_pqc | 200 | 2168.33 | 2193.87 | 2625.63 | 3284.29 | 3290.34 | 173.08 | 102.43 | 112.92 | 0.00 |
| attacks_freshness_rollback_hmac | 200 | 2110.45 | 2242.12 | 2302.74 | 2334.17 | 2383.72 | 8.78 | 0.00 | 6.67 | 0.00 |
| attacks_freshness_rollback_pqc | 200 | 2173.22 | 2205.37 | 2573.09 | 3247.75 | 3256.44 | 220.44 | 0.00 | 119.92 | 0.00 |
| attacks_harvest_now_hmac | 200 | 2102.06 | 2242.58 | 2307.66 | 2344.96 | 2376.05 | 4.83 | 8.64 | 7.16 | 0.00 |
| attacks_harvest_now_pqc | 200 | 2218.62 | 2187.99 | 2595.41 | 3248.59 | 3275.65 | 173.00 | 107.46 | 103.25 | 0.00 |
| attacks_mitm_key_confuse_hmac | 200 | 2084.12 | 2246.36 | 2296.64 | 2334.43 | 2334.44 | 4.44 | 9.10 | 7.13 | 0.00 |
| attacks_mitm_key_confuse_pqc | 200 | 2054.31 | 2176.62 | 2614.79 | 3261.75 | 3295.32 | 191.95 | 40.95 | 145.64 | 0.00 |
| attacks_replay_hmac | 200 | 2158.53 | 2257.49 | 2332.37 | 2354.87 | 2378.21 | 13.24 | 8.91 | 6.54 | 0.00 |
| attacks_replay_pqc | 200 | 2105.20 | 2190.73 | 2557.75 | 2622.01 | 3190.83 | 163.47 | 98.74 | 76.77 | 0.00 |
| attacks_sig_fuzz_hmac | 200 | 2079.79 | 2249.24 | 2297.16 | 2345.09 | 2443.97 | 4.43 | 8.93 | 6.84 | 0.00 |
| attacks_sig_fuzz_pqc | 200 | 2157.90 | 2205.13 | 2602.33 | 3257.51 | 3264.13 | 189.09 | 42.41 | 119.83 | 0.00 |
| attacks_tamper_auth_hmac | 200 | 2104.94 | 2247.66 | 2302.96 | 2346.72 | 2395.99 | 4.53 | 8.55 | 6.33 | 0.00 |
| attacks_tamper_auth_pqc | 200 | 2049.40 | 2176.12 | 2564.85 | 2820.19 | 3130.03 | 190.55 | 72.19 | 124.03 | 0.00 |
| attacks_tamper_payload_hmac | 200 | 2025.88 | 2251.59 | 2323.81 | 2383.61 | 2442.11 | 4.43 | 8.52 | 7.03 | 0.00 |
| attacks_tamper_payload_pqc | 200 | 2103.54 | 2188.77 | 2621.26 | 3232.90 | 3279.16 | 155.87 | 73.50 | 115.23 | 0.00 |
| attacks_timing_probe_hmac | 200 | 2164.49 | 2248.80 | 2313.39 | 2545.82 | 2552.23 | 4.70 | 9.08 | 6.95 | 0.00 |
| attacks_timing_probe_pqc | 200 | 2178.57 | 2191.28 | 2622.31 | 3367.04 | 3439.64 | 192.35 | 105.02 | 109.06 | 0.00 |
| baseline_hmac_r1 | 2800 | 1966.65 | 2241.57 | 2307.20 | 2369.63 | 2496.53 | 5.91 | 9.00 | 7.47 | 0.00 |
| baseline_hybrid_r1 | 2800 | 2145.53 | 2195.14 | 2531.47 | 2734.86 | 3409.51 | 172.68 | 114.74 | 28.06 | 0.00 |
| baseline_pqc_r1 | 2800 | 2153.03 | 2204.45 | 2534.88 | 2613.00 | 3318.59 | 164.13 | 106.56 | 27.50 | 0.00 |
| mixed_bus_can | 200 | 2130.03 | 2189.30 | 2556.52 | 2655.74 | 2745.13 | 216.68 | 106.52 | 54.16 | 0.00 |
| mixed_bus_eth | 200 | 3827.29 | 3551.07 | 5667.85 | 5894.60 | 5898.99 | 209.06 | 96.33 | 52.79 | 0.00 |
| rekey_r1 | 200 | 53.43 | 50.48 | 74.38 | 84.17 | 87.65 | 23.33 | 16.83 | 12.97 | 0.00 |
| tput_hmac_canfd_r1 | 946 | 2114.29 | 2237.76 | 2296.27 | 2355.35 | 2707.55 | 6.24 | 8.02 | 5.22 | 0.00 |
| tput_pqc_eth1000_r1 | 1463 | 1367.10 | 1383.78 | 2134.47 | 2217.70 | 3189.53 | 185.60 | 106.65 | 11.93 | 0.00 |
| tput_pqc_eth100_r1 | 899 | 2226.24 | 2199.79 | 2499.36 | 2550.34 | 3424.85 | 170.20 | 116.98 | 12.39 | 0.00 |


## 2. Attack detection (UN R155 Annex 5 mapping)

Source: `summary/attack_detection.csv`.

| scenario | attacks_injected | attacks_detected | attacks_delivered | detection_rate_pct |
| --- | --- | --- | --- | --- |
| attacks_aggregate_hmac | 1997 | 1003 | 997 | 50.15 |
| attacks_aggregate_pqc | 93806 | 1397 | 603 | 69.85 |
| attacks_dos_flood_hmac | 200 | 0 | 200 | 0.00 |
| attacks_dos_flood_pqc | 10400 | 0 | 200 | 0.00 |
| attacks_downgrade_hmac_hmac | 200 | 0 | 200 | 0.00 |
| attacks_downgrade_hmac_pqc | 10400 | 200 | 0 | 100.00 |
| attacks_freshness_rollback_hmac | 200 | 200 | 0 | 100.00 |
| attacks_freshness_rollback_pqc | 10400 | 200 | 0 | 100.00 |
| attacks_harvest_now_hmac | 200 | 0 | 200 | 0.00 |
| attacks_harvest_now_pqc | 10400 | 0 | 200 | 0.00 |
| attacks_mitm_key_confuse_hmac | 200 | 200 | 0 | 100.00 |
| attacks_mitm_key_confuse_pqc | 10400 | 200 | 0 | 100.00 |
| attacks_replay_hmac | 197 | 3 | 197 | 1.50 |
| attacks_replay_pqc | 206 | 197 | 3 | 98.50 |
| attacks_sig_fuzz_hmac | 200 | 200 | 0 | 100.00 |
| attacks_sig_fuzz_pqc | 10400 | 200 | 0 | 100.00 |
| attacks_tamper_auth_hmac | 200 | 200 | 0 | 100.00 |
| attacks_tamper_auth_pqc | 10400 | 200 | 0 | 100.00 |
| attacks_tamper_payload_hmac | 200 | 200 | 0 | 100.00 |
| attacks_tamper_payload_pqc | 10400 | 200 | 0 | 100.00 |
| attacks_timing_probe_hmac | 200 | 0 | 200 | 0.00 |
| attacks_timing_probe_pqc | 10400 | 0 | 200 | 0.00 |
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
| attacks_dos_flood_hmac | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_dos_flood_pqc | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_downgrade_hmac_hmac | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_downgrade_hmac_pqc | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_freshness_rollback_hmac | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_freshness_rollback_pqc | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_harvest_now_hmac | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_harvest_now_pqc | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
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
| attacks_timing_probe_hmac | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_timing_probe_pqc | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
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
| attacks_dos_flood_hmac | 34.0 | 34 | 34 | 1.00 | 1 |
| attacks_dos_flood_pqc | 3327.0 | 3327 | 3327 | 52.00 | 52 |
| attacks_downgrade_hmac_hmac | 34.0 | 34 | 34 | 1.00 | 1 |
| attacks_downgrade_hmac_pqc | 3327.0 | 3327 | 3327 | 52.00 | 52 |
| attacks_freshness_rollback_hmac | 34.0 | 34 | 34 | 1.00 | 1 |
| attacks_freshness_rollback_pqc | 3327.0 | 3327 | 3327 | 52.00 | 52 |
| attacks_harvest_now_hmac | 34.0 | 34 | 34 | 1.00 | 1 |
| attacks_harvest_now_pqc | 3327.0 | 3327 | 3327 | 52.00 | 52 |
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
| attacks_timing_probe_hmac | 34.0 | 34 | 34 | 1.00 | 1 |
| attacks_timing_probe_pqc | 3327.0 | 3327 | 3327 | 52.00 | 52 |
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

