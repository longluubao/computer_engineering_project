#!/usr/bin/env bash
# Run every thesis scenario and aggregate the output into one folder.
#
# Usage: bash run_all_scenarios.sh [iterations] [repeats]
#
# Output tree:
#   results/<UTC>/
#     raw/<scenario>_frames.csv
#     summary/<scenario>_summary.json
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

run_sc () {
    local name="$1" extra="${2:-}"
    for r in $(seq 1 "$REP"); do
        local tag="${name}_r${r}"
        echo
        echo "=================  ${tag}  ================="
        "$RUNNER" --scenario "$name" --iterations "$ITER" \
                  --out "${OUT}/raw" $extra
        mv "${OUT}/raw/${name}_summary.json" \
           "${OUT}/summary/${tag}_summary.json" 2>/dev/null || true
        mv "${OUT}/raw/${name}_frames.csv"    "${OUT}/raw/${tag}_frames.csv"    2>/dev/null || true
        mv "${OUT}/raw/${name}_attacks.csv"   "${OUT}/raw/${tag}_attacks.csv"   2>/dev/null || true
        mv "${OUT}/raw/${name}_console.log"   "${OUT}/raw/${tag}_console.log"   2>/dev/null || true
    done
}

# 1. Baselines — HMAC vs PQC vs HYBRID, per bus
run_sc baseline "--protection hmac"
run_sc baseline "--protection pqc"
run_sc baseline "--protection hybrid"

# 2. Throughput — PQC on Ethernet (the thesis hero case)
run_sc throughput "--protection pqc --bus eth100"
run_sc throughput "--protection pqc --bus eth1000"
run_sc throughput "--protection hmac --bus canfd"

# 3. Mixed-bus gateway
run_sc mixed_bus  "--protection pqc"

# 4. Attacks (runs all 10 kinds internally)
run_sc attacks    "--protection pqc"
run_sc attacks    "--protection hmac"

# 5. Rekey stress
run_sc rekey      ""

echo
echo "[ise] generating thesis report"
python3 "${HERE}/reports/generate_thesis_report.py" \
        --input  "${OUT}" \
        --output "${OUT}/report.md" || echo "[ise] (python report skipped)"

echo
echo "[ise] all scenarios complete. Results in: ${OUT}"
