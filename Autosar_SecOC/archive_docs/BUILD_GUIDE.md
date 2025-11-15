# Build & Test Guide - AUTOSAR SecOC with PQC

## Quick Start (Recommended Workflow)

### 1. Build the Comprehensive PQC Test
```bash
bash build_and_run.sh comprehensive
```

This builds `test_pqc_detailed.exe` which includes:
- ✅ **Phase 1.1:** ML-KEM-768 standalone testing (1000 iterations)
- ✅ **Phase 1.2:** ML-DSA-65 standalone testing (1000 iterations × 5 sizes)
- ✅ **Phase 2:** Classical comparison (AES-CMAC)
- ✅ **Detailed Metrics:** CPU time, Memory, Throughput, Correctness

### 2. Run All Tests
```bash
bash build_and_run.sh test
```

This automatically runs all available test executables:
- `test_pqc_detailed.exe` (if built)
- `test_autosar_integration.exe` (if built)
- `test_pqc_advanced.exe` (if built)

### 3. View Results
```bash
# Check CSV output
cat pqc_detailed_results.csv

# Or launch visualization dashboard
bash build_and_run.sh viz
```

---

## All Available Commands

| Command | Description |
|---------|-------------|
| `bash build_and_run.sh comprehensive` | Build comprehensive PQC test ⭐ **RECOMMENDED** |
| `bash build_and_run.sh all` | Build everything (all tests) |
| `bash build_and_run.sh liboqs` | Build only liboqs library |
| `bash build_and_run.sh integration` | Build AUTOSAR integration test |
| `bash build_and_run.sh advanced` | Build advanced PQC metrics |
| `bash build_and_run.sh test` | Run all available tests |
| `bash build_and_run.sh viz` | Launch visualization dashboard |
| `bash build_and_run.sh demo` | Quick demo (build + test + viz) |

---

## Build Details

### Comprehensive PQC Test Build Command
```bash
gcc -o test_pqc_detailed.exe \
    test_pqc_comprehensive_detailed.c \
    source/PQC/PQC.c \
    source/PQC/PQC_KeyExchange.c \
    source/Csm/Csm.c \
    source/Encrypt/encrypt.c \
    source/SecOC/SecOC.c \
    source/SecOC/FVM.c \
    source/SecOC/SecOC_Lcfg.c \
    source/SecOC/SecOC_PBcfg.c \
    -I include \
    -I include/PQC \
    -I include/Csm \
    -I include/Encrypt \
    -I include/SecOC \
    -I external/liboqs/build/include \
    -L external/liboqs/build/lib \
    -loqs \
    -lm \
    -lpthread \
    -DWINDOWS  # or -DRASPBERRY_PI for RPi
```

### What Gets Built

| Test Executable | Source File | Output CSV |
|----------------|-------------|------------|
| `test_pqc_detailed.exe` | `test_pqc_comprehensive_detailed.c` | `pqc_detailed_results.csv` |
| `test_autosar_integration.exe` | `test_autosar_integration_comprehensive.c` | `autosar_integration_results.csv` |
| `test_pqc_advanced.exe` | `test_pqc_advanced.c` | `pqc_performance_results.csv` |

---

## Test Phases Explained

### Phase 1.1: ML-KEM-768 Testing
```
┌─────────────────────────────────┐
│ ML-KEM-768 Standalone Tests     │
├─────────────────────────────────┤
│ • KeyGen (1000 iter)            │
│ • Encapsulation (1000 iter)     │
│ • Decapsulation (1000 iter)     │
│                                 │
│ Metrics Collected:              │
│ ✓ Time: min/max/avg/stddev      │
│ ✓ Throughput (ops/sec)          │
│ ✓ Memory (KB)                   │
│ ✓ Key sizes (bytes)             │
│ ✓ Correctness rate (%)          │
└─────────────────────────────────┘
```

### Phase 1.2: ML-DSA-65 Testing
```
┌─────────────────────────────────┐
│ ML-DSA-65 Standalone Tests      │
├─────────────────────────────────┤
│ • KeyGen (1000 iter)            │
│ • Sign (5 sizes × 1000 iter)    │
│   - 8, 64, 256, 512, 1024 bytes │
│ • Verify (5 sizes × 1000 iter)  │
│                                 │
│ Metrics Collected:              │
│ ✓ Time per message size         │
│ ✓ Signature size (3309 bytes)   │
│ ✓ Validity rate                 │
│ ✓ Memory footprint              │
└─────────────────────────────────┘
```

### Phase 2: Comparison & Integration
```
┌─────────────────────────────────┐
│ Classical Algorithm Comparison  │
├─────────────────────────────────┤
│ • AES-128-CMAC (1000 iter)      │
│ • RSA-2048 (optional)           │
│ • ECDH-256 (optional)           │
│                                 │
│ Overhead Analysis:              │
│ ✓ Time overhead ratio           │
│ ✓ Size overhead ratio           │
│ ✓ Throughput comparison         │
└─────────────────────────────────┘

┌─────────────────────────────────┐
│ AUTOSAR Integration (Optional)  │
├─────────────────────────────────┤
│ • COM → SecOC → PQC flow        │
│ • Security attack tests         │
│ • End-to-end latency            │
└─────────────────────────────────┘
```

---

## Expected Output

### Console Output Sample
```
╔══════════════════════════════════════════════════════════════╗
║          PHASE 1.1: ML-KEM-768 STANDALONE TESTING            ║
╚══════════════════════════════════════════════════════════════╝

Algorithm: ML-KEM-768 (NIST FIPS 203)
Security Level: Category 3 (AES-192 equivalent)

┌─────────────────────────────────────────────────────────┐
│  Testing ML-KEM-768 Key Generation                     │
└─────────────────────────────────────────────────────────┘

  🔥 Warming up (50 iterations)...
  📊 Benchmarking key generation (1000 iterations)...

  ✅ Key Generation Results:
  ┌────────────────────────┬────────────────┐
  │ Metric                 │ Value          │
  ├────────────────────────┼────────────────┤
  │ Average Time           │     2847.23 µs │
  │ Min Time               │     2654.12 µs │
  │ Max Time               │     3145.67 µs │
  │ Std Deviation          │      152.34 µs │
  │ Throughput             │      351.2/s   │
  │ Public Key Size        │     1184 B     │
  │ Secret Key Size        │     2400 B     │
  │ Success Rate           │   100.00 %     │
  │ Memory Usage           │      245 KB    │
  └────────────────────────┴────────────────┘
```

### CSV Output Sample (`pqc_detailed_results.csv`)
```csv
=== ML-KEM-768 Results ===
Operation,Avg_Time_us,Min_us,Max_us,Stddev_us,Throughput_ops_sec,Size_bytes
KeyGen,2847.23,2654.12,3145.67,152.34,351,3584
Encapsulate,3124.45,2987.23,3345.89,89.12,320,1088
Decapsulate,3892.67,3756.34,4123.45,101.23,257,2400

=== ML-DSA-65 Results ===
Message_Size,Sign_Avg_us,Sign_Min_us,Sign_Max_us,Verify_Avg_us,Verify_Min_us,Verify_Max_us,Signature_Size,Sign_Throughput,Verify_Throughput
8,8127.34,7845.23,8567.89,4893.45,4678.12,5234.56,3309,123,204
64,8156.78,7901.34,8623.45,4921.23,4712.45,5267.89,3309,122,203
...
```

---

## Troubleshooting

### Build Fails - liboqs not found
```bash
# Build liboqs first
bash build_and_run.sh liboqs

# Then build comprehensive test
bash build_and_run.sh comprehensive
```

### Missing math.h errors
- Already fixed in `test_pqc_comprehensive_detailed.c` (line 20: `#include <math.h>`)

### Compilation errors on Windows
- Make sure you're using MSYS2/MinGW64
- Verify gcc version: `gcc --version`
- Check CMake is available: `cmake --version`

### Test executable not found
```bash
# Check if file exists
ls -la test_pqc_detailed.exe

# If missing, rebuild
bash build_and_run.sh comprehensive
```

---

## Integration with Diagrams

The test results can be visualized using:

1. **Mermaid Diagrams** - See `DIAGRAMS.md`
   - Diagram #16: ML-KEM vs ML-DSA comparison
   - Diagram #17: Two-phase testing architecture
   - Diagram #18: Metrics collection breakdown
   - Diagram #19: ML-KEM-768 operation flow
   - Diagram #20: ML-DSA-65 operation flow

2. **Python Dashboard** - Run `pqc_premium_dashboard.py`
   ```bash
   bash build_and_run.sh viz
   ```

---

## Platform-Specific Notes

### Windows (MSYS2/MinGW64)
- Uses `-DWINDOWS` flag
- Requires Winsock2 libraries
- Interactive menu may not work → Use command-line arguments

### Raspberry Pi 4
- Uses `-DRASPBERRY_PI -mcpu=cortex-a72` flags
- BSD sockets for networking
- Optimized ARM build for liboqs
- Expect ~1.5x slower than x86_64

---

## Files Generated

After running tests, you'll have:

```
Autosar_SecOC/
├── test_pqc_detailed.exe                 # Comprehensive test executable
├── pqc_detailed_results.csv              # Detailed metrics CSV
├── test_autosar_integration.exe          # Integration test (optional)
├── autosar_integration_results.csv       # Integration results (optional)
└── external/liboqs/build/lib/liboqs.a   # PQC library
```

---

## Next Steps

After successful testing:

1. **Analyze Results**
   - Review CSV files
   - Compare PQC vs Classical metrics
   - Check correctness rates (should be 100%)

2. **Create Presentation**
   - Use DIAGRAMS.md for visualizations
   - Include performance comparison charts
   - Highlight quantum-resistance benefits

3. **Deploy to Raspberry Pi**
   - Follow RASPBERRY_PI_MIGRATION_GUIDE.md
   - Test with real hardware (MCP2515 CAN)
   - Validate Ethernet gateway functionality

4. **Report to Teacher**
   - Present TECHNICAL_REPORT.md
   - Show live test execution
   - Demonstrate dashboard visualization

---

**For questions or issues, check:**
- Technical details: `TECHNICAL_REPORT.md`
- Architecture diagrams: `DIAGRAMS.md`
- Project overview: `README.md`
