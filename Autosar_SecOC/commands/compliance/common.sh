#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
REPORT_ROOT="${PROJECT_ROOT}/compliance_reports"
RUN_ID="${RUN_ID:-$(date +%Y%m%d_%H%M%S)}"
RUN_DIR="${RUN_DIR:-${REPORT_ROOT}/${RUN_ID}}"
LATEST_LINK="${REPORT_ROOT}/latest"
LATEST_FILE="${REPORT_ROOT}/LATEST.txt"
ALLOW_STUB="${ALLOW_STUB:-false}"
COMPLIANCE_EXT_ROOT="${COMPLIANCE_EXT_ROOT:-/mnt/d/Long-Dev/automotive-claude-code-agents/commands/compliance}"

log_info() {
    echo "[INFO] "
}

log_warn() {
    echo "[WARN] "
}

ensure_report_dir() {
    mkdir -p "${RUN_DIR}"
    rm -f "${LATEST_LINK}" 2>/dev/null || true
    if ln -s "${RUN_DIR}" "${LATEST_LINK}" 2>/dev/null; then
        :
    else
        log_warn "Unable to create ${LATEST_LINK}; writing ${LATEST_FILE} fallback."
    fi
    printf "%s\n" "${RUN_DIR}" > "${LATEST_FILE}"
}

resolve_extension_script_path() {
    local script_name="$1"
    printf "%s/%s\n" "${COMPLIANCE_EXT_ROOT}" "${script_name}"
}

get_git_commit() {
    git -C "${PROJECT_ROOT}" rev-parse HEAD 2>/dev/null || echo "unknown"
}

get_git_dirty_flag() {
    if [[ -n "$(git -C "${PROJECT_ROOT}" status --porcelain 2>/dev/null)" ]]; then
        echo "true"
    else
        echo "false"
    fi
}

write_manifest() {
    local manifest_file="$1"
    local started_at="$2"
    local finished_at="$3"
    local run_status="$4"
    local checks_file="$5"

    if command -v cppcheck >/dev/null 2>&1; then
        cppcheck_version="$(cppcheck --version 2>/dev/null | awk '{print $2}' | tr -d '\r')"
    elif command -v cppcheck.exe >/dev/null 2>&1; then
        cppcheck_version="$(cppcheck.exe --version 2>/dev/null | awk '{print $2}' | tr -d '\r')"
    else
        cppcheck_version="not-found"
    fi

    cat > "${manifest_file}" <<EOF
{
  "run_id": "${RUN_ID}",
  "project_root": "${PROJECT_ROOT}",
  "report_root": "${REPORT_ROOT}",
  "run_dir": "${RUN_DIR}",
  "started_at_utc": "${started_at}",
  "finished_at_utc": "${finished_at}",
  "status": "${run_status}",
  "git_commit": "$(get_git_commit)",
  "git_dirty": $(get_git_dirty_flag),
  "tool_versions": {
    "bash": "$(bash --version | awk 'NR==1 {print $4}')",
    "cppcheck": "${cppcheck_version}"
  },
  "checks_file": "$(basename "${checks_file}")",
  "allow_stub": ${ALLOW_STUB}
}
EOF
}

run_extension_script_or_stub() {
    local extension_script="$1"
    local report_file="$2"
    local title="$3"
    local normalized_script=""

    if [[ -f "${extension_script}" ]]; then
        log_info "Running: ${extension_script}"
        normalized_script="$(mktemp "/tmp/compliance-${title// /_}-XXXXXX.sh")"
        tr -d '\r' < "${extension_script}" > "${normalized_script}"
        if bash "${normalized_script}" 2>&1 | sed -E 's/\x1B\[[0-9;]*[[:alpha:]]//g' > "${report_file}"; then
            rm -f "${normalized_script}" 2>/dev/null || true
            log_info "${title} check completed."
            return 0
        fi
        rm -f "${normalized_script}" 2>/dev/null || true
        log_warn "${title} script failed; see ${report_file}"
        return 1
    fi

    cat > "${report_file}" <<EOF
${title} check script not found:
${extension_script}

Action:
- Install/clone automotive-claude-code-agents, or
- Replace this stub with your internal checker implementation.
EOF
    log_warn "${title} script not found; stub report generated."
    if [[ "${ALLOW_STUB}" == "true" ]]; then
        return 0
    fi
    return 2
}
