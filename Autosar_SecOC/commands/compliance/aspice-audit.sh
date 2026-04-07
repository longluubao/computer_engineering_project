#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=common.sh
source "${SCRIPT_DIR}/common.sh"

ensure_report_dir

REPORT_FILE="${RUN_DIR}/aspice-audit.txt"
EXT_SCRIPT="$(resolve_extension_script_path "aspice-audit.sh")"

run_extension_script_or_stub "${EXT_SCRIPT}" "${REPORT_FILE}" "ASPICE"

