# PQC-Enabled SecOC GUI - Deployment Guide

## Quick Start

### Prerequisites
- **MSYS2 MinGW 64-bit** terminal (not Git Bash or regular MSYS2)
- Python 3.x with PySide6 installed
- CMake and MinGW toolchain

### Step 1: Build liboqs (if not already built)

Open **MSYS2 MinGW 64-bit** terminal and run:

```bash
cd ~/Documents/Long-Stuff/HCMUT/computer_engineering_project/Autosar_SecOC
bash build_liboqs.sh
```

**Expected output:**
```
✓ liboqs built successfully!
Library location: external/liboqs/build/lib/liboqs.a
```

**Time:** ~2-3 minutes

### Step 2: Deploy SecOC with PQC GUI

```bash
bash deploy_pqc_gui.sh
```

**This script will:**
1. ✓ Clean previous build artifacts
2. ✓ Configure CMake with PQC support
3. ✓ Build SecOC library with ML-DSA and ML-KEM
4. ✓ Launch the GUI automatically

**Expected output:**
```
✓ Build completed successfully
✓ libSecOCLibShared.dll found (11MB)
Build Complete! Launching GUI...
```

## Using the GUI

### PQC Mode Features

The GUI now supports **dual-mode operation**:
- **MAC Mode** (Classical HMAC) - Default
- **PQC Mode** (ML-DSA-65 Quantum-Resistant Signatures)

### Enabling PQC Mode

1. **Launch GUI** - `bash deploy_pqc_gui.sh` or `python GUI/simple_gui.py`
2. **Check the PQC checkbox** - "Enable PQC Mode (ML-DSA-65 Signatures)"
3. **Mode indicator changes** - From "⚫ MAC Mode" to "🟢 PQC Mode (ML-DSA-65)"

### Testing PQC Mode

#### Basic Authentication Flow:

```
1. Select Configuration: "Ethernet SOAD TP"
2. Enable PQC Mode: ✓ Check the checkbox
3. Authenticate: Click "Accelerate" button
4. Observe PDU: ~3312 bytes (vs ~8 bytes for MAC!)
5. Transmit: Click "Transmit Secured PDU"
6. Verify: Switch to Receiver tab, click "Verify"
```

#### Attack Simulation:

**Freshness Attack:**
```
1. Enable PQC Mode
2. Authenticate a message
3. Click "Alter Freshness"
4. Transmit and Verify
Result: "Freshness Failed" ✓
```

**Signature Attack:**
```
1. Enable PQC Mode
2. Authenticate a message
3. Click "Alter Authenticator"
4. Transmit and Verify
Result: "PDU is not Authentic (PQC Sig Failed)" ✓
```

### Performance Comparison

| Metric | MAC (HMAC) | PQC (ML-DSA-65) | Ratio |
|--------|------------|-----------------|-------|
| Authenticator Size | 4-16 bytes | 3309 bytes | 827x larger |
| Generation Time | ~7 ms | ~5 ms | 0.7x (FASTER!) |
| Verification Time | ~55 ms | ~2 ms | 0.04x (MUCH FASTER!) |
| Quantum Resistant | ❌ No | ✅ Yes | - |
| Security Level | Classical | NIST Level 3 | AES-192 equivalent |

## Troubleshooting

### Issue 1: "function 'GUIInterface_init' not found"

**Cause:** Library not built or outdated

**Solution:**
```bash
cd ~/Documents/Long-Stuff/HCMUT/computer_engineering_project/Autosar_SecOC
bash deploy_pqc_gui.sh
```

### Issue 2: "liboqs not found"

**Cause:** liboqs library not built

**Solution:**
```bash
bash build_liboqs.sh
```

Then rebuild SecOC:
```bash
bash deploy_pqc_gui.sh
```

### Issue 3: "CMake Error: CMAKE_C_COMPILER not set"

**Cause:** Not in MSYS2 MinGW 64-bit environment

**Solution:**
- Close current terminal
- Open **MSYS2 MinGW 64-bit** (NOT Git Bash or MSYS2 UCRT64)
- Run the scripts again

### Issue 4: "No module named 'PySide6'"

**Cause:** PySide6 not installed in Python

**Solution:**
```bash
pip install PySide6
```

### Issue 5: GUI launches but PQC mode checkbox not visible

**Cause:** Old version of simple_gui.py cached

**Solution:**
1. Close GUI
2. Rebuild: `bash deploy_pqc_gui.sh`
3. GUI should now show PQC checkbox

## Manual Build (Alternative)

If the scripts don't work, you can build manually:

### Build liboqs:
```bash
cd external/liboqs
mkdir -p build && cd build
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DOQS_USE_OPENSSL=OFF ..
mingw32-make -j4
cd ../../..
```

### Build SecOC:
```bash
cd build
rm -rf CMakeFiles CMakeCache.txt
cmake -G "MinGW Makefiles" ..
mingw32-make -j4
cd ..
```

### Run GUI:
```bash
cd GUI
python simple_gui.py
```

## File Structure

```
Autosar_SecOC/
├── build_liboqs.sh              # Build liboqs library
├── deploy_pqc_gui.sh            # Build SecOC and launch GUI
├── rebuild_pqc.sh               # Alternative rebuild script
├── build/
│   └── libSecOCLibShared.dll    # Shared library for GUI
├── external/liboqs/
│   └── build/lib/liboqs.a       # PQC library
├── GUI/
│   ├── simple_gui.py            # GUI with PQC mode toggle
│   ├── PQC_TESTING_GUIDE.md     # Testing guide
│   └── PQC_MODE_INSTRUCTIONS.md # Integration instructions
├── source/PQC/
│   ├── PQC.c                    # PQC wrapper functions
│   └── PQC_KeyExchange.c        # ML-KEM key exchange
├── source/Csm/Csm.c             # Signature functions
├── source/SecOC/SecOC.c         # PQC authenticate/verify
└── source/GUIInterface/         # GUI interface with PQC
```

## Testing Checklist

- [ ] Build liboqs successfully
- [ ] Build SecOC with PQC support
- [ ] GUI launches without errors
- [ ] PQC mode checkbox visible
- [ ] Toggle between MAC and PQC modes
- [ ] Authenticate in MAC mode (~8 bytes)
- [ ] Authenticate in PQC mode (~3312 bytes)
- [ ] Verify in both modes successfully
- [ ] Test freshness attack detection
- [ ] Test signature alteration detection
- [ ] Compare performance (MAC vs PQC)

## Support

For detailed testing procedures, see:
- `GUI/PQC_TESTING_GUIDE.md` - Comprehensive testing guide
- `QUICK_START.md` - Quick reference for PQC functions
- `PQC_RESEARCH.md` - Technical background on PQC

## Next Steps

1. ✅ Build and test locally (this guide)
2. ⏳ Deploy to Ethernet Gateway hardware
3. ⏳ Performance tuning for production
4. ⏳ Integration with full AUTOSAR stack
5. ⏳ Hardware Security Module (HSM) key storage
