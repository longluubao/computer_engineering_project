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

# 10. Wrong-key rejection (SWS_SecOC_00046 cryptographic binding)
run_sc keymismatch_pqc   keymismatch  --protection pqc
run_sc keymismatch_hmac  keymismatch  --protection hmac

# 11. Freshness counter wrap-around + rekey recovery (SWS_SecOC_00033)
run_sc rollover_pqc      rollover     --protection pqc
run_sc rollover_hmac     rollover     --protection hmac

# 12. Tx counter exhaustion (SWS_SecOC_00062)
run_sc freshness_overflow_pqc   freshness_overflow --protection pqc
run_sc freshness_overflow_hmac  freshness_overflow --protection hmac

# 13. Strict-monotonic boundary equality + older-than-last-accepted
#     (SWS_SecOC_00202)
run_sc replay_boundary_pqc      replay_boundary    --protection pqc
run_sc replay_boundary_hmac     replay_boundary    --protection hmac

# 14. Verification disabled passthrough (SWS_SecOC_00265)
run_sc verify_disabled          verify_disabled

# 15. FlexRay TDMA bus coverage (per-protection)
run_sc flexray_pqc              flexray_baseline   --protection pqc
run_sc flexray_hmac             flexray_baseline   --protection hmac
run_sc flexray_hybrid           flexray_baseline   --protection hybrid

# 16. Single-attack runs for the 3 catalogue entries that the
#     'attacks' aggregate excludes by design (DoS, harvest-now,
#     side-channel/timing) so every entry has its own summary file.
#
# Note: each single-attack invocation also writes an
# attacks_aggregate_<prot>_summary.json, which overwrites the proper
# aggregate from step (4). We deliberately re-run sc_attacks (no
# --attack) at step (4b) below so the final aggregate is the multi-
# attack one again.
for prot in pqc hmac hybrid; do
    for kind in 6 9 10; do
        full="attacks_extra_${prot}_k${kind}"
        echo
        echo "=================  ${full}  ================="
        "$RUNNER" --scenario attacks --iterations "$ITER" \
                  --protection "$prot" --attack "$kind" \
                  --out "${OUT}/raw"
        # Move per-attack summary into summary/ now (aggregate is left
        # in raw/ and will be re-written by step 4b).
        for k in dos_flood harvest_now timing_probe; do
            mv "${OUT}/raw/attacks_${k}_${prot}_summary.json" \
               "${OUT}/summary/" 2>/dev/null || true
        done
    done
done

# 4b. Restore the aggregate attacks summary (overwritten by step 16's
#     single-attack runs). Re-running sc_attacks without --attack
#     iterates the 7 detection-measurable kinds and writes
#     attacks_aggregate_<prot>_summary.json with the right totals.
for prot in pqc hmac hybrid; do
    full="attacks_aggregate_restore_${prot}"
    echo
    echo "=================  ${full}  ================="
    "$RUNNER" --scenario attacks --iterations "$ITER" \
              --protection "$prot" \
              --out "${OUT}/raw"
    mv "${OUT}/raw/attacks_aggregate_${prot}_summary.json" \
       "${OUT}/summary/" 2>/dev/null || true
done

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

echo "[ise] generating SWS clause traceability"
python3 "${HERE}/reports/build_sws_traceability.py" \
        --repo-root "${HERE}/.." \
        --out       "${OUT}/summary" \
        || echo "[ise] (sws traceability skipped)"


echo
echo "[ise] all scenarios complete. Results in: ${OUT}"
