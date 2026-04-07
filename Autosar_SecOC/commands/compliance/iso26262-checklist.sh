#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=common.sh
source "${SCRIPT_DIR}/common.sh"

ensure_report_dir

REPORT_FILE="${RUN_DIR}/iso26262-checklist.txt"
EXT_SCRIPT="$(resolve_extension_script_path "iso26262-checklist.sh")"

run_extension_script_or_stub "${EXT_SCRIPT}" "${REPORT_FILE}" "ISO 26262"

