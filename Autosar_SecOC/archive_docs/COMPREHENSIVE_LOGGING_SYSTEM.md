# Comprehensive Logging System Documentation

**Date:** November 15, 2025
**System:** AUTOSAR SecOC with Post-Quantum Cryptography
**Purpose:** Detailed test execution logging for thesis validation and reporting

---

## Overview

The comprehensive logging system has been implemented in `build_and_run.sh` to capture **every detail** of the thesis validation process. Each test phase generates a timestamped log file with complete execution output, results, and analysis.

---

## Logging Architecture

### Directory Structure

```
Autosar_SecOC/
├── test_logs/                                    # Main logging directory
│   ├── phase1_pqc_fundamentals_YYYYMMDD_HHMMSS.txt
│   ├── phase2_google_test_suite_YYYYMMDD_HHMMSS.txt
│   ├── phase3_autosar_integration_YYYYMMDD_HHMMSS.txt
│   ├── phase4_system_validation_YYYYMMDD_HHMMSS.txt
│   └── MASTER_THESIS_VALIDATION_YYYYMMDD_HHMMSS.txt
├── build_and_run.sh                             # Enhanced with logging
└── test_summary.txt                              # Generated summary
```

### Automatic Directory Creation

The logging system automatically creates the `test_logs/` directory when the thesis validation sequence runs:

```bash
mkdir -p test_logs
```

---

## Phase-by-Phase Logging

### Phase 1: PQC Fundamentals

**Log File:** `test_logs/phase1_pqc_fundamentals_YYYYMMDD_HHMMSS.txt`

**Contents:**
```
================================================================
PHASE 1: PQC FUNDAMENTALS TEST LOG
Date: 2025-11-15 18:30:45
================================================================

[Complete test execution output from test_pqc_standalone.exe]
- ML-KEM-768 operations (KeyGen, Encapsulate, Decapsulate)
- ML-DSA-65 operations (KeyGen, Sign, Verify)
- Performance metrics for all operations
- Success rates (1000 iterations each)

RESULT: PASSED

Generated: pqc_standalone_results.csv

--- CSV Results ---
[Complete CSV data embedded in log]
```

**Capture Method:**
```bash
./test_pqc_standalone.exe 2>&1 | tee -a "$phase1_log"
```

**What's Logged:**
- All stdout and stderr output
- ML-KEM-768 test results (keygen, encapsulation, decapsulation)
- ML-DSA-65 test results (keygen, signing, verification)
- Performance metrics (timing, throughput)
- CSV results appended
- Final pass/fail status

---

### Phase 2: Google Test Suite

**Log File:** `test_logs/phase2_google_test_suite_YYYYMMDD_HHMMSS.txt`

**Contents:**
```
================================================================
PHASE 2: GOOGLE TEST SUITE LOG
Date: 2025-11-15 18:35:12
================================================================

TEST SUITES:
  1. AuthenticationTests (3 tests)
  2. VerificationTests (5 tests)
  3. FreshnessTests (10 tests)
  4. DirectTxTests (1 test)
  5. DirectRxTests (0 tests - platform specific)
  6. startOfReceptionTests (5 tests)
  7. SecOCTests (3 tests)
  8. PQC_ComparisonTests (13 tests - THESIS CONTRIBUTION)

================================================================

[Complete CTest output with verbose mode]
Test project C:/Users/.../build
    Start 1: AuthenticationTests
1/8 Test #1: AuthenticationTests ..............   Passed    0.71 sec
[... all test details ...]

--- TEST RESULTS SUMMARY ---
Total test suites: 8
Passed: 8
Failed: 0

RESULT: PASSED (8/8 tests)
```

**Capture Method:**
```bash
ctest --output-on-failure --verbose > ctest_output.txt 2>&1
cat ctest_output.txt | tee -a "../$phase2_log"
```

**What's Logged:**
- Complete CTest execution output (verbose mode)
- Each test suite's execution details
- Individual test case results
- Test summary with pass/fail counts
- Final phase result

---

### Phase 3: AUTOSAR Integration

**Log File:** `test_logs/phase3_autosar_integration_YYYYMMDD_HHMMSS.txt`

**Contents:**
```
================================================================
PHASE 3: AUTOSAR INTEGRATION TEST LOG
Date: 2025-11-15 18:40:25
================================================================

INTEGRATION TESTS:
  - Csm Layer PQC Integration
  - Classical MAC vs PQC Signature Comparison
  - Performance Benchmarking
  - Security Testing (Tampering Detection)

EXPECTED OUTPUTS:
  - pqc_advanced_results.csv (Performance metrics)
  - Security test results (2 tampering tests)

================================================================

[Complete test execution output from test_pqc_secoc_integration.exe]
- Csm_SignatureGenerate (PQC) vs Csm_MacGenerate (Classical)
- Csm_SignatureVerify (PQC) vs Csm_MacVerify (Classical)
- Performance comparison data
- Tampering detection test results

RESULT: PASSED

--- CSV Performance Results ---
[Complete CSV data embedded in log]
```

**Capture Method:**
```bash
./test_pqc_secoc_integration.exe 2>&1 | tee -a "$phase3_log"
```

**What's Logged:**
- Complete integration test output
- Csm layer function calls and results
- Performance comparison (Classical vs PQC)
- Security test results (tampering detection)
- CSV performance data appended
- Final pass/fail status

---

### Phase 4: System Validation

**Log File:** `test_logs/phase4_system_validation_YYYYMMDD_HHMMSS.txt`

**Contents:**
```
================================================================
PHASE 4: SYSTEM VALIDATION LOG
Date: 2025-11-15 18:45:30
================================================================

CONFIGURATION VALIDATION CHECKS:
  1. SECOC_ETHERNET_GATEWAY_MODE (Multi-transport gateway)
  2. SECOC_USE_PQC_MODE (Quantum-resistant security)
  3. SECOC_PQC_MAX_PDU_SIZE (Large PDU support)
  4. ML-KEM-768 configuration
  5. ML-DSA-65 configuration

ARCHITECTURE VALIDATION:
  - Multi-transport support (CAN + Ethernet)
  - Gateway bridge function (CAN <-> Ethernet)
  - Dual-mode authentication (Classical MAC + PQC)

================================================================

[VALIDATE] Checking Ethernet Gateway configuration...

[OK] Ethernet Gateway mode: ENABLED
  - Value: TRUE
  - Location: include/SecOC/SecOC_PQC_Cfg.h

[OK] PQC mode: ENABLED
  - Value: TRUE
  - Location: include/SecOC/SecOC_PQC_Cfg.h

[OK] Max PDU size: 8192 bytes (sufficient for PQC signatures)
  - Value: 8192U
  - ML-DSA-65 signature: 3309 bytes
  - Sufficient headroom: YES

[VALIDATE] Checking transport layer configuration...

[OK] Socket Adapter TP (Ethernet) configured
  - Transport: SECOC_SECURED_PDU_SOADTP
  - Location: source/SecOC/SecOC_Lcfg.c

[OK] CAN Interface configured (Classical mode support)
  - Transport: SECOC_SECURED_PDU_CANIF
  - Purpose: Backward compatibility

[SUMMARY] Phase 4 Validation Results:
  - All Google Tests passed (including PQC comparison)
  - PQC standalone tests passed (ML-KEM + ML-DSA)
  - Integration tests passed (Csm layer)
  - Configuration validated for Ethernet Gateway

RESULT: PASSED
```

**Capture Method:**
```bash
echo -e "..." | tee -a "$phase4_log"  # All validation checks
```

**What's Logged:**
- Configuration file validation results
- Each configuration parameter check (with file location and value)
- Transport layer configuration analysis
- Multi-transport architecture verification
- System readiness assessment
- Final pass/fail status

---

### Master Consolidated Log

**Log File:** `test_logs/MASTER_THESIS_VALIDATION_YYYYMMDD_HHMMSS.txt`

**Contents:**
```
================================================================
           MASTER THESIS VALIDATION LOG
           AUTOSAR SecOC with Post-Quantum Cryptography
================================================================

Generated: 2025-11-15 18:50:00

RESEARCH QUESTION:
  "Can Post-Quantum Cryptography be successfully integrated into
   AUTOSAR SecOC module for quantum-resistant automotive security?"

================================================================

VALIDATION PHASES:
  Phase 1: PQC Fundamentals (ML-KEM-768 & ML-DSA-65)
  Phase 2: Classical vs PQC Comparison (Google Test Suite)
  Phase 3: AUTOSAR SecOC Integration (Csm Layer)
  Phase 4: Complete System Validation (Ethernet Gateway)

================================================================

PHASE 1 RESULTS: PQC FUNDAMENTALS
Status: PASSED
Log: test_logs/phase1_pqc_fundamentals_20251115_183045.txt

[Complete contents of Phase 1 log]

================================================================

PHASE 2 RESULTS: GOOGLE TEST SUITE
Status: PASSED (8/8 tests)
Log: test_logs/phase2_google_test_suite_20251115_183512.txt

[Complete contents of Phase 2 log]

================================================================

PHASE 3 RESULTS: AUTOSAR INTEGRATION
Status: PASSED
Log: test_logs/phase3_autosar_integration_20251115_184025.txt

[Complete contents of Phase 3 log]

================================================================

PHASE 4 RESULTS: SYSTEM VALIDATION
Status: PASSED
Log: test_logs/phase4_system_validation_20251115_184530.txt

[Complete contents of Phase 4 log]

================================================================

OVERALL VALIDATION RESULT:

  Phase 1: [PASS]
  Phase 2: [PASS] 8/8 tests
  Phase 3: [PASS]
  Phase 4: [PASS]

================================================================
           THESIS CONTRIBUTION SUCCESSFULLY VALIDATED!
================================================================

CONCLUSION:
  Post-Quantum Cryptography (ML-KEM-768 & ML-DSA-65) has been
  successfully integrated into AUTOSAR SecOC module.

KEY ACHIEVEMENTS:
  - ML-KEM-768: Quantum-resistant key exchange validated
  - ML-DSA-65: Post-quantum digital signatures validated
  - AUTOSAR Integration: Csm layer successfully adapted
  - Ethernet Gateway: Multi-transport architecture validated
  - Backward Compatibility: Classical MAC support maintained
  - Security: Tampering detection functional in both modes

================================================================
                    END OF MASTER LOG
================================================================
```

**Generation Method:**
```bash
{
    echo "Master header"
    cat "$phase1_log"
    cat "$phase2_log"
    cat "$phase3_log"
    cat "$phase4_log"
    echo "Overall summary"
} > "$master_log"
```

**What's Included:**
- Complete research question and thesis objective
- All four phase logs (full contents)
- Individual phase pass/fail status
- Overall validation result
- Complete conclusion with key achievements
- All CSV data from each phase

---

## How to Run and Access Logs

### Running the Thesis Validation Sequence

```bash
cd Autosar_SecOC
bash build_and_run.sh
# Select option: "Thesis Storytelling Sequence (RECOMMENDED)"
```

### Log File Locations Displayed

After each phase completes, the system displays the log file path:

```
[LOG] Phase 1 detailed results saved to: test_logs/phase1_pqc_fundamentals_20251115_183045.txt
[LOG] Phase 2 detailed results saved to: test_logs/phase2_google_test_suite_20251115_183512.txt
[LOG] Phase 3 detailed results saved to: test_logs/phase3_autosar_integration_20251115_184025.txt
[LOG] Phase 4 detailed results saved to: test_logs/phase4_system_validation_20251115_184530.txt

[OK] Master log created: test_logs/MASTER_THESIS_VALIDATION_20251115_185000.txt
```

At the end, a summary displays all log file paths:

```
+================================================================+
|                    DETAILED LOGS AVAILABLE                     |
+================================================================+

  Master Log: test_logs/MASTER_THESIS_VALIDATION_20251115_185000.txt
  Phase 1:    test_logs/phase1_pqc_fundamentals_20251115_183045.txt
  Phase 2:    test_logs/phase2_google_test_suite_20251115_183512.txt
  Phase 3:    test_logs/phase3_autosar_integration_20251115_184025.txt
  Phase 4:    test_logs/phase4_system_validation_20251115_184530.txt
```

---

## Log File Naming Convention

**Format:** `phaseN_description_YYYYMMDD_HHMMSS.txt`

**Examples:**
- `phase1_pqc_fundamentals_20251115_183045.txt`
- `phase2_google_test_suite_20251115_183512.txt`
- `MASTER_THESIS_VALIDATION_20251115_185000.txt`

**Timestamp Format:**
- `YYYYMMDD`: Year, Month, Day (e.g., 20251115 = November 15, 2025)
- `HHMMSS`: Hour, Minute, Second (24-hour format)

**Benefits:**
- Chronological sorting (oldest to newest)
- No filename conflicts (unique timestamps)
- Easy identification of test run date/time

---

## What Gets Logged in Each Phase

### Phase 1: PQC Fundamentals
- [x] Test execution banner with timestamp
- [x] ML-KEM-768 keygen, encapsulation, decapsulation results
- [x] ML-DSA-65 keygen, sign, verify results
- [x] Performance metrics (microseconds, throughput)
- [x] Success rates (100% expected)
- [x] CSV results (`pqc_standalone_results.csv`) embedded
- [x] Final PASS/FAIL result

### Phase 2: Google Test Suite
- [x] Test suite header with timestamp
- [x] List of all 8 test suites
- [x] Complete CTest verbose output
- [x] Each test suite execution (1/8, 2/8, ...)
- [x] Individual test case results (PASSED/FAILED)
- [x] Test summary (total, passed, failed)
- [x] Final PASS/FAIL result with test count

### Phase 3: AUTOSAR Integration
- [x] Integration test header with timestamp
- [x] Csm layer test descriptions
- [x] Complete test execution output
- [x] Classical MAC vs PQC comparison
- [x] Performance benchmark results
- [x] Security test results (tampering detection)
- [x] CSV results (`pqc_advanced_results.csv`) embedded
- [x] Final PASS/FAIL result

### Phase 4: System Validation
- [x] Validation header with timestamp
- [x] Configuration checks with file paths and values
- [x] ETHERNET_GATEWAY_MODE validation
- [x] PQC_MODE validation
- [x] PDU_SIZE validation (8192 bytes)
- [x] Transport layer configuration check
- [x] Multi-transport architecture verification
- [x] System readiness summary
- [x] Final PASS/FAIL result

### Master Log
- [x] Research question
- [x] Complete contents of all 4 phase logs
- [x] Individual phase status summary
- [x] Overall validation result (all phases)
- [x] Complete conclusion with achievements
- [x] All embedded CSV data from all phases

---

## Key Features

### 1. **Timestamped Logs**
Every log file has a unique timestamp preventing overwriting of previous test runs.

### 2. **Complete Output Capture**
Uses `tee -a` to capture both stdout and stderr while displaying to terminal:
```bash
./test.exe 2>&1 | tee -a "$logfile"
```

### 3. **Embedded CSV Data**
CSV results are automatically appended to logs for complete data traceability.

### 4. **Structured Headers**
Each log starts with:
- Separator line
- Phase name
- Timestamp
- Test objectives
- Expected outputs

### 5. **Pass/Fail Tracking**
Each log ends with clear RESULT status:
```
RESULT: PASSED
RESULT: FAILED
RESULT: PASSED (8/8 tests)
```

### 6. **Master Consolidation**
The master log combines all phase logs into a single comprehensive document with overall conclusion.

### 7. **Real-Time Feedback**
Log file paths displayed immediately after each phase so you know where to find detailed results.

---

## Usage for Thesis Reporting

### For Writing Thesis

1. **Run the validation sequence once**
2. **Access the master log** (`MASTER_THESIS_VALIDATION_*.txt`)
3. **Extract data for thesis:**
   - Research question validation
   - Phase-by-phase results
   - Performance metrics from embedded CSVs
   - Overall conclusion

### For Detailed Analysis

Access individual phase logs for deep-dive analysis:
- Phase 1: Raw PQC algorithm performance
- Phase 2: Unit test coverage details
- Phase 3: Integration-level benchmarks
- Phase 4: Configuration validation evidence

### For Evidence/Proof

All logs contain:
- Exact timestamps
- Complete command outputs
- All performance data
- Pass/fail status
- Configuration values with file locations

---

## Example Usage Scenario

```bash
# Step 1: Run validation
cd Autosar_SecOC
bash build_and_run.sh
# Select: "Thesis Storytelling Sequence"

# Step 2: Wait for completion (with Enter prompts)
# Each phase will display log file path

# Step 3: Access logs
cd test_logs
ls -lht  # List logs sorted by time (newest first)

# Step 4: View master log
cat MASTER_THESIS_VALIDATION_20251115_185000.txt

# Step 5: Extract specific phase data
cat phase2_google_test_suite_20251115_183512.txt | grep "PASSED"

# Step 6: Copy to thesis directory
cp MASTER_THESIS_VALIDATION_*.txt ~/thesis/evidence/
```

---

## Log File Sizes (Approximate)

| Log File | Expected Size | Content |
|----------|---------------|---------|
| Phase 1  | ~50 KB        | PQC standalone test output + CSV |
| Phase 2  | ~200 KB       | Complete CTest verbose output (8 suites) |
| Phase 3  | ~30 KB        | Integration test output + CSV |
| Phase 4  | ~10 KB        | Configuration validation results |
| Master   | ~300 KB       | All phases combined + summary |

**Total per test run:** ~600 KB (very manageable)

---

## Retention Policy

**Recommendation:** Keep all logs indefinitely (they're small and invaluable for thesis defense).

**Organization:**
```
test_logs/
├── 2025-11-15/  # Today's logs
│   ├── MASTER_THESIS_VALIDATION_20251115_185000.txt
│   └── phase*_*.txt
├── 2025-11-14/  # Yesterday's logs
└── archive/     # Older logs (if needed)
```

---

## Troubleshooting

### Log file not created

**Cause:** `test_logs/` directory doesn't exist
**Solution:** Automatically created by script (`mkdir -p test_logs`)

### Empty log file

**Cause:** Test executable failed before output
**Solution:** Check build logs, ensure test binary exists

### Incomplete log (truncated)

**Cause:** Test crashed mid-execution
**Solution:** Log will still contain output up to crash point; check for core dumps

### Missing CSV data in log

**Cause:** CSV file not generated by test
**Solution:** Check test execution; CSV appending is optional if file doesn't exist

---

## Conclusion

The comprehensive logging system provides **complete traceability** for the entire thesis validation process:

✅ **Every test execution logged**
✅ **All outputs captured (stdout + stderr)**
✅ **Performance data embedded (CSV)**
✅ **Configuration validation documented**
✅ **Master log consolidates everything**
✅ **Timestamped for non-destructive retention**
✅ **Real-time feedback (log paths displayed)**

**Result:** You have complete, detailed, timestamped evidence of all thesis validation work for reporting, defense, and future reference.

---

**Generated:** November 15, 2025
**System:** AUTOSAR SecOC with PQC
**Logging Version:** 2.0 (Comprehensive Deep Research Mode)
