# Hướng Dẫn Build & Test — AUTOSAR SecOC with PQC

> Tài liệu này liệt kê toàn bộ các lệnh cần thiết để build và chạy test suite trên **Windows (MinGW)** và **Linux / Raspberry Pi 4**.

---

## WINDOWS (MinGW)

### 1. Build lần đầu (full rebuild)
```bat
cd Autosar_SecOC
bash rebuild_pqc.sh
```

Hoặc build thủ công:
```bat
cd Autosar_SecOC
mkdir build
cd build
cmake -G "MinGW Makefiles" ..
mingw32-make -j4
```

### 2. Build kèm coverage (để đo code coverage)
```bat
cd Autosar_SecOC
rmdir /s /q build
mkdir build
cd build
cmake -G "MinGW Makefiles" -DENABLE_COVERAGE=ON ..
mingw32-make -j4
```

### 3. Chạy toàn bộ test suite
```bat
cd Autosar_SecOC\build
ctest --timeout 30 --output-on-failure
```

### 4. Chạy một test riêng lẻ
```bat
cd Autosar_SecOC\build\test
AuthenticationTests.exe
VerificationTests.exe
FreshnessTests.exe
PduRTests.exe
NvMTests.exe
SecOCTests.exe
PQC_ComparisonTests.exe
```

### 5. Chạy test với output XML (để sinh report)
```bat
cd Autosar_SecOC\build
ctest --timeout 30
```
(CMake đã config sẵn `--gtest_output=xml:build/test_results/<name>.xml`)

### 6. Test PQC functionality riêng
```bat
cd Autosar_SecOC
bash build_test_pqc.sh
```

### 7. Benchmark hiệu năng
```bat
cd Autosar_SecOC
bash build_perf.sh
```

### 8. Sinh coverage report (HTML + Cantata-style)
```bat
cd Autosar_SecOC\build
gcovr --root .. --filter "../source/.*" --exclude "../source/main\.c" --exclude "../external/.*" --html-details coverage_report/index.html --json coverage.json --txt=coverage_report/coverage_summary.txt

python ..\test\generate_report.py --test-results-dir test_results --coverage-json coverage.json --output report/SecOC_Test_Report.html
```

Mở file HTML trong browser:
```bat
start report\SecOC_Test_Report.html
start coverage_report\index.html
```

---

## LINUX / RASPBERRY PI 4

### 1. Cài đặt dependencies (chỉ làm 1 lần)
```bash
sudo apt update
sudo apt install -y build-essential cmake git python3 python3-pip \
                    libssl-dev gcovr can-utils
pip3 install jinja2
```

### 2. Build lần đầu trên Pi 4
```bash
cd Autosar_SecOC
mkdir -p build && cd build
cmake -G "Unix Makefiles" -DMCAL_TARGET=PI4 ..
make -j4
```

### 3. Build kèm coverage
```bash
cd Autosar_SecOC
rm -rf build && mkdir -p build && cd build
cmake -G "Unix Makefiles" -DMCAL_TARGET=PI4 -DENABLE_COVERAGE=ON ..
make -j4
```

### 4. Chạy toàn bộ test suite
```bash
cd Autosar_SecOC/build
ctest --timeout 30 --output-on-failure
```

### 5. Chạy một test riêng lẻ
```bash
cd Autosar_SecOC/build/test
./AuthenticationTests
./VerificationTests
./FreshnessTests
./PduRTests
./NvMTests
./SecOCTests
./PQC_ComparisonTests
```

### 6. Chạy test PQC Phase 3 (integration)
```bash
cd Autosar_SecOC/build
./Phase3_Complete_Test
```

### 7. Build script tổng hợp (tự detect ARM)
```bash
cd Autosar_SecOC
bash build_and_run.sh
```

### 8. Sinh coverage report
```bash
cd Autosar_SecOC/build
gcovr --root .. \
      --filter '../source/.*' \
      --exclude '../source/main\.c' \
      --exclude '../external/.*' \
      --html-details coverage_report/index.html \
      --json coverage.json \
      --txt=coverage_report/coverage_summary.txt

python3 ../test/generate_report.py \
        --test-results-dir test_results \
        --coverage-json coverage.json \
        --output report/SecOC_Test_Report.html
```

Mở report:
```bash
xdg-open report/SecOC_Test_Report.html    # desktop

# Hoặc copy về máy dev:
scp pi@<ip>:~/Autosar_SecOC/build/report/SecOC_Test_Report.html ./
```

### 9. Test trên Pi 4 với CAN thật (sau khi đã setup can0)
```bash
# Setup CAN interface
sudo modprobe can
sudo modprobe can_raw
sudo modprobe mcp251x
sudo ip link set can0 up type can bitrate 500000

# Kiểm tra CAN hoạt động
candump can0 &
cansend can0 123#DEADBEEF

# Chạy integration test
cd Autosar_SecOC/build
./Phase3_Complete_Test
```

### 10. Nếu chưa có CAN hardware, dùng virtual CAN (vcan0)
```bash
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link set up vcan0

# Test hoạt động
candump vcan0 &
cansend vcan0 123#DEADBEEF
```

---

## KẾT QUẢ MONG ĐỢI

Sau khi chạy `ctest`, output phải là:

```
100% tests passed, 0 tests failed out of 40

Total Test time (real) =   X.XX sec
```

| Chỉ số | Giá trị |
|---|---|
| Số test executables | **40** |
| Tổng số test cases | **669 / 669 passed** |
| Line coverage tối thiểu | **~55%** |

---

## TROUBLESHOOTING NHANH

| Lỗi | Nguyên nhân | Cách fix |
|---|---|---|
| `liboqs not found` | Chưa build liboqs | `bash build_liboqs.sh` |
| `gcovr: command not found` | Chưa cài gcovr | `pip3 install gcovr` hoặc `sudo apt install gcovr` |
| `No rule to make target PduRTests` | CMake chưa re-configure sau khi thêm file | `cmake ..` rồi build lại |
| `Cannot find liboqs.dll` (Windows) | DLL chưa trong PATH | Copy `external/liboqs/build/bin/liboqs.dll` vào cùng thư mục exe |
| Test PQC segfault / fail | PQC keys chưa có | OK — test có guard skip khi thiếu key, chỉ cần chạy lại |
| `ctest` không thấy test nào | Chưa build | Build trước bằng `make -j4` |

---

## LỆNH QUICK TEST (copy-paste 1 lần)

**Windows:**
```bat
cd Autosar_SecOC && rmdir /s /q build && mkdir build && cd build && cmake -G "MinGW Makefiles" -DENABLE_COVERAGE=ON .. && mingw32-make -j4 && ctest --timeout 30 --output-on-failure
```

**Linux / Pi 4:**
```bash
cd Autosar_SecOC && rm -rf build && mkdir -p build && cd build && cmake -DMCAL_TARGET=PI4 -DENABLE_COVERAGE=ON .. && make -j4 && ctest --timeout 30 --output-on-failure
```

Chạy một dòng duy nhất là xong — build từ đầu + test toàn bộ + show kết quả.

> Nếu thấy `100% tests passed` là OK để bàn giao.
