# Testing Quick Reference Guide

## Quick Commands

### 🚀 RECOMMENDED: Run All Tests with Visualization
```bash
cd Autosar_SecOC
bash build_and_run.sh report
```
**This will:**
- Build and run PQC standalone tests
- Build and run PQC integration tests
- Generate interactive HTML report with charts
- Export results to CSV
- Show test summary

**Output:**
- `test_results.html` - Interactive dashboard (open in browser)
- `pqc_standalone_results.csv` - Standalone test data
- `pqc_secoc_integration_results.csv` - Integration test data

---

### Individual Test Commands

#### 1. PQC Standalone Tests (ML-KEM-768 & ML-DSA-65)
```bash
bash build_and_run.sh standalone
./test_pqc_standalone.exe
```

#### 2. PQC Integration Tests (Csm Layer)
```bash
bash build_and_run.sh integration
./test_pqc_secoc_integration.exe
```

#### 3. Run Both Tests (No Report)
```bash
bash build_and_run.sh all
```

#### 4. Generate Report Only (No Test Execution)
```bash
bash build_and_run.sh genreport
```

---

## Test Files Reference

### Unit Tests (Google Test)
Located in `test/` directory:

| File | Description | Run Command |
|------|-------------|-------------|
| `AuthenticationTests.cpp` | MAC/Signature generation | `cd build && ./AuthenticationTests.exe` |
| `VerificationTests.cpp` | MAC/Signature verification | `./VerificationTests.exe` |
| `FreshnessTests.cpp` | Anti-replay protection | `./FreshnessTests.exe` |
| `DirectTxTests.cpp` | Direct transmission | `./DirectTxTests.exe` |
| `DirectRxTests.cpp` | Direct reception | `./DirectRxTests.exe` |
| `startOfReceptionTests.cpp` | TP reception | `./startOfReceptionTests.exe` |

**Run all Google Test unit tests:**
```bash
cd build
ctest
```

---

## Documentation Files

1. **TESTING_DOCUMENTATION.md** - Comprehensive testing guide
   - All test cases documented
   - Expected results
   - Performance benchmarks
   - Security test scenarios

2. **test_results.html** - Generated HTML report
   - Interactive dashboard
   - Performance charts
   - Test summary cards
   - Color-coded results

3. **CSV Files** - Raw data for analysis
   - `pqc_standalone_results.csv`
   - `pqc_secoc_integration_results.csv`

---

## Viewing Test Results

### HTML Report (Recommended)
```bash
# After running: bash build_and_run.sh report
# Open in browser:
start test_results.html  # Windows
open test_results.html   # macOS
xdg-open test_results.html  # Linux
```

### CSV Analysis
```bash
# View in Excel/LibreOffice
# Or use command line:
cat pqc_standalone_results.csv
cat pqc_secoc_integration_results.csv
```

---

## Test Results Interpretation

### PQC Standalone Tests
✅ **PASS** - All cryptographic operations work correctly
- ML-KEM-768: KeyGen, Encapsulate, Decapsulate
- ML-DSA-65: KeyGen, Sign, Verify
- Tampering detection working

### PQC Integration Tests
✅ **PASS** - AUTOSAR Csm layer integration successful
- `Csm_SignatureGenerate` working
- `Csm_SignatureVerify` working
- `Csm_MacGenerate` working (classical)
- `Csm_MacVerify` working (classical)
- Replay attack detection
- Tampering detection

### Expected Performance (x86_64)
| Operation | Expected Time |
|-----------|--------------|
| ML-DSA Sign | ~250 μs |
| ML-DSA Verify | ~120 μs |
| AES-CMAC Gen | ~2 μs |
| AES-CMAC Verify | ~2 μs |

---

## Troubleshooting

### "Python not found"
```bash
# Windows (MSYS2/MINGW64)
pacman -S mingw-w64-x86_64-python

# Or download from python.org
```

### "liboqs not found"
```bash
cd Autosar_SecOC
bash build_liboqs.sh
```

### "Test executable not found"
```bash
# Rebuild the specific test
bash build_and_run.sh standalone
# or
bash build_and_run.sh integration
```

### Report Generation Fails
```bash
# Check Python installation
python3 --version
# or
python --version

# Check if script exists
ls generate_test_report.py

# Run manually
python3 generate_test_report.py
```

---

## Example Workflow

### Full Testing Session
```bash
cd Autosar_SecOC

# 1. Run comprehensive tests with report
bash build_and_run.sh report

# 2. View HTML report in browser
start test_results.html

# 3. Check CSV data if needed
cat pqc_standalone_results.csv

# 4. Read detailed documentation
cat TESTING_DOCUMENTATION.md
```

### Quick Test Check
```bash
cd Autosar_SecOC

# Run just integration tests
bash build_and_run.sh integration

# Or just standalone
bash build_and_run.sh standalone
```

---

## Advanced Usage

### Custom Python Analysis
```python
import pandas as pd

# Load results
df = pd.read_csv('pqc_standalone_results.csv')

# Analyze ML-DSA performance
mldsa = df[df['Algorithm'] == 'ML-DSA-65']
print(f"Average sign time: {mldsa[mldsa['Operation'] == 'Sign']['Time_us'].mean()} μs")
```

### Export to Excel
The `generate_test_report.py` script can export to Excel if pandas is installed:
```bash
pip install pandas openpyxl
python3 generate_test_report.py
# Creates test_results.xlsx
```

---

## Summary

**For comprehensive testing and visualization:**
```bash
bash build_and_run.sh report
```

**For detailed documentation:**
```bash
cat TESTING_DOCUMENTATION.md
```

**For individual tests, see table above.**
