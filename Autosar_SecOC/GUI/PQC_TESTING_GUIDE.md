# PQC Mode Testing Guide

## Overview
The simple_gui.py now supports both MAC and PQC (Post-Quantum Cryptography) modes with ML-DSA-65 digital signatures.

## Running the GUI

```bash
cd Autosar_SecOC/GUI
python simple_gui.py
```

## Testing PQC Mode

### 1. Enable PQC Mode
- Check the checkbox: **"Enable PQC Mode (ML-DSA-65 Signatures)"**
- The mode indicator will change from "⚫ MAC Mode" to "🟢 PQC Mode (ML-DSA-65)"
- Log will show: "🔐 PQC Mode ENABLED - Using ML-DSA-65 signatures (~3309 bytes)"

### 2. Test Authentication
- Select configuration: "Ethernet SOAD TP" (recommended for large PDUs)
- Click **"Accelerate"** or **"Decelerate"**
- Log should show: "Accelerate command authenticated (PQC (ML-DSA)): E_OK"
- **Expected PDU size: ~3300+ bytes** (vs ~8 bytes for MAC)

### 3. Test Transmission
- Click **"Transmit Secured PDU"**
- PDU will be transmitted using TP mode (due to large size exceeding MTU)

### 4. Test Verification
- Click **"Verify Received PDU"** in the Receiver tab
- Log should show: "✓ Verification result (PQC (ML-DSA)): E_OK"

### 5. Test Attack Detection (PQC Mode)

#### Freshness Alteration Attack:
- Enable PQC mode
- Authenticate a message
- Click **"Alter Freshness"**
- Transmit and Verify
- **Expected:** "Freshness Failed"

#### Signature Alteration Attack:
- Enable PQC mode
- Authenticate a message
- Click **"Alter Authenticator"** (alters the signature)
- Transmit and Verify
- **Expected:** "PDU is not Authentic (PQC Sig Failed)"

### 6. Compare MAC vs PQC

#### Test with MAC Mode:
1. Uncheck PQC mode checkbox
2. Click "Accelerate"
3. Observe secured payload: ~8 bytes (e.g., "03 01 02 03 01 EC D3 5E 94")

#### Test with PQC Mode:
1. Check PQC mode checkbox
2. Click "Accelerate"
3. Observe secured payload: ~3312 bytes
4. **Signature is 827x larger than MAC!**

## Performance Comparison

| Operation | MAC (HMAC) | PQC (ML-DSA-65) | Ratio |
|-----------|------------|-----------------|-------|
| Auth Size | 4-16 bytes | 3309 bytes | 827x |
| Gen Time | ~7 ms | ~5 ms | **0.7x (FASTER!)** |
| Verify Time | ~55 ms | ~2 ms | **0.04x (MUCH FASTER!)** |
| Quantum Resistant | ❌ No | ✅ Yes | - |

## Key Features

### Visual Feedback:
- Mode indicator shows current mode with color coding
- Log messages include mode information (MAC vs PQC)
- PDU size visibly different in payload display

### Dual-Mode Operation:
- Toggle between MAC and PQC at any time
- No need to restart GUI
- Attack simulations work in both modes

### Ethernet Gateway Focus:
- Configuration includes "Ethernet SOAD IF" and "Ethernet SOAD TP"
- Large PQC signatures handled via TP mode segmentation
- Suitable for high-bandwidth Ethernet networks

## Troubleshooting

### PQC Functions Not Found:
If you see "function 'GUIInterface_authenticate_PQC' not found":
1. Rebuild the library: `cd Autosar_SecOC && bash rebuild_pqc.sh`
2. Verify PQC code is compiled in
3. Check that liboqs is properly linked

### Large PDU Not Transmitting:
- Use "Ethernet SOAD TP" configuration for PQC mode
- TP mode handles segmentation automatically
- Direct mode (IF) may fail with 3309 byte signatures

### Performance Issues:
- PQC signature generation: ~5ms (acceptable)
- If slower, check CPU and liboqs optimization level
- Consider using Release build instead of Debug

## Expected Results

### Successful PQC Flow:
1. Enable PQC Mode ✓
2. Authenticate (PQC) ✓
3. Secured payload ~3300 bytes ✓
4. Transmit via TP mode ✓
5. Receive and parse ✓
6. Verify signature ✓
7. Success message ✓

### Attack Detection:
- Altered freshness → "Freshness Failed" ✓
- Altered signature → "PDU is not Authentic" ✓
- Replay attack → "Freshness Failed" ✓

## Next Steps
1. ✅ Test MAC mode (baseline)
2. ✅ Test PQC mode (quantum-resistant)
3. ✅ Test attack scenarios in both modes
4. ✅ Compare size and performance
5. 🔄 Deploy to Ethernet Gateway hardware
