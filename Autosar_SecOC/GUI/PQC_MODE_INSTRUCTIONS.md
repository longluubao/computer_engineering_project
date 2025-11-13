# GUI PQC Mode Integration Instructions

## Quick Integration Steps

### 1. Add PQC Toggle to GUI

Add this to your main dialog UI (or simple_gui.py):

```python
# Add after existing buttons
self.pqcModeCheckbox = QCheckBox("PQC Mode (ML-DSA)")
self.pqcModeCheckbox.setChecked(False)  # Default: MAC mode
```

### 2. Update connections.py

Add to `__init__`:
```python
# PQC mode state
self.pqc_mode_enabled = False

# Connect PQC checkbox
self.dialog.pqcModeCheckbox.stateChanged.connect(self.OnPQCModeChanged)

# Declare PQC functions
self.mylib.GUIInterface_authenticate_PQC.argtypes = [c_uint8, POINTER(c_ubyte), c_uint8]
self.mylib.GUIInterface_authenticate_PQC.restype = c_char_p
self.mylib.GUIInterface_verify_PQC.argtypes = [c_uint8]
self.mylib.GUIInterface_verify_PQC.restype = c_char_p
```

Add new method:
```python
def OnPQCModeChanged(self, state):
    self.pqc_mode_enabled = (state == Qt.Checked)
    mode_text = "PQC (ML-DSA-65)" if self.pqc_mode_enabled else "MAC (HMAC)"
    self.dialog.tlog.info(f"Switched to {mode_text} mode")
```

### 3. Update OnAccelButtonClicked

Replace:
```python
# Old
self.mylib.GUIInterface_authenticate(currentIndex, data, dataLen)
```

With:
```python
# New - use PQC if enabled
if self.pqc_mode_enabled:
    self.mylib.GUIInterface_authenticate_PQC(currentIndex, data, dataLen)
else:
    self.mylib.GUIInterface_authenticate(currentIndex, data, dataLen)
```

### 4. Update OnVerifyButtonClicked

Replace:
```python
# Old
result = self.mylib.GUIInterface_verify(self.current_rx_id)
```

With:
```python
# New - use PQC if enabled
if self.pqc_mode_enabled:
    result = self.mylib.GUIInterface_verify_PQC(self.current_rx_id)
else:
    result = self.mylib.GUIInterface_verify(self.current_rx_id)
```

### 5. Apply same changes to:
- `OnDecelButtonClicked()`
- `OnShowTimeButtonClicked()`
- `OnShowDateButtonClicked()`

## Example: Complete OnAccelButtonClicked

```python
def OnAccelButtonClicked(self):
    mode = "PQC" if self.pqc_mode_enabled else "MAC"
    self.dialog.tlog.debug(f"Accelerate ⬆ ({mode})")

    currentIndex = self.dialog.configSelect.currentIndex()

    if currentIndex in [3]:
        data = (c_ubyte * 4)(1,0,0,0)
    elif currentIndex in [4]:
        data = (c_ubyte * 19)(1,0,0,0)
    else:
        data = (c_ubyte * 1)(1)

    dataLen = len(data)

    # Use PQC or MAC based on mode
    if self.pqc_mode_enabled:
        self.mylib.GUIInterface_authenticate_PQC(currentIndex, data, dataLen)
    else:
        self.mylib.GUIInterface_authenticate(currentIndex, data, dataLen)

    self.UpdateTransmitterSecPayload()
```

## Testing PQC Mode

1. Rebuild: `bash rebuild_pqc.sh`
2. Run GUI: `cd GUI && python simple_gui.py`
3. Check "PQC Mode" checkbox
4. Click "Accelerate" - should see much larger secured PDU (signature is 3309 bytes)
5. Transmit and Verify
6. Try "Alter Authenticator" attack - should fail verification

## Performance Note

With PQC mode:
- Secured PDU size increases from ~8 bytes to ~3300+ bytes
- Authentication takes ~125x longer than MAC
- Verification takes ~60x longer than MAC
- But provides quantum resistance!

## Visual Feedback

The GUI should show:
- Checkbox state: MAC vs PQC mode
- PDU size difference (huge!)
- Performance difference (slower but acceptable for Ethernet Gateway)
