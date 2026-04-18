# Integrated Simulation Environment — Thesis Report

Generated automatically by `generate_thesis_report.py`.

Number of scenarios collected: **52**.


## 1. Latency and overhead

Raw numbers in `summary/latency_stats.csv`. The table below is copy-pasteable into LaTeX via `pgfplotstable`.

| scenario | samples | e2e_mean_us | e2e_p50_us | e2e_p95_us | e2e_p99_us | e2e_p999_us | auth_mean_us | verify_mean_us | cantp_mean_us | bus_mean_us |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| attacks_aggregate_hmac | 350 | 2164.79 | 2285.93 | 2361.83 | 2439.36 | 2691.44 | 12.12 | 9.91 | 0.09 | 0.00 |
| attacks_aggregate_hybrid | 350 | 1991.69 | 2177.94 | 2621.55 | 3330.29 | 3346.40 | 184.31 | 34.01 | 2.26 | 0.00 |
| attacks_aggregate_pqc | 350 | 1980.55 | 2174.54 | 2441.52 | 3252.70 | 3291.39 | 171.72 | 50.92 | 2.37 | 0.00 |
| attacks_downgrade_hmac_hmac | 50 | 2261.14 | 2298.65 | 2371.73 | 2391.23 | 2391.23 | 5.05 | 13.26 | 0.09 | 9.01 |
| attacks_downgrade_hmac_hybrid | 50 | 1813.06 | 2160.14 | 2231.92 | 2287.68 | 2287.68 | 182.88 | 11.18 | 2.44 | 273.70 |
| attacks_downgrade_hmac_pqc | 50 | 2075.03 | 2181.55 | 2253.09 | 3268.84 | 3268.84 | 176.54 | 76.75 | 2.14 | 265.29 |
| attacks_freshness_rollback_hmac | 50 | 2053.90 | 2265.31 | 2351.43 | 2439.36 | 2439.36 | 11.50 | 0.00 | 0.10 | 12.43 |
| attacks_freshness_rollback_hybrid | 50 | 2132.23 | 2180.02 | 3293.93 | 3337.47 | 3337.47 | 201.83 | 0.00 | 2.23 | 286.11 |
| attacks_freshness_rollback_pqc | 50 | 1989.81 | 2167.27 | 2274.35 | 3190.07 | 3190.07 | 184.21 | 0.00 | 2.60 | 258.62 |
| attacks_mitm_key_confuse_hmac | 50 | 1969.32 | 2295.32 | 2355.53 | 2362.14 | 2362.14 | 4.47 | 11.53 | 0.09 | 10.58 |
| attacks_mitm_key_confuse_hybrid | 50 | 2066.31 | 2185.89 | 2232.97 | 2524.75 | 2524.75 | 177.73 | 12.06 | 2.22 | 301.91 |
| attacks_mitm_key_confuse_pqc | 50 | 2030.72 | 2180.89 | 2230.89 | 3252.70 | 3252.70 | 162.38 | 39.51 | 2.17 | 286.79 |
| attacks_replay_hmac | 50 | 2273.49 | 2303.95 | 2369.82 | 2545.54 | 2545.54 | 48.82 | 10.56 | 0.08 | 10.24 |
| attacks_replay_hybrid | 50 | 1933.43 | 2133.96 | 2649.22 | 3342.07 | 3342.07 | 185.06 | 65.25 | 2.29 | 233.91 |
| attacks_replay_pqc | 50 | 1971.94 | 2180.21 | 2612.48 | 3221.74 | 3221.74 | 156.71 | 70.99 | 2.64 | 199.21 |
| attacks_sig_fuzz_hmac | 50 | 2144.99 | 2279.57 | 2329.63 | 2343.82 | 2343.82 | 4.73 | 10.30 | 0.08 | 10.54 |
| attacks_sig_fuzz_hybrid | 50 | 1924.03 | 2156.73 | 2621.55 | 3209.68 | 3209.68 | 176.89 | 48.49 | 2.25 | 285.19 |
| attacks_sig_fuzz_pqc | 50 | 1868.41 | 2156.86 | 2280.82 | 2441.52 | 2441.52 | 173.85 | 38.39 | 2.17 | 274.30 |
| attacks_tamper_auth_hmac | 50 | 2205.59 | 2272.24 | 2328.59 | 2345.72 | 2345.72 | 4.99 | 11.55 | 0.08 | 11.14 |
| attacks_tamper_auth_hybrid | 50 | 1950.94 | 2175.59 | 2231.60 | 2317.55 | 2317.55 | 173.41 | 62.89 | 2.16 | 264.89 |
| attacks_tamper_auth_pqc | 50 | 2001.98 | 2168.85 | 2349.35 | 2550.22 | 2550.22 | 165.38 | 57.66 | 2.20 | 271.83 |
| attacks_tamper_payload_hmac | 50 | 2245.08 | 2282.70 | 2361.83 | 2379.86 | 2379.86 | 5.24 | 12.15 | 0.09 | 10.32 |
| attacks_tamper_payload_hybrid | 50 | 2121.83 | 2190.95 | 2922.64 | 3220.78 | 3220.78 | 192.34 | 38.23 | 2.23 | 275.69 |
| attacks_tamper_payload_pqc | 50 | 1925.96 | 2132.41 | 2661.46 | 3224.49 | 3224.49 | 183.00 | 73.13 | 2.66 | 281.65 |
| baseline_hmac_r1 | 700 | 1851.30 | 2271.29 | 2375.98 | 2423.33 | 2664.27 | 8.71 | 12.26 | 0.12 | 11.67 |
| baseline_hybrid_r1 | 700 | 2043.72 | 2188.28 | 3268.73 | 3360.64 | 3506.40 | 171.38 | 80.52 | 4.72 | 100.18 |
| baseline_pqc_r1 | 700 | 2108.25 | 2195.07 | 3271.53 | 3329.67 | 3534.11 | 171.86 | 74.16 | 4.79 | 93.48 |
| bus_failure_aggregate | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| bus_failure_clean | 100 | 2061.77 | 2275.76 | 2364.98 | 2455.39 | 2455.39 | 24.72 | 12.14 | 0.09 | 8.83 |
| bus_failure_heavy | 100 | 2157.90 | 2267.55 | 2322.98 | 2341.79 | 2341.79 | 5.35 | 11.44 | 0.09 | 8.63 |
| bus_failure_noisy | 100 | 2178.63 | 2273.43 | 2342.45 | 2373.68 | 2373.68 | 4.95 | 11.85 | 0.09 | 9.25 |
| deadline_relaxed | 50 | 1157.38 | 1163.73 | 1199.20 | 1204.03 | 1204.03 | 4.93 | 7.66 | 0.07 | 5.99 |
| deadline_tight | 50 | 50839.56 | 50794.75 | 51209.48 | 51245.99 | 51245.99 | 75.48 | 0.00 | 114.19 | 0.00 |
| keymismatch_aggregate | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| keymismatch_mismatch | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 5.17 | 11.75 | 0.09 | 10.04 |
| keymismatch_shared | 50 | 2047.09 | 2286.13 | 2362.21 | 2368.52 | 2368.52 | 45.40 | 11.90 | 0.09 | 9.05 |
| mixed_bus_can | 50 | 1962.02 | 2173.33 | 2578.23 | 2621.88 | 2621.88 | 150.54 | 74.39 | 2.15 | 182.55 |
| mixed_bus_eth | 50 | 3105.38 | 3269.00 | 3689.93 | 4379.63 | 4379.63 | 255.05 | 71.48 | 2.15 | 183.21 |
| multi_ecu_aggregate | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| multi_ecu_rx0 | 50 | 2198.25 | 2291.13 | 2353.83 | 2415.88 | 2415.88 | 0.00 | 12.16 | 0.00 | 0.00 |
| multi_ecu_rx1 | 50 | 2217.90 | 2292.24 | 2349.03 | 2410.91 | 2410.91 | 0.00 | 11.42 | 0.00 | 0.00 |
| multi_ecu_rx2 | 50 | 2262.24 | 2303.70 | 2349.58 | 2384.15 | 2384.15 | 0.00 | 11.66 | 0.00 | 0.00 |
| multi_ecu_tx | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 19.50 | 0.00 | 0.09 | 8.20 |
| persistence_aggregate | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| persistence_no_nvm | 50 | 2257.32 | 2292.79 | 2342.79 | 2399.96 | 2399.96 | 21.99 | 11.59 | 0.08 | 8.15 |
| persistence_with_nvm | 50 | 1852.90 | 2264.97 | 2370.31 | 2389.48 | 2389.48 | 5.94 | 9.40 | 0.09 | 9.20 |
| rekey_r1 | 50 | 75.70 | 70.11 | 96.10 | 106.89 | 106.89 | 28.25 | 28.21 | 18.84 | 0.00 |
| rollover_hmac_r1 | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 68.77 | 8.45 | 0.09 | 8.19 |
| rollover_pqc_r1 | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 188.74 | 55.13 | 2.21 | 205.51 |
| tput_hmac_canfd_r1 | 932 | 2145.98 | 2276.13 | 2339.03 | 2410.50 | 3096.52 | 6.93 | 11.28 | 0.09 | 9.27 |
| tput_pqc_eth1000_r1 | 1455 | 1374.52 | 1377.21 | 1658.45 | 2254.67 | 3329.07 | 168.89 | 77.80 | 0.19 | 18.80 |
| tput_pqc_eth100_r1 | 883 | 2265.73 | 2255.52 | 2530.77 | 2583.68 | 2721.59 | 172.45 | 79.76 | 0.21 | 20.44 |


## 2. Attack detection (UN R155 Annex 5 mapping)

Source: `summary/attack_detection.csv`.

| scenario | attacks_injected | attacks_detected | attacks_delivered | detection_rate_pct |
| --- | --- | --- | --- | --- |
| attacks_aggregate_hmac | 347 | 253 | 97 | 72.29 |
| attacks_aggregate_hybrid | 15957 | 347 | 3 | 99.14 |
| attacks_aggregate_pqc | 15655 | 347 | 3 | 99.14 |
| attacks_downgrade_hmac_hmac | 50 | 0 | 50 | 0.00 |
| attacks_downgrade_hmac_hybrid | 2650 | 50 | 0 | 100.00 |
| attacks_downgrade_hmac_pqc | 2600 | 50 | 0 | 100.00 |
| attacks_freshness_rollback_hmac | 50 | 50 | 0 | 100.00 |
| attacks_freshness_rollback_hybrid | 2650 | 50 | 0 | 100.00 |
| attacks_freshness_rollback_pqc | 2600 | 50 | 0 | 100.00 |
| attacks_mitm_key_confuse_hmac | 50 | 50 | 0 | 100.00 |
| attacks_mitm_key_confuse_hybrid | 2650 | 50 | 0 | 100.00 |
| attacks_mitm_key_confuse_pqc | 2600 | 50 | 0 | 100.00 |
| attacks_replay_hmac | 47 | 3 | 47 | 6.00 |
| attacks_replay_hybrid | 57 | 47 | 3 | 94.00 |
| attacks_replay_pqc | 55 | 47 | 3 | 94.00 |
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
| attacks_aggregate_hmac | 350 | 12.12 | 15.59 | 0.09 | 0.13 | 0.00 | 0.00 | 9.91 | 23.36 | 2142.68 | 2164.79 | 2439.36 |
| attacks_aggregate_hybrid | 350 | 184.31 | 510.49 | 2.26 | 2.87 | 0.00 | 0.00 | 34.01 | 101.60 | 1771.11 | 1991.69 | 3330.29 |
| attacks_aggregate_pqc | 350 | 171.72 | 495.47 | 2.37 | 8.74 | 0.00 | 0.00 | 50.92 | 103.74 | 1755.54 | 1980.55 | 3252.70 |
| attacks_downgrade_hmac_hmac | 50 | 5.05 | 7.92 | 0.09 | 0.12 | 9.01 | 13.78 | 13.26 | 28.36 | 2233.73 | 2261.14 | 2391.23 |
| attacks_downgrade_hmac_hybrid | 50 | 182.88 | 387.35 | 2.44 | 2.88 | 273.70 | 460.38 | 11.18 | 14.36 | 1342.86 | 1813.06 | 2287.68 |
| attacks_downgrade_hmac_pqc | 50 | 176.54 | 403.12 | 2.14 | 2.39 | 265.29 | 429.80 | 76.75 | 103.74 | 1554.31 | 2075.03 | 3268.84 |
| attacks_freshness_rollback_hmac | 50 | 11.50 | 16.15 | 0.10 | 0.13 | 12.43 | 18.42 | 0.00 | 0.00 | 2029.87 | 2053.90 | 2439.36 |
| attacks_freshness_rollback_hybrid | 50 | 201.83 | 500.23 | 2.23 | 2.69 | 286.11 | 462.83 | 0.00 | 0.00 | 1642.06 | 2132.23 | 3337.47 |
| attacks_freshness_rollback_pqc | 50 | 184.21 | 385.36 | 2.60 | 9.70 | 258.62 | 405.75 | 0.00 | 0.00 | 1544.38 | 1989.81 | 3190.07 |
| attacks_mitm_key_confuse_hmac | 50 | 4.47 | 7.21 | 0.09 | 0.14 | 10.58 | 20.17 | 11.53 | 22.10 | 1942.66 | 1969.32 | 2362.14 |
| attacks_mitm_key_confuse_hybrid | 50 | 177.73 | 415.64 | 2.22 | 2.65 | 301.91 | 432.40 | 12.06 | 15.62 | 1572.39 | 2066.31 | 2524.75 |
| attacks_mitm_key_confuse_pqc | 50 | 162.38 | 334.87 | 2.17 | 2.66 | 286.79 | 438.18 | 39.51 | 56.15 | 1539.88 | 2030.72 | 3252.70 |
| attacks_replay_hmac | 50 | 48.82 | 15.59 | 0.08 | 0.11 | 10.24 | 16.33 | 10.56 | 19.12 | 2203.79 | 2273.49 | 2545.54 |
| attacks_replay_hybrid | 50 | 185.06 | 321.40 | 2.29 | 2.85 | 233.91 | 557.31 | 65.25 | 112.14 | 1446.92 | 1933.43 | 3342.07 |
| attacks_replay_pqc | 50 | 156.71 | 483.32 | 2.64 | 2.63 | 199.21 | 327.88 | 70.99 | 93.33 | 1542.39 | 1971.94 | 3221.74 |
| attacks_sig_fuzz_hmac | 50 | 4.73 | 7.17 | 0.08 | 0.10 | 10.54 | 16.94 | 10.30 | 13.72 | 2119.33 | 2144.99 | 2343.82 |
| attacks_sig_fuzz_hybrid | 50 | 176.89 | 345.33 | 2.25 | 2.66 | 285.19 | 449.24 | 48.49 | 76.48 | 1411.21 | 1924.03 | 3209.68 |
| attacks_sig_fuzz_pqc | 50 | 173.85 | 361.95 | 2.17 | 2.55 | 274.30 | 498.15 | 38.39 | 53.09 | 1379.69 | 1868.41 | 2441.52 |
| attacks_tamper_auth_hmac | 50 | 4.99 | 7.51 | 0.08 | 0.12 | 11.14 | 19.67 | 11.55 | 22.98 | 2177.84 | 2205.59 | 2345.72 |
| attacks_tamper_auth_hybrid | 50 | 173.41 | 451.06 | 2.16 | 2.51 | 264.89 | 501.04 | 62.89 | 85.90 | 1447.59 | 1950.94 | 2317.55 |
| attacks_tamper_auth_pqc | 50 | 165.38 | 385.37 | 2.20 | 2.79 | 271.83 | 399.80 | 57.66 | 78.03 | 1504.91 | 2001.98 | 2550.22 |
| attacks_tamper_payload_hmac | 50 | 5.24 | 13.45 | 0.09 | 0.12 | 10.32 | 19.79 | 12.15 | 22.67 | 2217.28 | 2245.08 | 2379.86 |
| attacks_tamper_payload_hybrid | 50 | 192.34 | 474.78 | 2.23 | 2.88 | 275.69 | 516.31 | 38.23 | 95.40 | 1613.34 | 2121.83 | 3220.78 |
| attacks_tamper_payload_pqc | 50 | 183.00 | 499.96 | 2.66 | 8.74 | 281.65 | 503.17 | 73.13 | 104.50 | 1385.52 | 1925.96 | 3224.49 |
| baseline_hmac_r1 | 700 | 8.71 | 17.66 | 0.12 | 0.29 | 11.67 | 35.65 | 12.26 | 29.79 | 1818.54 | 1851.30 | 2423.33 |
| baseline_hybrid_r1 | 700 | 171.38 | 427.01 | 4.72 | 19.64 | 100.18 | 1481.34 | 80.52 | 117.22 | 1686.92 | 2043.72 | 3360.64 |
| baseline_pqc_r1 | 700 | 171.86 | 507.30 | 4.79 | 23.45 | 93.48 | 1390.31 | 74.16 | 112.45 | 1763.96 | 2108.25 | 3329.67 |
| bus_failure_aggregate | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| bus_failure_clean | 100 | 24.72 | 8.53 | 0.09 | 0.14 | 8.83 | 18.70 | 12.14 | 38.33 | 2015.99 | 2061.77 | 2455.39 |
| bus_failure_heavy | 100 | 5.35 | 16.76 | 0.09 | 0.13 | 8.63 | 15.76 | 11.44 | 25.61 | 2132.39 | 2157.90 | 2341.79 |
| bus_failure_noisy | 100 | 4.95 | 9.69 | 0.09 | 0.15 | 9.25 | 21.70 | 11.85 | 19.30 | 2152.49 | 2178.63 | 2373.68 |
| deadline_relaxed | 50 | 4.93 | 7.50 | 0.07 | 0.11 | 5.99 | 10.78 | 7.66 | 13.90 | 1138.73 | 1157.38 | 1204.03 |
| deadline_tight | 50 | 75.48 | 65.20 | 114.19 | 156.71 | 0.00 | 0.00 | 0.00 | 0.00 | 50649.89 | 50839.56 | 51245.99 |
| keymismatch_aggregate | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| keymismatch_mismatch | 0 | 5.17 | 14.84 | 0.09 | 0.13 | 10.04 | 33.13 | 11.75 | 27.22 | 0.00 | 0.00 | 0.00 |
| keymismatch_shared | 50 | 45.40 | 7.06 | 0.09 | 0.13 | 9.05 | 21.53 | 11.90 | 22.85 | 1980.64 | 2047.09 | 2368.52 |
| mixed_bus_can | 50 | 150.54 | 351.00 | 2.15 | 2.39 | 182.55 | 259.21 | 74.39 | 90.96 | 1552.38 | 1962.02 | 2621.88 |
| mixed_bus_eth | 50 | 255.05 | 509.27 | 2.15 | 2.51 | 183.21 | 273.11 | 71.48 | 84.84 | 2593.49 | 3105.38 | 4379.63 |
| multi_ecu_aggregate | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| multi_ecu_rx0 | 50 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 12.16 | 27.27 | 2186.08 | 2198.25 | 2415.88 |
| multi_ecu_rx1 | 50 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 11.42 | 19.38 | 2206.48 | 2217.90 | 2410.91 |
| multi_ecu_rx2 | 50 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 11.66 | 21.99 | 2250.58 | 2262.24 | 2384.15 |
| multi_ecu_tx | 0 | 19.50 | 8.71 | 0.09 | 0.13 | 8.20 | 13.26 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| persistence_aggregate | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| persistence_no_nvm | 50 | 21.99 | 9.63 | 0.08 | 0.12 | 8.15 | 13.15 | 11.59 | 26.96 | 2215.50 | 2257.32 | 2399.96 |
| persistence_with_nvm | 50 | 5.94 | 12.45 | 0.09 | 0.12 | 9.20 | 23.89 | 9.40 | 21.08 | 1828.28 | 1852.90 | 2389.48 |
| rekey_r1 | 50 | 28.25 | 46.41 | 18.84 | 27.76 | 0.00 | 0.00 | 28.21 | 47.44 | 0.40 | 75.70 | 106.89 |
| rollover_hmac_r1 | 0 | 68.77 | 14.41 | 0.09 | 0.13 | 8.19 | 11.78 | 8.45 | 17.42 | 0.00 | 0.00 | 0.00 |
| rollover_pqc_r1 | 0 | 188.74 | 344.09 | 2.21 | 2.59 | 205.51 | 368.06 | 55.13 | 86.09 | 0.00 | 0.00 | 0.00 |
| tput_hmac_canfd_r1 | 932 | 6.93 | 14.05 | 0.09 | 0.12 | 9.27 | 21.33 | 11.28 | 23.20 | 2118.41 | 2145.98 | 2410.50 |
| tput_pqc_eth1000_r1 | 1455 | 168.89 | 466.01 | 0.19 | 0.26 | 18.80 | 97.05 | 77.80 | 112.92 | 1108.84 | 1374.52 | 2254.67 |
| tput_pqc_eth100_r1 | 883 | 172.45 | 459.81 | 0.21 | 0.29 | 20.44 | 105.24 | 79.76 | 117.09 | 1992.87 | 2265.73 | 2583.68 |


## 6. Compliance constraints (time / signal / security)

See [`compliance_constraints.md`](compliance_constraints.md) for the per-scenario pass/fail matrix covering ASIL deadlines, PDU size constraints and AUTOSAR SWS_SecOC clauses.


## 7. Standards compliance matrix

See [`compliance_matrix.md`](compliance_matrix.md) for the mapping to AUTOSAR, FIPS, ISO/SAE 21434 and UN R155.

