# Integrated Simulation Environment — Thesis Report

Generated automatically by `generate_thesis_report.py`.

Number of scenarios collected: **25**.


## 1. Latency and overhead

Raw numbers in `summary/latency_stats.csv`. The table below is copy-pasteable into LaTeX via `pgfplotstable`.

| scenario | samples | e2e_mean_us | e2e_p50_us | e2e_p95_us | e2e_p99_us | e2e_p999_us | auth_mean_us | verify_mean_us | cantp_mean_us | bus_mean_us |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| attacks_aggregate_hmac | 700 | 2134.84 | 2277.90 | 2360.88 | 2474.41 | 3878.18 | 5.45 | 6.68 | 0.10 | 0.00 |
| attacks_aggregate_pqc | 700 | 1924.69 | 2153.23 | 2588.47 | 3261.16 | 3343.47 | 132.72 | 23.58 | 2.30 | 0.00 |
| attacks_downgrade_hmac_hmac | 100 | 2054.58 | 2272.75 | 2343.80 | 2398.21 | 2398.21 | 2.37 | 7.54 | 0.10 | 7.83 |
| attacks_downgrade_hmac_pqc | 100 | 1975.48 | 2168.78 | 2568.03 | 3203.82 | 3203.82 | 127.58 | 37.46 | 2.39 | 179.34 |
| attacks_freshness_rollback_hmac | 100 | 2181.85 | 2273.54 | 2357.73 | 2439.23 | 2439.23 | 7.88 | 0.00 | 0.10 | 9.15 |
| attacks_freshness_rollback_pqc | 100 | 1945.09 | 2176.50 | 2587.59 | 3199.32 | 3199.32 | 128.85 | 0.00 | 2.31 | 180.55 |
| attacks_mitm_key_confuse_hmac | 100 | 2189.33 | 2275.42 | 2333.52 | 2381.79 | 2381.79 | 2.09 | 6.60 | 0.10 | 7.58 |
| attacks_mitm_key_confuse_pqc | 100 | 1916.38 | 2166.19 | 2585.47 | 2654.36 | 2654.36 | 139.74 | 16.68 | 2.27 | 201.68 |
| attacks_replay_hmac | 100 | 2120.09 | 2272.65 | 2374.72 | 3611.84 | 3611.84 | 18.84 | 6.19 | 0.10 | 6.84 |
| attacks_replay_pqc | 100 | 1791.68 | 1554.28 | 2611.28 | 3288.77 | 3288.77 | 130.73 | 35.27 | 2.30 | 130.71 |
| attacks_sig_fuzz_hmac | 100 | 2161.67 | 2289.60 | 2370.91 | 2474.27 | 2474.27 | 2.30 | 8.94 | 0.10 | 9.43 |
| attacks_sig_fuzz_pqc | 100 | 1948.75 | 2134.75 | 2649.39 | 3321.76 | 3321.76 | 128.99 | 15.77 | 2.27 | 177.16 |
| attacks_tamper_auth_hmac | 100 | 2166.61 | 2284.17 | 2360.88 | 2474.41 | 2474.41 | 2.30 | 8.75 | 0.11 | 9.95 |
| attacks_tamper_auth_pqc | 100 | 1923.86 | 2134.77 | 2561.60 | 3254.78 | 3254.78 | 136.70 | 28.88 | 2.29 | 181.87 |
| attacks_tamper_payload_hmac | 100 | 2069.73 | 2278.88 | 2366.60 | 2407.57 | 2407.57 | 2.35 | 8.71 | 0.11 | 9.34 |
| attacks_tamper_payload_pqc | 100 | 1971.56 | 2166.06 | 2586.65 | 2799.56 | 2799.56 | 136.47 | 30.99 | 2.28 | 176.95 |
| baseline_hmac_r1 | 1400 | 1965.87 | 2257.06 | 2334.66 | 2397.51 | 2537.67 | 3.41 | 5.58 | 0.14 | 7.85 |
| baseline_hybrid_r1 | 1400 | 2284.17 | 2395.18 | 3305.11 | 3378.71 | 3467.78 | 135.61 | 40.93 | 4.83 | 47.55 |
| baseline_pqc_r1 | 1400 | 2205.91 | 2371.88 | 3285.18 | 3354.82 | 3422.21 | 130.79 | 34.98 | 4.82 | 48.50 |
| mixed_bus_can | 100 | 1796.60 | 1544.91 | 2579.45 | 2659.91 | 2659.91 | 127.39 | 36.53 | 2.28 | 119.82 |
| mixed_bus_eth | 100 | 2892.86 | 2628.61 | 3671.26 | 4254.74 | 4254.74 | 135.17 | 34.58 | 2.29 | 124.52 |
| rekey_r1 | 100 | 54.23 | 49.06 | 70.17 | 129.60 | 129.60 | 24.32 | 16.80 | 12.73 | 0.00 |
| tput_hmac_canfd_r1 | 928 | 2155.96 | 2274.36 | 2360.42 | 2437.93 | 2511.06 | 3.88 | 5.83 | 0.10 | 5.68 |
| tput_pqc_eth1000_r1 | 1513 | 1322.43 | 1315.45 | 1471.45 | 2190.72 | 2428.20 | 129.47 | 37.43 | 0.20 | 13.08 |
| tput_pqc_eth100_r1 | 868 | 2303.95 | 2398.60 | 2510.23 | 2549.00 | 2646.49 | 130.12 | 37.22 | 0.20 | 15.17 |


## 2. Attack detection (UN R155 Annex 5 mapping)

Source: `summary/attack_detection.csv`.

| scenario | attacks_injected | attacks_detected | attacks_delivered | detection_rate_pct |
| --- | --- | --- | --- | --- |
| attacks_aggregate_hmac | 697 | 503 | 197 | 71.86 |
| attacks_aggregate_pqc | 31306 | 697 | 3 | 99.57 |
| attacks_downgrade_hmac_hmac | 100 | 0 | 100 | 0.00 |
| attacks_downgrade_hmac_pqc | 5200 | 100 | 0 | 100.00 |
| attacks_freshness_rollback_hmac | 100 | 100 | 0 | 100.00 |
| attacks_freshness_rollback_pqc | 5200 | 100 | 0 | 100.00 |
| attacks_mitm_key_confuse_hmac | 100 | 100 | 0 | 100.00 |
| attacks_mitm_key_confuse_pqc | 5200 | 100 | 0 | 100.00 |
| attacks_replay_hmac | 97 | 3 | 97 | 3.00 |
| attacks_replay_pqc | 106 | 97 | 3 | 97.00 |
| attacks_sig_fuzz_hmac | 100 | 100 | 0 | 100.00 |
| attacks_sig_fuzz_pqc | 5200 | 100 | 0 | 100.00 |
| attacks_tamper_auth_hmac | 100 | 100 | 0 | 100.00 |
| attacks_tamper_auth_pqc | 5200 | 100 | 0 | 100.00 |
| attacks_tamper_payload_hmac | 100 | 100 | 0 | 100.00 |
| attacks_tamper_payload_pqc | 5200 | 100 | 0 | 100.00 |
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


## 5. Per-layer AUTOSAR latency breakdown

Source: `summary/layer_latency.csv` and `summary/flow_timeline.md`. Splits the end-to-end latency into Com/PduR, SecOC build, Csm sign, CanTP fragmentation, bus transit and Csm verify.

| scenario | samples | csm_sign_mean_us | csm_sign_p99_us | cantp_frag_mean_us | cantp_frag_p99_us | bus_transit_mean_us | bus_transit_p99_us | csm_verify_mean_us | csm_verify_p99_us | com_pdur_secoc_mean_us | e2e_mean_us | e2e_p99_us |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| attacks_aggregate_hmac | 700 | 5.45 | 13.52 | 0.10 | 0.16 | 0.00 | 0.00 | 6.68 | 23.61 | 2122.61 | 2134.84 | 2474.41 |
| attacks_aggregate_pqc | 700 | 132.72 | 357.24 | 2.30 | 2.66 | 0.00 | 0.00 | 23.58 | 54.63 | 1766.08 | 1924.69 | 3261.16 |
| attacks_downgrade_hmac_hmac | 100 | 2.37 | 11.48 | 0.10 | 0.14 | 7.83 | 21.79 | 7.54 | 21.59 | 2036.74 | 2054.58 | 2398.21 |
| attacks_downgrade_hmac_pqc | 100 | 127.58 | 365.98 | 2.39 | 2.71 | 179.34 | 410.18 | 37.46 | 57.27 | 1628.71 | 1975.48 | 3203.82 |
| attacks_freshness_rollback_hmac | 100 | 7.88 | 21.49 | 0.10 | 0.14 | 9.15 | 83.31 | 0.00 | 0.00 | 2164.72 | 2181.85 | 2439.23 |
| attacks_freshness_rollback_pqc | 100 | 128.85 | 293.54 | 2.31 | 2.70 | 180.55 | 366.46 | 0.00 | 0.00 | 1633.39 | 1945.09 | 3199.32 |
| attacks_mitm_key_confuse_hmac | 100 | 2.09 | 4.78 | 0.10 | 0.13 | 7.58 | 14.43 | 6.60 | 14.93 | 2172.97 | 2189.33 | 2381.79 |
| attacks_mitm_key_confuse_pqc | 100 | 139.74 | 349.24 | 2.27 | 2.53 | 201.68 | 404.27 | 16.68 | 25.20 | 1556.00 | 1916.38 | 2654.36 |
| attacks_replay_hmac | 100 | 18.84 | 4.29 | 0.10 | 0.16 | 6.84 | 14.88 | 6.19 | 16.61 | 2088.11 | 2120.09 | 3611.84 |
| attacks_replay_pqc | 100 | 130.73 | 288.21 | 2.30 | 2.64 | 130.71 | 296.80 | 35.27 | 58.57 | 1492.68 | 1791.68 | 3288.77 |
| attacks_sig_fuzz_hmac | 100 | 2.30 | 10.79 | 0.10 | 0.16 | 9.43 | 24.70 | 8.94 | 24.90 | 2140.90 | 2161.67 | 2474.27 |
| attacks_sig_fuzz_pqc | 100 | 128.99 | 308.61 | 2.27 | 2.64 | 177.16 | 365.34 | 15.77 | 25.23 | 1624.55 | 1948.75 | 3321.76 |
| attacks_tamper_auth_hmac | 100 | 2.30 | 4.66 | 0.11 | 0.18 | 9.95 | 33.12 | 8.75 | 21.22 | 2145.51 | 2166.61 | 2474.41 |
| attacks_tamper_auth_pqc | 100 | 136.70 | 301.98 | 2.29 | 2.54 | 181.87 | 399.34 | 28.88 | 50.99 | 1574.11 | 1923.86 | 3254.78 |
| attacks_tamper_payload_hmac | 100 | 2.35 | 6.32 | 0.11 | 0.25 | 9.34 | 23.79 | 8.71 | 24.57 | 2049.22 | 2069.73 | 2407.57 |
| attacks_tamper_payload_pqc | 100 | 136.47 | 419.42 | 2.28 | 2.66 | 176.95 | 407.75 | 30.99 | 50.12 | 1624.87 | 1971.56 | 2799.56 |
| baseline_hmac_r1 | 1400 | 3.41 | 12.63 | 0.14 | 0.34 | 7.85 | 27.65 | 5.58 | 17.17 | 1948.88 | 1965.87 | 2397.51 |
| baseline_hybrid_r1 | 1400 | 135.61 | 372.26 | 4.83 | 19.75 | 47.55 | 489.62 | 40.93 | 70.37 | 2055.25 | 2284.17 | 3378.71 |
| baseline_pqc_r1 | 1400 | 130.79 | 347.02 | 4.82 | 20.27 | 48.50 | 494.56 | 34.98 | 58.67 | 1986.83 | 2205.91 | 3354.82 |
| mixed_bus_can | 100 | 127.39 | 432.22 | 2.28 | 2.73 | 119.82 | 244.74 | 36.53 | 59.44 | 1510.58 | 1796.60 | 2659.91 |
| mixed_bus_eth | 100 | 135.17 | 286.21 | 2.29 | 2.69 | 124.52 | 251.51 | 34.58 | 50.36 | 2596.30 | 2892.86 | 4254.74 |
| rekey_r1 | 100 | 24.32 | 63.32 | 12.73 | 20.45 | 0.00 | 0.00 | 16.80 | 51.91 | 0.37 | 54.23 | 129.60 |
| tput_hmac_canfd_r1 | 928 | 3.88 | 12.34 | 0.10 | 0.15 | 5.68 | 18.88 | 5.83 | 15.68 | 2140.48 | 2155.96 | 2437.93 |
| tput_pqc_eth1000_r1 | 1513 | 129.47 | 343.52 | 0.20 | 0.29 | 13.08 | 105.96 | 37.43 | 64.10 | 1142.25 | 1322.43 | 2190.72 |
| tput_pqc_eth100_r1 | 868 | 130.12 | 364.07 | 0.20 | 0.29 | 15.17 | 120.91 | 37.22 | 59.09 | 2121.24 | 2303.95 | 2549.00 |


## 6. Compliance constraints (time / signal / security)

See [`compliance_constraints.md`](compliance_constraints.md) for the per-scenario pass/fail matrix covering ASIL deadlines, PDU size constraints and AUTOSAR SWS_SecOC clauses.


## 7. Standards compliance matrix

See [`compliance_matrix.md`](compliance_matrix.md) for the mapping to AUTOSAR, FIPS, ISO/SAE 21434 and UN R155.

