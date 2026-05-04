# Integrated Simulation Environment — Thesis Report

Generated automatically by `generate_thesis_report.py`.

Number of scenarios collected: **61**.


## 1. Latency and overhead

Raw numbers in `summary/latency_stats.csv`. The table below is copy-pasteable into LaTeX via `pgfplotstable`.

| scenario | samples | e2e_mean_us | e2e_p50_us | e2e_p95_us | e2e_p99_us | e2e_p999_us | auth_mean_us | verify_mean_us | cantp_mean_us | bus_mean_us |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| attacks_aggregate_hmac | 1400 | 756.52 | 747.01 | 840.93 | 894.83 | 953.38 | 3.64 | 3.89 | 0.09 | 0.00 |
| attacks_aggregate_hybrid | 1400 | 945.70 | 917.18 | 1106.41 | 1429.19 | 2071.12 | 106.60 | 11.40 | 2.16 | 0.00 |
| attacks_aggregate_pqc | 1400 | 941.87 | 911.52 | 1094.07 | 1422.88 | 2053.57 | 102.85 | 16.73 | 2.11 | 0.00 |
| attacks_dos_flood_hmac | 200 | 762.40 | 747.74 | 849.86 | 926.12 | 940.64 | 8.46 | 5.20 | 0.10 | 3.79 |
| attacks_dos_flood_hybrid | 200 | 1060.93 | 1011.27 | 1317.43 | 2031.77 | 2054.62 | 112.69 | 47.57 | 2.08 | 71.58 |
| attacks_dos_flood_pqc | 200 | 1003.34 | 954.06 | 1184.17 | 2018.20 | 2044.92 | 109.87 | 33.46 | 2.20 | 63.78 |
| attacks_downgrade_hmac_hmac | 200 | 745.36 | 745.34 | 775.14 | 823.08 | 857.88 | 2.28 | 4.12 | 0.10 | 2.97 |
| attacks_downgrade_hmac_hybrid | 200 | 955.55 | 912.49 | 1157.00 | 1939.66 | 1972.52 | 110.75 | 5.38 | 2.20 | 57.21 |
| attacks_downgrade_hmac_pqc | 200 | 941.51 | 924.57 | 1080.69 | 1193.49 | 1277.48 | 102.68 | 32.94 | 2.09 | 56.77 |
| attacks_freshness_rollback_hmac | 200 | 753.54 | 745.67 | 823.59 | 890.53 | 931.37 | 4.44 | 0.00 | 0.09 | 3.44 |
| attacks_freshness_rollback_hybrid | 200 | 979.01 | 966.73 | 1161.63 | 1203.01 | 1220.12 | 124.55 | 0.00 | 2.33 | 66.19 |
| attacks_freshness_rollback_pqc | 200 | 908.66 | 886.68 | 1091.96 | 1206.36 | 1266.84 | 103.48 | 0.00 | 2.04 | 56.88 |
| attacks_harvest_now_hmac | 200 | 755.46 | 742.85 | 827.87 | 926.09 | 929.65 | 7.34 | 4.92 | 0.12 | 3.99 |
| attacks_harvest_now_hybrid | 200 | 1010.47 | 998.48 | 1196.88 | 1299.88 | 1319.83 | 117.11 | 44.33 | 2.35 | 74.00 |
| attacks_harvest_now_pqc | 200 | 966.61 | 940.98 | 1105.11 | 1878.78 | 1931.36 | 97.43 | 33.53 | 2.04 | 61.52 |
| attacks_mitm_key_confuse_hmac | 200 | 762.08 | 741.93 | 859.04 | 906.48 | 914.01 | 2.32 | 6.16 | 0.10 | 4.74 |
| attacks_mitm_key_confuse_hybrid | 200 | 982.63 | 960.38 | 1171.97 | 1381.17 | 2027.66 | 116.63 | 7.09 | 2.15 | 90.42 |
| attacks_mitm_key_confuse_pqc | 200 | 975.62 | 939.62 | 1140.14 | 1986.74 | 2045.85 | 103.50 | 14.15 | 2.04 | 84.83 |
| attacks_replay_hmac | 200 | 752.35 | 735.14 | 829.14 | 907.09 | 946.32 | 10.63 | 0.07 | 0.10 | 3.67 |
| attacks_replay_hybrid | 200 | 914.60 | 894.39 | 1056.68 | 1199.49 | 1334.19 | 110.19 | 0.54 | 2.07 | 43.96 |
| attacks_replay_pqc | 200 | 911.19 | 889.15 | 1068.75 | 1121.97 | 1150.85 | 97.55 | 0.56 | 2.05 | 39.26 |
| attacks_sig_fuzz_hmac | 200 | 761.43 | 749.91 | 855.95 | 903.21 | 923.70 | 2.21 | 4.69 | 0.10 | 3.82 |
| attacks_sig_fuzz_hybrid | 200 | 951.97 | 912.20 | 1097.29 | 1950.72 | 1969.72 | 92.82 | 20.05 | 2.17 | 64.23 |
| attacks_sig_fuzz_pqc | 200 | 945.01 | 918.14 | 1108.14 | 1415.79 | 1908.84 | 104.17 | 15.14 | 2.03 | 63.81 |
| attacks_tamper_auth_hmac | 200 | 765.76 | 750.52 | 884.28 | 936.36 | 956.83 | 2.20 | 4.94 | 0.10 | 3.39 |
| attacks_tamper_auth_hybrid | 200 | 984.10 | 951.00 | 1171.92 | 1979.23 | 2021.80 | 108.33 | 30.28 | 2.08 | 63.33 |
| attacks_tamper_auth_pqc | 200 | 950.52 | 923.18 | 1098.33 | 1225.17 | 1237.44 | 103.42 | 26.50 | 2.08 | 56.64 |
| attacks_tamper_payload_hmac | 200 | 753.82 | 736.68 | 846.88 | 910.66 | 1008.03 | 2.17 | 5.33 | 0.10 | 3.68 |
| attacks_tamper_payload_hybrid | 200 | 932.47 | 915.75 | 1068.97 | 1148.61 | 1167.54 | 106.30 | 16.48 | 2.14 | 59.21 |
| attacks_tamper_payload_pqc | 200 | 947.40 | 925.99 | 1069.30 | 1146.71 | 1937.53 | 102.62 | 28.62 | 2.08 | 57.11 |
| attacks_timing_probe_hmac | 200 | 753.72 | 743.33 | 799.25 | 912.79 | 983.21 | 7.63 | 4.51 | 0.10 | 2.96 |
| attacks_timing_probe_hybrid | 200 | 975.27 | 943.88 | 1114.13 | 1961.81 | 1992.55 | 94.88 | 38.64 | 2.26 | 66.50 |
| attacks_timing_probe_pqc | 200 | 970.10 | 927.41 | 1138.52 | 1933.90 | 2011.29 | 108.13 | 34.10 | 2.47 | 60.59 |
| baseline_hmac_r1 | 2800 | 760.55 | 756.35 | 858.47 | 920.84 | 1017.28 | 2.70 | 5.77 | 0.13 | 4.36 |
| baseline_hybrid_r1 | 2800 | 1015.41 | 942.59 | 1891.12 | 2071.65 | 2216.37 | 105.61 | 40.36 | 4.44 | 24.93 |
| baseline_pqc_r1 | 2800 | 1015.94 | 941.11 | 1905.52 | 2068.87 | 2194.58 | 100.98 | 34.53 | 4.40 | 24.32 |
| bus_failure_aggregate | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| bus_failure_clean | 200 | 759.54 | 746.68 | 816.26 | 866.56 | 900.90 | 8.05 | 4.81 | 0.10 | 2.77 |
| bus_failure_heavy | 195 | 752.72 | 746.97 | 808.19 | 876.14 | 889.78 | 2.15 | 4.70 | 0.10 | 2.55 |
| bus_failure_noisy | 200 | 761.57 | 743.54 | 853.95 | 907.28 | 909.49 | 2.08 | 6.09 | 0.09 | 3.17 |
| deadline_relaxed | 200 | 706.88 | 686.22 | 809.99 | 857.97 | 862.52 | 2.34 | 6.37 | 0.09 | 3.42 |
| deadline_tight | 200 | 50170.81 | 50152.68 | 50226.46 | 50319.55 | 50630.30 | 22.65 | 0.00 | 17.07 | 0.00 |
| keymismatch_aggregate | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| keymismatch_mismatch | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 2.29 | 8.89 | 0.10 | 5.01 |
| keymismatch_shared | 200 | 753.47 | 738.45 | 827.27 | 866.84 | 917.77 | 8.73 | 5.35 | 0.10 | 2.83 |
| mixed_bus_can | 200 | 941.31 | 917.68 | 1067.15 | 1202.77 | 1323.95 | 99.22 | 33.71 | 2.04 | 42.70 |
| mixed_bus_eth | 200 | 1720.72 | 1693.94 | 1926.70 | 2074.92 | 2105.84 | 120.36 | 33.25 | 2.27 | 38.73 |
| multi_ecu_aggregate | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| multi_ecu_rx0 | 200 | 766.16 | 745.03 | 856.88 | 921.83 | 931.46 | 0.00 | 5.79 | 0.00 | 0.00 |
| multi_ecu_rx1 | 200 | 761.96 | 745.28 | 854.55 | 902.42 | 957.31 | 0.00 | 5.71 | 0.00 | 0.00 |
| multi_ecu_rx2 | 200 | 759.19 | 744.92 | 851.22 | 896.95 | 923.35 | 0.00 | 6.18 | 0.00 | 0.00 |
| multi_ecu_tx | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 4.19 | 0.00 | 0.12 | 3.18 |
| persistence_aggregate | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| persistence_no_nvm | 200 | 773.94 | 743.87 | 872.14 | 992.12 | 1026.95 | 4.35 | 6.56 | 0.10 | 3.62 |
| persistence_with_nvm | 200 | 748.18 | 747.32 | 778.09 | 860.87 | 911.70 | 2.44 | 4.00 | 0.10 | 2.71 |
| rekey_r1 | 200 | 36.91 | 32.66 | 54.69 | 104.78 | 114.20 | 10.70 | 12.17 | 13.71 | 0.00 |
| rollover_hmac_r1 | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 38.26 | 4.68 | 0.09 | 3.39 |
| rollover_pqc_r1 | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 137.30 | 27.72 | 2.20 | 79.21 |
| tput_hmac_canfd_r1 | 2627 | 761.29 | 746.09 | 857.79 | 915.45 | 1024.78 | 2.55 | 5.30 | 0.10 | 3.13 |
| tput_pqc_eth1000_r1 | 2383 | 839.25 | 817.20 | 983.25 | 1096.26 | 1318.52 | 100.69 | 35.25 | 0.20 | 5.73 |
| tput_pqc_eth100_r1 | 2096 | 954.18 | 931.64 | 1107.87 | 1214.36 | 1419.43 | 105.19 | 36.86 | 0.23 | 6.20 |


## 2. Attack detection (UN R155 Annex 5 mapping)

Source: `summary/attack_detection.csv`.

| scenario | attacks_injected | attacks_detected | attacks_delivered | detection_rate_pct |
| --- | --- | --- | --- | --- |
| attacks_aggregate_hmac | 1397 | 1197 | 203 | 85.50 |
| attacks_aggregate_hybrid | 63799 | 1397 | 3 | 99.79 |
| attacks_aggregate_pqc | 62597 | 1397 | 3 | 99.79 |
| attacks_dos_flood_hmac | 200 | 0 | 200 | 0.00 |
| attacks_dos_flood_hybrid | 10600 | 0 | 200 | 0.00 |
| attacks_dos_flood_pqc | 10400 | 0 | 200 | 0.00 |
| attacks_downgrade_hmac_hmac | 200 | 0 | 200 | 0.00 |
| attacks_downgrade_hmac_hybrid | 10600 | 200 | 0 | 100.00 |
| attacks_downgrade_hmac_pqc | 10400 | 200 | 0 | 100.00 |
| attacks_freshness_rollback_hmac | 200 | 200 | 0 | 100.00 |
| attacks_freshness_rollback_hybrid | 10600 | 200 | 0 | 100.00 |
| attacks_freshness_rollback_pqc | 10400 | 200 | 0 | 100.00 |
| attacks_harvest_now_hmac | 200 | 0 | 200 | 0.00 |
| attacks_harvest_now_hybrid | 10600 | 0 | 200 | 0.00 |
| attacks_harvest_now_pqc | 10400 | 0 | 200 | 0.00 |
| attacks_mitm_key_confuse_hmac | 200 | 200 | 0 | 100.00 |
| attacks_mitm_key_confuse_hybrid | 10600 | 200 | 0 | 100.00 |
| attacks_mitm_key_confuse_pqc | 10400 | 200 | 0 | 100.00 |
| attacks_replay_hmac | 197 | 197 | 3 | 98.50 |
| attacks_replay_hybrid | 199 | 197 | 3 | 98.50 |
| attacks_replay_pqc | 197 | 197 | 3 | 98.50 |
| attacks_sig_fuzz_hmac | 200 | 200 | 0 | 100.00 |
| attacks_sig_fuzz_hybrid | 10600 | 200 | 0 | 100.00 |
| attacks_sig_fuzz_pqc | 10400 | 200 | 0 | 100.00 |
| attacks_tamper_auth_hmac | 200 | 200 | 0 | 100.00 |
| attacks_tamper_auth_hybrid | 10600 | 200 | 0 | 100.00 |
| attacks_tamper_auth_pqc | 10400 | 200 | 0 | 100.00 |
| attacks_tamper_payload_hmac | 200 | 200 | 0 | 100.00 |
| attacks_tamper_payload_hybrid | 10600 | 200 | 0 | 100.00 |
| attacks_tamper_payload_pqc | 10400 | 200 | 0 | 100.00 |
| attacks_timing_probe_hmac | 200 | 0 | 200 | 0.00 |
| attacks_timing_probe_hybrid | 10600 | 0 | 200 | 0.00 |
| attacks_timing_probe_pqc | 10400 | 0 | 200 | 0.00 |
| baseline_hmac_r1 | 0 | 0 | 0 | 0.00 |
| baseline_hybrid_r1 | 0 | 0 | 0 | 0.00 |
| baseline_pqc_r1 | 0 | 0 | 0 | 0.00 |
| bus_failure_aggregate | 0 | 0 | 0 | 0.00 |
| bus_failure_clean | 0 | 0 | 0 | 0.00 |
| bus_failure_heavy | 0 | 0 | 0 | 0.00 |
| bus_failure_noisy | 0 | 0 | 0 | 0.00 |
| deadline_relaxed | 0 | 0 | 0 | 0.00 |
| deadline_tight | 0 | 0 | 0 | 0.00 |
| keymismatch_aggregate | 200 | 200 | 0 | 100.00 |
| keymismatch_mismatch | 200 | 200 | 0 | 100.00 |
| keymismatch_shared | 0 | 0 | 0 | 0.00 |
| mixed_bus_can | 0 | 0 | 0 | 0.00 |
| mixed_bus_eth | 0 | 0 | 0 | 0.00 |
| multi_ecu_aggregate | 0 | 0 | 0 | 0.00 |
| multi_ecu_rx0 | 0 | 0 | 0 | 0.00 |
| multi_ecu_rx1 | 0 | 0 | 0 | 0.00 |
| multi_ecu_rx2 | 0 | 0 | 0 | 0.00 |
| multi_ecu_tx | 0 | 0 | 0 | 0.00 |
| persistence_aggregate | 200 | 100 | 100 | 50.00 |
| persistence_no_nvm | 100 | 0 | 100 | 0.00 |
| persistence_with_nvm | 100 | 100 | 0 | 100.00 |
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
| attacks_dos_flood_hmac | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_dos_flood_hybrid | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_dos_flood_pqc | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_downgrade_hmac_hmac | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_downgrade_hmac_hybrid | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_downgrade_hmac_pqc | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_freshness_rollback_hmac | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_freshness_rollback_hybrid | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_freshness_rollback_pqc | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_harvest_now_hmac | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_harvest_now_hybrid | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_harvest_now_pqc | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
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
| attacks_timing_probe_hmac | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_timing_probe_hybrid | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| attacks_timing_probe_pqc | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
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
| attacks_dos_flood_hmac | 34.0 | 34 | 34 | 1.00 | 1 |
| attacks_dos_flood_hybrid | 3343.0 | 3343 | 3343 | 53.00 | 53 |
| attacks_dos_flood_pqc | 3327.0 | 3327 | 3327 | 52.00 | 52 |
| attacks_downgrade_hmac_hmac | 34.0 | 34 | 34 | 1.00 | 1 |
| attacks_downgrade_hmac_hybrid | 3343.0 | 3343 | 3343 | 53.00 | 53 |
| attacks_downgrade_hmac_pqc | 3327.0 | 3327 | 3327 | 52.00 | 52 |
| attacks_freshness_rollback_hmac | 34.0 | 34 | 34 | 1.00 | 1 |
| attacks_freshness_rollback_hybrid | 3343.0 | 3343 | 3343 | 53.00 | 53 |
| attacks_freshness_rollback_pqc | 3327.0 | 3327 | 3327 | 52.00 | 52 |
| attacks_harvest_now_hmac | 34.0 | 34 | 34 | 1.00 | 1 |
| attacks_harvest_now_hybrid | 3343.0 | 3343 | 3343 | 53.00 | 53 |
| attacks_harvest_now_pqc | 3327.0 | 3327 | 3327 | 52.00 | 52 |
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
| attacks_timing_probe_hmac | 34.0 | 34 | 34 | 1.00 | 1 |
| attacks_timing_probe_hybrid | 3343.0 | 3343 | 3343 | 53.00 | 53 |
| attacks_timing_probe_pqc | 3327.0 | 3327 | 3327 | 52.00 | 52 |
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
| attacks_aggregate_hmac | 1400 | 3.64 | 13.71 | 0.09 | 0.13 | 0.00 | 0.00 | 3.89 | 15.31 | 748.90 | 756.52 | 894.83 |
| attacks_aggregate_hybrid | 1400 | 106.60 | 306.05 | 2.16 | 2.60 | 0.00 | 0.00 | 11.40 | 50.31 | 825.55 | 945.70 | 1429.19 |
| attacks_aggregate_pqc | 1400 | 102.85 | 318.17 | 2.11 | 2.44 | 0.00 | 0.00 | 16.73 | 45.87 | 820.17 | 941.87 | 1422.88 |
| attacks_dos_flood_hmac | 200 | 8.46 | 3.35 | 0.10 | 0.13 | 3.79 | 14.87 | 5.20 | 16.45 | 744.85 | 762.40 | 926.12 |
| attacks_dos_flood_hybrid | 200 | 112.69 | 389.40 | 2.08 | 2.38 | 71.58 | 160.06 | 47.57 | 81.36 | 827.01 | 1060.93 | 2031.77 |
| attacks_dos_flood_pqc | 200 | 109.87 | 310.03 | 2.20 | 8.96 | 63.78 | 158.71 | 33.46 | 51.01 | 794.03 | 1003.34 | 2018.20 |
| attacks_downgrade_hmac_hmac | 200 | 2.28 | 7.12 | 0.10 | 0.14 | 2.97 | 12.05 | 4.12 | 13.26 | 735.88 | 745.36 | 823.08 |
| attacks_downgrade_hmac_hybrid | 200 | 110.75 | 359.18 | 2.20 | 7.07 | 57.21 | 135.55 | 5.38 | 14.67 | 780.00 | 955.55 | 1939.66 |
| attacks_downgrade_hmac_pqc | 200 | 102.68 | 253.21 | 2.09 | 2.65 | 56.77 | 147.69 | 32.94 | 59.98 | 747.03 | 941.51 | 1193.49 |
| attacks_freshness_rollback_hmac | 200 | 4.44 | 13.76 | 0.09 | 0.12 | 3.44 | 16.18 | 0.00 | 0.00 | 745.57 | 753.54 | 890.53 |
| attacks_freshness_rollback_hybrid | 200 | 124.55 | 324.77 | 2.33 | 6.75 | 66.19 | 131.07 | 0.00 | 0.00 | 785.94 | 979.01 | 1203.01 |
| attacks_freshness_rollback_pqc | 200 | 103.48 | 387.58 | 2.04 | 2.25 | 56.88 | 129.75 | 0.00 | 0.00 | 746.25 | 908.66 | 1206.36 |
| attacks_harvest_now_hmac | 200 | 7.34 | 3.12 | 0.12 | 0.13 | 3.99 | 17.56 | 4.92 | 17.82 | 739.09 | 755.46 | 926.09 |
| attacks_harvest_now_hybrid | 200 | 117.11 | 342.78 | 2.35 | 2.86 | 74.00 | 179.92 | 44.33 | 79.48 | 772.68 | 1010.47 | 1299.88 |
| attacks_harvest_now_pqc | 200 | 97.43 | 283.85 | 2.04 | 2.36 | 61.52 | 129.11 | 33.53 | 56.50 | 772.09 | 966.61 | 1878.78 |
| attacks_mitm_key_confuse_hmac | 200 | 2.32 | 9.80 | 0.10 | 0.14 | 4.74 | 20.67 | 6.16 | 19.12 | 748.76 | 762.08 | 906.48 |
| attacks_mitm_key_confuse_hybrid | 200 | 116.63 | 395.24 | 2.15 | 2.66 | 90.42 | 178.19 | 7.09 | 19.70 | 766.34 | 982.63 | 1381.17 |
| attacks_mitm_key_confuse_pqc | 200 | 103.50 | 329.56 | 2.04 | 2.51 | 84.83 | 176.24 | 14.15 | 28.60 | 771.10 | 975.62 | 1986.74 |
| attacks_replay_hmac | 200 | 10.63 | 18.49 | 0.10 | 0.13 | 3.67 | 20.66 | 0.07 | 3.68 | 737.88 | 752.35 | 907.09 |
| attacks_replay_hybrid | 200 | 110.19 | 317.47 | 2.07 | 2.36 | 43.96 | 155.01 | 0.54 | 34.55 | 757.85 | 914.60 | 1199.49 |
| attacks_replay_pqc | 200 | 97.55 | 317.73 | 2.05 | 2.54 | 39.26 | 116.02 | 0.56 | 31.54 | 771.78 | 911.19 | 1121.97 |
| attacks_sig_fuzz_hmac | 200 | 2.21 | 9.03 | 0.10 | 0.13 | 3.82 | 20.54 | 4.69 | 14.39 | 750.62 | 761.43 | 903.21 |
| attacks_sig_fuzz_hybrid | 200 | 92.82 | 258.54 | 2.17 | 2.37 | 64.23 | 130.24 | 20.05 | 38.06 | 772.70 | 951.97 | 1950.72 |
| attacks_sig_fuzz_pqc | 200 | 104.17 | 307.45 | 2.03 | 2.35 | 63.81 | 141.31 | 15.14 | 43.15 | 759.85 | 945.01 | 1415.79 |
| attacks_tamper_auth_hmac | 200 | 2.20 | 5.19 | 0.10 | 0.13 | 3.39 | 11.34 | 4.94 | 15.99 | 755.13 | 765.76 | 936.36 |
| attacks_tamper_auth_hybrid | 200 | 108.33 | 276.55 | 2.08 | 2.33 | 63.33 | 138.80 | 30.28 | 63.86 | 780.08 | 984.10 | 1979.23 |
| attacks_tamper_auth_pqc | 200 | 103.42 | 268.82 | 2.08 | 2.50 | 56.64 | 124.23 | 26.50 | 54.39 | 761.88 | 950.52 | 1225.17 |
| attacks_tamper_payload_hmac | 200 | 2.17 | 6.37 | 0.10 | 0.13 | 3.68 | 12.83 | 5.33 | 14.40 | 742.54 | 753.82 | 910.66 |
| attacks_tamper_payload_hybrid | 200 | 106.30 | 323.39 | 2.14 | 2.45 | 59.21 | 122.68 | 16.48 | 50.20 | 748.33 | 932.47 | 1148.61 |
| attacks_tamper_payload_pqc | 200 | 102.62 | 266.35 | 2.08 | 2.60 | 57.11 | 128.18 | 28.62 | 49.27 | 756.97 | 947.40 | 1146.71 |
| attacks_timing_probe_hmac | 200 | 7.63 | 3.13 | 0.10 | 0.12 | 2.96 | 8.30 | 4.51 | 16.75 | 738.51 | 753.72 | 912.79 |
| attacks_timing_probe_hybrid | 200 | 94.88 | 298.76 | 2.26 | 7.31 | 66.50 | 157.87 | 38.64 | 67.75 | 773.00 | 975.27 | 1961.81 |
| attacks_timing_probe_pqc | 200 | 108.13 | 462.92 | 2.47 | 14.51 | 60.59 | 147.61 | 34.10 | 69.65 | 764.80 | 970.10 | 1933.90 |
| baseline_hmac_r1 | 2800 | 2.70 | 4.17 | 0.13 | 0.29 | 4.36 | 16.08 | 5.77 | 20.24 | 747.59 | 760.55 | 920.84 |
| baseline_hybrid_r1 | 2800 | 105.61 | 333.01 | 4.44 | 18.74 | 24.93 | 92.15 | 40.36 | 73.63 | 840.06 | 1015.41 | 2071.65 |
| baseline_pqc_r1 | 2800 | 100.98 | 307.06 | 4.40 | 17.51 | 24.32 | 94.44 | 34.53 | 62.67 | 851.70 | 1015.94 | 2068.87 |
| bus_failure_aggregate | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| bus_failure_clean | 200 | 8.05 | 3.25 | 0.10 | 0.13 | 2.77 | 18.61 | 4.81 | 16.45 | 743.80 | 759.54 | 866.56 |
| bus_failure_heavy | 195 | 2.15 | 3.14 | 0.10 | 0.13 | 2.55 | 15.70 | 4.70 | 14.42 | 743.21 | 752.72 | 876.14 |
| bus_failure_noisy | 200 | 2.08 | 2.95 | 0.09 | 0.12 | 3.17 | 9.08 | 6.09 | 19.32 | 750.13 | 761.57 | 907.28 |
| deadline_relaxed | 200 | 2.34 | 3.27 | 0.09 | 0.12 | 3.42 | 13.58 | 6.37 | 17.44 | 694.66 | 706.88 | 857.97 |
| deadline_tight | 200 | 22.65 | 38.78 | 17.07 | 41.45 | 0.00 | 0.00 | 0.00 | 0.00 | 50131.10 | 50170.81 | 50319.55 |
| keymismatch_aggregate | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| keymismatch_mismatch | 0 | 2.29 | 6.51 | 0.10 | 0.14 | 5.01 | 12.02 | 8.89 | 20.43 | 0.00 | 0.00 | 0.00 |
| keymismatch_shared | 200 | 8.73 | 16.23 | 0.10 | 0.13 | 2.83 | 12.33 | 5.35 | 20.11 | 736.46 | 753.47 | 866.84 |
| mixed_bus_can | 200 | 99.22 | 289.46 | 2.04 | 2.48 | 42.70 | 316.84 | 33.71 | 62.81 | 763.64 | 941.31 | 1202.77 |
| mixed_bus_eth | 200 | 120.36 | 329.89 | 2.27 | 2.40 | 38.73 | 108.42 | 33.25 | 57.17 | 1526.11 | 1720.72 | 2074.92 |
| multi_ecu_aggregate | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| multi_ecu_rx0 | 200 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 5.79 | 20.70 | 760.37 | 766.16 | 921.83 |
| multi_ecu_rx1 | 200 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 5.71 | 15.98 | 756.25 | 761.96 | 902.42 |
| multi_ecu_rx2 | 200 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 6.18 | 15.88 | 753.01 | 759.19 | 896.95 |
| multi_ecu_tx | 0 | 4.19 | 3.95 | 0.12 | 0.14 | 3.18 | 10.86 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| persistence_aggregate | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| persistence_no_nvm | 200 | 4.35 | 3.40 | 0.10 | 0.13 | 3.62 | 13.05 | 6.56 | 30.17 | 759.31 | 773.94 | 992.12 |
| persistence_with_nvm | 200 | 2.44 | 5.39 | 0.10 | 0.13 | 2.71 | 12.24 | 4.00 | 14.42 | 738.93 | 748.18 | 860.87 |
| rekey_r1 | 200 | 10.70 | 28.12 | 13.71 | 66.72 | 0.00 | 0.00 | 12.17 | 61.15 | 0.33 | 36.91 | 104.78 |
| rollover_hmac_r1 | 0 | 38.26 | 17.40 | 0.09 | 0.11 | 3.39 | 7.92 | 4.68 | 16.65 | 0.00 | 0.00 | 0.00 |
| rollover_pqc_r1 | 0 | 137.30 | 305.18 | 2.20 | 2.43 | 79.21 | 97.05 | 27.72 | 53.38 | 0.00 | 0.00 | 0.00 |
| tput_hmac_canfd_r1 | 2627 | 2.55 | 3.27 | 0.10 | 0.13 | 3.13 | 8.88 | 5.30 | 17.21 | 750.23 | 761.29 | 915.45 |
| tput_pqc_eth1000_r1 | 2383 | 100.69 | 309.29 | 0.20 | 0.25 | 5.73 | 19.17 | 35.25 | 62.93 | 697.38 | 839.25 | 1096.26 |
| tput_pqc_eth100_r1 | 2096 | 105.19 | 337.87 | 0.23 | 0.28 | 6.20 | 20.27 | 36.86 | 66.73 | 805.70 | 954.18 | 1214.36 |


## 6. Compliance constraints (time / signal / security)

See [`compliance_constraints.md`](compliance_constraints.md) for the per-scenario pass/fail matrix covering ASIL deadlines, PDU size constraints and AUTOSAR SWS_SecOC clauses.


## 7. Standards compliance matrix

See [`compliance_matrix.md`](compliance_matrix.md) for the mapping to AUTOSAR, FIPS, ISO/SAE 21434 and UN R155.

