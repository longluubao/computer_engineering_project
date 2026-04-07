#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=common.sh
source "${SCRIPT_DIR}/common.sh"

if [[ "${1:-}" == "--allow-stub" ]]; then
    ALLOW_STUB="true"
fi

export RUN_ID
export RUN_DIR
export ALLOW_STUB
export COMPLIANCE_EXT_ROOT

ensure_report_dir

CHECKS=(
    "misra-report.sh"
    "aspice-audit.sh"
    "iso26262-checklist.sh"
    "unr155-assess.sh"
    "homologation-check.sh"
)

STATUS_FILE="${RUN_DIR}/run-all-status.txt"
CHECKS_JSON="${RUN_DIR}/checks.json"
MANIFEST_FILE="${RUN_DIR}/manifest.json"
OVERALL_RC=0
STARTED_AT="$(date -u +"%Y-%m-%dT%H:%M:%SZ")"

{
    echo "Compliance run started: ${RUN_ID}"
    echo "Project root: ${PROJECT_ROOT}"
    echo "Allow stub: ${ALLOW_STUB}"
    echo "External root: ${COMPLIANCE_EXT_ROOT}"
    echo
} > "${STATUS_FILE}"
echo "[" > "${CHECKS_JSON}"

first=true
for check in "${CHECKS[@]}"; do
    check_started="$(date -u +"%Y-%m-%dT%H:%M:%SZ")"
    log_info "Running ${check}"
    set +e
    bash "${SCRIPT_DIR}/${check}"
    rc=$?
    set -e
    check_finished="$(date -u +"%Y-%m-%dT%H:%M:%SZ")"

    if [[ $rc -eq 0 ]]; then
        status="PASS"
    elif [[ $rc -eq 2 ]]; then
        status="NOT_RUN"
    else
        status="FAIL"
    fi

    echo "[${status}] ${check} (rc=${rc})" >> "${STATUS_FILE}"

    if [[ "${first}" == "true" ]]; then
        first=false
    else
        echo "," >> "${CHECKS_JSON}"
    fi
    cat >> "${CHECKS_JSON}" <<EOF
  {"check":"${check}","status":"${status}","rc":${rc},"started_at_utc":"${check_started}","finished_at_utc":"${check_finished}"}
EOF

    if [[ "${status}" != "PASS" ]]; then
        OVERALL_RC=1
    fi
done
echo "]" >> "${CHECKS_JSON}"

FINISHED_AT="$(date -u +"%Y-%m-%dT%H:%M:%SZ")"
if [[ "${OVERALL_RC}" -eq 0 ]]; then
    RUN_STATUS="PASS"
else
    RUN_STATUS="FAIL"
fi
write_manifest "${MANIFEST_FILE}" "${STARTED_AT}" "${FINISHED_AT}" "${RUN_STATUS}" "${CHECKS_JSON}"

{
    echo
    echo "Run status: ${RUN_STATUS}"
    echo "Reports directory: ${RUN_DIR}"
    echo "Checks file: ${CHECKS_JSON}"
    echo "Manifest file: ${MANIFEST_FILE}"
} >> "${STATUS_FILE}"

log_info "Compliance run complete. See:"
log_info "  ${STATUS_FILE}"
log_info "  ${RUN_DIR}"

exit "${OVERALL_RC}"

