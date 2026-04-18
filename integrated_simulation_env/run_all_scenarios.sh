#!/usr/bin/env bash
# Run every thesis scenario and aggregate the output into one folder.
#
# Usage: bash run_all_scenarios.sh [iterations] [repeats]
#
# Output tree:
#   results/<UTC>/
#     raw/<scenario>_<tag>_frames.csv
#     summary/<scenario>_<tag>_summary.json
#     plots/                          (created by generate_thesis_report.py)
#     report.md                       (thesis-ready markdown)

set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RUNNER="${HERE}/build/ise_runner"
if [[ ! -x "$RUNNER" ]]; then
    echo "[ise] ise_runner not found — building first..."
    bash "${HERE}/build.sh"
fi

ITER="${1:-200}"
REP="${2:-1}"

UTC="$(date -u +%Y%m%dT%H%M%SZ)"
OUT="${HERE}/results/${UTC}"
mkdir -p "${OUT}/raw" "${OUT}/summary" "${OUT}/plots"

echo "[ise] output directory: ${OUT}"
echo "[ise] iterations=${ITER}  repeats=${REP}"

# Run one scenario. Args: <tag> <scenario> <extra-flags>
# The tag is part of the filename so different protection/bus combos
# don't overwrite each other.
run_sc () {
    local tag="$1" name="$2"; shift 2
    for r in $(seq 1 "$REP"); do
        local full="${tag}_r${r}"
        echo
        echo "=================  ${full}  ================="
        "$RUNNER" --scenario "$name" --iterations "$ITER" \
                  --out "${OUT}/raw" "$@"
        mv "${OUT}/raw/${name}_summary.json" \
           "${OUT}/summary/${full}_summary.json" 2>/dev/null || true
        mv "${OUT}/raw/${name}_frames.csv"    "${OUT}/raw/${full}_frames.csv"    2>/dev/null || true
        mv "${OUT}/raw/${name}_attacks.csv"   "${OUT}/raw/${full}_attacks.csv"   2>/dev/null || true
        mv "${OUT}/raw/${name}_console.log"   "${OUT}/raw/${full}_console.log"   2>/dev/null || true
    done
}

# 1. Baselines — HMAC vs PQC vs HYBRID, per bus
run_sc baseline_hmac    baseline   --protection hmac
run_sc baseline_pqc     baseline   --protection pqc
run_sc baseline_hybrid  baseline   --protection hybrid

# 2. Throughput — PQC on Ethernet (the thesis hero case)
run_sc tput_pqc_eth100   throughput --protection pqc  --bus eth100
run_sc tput_pqc_eth1000  throughput --protection pqc  --bus eth1000
run_sc tput_hmac_canfd   throughput --protection hmac --bus canfd

# 3. Mixed-bus gateway
run_sc mixed_bus_pqc     mixed_bus  --protection pqc

# 4. Attacks (runs all 10 kinds internally)
run_sc attacks_pqc       attacks    --protection pqc
run_sc attacks_hmac      attacks    --protection hmac
run_sc attacks_hybrid    attacks    --protection hybrid

# 5. Rekey stress
run_sc rekey             rekey

# 6. NvM freshness persistence (SWS_SecOC_00194)
run_sc persistence_pqc   persistence  --protection pqc
run_sc persistence_hmac  persistence  --protection hmac

# 7. Physical-layer bus-error / BER sweep (CanSM / EthSM surface)
run_sc bus_failure_pqc   bus_failure  --protection pqc
run_sc bus_failure_hmac  bus_failure  --protection hmac

# 8. Deadline-stress (per-ASIL class deadline miss counters)
run_sc deadline_pqc      deadline_stress --protection pqc
run_sc deadline_hmac     deadline_stress --protection hmac

# 9. Multi-ECU broadcast (1 TX : N RX)
run_sc multi_ecu_pqc     multi_ecu    --protection pqc
run_sc multi_ecu_hmac    multi_ecu    --protection hmac

echo
# Some scenarios (attacks, mixed_bus) emit multiple summary files that
# don't match their scenario name. Sweep everything into summary/.
find "${OUT}/raw" -maxdepth 1 -name '*_summary.json' -exec mv {} "${OUT}/summary/" \; 2>/dev/null || true

echo "[ise] generating thesis report"
python3 "${HERE}/reports/generate_thesis_report.py" \
        --input  "${OUT}" \
        --output "${OUT}/report.md" || echo "[ise] (python report skipped)"

echo "[ise] generating cross-environment comparison"
python3 "${HERE}/reports/compare_environments.py" \
        --ise     "${OUT}" \
        --pi      "${HERE}/../PiTest" \
        --win     "${HERE}/../Autosar_SecOC/test_logs" \
        --output  "${OUT}/summary/cross_env_comparison.md" \
        || echo "[ise] (cross-env comparison skipped)"


echo
echo "[ise] all scenarios complete. Results in: ${OUT}"
