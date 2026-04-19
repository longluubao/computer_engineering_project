# Integrated Simulation Environment — Thesis Report

Generated automatically by `generate_thesis_report.py`.

Number of scenarios collected: **52**.


## 1. Latency and overhead

Raw numbers in `summary/latency_stats.csv`. The table below is copy-pasteable into LaTeX via `pgfplotstable`.

| scenario | samples | e2e_mean_us | e2e_p50_us | e2e_p95_us | e2e_p99_us | e2e_p999_us | auth_mean_us | verify_mean_us | cantp_mean_us | bus_mean_us |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| attacks_aggregate_hmac | 350 | 2118.40 | 2341.43 | 2482.26 | 2579.27 | 2704.25 | 11.04 | 9.16 | 0.13 | 0.00 |
| attacks_aggregate_hybrid | 350 | 1838.35 | 2143.09 | 2311.53 | 2620.36 | 3473.01 | 161.21 | 22.58 | 2.17 | 0.00 |
| attacks_aggregate_pqc | 350 | 1763.06 | 1617.85 | 2312.82 | 3194.08 | 3336.16 | 139.27 | 20.96 | 2.06 | 0.00 |
| attacks_downgrade_hmac_hmac | 50 | 2144.49 | 2359.76 | 2494.70 | 2543.30 | 2543.30 | 2.86 | 14.60 | 0.10 | 13.36 |
| attacks_downgrade_hmac_hybrid | 50 | 1955.71 | 2168.61 | 2273.36 | 2292.67 | 2292.67 | 172.42 | 16.42 | 2.05 | 276.44 |
| attacks_downgrade_hmac_pqc | 50 | 1834.91 | 2156.26 | 2289.09 | 2340.63 | 2340.63 | 144.06 | 38.89 | 2.14 | 256.66 |
| attacks_freshness_rollback_hmac | 50 | 2040.83 | 2304.84 | 2426.66 | 2474.02 | 2474.02 | 12.13 | 0.00 | 0.09 | 14.12 |
| attacks_freshness_rollback_hybrid | 50 | 1901.49 | 2190.80 | 2306.52 | 2620.36 | 2620.36 | 165.10 | 0.00 | 2.50 | 275.45 |
| attacks_freshness_rollback_pqc | 50 | 1864.46 | 2139.62 | 2316.00 | 2405.85 | 2405.85 | 158.28 | 0.00 | 2.12 | 288.50 |
| attacks_mitm_key_confuse_hmac | 50 | 2089.12 | 2343.47 | 2461.27 | 2540.24 | 2540.24 | 2.29 | 13.62 | 0.10 | 13.06 |
| attacks_mitm_key_confuse_hybrid | 50 | 1878.54 | 2156.39 | 2314.21 | 2441.60 | 2441.60 | 181.19 | 15.07 | 2.03 | 297.53 |
| attacks_mitm_key_confuse_pqc | 50 | 1752.64 | 1988.34 | 2299.80 | 2324.29 | 2324.29 | 136.75 | 20.40 | 2.01 | 278.08 |
| attacks_replay_hmac | 50 | 2178.96 | 2332.54 | 2520.57 | 2667.79 | 2667.79 | 52.64 | 1.07 | 0.10 | 14.83 |
| attacks_replay_hybrid | 50 | 1652.92 | 1534.58 | 2285.20 | 2286.68 | 2286.68 | 195.15 | 3.95 | 2.10 | 200.69 |
| attacks_replay_pqc | 50 | 1614.25 | 1533.62 | 2269.83 | 2312.82 | 2312.82 | 141.23 | 2.33 | 1.99 | 182.80 |
| attacks_sig_fuzz_hmac | 50 | 2063.47 | 2365.71 | 2466.97 | 2545.58 | 2545.58 | 2.69 | 15.47 | 0.10 | 19.12 |
| attacks_sig_fuzz_hybrid | 50 | 1797.70 | 1995.55 | 2285.80 | 2301.46 | 2301.46 | 139.37 | 39.45 | 2.22 | 309.31 |
| attacks_sig_fuzz_pqc | 50 | 1697.50 | 1573.42 | 2260.89 | 2271.22 | 2271.22 | 126.39 | 20.59 | 2.01 | 261.84 |
| attacks_tamper_auth_hmac | 50 | 2099.59 | 2319.52 | 2402.78 | 2458.95 | 2458.95 | 2.27 | 9.68 | 0.10 | 11.86 |
| attacks_tamper_auth_hybrid | 50 | 1765.64 | 1688.54 | 2285.16 | 2313.27 | 2313.27 | 141.90 | 48.08 | 2.05 | 274.60 |
| attacks_tamper_auth_pqc | 50 | 1887.64 | 2060.86 | 2582.71 | 3308.05 | 3308.05 | 135.08 | 30.57 | 2.20 | 239.21 |
| attacks_tamper_payload_hmac | 50 | 2212.37 | 2334.88 | 2432.17 | 2470.07 | 2470.07 | 2.38 | 9.69 | 0.30 | 8.96 |
| attacks_tamper_payload_hybrid | 50 | 1916.47 | 2177.03 | 2346.48 | 2361.59 | 2361.59 | 133.35 | 35.10 | 2.22 | 263.99 |
| attacks_tamper_payload_pqc | 50 | 1690.00 | 1565.89 | 2287.69 | 2311.43 | 2311.43 | 133.08 | 33.97 | 1.99 | 240.64 |
| baseline_hmac_r1 | 700 | 1902.17 | 2308.75 | 2487.74 | 2551.06 | 2677.55 | 5.03 | 12.86 | 0.13 | 13.90 |
| baseline_hybrid_r1 | 700 | 2057.86 | 2214.34 | 3312.75 | 3443.18 | 3687.80 | 156.63 | 55.24 | 4.29 | 84.66 |
| baseline_pqc_r1 | 700 | 2044.57 | 2222.76 | 3297.99 | 3468.51 | 3782.32 | 147.80 | 40.92 | 4.38 | 83.96 |
| bus_failure_aggregate | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| bus_failure_clean | 100 | 2106.24 | 2356.20 | 2476.91 | 2593.16 | 2593.16 | 19.57 | 16.18 | 0.10 | 13.08 |
| bus_failure_heavy | 100 | 2048.49 | 2324.55 | 2457.91 | 2480.35 | 2480.35 | 2.27 | 10.96 | 0.09 | 10.09 |
| bus_failure_noisy | 100 | 2123.53 | 2344.43 | 2460.77 | 2516.33 | 2516.33 | 2.35 | 13.56 | 0.09 | 11.00 |
| deadline_relaxed | 50 | 1260.89 | 1233.98 | 1308.79 | 1346.86 | 1346.86 | 3.03 | 15.38 | 0.11 | 12.97 |
| deadline_tight | 50 | 50897.02 | 50888.06 | 51272.02 | 51429.06 | 51429.06 | 69.91 | 0.00 | 185.28 | 0.00 |
| keymismatch_aggregate | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| keymismatch_mismatch | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 2.16 | 17.50 | 0.15 | 16.35 |
| keymismatch_shared | 50 | 2138.76 | 2371.64 | 2480.62 | 2582.22 | 2582.22 | 41.91 | 17.26 | 0.72 | 14.47 |
| mixed_bus_can | 50 | 1716.02 | 1520.89 | 2596.73 | 3280.70 | 3280.70 | 155.47 | 41.17 | 2.11 | 181.63 |
| mixed_bus_eth | 50 | 2915.67 | 2676.79 | 4410.94 | 4516.90 | 4516.90 | 232.14 | 40.07 | 2.45 | 176.52 |
| multi_ecu_aggregate | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| multi_ecu_rx0 | 50 | 2098.29 | 2331.98 | 2410.03 | 2459.63 | 2459.63 | 0.00 | 16.44 | 0.00 | 0.00 |
| multi_ecu_rx1 | 50 | 2102.55 | 2362.71 | 2455.19 | 2477.26 | 2477.26 | 0.00 | 16.51 | 0.00 | 0.00 |
| multi_ecu_rx2 | 50 | 2177.94 | 2343.88 | 2474.36 | 2480.99 | 2480.99 | 0.00 | 16.14 | 0.00 | 0.00 |
| multi_ecu_tx | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 16.31 | 0.00 | 0.11 | 14.46 |
| persistence_aggregate | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| persistence_no_nvm | 50 | 2201.55 | 2315.01 | 2432.16 | 2452.05 | 2452.05 | 17.53 | 13.80 | 0.11 | 9.78 |
| persistence_with_nvm | 50 | 2104.36 | 2362.83 | 2467.18 | 2487.42 | 2487.42 | 5.14 | 13.37 | 0.11 | 14.11 |
| rekey_r1 | 50 | 50.60 | 46.62 | 65.00 | 66.75 | 66.75 | 23.14 | 15.38 | 11.76 | 0.00 |
| rollover_hmac_r1 | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 63.30 | 12.85 | 0.11 | 13.82 |
| rollover_pqc_r1 | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 145.97 | 31.87 | 1.98 | 183.45 |
| tput_hmac_canfd_r1 | 940 | 2129.38 | 2347.08 | 2503.09 | 2619.26 | 3583.31 | 4.47 | 14.98 | 0.11 | 13.80 |
| tput_pqc_eth1000_r1 | 1429 | 1399.68 | 1406.94 | 1621.06 | 2367.36 | 3376.32 | 155.50 | 43.56 | 0.20 | 22.77 |
| tput_pqc_eth100_r1 | 897 | 2230.70 | 2321.57 | 2627.10 | 2697.16 | 2830.99 | 156.40 | 44.48 | 0.21 | 24.72 |


## 2. Attack detection (UN R155 Annex 5 mapping)

Source: `summary/attack_detection.csv`.

| scenario | attacks_injected | attacks_detected | attacks_delivered | detection_rate_pct |
| --- | --- | --- | --- | --- |
| attacks_aggregate_hmac | 347 | 297 | 53 | 84.86 |
| attacks_aggregate_hybrid | 15957 | 347 | 3 | 99.14 |
| attacks_aggregate_pqc | 15657 | 347 | 3 | 99.14 |
| attacks_downgrade_hmac_hmac | 50 | 0 | 50 | 0.00 |
| attacks_downgrade_hmac_hybrid | 2650 | 50 | 0 | 100.00 |
| attacks_downgrade_hmac_pqc | 2600 | 50 | 0 | 100.00 |
| attacks_freshness_rollback_hmac | 50 | 50 | 0 | 100.00 |
| attacks_freshness_rollback_hybrid | 2650 | 50 | 0 | 100.00 |
| attacks_freshness_rollback_pqc | 2600 | 50 | 0 | 100.00 |
| attacks_mitm_key_confuse_hmac | 50 | 50 | 0 | 100.00 |
| attacks_mitm_key_confuse_hybrid | 2650 | 50 | 0 | 100.00 |
| attacks_mitm_key_confuse_pqc | 2600 | 50 | 0 | 100.00 |
| attacks_replay_hmac | 47 | 47 | 3 | 94.00 |
| attacks_replay_hybrid | 57 | 47 | 3 | 94.00 |
| attacks_replay_pqc | 57 | 47 | 3 | 94.00 |
| attacks_sig_fuzz_hmac | 50 | 50 | 0 | 100.00 |
| attacks_sig_fuzz_hybrid | 2650 | 50 | 0 | 100.00 |
| attacks_sig_fuzz_pqc | 2600 | 50 | 0 | 100.00 |
| attacks_tamper_auth_hmac | 50 | 50 | 0 | 100.00 |
| attacks_tamper_auth_hybrid | 2650 | 50 | 0 | 100.00 |
| attacks_tamper_auth_pqc | 2600 | 50 | 0 | 100.00 |
| attacks_tamper_payload_hmac | 50 | 50 | 0 | 100.00 |
| attacks_tamper_payload_hybrid | 2650 | 50 | 0 | 100.00 |
| attacks_tamper_payload_pqc | 2600 | 50 | 0 | 100.00 |
| baseline_hmac_r1 | 0 | 0 | 0 | 0.00 |
| baseline_hybrid_r1 | 0 | 0 | 0 | 0.00 |
| baseline_pqc_r1 | 0 | 0 | 0 | 0.00 |
| bus_failure_aggregate | 0 | 0 | 0 | 0.00 |
| bus_failure_clean | 0 | 0 | 0 | 0.00 |
| bus_failure_heavy | 0 | 0 | 0 | 0.00 |
| bus_failure_noisy | 0 | 0 | 0 | 0.00 |
| deadline_relaxed | 0 | 0 | 0 | 0.00 |
| deadline_tight | 0 | 0 | 0 | 0.00 |
| keymismatch_aggregate | 50 | 50 | 0 | 100.00 |
| keymismatch_mismatch | 50 | 50 | 0 | 100.00 |
| keymismatch_shared | 0 | 0 | 0 | 0.00 |
| mixed_bus_can | 0 | 0 | 0 | 0.00 |
| mixed_bus_eth | 0 | 0 | 0 | 0.00 |
| multi_ecu_aggregate | 0 | 0 | 0 | 0.00 |
| multi_ecu_rx0 | 0 | 0 | 0 | 0.00 |
| multi_ecu_rx1 | 0 | 0 | 0 | 0.00 |
| multi_ecu_rx2 | 0 | 0 | 0 | 0.00 |
| multi_ecu_tx | 0 | 0 | 0 | 0.00 |
| persistence_aggregate | 50 | 25 | 25 | 50.00 |
| persistence_no_nvm | 25 | 0 | 25 | 0.00 |
| persistence_with_nvm | 25 | 25 | 0 | 100.00 |
| rekey_r1 | 0 | 0 | 0 | 0.00 |
| rollover_hmac_r1 | 8 | 8 | 0 | 100.00 |
| rollover_pqc_r1 | 8 | 8 | 0 | 100.00 |
| tput_hmac_canfd_r1 | 0 | 0 | 0 | 0.00 |
| tput_pqc_eth1000_r1 | 0 | 0 | 0 | 0.00 |
| tput_pqc_eth100_r1 | 0 | 0 | 0 | 0.00 |


## 3. Deadline-miss rate per ASIL/deadline class

Source: `summary/deadline_miss.csv`.

| scenario | D1_miss_pct | D2_miss_pct | D3_miss_pct | D4_miss_pct | D5_miss_pct | D6_miss_pct |
| --- | --- | --- | --- | --- | --- | --- |
| attacks_aggregate_hmac | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_aggregate_hybrid | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_aggregate_pqc | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_downgrade_hmac_hmac | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_downgrade_hmac_hybrid | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_downgrade_hmac_pqc | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_freshness_rollback_hmac | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_freshness_rollback_hybrid | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_freshness_rollback_pqc | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_mitm_key_confuse_hmac | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_mitm_key_confuse_hybrid | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_mitm_key_confuse_pqc | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_replay_hmac | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_replay_hybrid | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_replay_pqc | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_sig_fuzz_hmac | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_sig_fuzz_hybrid | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_sig_fuzz_pqc | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_tamper_auth_hmac | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_tamper_auth_hybrid | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_tamper_auth_pqc | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_tamper_payload_hmac | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_tamper_payload_hybrid | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_tamper_payload_pqc | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| baseline_hmac_r1 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| baseline_hybrid_r1 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| baseline_pqc_r1 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| bus_failure_aggregate | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| bus_failure_clean | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| bus_failure_heavy | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| bus_failure_noisy | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| deadline_relaxed | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| deadline_tight | 100.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| keymismatch_aggregate | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| keymismatch_mismatch | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| keymismatch_shared | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| mixed_bus_can | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| mixed_bus_eth | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| multi_ecu_aggregate | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| multi_ecu_rx0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| multi_ecu_rx1 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| multi_ecu_rx2 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| multi_ecu_tx | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| persistence_aggregate | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| persistence_no_nvm | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| persistence_with_nvm | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| rekey_r1 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| rollover_hmac_r1 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| rollover_pqc_r1 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| tput_hmac_canfd_r1 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| tput_pqc_eth1000_r1 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| tput_pqc_eth100_r1 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |


## 4. PDU size and fragmentation

Source: `summary/pdu_overhead.csv`.

| scenario | pdu_mean_bytes | pdu_p95_bytes | pdu_max_bytes | fragments_mean | fragments_max |
| --- | --- | --- | --- | --- | --- |
| attacks_aggregate_hmac | 34.0 | 34 | 34 | 1.00 | 1 |
| attacks_aggregate_hybrid | 3343.0 | 3343 | 3343 | 53.00 | 53 |
| attacks_aggregate_pqc | 3327.0 | 3327 | 3327 | 52.00 | 52 |
| attacks_downgrade_hmac_hmac | 34.0 | 34 | 34 | 1.00 | 1 |
| attacks_downgrade_hmac_hybrid | 3343.0 | 3343 | 3343 | 53.00 | 53 |
| attacks_downgrade_hmac_pqc | 3327.0 | 3327 | 3327 | 52.00 | 52 |
| attacks_freshness_rollback_hmac | 34.0 | 34 | 34 | 1.00 | 1 |
| attacks_freshness_rollback_hybrid | 3343.0 | 3343 | 3343 | 53.00 | 53 |
| attacks_freshness_rollback_pqc | 3327.0 | 3327 | 3327 | 52.00 | 52 |
| attacks_mitm_key_confuse_hmac | 34.0 | 34 | 34 | 1.00 | 1 |
| attacks_mitm_key_confuse_hybrid | 3343.0 | 3343 | 3343 | 53.00 | 53 |
| attacks_mitm_key_confuse_pqc | 3327.0 | 3327 | 3327 | 52.00 | 52 |
| attacks_replay_hmac | 34.0 | 34 | 34 | 1.00 | 1 |
| attacks_replay_hybrid | 3343.0 | 3343 | 3343 | 53.00 | 53 |
| attacks_replay_pqc | 3327.0 | 3327 | 3327 | 52.00 | 52 |
| attacks_sig_fuzz_hmac | 34.0 | 34 | 34 | 1.00 | 1 |
| attacks_sig_fuzz_hybrid | 3343.0 | 3343 | 3343 | 53.00 | 53 |
| attacks_sig_fuzz_pqc | 3327.0 | 3327 | 3327 | 52.00 | 52 |
| attacks_tamper_auth_hmac | 34.0 | 34 | 34 | 1.00 | 1 |
| attacks_tamper_auth_hybrid | 3343.0 | 3343 | 3343 | 53.00 | 53 |
| attacks_tamper_auth_pqc | 3327.0 | 3327 | 3327 | 52.00 | 52 |
| attacks_tamper_payload_hmac | 34.0 | 34 | 34 | 1.00 | 1 |
| attacks_tamper_payload_hybrid | 3343.0 | 3343 | 3343 | 53.00 | 53 |
| attacks_tamper_payload_pqc | 3327.0 | 3327 | 3327 | 52.00 | 52 |
| baseline_hmac_r1 | 173.1 | 1050 | 1050 | 1.79 | 5 |
| baseline_hybrid_r1 | 3482.1 | 4359 | 4359 | 114.14 | 418 |
| baseline_pqc_r1 | 3466.1 | 4343 | 4343 | 113.57 | 416 |
| bus_failure_aggregate | 0.0 | 0 | 0 | 0.00 | 0 |
| bus_failure_clean | 34.0 | 34 | 34 | 1.00 | 1 |
| bus_failure_heavy | 34.0 | 34 | 34 | 1.00 | 1 |
| bus_failure_noisy | 34.0 | 34 | 34 | 1.00 | 1 |
| deadline_relaxed | 42.0 | 42 | 42 | 1.00 | 1 |
| deadline_tight | 34.0 | 34 | 34 | 0.00 | 0 |
| keymismatch_aggregate | 0.0 | 0 | 0 | 0.00 | 0 |
| keymismatch_mismatch | 34.0 | 34 | 34 | 1.00 | 1 |
| keymismatch_shared | 34.0 | 34 | 34 | 1.00 | 1 |
| mixed_bus_can | 3327.0 | 3327 | 3327 | 52.00 | 52 |
| mixed_bus_eth | 3327.0 | 3327 | 3327 | 52.00 | 52 |
| multi_ecu_aggregate | 0.0 | 0 | 0 | 0.00 | 0 |
| multi_ecu_rx0 | 0.0 | 0 | 0 | 0.00 | 0 |
| multi_ecu_rx1 | 0.0 | 0 | 0 | 0.00 | 0 |
| multi_ecu_rx2 | 0.0 | 0 | 0 | 0.00 | 0 |
| multi_ecu_tx | 34.0 | 34 | 34 | 1.00 | 1 |
| persistence_aggregate | 0.0 | 0 | 0 | 0.00 | 0 |
| persistence_no_nvm | 34.0 | 34 | 34 | 1.00 | 1 |
| persistence_with_nvm | 34.0 | 34 | 34 | 1.00 | 1 |
| rekey_r1 | 0.0 | 0 | 0 | 0.00 | 0 |
| rollover_hmac_r1 | 34.0 | 34 | 34 | 1.00 | 1 |
| rollover_pqc_r1 | 3327.0 | 3327 | 3327 | 52.00 | 52 |
| tput_hmac_canfd_r1 | 34.0 | 34 | 34 | 1.00 | 1 |
| tput_pqc_eth1000_r1 | 4343.0 | 4343 | 4343 | 3.00 | 3 |
| tput_pqc_eth100_r1 | 4343.0 | 4343 | 4343 | 3.00 | 3 |


## 5. Per-layer AUTOSAR latency breakdown

Source: `summary/layer_latency.csv` and `summary/flow_timeline.md`. Splits the end-to-end latency into Com/PduR, SecOC build, Csm sign, CanTP fragmentation, bus transit and Csm verify.

| scenario | samples | csm_sign_mean_us | csm_sign_p99_us | cantp_frag_mean_us | cantp_frag_p99_us | bus_transit_mean_us | bus_transit_p99_us | csm_verify_mean_us | csm_verify_p99_us | com_pdur_secoc_mean_us | e2e_mean_us | e2e_p99_us |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| attacks_aggregate_hmac | 350 | 11.04 | 22.53 | 0.13 | 0.19 | 0.00 | 0.00 | 9.16 | 27.27 | 2098.08 | 2118.40 | 2579.27 |
| attacks_aggregate_hybrid | 350 | 161.21 | 365.54 | 2.17 | 4.84 | 0.00 | 0.00 | 22.58 | 71.42 | 1652.39 | 1838.35 | 2620.36 |
| attacks_aggregate_pqc | 350 | 139.27 | 342.38 | 2.06 | 2.42 | 0.00 | 0.00 | 20.96 | 52.22 | 1600.76 | 1763.06 | 3194.08 |
| attacks_downgrade_hmac_hmac | 50 | 2.86 | 15.55 | 0.10 | 0.13 | 13.36 | 20.68 | 14.60 | 31.80 | 2113.57 | 2144.49 | 2543.30 |
| attacks_downgrade_hmac_hybrid | 50 | 172.42 | 310.36 | 2.05 | 2.29 | 276.44 | 563.69 | 16.42 | 26.50 | 1488.38 | 1955.71 | 2292.67 |
| attacks_downgrade_hmac_pqc | 50 | 144.06 | 347.19 | 2.14 | 4.48 | 256.66 | 460.62 | 38.89 | 52.17 | 1393.17 | 1834.91 | 2340.63 |
| attacks_freshness_rollback_hmac | 50 | 12.13 | 20.45 | 0.09 | 0.13 | 14.12 | 37.48 | 0.00 | 0.00 | 2014.49 | 2040.83 | 2474.02 |
| attacks_freshness_rollback_hybrid | 50 | 165.10 | 423.22 | 2.50 | 4.83 | 275.45 | 521.46 | 0.00 | 0.00 | 1458.45 | 1901.49 | 2620.36 |
| attacks_freshness_rollback_pqc | 50 | 158.28 | 275.58 | 2.12 | 2.29 | 288.50 | 547.79 | 0.00 | 0.00 | 1415.56 | 1864.46 | 2405.85 |
| attacks_mitm_key_confuse_hmac | 50 | 2.29 | 4.30 | 0.10 | 0.12 | 13.06 | 21.52 | 13.62 | 26.32 | 2060.07 | 2089.12 | 2540.24 |
| attacks_mitm_key_confuse_hybrid | 50 | 181.19 | 360.64 | 2.03 | 2.21 | 297.53 | 609.90 | 15.07 | 27.36 | 1382.73 | 1878.54 | 2441.60 |
| attacks_mitm_key_confuse_pqc | 50 | 136.75 | 322.08 | 2.01 | 2.24 | 278.08 | 505.38 | 20.40 | 30.42 | 1315.40 | 1752.64 | 2324.29 |
| attacks_replay_hmac | 50 | 52.64 | 30.78 | 0.10 | 0.12 | 14.83 | 29.78 | 1.07 | 16.70 | 2110.31 | 2178.96 | 2667.79 |
| attacks_replay_hybrid | 50 | 195.15 | 367.17 | 2.10 | 2.35 | 200.69 | 410.73 | 3.95 | 66.25 | 1251.04 | 1652.92 | 2286.68 |
| attacks_replay_pqc | 50 | 141.23 | 245.28 | 1.99 | 2.22 | 182.80 | 386.49 | 2.33 | 35.82 | 1285.91 | 1614.25 | 2312.82 |
| attacks_sig_fuzz_hmac | 50 | 2.69 | 15.88 | 0.10 | 0.14 | 19.12 | 29.59 | 15.47 | 25.70 | 2026.09 | 2063.47 | 2545.58 |
| attacks_sig_fuzz_hybrid | 50 | 139.37 | 262.05 | 2.22 | 4.75 | 309.31 | 611.61 | 39.45 | 60.69 | 1307.34 | 1797.70 | 2301.46 |
| attacks_sig_fuzz_pqc | 50 | 126.39 | 243.95 | 2.01 | 2.23 | 261.84 | 520.78 | 20.59 | 32.41 | 1286.68 | 1697.50 | 2271.22 |
| attacks_tamper_auth_hmac | 50 | 2.27 | 9.20 | 0.10 | 0.19 | 11.86 | 33.12 | 9.68 | 18.89 | 2075.68 | 2099.59 | 2458.95 |
| attacks_tamper_auth_hybrid | 50 | 141.90 | 311.33 | 2.05 | 2.37 | 274.60 | 633.23 | 48.08 | 63.12 | 1299.00 | 1765.64 | 2313.27 |
| attacks_tamper_auth_pqc | 50 | 135.08 | 270.65 | 2.20 | 2.23 | 239.21 | 533.44 | 30.57 | 52.22 | 1480.59 | 1887.64 | 3308.05 |
| attacks_tamper_payload_hmac | 50 | 2.38 | 14.01 | 0.30 | 0.25 | 8.96 | 22.02 | 9.69 | 24.04 | 2191.05 | 2212.37 | 2470.07 |
| attacks_tamper_payload_hybrid | 50 | 133.35 | 279.40 | 2.22 | 5.59 | 263.99 | 515.16 | 35.10 | 79.28 | 1481.80 | 1916.47 | 2361.59 |
| attacks_tamper_payload_pqc | 50 | 133.08 | 290.68 | 1.99 | 2.13 | 240.64 | 507.85 | 33.97 | 50.86 | 1280.31 | 1690.00 | 2311.43 |
| baseline_hmac_r1 | 700 | 5.03 | 16.76 | 0.13 | 0.30 | 13.90 | 35.42 | 12.86 | 27.97 | 1870.26 | 1902.17 | 2551.06 |
| baseline_hybrid_r1 | 700 | 156.63 | 442.12 | 4.29 | 19.46 | 84.66 | 1230.15 | 55.24 | 84.92 | 1757.03 | 2057.86 | 3443.18 |
| baseline_pqc_r1 | 700 | 147.80 | 335.06 | 4.38 | 23.72 | 83.96 | 1201.08 | 40.92 | 67.42 | 1767.51 | 2044.57 | 3468.51 |
| bus_failure_aggregate | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| bus_failure_clean | 100 | 19.57 | 15.21 | 0.10 | 0.14 | 13.08 | 25.08 | 16.18 | 35.31 | 2057.31 | 2106.24 | 2593.16 |
| bus_failure_heavy | 100 | 2.27 | 15.55 | 0.09 | 0.12 | 10.09 | 31.94 | 10.96 | 24.15 | 2025.09 | 2048.49 | 2480.35 |
| bus_failure_noisy | 100 | 2.35 | 17.98 | 0.09 | 0.12 | 11.00 | 20.72 | 13.56 | 35.06 | 2096.52 | 2123.53 | 2516.33 |
| deadline_relaxed | 50 | 3.03 | 19.99 | 0.11 | 0.14 | 12.97 | 27.03 | 15.38 | 28.93 | 1229.40 | 1260.89 | 1346.86 |
| deadline_tight | 50 | 69.91 | 47.81 | 185.28 | 326.68 | 0.00 | 0.00 | 0.00 | 0.00 | 50641.84 | 50897.02 | 51429.06 |
| keymismatch_aggregate | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| keymismatch_mismatch | 0 | 2.16 | 4.66 | 0.15 | 0.14 | 16.35 | 48.60 | 17.50 | 33.15 | 0.00 | 0.00 | 0.00 |
| keymismatch_shared | 50 | 41.91 | 16.44 | 0.72 | 0.13 | 14.47 | 32.10 | 17.26 | 40.60 | 2064.40 | 2138.76 | 2582.22 |
| mixed_bus_can | 50 | 155.47 | 324.82 | 2.11 | 2.32 | 181.63 | 373.41 | 41.17 | 56.73 | 1335.64 | 1716.02 | 3280.70 |
| mixed_bus_eth | 50 | 232.14 | 652.82 | 2.45 | 2.40 | 176.52 | 396.97 | 40.07 | 56.86 | 2464.49 | 2915.67 | 4516.90 |
| multi_ecu_aggregate | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| multi_ecu_rx0 | 50 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 16.44 | 22.56 | 2081.86 | 2098.29 | 2459.63 |
| multi_ecu_rx1 | 50 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 16.51 | 27.34 | 2086.05 | 2102.55 | 2477.26 |
| multi_ecu_rx2 | 50 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 16.14 | 22.11 | 2161.80 | 2177.94 | 2480.99 |
| multi_ecu_tx | 0 | 16.31 | 20.45 | 0.11 | 0.14 | 14.46 | 33.90 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| persistence_aggregate | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| persistence_no_nvm | 50 | 17.53 | 14.62 | 0.11 | 0.24 | 9.78 | 22.59 | 13.80 | 25.20 | 2160.33 | 2201.55 | 2452.05 |
| persistence_with_nvm | 50 | 5.14 | 19.25 | 0.11 | 0.14 | 14.11 | 27.59 | 13.37 | 30.70 | 2071.64 | 2104.36 | 2487.42 |
| rekey_r1 | 50 | 23.14 | 40.78 | 11.76 | 18.26 | 0.00 | 0.00 | 15.38 | 25.09 | 0.32 | 50.60 | 66.75 |
| rollover_hmac_r1 | 0 | 63.30 | 20.39 | 0.11 | 0.13 | 13.82 | 17.84 | 12.85 | 22.24 | 0.00 | 0.00 | 0.00 |
| rollover_pqc_r1 | 0 | 145.97 | 222.39 | 1.98 | 2.10 | 183.45 | 384.07 | 31.87 | 46.69 | 0.00 | 0.00 | 0.00 |
| tput_hmac_canfd_r1 | 940 | 4.47 | 15.41 | 0.11 | 0.15 | 13.80 | 31.02 | 14.98 | 29.48 | 2096.01 | 2129.38 | 2619.26 |
| tput_pqc_eth1000_r1 | 1429 | 155.50 | 379.55 | 0.20 | 0.32 | 22.77 | 168.19 | 43.56 | 72.84 | 1177.65 | 1399.68 | 2367.36 |
| tput_pqc_eth100_r1 | 897 | 156.40 | 385.41 | 0.21 | 0.32 | 24.72 | 202.91 | 44.48 | 68.04 | 2004.89 | 2230.70 | 2697.16 |


## 6. Compliance constraints (time / signal / security)

See [`compliance_constraints.md`](compliance_constraints.md) for the per-scenario pass/fail matrix covering ASIL deadlines, PDU size constraints and AUTOSAR SWS_SecOC clauses.


## 7. Standards compliance matrix

See [`compliance_matrix.md`](compliance_matrix.md) for the mapping to AUTOSAR, FIPS, ISO/SAE 21434 and UN R155.

