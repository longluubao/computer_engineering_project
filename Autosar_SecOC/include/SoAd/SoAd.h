/**
 * @file SoAd.h
 * @brief AUTOSAR Socket Adapter Module
 * @details Provides AUTOSAR-compliant socket communication abstraction.
 *          Maps PDU transmissions to TcpIp socket operations.
 *          Conforms to AUTOSAR CP R24-11 SWS_SoAd.
 */

#ifndef INCLUDE_SOAD_H_
#define INCLUDE_SOAD_H_

/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "Std_Types.h"
#include "Com/ComStack_Types.h"
#include "TcpIp/TcpIp.h"

/********************************************************************************************************/
/************************************************Defines*************************************************/
/********************************************************************************************************/

/* Module identification */
#define SOAD_MODULE_ID                   (56U)
#define SOAD_VENDOR_ID                   (0U)
#define SOAD_SW_MAJOR_VERSION            (1U)
#define SOAD_SW_MINOR_VERSION            (0U)
#define SOAD_SW_PATCH_VERSION            (0U)

/* Service IDs for DET reporting */
#define SOAD_SID_INIT                    (0x01U)
#define SOAD_SID_IF_TRANSMIT             (0x03U)
#define SOAD_SID_TP_TRANSMIT             (0x04U)
#define SOAD_SID_GET_SOCON_ID            (0x05U)
#define SOAD_SID_OPEN_SOCON              (0x06U)
#define SOAD_SID_CLOSE_SOCON             (0x07U)
#define SOAD_SID_GET_LOCAL_ADDR          (0x08U)
#define SOAD_SID_GET_REMOTE_ADDR         (0x0BU)
#define SOAD_SID_SET_REMOTE_ADDR         (0x0CU)
#define SOAD_SID_ENABLE_ROUTING          (0x0EU)
#define SOAD_SID_DISABLE_ROUTING         (0x0FU)
#define SOAD_SID_GET_VERSION_INFO        (0x02U)
#define SOAD_SID_MAIN_FUNCTION_TX        (0x10U)
#define SOAD_SID_MAIN_FUNCTION_RX        (0x11U)
#define SOAD_SID_RX_INDICATION           (0x12U)
#define SOAD_SID_TX_CONFIRMATION         (0x13U)
#define SOAD_SID_SET_AP_BRIDGE_STATE     (0x14U)
#define SOAD_SID_DEINIT                  (0x15U)

/* DET error codes */
#define SOAD_E_NOTINIT                   (0x01U)
#define SOAD_E_PARAM_POINTER             (0x02U)
#define SOAD_E_INV_ARG                   (0x03U)
#define SOAD_E_INV_PDUHEADER_ID          (0x04U)
#define SOAD_E_INV_PDUID                 (0x05U)
#define SOAD_E_INV_SOCKETID              (0x06U)
#define SOAD_E_INIT_FAILED               (0x07U)
#define SOAD_E_INV_SOCON_ID              (0x08U)
#define SOAD_E_INV_ROUTING_GROUP_ID      (0x09U)

/* Development Error Detection */
#ifndef SOAD_DEV_ERROR_DETECT
#define SOAD_DEV_ERROR_DETECT            STD_ON
#endif

/* Configuration */
#define SOAD_BUFFERLENGTH                ((uint32)100)
#ifndef BUS_LENGTH
#define BUS_LENGTH                       (8U)
#endif
#define SOAD_MAX_SOCKET_CONNECTIONS      (16U)
#define SOAD_MAX_ROUTING_GROUPS          (8U)

/* Compile-time backend for SoAd payload path (default preserves current sockets behavior). */
#define SOAD_TCPIP_PAYLOAD_BACKEND_SOCKETS   (0U)
#define SOAD_TCPIP_PAYLOAD_BACKEND_ETHIF     (1U)
#ifndef SOAD_TCPIP_PAYLOAD_BACKEND
#define SOAD_TCPIP_PAYLOAD_BACKEND           SOAD_TCPIP_PAYLOAD_BACKEND_SOCKETS
#endif

/********************************************************************************************************/
/*******************************************TypeDefinitions**********************************************/
/********************************************************************************************************/

/** @brief Socket connection identifier type */
typedef uint16 SoAd_SoConIdType;

/** @brief Routing group identifier type */
typedef uint16 SoAd_RoutingGroupIdType;

/** @brief Socket connection mode */
typedef enum
{
    SOAD_SOCON_ONLINE    = 0U,
    SOAD_SOCON_RECONNECT = 1U,
    SOAD_SOCON_OFFLINE   = 2U
} SoAd_SoConModeType;

/** @brief Socket connection state (internal) */
typedef struct
{
    TcpIp_SocketIdType    SocketId;         /* TcpIp socket handle */
    TcpIp_ProtocolType    Protocol;         /* TCP or UDP */
    TcpIp_SockAddrType    RemoteAddr;       /* Remote peer address */
    TcpIp_LocalAddrIdType LocalAddrId;      /* Local address binding */
    uint16                LocalPort;        /* Local port */
    SoAd_SoConModeType    Mode;             /* Connection mode */
    boolean               IsAllocated;      /* TRUE if this entry is in use */
    PduIdType             TxPduId;          /* Mapped Tx PDU ID */
} SoAd_SoConStateType;

/** @brief Routing group state */
typedef struct
{
    boolean Enabled;
} SoAd_RoutingGroupStateType;

typedef enum
{
    SOAD_PDU_ROUTE_NONE = 0U,
    SOAD_PDU_ROUTE_IF = 1U,
    SOAD_PDU_ROUTE_TP = 2U
} SoAd_PduRouteType;

typedef struct
{
    PduIdType               SoAdPduId;
    SoAd_PduRouteType       SoAdPduRouteType;
    SoAd_RoutingGroupIdType SoAdRoutingGroupId;
    uint8                   SoAdAuthPduHeaderLength;
    uint8                   SoAdFreshnessValueTruncLength;
    uint16                  SoAdAuthInfoLengthBits;
    PduLengthType           SoAdAuthenticPduLength;
} SoAd_PduRouteConfigType;

typedef struct
{
    PduIdType             SoAdTxPduId;
    TcpIp_ProtocolType    SoAdProtocol;
    TcpIp_LocalAddrIdType SoAdLocalAddrId;
    uint16                SoAdLocalPort;
    TcpIp_SockAddrType    SoAdRemoteAddr;
    SoAd_SoConModeType    SoAdInitialMode;
} SoAd_SoConConfigType;

/** @brief Configuration type */
typedef struct
{
    const SoAd_PduRouteConfigType* SoAdPduRouteConfigPtr;
    uint16                         SoAdPduRouteConfigCount;
    const SoAd_SoConConfigType*    SoAdSoConConfigPtr;
    uint16                         SoAdSoConConfigCount;
    boolean                        SoAdEnableUnauthApControl;
} SoAd_ConfigType;

typedef enum
{
    SOAD_AP_BRIDGE_NOT_READY = 0U,
    SOAD_AP_BRIDGE_READY = 1U,
    SOAD_AP_BRIDGE_DEGRADED = 2U
} SoAd_ApBridgeStateType;

/********************************************************************************************************/
/*****************************************FunctionPrototype**********************************************/
/********************************************************************************************************/

/**
 * @brief Initialize the SoAd module.
 * @param[in] ConfigPtr Pointer to configuration (may be NULL for default).
 * @implements SWS_SoAd_00093
 */
void SoAd_Init(const SoAd_ConfigType* ConfigPtr);
void SoAd_DeInit(void);

/**
 * @brief Requests transmission of an IF-PDU over socket.
 * @param[in] TxPduId PDU identifier.
 * @param[in] PduInfoPtr Pointer to PDU data and length.
 * @return E_OK on success, E_NOT_OK on failure.
 * @implements SWS_SoAd_00091
 */
Std_ReturnType SoAd_IfTransmit(
    PduIdType TxPduId,
    const PduInfoType* PduInfoPtr
);

/**
 * @brief Requests transmission of a TP-PDU over socket.
 * @param[in] TxPduId PDU identifier.
 * @param[in] PduInfoPtr Pointer to PDU info with total length.
 * @return E_OK on success, E_NOT_OK on failure.
 * @implements SWS_SoAd_00092
 */
Std_ReturnType SoAd_TpTransmit(
    PduIdType TxPduId,
    const PduInfoType* PduInfoPtr
);

/**
 * @brief Get socket connection ID for a given Tx PDU.
 * @param[in]  TxPduId   PDU identifier.
 * @param[out] SoConIdPtr Pointer to receive the socket connection ID.
 * @return E_OK if found, E_NOT_OK otherwise.
 * @implements SWS_SoAd_00609
 */
Std_ReturnType SoAd_GetSoConId(
    PduIdType TxPduId,
    SoAd_SoConIdType* SoConIdPtr
);

/**
 * @brief Open a socket connection.
 * @param[in] SoConId Socket connection identifier.
 * @return E_OK on success, E_NOT_OK on failure.
 * @implements SWS_SoAd_00588
 */
Std_ReturnType SoAd_OpenSoCon(SoAd_SoConIdType SoConId);

/**
 * @brief Close a socket connection.
 * @param[in] SoConId Socket connection identifier.
 * @param[in] Abort   TRUE for immediate close, FALSE for graceful.
 * @return E_OK on success, E_NOT_OK on failure.
 * @implements SWS_SoAd_00590
 */
Std_ReturnType SoAd_CloseSoCon(SoAd_SoConIdType SoConId, boolean Abort);

/**
 * @brief Get local address of a socket connection.
 * @param[in]  SoConId         Socket connection identifier.
 * @param[out] LocalAddrPtr    Local IP address.
 * @param[out] NetmaskPtr      Network mask (CIDR prefix).
 * @param[out] DefaultRouterPtr Default gateway.
 * @return E_OK on success, E_NOT_OK on failure.
 * @implements SWS_SoAd_00512
 */
Std_ReturnType SoAd_GetLocalAddr(
    SoAd_SoConIdType SoConId,
    TcpIp_SockAddrType* LocalAddrPtr,
    uint8* NetmaskPtr,
    TcpIp_SockAddrType* DefaultRouterPtr
);

/**
 * @brief Get remote address of a socket connection.
 * @param[in]  SoConId    Socket connection identifier.
 * @param[out] IpAddrPtr  Remote IP address.
 * @return E_OK on success, E_NOT_OK on failure.
 * @implements SWS_SoAd_00514
 */
Std_ReturnType SoAd_GetRemoteAddr(
    SoAd_SoConIdType SoConId,
    TcpIp_SockAddrType* IpAddrPtr
);

/**
 * @brief Set remote address of a socket connection.
 * @param[in] SoConId       Socket connection identifier.
 * @param[in] RemoteAddrPtr Remote IP address to set.
 * @return E_OK on success, E_NOT_OK on failure.
 * @implements SWS_SoAd_00515
 */
Std_ReturnType SoAd_SetRemoteAddr(
    SoAd_SoConIdType SoConId,
    const TcpIp_SockAddrType* RemoteAddrPtr
);

/**
 * @brief Enable a routing group.
 * @param[in] RoutingGroupId Routing group identifier.
 * @return E_OK on success, E_NOT_OK on failure.
 * @implements SWS_SoAd_00516
 */
Std_ReturnType SoAd_EnableRouting(SoAd_RoutingGroupIdType RoutingGroupId);

/**
 * @brief Disable a routing group.
 * @param[in] RoutingGroupId Routing group identifier.
 * @return E_OK on success, E_NOT_OK on failure.
 * @implements SWS_SoAd_00517
 */
Std_ReturnType SoAd_DisableRouting(SoAd_RoutingGroupIdType RoutingGroupId);

/**
 * @brief Get version information of the SoAd module.
 * @param[out] VersionInfoPtr Pointer to version info structure.
 * @implements SWS_SoAd_00096
 */
void SoAd_GetVersionInfo(Std_VersionInfoType* VersionInfoPtr);

/**
 * @brief Cyclic main function for transmission processing.
 * @implements SWS_SoAd_00094
 */
void SoAd_MainFunctionTx(void);

/**
 * @brief Cyclic main function for reception processing.
 * @implements SWS_SoAd_00095
 */
void SoAd_MainFunctionRx(void);

Std_ReturnType SoAd_SetApBridgeState(SoAd_ApBridgeStateType State);

/**
 * @brief Indicate received socket data from TcpIp into SoAd routing.
 * @param[in] SocketId      Socket that received the payload.
 * @param[in] RemoteAddrPtr Remote peer address.
 * @param[in] BufPtr        Payload buffer.
 * @param[in] Length        Payload length.
 */
void SoAd_RxIndication(
    TcpIp_SocketIdType SocketId,
    const TcpIp_SockAddrType* RemoteAddrPtr,
    const uint8* BufPtr,
    uint16 Length
);

/* Internal callbacks used by SoAd TP processing */
void SoAdTp_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);
void SoAdTp_TxConfirmation(PduIdType TxPduId, Std_ReturnType result);

/* PduR callback declarations (provided by PduR_SoAd) */
void PduR_SoAdIfRxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);
void PduR_SoAdIfTxConfirmation(PduIdType TxPduId, Std_ReturnType result);
BufReq_ReturnType PduR_SoAdTpCopyTxData(PduIdType id, const PduInfoType* info, const RetryInfoType* retry, PduLengthType* availableDataPtr);
void PduR_SoAdTpTxConfirmation(PduIdType TxPduId, Std_ReturnType result);
BufReq_ReturnType PduR_SoAdTpCopyRxData(PduIdType id, const PduInfoType* info, PduLengthType* bufferSizePtr);
BufReq_ReturnType PduR_SoAdStartOfReception(PduIdType id, const PduInfoType* info, PduLengthType TpSduLength, PduLengthType* bufferSizePtr);
void PduR_SoAdTpRxIndication(PduIdType id, Std_ReturnType result);

#endif  /* INCLUDE_SOAD_H_ */
