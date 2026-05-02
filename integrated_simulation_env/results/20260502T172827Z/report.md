# Integrated Simulation Environment — Thesis Report

Generated automatically by `generate_thesis_report.py`.

Number of scenarios collected: **69**.


## 1. Latency and overhead

Raw numbers in `summary/latency_stats.csv`. The table below is copy-pasteable into LaTeX via `pgfplotstable`.

| scenario | samples | e2e_mean_us | e2e_p50_us | e2e_p95_us | e2e_p99_us | e2e_p999_us | auth_mean_us | verify_mean_us | cantp_mean_us | bus_mean_us |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| attacks_aggregate_hmac | 1400 | 757.51 | 743.60 | 860.43 | 915.40 | 948.10 | 4.11 | 3.84 | 0.09 | 0.00 |
| attacks_aggregate_hybrid | 1400 | 956.02 | 932.84 | 1134.52 | 1344.02 | 1983.53 | 109.62 | 12.44 | 2.24 | 0.00 |
| attacks_aggregate_pqc | 1400 | 974.10 | 920.16 | 1104.75 | 1299.30 | 2140.00 | 105.30 | 17.12 | 2.08 | 0.00 |
| attacks_dos_flood_hmac | 200 | 747.71 | 734.15 | 817.74 | 913.55 | 993.27 | 7.43 | 4.65 | 0.10 | 3.85 |
| attacks_dos_flood_hybrid | 200 | 1000.95 | 968.76 | 1156.35 | 1965.48 | 1978.15 | 110.11 | 40.37 | 2.10 | 64.80 |
| attacks_dos_flood_pqc | 200 | 988.40 | 955.54 | 1162.42 | 2016.07 | 2098.41 | 112.60 | 34.36 | 2.03 | 65.70 |
| attacks_downgrade_hmac_hmac | 200 | 769.51 | 752.36 | 865.61 | 916.31 | 926.68 | 2.13 | 5.32 | 0.09 | 3.78 |
| attacks_downgrade_hmac_hybrid | 200 | 936.54 | 903.52 | 1110.21 | 1272.59 | 1331.21 | 106.83 | 6.82 | 2.12 | 65.27 |
| attacks_downgrade_hmac_pqc | 200 | 997.53 | 966.93 | 1156.57 | 2004.25 | 2046.61 | 98.01 | 36.55 | 2.04 | 65.52 |
| attacks_freshness_rollback_hmac | 200 | 785.70 | 767.60 | 891.13 | 944.35 | 962.25 | 8.39 | 0.00 | 0.10 | 5.89 |
| attacks_freshness_rollback_hybrid | 200 | 981.86 | 941.55 | 1165.25 | 2000.49 | 2025.96 | 114.42 | 0.00 | 2.09 | 65.63 |
| attacks_freshness_rollback_pqc | 200 | 919.17 | 888.36 | 1078.73 | 1222.76 | 1223.39 | 101.43 | 0.00 | 2.04 | 58.13 |
| attacks_harvest_now_hmac | 200 | 749.40 | 733.86 | 820.44 | 903.59 | 911.07 | 8.04 | 4.92 | 0.09 | 3.52 |
| attacks_harvest_now_hybrid | 200 | 1006.34 | 971.37 | 1195.29 | 2039.74 | 2065.01 | 107.78 | 43.98 | 2.12 | 67.02 |
| attacks_harvest_now_pqc | 200 | 976.63 | 940.23 | 1141.09 | 1958.74 | 1961.51 | 107.41 | 33.78 | 2.12 | 64.95 |
| attacks_mitm_key_confuse_hmac | 200 | 759.55 | 740.29 | 847.41 | 894.58 | 903.51 | 2.21 | 5.89 | 0.09 | 4.58 |
| attacks_mitm_key_confuse_hybrid | 200 | 967.33 | 936.99 | 1167.05 | 1890.62 | 1945.52 | 105.29 | 5.37 | 2.06 | 84.42 |
| attacks_mitm_key_confuse_pqc | 200 | 971.13 | 952.08 | 1148.33 | 1958.37 | 1999.65 | 99.87 | 16.31 | 2.06 | 85.74 |
| attacks_replay_hmac | 200 | 763.20 | 736.85 | 863.08 | 937.07 | 1097.30 | 11.87 | 0.15 | 0.10 | 4.70 |
| attacks_replay_hybrid | 200 | 939.36 | 905.02 | 1116.17 | 1197.82 | 1281.38 | 119.61 | 0.87 | 2.06 | 44.83 |
| attacks_replay_pqc | 200 | 931.76 | 911.38 | 1062.32 | 1240.53 | 1281.86 | 114.45 | 0.62 | 2.09 | 49.67 |
| attacks_sig_fuzz_hmac | 200 | 767.78 | 752.10 | 874.60 | 921.61 | 923.25 | 2.26 | 5.48 | 0.10 | 4.20 |
| attacks_sig_fuzz_hybrid | 200 | 970.27 | 950.85 | 1110.01 | 1260.37 | 1934.31 | 103.73 | 22.61 | 2.21 | 67.58 |
| attacks_sig_fuzz_pqc | 200 | 942.68 | 926.82 | 1062.48 | 1197.21 | 1239.18 | 101.15 | 14.21 | 2.06 | 68.24 |
| attacks_tamper_auth_hmac | 200 | 759.64 | 744.04 | 849.12 | 899.75 | 903.85 | 2.20 | 5.90 | 0.10 | 4.42 |
| attacks_tamper_auth_hybrid | 200 | 940.18 | 919.91 | 1071.69 | 1170.63 | 1198.57 | 109.14 | 30.22 | 2.13 | 55.69 |
| attacks_tamper_auth_pqc | 200 | 969.32 | 940.64 | 1109.50 | 1313.86 | 1976.83 | 102.42 | 28.58 | 2.03 | 60.07 |
| attacks_tamper_payload_hmac | 200 | 766.04 | 751.83 | 848.12 | 903.42 | 939.84 | 2.25 | 5.50 | 0.10 | 4.45 |
| attacks_tamper_payload_hybrid | 200 | 924.17 | 903.74 | 1057.39 | 1143.64 | 1145.86 | 100.63 | 17.27 | 2.24 | 58.08 |
| attacks_tamper_payload_pqc | 200 | 955.25 | 934.85 | 1098.41 | 1166.31 | 1170.49 | 97.43 | 30.09 | 2.06 | 60.07 |
| attacks_timing_probe_hmac | 200 | 758.53 | 741.59 | 845.67 | 909.68 | 935.73 | 7.63 | 5.15 | 0.09 | 3.97 |
| attacks_timing_probe_hybrid | 200 | 982.28 | 963.07 | 1133.73 | 1338.94 | 1858.22 | 102.95 | 45.78 | 2.11 | 62.56 |
| attacks_timing_probe_pqc | 200 | 958.92 | 933.84 | 1129.10 | 1281.93 | 1339.33 | 106.39 | 33.37 | 2.38 | 62.96 |
| baseline_hmac_r1 | 2800 | 775.24 | 759.57 | 893.25 | 963.85 | 1432.32 | 2.83 | 7.09 | 0.14 | 4.98 |
| baseline_hybrid_r1 | 2800 | 1065.52 | 972.22 | 1985.58 | 2149.82 | 2304.16 | 106.98 | 43.18 | 4.56 | 26.75 |
| baseline_pqc_r1 | 2800 | 1090.93 | 984.35 | 2022.61 | 2162.74 | 2334.76 | 111.93 | 38.98 | 4.60 | 28.15 |
| bus_failure_aggregate | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| bus_failure_clean | 200 | 768.69 | 746.84 | 876.87 | 908.62 | 925.43 | 7.99 | 6.62 | 0.09 | 3.40 |
| bus_failure_heavy | 195 | 760.51 | 746.80 | 848.89 | 931.45 | 951.54 | 2.14 | 5.32 | 0.09 | 3.01 |
| bus_failure_noisy | 200 | 786.51 | 758.40 | 888.00 | 935.45 | 1038.18 | 2.20 | 7.53 | 0.09 | 4.18 |
| deadline_relaxed | 200 | 729.77 | 705.81 | 842.65 | 880.77 | 888.74 | 2.41 | 8.29 | 0.10 | 4.78 |
| deadline_tight | 200 | 50172.85 | 50156.42 | 50228.32 | 50318.90 | 50396.59 | 21.43 | 0.00 | 16.27 | 0.00 |
| flexray_hmac_r1 | 600 | 742.55 | 729.27 | 834.84 | 877.67 | 928.68 | 6.07 | 5.32 | 3.43 | 2.22 |
| flexray_hybrid_r1 | 600 | 911.62 | 878.25 | 1075.41 | 1262.48 | 1985.91 | 111.39 | 38.72 | 4.77 | 16.55 |
| flexray_pqc_r1 | 600 | 912.19 | 885.40 | 1058.54 | 1179.14 | 1872.62 | 106.41 | 35.62 | 5.19 | 16.79 |
| freshness_overflow_hmac_r1 | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 122.66 | 0.72 | 0.09 | 1.85 |
| freshness_overflow_pqc_r1 | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 91.07 | 3.75 | 3.43 | 80.18 |
| keymismatch_aggregate | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| keymismatch_mismatch | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 2.26 | 7.45 | 0.10 | 5.01 |
| keymismatch_shared | 200 | 765.67 | 753.72 | 840.56 | 885.03 | 936.81 | 8.33 | 6.15 | 0.10 | 3.44 |
| mixed_bus_can | 200 | 947.36 | 929.03 | 1072.94 | 1206.20 | 1259.30 | 97.18 | 35.12 | 2.21 | 40.38 |
| mixed_bus_eth | 200 | 1748.61 | 1723.93 | 1973.48 | 2194.66 | 2215.22 | 133.32 | 34.91 | 2.08 | 44.72 |
| multi_ecu_aggregate | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| multi_ecu_rx0 | 200 | 760.09 | 743.71 | 837.07 | 919.94 | 933.87 | 0.00 | 5.24 | 0.00 | 0.00 |
| multi_ecu_rx1 | 200 | 751.60 | 741.87 | 832.75 | 883.59 | 884.88 | 0.00 | 4.90 | 0.00 | 0.00 |
| multi_ecu_rx2 | 200 | 756.47 | 744.04 | 849.16 | 914.44 | 923.38 | 0.00 | 5.44 | 0.00 | 0.00 |
| multi_ecu_tx | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 3.94 | 0.00 | 0.10 | 2.79 |
| persistence_aggregate | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| persistence_no_nvm | 200 | 753.97 | 737.04 | 827.45 | 853.40 | 855.40 | 4.33 | 5.50 | 0.10 | 3.05 |
| persistence_with_nvm | 200 | 734.46 | 727.16 | 787.75 | 841.17 | 849.98 | 2.64 | 4.09 | 0.11 | 2.65 |
| rekey_r1 | 200 | 34.45 | 32.51 | 43.76 | 76.59 | 79.20 | 10.32 | 11.30 | 12.51 | 0.00 |
| replay_boundary_hmac_r1 | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 21.03 | 1.45 | 0.10 | 2.62 |
| replay_boundary_pqc_r1 | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 100.65 | 9.13 | 2.18 | 80.97 |
| rollover_hmac_r1 | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 39.92 | 5.54 | 0.10 | 3.61 |
| rollover_pqc_r1 | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 98.42 | 28.77 | 2.06 | 80.96 |
| tput_hmac_canfd_r1 | 2575 | 776.69 | 755.40 | 874.09 | 933.15 | 1038.71 | 2.64 | 6.32 | 0.10 | 3.61 |
| tput_pqc_eth1000_r1 | 2328 | 859.07 | 836.47 | 1017.01 | 1122.34 | 1205.25 | 104.75 | 37.17 | 0.20 | 6.37 |
| tput_pqc_eth100_r1 | 2044 | 978.31 | 958.21 | 1135.76 | 1243.40 | 1370.40 | 105.40 | 38.33 | 0.21 | 7.08 |
| verify_disabled_r1 | 200 | 727.02 | 714.39 | 811.51 | 866.35 | 870.08 | 0.05 | 0.05 | 0.11 | 3.15 |


## 2. Attack detection (UN R155 Annex 5 mapping)

Source: `summary/attack_detection.csv`.

| scenario | attacks_injected | attacks_detected | attacks_delivered | detection_rate_pct |
| --- | --- | --- | --- | --- |
| attacks_aggregate_hmac | 1397 | 1197 | 203 | 85.50 |
| attacks_aggregate_hybrid | 63798 | 1397 | 3 | 99.79 |
| attacks_aggregate_pqc | 62599 | 1397 | 3 | 99.79 |
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
| attacks_replay_hybrid | 200 | 197 | 3 | 98.50 |
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
| flexray_hmac_r1 | 0 | 0 | 0 | 0.00 |
| flexray_hybrid_r1 | 0 | 0 | 0 | 0.00 |
| flexray_pqc_r1 | 0 | 0 | 0 | 0.00 |
| freshness_overflow_hmac_r1 | 8 | 8 | 0 | 100.00 |
| freshness_overflow_pqc_r1 | 8 | 8 | 0 | 100.00 |
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
| replay_boundary_hmac_r1 | 47 | 47 | 0 | 100.00 |
| replay_boundary_pqc_r1 | 47 | 47 | 0 | 100.00 |
| rollover_hmac_r1 | 8 | 8 | 0 | 100.00 |
| rollover_pqc_r1 | 8 | 8 | 0 | 100.00 |
| tput_hmac_canfd_r1 | 0 | 0 | 0 | 0.00 |
| tput_pqc_eth1000_r1 | 0 | 0 | 0 | 0.00 |
| tput_pqc_eth100_r1 | 0 | 0 | 0 | 0.00 |
| verify_disabled_r1 | 50 | 0 | 50 | 0.00 |


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
| flexray_hmac_r1 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| flexray_hybrid_r1 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| flexray_pqc_r1 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| freshness_overflow_hmac_r1 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| freshness_overflow_pqc_r1 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
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
| replay_boundary_hmac_r1 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| replay_boundary_pqc_r1 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| rollover_hmac_r1 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| rollover_pqc_r1 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| tput_hmac_canfd_r1 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| tput_pqc_eth1000_r1 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| tput_pqc_eth100_r1 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| verify_disabled_r1 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |


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
| flexray_hmac_r1 | 99.5 | 280 | 280 | 0.75 | 1 |
| flexray_hybrid_r1 | 3408.5 | 3589 | 3589 | 39.75 | 53 |
| flexray_pqc_r1 | 3392.5 | 3573 | 3573 | 39.25 | 53 |
| freshness_overflow_hmac_r1 | 34.0 | 34 | 34 | 1.00 | 1 |
| freshness_overflow_pqc_r1 | 3327.0 | 3327 | 3327 | 52.00 | 52 |
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
| replay_boundary_hmac_r1 | 34.0 | 34 | 34 | 1.00 | 1 |
| replay_boundary_pqc_r1 | 3327.0 | 3327 | 3327 | 52.00 | 52 |
| rollover_hmac_r1 | 34.0 | 34 | 34 | 1.00 | 1 |
| rollover_pqc_r1 | 3327.0 | 3327 | 3327 | 52.00 | 52 |
| tput_hmac_canfd_r1 | 34.0 | 34 | 34 | 1.00 | 1 |
| tput_pqc_eth1000_r1 | 4343.0 | 4343 | 4343 | 3.00 | 3 |
| tput_pqc_eth100_r1 | 4343.0 | 4343 | 4343 | 3.00 | 3 |
| verify_disabled_r1 | 18.0 | 18 | 18 | 1.00 | 1 |


## 5. Per-layer AUTOSAR latency breakdown

Source: `summary/layer_latency.csv` and `summary/flow_timeline.md`. Splits the end-to-end latency into Com/PduR, SecOC build, Csm sign, CanTP fragmentation, bus transit and Csm verify.

| scenario | samples | csm_sign_mean_us | csm_sign_p99_us | cantp_frag_mean_us | cantp_frag_p99_us | bus_transit_mean_us | bus_transit_p99_us | csm_verify_mean_us | csm_verify_p99_us | com_pdur_secoc_mean_us | e2e_mean_us | e2e_p99_us |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| attacks_aggregate_hmac | 1400 | 4.11 | 15.71 | 0.09 | 0.13 | 0.00 | 0.00 | 3.84 | 14.91 | 749.47 | 757.51 | 915.40 |
| attacks_aggregate_hybrid | 1400 | 109.62 | 358.38 | 2.24 | 5.36 | 0.00 | 0.00 | 12.44 | 53.58 | 831.72 | 956.02 | 1344.02 |
| attacks_aggregate_pqc | 1400 | 105.30 | 338.06 | 2.08 | 2.52 | 0.00 | 0.00 | 17.12 | 49.00 | 849.61 | 974.10 | 1299.30 |
| attacks_dos_flood_hmac | 200 | 7.43 | 3.16 | 0.10 | 0.15 | 3.85 | 17.63 | 4.65 | 14.74 | 731.68 | 747.71 | 913.55 |
| attacks_dos_flood_hybrid | 200 | 110.11 | 290.30 | 2.10 | 2.59 | 64.80 | 152.15 | 40.37 | 78.18 | 783.58 | 1000.95 | 1965.48 |
| attacks_dos_flood_pqc | 200 | 112.60 | 325.87 | 2.03 | 2.29 | 65.70 | 161.40 | 34.36 | 53.58 | 773.71 | 988.40 | 2016.07 |
| attacks_downgrade_hmac_hmac | 200 | 2.13 | 2.91 | 0.09 | 0.13 | 3.78 | 14.01 | 5.32 | 14.46 | 758.18 | 769.51 | 916.31 |
| attacks_downgrade_hmac_hybrid | 200 | 106.83 | 342.66 | 2.12 | 2.67 | 65.27 | 190.64 | 6.82 | 30.23 | 755.50 | 936.54 | 1272.59 |
| attacks_downgrade_hmac_pqc | 200 | 98.01 | 272.95 | 2.04 | 2.56 | 65.52 | 180.13 | 36.55 | 68.36 | 795.40 | 997.53 | 2004.25 |
| attacks_freshness_rollback_hmac | 200 | 8.39 | 19.66 | 0.10 | 0.14 | 5.89 | 19.29 | 0.00 | 0.00 | 771.32 | 785.70 | 944.35 |
| attacks_freshness_rollback_hybrid | 200 | 114.42 | 324.48 | 2.09 | 2.42 | 65.63 | 150.26 | 0.00 | 0.00 | 799.73 | 981.86 | 2000.49 |
| attacks_freshness_rollback_pqc | 200 | 101.43 | 312.94 | 2.04 | 2.16 | 58.13 | 132.67 | 0.00 | 0.00 | 757.57 | 919.17 | 1222.76 |
| attacks_harvest_now_hmac | 200 | 8.04 | 21.07 | 0.09 | 0.14 | 3.52 | 11.18 | 4.92 | 18.95 | 732.82 | 749.40 | 903.59 |
| attacks_harvest_now_hybrid | 200 | 107.78 | 318.55 | 2.12 | 2.46 | 67.02 | 191.95 | 43.98 | 70.95 | 785.44 | 1006.34 | 2039.74 |
| attacks_harvest_now_pqc | 200 | 107.41 | 411.65 | 2.12 | 7.11 | 64.95 | 225.61 | 33.78 | 60.21 | 768.36 | 976.63 | 1958.74 |
| attacks_mitm_key_confuse_hmac | 200 | 2.21 | 5.76 | 0.09 | 0.12 | 4.58 | 21.31 | 5.89 | 18.18 | 746.78 | 759.55 | 894.58 |
| attacks_mitm_key_confuse_hybrid | 200 | 105.29 | 281.92 | 2.06 | 2.45 | 84.42 | 159.49 | 5.37 | 14.99 | 770.19 | 967.33 | 1890.62 |
| attacks_mitm_key_confuse_pqc | 200 | 99.87 | 358.43 | 2.06 | 2.34 | 85.74 | 152.96 | 16.31 | 43.28 | 767.16 | 971.13 | 1958.37 |
| attacks_replay_hmac | 200 | 11.87 | 25.24 | 0.10 | 0.14 | 4.70 | 26.65 | 0.15 | 4.66 | 746.37 | 763.20 | 937.07 |
| attacks_replay_hybrid | 200 | 119.61 | 331.42 | 2.06 | 2.56 | 44.83 | 214.31 | 0.87 | 34.73 | 771.99 | 939.36 | 1197.82 |
| attacks_replay_pqc | 200 | 114.45 | 367.53 | 2.09 | 5.21 | 49.67 | 130.55 | 0.62 | 35.30 | 764.93 | 931.76 | 1240.53 |
| attacks_sig_fuzz_hmac | 200 | 2.26 | 8.73 | 0.10 | 0.13 | 4.20 | 21.79 | 5.48 | 14.54 | 755.75 | 767.78 | 921.61 |
| attacks_sig_fuzz_hybrid | 200 | 103.73 | 305.48 | 2.21 | 5.79 | 67.58 | 153.19 | 22.61 | 37.50 | 774.14 | 970.27 | 1260.37 |
| attacks_sig_fuzz_pqc | 200 | 101.15 | 268.98 | 2.06 | 2.43 | 68.24 | 161.87 | 14.21 | 24.68 | 757.01 | 942.68 | 1197.21 |
| attacks_tamper_auth_hmac | 200 | 2.20 | 7.65 | 0.10 | 0.14 | 4.42 | 17.33 | 5.90 | 16.07 | 747.01 | 759.64 | 899.75 |
| attacks_tamper_auth_hybrid | 200 | 109.14 | 299.94 | 2.13 | 2.30 | 55.69 | 137.79 | 30.22 | 58.39 | 743.00 | 940.18 | 1170.63 |
| attacks_tamper_auth_pqc | 200 | 102.42 | 277.00 | 2.03 | 2.31 | 60.07 | 142.85 | 28.58 | 77.30 | 776.22 | 969.32 | 1313.86 |
| attacks_tamper_payload_hmac | 200 | 2.25 | 3.40 | 0.10 | 0.13 | 4.45 | 27.45 | 5.50 | 16.40 | 753.74 | 766.04 | 903.42 |
| attacks_tamper_payload_hybrid | 200 | 100.63 | 248.84 | 2.24 | 6.43 | 58.08 | 139.73 | 17.27 | 56.37 | 745.95 | 924.17 | 1143.64 |
| attacks_tamper_payload_pqc | 200 | 97.43 | 335.84 | 2.06 | 2.32 | 60.07 | 133.75 | 30.09 | 49.56 | 765.60 | 955.25 | 1166.31 |
| attacks_timing_probe_hmac | 200 | 7.63 | 2.94 | 0.09 | 0.13 | 3.97 | 18.27 | 5.15 | 16.59 | 741.69 | 758.53 | 909.68 |
| attacks_timing_probe_hybrid | 200 | 102.95 | 344.02 | 2.11 | 2.56 | 62.56 | 172.05 | 45.78 | 115.74 | 768.88 | 982.28 | 1338.94 |
| attacks_timing_probe_pqc | 200 | 106.39 | 338.56 | 2.38 | 6.78 | 62.96 | 212.04 | 33.37 | 50.27 | 753.82 | 958.92 | 1281.93 |
| baseline_hmac_r1 | 2800 | 2.83 | 7.28 | 0.14 | 0.31 | 4.98 | 19.73 | 7.09 | 20.62 | 760.20 | 775.24 | 963.85 |
| baseline_hybrid_r1 | 2800 | 106.98 | 337.55 | 4.56 | 21.51 | 26.75 | 105.99 | 43.18 | 84.36 | 884.05 | 1065.52 | 2149.82 |
| baseline_pqc_r1 | 2800 | 111.93 | 365.89 | 4.60 | 21.05 | 28.15 | 107.09 | 38.98 | 77.29 | 907.26 | 1090.93 | 2162.74 |
| bus_failure_aggregate | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| bus_failure_clean | 200 | 7.99 | 6.49 | 0.09 | 0.12 | 3.40 | 7.98 | 6.62 | 23.48 | 750.58 | 768.69 | 908.62 |
| bus_failure_heavy | 195 | 2.14 | 3.31 | 0.09 | 0.12 | 3.01 | 13.22 | 5.32 | 15.92 | 749.94 | 760.51 | 931.45 |
| bus_failure_noisy | 200 | 2.20 | 5.58 | 0.09 | 0.12 | 4.18 | 15.25 | 7.53 | 18.25 | 772.51 | 786.51 | 935.45 |
| deadline_relaxed | 200 | 2.41 | 3.45 | 0.10 | 0.15 | 4.78 | 14.84 | 8.29 | 22.54 | 714.19 | 729.77 | 880.77 |
| deadline_tight | 200 | 21.43 | 75.50 | 16.27 | 48.72 | 0.00 | 0.00 | 0.00 | 0.00 | 50135.15 | 50172.85 | 50318.90 |
| flexray_hmac_r1 | 600 | 6.07 | 17.17 | 3.43 | 21.61 | 2.22 | 8.26 | 5.32 | 16.03 | 725.51 | 742.55 | 877.67 |
| flexray_hybrid_r1 | 600 | 111.39 | 349.16 | 4.77 | 21.18 | 16.55 | 97.25 | 38.72 | 73.92 | 740.19 | 911.62 | 1262.48 |
| flexray_pqc_r1 | 600 | 106.41 | 329.23 | 5.19 | 25.56 | 16.79 | 96.26 | 35.62 | 67.62 | 748.18 | 912.19 | 1179.14 |
| freshness_overflow_hmac_r1 | 0 | 122.66 | 4.20 | 0.09 | 0.10 | 1.85 | 2.31 | 0.72 | 0.00 | 0.00 | 0.00 | 0.00 |
| freshness_overflow_pqc_r1 | 0 | 91.07 | 131.36 | 3.43 | 2.07 | 80.18 | 94.20 | 3.75 | 0.00 | 0.00 | 0.00 | 0.00 |
| keymismatch_aggregate | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| keymismatch_mismatch | 0 | 2.26 | 7.30 | 0.10 | 0.14 | 5.01 | 32.62 | 7.45 | 17.56 | 0.00 | 0.00 | 0.00 |
| keymismatch_shared | 200 | 8.33 | 8.82 | 0.10 | 0.14 | 3.44 | 11.14 | 6.15 | 20.52 | 747.65 | 765.67 | 885.03 |
| mixed_bus_can | 200 | 97.18 | 295.51 | 2.21 | 2.54 | 40.38 | 149.20 | 35.12 | 64.11 | 772.48 | 947.36 | 1206.20 |
| mixed_bus_eth | 200 | 133.32 | 445.65 | 2.08 | 2.58 | 44.72 | 320.39 | 34.91 | 64.48 | 1533.58 | 1748.61 | 2194.66 |
| multi_ecu_aggregate | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| multi_ecu_rx0 | 200 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 5.24 | 19.64 | 754.85 | 760.09 | 919.94 |
| multi_ecu_rx1 | 200 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 4.90 | 14.47 | 746.70 | 751.60 | 883.59 |
| multi_ecu_rx2 | 200 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 5.44 | 18.98 | 751.03 | 756.47 | 914.44 |
| multi_ecu_tx | 0 | 3.94 | 3.29 | 0.10 | 0.14 | 2.79 | 7.59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| persistence_aggregate | 0 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| persistence_no_nvm | 200 | 4.33 | 3.28 | 0.10 | 0.14 | 3.05 | 8.72 | 5.50 | 20.41 | 740.98 | 753.97 | 853.40 |
| persistence_with_nvm | 200 | 2.64 | 11.46 | 0.11 | 0.14 | 2.65 | 8.62 | 4.09 | 16.54 | 724.98 | 734.46 | 841.17 |
| rekey_r1 | 200 | 10.32 | 23.51 | 12.51 | 19.98 | 0.00 | 0.00 | 11.30 | 29.16 | 0.33 | 34.45 | 76.59 |
| replay_boundary_hmac_r1 | 0 | 21.03 | 21.10 | 0.10 | 0.13 | 2.62 | 6.67 | 1.45 | 6.21 | 0.00 | 0.00 | 0.00 |
| replay_boundary_pqc_r1 | 0 | 100.65 | 289.30 | 2.18 | 5.15 | 80.97 | 179.04 | 9.13 | 39.95 | 0.00 | 0.00 | 0.00 |
| rollover_hmac_r1 | 0 | 39.92 | 14.97 | 0.10 | 0.13 | 3.61 | 7.73 | 5.54 | 15.77 | 0.00 | 0.00 | 0.00 |
| rollover_pqc_r1 | 0 | 98.42 | 234.30 | 2.06 | 2.25 | 80.96 | 130.91 | 28.77 | 57.27 | 0.00 | 0.00 | 0.00 |
| tput_hmac_canfd_r1 | 2575 | 2.64 | 5.68 | 0.10 | 0.13 | 3.61 | 13.54 | 6.32 | 20.95 | 764.01 | 776.69 | 933.15 |
| tput_pqc_eth1000_r1 | 2328 | 104.75 | 296.16 | 0.20 | 0.25 | 6.37 | 23.96 | 37.17 | 67.20 | 710.59 | 859.07 | 1122.34 |
| tput_pqc_eth100_r1 | 2044 | 105.40 | 308.09 | 0.21 | 0.27 | 7.08 | 23.16 | 38.33 | 79.11 | 827.29 | 978.31 | 1243.40 |
| verify_disabled_r1 | 200 | 0.05 | 0.07 | 0.11 | 0.14 | 3.15 | 11.65 | 0.05 | 0.08 | 723.66 | 727.02 | 866.35 |


## 6. Compliance constraints (time / signal / security)

See [`compliance_constraints.md`](compliance_constraints.md) for the per-scenario pass/fail matrix covering ASIL deadlines, PDU size constraints and AUTOSAR SWS_SecOC clauses.


## 7. Standards compliance matrix

See [`compliance_matrix.md`](compliance_matrix.md) for the mapping to AUTOSAR, FIPS, ISO/SAE 21434 and UN R155.

