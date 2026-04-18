# Integrated Simulation Environment — Thesis Report

Generated automatically by `generate_thesis_report.py`.

Number of scenarios collected: **47**.


## 1. Latency and overhead

Raw numbers in `summary/latency_stats.csv`. The table below is copy-pasteable into LaTeX via `pgfplotstable`.

| scenario | samples | e2e_mean_us | e2e_p50_us | e2e_p95_us | e2e_p99_us | e2e_p999_us | auth_mean_us | verify_mean_us | cantp_mean_us | bus_mean_us |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| attacks_aggregate_hmac | 560 | 2159.64 | 2289.28 | 2372.06 | 2477.47 | 2815.47 | 10.65 | 12.18 | 0.09 | 0.00 |
| attacks_aggregate_hybrid | 560 | 2018.51 | 2173.34 | 2546.32 | 3313.19 | 3801.95 | 189.22 | 35.96 | 2.26 | 0.00 |
| attacks_aggregate_pqc | 560 | 2037.49 | 2179.92 | 2293.15 | 3252.90 | 3387.32 | 183.24 | 51.57 | 2.24 | 0.00 |
| attacks_downgrade_hmac_hmac | 80 | 2043.23 | 2285.27 | 2324.75 | 2350.90 | 2350.90 | 5.23 | 14.14 | 0.09 | 12.47 |
| attacks_downgrade_hmac_hybrid | 80 | 1900.56 | 2163.95 | 2466.07 | 2761.87 | 2761.87 | 188.96 | 14.34 | 2.41 | 272.28 |
| attacks_downgrade_hmac_pqc | 80 | 2071.81 | 2181.18 | 2293.15 | 3227.44 | 3227.44 | 180.32 | 78.96 | 2.28 | 282.00 |
| attacks_freshness_rollback_hmac | 80 | 2068.18 | 2269.43 | 2337.99 | 2594.88 | 2594.88 | 13.95 | 0.00 | 0.09 | 12.70 |
| attacks_freshness_rollback_hybrid | 80 | 1990.03 | 2169.88 | 2287.80 | 2568.57 | 2568.57 | 197.02 | 0.00 | 2.15 | 273.01 |
| attacks_freshness_rollback_pqc | 80 | 1999.88 | 2180.15 | 2283.31 | 3226.86 | 3226.86 | 197.64 | 0.00 | 2.17 | 294.70 |
| attacks_mitm_key_confuse_hmac | 80 | 2189.19 | 2285.23 | 2380.95 | 2421.61 | 2421.61 | 5.52 | 13.90 | 0.09 | 13.74 |
| attacks_mitm_key_confuse_hybrid | 80 | 2052.20 | 2176.53 | 2271.05 | 3325.78 | 3325.78 | 180.84 | 14.97 | 2.19 | 311.90 |
| attacks_mitm_key_confuse_pqc | 80 | 2118.52 | 2175.59 | 2300.03 | 3260.22 | 3260.22 | 181.96 | 41.74 | 2.33 | 311.52 |
| attacks_replay_hmac | 80 | 2153.09 | 2296.47 | 2371.47 | 2472.66 | 2472.66 | 34.50 | 14.53 | 0.09 | 12.96 |
| attacks_replay_hybrid | 80 | 1913.42 | 2142.33 | 2271.62 | 2657.00 | 2657.00 | 187.28 | 68.49 | 2.21 | 213.48 |
| attacks_replay_pqc | 80 | 1987.55 | 2171.59 | 2377.36 | 3252.90 | 3252.90 | 164.30 | 72.67 | 2.28 | 204.46 |
| attacks_sig_fuzz_hmac | 80 | 2201.85 | 2287.14 | 2355.47 | 2475.97 | 2475.97 | 5.26 | 13.93 | 0.09 | 12.20 |
| attacks_sig_fuzz_hybrid | 80 | 2094.42 | 2174.32 | 3214.62 | 3371.64 | 3371.64 | 211.47 | 49.16 | 2.50 | 312.01 |
| attacks_sig_fuzz_pqc | 80 | 2068.71 | 2181.67 | 2267.57 | 2899.68 | 2899.68 | 199.18 | 41.45 | 2.12 | 286.52 |
| attacks_tamper_auth_hmac | 80 | 2226.32 | 2294.00 | 2383.68 | 2401.93 | 2401.93 | 4.90 | 13.82 | 0.09 | 13.53 |
| attacks_tamper_auth_hybrid | 80 | 2096.04 | 2177.00 | 2657.32 | 3271.93 | 3271.93 | 165.41 | 66.94 | 2.14 | 277.88 |
| attacks_tamper_auth_pqc | 80 | 2034.16 | 2190.10 | 2274.55 | 2415.90 | 2415.90 | 186.46 | 62.71 | 2.14 | 288.58 |
| attacks_tamper_payload_hmac | 80 | 2235.65 | 2314.08 | 2390.50 | 2451.41 | 2451.41 | 5.17 | 14.94 | 0.09 | 14.95 |
| attacks_tamper_payload_hybrid | 80 | 2082.94 | 2179.99 | 2401.98 | 3270.46 | 3270.46 | 193.58 | 37.80 | 2.23 | 272.15 |
| attacks_tamper_payload_pqc | 80 | 1981.78 | 2169.49 | 2273.63 | 2570.12 | 2570.12 | 172.81 | 63.44 | 2.33 | 264.53 |
| baseline_hmac_r1 | 1120 | 1917.56 | 2282.92 | 2378.26 | 2469.65 | 3494.83 | 7.89 | 15.34 | 0.12 | 14.64 |
| baseline_hybrid_r1 | 1120 | 2096.80 | 2194.09 | 3268.99 | 3348.53 | 3698.39 | 182.20 | 86.68 | 4.80 | 82.22 |
| baseline_pqc_r1 | 1120 | 2071.35 | 2197.72 | 3229.58 | 3356.86 | 3453.49 | 176.71 | 78.49 | 4.79 | 86.02 |
| bus_failure_aggregate | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| bus_failure_clean | 100 | 2131.86 | 2299.71 | 2383.46 | 2444.08 | 2444.08 | 28.37 | 15.36 | 0.09 | 11.30 |
| bus_failure_heavy | 100 | 2180.54 | 2286.22 | 2362.57 | 2446.35 | 2446.35 | 5.06 | 15.51 | 0.09 | 11.99 |
| bus_failure_noisy | 100 | 2165.03 | 2302.41 | 2357.53 | 2376.84 | 2376.84 | 5.08 | 15.40 | 0.09 | 11.69 |
| deadline_relaxed | 80 | 1280.19 | 1209.84 | 1455.16 | 3199.87 | 3199.87 | 5.83 | 15.70 | 0.10 | 12.00 |
| deadline_tight | 80 | 50865.89 | 50824.19 | 51296.99 | 51361.46 | 51361.46 | 64.03 | 0.00 | 162.68 | 0.00 |
| mixed_bus_can | 80 | 1842.41 | 2153.68 | 2231.61 | 3265.51 | 3265.51 | 173.71 | 75.08 | 2.28 | 189.52 |
| mixed_bus_eth | 80 | 3009.42 | 3247.62 | 3541.51 | 4381.45 | 4381.45 | 152.62 | 75.96 | 2.14 | 190.50 |
| multi_ecu_aggregate | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| multi_ecu_rx0 | 80 | 2151.21 | 2283.77 | 2377.55 | 2449.34 | 2449.34 | 0.00 | 14.08 | 0.00 | 0.00 |
| multi_ecu_rx1 | 80 | 2108.66 | 2286.84 | 2344.08 | 2387.08 | 2387.08 | 0.00 | 15.06 | 0.00 | 0.00 |
| multi_ecu_rx2 | 80 | 2193.62 | 2282.23 | 2353.50 | 2405.77 | 2405.77 | 0.00 | 13.77 | 0.00 | 0.00 |
| multi_ecu_tx | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 14.59 | 0.00 | 0.09 | 10.73 |
| persistence_aggregate | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| persistence_no_nvm | 80 | 2280.59 | 2316.08 | 2414.72 | 2554.08 | 2554.08 | 17.99 | 16.04 | 0.09 | 12.11 |
| persistence_with_nvm | 80 | 2214.96 | 2303.02 | 2401.75 | 2434.17 | 2434.17 | 7.25 | 12.34 | 0.09 | 12.09 |
| rekey_r1 | 80 | 72.81 | 71.04 | 91.84 | 102.22 | 102.22 | 27.77 | 26.95 | 17.71 | 0.00 |
| tput_hmac_canfd_r1 | 940 | 2128.58 | 2282.74 | 2359.00 | 2482.59 | 4529.79 | 7.49 | 14.12 | 0.11 | 11.19 |
| tput_pqc_eth1000_r1 | 1457 | 1372.83 | 1391.05 | 1555.38 | 2222.95 | 3297.00 | 171.30 | 81.52 | 0.20 | 21.93 |
| tput_pqc_eth100_r1 | 898 | 2226.95 | 2220.67 | 2541.78 | 2597.67 | 2725.02 | 176.01 | 82.23 | 0.20 | 24.47 |


## 2. Attack detection (UN R155 Annex 5 mapping)

Source: `summary/attack_detection.csv`.

| scenario | attacks_injected | attacks_detected | attacks_delivered | detection_rate_pct |
| --- | --- | --- | --- | --- |
| attacks_aggregate_hmac | 557 | 403 | 157 | 71.96 |
| attacks_aggregate_hybrid | 25529 | 556 | 4 | 99.29 |
| attacks_aggregate_pqc | 25050 | 557 | 3 | 99.46 |
| attacks_downgrade_hmac_hmac | 80 | 0 | 80 | 0.00 |
| attacks_downgrade_hmac_hybrid | 4240 | 80 | 0 | 100.00 |
| attacks_downgrade_hmac_pqc | 4160 | 80 | 0 | 100.00 |
| attacks_freshness_rollback_hmac | 80 | 80 | 0 | 100.00 |
| attacks_freshness_rollback_hybrid | 4240 | 80 | 0 | 100.00 |
| attacks_freshness_rollback_pqc | 4160 | 80 | 0 | 100.00 |
| attacks_mitm_key_confuse_hmac | 80 | 80 | 0 | 100.00 |
| attacks_mitm_key_confuse_hybrid | 4240 | 80 | 0 | 100.00 |
| attacks_mitm_key_confuse_pqc | 4160 | 80 | 0 | 100.00 |
| attacks_replay_hmac | 77 | 3 | 77 | 3.75 |
| attacks_replay_hybrid | 89 | 76 | 4 | 95.00 |
| attacks_replay_pqc | 90 | 77 | 3 | 96.25 |
| attacks_sig_fuzz_hmac | 80 | 80 | 0 | 100.00 |
| attacks_sig_fuzz_hybrid | 4240 | 80 | 0 | 100.00 |
| attacks_sig_fuzz_pqc | 4160 | 80 | 0 | 100.00 |
| attacks_tamper_auth_hmac | 80 | 80 | 0 | 100.00 |
| attacks_tamper_auth_hybrid | 4240 | 80 | 0 | 100.00 |
| attacks_tamper_auth_pqc | 4160 | 80 | 0 | 100.00 |
| attacks_tamper_payload_hmac | 80 | 80 | 0 | 100.00 |
| attacks_tamper_payload_hybrid | 4240 | 80 | 0 | 100.00 |
| attacks_tamper_payload_pqc | 4160 | 80 | 0 | 100.00 |
| baseline_hmac_r1 | 0 | 0 | 0 | 0.00 |
| baseline_hybrid_r1 | 0 | 0 | 0 | 0.00 |
| baseline_pqc_r1 | 0 | 0 | 0 | 0.00 |
| bus_failure_aggregate | 0 | 0 | 0 | 0.00 |
| bus_failure_clean | 0 | 0 | 0 | 0.00 |
| bus_failure_heavy | 0 | 0 | 0 | 0.00 |
| bus_failure_noisy | 0 | 0 | 0 | 0.00 |
| deadline_relaxed | 0 | 0 | 0 | 0.00 |
| deadline_tight | 0 | 0 | 0 | 0.00 |
| mixed_bus_can | 0 | 0 | 0 | 0.00 |
| mixed_bus_eth | 0 | 0 | 0 | 0.00 |
| multi_ecu_aggregate | 0 | 0 | 0 | 0.00 |
| multi_ecu_rx0 | 0 | 0 | 0 | 0.00 |
| multi_ecu_rx1 | 0 | 0 | 0 | 0.00 |
| multi_ecu_rx2 | 0 | 0 | 0 | 0.00 |
| multi_ecu_tx | 0 | 0 | 0 | 0.00 |
| persistence_aggregate | 80 | 40 | 40 | 50.00 |
| persistence_no_nvm | 40 | 0 | 40 | 0.00 |
| persistence_with_nvm | 40 | 40 | 0 | 100.00 |
| rekey_r1 | 0 | 0 | 0 | 0.00 |
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
| tput_hmac_canfd_r1 | 34.0 | 34 | 34 | 1.00 | 1 |
| tput_pqc_eth1000_r1 | 4343.0 | 4343 | 4343 | 3.00 | 3 |
| tput_pqc_eth100_r1 | 4343.0 | 4343 | 4343 | 3.00 | 3 |


## 5. Per-layer AUTOSAR latency breakdown

Source: `summary/layer_latency.csv` and `summary/flow_timeline.md`. Splits the end-to-end latency into Com/PduR, SecOC build, Csm sign, CanTP fragmentation, bus transit and Csm verify.

| scenario | samples | csm_sign_mean_us | csm_sign_p99_us | cantp_frag_mean_us | cantp_frag_p99_us | bus_transit_mean_us | bus_transit_p99_us | csm_verify_mean_us | csm_verify_p99_us | com_pdur_secoc_mean_us | e2e_mean_us | e2e_p99_us |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| attacks_aggregate_hmac | 560 | 10.65 | 23.29 | 0.09 | 0.13 | 0.00 | 0.00 | 12.18 | 28.50 | 2136.72 | 2159.64 | 2477.47 |
| attacks_aggregate_hybrid | 560 | 189.22 | 449.06 | 2.26 | 2.96 | 0.00 | 0.00 | 35.96 | 101.36 | 1791.07 | 2018.51 | 3313.19 |
| attacks_aggregate_pqc | 560 | 183.24 | 458.41 | 2.24 | 3.02 | 0.00 | 0.00 | 51.57 | 100.41 | 1800.45 | 2037.49 | 3252.90 |
| attacks_downgrade_hmac_hmac | 80 | 5.23 | 13.62 | 0.09 | 0.12 | 12.47 | 22.15 | 14.14 | 24.68 | 2011.30 | 2043.23 | 2350.90 |
| attacks_downgrade_hmac_hybrid | 80 | 188.96 | 427.61 | 2.41 | 3.08 | 272.28 | 534.66 | 14.34 | 22.65 | 1422.56 | 1900.56 | 2761.87 |
| attacks_downgrade_hmac_pqc | 80 | 180.32 | 478.61 | 2.28 | 2.77 | 282.00 | 486.38 | 78.96 | 113.74 | 1528.25 | 2071.81 | 3227.44 |
| attacks_freshness_rollback_hmac | 80 | 13.95 | 25.59 | 0.09 | 0.13 | 12.70 | 30.56 | 0.00 | 0.00 | 2041.44 | 2068.18 | 2594.88 |
| attacks_freshness_rollback_hybrid | 80 | 197.02 | 416.50 | 2.15 | 2.52 | 273.01 | 481.87 | 0.00 | 0.00 | 1517.86 | 1990.03 | 2568.57 |
| attacks_freshness_rollback_pqc | 80 | 197.64 | 430.94 | 2.17 | 3.02 | 294.70 | 517.14 | 0.00 | 0.00 | 1505.38 | 1999.88 | 3226.86 |
| attacks_mitm_key_confuse_hmac | 80 | 5.52 | 13.74 | 0.09 | 0.12 | 13.74 | 33.67 | 13.90 | 23.52 | 2155.93 | 2189.19 | 2421.61 |
| attacks_mitm_key_confuse_hybrid | 80 | 180.84 | 364.09 | 2.19 | 2.73 | 311.90 | 588.66 | 14.97 | 28.71 | 1542.30 | 2052.20 | 3325.78 |
| attacks_mitm_key_confuse_pqc | 80 | 181.96 | 425.38 | 2.33 | 2.78 | 311.52 | 571.25 | 41.74 | 65.58 | 1580.98 | 2118.52 | 3260.22 |
| attacks_replay_hmac | 80 | 34.50 | 26.61 | 0.09 | 0.13 | 12.96 | 27.68 | 14.53 | 32.22 | 2091.02 | 2153.09 | 2472.66 |
| attacks_replay_hybrid | 80 | 187.28 | 583.41 | 2.21 | 2.71 | 213.48 | 411.75 | 68.49 | 106.48 | 1441.95 | 1913.42 | 2657.00 |
| attacks_replay_pqc | 80 | 164.30 | 334.33 | 2.28 | 2.82 | 204.46 | 381.44 | 72.67 | 97.20 | 1543.84 | 1987.55 | 3252.90 |
| attacks_sig_fuzz_hmac | 80 | 5.26 | 15.80 | 0.09 | 0.12 | 12.20 | 21.97 | 13.93 | 20.03 | 2170.36 | 2201.85 | 2475.97 |
| attacks_sig_fuzz_hybrid | 80 | 211.47 | 449.06 | 2.50 | 10.26 | 312.01 | 584.47 | 49.16 | 79.68 | 1519.27 | 2094.42 | 3371.64 |
| attacks_sig_fuzz_pqc | 80 | 199.18 | 562.58 | 2.12 | 2.68 | 286.52 | 484.97 | 41.45 | 55.41 | 1539.44 | 2068.71 | 2899.68 |
| attacks_tamper_auth_hmac | 80 | 4.90 | 12.20 | 0.09 | 0.13 | 13.53 | 32.40 | 13.82 | 24.09 | 2193.97 | 2226.32 | 2401.93 |
| attacks_tamper_auth_hybrid | 80 | 165.41 | 372.14 | 2.14 | 2.56 | 277.88 | 419.00 | 66.94 | 99.92 | 1583.68 | 2096.04 | 3271.93 |
| attacks_tamper_auth_pqc | 80 | 186.46 | 453.79 | 2.14 | 2.47 | 288.58 | 539.78 | 62.71 | 100.41 | 1494.28 | 2034.16 | 2415.90 |
| attacks_tamper_payload_hmac | 80 | 5.17 | 11.25 | 0.09 | 0.13 | 14.95 | 22.78 | 14.94 | 27.36 | 2200.51 | 2235.65 | 2451.41 |
| attacks_tamper_payload_hybrid | 80 | 193.58 | 437.19 | 2.23 | 2.78 | 272.15 | 479.98 | 37.80 | 92.23 | 1577.19 | 2082.94 | 3270.46 |
| attacks_tamper_payload_pqc | 80 | 172.81 | 442.13 | 2.33 | 7.65 | 264.53 | 479.64 | 63.44 | 100.37 | 1478.68 | 1981.78 | 2570.12 |
| baseline_hmac_r1 | 1120 | 7.89 | 15.51 | 0.12 | 0.30 | 14.64 | 38.61 | 15.34 | 29.72 | 1879.56 | 1917.56 | 2469.65 |
| baseline_hybrid_r1 | 1120 | 182.20 | 459.94 | 4.80 | 22.06 | 82.22 | 693.92 | 86.68 | 119.99 | 1740.92 | 2096.80 | 3348.53 |
| baseline_pqc_r1 | 1120 | 176.71 | 484.39 | 4.79 | 20.43 | 86.02 | 750.04 | 78.49 | 106.39 | 1725.34 | 2071.35 | 3356.86 |
| bus_failure_aggregate | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| bus_failure_clean | 100 | 28.37 | 14.29 | 0.09 | 0.14 | 11.30 | 21.24 | 15.36 | 32.71 | 2076.74 | 2131.86 | 2444.08 |
| bus_failure_heavy | 100 | 5.06 | 12.64 | 0.09 | 0.13 | 11.99 | 18.66 | 15.51 | 26.44 | 2147.88 | 2180.54 | 2446.35 |
| bus_failure_noisy | 100 | 5.08 | 12.61 | 0.09 | 0.13 | 11.69 | 21.29 | 15.40 | 26.62 | 2132.77 | 2165.03 | 2376.84 |
| deadline_relaxed | 80 | 5.83 | 20.18 | 0.10 | 0.13 | 12.00 | 21.23 | 15.70 | 34.02 | 1246.56 | 1280.19 | 3199.87 |
| deadline_tight | 80 | 64.03 | 75.52 | 162.68 | 222.96 | 0.00 | 0.00 | 0.00 | 0.00 | 50639.18 | 50865.89 | 51361.46 |
| mixed_bus_can | 80 | 173.71 | 467.07 | 2.28 | 2.96 | 189.52 | 312.94 | 75.08 | 97.12 | 1401.83 | 1842.41 | 3265.51 |
| mixed_bus_eth | 80 | 152.62 | 400.63 | 2.14 | 2.40 | 190.50 | 298.56 | 75.96 | 106.95 | 2588.20 | 3009.42 | 4381.45 |
| multi_ecu_aggregate | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| multi_ecu_rx0 | 80 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 14.08 | 19.99 | 2137.13 | 2151.21 | 2449.34 |
| multi_ecu_rx1 | 80 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 15.06 | 38.62 | 2093.60 | 2108.66 | 2387.08 |
| multi_ecu_rx2 | 80 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 13.77 | 18.20 | 2179.84 | 2193.62 | 2405.77 |
| multi_ecu_tx | 0 | 14.59 | 18.47 | 0.09 | 0.13 | 10.73 | 21.93 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| persistence_aggregate | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| persistence_no_nvm | 80 | 17.99 | 21.71 | 0.09 | 0.13 | 12.11 | 23.71 | 16.04 | 33.12 | 2234.35 | 2280.59 | 2554.08 |
| persistence_with_nvm | 80 | 7.25 | 19.49 | 0.09 | 0.13 | 12.09 | 24.53 | 12.34 | 29.77 | 2183.19 | 2214.96 | 2434.17 |
| rekey_r1 | 80 | 27.77 | 54.43 | 17.71 | 36.89 | 0.00 | 0.00 | 26.95 | 36.03 | 0.37 | 72.81 | 102.22 |
| tput_hmac_canfd_r1 | 940 | 7.49 | 13.63 | 0.11 | 0.13 | 11.19 | 26.02 | 14.12 | 30.88 | 2095.67 | 2128.58 | 2482.59 |
| tput_pqc_eth1000_r1 | 1457 | 171.30 | 454.11 | 0.20 | 0.28 | 21.93 | 105.63 | 81.52 | 114.17 | 1097.88 | 1372.83 | 2222.95 |
| tput_pqc_eth100_r1 | 898 | 176.01 | 430.33 | 0.20 | 0.27 | 24.47 | 126.60 | 82.23 | 114.76 | 1944.05 | 2226.95 | 2597.67 |


## 6. Compliance constraints (time / signal / security)

See [`compliance_constraints.md`](compliance_constraints.md) for the per-scenario pass/fail matrix covering ASIL deadlines, PDU size constraints and AUTOSAR SWS_SecOC clauses.


## 7. Standards compliance matrix

See [`compliance_matrix.md`](compliance_matrix.md) for the mapping to AUTOSAR, FIPS, ISO/SAE 21434 and UN R155.

