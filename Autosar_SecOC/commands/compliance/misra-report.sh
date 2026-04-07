#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=common.sh
source "${SCRIPT_DIR}/common.sh"

ensure_report_dir

OUTPUT_TXT="${RUN_DIR}/misra-cppcheck.txt"
OUTPUT_CLEAN_TXT="${RUN_DIR}/misra-cppcheck-clean.txt"
MAIN_VIOLATIONS_TXT="${RUN_DIR}/misra-main-violations.txt"
SUMMARY_JSON="${RUN_DIR}/misra-summary.json"
INTEGRITY_TXT="${RUN_DIR}/misra-metrics-integrity.txt"
CPP_INCLUDES_FILE="${PROJECT_ROOT}/build/cppcheck_includes.txt"
MISRA_SUPPRESS_CONFIG_NOISE="${MISRA_SUPPRESS_CONFIG_NOISE:-true}"
MISRA_TOP_N="${MISRA_TOP_N:-20}"
MISRA_STRICT_METRICS="${MISRA_STRICT_METRICS:-true}"

log_info "Starting MISRA check (cppcheck + misra addon)."

CPPCHECK_BIN=""
if command -v cppcheck >/dev/null 2>&1; then
    CPPCHECK_BIN="cppcheck"
elif command -v cppcheck.exe >/dev/null 2>&1; then
    CPPCHECK_BIN="cppcheck.exe"
fi

if [[ -z "${CPPCHECK_BIN}" ]]; then
    cat > "${OUTPUT_TXT}" <<EOF
cppcheck not found in PATH.
Install cppcheck, then rerun:
bash commands/compliance/misra-report.sh
EOF
    log_warn "cppcheck not found; wrote stub report."
    exit 1
fi

MISRA_ADDON=""
CPPCHECK_PATH="$(command -v "${CPPCHECK_BIN}" 2>/dev/null || true)"

for candidate in \
    "${CPPCHECK_PATH%/bin/*}/share/cppcheck/addons/misra.py" \
    "C:/MinGW/mingw64/share/cppcheck/addons/misra.py" \
    "/c/MinGW/mingw64/share/cppcheck/addons/misra.py" \
    "/mnt/c/MinGW/mingw64/share/cppcheck/addons/misra.py" \
    "/usr/share/cppcheck/addons/misra.py" \
    "/mingw64/share/cppcheck/addons/misra.py" \
    "/usr/lib/x86_64-linux-gnu/cppcheck/addons/misra.py" \
    "/usr/lib/cppcheck/addons/misra.py"; do
    [[ -z "${candidate}" ]] && continue
    if [[ -f "${candidate}" ]]; then
        MISRA_ADDON="${candidate}"
        break
    fi
done

if [[ -z "${MISRA_ADDON}" ]]; then
    cat > "${OUTPUT_TXT}" <<EOF
MISRA addon (misra.py) not found for cppcheck.
Checked common locations:
- ${CPPCHECK_PATH%/bin/*}/share/cppcheck/addons/misra.py
- C:/MinGW/mingw64/share/cppcheck/addons/misra.py
- /c/MinGW/mingw64/share/cppcheck/addons/misra.py
- /mnt/c/MinGW/mingw64/share/cppcheck/addons/misra.py
- /usr/share/cppcheck/addons/misra.py
- /mingw64/share/cppcheck/addons/misra.py
EOF
    log_warn "MISRA addon not found; wrote stub report."
    exit 1
fi

if [[ "${CPPCHECK_BIN}" == "cppcheck.exe" ]]; then
    case "${MISRA_ADDON}" in
        /mnt/c/*)
            MISRA_ADDON="C:/${MISRA_ADDON#/mnt/c/}"
            ;;
        /c/*)
            MISRA_ADDON="C:/${MISRA_ADDON#/c/}"
            ;;
        /mnt/*)
            drive_letter="$(printf "%s" "${MISRA_ADDON}" | cut -d'/' -f3 | tr '[:lower:]' '[:upper:]')"
            remainder="$(printf "%s" "${MISRA_ADDON}" | cut -d'/' -f4-)"
            MISRA_ADDON="${drive_letter}:/${remainder}"
            ;;
    esac
fi

(
    cd "${PROJECT_ROOT}"
    CPPCHECK_INCLUDE_ARGS=()
    CPPCHECK_SUPPRESS_ARGS=("--suppress=missingIncludeSystem")
    CPPCHECK_DEFINE_ARGS=(
        "-DSTD_ON=0x01u"
        "-DSTD_OFF=0x00u"
        "-DTRUE=1u"
        "-DFALSE=0u"
        "-DOS_DEV_ERROR_DETECT=STD_ON"
        "-DSOAD_DEV_ERROR_DETECT=STD_ON"
        "-DETHIF_DEV_ERROR_DETECT=STD_ON"
        "-DETHSM_DEV_ERROR_DETECT=STD_ON"
        "-DTCPIP_DEV_ERROR_DETECT=STD_ON"
        "-DSECOC_DEV_ERROR_DETECT=FALSE"
        "-DSOAD_TCPIP_PAYLOAD_BACKEND_SOCKETS=0U"
        "-DSOAD_TCPIP_PAYLOAD_BACKEND_ETHIF=1U"
        "-DSOAD_TCPIP_PAYLOAD_BACKEND=0U"
        "-DTCPIP_PAYLOAD_BACKEND_SOCKETS=0U"
        "-DTCPIP_PAYLOAD_BACKEND_ETHIF=1U"
        "-DTCPIP_PAYLOAD_BACKEND=0U"
        "-DSECOC_USE_PQC_MODE=TRUE"
        "-DSECOC_VERSION_INFO_API=STD_ON"
        "-DSCHEDULER_SECOC_HANDLED_BY_ECUM=1"
    )

    # AUTOSAR-standard deviations (Advisory rules):
    # Rule 15.5 (Advisory) - AUTOSAR DET error-handling pattern uses early returns
    CPPCHECK_SUPPRESS_ARGS+=("--suppress=misra-c2012-15.5")
    # Rule 8.7 (Advisory) - AUTOSAR modules expose APIs with external linkage even
    #   when called from a single TU in this build; usage is configuration-dependent
    CPPCHECK_SUPPRESS_ARGS+=("--suppress=misra-c2012-8.7")
    # Rule 8.9 (Advisory) - File-scope objects used by one function are acceptable
    #   for AUTOSAR configuration data patterns
    CPPCHECK_SUPPRESS_ARGS+=("--suppress=misra-c2012-8.9")
    # Rule 2.5 (Advisory) - Unused macros kept for AUTOSAR configuration completeness
    CPPCHECK_SUPPRESS_ARGS+=("--suppress=misra-c2012-2.5")
    # Rule 21.6 (Required) - stdio.h used only in host-side debug/test code
    CPPCHECK_SUPPRESS_ARGS+=("--suppress=misra-c2012-21.6")
    # Rule 21.10 (Required) - time.h used only in host scheduler/Os simulation
    CPPCHECK_SUPPRESS_ARGS+=("--suppress=misra-c2012-21.10")

    if [[ "${MISRA_SUPPRESS_CONFIG_NOISE}" == "true" ]]; then
        CPPCHECK_SUPPRESS_ARGS+=("--suppress=misra-config")
    fi
    # Build include paths from the include/ tree (relative paths avoid
    # Windows/MSYS path mismatches that cause cppcheck 8.4 false positives).
    CPPCHECK_INCLUDE_ARGS+=("-I" "include")
    for _d in include/*/; do
        [[ -d "${_d}" ]] && CPPCHECK_INCLUDE_ARGS+=("-I" "${_d}")
    done
    # Also pick up external liboqs headers if present
    if [[ -d "external/liboqs/build/include" ]]; then
        CPPCHECK_INCLUDE_ARGS+=("-I" "external/liboqs/build/include")
    fi

    "${CPPCHECK_BIN}" \
        --addon="${MISRA_ADDON}" \
        --std=c99 \
        --language=c \
        --enable=warning,style,performance,portability \
        "${CPPCHECK_SUPPRESS_ARGS[@]}" \
        --inline-suppr \
        "${CPPCHECK_DEFINE_ARGS[@]}" \
        "${CPPCHECK_INCLUDE_ARGS[@]}" \
        -i source/GUIInterface \
        source \
        2> "${OUTPUT_TXT}"
)

TMP_DIR="$(mktemp -d)"
RULE_COUNTS_TSV="${TMP_DIR}/rule_counts.tsv"
FILE_COUNTS_TSV="${TMP_DIR}/file_counts.tsv"
RULE_FILE_COUNTS_TSV="${TMP_DIR}/rule_file_counts.tsv"
DIAG_COUNTS_TSV="${TMP_DIR}/diag_counts.tsv"
MISRA_RULE_COUNTS_TSV="${TMP_DIR}/misra_rule_counts.tsv"
MISRA_FILE_COUNTS_TSV="${TMP_DIR}/misra_file_counts.tsv"
: > "${RULE_COUNTS_TSV}"
: > "${FILE_COUNTS_TSV}"
: > "${RULE_FILE_COUNTS_TSV}"
: > "${DIAG_COUNTS_TSV}"
: > "${MISRA_RULE_COUNTS_TSV}"
: > "${MISRA_FILE_COUNTS_TSV}"

awk -v clean_file="${OUTPUT_CLEAN_TXT}" \
    -v rule_counts_file="${RULE_COUNTS_TSV}" \
    -v file_counts_file="${FILE_COUNTS_TSV}" \
    -v rule_file_counts_file="${RULE_FILE_COUNTS_TSV}" \
    -v diag_counts_file="${DIAG_COUNTS_TSV}" \
    -v misra_rule_counts_file="${MISRA_RULE_COUNTS_TSV}" \
    -v misra_file_counts_file="${MISRA_FILE_COUNTS_TSV}" '
function is_diag_line(line) {
    return (line ~ /^[^:]+:[0-9]+:[0-9]+: .* \[[^]]+\]$/);
}
{
    line = $0;
    sub(/\r$/, "", line);
    if (is_diag_line(line)) {
        diag = line;
        code = "";
        caret = "";
        if (getline code_line > 0) {
            sub(/\r$/, "", code_line);
            code = code_line;
        }
        if (getline caret_line > 0) {
            sub(/\r$/, "", caret_line);
            caret = caret_line;
        }

        if (!(diag in diag_seen)) {
            diag_order[++diag_count_unique] = diag;
            diag_code[diag] = code;
            diag_caret[diag] = caret;
            diag_seen[diag] = 0;
        }
        diag_seen[diag]++;

        split(diag, fields, ":");
        file_key = fields[1];
        file_counts[file_key]++;

        rule = "unknown";
        if (match(diag, /\[[^]]+\]$/)) {
            rule = substr(diag, RSTART + 1, RLENGTH - 2);
        }
        rule_counts[rule]++;
        rule_file_counts[rule " | " file_key]++;
        if (rule ~ /^misra-c2012-/) {
            misra_rule_counts[rule]++;
            misra_file_counts[file_key]++;
        }
    }
}
END {
    for (i = 1; i <= diag_count_unique; i++) {
        d = diag_order[i];
        c = diag_seen[d];
        printf("[x%d] %s\n", c, d) >> clean_file;
        if (diag_code[d] != "") {
            printf("%s\n", diag_code[d]) >> clean_file;
        }
        if (diag_caret[d] != "") {
            printf("%s\n", diag_caret[d]) >> clean_file;
        }
        printf("\n") >> clean_file;
    }

    for (r in rule_counts) {
        printf("%d\t%s\n", rule_counts[r], r) >> rule_counts_file;
    }
    for (f in file_counts) {
        printf("%d\t%s\n", file_counts[f], f) >> file_counts_file;
    }
    for (rf in rule_file_counts) {
        printf("%d\t%s\n", rule_file_counts[rf], rf) >> rule_file_counts_file;
    }
    for (k in diag_seen) {
        printf("%d\t%s\n", diag_seen[k], k) >> diag_counts_file;
    }
    for (mr in misra_rule_counts) {
        printf("%d\t%s\n", misra_rule_counts[mr], mr) >> misra_rule_counts_file;
    }
    for (mf in misra_file_counts) {
        printf("%d\t%s\n", misra_file_counts[mf], mf) >> misra_file_counts_file;
    }
}
' "${OUTPUT_TXT}"

UNIQUE_TOTAL="$(awk 'END {print NR+0}' "${DIAG_COUNTS_TSV}")"
TOTAL_DIAGNOSTICS="$(awk -F '\t' '{s+=$1} END {print s+0}' "${DIAG_COUNTS_TSV}")"
TOTAL="$(awk -F '\t' '$2 ~ /\[misra-c2012-/{s+=$1} END {print s+0}' "${DIAG_COUNTS_TSV}")"
UNIQUE_MISRA="$(awk -F '\t' '$2 ~ /\[misra-c2012-/{count++} END {print count+0}' "${DIAG_COUNTS_TSV}")"
REQ="$(awk -F '\t' '$2 ~ /\[misra-c2012-(10|11|14)\./{s+=$1} END {print s+0}' "${DIAG_COUNTS_TSV}")"
DUPLICATE_TOTAL=$((TOTAL - UNIQUE_MISRA))
if [[ "${DUPLICATE_TOTAL}" -lt 0 ]]; then
    DUPLICATE_TOTAL=0
fi

TOP_ANY_RULE="$(sort -nr "${RULE_COUNTS_TSV}" | awk 'NR==1{print $2}')"
if [[ -z "${TOP_ANY_RULE}" ]]; then
    TOP_ANY_RULE="none"
fi
TOP_ANY_FILE="$(sort -nr "${FILE_COUNTS_TSV}" | awk 'NR==1{print $2}')"
if [[ -z "${TOP_ANY_FILE}" ]]; then
    TOP_ANY_FILE="none"
fi
TOP_MISRA_RULE="$(sort -nr "${MISRA_RULE_COUNTS_TSV}" | awk 'NR==1{print $2}')"
if [[ -z "${TOP_MISRA_RULE}" ]]; then
    TOP_MISRA_RULE="none"
fi
TOP_MISRA_FILE="$(sort -nr "${MISRA_FILE_COUNTS_TSV}" | awk 'NR==1{print $2}')"
if [[ -z "${TOP_MISRA_FILE}" ]]; then
    TOP_MISRA_FILE="none"
fi

# JSON/path normalization for audit-safe outputs.
TOP_ANY_FILE="${TOP_ANY_FILE//\\//}"
TOP_MISRA_FILE="${TOP_MISRA_FILE//\\//}"

TOTAL_FROM_RULES="$(awk '{s+=$1} END {print s+0}' "${RULE_COUNTS_TSV}")"
TOTAL_FROM_MISRA_RULES="$(awk '{s+=$1} END {print s+0}' "${MISRA_RULE_COUNTS_TSV}")"
INTEGRITY_OK="true"
if [[ "${TOTAL_DIAGNOSTICS}" -ne "${TOTAL_FROM_RULES}" ]] || [[ "${TOTAL}" -ne "${TOTAL_FROM_MISRA_RULES}" ]] || [[ "${TOTAL}" -lt "${UNIQUE_MISRA}" ]]; then
    INTEGRITY_OK="false"
fi

{
    echo "MISRA metrics integrity"
    echo "======================="
    echo "- integrity_ok: ${INTEGRITY_OK}"
    echo "- total_diagnostics_all: ${TOTAL_DIAGNOSTICS}"
    echo "- total_from_rule_counts: ${TOTAL_FROM_RULES}"
    echo "- total_misra_findings: ${TOTAL}"
    echo "- total_from_misra_rule_counts: ${TOTAL_FROM_MISRA_RULES}"
    echo "- unique_misra_findings: ${UNIQUE_MISRA}"
} > "${INTEGRITY_TXT}"

if [[ "${INTEGRITY_OK}" != "true" ]]; then
    log_warn "MISRA metrics integrity failed; see ${INTEGRITY_TXT}"
    if [[ "${MISRA_STRICT_METRICS}" == "true" ]]; then
        rm -rf "${TMP_DIR}"
        exit 2
    fi
fi

{
    echo "MISRA Main Violations (Top ${MISRA_TOP_N})"
    echo "==========================================="
    echo ""
    echo "Totals"
    echo "- Raw MISRA findings: ${TOTAL}"
    echo "- Unique MISRA diagnostics: ${UNIQUE_MISRA}"
    echo "- Duplicate MISRA diagnostics removed: ${DUPLICATE_TOTAL}"
    echo "- Raw all diagnostics (MISRA + cppcheck): ${TOTAL_DIAGNOSTICS}"
    echo "- Unique all diagnostics (MISRA + cppcheck): ${UNIQUE_TOTAL}"
    echo ""
    echo "Top MISRA rules"
    sort -nr "${MISRA_RULE_COUNTS_TSV}" | head -n "${MISRA_TOP_N}" | awk -F '\t' '{printf("- %s: %s\n", $2, $1)}'
    echo ""
    echo "Top rules (all diagnostics)"
    sort -nr "${RULE_COUNTS_TSV}" | head -n "${MISRA_TOP_N}" | awk -F '\t' '{printf("- %s: %s\n", $2, $1)}'
    echo ""
    echo "Top MISRA files"
    sort -nr "${MISRA_FILE_COUNTS_TSV}" | head -n "${MISRA_TOP_N}" | awk -F '\t' '{printf("- %s: %s\n", $2, $1)}'
    echo ""
    echo "Top files (all diagnostics)"
    sort -nr "${FILE_COUNTS_TSV}" | head -n "${MISRA_TOP_N}" | awk -F '\t' '{printf("- %s: %s\n", $2, $1)}'
    echo ""
    echo "Top rule+file pairs"
    sort -nr "${RULE_FILE_COUNTS_TSV}" | head -n "${MISRA_TOP_N}" | awk -F '\t' '{printf("- %s: %s\n", $2, $1)}'
    echo ""
    echo "Most duplicated exact diagnostics"
    sort -nr "${DIAG_COUNTS_TSV}" | awk '$1 > 1' | head -n "${MISRA_TOP_N}" | awk -F '\t' '{printf("- x%s %s\n", $1, $2)}'
} > "${MAIN_VIOLATIONS_TXT}"

cat > "${SUMMARY_JSON}" <<EOF
{
  "tool": "cppcheck-misra-addon",
  "output": "$(basename "${OUTPUT_TXT}")",
  "deduplicated_output": "$(basename "${OUTPUT_CLEAN_TXT}")",
  "main_violations_output": "$(basename "${MAIN_VIOLATIONS_TXT}")",
  "total_misra_findings": ${TOTAL},
  "unique_misra_findings": ${UNIQUE_MISRA},
  "duplicate_misra_findings": ${DUPLICATE_TOTAL},
  "total_diagnostics_all": ${TOTAL_DIAGNOSTICS},
  "unique_diagnostics_all": ${UNIQUE_TOTAL},
  "top_rule": "${TOP_MISRA_RULE}",
  "top_file": "${TOP_MISRA_FILE}",
  "top_any_rule": "${TOP_ANY_RULE}",
  "top_any_file": "${TOP_ANY_FILE}",
  "integrity_file": "$(basename "${INTEGRITY_TXT}")",
  "key_rule_family_hits_10_11_14": ${REQ}
}
EOF

log_info "MISRA report written:"
log_info "  ${OUTPUT_TXT}"
log_info "  ${OUTPUT_CLEAN_TXT}"
log_info "  ${MAIN_VIOLATIONS_TXT}"
log_info "  ${INTEGRITY_TXT}"
log_info "  ${SUMMARY_JSON}"

rm -rf "${TMP_DIR}"

