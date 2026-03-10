/**
 * @file TcpIp.h
 * @brief AUTOSAR TcpIp Module - TCP/IP Stack Abstraction
 * @details Provides AUTOSAR-compliant API wrapping platform sockets (Winsock2 / POSIX).
 *          Sits between SoAd (upper layer) and EthIf (lower layer).
 *          Conforms to AUTOSAR CP R24-11 SWS_TcpIp.
 */

#ifndef TCPIP_H
#define TCPIP_H

/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "Std_Types.h"

/********************************************************************************************************/
/************************************************Defines*************************************************/
/********************************************************************************************************/

/* Module identification */
#define TCPIP_MODULE_ID                  (170U)
#define TCPIP_VENDOR_ID                  (0U)
#define TCPIP_SW_MAJOR_VERSION           (1U)
#define TCPIP_SW_MINOR_VERSION           (0U)
#define TCPIP_SW_PATCH_VERSION           (0U)

/* Service IDs for DET reporting */
#define TCPIP_SID_INIT                   (0x01U)
#define TCPIP_SID_SHUTDOWN               (0x02U)
#define TCPIP_SID_GET_SOCKET             (0x03U)
#define TCPIP_SID_CLOSE                  (0x04U)
#define TCPIP_SID_BIND                   (0x05U)
#define TCPIP_SID_TCP_CONNECT            (0x06U)
#define TCPIP_SID_TCP_LISTEN             (0x07U)
#define TCPIP_SID_UDP_TRANSMIT           (0x08U)
#define TCPIP_SID_TCP_TRANSMIT           (0x09U)
#define TCPIP_SID_RX_INDICATION          (0x0AU)
#define TCPIP_SID_MAIN_FUNCTION          (0x0BU)
#define TCPIP_SID_GET_VERSION_INFO       (0x0CU)
#define TCPIP_SID_REQUEST_IP_ADDR        (0x0DU)
#define TCPIP_SID_GET_IP_ADDR            (0x0EU)

/* DET error codes */
#define TCPIP_E_NOTINIT                  (0x01U)
#define TCPIP_E_PARAM_POINTER            (0x02U)
#define TCPIP_E_INV_ARG                  (0x03U)
#define TCPIP_E_INV_SOCKETID             (0x04U)
#define TCPIP_E_ALREADY_INIT             (0x05U)
#define TCPIP_E_NORESOURCE               (0x06U)

/* Configuration */
#define TCPIP_MAX_SOCKETS                (16U)
#define TCPIP_MAX_LOCAL_ADDR             (4U)
#define TCPIP_SOCKET_ID_INVALID          (0xFFFFU)

/* Development Error Detection */
#ifndef TCPIP_DEV_ERROR_DETECT
#define TCPIP_DEV_ERROR_DETECT           STD_ON
#endif

/********************************************************************************************************/
/*******************************************TypeDefinitions**********************************************/
/********************************************************************************************************/

/** @brief Socket identifier type */
typedef uint16 TcpIp_SocketIdType;

/** @brief Local address identifier type */
typedef uint8 TcpIp_LocalAddrIdType;

/** @brief Address domain */
typedef enum
{
    TCPIP_AF_INET  = 2U,
    TCPIP_AF_INET6 = 10U
} TcpIp_DomainType;

/** @brief Transport protocol */
typedef enum
{
    TCPIP_IPPROTO_TCP = 6U,
    TCPIP_IPPROTO_UDP = 17U
} TcpIp_ProtocolType;

/** @brief IP address assignment type */
typedef enum
{
    TCPIP_IPADDR_ASSIGNMENT_STATIC    = 0U,
    TCPIP_IPADDR_ASSIGNMENT_DHCP      = 1U,
    TCPIP_IPADDR_ASSIGNMENT_LINKLOCAL = 2U
} TcpIp_IpAddrAssignmentType;

/** @brief Module state */
typedef enum
{
    TCPIP_STATE_UNINIT  = 0U,
    TCPIP_STATE_ONLINE  = 1U,
    TCPIP_STATE_OFFLINE = 2U
} TcpIp_StateType;

/** @brief Socket state */
typedef enum
{
    TCPIP_SOCKET_STATE_UNUSED     = 0U,
    TCPIP_SOCKET_STATE_ALLOCATED  = 1U,
    TCPIP_SOCKET_STATE_BOUND      = 2U,
    TCPIP_SOCKET_STATE_LISTENING  = 3U,
    TCPIP_SOCKET_STATE_CONNECTED  = 4U
} TcpIp_SocketStateType;

/** @brief Socket address (IPv4) */
typedef struct
{
    TcpIp_DomainType domain;
    uint8            addr[4];   /* IPv4 address in network byte order */
    uint16           port;      /* Port in host byte order */
} TcpIp_SockAddrType;

/** @brief Configuration type */
typedef struct
{
    uint8 dummy;
} TcpIp_ConfigType;

/********************************************************************************************************/
/*****************************************FunctionPrototypes*********************************************/
/********************************************************************************************************/

/**
 * @brief Initialize the TcpIp module.
 * @param[in] ConfigPtr Pointer to configuration (may be NULL for default).
 */
void TcpIp_Init(const TcpIp_ConfigType* ConfigPtr);

/**
 * @brief Shut down the TcpIp module.
 */
void TcpIp_Shutdown(void);

/**
 * @brief Allocate a new socket.
 * @param[in]  Domain     Address family (TCPIP_AF_INET).
 * @param[in]  Protocol   Transport protocol (TCP or UDP).
 * @param[out] SocketIdPtr Pointer to receive the socket ID.
 * @return E_OK on success, E_NOT_OK on failure.
 */
Std_ReturnType TcpIp_GetSocketId(
    TcpIp_DomainType Domain,
    TcpIp_ProtocolType Protocol,
    TcpIp_SocketIdType* SocketIdPtr
);

/**
 * @brief Close a socket.
 * @param[in] SocketId Socket to close.
 * @param[in] Abort    TRUE for immediate RST, FALSE for graceful close.
 * @return E_OK on success, E_NOT_OK on failure.
 */
Std_ReturnType TcpIp_Close(TcpIp_SocketIdType SocketId, boolean Abort);

/**
 * @brief Bind a socket to a local address and port.
 * @param[in]     SocketId    Socket to bind.
 * @param[in]     LocalAddrId Local address identifier.
 * @param[in,out] PortPtr     Pointer to port (0 = auto-assign, updated on return).
 * @return E_OK on success, E_NOT_OK on failure.
 */
Std_ReturnType TcpIp_Bind(
    TcpIp_SocketIdType SocketId,
    TcpIp_LocalAddrIdType LocalAddrId,
    uint16* PortPtr
);

/**
 * @brief Initiate a TCP connection to a remote address.
 * @param[in] SocketId     Socket to connect.
 * @param[in] RemoteAddrPtr Remote address.
 * @return E_OK on success, E_NOT_OK on failure.
 */
Std_ReturnType TcpIp_TcpConnect(
    TcpIp_SocketIdType SocketId,
    const TcpIp_SockAddrType* RemoteAddrPtr
);

/**
 * @brief Set a TCP socket to listening state.
 * @param[in] SocketId    Socket to listen on.
 * @param[in] MaxChannels Maximum pending connections.
 * @return E_OK on success, E_NOT_OK on failure.
 */
Std_ReturnType TcpIp_TcpListen(TcpIp_SocketIdType SocketId, uint16 MaxChannels);

/**
 * @brief Transmit data on a UDP socket.
 * @param[in] SocketId      Socket to send from.
 * @param[in] DataPtr        Data buffer.
 * @param[in] RemoteAddrPtr  Destination address.
 * @param[in] TotalLength    Number of bytes to send.
 * @return E_OK on success, E_NOT_OK on failure.
 */
Std_ReturnType TcpIp_UdpTransmit(
    TcpIp_SocketIdType SocketId,
    const uint8* DataPtr,
    const TcpIp_SockAddrType* RemoteAddrPtr,
    uint16 TotalLength
);

/**
 * @brief Transmit data on a TCP socket.
 * @param[in] SocketId        Socket to send on.
 * @param[in] DataPtr          Data buffer.
 * @param[in] AvailableLength  Number of bytes to send.
 * @param[in] ForceRetrieve    TRUE to force send.
 * @return E_OK on success, E_NOT_OK on failure.
 */
Std_ReturnType TcpIp_TcpTransmit(
    TcpIp_SocketIdType SocketId,
    const uint8* DataPtr,
    uint32 AvailableLength,
    boolean ForceRetrieve
);

/**
 * @brief Indicate received data on a socket (called by lower layer).
 * @param[in] SocketId      Socket that received data.
 * @param[in] RemoteAddrPtr Source address.
 * @param[in] BufPtr        Received data.
 * @param[in] Length         Number of bytes received.
 */
void TcpIp_RxIndication(
    TcpIp_SocketIdType SocketId,
    const TcpIp_SockAddrType* RemoteAddrPtr,
    const uint8* BufPtr,
    uint16 Length
);

/**
 * @brief Cyclic main function - polls sockets for incoming data.
 */
void TcpIp_MainFunction(void);

/**
 * @brief Get module version information.
 * @param[out] VersionInfoPtr Pointer to version info structure.
 */
void TcpIp_GetVersionInfo(Std_VersionInfoType* VersionInfoPtr);

/**
 * @brief Request IP address assignment for a local address.
 * @param[in] LocalAddrId   Local address identifier.
 * @param[in] Type           Assignment type.
 * @param[in] LocalIpAddrPtr IP address to assign (for static).
 * @return E_OK on success, E_NOT_OK on failure.
 */
Std_ReturnType TcpIp_RequestIpAddrAssignment(
    TcpIp_LocalAddrIdType LocalAddrId,
    TcpIp_IpAddrAssignmentType Type,
    const TcpIp_SockAddrType* LocalIpAddrPtr
);

/**
 * @brief Get IP address information for a local address.
 * @param[in]  LocalAddrId     Local address identifier.
 * @param[out] IpAddrPtr       Current IP address.
 * @param[out] NetmaskPtr      Network mask (CIDR prefix length).
 * @param[out] DefaultRouterPtr Default gateway address.
 * @return E_OK on success, E_NOT_OK on failure.
 */
Std_ReturnType TcpIp_GetIpAddr(
    TcpIp_LocalAddrIdType LocalAddrId,
    TcpIp_SockAddrType* IpAddrPtr,
    uint8* NetmaskPtr,
    TcpIp_SockAddrType* DefaultRouterPtr
);

#endif /* TCPIP_H */
