/**
 * @file TcpIp.c
 * @brief AUTOSAR TcpIp Module - TCP/IP Stack Abstraction Implementation
 * @details Wraps platform sockets (Winsock2 / POSIX) with AUTOSAR-compliant API.
 *          Conforms to AUTOSAR CP R24-11 SWS_TcpIp.
 */

/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "Det/Det.h"
#include "EthSM/EthSM.h"
#include "SoAd/SoAd.h"
#include "TcpIp/TcpIp.h"
#if (TCPIP_PAYLOAD_BACKEND == TCPIP_PAYLOAD_BACKEND_ETHIF)
    #include "EthIf/EthIf.h"
#endif
#include <string.h>

#ifdef WINDOWS
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef SOCKET PlatformSocketType;
    #define TCPIP_PLATFORM_INVALID_SOCKET  INVALID_SOCKET
    #define TCPIP_PLATFORM_SOCKET_ERROR    SOCKET_ERROR
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <errno.h>
    typedef int PlatformSocketType;
    #define TCPIP_PLATFORM_INVALID_SOCKET  (-1)
    #define TCPIP_PLATFORM_SOCKET_ERROR    (-1)
#endif

/* Socket prototypes provided by ethernet.h (Rule 8.5 - no duplicate declarations) */
#if !defined(_FCNTL_H) && !defined(__FCNTL_H__)
extern int fcntl(int fd, int cmd, ...);
#endif

/* MISRA C:2012 Rule 8.4 - Forward declarations for external linkage functions */
extern void TcpIp_Init(const TcpIp_ConfigType* ConfigPtr);
extern void TcpIp_Shutdown(void);
extern Std_ReturnType TcpIp_GetSocketId(TcpIp_DomainType Domain, TcpIp_ProtocolType Protocol, TcpIp_SocketIdType* SocketIdPtr);
extern Std_ReturnType TcpIp_Close(TcpIp_SocketIdType SocketId, boolean Abort);
extern Std_ReturnType TcpIp_Bind(TcpIp_SocketIdType SocketId, TcpIp_LocalAddrIdType LocalAddrId, uint16* PortPtr);
extern Std_ReturnType TcpIp_TcpConnect(TcpIp_SocketIdType SocketId, const TcpIp_SockAddrType* RemoteAddrPtr);
extern Std_ReturnType TcpIp_TcpListen(TcpIp_SocketIdType SocketId, uint16 MaxChannels);
extern Std_ReturnType TcpIp_UdpTransmit(TcpIp_SocketIdType SocketId, const uint8* DataPtr, const TcpIp_SockAddrType* RemoteAddrPtr, uint16 TotalLength);
extern Std_ReturnType TcpIp_TcpTransmit(TcpIp_SocketIdType SocketId, const uint8* DataPtr, uint32 AvailableLength, boolean ForceRetrieve);
extern void TcpIp_RxIndication(TcpIp_SocketIdType SocketId, const TcpIp_SockAddrType* RemoteAddrPtr, const uint8* BufPtr, uint16 Length);
extern void TcpIp_MainFunction(void);
extern void TcpIp_GetVersionInfo(Std_VersionInfoType* VersionInfoPtr);
extern Std_ReturnType TcpIp_RequestIpAddrAssignment(TcpIp_LocalAddrIdType LocalAddrId, TcpIp_IpAddrAssignmentType Type, const TcpIp_SockAddrType* LocalIpAddrPtr);
extern Std_ReturnType TcpIp_GetIpAddr(TcpIp_LocalAddrIdType LocalAddrId, TcpIp_SockAddrType* IpAddrPtr, uint8* NetmaskPtr, TcpIp_SockAddrType* DefaultRouterPtr);

/********************************************************************************************************/
/************************************************Defines*************************************************/
/********************************************************************************************************/

#define TCPIP_RX_BUFFER_SIZE  (4096U)
#if (TCPIP_PAYLOAD_BACKEND == TCPIP_PAYLOAD_BACKEND_ETHIF)
#define TCPIP_ETHIF_CTRL_IDX        ((uint8)0U)
#define TCPIP_ETHIF_FRAME_TYPE_BASE ((Eth_FrameType)0x9100U)
#endif

/********************************************************************************************************/
/*******************************************TypeDefinitions**********************************************/
/********************************************************************************************************/

/** @brief Internal socket entry */
typedef struct
{
    PlatformSocketType     platformSocket;
    TcpIp_SocketStateType  state;
    TcpIp_DomainType       domain;
    TcpIp_ProtocolType     protocol;
    TcpIp_LocalAddrIdType  localAddrId;
    uint16                 localPort;
    TcpIp_SockAddrType     remoteAddr;
} TcpIp_SocketEntryType;

/** @brief Local address entry */
typedef struct
{
    TcpIp_SockAddrType         ipAddr;
    uint8                      netmask;
    TcpIp_SockAddrType         defaultRouter;
    boolean                    assigned;
    TcpIp_IpAddrAssignmentType assignmentType;
} TcpIp_LocalAddrEntryType;

/********************************************************************************************************/
/******************************************GlobalVariables**********************************************/
/********************************************************************************************************/

static TcpIp_StateType TcpIp_ModuleState = TCPIP_STATE_UNINIT;

static TcpIp_SocketEntryType TcpIp_SocketTable[TCPIP_MAX_SOCKETS];

static TcpIp_LocalAddrEntryType TcpIp_LocalAddrTable[TCPIP_MAX_LOCAL_ADDR];

#ifdef WINDOWS
static WSADATA TcpIp_WsaData;
static boolean TcpIp_WinsockInitialized = FALSE;
#endif

static uint8 TcpIp_RxBuffer[TCPIP_RX_BUFFER_SIZE];

/********************************************************************************************************/
/**************************************StaticFunctionPrototypes******************************************/
/********************************************************************************************************/

static void TcpIp_SockAddrToSockAddrIn(const TcpIp_SockAddrType* SrcAddr, struct sockaddr_in* DstAddr);
static void TcpIp_SockAddrInToSockAddr(const struct sockaddr_in* SrcAddr, TcpIp_SockAddrType* DstAddr);
static Std_ReturnType TcpIp_SetNonBlocking(PlatformSocketType Sock);
static void TcpIp_ClosePlatformSocket(PlatformSocketType Sock);
#if (TCPIP_PAYLOAD_BACKEND == TCPIP_PAYLOAD_BACKEND_ETHIF)
static void TcpIp_EthIfRxIndicationCbk(uint8 CtrlIdx,
                                       Eth_FrameType FrameType,
                                       boolean IsBroadcast,
                                       const uint8* PhysAddrPtr,
                                       const uint8* DataPtr,
                                       uint16 LenByte);
#endif

/********************************************************************************************************/
/****************************************StaticFunctions*************************************************/
/********************************************************************************************************/

static void TcpIp_SockAddrToSockAddrIn(const TcpIp_SockAddrType* SrcAddr, struct sockaddr_in* DstAddr)
{
    (void)memset(DstAddr, 0, sizeof(struct sockaddr_in));
    DstAddr->sin_family = AF_INET;
    DstAddr->sin_port   = htons(SrcAddr->port);
    (void)memcpy(&DstAddr->sin_addr.s_addr, SrcAddr->addr, 4U);
}

static void TcpIp_SockAddrInToSockAddr(const struct sockaddr_in* SrcAddr, TcpIp_SockAddrType* DstAddr)
{
    DstAddr->domain = TCPIP_AF_INET;
    DstAddr->port   = ntohs(SrcAddr->sin_port);
    (void)memcpy(DstAddr->addr, &SrcAddr->sin_addr.s_addr, 4U);
}

static Std_ReturnType TcpIp_SetNonBlocking(PlatformSocketType Sock)
{
#ifdef WINDOWS
    u_long mode = 1U;
    if (ioctlsocket(Sock, (long)FIONBIO, &mode) != 0)
    {
        return E_NOT_OK;
    }
#else
    int flags = fcntl(Sock, F_GETFL, 0);
    if (flags == -1)
    {
        return E_NOT_OK;
    }
    if (fcntl(Sock, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        return E_NOT_OK;
    }
#endif
    return E_OK;
}

static void TcpIp_ClosePlatformSocket(PlatformSocketType Sock)
{
#ifdef WINDOWS
    (void)closesocket(Sock);
#else
    (void)close(Sock);
#endif
}

#if (TCPIP_PAYLOAD_BACKEND == TCPIP_PAYLOAD_BACKEND_ETHIF)
static void TcpIp_EthIfRxIndicationCbk(uint8 CtrlIdx,
                                       Eth_FrameType FrameType,
                                       boolean IsBroadcast,
                                       const uint8* PhysAddrPtr,
                                       const uint8* DataPtr,
                                       uint16 LenByte)
{
    TcpIp_SocketIdType SocketId;
    TcpIp_SockAddrType RemoteAddr;
    (void)CtrlIdx;
    (void)IsBroadcast;
    (void)PhysAddrPtr;

    if ((DataPtr == NULL) || (LenByte == 0U))
    {
        return;
    }
    if ((FrameType < TCPIP_ETHIF_FRAME_TYPE_BASE) ||
        (FrameType >= (Eth_FrameType)(TCPIP_ETHIF_FRAME_TYPE_BASE + TCPIP_MAX_SOCKETS)))
    {
        return;
    }

    SocketId = (TcpIp_SocketIdType)(FrameType - TCPIP_ETHIF_FRAME_TYPE_BASE);
    if ((SocketId >= TCPIP_MAX_SOCKETS) ||
        (TcpIp_SocketTable[SocketId].state == TCPIP_SOCKET_STATE_UNUSED))
    {
        return;
    }

    /* EthIf backend has no IP peer metadata; preserve latest socket peer snapshot. */
    RemoteAddr = TcpIp_SocketTable[SocketId].remoteAddr;
    TcpIp_RxIndication(SocketId, &RemoteAddr, DataPtr, LenByte);
}
#endif

/********************************************************************************************************/
/********************************************Functions***************************************************/
/********************************************************************************************************/

void TcpIp_Init(const TcpIp_ConfigType* ConfigPtr)
{
    uint16 idx;

    (void)ConfigPtr;

#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (TcpIp_ModuleState == TCPIP_STATE_ONLINE)
    {
        (void)Det_ReportError(TCPIP_MODULE_ID, 0U, TCPIP_SID_INIT, TCPIP_E_ALREADY_INIT);
        return;
    }
#endif

#ifdef WINDOWS
    if (TcpIp_WinsockInitialized == FALSE)
    {
        if (WSAStartup(MAKEWORD(2, 2), &TcpIp_WsaData) != 0)
        {
            return;
        }
        TcpIp_WinsockInitialized = TRUE;
    }
#endif

#if (TCPIP_PAYLOAD_BACKEND == TCPIP_PAYLOAD_BACKEND_ETHIF)
    EthIf_SetRxIndicationCallback(TcpIp_EthIfRxIndicationCbk);
#endif

    /* Initialize socket table */
    for (idx = 0U; idx < TCPIP_MAX_SOCKETS; idx++)
    {
        TcpIp_SocketTable[idx].platformSocket = TCPIP_PLATFORM_INVALID_SOCKET;
        TcpIp_SocketTable[idx].state          = TCPIP_SOCKET_STATE_UNUSED;
        TcpIp_SocketTable[idx].domain         = TCPIP_AF_INET;
        TcpIp_SocketTable[idx].protocol       = TCPIP_IPPROTO_TCP;
        TcpIp_SocketTable[idx].localAddrId    = 0U;
        TcpIp_SocketTable[idx].localPort      = 0U;
        (void)memset(&TcpIp_SocketTable[idx].remoteAddr, 0, sizeof(TcpIp_SockAddrType));
    }

    /* Initialize local address table */
    for (idx = 0U; idx < TCPIP_MAX_LOCAL_ADDR; idx++)
    {
        (void)memset(&TcpIp_LocalAddrTable[idx], 0, sizeof(TcpIp_LocalAddrEntryType));
        TcpIp_LocalAddrTable[idx].assigned = FALSE;
    }

    /* Set default local address 0 to INADDR_ANY */
    TcpIp_LocalAddrTable[0].ipAddr.domain = TCPIP_AF_INET;
    (void)memset(TcpIp_LocalAddrTable[0].ipAddr.addr, 0, 4U);
    TcpIp_LocalAddrTable[0].ipAddr.port   = 0U;
    TcpIp_LocalAddrTable[0].netmask       = 0U;
    TcpIp_LocalAddrTable[0].assigned      = TRUE;
    TcpIp_LocalAddrTable[0].assignmentType = TCPIP_IPADDR_ASSIGNMENT_STATIC;

    TcpIp_ModuleState = TCPIP_STATE_ONLINE;
}

void TcpIp_Shutdown(void)
{
    uint16 idx;

#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (TcpIp_ModuleState != TCPIP_STATE_ONLINE)
    {
        (void)Det_ReportError(TCPIP_MODULE_ID, 0U, TCPIP_SID_SHUTDOWN, TCPIP_E_NOTINIT);
        return;
    }
#endif

    /* Close all open sockets */
    for (idx = 0U; idx < TCPIP_MAX_SOCKETS; idx++)
    {
        if (TcpIp_SocketTable[idx].state != TCPIP_SOCKET_STATE_UNUSED)
        {
            if (TcpIp_SocketTable[idx].platformSocket != TCPIP_PLATFORM_INVALID_SOCKET)
            {
                TcpIp_ClosePlatformSocket(TcpIp_SocketTable[idx].platformSocket);
            }
            TcpIp_SocketTable[idx].platformSocket = TCPIP_PLATFORM_INVALID_SOCKET;
            TcpIp_SocketTable[idx].state          = TCPIP_SOCKET_STATE_UNUSED;
        }
    }

#ifdef WINDOWS
    if (TcpIp_WinsockInitialized == TRUE)
    {
        WSACleanup();
        TcpIp_WinsockInitialized = FALSE;
    }
#endif

#if (TCPIP_PAYLOAD_BACKEND == TCPIP_PAYLOAD_BACKEND_ETHIF)
    EthIf_SetRxIndicationCallback(NULL);
#endif

    TcpIp_ModuleState = TCPIP_STATE_OFFLINE;
    EthSM_TcpIpModeIndication(0U, TCPIP_STATE_OFFLINE);
}

Std_ReturnType TcpIp_GetSocketId(
    TcpIp_DomainType Domain,
    TcpIp_ProtocolType Protocol,
    TcpIp_SocketIdType* SocketIdPtr
)
{
    uint16 idx;
#if (TCPIP_PAYLOAD_BACKEND == TCPIP_PAYLOAD_BACKEND_SOCKETS)
    int sockType;
    int sockProto;
#endif
    PlatformSocketType newSock;

#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (TcpIp_ModuleState != TCPIP_STATE_ONLINE)
    {
        (void)Det_ReportError(TCPIP_MODULE_ID, 0U, TCPIP_SID_GET_SOCKET, TCPIP_E_NOTINIT);
        return E_NOT_OK;
    }
    if (SocketIdPtr == NULL)
    {
        (void)Det_ReportError(TCPIP_MODULE_ID, 0U, TCPIP_SID_GET_SOCKET, TCPIP_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    if ((Domain != TCPIP_AF_INET) && (Domain != TCPIP_AF_INET6))
    {
        (void)Det_ReportError(TCPIP_MODULE_ID, 0U, TCPIP_SID_GET_SOCKET, TCPIP_E_INV_ARG);
        return E_NOT_OK;
    }
    if ((Protocol != TCPIP_IPPROTO_TCP) && (Protocol != TCPIP_IPPROTO_UDP))
    {
        (void)Det_ReportError(TCPIP_MODULE_ID, 0U, TCPIP_SID_GET_SOCKET, TCPIP_E_INV_ARG);
        return E_NOT_OK;
    }
#endif

    /* Find free slot */
    for (idx = 0U; idx < TCPIP_MAX_SOCKETS; idx++)
    {
        if (TcpIp_SocketTable[idx].state == TCPIP_SOCKET_STATE_UNUSED)
        {
            break;
        }
    }

    if (idx >= TCPIP_MAX_SOCKETS)
    {
#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(TCPIP_MODULE_ID, 0U, TCPIP_SID_GET_SOCKET, TCPIP_E_NORESOURCE);
#endif
        return E_NOT_OK;
    }

    /* Map protocol to platform socket type */
#if (TCPIP_PAYLOAD_BACKEND == TCPIP_PAYLOAD_BACKEND_SOCKETS)
    if (Protocol == TCPIP_IPPROTO_TCP)
    {
        sockType  = SOCK_STREAM;
        sockProto = IPPROTO_TCP;
    }
    else
    {
        sockType  = SOCK_DGRAM;
        sockProto = IPPROTO_UDP;
    }
#endif

    newSock = TCPIP_PLATFORM_INVALID_SOCKET;
#if (TCPIP_PAYLOAD_BACKEND == TCPIP_PAYLOAD_BACKEND_SOCKETS)
    newSock = socket(AF_INET, sockType, sockProto);
    if (newSock == TCPIP_PLATFORM_INVALID_SOCKET)
    {
        return E_NOT_OK;
    }
#endif

    TcpIp_SocketTable[idx].platformSocket = newSock;
    TcpIp_SocketTable[idx].state          = TCPIP_SOCKET_STATE_ALLOCATED;
    TcpIp_SocketTable[idx].domain         = Domain;
    TcpIp_SocketTable[idx].protocol       = Protocol;
    TcpIp_SocketTable[idx].localPort      = 0U;
    (void)memset(&TcpIp_SocketTable[idx].remoteAddr, 0, sizeof(TcpIp_SockAddrType));

    *SocketIdPtr = (TcpIp_SocketIdType)idx;
    return E_OK;
}

Std_ReturnType TcpIp_Close(TcpIp_SocketIdType SocketId, boolean Abort)
{
    (void)Abort;

#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (TcpIp_ModuleState != TCPIP_STATE_ONLINE)
    {
        (void)Det_ReportError(TCPIP_MODULE_ID, 0U, TCPIP_SID_CLOSE, TCPIP_E_NOTINIT);
        return E_NOT_OK;
    }
    if (SocketId >= TCPIP_MAX_SOCKETS)
    {
        (void)Det_ReportError(TCPIP_MODULE_ID, 0U, TCPIP_SID_CLOSE, TCPIP_E_INV_SOCKETID);
        return E_NOT_OK;
    }
    if (TcpIp_SocketTable[SocketId].state == TCPIP_SOCKET_STATE_UNUSED)
    {
        (void)Det_ReportError(TCPIP_MODULE_ID, 0U, TCPIP_SID_CLOSE, TCPIP_E_INV_SOCKETID);
        return E_NOT_OK;
    }
#endif

    if (Abort == TRUE)
    {
#if (TCPIP_PAYLOAD_BACKEND == TCPIP_PAYLOAD_BACKEND_SOCKETS)
        /* Set SO_LINGER with timeout 0 for immediate RST */
        struct linger lingerOpt;
        lingerOpt.l_onoff  = 1;
        lingerOpt.l_linger = 0;
        (void)setsockopt(
            TcpIp_SocketTable[SocketId].platformSocket,
            SOL_SOCKET, SO_LINGER,
            (const char*)&lingerOpt, sizeof(lingerOpt)
        );
#endif
    }

    if (TcpIp_SocketTable[SocketId].platformSocket != TCPIP_PLATFORM_INVALID_SOCKET)
    {
        TcpIp_ClosePlatformSocket(TcpIp_SocketTable[SocketId].platformSocket);
    }
    TcpIp_SocketTable[SocketId].platformSocket = TCPIP_PLATFORM_INVALID_SOCKET;
    TcpIp_SocketTable[SocketId].state          = TCPIP_SOCKET_STATE_UNUSED;
    TcpIp_SocketTable[SocketId].localPort      = 0U;
    (void)memset(&TcpIp_SocketTable[SocketId].remoteAddr, 0, sizeof(TcpIp_SockAddrType));

    return E_OK;
}

Std_ReturnType TcpIp_Bind(
    TcpIp_SocketIdType SocketId,
    TcpIp_LocalAddrIdType LocalAddrId,
    uint16* PortPtr
)
{
#if (TCPIP_PAYLOAD_BACKEND == TCPIP_PAYLOAD_BACKEND_SOCKETS)
    struct sockaddr_in bindAddr;
    int optVal = 1;
#endif

#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (TcpIp_ModuleState != TCPIP_STATE_ONLINE)
    {
        (void)Det_ReportError(TCPIP_MODULE_ID, 0U, TCPIP_SID_BIND, TCPIP_E_NOTINIT);
        return E_NOT_OK;
    }
    if (SocketId >= TCPIP_MAX_SOCKETS)
    {
        (void)Det_ReportError(TCPIP_MODULE_ID, 0U, TCPIP_SID_BIND, TCPIP_E_INV_SOCKETID);
        return E_NOT_OK;
    }
    if (TcpIp_SocketTable[SocketId].state == TCPIP_SOCKET_STATE_UNUSED)
    {
        (void)Det_ReportError(TCPIP_MODULE_ID, 0U, TCPIP_SID_BIND, TCPIP_E_INV_SOCKETID);
        return E_NOT_OK;
    }
    if (PortPtr == NULL)
    {
        (void)Det_ReportError(TCPIP_MODULE_ID, 0U, TCPIP_SID_BIND, TCPIP_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    if (LocalAddrId >= TCPIP_MAX_LOCAL_ADDR)
    {
        (void)Det_ReportError(TCPIP_MODULE_ID, 0U, TCPIP_SID_BIND, TCPIP_E_INV_ARG);
        return E_NOT_OK;
    }
#endif

    /* Set SO_REUSEADDR */
#if (TCPIP_PAYLOAD_BACKEND == TCPIP_PAYLOAD_BACKEND_SOCKETS)
    (void)setsockopt(
        TcpIp_SocketTable[SocketId].platformSocket,
        SOL_SOCKET, SO_REUSEADDR,
        (const char*)&optVal, sizeof(optVal)
    );

    /* Build bind address from local address table */
    (void)memset(&bindAddr, 0, sizeof(bindAddr));
    bindAddr.sin_family = AF_INET;
    bindAddr.sin_port   = htons(*PortPtr);

    if (TcpIp_LocalAddrTable[LocalAddrId].assigned == TRUE)
    {
        (void)memcpy(&bindAddr.sin_addr.s_addr, TcpIp_LocalAddrTable[LocalAddrId].ipAddr.addr, 4U);
    }
    else
    {
        bindAddr.sin_addr.s_addr = INADDR_ANY;
    }

    /* cppcheck-suppress misra-c2012-17.3 */
    if (bind(TcpIp_SocketTable[SocketId].platformSocket,
             (struct sockaddr*)&bindAddr, sizeof(bindAddr)) == TCPIP_PLATFORM_SOCKET_ERROR)
    {
        return E_NOT_OK;
    }

    /* If port was 0 (auto-assign), retrieve the assigned port */
    if (*PortPtr == 0U)
    {
        struct sockaddr_in assignedAddr;
#ifdef WINDOWS
        int addrLen = (int)sizeof(assignedAddr);
#else
        socklen_t addrLen = sizeof(assignedAddr);
#endif
        /* cppcheck-suppress misra-c2012-17.3 */
        if (getsockname(TcpIp_SocketTable[SocketId].platformSocket,
                        (struct sockaddr*)&assignedAddr, &addrLen) == 0)
        {
            *PortPtr = ntohs(assignedAddr.sin_port);
        }
    }
#endif

    TcpIp_SocketTable[SocketId].localAddrId = LocalAddrId;
    TcpIp_SocketTable[SocketId].localPort   = *PortPtr;
    TcpIp_SocketTable[SocketId].state       = TCPIP_SOCKET_STATE_BOUND;

    return E_OK;
}

Std_ReturnType TcpIp_TcpConnect(
    TcpIp_SocketIdType SocketId,
    const TcpIp_SockAddrType* RemoteAddrPtr
)
{
#if (TCPIP_PAYLOAD_BACKEND == TCPIP_PAYLOAD_BACKEND_SOCKETS)
    struct sockaddr_in remoteAddr;
#endif

#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (TcpIp_ModuleState != TCPIP_STATE_ONLINE)
    {
        (void)Det_ReportError(TCPIP_MODULE_ID, 0U, TCPIP_SID_TCP_CONNECT, TCPIP_E_NOTINIT);
        return E_NOT_OK;
    }
    if (SocketId >= TCPIP_MAX_SOCKETS)
    {
        (void)Det_ReportError(TCPIP_MODULE_ID, 0U, TCPIP_SID_TCP_CONNECT, TCPIP_E_INV_SOCKETID);
        return E_NOT_OK;
    }
    if (TcpIp_SocketTable[SocketId].state == TCPIP_SOCKET_STATE_UNUSED)
    {
        (void)Det_ReportError(TCPIP_MODULE_ID, 0U, TCPIP_SID_TCP_CONNECT, TCPIP_E_INV_SOCKETID);
        return E_NOT_OK;
    }
    if (RemoteAddrPtr == NULL)
    {
        (void)Det_ReportError(TCPIP_MODULE_ID, 0U, TCPIP_SID_TCP_CONNECT, TCPIP_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    if (TcpIp_SocketTable[SocketId].protocol != TCPIP_IPPROTO_TCP)
    {
        (void)Det_ReportError(TCPIP_MODULE_ID, 0U, TCPIP_SID_TCP_CONNECT, TCPIP_E_INV_ARG);
        return E_NOT_OK;
    }
#endif

#if (TCPIP_PAYLOAD_BACKEND == TCPIP_PAYLOAD_BACKEND_SOCKETS)
    TcpIp_SockAddrToSockAddrIn(RemoteAddrPtr, &remoteAddr);
    /* cppcheck-suppress misra-c2012-17.3 */
    if (connect(TcpIp_SocketTable[SocketId].platformSocket,
                (struct sockaddr*)&remoteAddr, sizeof(remoteAddr)) == TCPIP_PLATFORM_SOCKET_ERROR)
    {
        return E_NOT_OK;
    }
#endif

    TcpIp_SocketTable[SocketId].remoteAddr = *RemoteAddrPtr;
    TcpIp_SocketTable[SocketId].state      = TCPIP_SOCKET_STATE_CONNECTED;

    return E_OK;
}

Std_ReturnType TcpIp_TcpListen(TcpIp_SocketIdType SocketId, uint16 MaxChannels)
{
#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (TcpIp_ModuleState != TCPIP_STATE_ONLINE)
    {
        (void)Det_ReportError(TCPIP_MODULE_ID, 0U, TCPIP_SID_TCP_LISTEN, TCPIP_E_NOTINIT);
        return E_NOT_OK;
    }
    if (SocketId >= TCPIP_MAX_SOCKETS)
    {
        (void)Det_ReportError(TCPIP_MODULE_ID, 0U, TCPIP_SID_TCP_LISTEN, TCPIP_E_INV_SOCKETID);
        return E_NOT_OK;
    }
    if (TcpIp_SocketTable[SocketId].state == TCPIP_SOCKET_STATE_UNUSED)
    {
        (void)Det_ReportError(TCPIP_MODULE_ID, 0U, TCPIP_SID_TCP_LISTEN, TCPIP_E_INV_SOCKETID);
        return E_NOT_OK;
    }
    if (TcpIp_SocketTable[SocketId].protocol != TCPIP_IPPROTO_TCP)
    {
        (void)Det_ReportError(TCPIP_MODULE_ID, 0U, TCPIP_SID_TCP_LISTEN, TCPIP_E_INV_ARG);
        return E_NOT_OK;
    }
#endif

#if (TCPIP_PAYLOAD_BACKEND == TCPIP_PAYLOAD_BACKEND_SOCKETS)
    /* cppcheck-suppress misra-c2012-17.3 */
    if (listen(TcpIp_SocketTable[SocketId].platformSocket, (int)MaxChannels) == TCPIP_PLATFORM_SOCKET_ERROR)
    {
        return E_NOT_OK;
    }

    /* Set non-blocking so MainFunction can poll */
    (void)TcpIp_SetNonBlocking(TcpIp_SocketTable[SocketId].platformSocket);
#else
    (void)MaxChannels;
#endif

    TcpIp_SocketTable[SocketId].state = TCPIP_SOCKET_STATE_LISTENING;

    return E_OK;
}

Std_ReturnType TcpIp_UdpTransmit(
    TcpIp_SocketIdType SocketId,
    const uint8* DataPtr,
    const TcpIp_SockAddrType* RemoteAddrPtr,
    uint16 TotalLength
)
{
#if (TCPIP_PAYLOAD_BACKEND == TCPIP_PAYLOAD_BACKEND_SOCKETS)
    struct sockaddr_in destAddr;
    int sendResult;
#endif
#if (TCPIP_PAYLOAD_BACKEND == TCPIP_PAYLOAD_BACKEND_ETHIF)
    Eth_BufIdxType BufIdx;
    uint8* BufPtr;
    uint16 RequestedLength;
    Eth_FrameType FrameType;
#endif

#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (TcpIp_ModuleState != TCPIP_STATE_ONLINE)
    {
        (void)Det_ReportError(TCPIP_MODULE_ID, 0U, TCPIP_SID_UDP_TRANSMIT, TCPIP_E_NOTINIT);
        return E_NOT_OK;
    }
    if (SocketId >= TCPIP_MAX_SOCKETS)
    {
        (void)Det_ReportError(TCPIP_MODULE_ID, 0U, TCPIP_SID_UDP_TRANSMIT, TCPIP_E_INV_SOCKETID);
        return E_NOT_OK;
    }
    if (TcpIp_SocketTable[SocketId].state == TCPIP_SOCKET_STATE_UNUSED)
    {
        (void)Det_ReportError(TCPIP_MODULE_ID, 0U, TCPIP_SID_UDP_TRANSMIT, TCPIP_E_INV_SOCKETID);
        return E_NOT_OK;
    }
    if (DataPtr == NULL)
    {
        (void)Det_ReportError(TCPIP_MODULE_ID, 0U, TCPIP_SID_UDP_TRANSMIT, TCPIP_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    if (RemoteAddrPtr == NULL)
    {
        (void)Det_ReportError(TCPIP_MODULE_ID, 0U, TCPIP_SID_UDP_TRANSMIT, TCPIP_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    if (TcpIp_SocketTable[SocketId].protocol != TCPIP_IPPROTO_UDP)
    {
        (void)Det_ReportError(TCPIP_MODULE_ID, 0U, TCPIP_SID_UDP_TRANSMIT, TCPIP_E_INV_ARG);
        return E_NOT_OK;
    }
#endif

    TcpIp_SocketTable[SocketId].remoteAddr = *RemoteAddrPtr;
#if (TCPIP_PAYLOAD_BACKEND == TCPIP_PAYLOAD_BACKEND_ETHIF)
    RequestedLength = TotalLength;
    FrameType = (Eth_FrameType)(TCPIP_ETHIF_FRAME_TYPE_BASE + (Eth_FrameType)SocketId);
    if (EthIf_ProvideTxBuffer(TCPIP_ETHIF_CTRL_IDX,
                              FrameType,
                              0U,
                              &BufIdx,
                              &BufPtr,
                              &RequestedLength) != BUFREQ_OK)
    {
        return E_NOT_OK;
    }
    if ((BufPtr == NULL) || (RequestedLength < TotalLength))
    {
        return E_NOT_OK;
    }
    (void)memcpy(BufPtr, DataPtr, TotalLength);
    if (EthIf_Transmit(TCPIP_ETHIF_CTRL_IDX, BufIdx, FrameType, FALSE, TotalLength, NULL) != E_OK)
    {
        return E_NOT_OK;
    }
#else
    TcpIp_SockAddrToSockAddrIn(RemoteAddrPtr, &destAddr);
    sendResult = sendto(
        TcpIp_SocketTable[SocketId].platformSocket,
        (const char*)DataPtr,
        (int)TotalLength,
        0,
        (struct sockaddr*)&destAddr,
        sizeof(destAddr)
    );

    if (sendResult == TCPIP_PLATFORM_SOCKET_ERROR)
    {
        return E_NOT_OK;
    }
#endif

    return E_OK;
}

Std_ReturnType TcpIp_TcpTransmit(
    TcpIp_SocketIdType SocketId,
    const uint8* DataPtr,
    uint32 AvailableLength,
    boolean ForceRetrieve
)
{
#if (TCPIP_PAYLOAD_BACKEND == TCPIP_PAYLOAD_BACKEND_SOCKETS)
    int sendResult;
#endif
#if (TCPIP_PAYLOAD_BACKEND == TCPIP_PAYLOAD_BACKEND_ETHIF)
    Eth_BufIdxType BufIdx;
    uint8* BufPtr;
    uint16 RequestedLength;
    Eth_FrameType FrameType;
#endif

    (void)ForceRetrieve;

#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (TcpIp_ModuleState != TCPIP_STATE_ONLINE)
    {
        (void)Det_ReportError(TCPIP_MODULE_ID, 0U, TCPIP_SID_TCP_TRANSMIT, TCPIP_E_NOTINIT);
        return E_NOT_OK;
    }
    if (SocketId >= TCPIP_MAX_SOCKETS)
    {
        (void)Det_ReportError(TCPIP_MODULE_ID, 0U, TCPIP_SID_TCP_TRANSMIT, TCPIP_E_INV_SOCKETID);
        return E_NOT_OK;
    }
    if (TcpIp_SocketTable[SocketId].state == TCPIP_SOCKET_STATE_UNUSED)
    {
        (void)Det_ReportError(TCPIP_MODULE_ID, 0U, TCPIP_SID_TCP_TRANSMIT, TCPIP_E_INV_SOCKETID);
        return E_NOT_OK;
    }
    if (DataPtr == NULL)
    {
        (void)Det_ReportError(TCPIP_MODULE_ID, 0U, TCPIP_SID_TCP_TRANSMIT, TCPIP_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    if (TcpIp_SocketTable[SocketId].protocol != TCPIP_IPPROTO_TCP)
    {
        (void)Det_ReportError(TCPIP_MODULE_ID, 0U, TCPIP_SID_TCP_TRANSMIT, TCPIP_E_INV_ARG);
        return E_NOT_OK;
    }
    if (TcpIp_SocketTable[SocketId].state != TCPIP_SOCKET_STATE_CONNECTED)
    {
        (void)Det_ReportError(TCPIP_MODULE_ID, 0U, TCPIP_SID_TCP_TRANSMIT, TCPIP_E_INV_ARG);
        return E_NOT_OK;
    }
#endif

#if (TCPIP_PAYLOAD_BACKEND == TCPIP_PAYLOAD_BACKEND_ETHIF)
    if (AvailableLength > 0xFFFFUL)
    {
        return E_NOT_OK;
    }
    RequestedLength = (uint16)AvailableLength;
    FrameType = (Eth_FrameType)(TCPIP_ETHIF_FRAME_TYPE_BASE + (Eth_FrameType)SocketId);
    if (EthIf_ProvideTxBuffer(TCPIP_ETHIF_CTRL_IDX,
                              FrameType,
                              0U,
                              &BufIdx,
                              &BufPtr,
                              &RequestedLength) != BUFREQ_OK)
    {
        return E_NOT_OK;
    }
    if ((BufPtr == NULL) || (RequestedLength < (uint16)AvailableLength))
    {
        return E_NOT_OK;
    }
    (void)memcpy(BufPtr, DataPtr, (uint16)AvailableLength);
    if (EthIf_Transmit(TCPIP_ETHIF_CTRL_IDX,
                       BufIdx,
                       FrameType,
                       FALSE,
                       (uint16)AvailableLength,
                       NULL) != E_OK)
    {
        return E_NOT_OK;
    }
#else
    sendResult = send(
        TcpIp_SocketTable[SocketId].platformSocket,
        (const char*)DataPtr,
        (int)AvailableLength,
        0
    );

    if (sendResult == TCPIP_PLATFORM_SOCKET_ERROR)
    {
        return E_NOT_OK;
    }
#endif

    return E_OK;
}

void TcpIp_RxIndication(
    TcpIp_SocketIdType SocketId,
    const TcpIp_SockAddrType* RemoteAddrPtr,
    const uint8* BufPtr,
    uint16 Length
)
{
#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (TcpIp_ModuleState != TCPIP_STATE_ONLINE)
    {
        (void)Det_ReportError(TCPIP_MODULE_ID, 0U, TCPIP_SID_RX_INDICATION, TCPIP_E_NOTINIT);
        return;
    }
    if (SocketId >= TCPIP_MAX_SOCKETS)
    {
        (void)Det_ReportError(TCPIP_MODULE_ID, 0U, TCPIP_SID_RX_INDICATION, TCPIP_E_INV_SOCKETID);
        return;
    }
    if ((RemoteAddrPtr == NULL) || (BufPtr == NULL))
    {
        (void)Det_ReportError(TCPIP_MODULE_ID, 0U, TCPIP_SID_RX_INDICATION, TCPIP_E_PARAM_POINTER);
        return;
    }
#endif

    SoAd_RxIndication(SocketId, RemoteAddrPtr, BufPtr, Length);
}

void TcpIp_MainFunction(void)
{
#if (TCPIP_PAYLOAD_BACKEND == TCPIP_PAYLOAD_BACKEND_ETHIF)
#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (TcpIp_ModuleState != TCPIP_STATE_ONLINE)
    {
        return;
    }
#endif

    EthIf_MainFunctionRx();
    EthIf_MainFunctionTx();
    return;
#else
    uint16 idx;
    int recvLen;

#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (TcpIp_ModuleState != TCPIP_STATE_ONLINE)
    {
        return;
    }
#endif

    for (idx = 0U; idx < TCPIP_MAX_SOCKETS; idx++)
    {
        if (TcpIp_SocketTable[idx].state == TCPIP_SOCKET_STATE_UNUSED)
        {
            continue;
        }

        /* Handle listening TCP sockets - accept new connections */
        if (TcpIp_SocketTable[idx].state == TCPIP_SOCKET_STATE_LISTENING)
        {
            struct sockaddr_in clientAddr;
#ifdef WINDOWS
            int addrLen = (int)sizeof(clientAddr);
#else
            socklen_t addrLen = sizeof(clientAddr);
#endif
            PlatformSocketType clientSock;
            uint16 freeSlot;

            clientSock = accept(
                TcpIp_SocketTable[idx].platformSocket,
                (struct sockaddr*)&clientAddr, &addrLen
            );

            if (clientSock != TCPIP_PLATFORM_INVALID_SOCKET)
            {
                /* Find a free slot for the accepted connection */
                for (freeSlot = 0U; freeSlot < TCPIP_MAX_SOCKETS; freeSlot++)
                {
                    if (TcpIp_SocketTable[freeSlot].state == TCPIP_SOCKET_STATE_UNUSED)
                    {
                        break;
                    }
                }

                if (freeSlot < TCPIP_MAX_SOCKETS)
                {
                    TcpIp_SocketTable[freeSlot].platformSocket = clientSock;
                    TcpIp_SocketTable[freeSlot].state          = TCPIP_SOCKET_STATE_CONNECTED;
                    TcpIp_SocketTable[freeSlot].domain         = TcpIp_SocketTable[idx].domain;
                    TcpIp_SocketTable[freeSlot].protocol       = TCPIP_IPPROTO_TCP;
                    TcpIp_SocketTable[freeSlot].localAddrId    = TcpIp_SocketTable[idx].localAddrId;
                    TcpIp_SocketTable[freeSlot].localPort      = TcpIp_SocketTable[idx].localPort;
                    TcpIp_SockAddrInToSockAddr(&clientAddr, &TcpIp_SocketTable[freeSlot].remoteAddr);

                    /* Set accepted socket non-blocking for polling */
                    (void)TcpIp_SetNonBlocking(clientSock);
                }
                else
                {
                    /* No free slot, reject connection */
                    TcpIp_ClosePlatformSocket(clientSock);
                }
            }
        }

        /* Poll connected TCP sockets and bound UDP sockets for incoming data */
        if ((TcpIp_SocketTable[idx].state == TCPIP_SOCKET_STATE_CONNECTED) ||
            ((TcpIp_SocketTable[idx].state == TCPIP_SOCKET_STATE_BOUND) &&
             (TcpIp_SocketTable[idx].protocol == TCPIP_IPPROTO_UDP)))
        {
            if (TcpIp_SocketTable[idx].protocol == TCPIP_IPPROTO_UDP)
            {
                struct sockaddr_in srcAddr;
#ifdef WINDOWS
                int addrLen = (int)sizeof(srcAddr);
#else
                socklen_t addrLen = sizeof(srcAddr);
#endif
                recvLen = recvfrom(
                    TcpIp_SocketTable[idx].platformSocket,
                    (char*)TcpIp_RxBuffer,
                    (int)TCPIP_RX_BUFFER_SIZE,
                    0,
                    (struct sockaddr*)&srcAddr,
                    &addrLen
                );

                if (recvLen > 0)
                {
                    TcpIp_SockAddrType remoteSockAddr;
                    TcpIp_SockAddrInToSockAddr(&srcAddr, &remoteSockAddr);
                    TcpIp_RxIndication((TcpIp_SocketIdType)idx, &remoteSockAddr, TcpIp_RxBuffer, (uint16)recvLen);
                }
            }
            else
            {
                recvLen = recv(
                    TcpIp_SocketTable[idx].platformSocket,
                    (char*)TcpIp_RxBuffer,
                    (int)TCPIP_RX_BUFFER_SIZE,
                    0
                );

                if (recvLen > 0)
                {
                    TcpIp_RxIndication(
                        (TcpIp_SocketIdType)idx,
                        &TcpIp_SocketTable[idx].remoteAddr,
                        TcpIp_RxBuffer,
                        (uint16)recvLen
                    );
                }
                else if (recvLen == 0)
                {
                    /* Connection closed by peer */
                    TcpIp_ClosePlatformSocket(TcpIp_SocketTable[idx].platformSocket);
                    TcpIp_SocketTable[idx].platformSocket = TCPIP_PLATFORM_INVALID_SOCKET;
                    TcpIp_SocketTable[idx].state          = TCPIP_SOCKET_STATE_UNUSED;
                }
                else
                {
                    /* No action required */
                }
            }
        }
    }
#endif
}

void TcpIp_GetVersionInfo(Std_VersionInfoType* VersionInfoPtr)
{
#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (VersionInfoPtr == NULL)
    {
        (void)Det_ReportError(TCPIP_MODULE_ID, 0U, TCPIP_SID_GET_VERSION_INFO, TCPIP_E_PARAM_POINTER);
        return;
    }
#endif

    VersionInfoPtr->vendorID         = TCPIP_VENDOR_ID;
    VersionInfoPtr->moduleID         = TCPIP_MODULE_ID;
    VersionInfoPtr->sw_major_version = TCPIP_SW_MAJOR_VERSION;
    VersionInfoPtr->sw_minor_version = TCPIP_SW_MINOR_VERSION;
    VersionInfoPtr->sw_patch_version = TCPIP_SW_PATCH_VERSION;
}

Std_ReturnType TcpIp_RequestIpAddrAssignment(
    TcpIp_LocalAddrIdType LocalAddrId,
    TcpIp_IpAddrAssignmentType Type,
    const TcpIp_SockAddrType* LocalIpAddrPtr
)
{
#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (TcpIp_ModuleState != TCPIP_STATE_ONLINE)
    {
        (void)Det_ReportError(TCPIP_MODULE_ID, 0U, TCPIP_SID_REQUEST_IP_ADDR, TCPIP_E_NOTINIT);
        return E_NOT_OK;
    }
    if (LocalAddrId >= TCPIP_MAX_LOCAL_ADDR)
    {
        (void)Det_ReportError(TCPIP_MODULE_ID, 0U, TCPIP_SID_REQUEST_IP_ADDR, TCPIP_E_INV_ARG);
        return E_NOT_OK;
    }
#endif

    TcpIp_LocalAddrTable[LocalAddrId].assignmentType = Type;

    if ((Type == TCPIP_IPADDR_ASSIGNMENT_STATIC) && (LocalIpAddrPtr != NULL))
    {
        TcpIp_LocalAddrTable[LocalAddrId].ipAddr = *LocalIpAddrPtr;
    }

    TcpIp_LocalAddrTable[LocalAddrId].assigned = TRUE;
    EthSM_TcpIpModeIndication(0U, TCPIP_STATE_ONLINE);

    return E_OK;
}

Std_ReturnType TcpIp_GetIpAddr(
    TcpIp_LocalAddrIdType LocalAddrId,
    TcpIp_SockAddrType* IpAddrPtr,
    uint8* NetmaskPtr,
    TcpIp_SockAddrType* DefaultRouterPtr
)
{
#if (TCPIP_DEV_ERROR_DETECT == STD_ON)
    if (TcpIp_ModuleState != TCPIP_STATE_ONLINE)
    {
        (void)Det_ReportError(TCPIP_MODULE_ID, 0U, TCPIP_SID_GET_IP_ADDR, TCPIP_E_NOTINIT);
        return E_NOT_OK;
    }
    if (LocalAddrId >= TCPIP_MAX_LOCAL_ADDR)
    {
        (void)Det_ReportError(TCPIP_MODULE_ID, 0U, TCPIP_SID_GET_IP_ADDR, TCPIP_E_INV_ARG);
        return E_NOT_OK;
    }
    if (IpAddrPtr == NULL)
    {
        (void)Det_ReportError(TCPIP_MODULE_ID, 0U, TCPIP_SID_GET_IP_ADDR, TCPIP_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    if (TcpIp_LocalAddrTable[LocalAddrId].assigned == FALSE)
    {
        return E_NOT_OK;
    }

    *IpAddrPtr = TcpIp_LocalAddrTable[LocalAddrId].ipAddr;

    if (NetmaskPtr != NULL)
    {
        *NetmaskPtr = TcpIp_LocalAddrTable[LocalAddrId].netmask;
    }

    if (DefaultRouterPtr != NULL)
    {
        *DefaultRouterPtr = TcpIp_LocalAddrTable[LocalAddrId].defaultRouter;
    }

    return E_OK;
}
