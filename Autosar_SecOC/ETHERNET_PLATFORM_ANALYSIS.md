# Ethernet Platform Analysis
## Winsock2 (Windows) vs BSD Sockets (Linux/Raspberry Pi)

**Document Purpose**: Technical analysis of the dual-platform Ethernet implementation in AUTOSAR SecOC

---

## 📋 Executive Summary

The AUTOSAR SecOC project implements **dual-platform Ethernet communication**:
- **Windows Development**: Uses Winsock2 API (`ws2_32.dll`)
- **Raspberry Pi Deployment**: Uses BSD sockets (POSIX standard)

Both implementations provide the same TCP/IP socket interface, allowing seamless development on Windows and deployment on Raspberry Pi.

---

## 🏗️ Architecture Overview

### Platform-Specific Files

```
Autosar_SecOC/
├── source/Ethernet/
│   ├── ethernet.c              # BSD sockets (Linux/Raspberry Pi)
│   ├── ethernet_windows.c      # Winsock2 (Windows)
│   └── ip_address.txt          # Target IP configuration
├── include/Ethernet/
│   ├── ethernet.h              # BSD sockets header
│   └── ethernet_windows.h      # Winsock2 header
└── CMakeLists.txt              # Platform detection & build config
```

### CMake Platform Detection

**File**: `CMakeLists.txt` (Lines 5-28)

```cmake
if (WIN32)
    # Windows build configuration
    file(GLOB_RECURSE SCR_FILES "source/*.c")
    file(GLOB_RECURSE HDR_FILES "include/*.h")

    # Remove Linux-specific files
    list(REMOVE_ITEM SCR_FILES "${CMAKE_SOURCE_DIR}/source/Ethernet/ethernet.c")
    list(REMOVE_ITEM HDR_FILES "${CMAKE_SOURCE_DIR}/include/Ethernet/ethernet.h")
    list(REMOVE_ITEM SCR_FILES "${CMAKE_SOURCE_DIR}/source/Scheduler/scheduler.c")
    list(REMOVE_ITEM HDR_FILES "${CMAKE_SOURCE_DIR}/include/Scheduler/scheduler.h")

    # Add Windows-specific files
    list(APPEND SCR_FILES "${CMAKE_SOURCE_DIR}/source/Ethernet/ethernet_windows.c")
    list(APPEND HDR_FILES "${CMAKE_SOURCE_DIR}/include/Ethernet/ethernet_windows.h")

elseif(UNIX)
    # Linux/Raspberry Pi build configuration
    file(GLOB_RECURSE SCR_FILES "source/*.c")
    file(GLOB_RECURSE HDR_FILES "include/*.h")

    # Remove Windows-specific files
    list(REMOVE_ITEM SCR_FILES "${CMAKE_SOURCE_DIR}/source/Ethernet/ethernet_windows.c")
    list(REMOVE_ITEM HDR_FILES "${CMAKE_SOURCE_DIR}/include/Ethernet/ethernet_windows.h")

    set(CMAKE_CXX_COMPILER "/usr/bin/g++")
    set(CMAKE_C_COMPILER "/usr/bin/gcc")
    set(CMAKE_GENERATOR "Unix Makefiles")
endif()
```

### Compiler Definitions & Linking

**Lines 67-73**:

```cmake
if(WIN32)
    target_compile_definitions(SecOC PUBLIC WINDOWS)
    # Link Winsock2 library for Windows sockets
    target_link_libraries(SecOC ws2_32)
elseif(UNIX)
    target_compile_definitions(SecOC PUBLIC LINUX)
endif()
```

**Key Points**:
- `WINDOWS` preprocessor definition enables Winsock2 code paths
- `ws2_32` is the Windows Socket API 2.0 library
- `LINUX` definition enables POSIX BSD socket code paths

---

## 🔌 Winsock2 Implementation (Windows)

### File: `ethernet_windows.c`

**Key Features**:

#### 1. Winsock Initialization

```c
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

static WSADATA wsa_data;
static boolean winsock_initialized = FALSE;

static Std_ReturnType winsock_init(void)
{
    if (!winsock_initialized)
    {
        if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
        {
            printf("WSAStartup failed\n");
            return E_NOT_OK;
        }
        winsock_initialized = TRUE;
    }
    return E_OK;
}
```

**Purpose**:
- `WSAStartup()` initializes Winsock2 DLL (version 2.2)
- Must be called before any socket operations
- `WSADATA` structure receives implementation details

#### 2. Socket Creation (Winsock2)

```c
SOCKET network_socket;
network_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
if (network_socket == INVALID_SOCKET)
{
    printf("Create Socket Error: %d\n", WSAGetLastError());
    return E_NOT_OK;
}
```

**Winsock2 Specifics**:
- Returns `SOCKET` type (Windows handle)
- Error check: `INVALID_SOCKET` (not `-1` like BSD)
- Error retrieval: `WSAGetLastError()` (not `errno`)

#### 3. TCP Connection (Send)

```c
struct sockaddr_in server_address;
server_address.sin_family = AF_INET;
server_address.sin_port = htons(PORT_NUMBER);
inet_pton(AF_INET, ip_address_send, &server_address.sin_addr);

int connection_status = connect(network_socket,
                                (struct sockaddr*)&server_address,
                                sizeof(server_address));
if (connection_status == SOCKET_ERROR)
{
    printf("Connection Error: %d\n", WSAGetLastError());
    closesocket(network_socket);  // Winsock-specific close
    return E_NOT_OK;
}
```

**Winsock2 Specifics**:
- Uses `inet_pton()` (standard across both platforms)
- Error check: `SOCKET_ERROR` constant
- Socket closure: `closesocket()` (not `close()`)

#### 4. Data Transmission

```c
uint8 sendData[BUS_LENGTH_RECEIVE + sizeof(id)] = {0};
memcpy(sendData, data, dataLen);

// Append message ID (2 bytes, little-endian)
for (unsigned char indx = 0; indx < sizeof(id); indx++)
{
    sendData[BUS_LENGTH_RECEIVE + indx] = (id >> (8 * indx));
}

send(network_socket, (const char*)sendData, 10, 0);
closesocket(network_socket);
```

**Protocol**:
- Fixed frame: **[8-byte payload] + [2-byte message ID]** = 10 bytes total
- TCP stream (connection-oriented, reliable)

#### 5. TCP Server (Receive)

```c
SOCKET server_socket, client_socket;
server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

// Bind to INADDR_ANY (0.0.0.0) - listen on all interfaces
struct sockaddr_in server_address;
server_address.sin_family = AF_INET;
server_address.sin_port = htons(PORT_NUMBER);
server_address.sin_addr.s_addr = INADDR_ANY;

// Enable socket reuse
BOOL opt = TRUE;
setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address));
listen(server_socket, 5);  // Backlog of 5 connections

client_socket = accept(server_socket, NULL, NULL);

// Receive data
unsigned char recData[dataLen + sizeof(unsigned short)];
int recv_result = recv(client_socket, (char*)recData,
                       (dataLen + sizeof(unsigned short)), 0);

memcpy(id, recData + dataLen, sizeof(unsigned short));  // Extract ID
memcpy(data, recData, dataLen);                         // Extract payload

closesocket(client_socket);
closesocket(server_socket);
```

**Key Differences from BSD Sockets**:
- `SOCKET` type instead of `int`
- `BOOL` type for socket options
- `closesocket()` instead of `close()`
- `WSAGetLastError()` instead of `errno`

---

## 🐧 BSD Sockets Implementation (Linux/Raspberry Pi)

### File: `ethernet.c`

**Key Features**:

#### 1. No Initialization Required

```c
void ethernet_init(void)
{
    // No WSAStartup() needed - BSD sockets always available

    // Read IP address from configuration file
    FILE* fp = fopen("./source/Ethernet/ip_address.txt", "r");
    if (fp == NULL) {
        printf("Error opening file\n");
        return;
    }
    fgets(ip_address_read, 16, fp);
    fclose(fp);

    strcpy(ip_address_send, ip_address_read);
}
```

**Key Point**: BSD sockets are built into POSIX OS - no DLL initialization

#### 2. Socket Creation (BSD)

```c
int network_socket;
if ((network_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
{
    printf("Create Socket Error\n");
    return E_NOT_OK;
}
```

**BSD Specifics**:
- Returns `int` file descriptor (not `SOCKET` handle)
- Error check: `< 0` (traditional UNIX convention)
- Error retrieval: `errno` global variable

#### 3. TCP Connection (Send)

```c
struct sockaddr_in server_address;
server_address.sin_family = AF_INET;
server_address.sin_port = htons(PORT_NUMBER);
server_address.sin_addr.s_addr = inet_addr(ip_address_send);

int connection_status = connect(network_socket,
                                (struct sockaddr*)&server_address,
                                sizeof(server_address));
if (connection_status != 0)
{
    printf("Connection Error\n");
    return E_NOT_OK;
}
```

**BSD Specifics**:
- Uses `inet_addr()` (deprecated but still works)
- Error check: `!= 0` (POSIX standard)
- No special error function (uses `perror()` or `strerror(errno)`)

#### 4. Data Transmission (Same as Windows)

```c
uint8 sendData[BUS_LENGTH_RECEIVE + sizeof(id)] = {0};
memcpy(sendData, data, dataLen);

for (unsigned char indx = 0; indx < sizeof(id); indx++)
{
    sendData[BUS_LENGTH_RECEIVE + indx] = (id >> (8 * indx));
}

send(network_socket, sendData, 10, 0);  // No (char*) cast needed
close(network_socket);  // BSD close (not closesocket)
```

#### 5. TCP Server (Receive)

```c
int server_socket, client_socket;
server_socket = socket(AF_INET, SOCK_STREAM, 0);

struct sockaddr_in server_address;
server_address.sin_family = AF_INET;
server_address.sin_port = htons(PORT_NUMBER);
server_address.sin_addr.s_addr = INADDR_ANY;

// Enable socket reuse (IMPORTANT for rapid restart)
int opt = 1;
setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
           &opt, sizeof(opt));

bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address));
listen(server_socket, 5);

client_socket = accept(server_socket, NULL, NULL);

unsigned char recData[dataLen + sizeof(unsigned short)];
recv(client_socket, recData, (dataLen + sizeof(unsigned short)), 0);

memcpy(id, recData + dataLen, sizeof(unsigned short));
memcpy(data, recData, dataLen);

close(client_socket);
close(server_socket);
```

**BSD Specifics**:
- `int` type for sockets and options
- `SO_REUSEADDR | SO_REUSEPORT` bitwise OR
- `close()` function (POSIX standard)

#### 6. Optional: pthread Support

```c
#ifdef SCHEDULER_ON
    #include <pthread.h>
    pthread_mutex_t lock;
#endif

// In send/receive functions:
#ifdef SCHEDULER_ON
    pthread_mutex_lock(&lock);
    // Critical section
    pthread_mutex_unlock(&lock);
#endif
```

**Purpose**: Thread-safe Ethernet operations for multi-threaded AUTOSAR stack

---

## 📊 Side-by-Side Comparison

| Feature | Windows (Winsock2) | Linux/Raspberry Pi (BSD) |
|---------|-------------------|--------------------------|
| **Header Files** | `<winsock2.h>`, `<ws2tcpip.h>` | `<sys/socket.h>`, `<netinet/in.h>`, `<arpa/inet.h>` |
| **Initialization** | `WSAStartup(MAKEWORD(2,2), &wsa_data)` | None (built-in) |
| **Socket Type** | `SOCKET` (handle) | `int` (file descriptor) |
| **Socket Creation** | `socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)` | `socket(AF_INET, SOCK_STREAM, 0)` |
| **Invalid Socket** | `INVALID_SOCKET` | `< 0` |
| **Error Check** | `== SOCKET_ERROR` | `< 0` or `!= 0` |
| **Error Retrieval** | `WSAGetLastError()` | `errno` |
| **Socket Close** | `closesocket()` | `close()` |
| **Cleanup** | `WSACleanup()` | None |
| **Linking** | `-lws2_32` (Winsock2 library) | None (built-in libc) |
| **Option Types** | `BOOL` | `int` |
| **Threading** | Win32 threads | `pthread` (POSIX threads) |

---

## 🎯 Protocol Details

### Ethernet Frame Format

```
┌────────────────────────────────────────────┐
│         Ethernet TCP Payload               │
├────────────────────────────┬───────────────┤
│   Payload Data (8 bytes)   │  ID (2 bytes) │
└────────────────────────────┴───────────────┘
         BUS_LENGTH_RECEIVE       Little-endian

Total: 10 bytes per transmission
```

**Key Characteristics**:
- **Protocol**: TCP (SOCK_STREAM) over IPv4 (AF_INET)
- **Port**: Configurable via `PORT_NUMBER` constant
- **IP Address**: Read from `ip_address.txt` (default: 127.0.0.1 loopback)
- **Connection**: One TCP connection per message (not persistent)
- **Byte Order**: Little-endian for message ID
- **Reliability**: TCP ensures in-order delivery

### Configuration File

**File**: `source/Ethernet/ip_address.txt`

```
127.0.0.1
```

**Usage**:
- Read during `ethernet_init()`
- Sender connects to this IP
- Receiver binds to `INADDR_ANY` (0.0.0.0 - all interfaces)

---

## 🔄 AUTOSAR Integration

### PduR Routing

**File**: `ethernet_windows.c:275-333` (same logic in `ethernet.c`)

```c
void ethernet_RecieveMainFunction(void)
{
    static uint8 dataRecieve[BUS_LENGTH_RECEIVE];
    uint16 id;

    if (ethernet_receive(dataRecieve, BUS_LENGTH_RECEIVE, &id) != E_OK)
    {
        return;
    }

    PduInfoType PduInfoPtr = {
        .SduDataPtr = dataRecieve,
        .MetaDataPtr = (uint8*)&PdusCollections[id],
        .SduLength = BUS_LENGTH_RECEIVE,
    };

    switch (PdusCollections[id].Type)
    {
    case SECOC_SECURED_PDU_CANIF:
        PduR_CanIfRxIndication(id, &PduInfoPtr);
        break;
    case SECOC_SECURED_PDU_CANTP:
        CanTp_RxIndication(id, &PduInfoPtr);
        break;
    case SECOC_SECURED_PDU_SOADTP:
        SoAdTp_RxIndication(id, &PduInfoPtr);
        break;
    case SECOC_SECURED_PDU_SOADIF:
        PduR_SoAdIfRxIndication(id, &PduInfoPtr);
        break;
    case SECOC_AUTH_COLLECTON_PDU:
    case SECOC_CRYPTO_COLLECTON_PDU:
        PduR_CanIfRxIndication(id, &PduInfoPtr);
        break;
    default:
        printf("Unknown PDU type for ID: %d\n", id);
        break;
    }
}
```

**Routing Logic**:
1. Receive Ethernet frame with embedded ID
2. Lookup PDU type in `PdusCollections[]` table
3. Route to appropriate AUTOSAR layer:
   - **CANIF**: Direct CAN interface
   - **CANTP**: CAN Transport Protocol
   - **SOADIF**: Socket Adapter (Ethernet) IF mode
   - **SOADTP**: Socket Adapter (Ethernet) TP mode
   - **PDU Collection**: Split Auth/Crypto PDUs

---

## 🚀 Development Workflow

### On Windows (Development)

```bash
cd Autosar_SecOC
mkdir build && cd build

# Configure with MinGW or Visual Studio
cmake -G "MinGW Makefiles" ..
# or
cmake -G "Visual Studio 17 2022" ..

# Build
make  # or: cmake --build .

# Run
./SecOC.exe
```

**What Happens**:
- CMake detects `WIN32`
- Includes `ethernet_windows.c`
- Links `ws2_32.lib` (Winsock2)
- Defines `WINDOWS` preprocessor symbol
- Executable uses Windows sockets

### On Raspberry Pi (Deployment)

```bash
cd ~/Autosar_SecOC
mkdir build && cd build

# Configure for Unix
cmake -G "Unix Makefiles" ..

# Build
make

# Run
./SecOC
```

**What Happens**:
- CMake detects `UNIX`
- Includes `ethernet.c`
- No special linking (BSD sockets built-in)
- Defines `LINUX` preprocessor symbol
- Executable uses POSIX sockets

---

## 🔧 Advantages of Dual Implementation

### 1. **Development Efficiency**
- Developers can work on Windows (familiar environment)
- Full IDE support (Visual Studio, CLion, etc.)
- Faster compile times on powerful desktops
- Easy debugging with Windows tools

### 2. **Deployment Flexibility**
- Seamless transition to Raspberry Pi
- No code changes needed (abstracted by Ethernet layer)
- Same AUTOSAR logic on both platforms
- Production deployment on embedded Linux

### 3. **Testing Coverage**
- Test on Windows during development
- Validate on Raspberry Pi before production
- Both platforms ensure portability
- Catch platform-specific bugs early

### 4. **Cost-Effective**
- No need for multiple Raspberry Pi units for each developer
- Single codebase for both platforms
- CMake handles platform differences automatically

---

## 🆚 Comparison with PQC Integration

### Current State (Without PQC)

```
Application
    ↓
  COM Layer
    ↓
  SecOC (Classical MAC)
    ↓
  PduR Routing
    ↓
Ethernet (Winsock2/BSD)
    ↓
Network (TCP/IP)
```

### With PQC Integration (Your Implementation)

```
Application
    ↓
  COM Layer
    ↓
  SecOC (Classical MAC or PQC)
    ├→ Csm_MacGenerate() → Classical HMAC
    └→ Csm_SignatureGenerate() → PQC ML-DSA-65
    ↓
  PduR Routing
    ↓
Ethernet (Winsock2/BSD)
    ↓
Network (TCP/IP with 3.3KB signatures!)
```

**Challenge**:
- Classical MAC: 4-16 bytes → Fits in 10-byte Ethernet frame ✓
- PQC Signature: 3,309 bytes → **Doesn't fit!** ❌

**Solution Required**:
1. **Increase Frame Size**:
   ```c
   #define BUS_LENGTH_RECEIVE 4096  // Instead of 8
   ```

2. **Fragmentation** (Better approach):
   - Implement TP mode fragmentation in Ethernet layer
   - Split large PQC signatures across multiple TCP packets
   - Reassemble at receiver

3. **Use SOADTP Mode**:
   - Already supports Transport Protocol
   - Handles segmentation/reassembly
   - Modify `ethernet_send()` to support variable length

---

## 📝 Recommendations for PQC Integration

### Option 1: Increase Ethernet Frame Size (Quick Fix)

**Modify**: `include/Ethernet/ethernet.h` and `include/Ethernet/ethernet_windows.h`

```c
// OLD:
#define BUS_LENGTH_RECEIVE 8

// NEW:
#define BUS_LENGTH_RECEIVE 4096  // Support up to 4KB payloads

// In ethernet_send():
send(network_socket, (const char*)sendData, dataLen + sizeof(id), 0);
// Instead of hardcoded "10"
```

**Pros**:
- Simple to implement
- Works for PQC signatures (~3.3KB)

**Cons**:
- Wastes bandwidth for small messages
- Not aligned with AUTOSAR TP mode philosophy

### Option 2: Implement SOADTP Fragmentation (Proper Solution)

**Create**: `ethernet_tp.c` with fragmentation support

```c
typedef struct {
    uint16 totalLength;
    uint16 sequenceNumber;
    uint8 isLastFragment;
    uint8 payload[1024];  // Fragment size
} EthernetTPFrame;

Std_ReturnType ethernet_tp_send(uint16 id, uint8* data, uint16 dataLen)
{
    uint16 numFragments = (dataLen + 1023) / 1024;

    for (uint16 i = 0; i < numFragments; i++)
    {
        EthernetTPFrame frame;
        frame.totalLength = dataLen;
        frame.sequenceNumber = i;
        frame.isLastFragment = (i == numFragments - 1);

        uint16 fragmentSize = (frame.isLastFragment)
            ? (dataLen - i * 1024)
            : 1024;

        memcpy(frame.payload, data + i * 1024, fragmentSize);

        ethernet_send(id, (uint8*)&frame, sizeof(frame));
    }

    return E_OK;
}
```

**Pros**:
- Proper AUTOSAR TP mode implementation
- Efficient for all message sizes
- Aligned with automotive standards

**Cons**:
- More complex implementation
- Requires reassembly buffer on receiver

### Option 3: Hybrid Approach

**Use Cases**:
- **Small Messages (< 1KB)**: Classical MAC (4-16 bytes) → Direct Ethernet
- **Large Messages (> 1KB)**: PQC Signature (3.3KB) → Ethernet TP mode

**Implementation**:
```c
if (securedPduSize <= 1024)
{
    // Use SOADIF (direct)
    ethernet_send(id, securedPdu, securedPduSize);
}
else
{
    // Use SOADTP (fragmented)
    ethernet_tp_send(id, securedPdu, securedPduSize);
}
```

---

## 🎓 Summary

### Key Takeaways

1. **Dual Platform Support**:
   - Windows (Winsock2) for development
   - Linux/Raspberry Pi (BSD sockets) for deployment
   - CMake handles platform differences automatically

2. **Protocol**:
   - TCP/IP over Ethernet
   - Fixed 10-byte frames (8 bytes payload + 2 bytes ID)
   - Connection-oriented, reliable delivery

3. **AUTOSAR Integration**:
   - Ethernet layer routes to PduR based on PDU ID
   - Supports CANIF, CANTP, SOADIF, SOADTP modes
   - PDU Collection mode for split Auth/Crypto

4. **PQC Challenge**:
   - Current frame size (10 bytes) insufficient for PQC signatures (3.3KB)
   - **Solution**: Implement TP fragmentation or increase frame size

5. **Recommendation**:
   - **For presentation**: Increase `BUS_LENGTH_RECEIVE` to 4096 (quick demo)
   - **For production**: Implement proper SOADTP fragmentation (proper AUTOSAR)

---

## 📚 References

- **Winsock2 Documentation**: https://docs.microsoft.com/en-us/windows/win32/winsock/
- **BSD Sockets**: POSIX.1-2001, POSIX.1-2008 standards
- **AUTOSAR SWS SecOC**: R21-11 specification
- **AUTOSAR SWS SoAd**: Socket Adapter specification
- **TCP/IP Illustrated**: Stevens, W. Richard (classic reference)

---

**End of Analysis**
