/**
 * @file SoAd.c
 * @brief AUTOSAR Socket Adapter Module Implementation
 * @details Routes PDU transmissions through TcpIp module instead of raw ethernet.
 *          Manages socket connections and routing groups per AUTOSAR SWS_SoAd.
 */

/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "ApBridge/ApBridge.h"
#include "BswM/BswM.h"
#include "Det/Det.h"
#include "PduR/PduR_SoAd.h"
#include "PQC/PQC.h"
#include "SecOC/SecOC.h"
#include "SecOC/SecOC_Cfg.h"
#include "SecOC/SecOC_Debug.h"
#include "SecOC/SecOC_PQC_Cfg.h"
#include "SoAd/SoAd.h"
#include "SoAd/SoAd_PQC.h"
#include "Std_Types.h"
#include "TcpIp/TcpIp.h"
#if (SOAD_TCPIP_PAYLOAD_BACKEND == SOAD_TCPIP_PAYLOAD_BACKEND_ETHIF)
    #include "EthIf/EthIf.h"
#endif

#ifdef SCHEDULER_ON
    #include <pthread.h>
#endif

#include <string.h>

/* MISRA C:2012 Rule 8.4 - Forward declarations for external linkage functions */
extern void SoAd_Init(const SoAd_ConfigType* ConfigPtr);
extern void SoAd_DeInit(void);
extern Std_ReturnType SoAd_IfTransmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr);
extern Std_ReturnType SoAd_TpTransmit(PduIdType SoAdTxSduId, const PduInfoType* SoAdTxInfoPtr);
extern Std_ReturnType SoAd_GetSoConId(PduIdType TxPduId, SoAd_SoConIdType* SoConIdPtr);
extern Std_ReturnType SoAd_OpenSoCon(SoAd_SoConIdType SoConId);
extern Std_ReturnType SoAd_CloseSoCon(SoAd_SoConIdType SoConId, boolean Abort);
extern Std_ReturnType SoAd_GetLocalAddr(SoAd_SoConIdType SoConId, TcpIp_SockAddrType* LocalAddrPtr, uint8* NetmaskPtr, TcpIp_SockAddrType* DefaultRouterPtr);
extern Std_ReturnType SoAd_GetRemoteAddr(SoAd_SoConIdType SoConId, TcpIp_SockAddrType* IpAddrPtr);
extern Std_ReturnType SoAd_SetRemoteAddr(SoAd_SoConIdType SoConId, const TcpIp_SockAddrType* RemoteAddrPtr);
extern Std_ReturnType SoAd_EnableRouting(SoAd_RoutingGroupIdType RoutingGroupId);
extern Std_ReturnType SoAd_DisableRouting(SoAd_RoutingGroupIdType RoutingGroupId);
extern void SoAd_GetVersionInfo(Std_VersionInfoType* VersionInfoPtr);
extern void SoAd_RxIndication(TcpIp_SocketIdType SocketId, const TcpIp_SockAddrType* RemoteAddrPtr, const uint8* BufPtr, uint16 Length);
extern void SoAdTp_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);
extern void SoAdTp_TxConfirmation(PduIdType TxPduId, Std_ReturnType result);
extern void SoAd_MainFunctionTx(void);
extern void SoAd_MainFunctionRx(void);
extern Std_ReturnType SoAd_SetApBridgeState(SoAd_ApBridgeStateType State);

/* MISRA C:2012 Rule 17.3 - Redundant forward declarations to guarantee visibility */
extern boolean SoAd_PQC_HandleControlMessage(const uint8* BufPtr, uint16 Length);
extern Std_ReturnType SoAd_PQC_Init(void);

/* Forward declarations for cross-module AUTOSAR functions called herein */
extern Std_ReturnType TcpIp_GetSocketId(TcpIp_DomainType Domain, TcpIp_ProtocolType Protocol, TcpIp_SocketIdType* SocketIdPtr);
extern Std_ReturnType TcpIp_Bind(TcpIp_SocketIdType SocketId, TcpIp_LocalAddrIdType LocalAddrId, uint16* PortPtr);

#if (SOAD_TCPIP_PAYLOAD_BACKEND == SOAD_TCPIP_PAYLOAD_BACKEND_ETHIF)
#define SOAD_ETHIF_CTRL_IDX                  ((uint8)0U)
#define SOAD_ETHIF_FRAME_TYPE_BASE           ((Eth_FrameType)0x9000U)
#endif
#define SOAD_ROUTING_GROUP_GATEWAY           ((SoAd_RoutingGroupIdType)0U)
#define SOAD_ROUTING_GROUP_DIAG              ((SoAd_RoutingGroupIdType)1U)
#define SOAD_AP_CTRL_MAGIC0                  ((uint8)0xA5U)
#define SOAD_AP_CTRL_MAGIC1                  ((uint8)0x5AU)
#define SOAD_AP_CTRL_VERSION                 ((uint8)0x01U)
#define SOAD_AP_CTRL_PDU_LENGTH              ((uint16)6U)
#define SOAD_ENABLE_UNAUTH_AP_CONTROL        STD_OFF
#define SOAD_TP_RX_QUEUE_DEPTH               ((uint8)4U)
#define SOAD_DEFAULT_ETH_GATEWAY_PDU_ID      ((PduIdType)2U)
#define SOAD_MAX_PDU_PROCESSING              ((SECOC_NUM_OF_RX_PDU_PROCESSING > SECOC_NUM_OF_TX_PDU_PROCESSING) ? \
                                              SECOC_NUM_OF_RX_PDU_PROCESSING : SECOC_NUM_OF_TX_PDU_PROCESSING)

/********************************************************************************************************/
/******************************************GlobalVaribles************************************************/
/********************************************************************************************************/

static boolean SoAd_Initialized = FALSE;

/* TP buffers (preserved from original) */
static PduInfoType SoAdTp_Buffer[SOAD_BUFFERLENGTH];
static PduInfoType SoAdTp_Buffer_Rx[SECOC_NUM_OF_RX_PDU_PROCESSING];
static uint8 SoAdTp_Buffer_RxData[SECOC_NUM_OF_RX_PDU_PROCESSING][SECOC_SECPDU_MAX_LENGTH];
static PduInfoType SoAdTp_RxQueue[SECOC_NUM_OF_RX_PDU_PROCESSING][SOAD_TP_RX_QUEUE_DEPTH];
static uint8 SoAdTp_RxQueueData[SECOC_NUM_OF_RX_PDU_PROCESSING][SOAD_TP_RX_QUEUE_DEPTH][SECOC_SECPDU_MAX_LENGTH];
static uint8 SoAdTp_RxQueueHead[SECOC_NUM_OF_RX_PDU_PROCESSING] = {0};
static uint8 SoAdTp_RxQueueTail[SECOC_NUM_OF_RX_PDU_PROCESSING] = {0};
static uint8 SoAdTp_RxQueueCount[SECOC_NUM_OF_RX_PDU_PROCESSING] = {0};
static uint8 SoAdTp_Recieve_Counter[SECOC_NUM_OF_RX_PDU_PROCESSING] = {0};
static uint8 SoAdTp_Processed_Counter[SECOC_NUM_OF_RX_PDU_PROCESSING] = {0};
static PduLengthType SoAdTp_ReceivedBytes[SECOC_NUM_OF_RX_PDU_PROCESSING] = {0};
static PduLengthType SoAdTp_ProcessedBytes[SECOC_NUM_OF_RX_PDU_PROCESSING] = {0};
static PduLengthType SoAdTp_secureLength_Recieve[SECOC_NUM_OF_RX_PDU_PROCESSING] = {0};

/* Socket connection table */
static SoAd_SoConStateType SoAd_SoConStates[SOAD_MAX_SOCKET_CONNECTIONS];

/* Routing group table */
static SoAd_RoutingGroupStateType SoAd_RoutingGroupStates[SOAD_MAX_ROUTING_GROUPS];
static SoAd_ApBridgeStateType SoAd_ApBridgeState = SOAD_AP_BRIDGE_NOT_READY;
static boolean SoAd_ApBridgeExternalControl = FALSE;
static boolean SoAd_UnauthApControlEnabled = SOAD_ENABLE_UNAUTH_AP_CONTROL;

static const SoAd_PduRouteConfigType SoAd_DefaultPduRouteConfig[] =
{
    {
        SOAD_DEFAULT_ETH_GATEWAY_PDU_ID,
        SOAD_PDU_ROUTE_TP,
        SOAD_ROUTING_GROUP_GATEWAY,
        0U,
        8U,
        32U,
        3U
    }
};

static const SoAd_SoConConfigType SoAd_DefaultSoConConfig[] =
{
    {
        SOAD_DEFAULT_ETH_GATEWAY_PDU_ID,
        TCPIP_IPPROTO_UDP,
        0U,
        50002U,
        {
            TCPIP_AF_INET,
            {127U, 0U, 0U, 1U},
            50002U
        },
        SOAD_SOCON_ONLINE
    }
};

static SoAd_ConfigType SoAd_ActiveConfig = {
    SoAd_DefaultPduRouteConfig,
    (uint16)(sizeof(SoAd_DefaultPduRouteConfig) / sizeof(SoAd_DefaultPduRouteConfig[0])),
    SoAd_DefaultSoConConfig,
    (uint16)(sizeof(SoAd_DefaultSoConConfig) / sizeof(SoAd_DefaultSoConConfig[0])),
    SOAD_ENABLE_UNAUTH_AP_CONTROL
};

static boolean SoAd_IsRoutingAllowedForPdu(PduIdType PduId);
static Std_ReturnType SoAd_GetPduRouteConfig(PduIdType PduId, const SoAd_PduRouteConfigType** RouteCfgPtr);

#ifdef SCHEDULER_ON
    extern pthread_mutex_t lock;
#endif

/********************************************************************************************************/
/**************************************Static Helper Functions*******************************************/
/********************************************************************************************************/

/**
 * @brief Find socket connection for a given TxPduId.
 * @details Searches the connection table for a connection mapped to TxPduId.
 *          If none exists and there is a free slot, allocates a default UDP connection.
 */
static SoAd_SoConIdType SoAd_FindOrCreateSoCon(PduIdType TxPduId)
{
    SoAd_SoConIdType idx;

    for (idx = 0U; idx < SOAD_MAX_SOCKET_CONNECTIONS; idx++)
    {
        if ((SoAd_SoConStates[idx].IsAllocated == TRUE) &&
            (SoAd_SoConStates[idx].TxPduId == TxPduId))
        {
            return idx;
        }
    }

    return SOAD_MAX_SOCKET_CONNECTIONS;
}

static void SoAd_UpdateApReadinessStatus(void)
{
    SoAd_SoConIdType idx;
    SoAd_ApBridgeStateType DerivedState = SOAD_AP_BRIDGE_NOT_READY;

    if (SoAd_ApBridgeExternalControl == TRUE)
    {
        return;
    }

    for (idx = 0U; idx < SOAD_MAX_SOCKET_CONNECTIONS; idx++)
    {
        if ((SoAd_SoConStates[idx].IsAllocated == TRUE) &&
            (SoAd_SoConStates[idx].Mode == SOAD_SOCON_ONLINE))
        {
            DerivedState = SOAD_AP_BRIDGE_READY;
            break;
        }
    }

    if (DerivedState != SoAd_ApBridgeState)
    {
        SoAd_ApBridgeState = DerivedState;
        (void)BswM_RequestMode(BSWM_REQUESTER_ID_AP_READY,
                               (BswM_ModeType)SoAd_ApBridgeState);
    }
}

static boolean SoAd_IsApControlPdu(const uint8* BufPtr, uint16 Length)
{
    if ((BufPtr == NULL) || (Length != SOAD_AP_CTRL_PDU_LENGTH))
    {
        return FALSE;
    }

    if ((BufPtr[0] != SOAD_AP_CTRL_MAGIC0) ||
        (BufPtr[1] != SOAD_AP_CTRL_MAGIC1) ||
        (BufPtr[2] != SOAD_AP_CTRL_VERSION))
    {
        return FALSE;
    }

    if (BufPtr[3] > (uint8)SOAD_AP_BRIDGE_DEGRADED)
    {
        return FALSE;
    }

    if ((uint8)(BufPtr[0] ^ BufPtr[1] ^ BufPtr[2] ^ BufPtr[3] ^ BufPtr[4]) != BufPtr[5])
    {
        return FALSE;
    }

    return TRUE;
}

static void SoAd_HandleApControlPdu(const uint8* BufPtr)
{
    SoAd_ApBridgeStateType ControlState;

    if (BufPtr == NULL)
    {
        return;
    }

    ControlState = (SoAd_ApBridgeStateType)BufPtr[3];
    ApBridge_ReportHeartbeat(TRUE);

    if (ControlState == SOAD_AP_BRIDGE_READY)
    {
        ApBridge_ReportServiceStatus(TRUE);
    }
    else
    {
        ApBridge_ReportServiceStatus(FALSE);
    }

    (void)SoAd_SetApBridgeState(ControlState);
}

/**
 * @brief Transmit data via TcpIp using the socket connection for TxPduId.
 */
static Std_ReturnType SoAd_TcpIpTransmit(PduIdType TxPduId, const uint8* DataPtr, uint16 Length)
{
    SoAd_SoConIdType soConId;
    Std_ReturnType result;

    soConId = SoAd_FindOrCreateSoCon(TxPduId);
    if (soConId >= SOAD_MAX_SOCKET_CONNECTIONS)
    {
        return E_NOT_OK;
    }

    if (SoAd_SoConStates[soConId].Mode != SOAD_SOCON_ONLINE)
    {
        return E_NOT_OK;
    }

    if (SoAd_SoConStates[soConId].Protocol == TCPIP_IPPROTO_UDP)
    {
        result = TcpIp_UdpTransmit(
            SoAd_SoConStates[soConId].SocketId,
            DataPtr,
            &SoAd_SoConStates[soConId].RemoteAddr,
            Length
        );
    }
    else
    {
        result = TcpIp_TcpTransmit(
            SoAd_SoConStates[soConId].SocketId,
            DataPtr,
            (uint32)Length,
            TRUE
        );
    }

    return result;
}

#if (SOAD_TCPIP_PAYLOAD_BACKEND == SOAD_TCPIP_PAYLOAD_BACKEND_ETHIF)
static Std_ReturnType SoAd_EthIfTransmit(PduIdType TxPduId, const uint8* DataPtr, uint16 Length)
{
    Eth_BufIdxType BufIdx = 0U;
    uint8* BufPtr = NULL;
    uint16 RequestedLength = Length;
    Eth_FrameType FrameType;

    if ((Length == 0U) || (Length > ETHIF_TX_BUF_SIZE))
    {
        return E_NOT_OK;
    }

    FrameType = (Eth_FrameType)(SOAD_ETHIF_FRAME_TYPE_BASE + (Eth_FrameType)TxPduId);
    if (EthIf_ProvideTxBuffer(SOAD_ETHIF_CTRL_IDX,
                              FrameType,
                              0U,
                              &BufIdx,
                              &BufPtr,
                              &RequestedLength) != BUFREQ_OK)
    {
        return E_NOT_OK;
    }

    if ((BufPtr == NULL) || (RequestedLength < Length))
    {
        return E_NOT_OK;
    }

    (void)memcpy(BufPtr, DataPtr, Length);
    return EthIf_Transmit(SOAD_ETHIF_CTRL_IDX, BufIdx, FrameType, TRUE, Length, NULL);
}

static void SoAd_EthIfRxIndicationCbk(uint8 CtrlIdx,
                                      Eth_FrameType FrameType,
                                      boolean IsBroadcast,
                                      const uint8* PhysAddrPtr,
                                      const uint8* DataPtr,
                                      uint16 LenByte)
{
    PduIdType RxPduId;
    PduInfoType pduInfo;
    const SoAd_PduRouteConfigType* RouteCfgPtr = NULL;
    (void)CtrlIdx;
    (void)IsBroadcast;
    (void)PhysAddrPtr;

    if ((DataPtr == NULL) || (LenByte == 0U))
    {
        return;
    }
    if ((FrameType < SOAD_ETHIF_FRAME_TYPE_BASE) ||
        (FrameType >= (Eth_FrameType)(SOAD_ETHIF_FRAME_TYPE_BASE + SECOC_NUM_OF_RX_PDU_PROCESSING)))
    {
        return;
    }

    RxPduId = (PduIdType)(FrameType - SOAD_ETHIF_FRAME_TYPE_BASE);
    if (SoAd_IsRoutingAllowedForPdu(RxPduId) == FALSE)
    {
        return;
    }
    if (SoAd_GetPduRouteConfig(RxPduId, &RouteCfgPtr) != E_OK)
    {
        return;
    }
    pduInfo.SduDataPtr = (uint8*)DataPtr;
    pduInfo.MetaDataPtr = NULL;
    pduInfo.SduLength = (PduLengthType)LenByte;

    if (RouteCfgPtr->SoAdPduRouteType == SOAD_PDU_ROUTE_TP)
    {
        SoAdTp_RxIndication(RxPduId, &pduInfo);
    }
    else if (RouteCfgPtr->SoAdPduRouteType == SOAD_PDU_ROUTE_IF)
    {
        PduR_SoAdIfRxIndication(RxPduId, &pduInfo);
    }
    else
    {
        /* Ignore non-SoAd routed types. */
    }
}
#endif

/**
 * @brief Find mapped Rx PDU ID for an incoming socket.
 */
static Std_ReturnType SoAd_FindRxPduIdBySocket(
    TcpIp_SocketIdType SocketId,
    PduIdType* RxPduIdPtr,
    SoAd_SoConIdType* SoConIdPtr
)
{
    SoAd_SoConIdType idx;
    for (idx = 0U; idx < SOAD_MAX_SOCKET_CONNECTIONS; idx++)
    {
        if ((SoAd_SoConStates[idx].IsAllocated == TRUE) &&
            (SoAd_SoConStates[idx].SocketId == SocketId))
        {
            if (RxPduIdPtr != NULL)
            {
                *RxPduIdPtr = SoAd_SoConStates[idx].TxPduId;
            }
            if (SoConIdPtr != NULL)
            {
                *SoConIdPtr = idx;
            }
            return E_OK;
        }
    }
    return E_NOT_OK;
}

static Std_ReturnType SoAd_GetPduRouteConfig(PduIdType PduId, const SoAd_PduRouteConfigType** RouteCfgPtr)
{
    uint16 idx;

    if (RouteCfgPtr == NULL)
    {
        return E_NOT_OK;
    }

    for (idx = 0U; idx < SoAd_ActiveConfig.SoAdPduRouteConfigCount; idx++)
    {
        if (SoAd_ActiveConfig.SoAdPduRouteConfigPtr[idx].SoAdPduId == PduId)
        {
            *RouteCfgPtr = &SoAd_ActiveConfig.SoAdPduRouteConfigPtr[idx];
            return E_OK;
        }
    }

    return E_NOT_OK;
}

static boolean SoAd_IsRoutingGroupEnabled(SoAd_RoutingGroupIdType RoutingGroupId)
{
    if (RoutingGroupId >= SOAD_MAX_ROUTING_GROUPS)
    {
        return FALSE;
    }

    return SoAd_RoutingGroupStates[RoutingGroupId].Enabled;
}

static boolean SoAd_IsRoutingAllowedForPdu(PduIdType PduId)
{
    const SoAd_PduRouteConfigType* RouteCfgPtr = NULL;

    if (PduId >= SOAD_MAX_PDU_PROCESSING)
    {
        return FALSE;
    }

    if (SoAd_GetPduRouteConfig(PduId, &RouteCfgPtr) != E_OK)
    {
        return FALSE;
    }
    if (RouteCfgPtr->SoAdPduRouteType == SOAD_PDU_ROUTE_NONE)
    {
        return FALSE;
    }

    return SoAd_IsRoutingGroupEnabled(RouteCfgPtr->SoAdRoutingGroupId);
}

static void SoAd_ResetTpReceptionState(PduIdType RxPduId)
{
    SoAdTp_RxQueueHead[RxPduId] = 0U;
    SoAdTp_RxQueueTail[RxPduId] = 0U;
    SoAdTp_RxQueueCount[RxPduId] = 0U;
    SoAdTp_Recieve_Counter[RxPduId] = 0U;
    SoAdTp_Processed_Counter[RxPduId] = 0U;
    SoAdTp_ReceivedBytes[RxPduId] = 0U;
    SoAdTp_ProcessedBytes[RxPduId] = 0U;
    SoAdTp_secureLength_Recieve[RxPduId] = 0U;
    SoAdTp_Buffer_Rx[RxPduId].SduLength = 0U;
}

static Std_ReturnType SoAd_RecreateSoConSocket(SoAd_SoConIdType SoConId)
{
    TcpIp_SocketIdType SockId = TCPIP_SOCKET_ID_INVALID;
    uint16 BindPort;

    if (SoConId >= SOAD_MAX_SOCKET_CONNECTIONS)
    {
        return E_NOT_OK;
    }

    if (TcpIp_GetSocketId(TCPIP_AF_INET, SoAd_SoConStates[SoConId].Protocol, &SockId) != E_OK)
    {
        return E_NOT_OK;
    }

    SoAd_SoConStates[SoConId].SocketId = SockId;

    if (SoAd_SoConStates[SoConId].LocalPort == 0U)
    {
        SoAd_SoConStates[SoConId].LocalPort = (uint16)(50000U + SoAd_SoConStates[SoConId].TxPduId);
    }

    BindPort = SoAd_SoConStates[SoConId].LocalPort;
    if (TcpIp_Bind(SockId, SoAd_SoConStates[SoConId].LocalAddrId, &BindPort) != E_OK)
    {
        (void)TcpIp_Close(SockId, TRUE);
        SoAd_SoConStates[SoConId].SocketId = TCPIP_SOCKET_ID_INVALID;
        return E_NOT_OK;
    }

    SoAd_SoConStates[SoConId].LocalPort = BindPort;
    return E_OK;
}

/********************************************************************************************************/
/********************************************Functions***************************************************/
/********************************************************************************************************/

void SoAd_Init(const SoAd_ConfigType* ConfigPtr)
{
    uint16 idx;

    if ((ConfigPtr != NULL) &&
        (ConfigPtr->SoAdPduRouteConfigPtr != NULL) &&
        (ConfigPtr->SoAdSoConConfigPtr != NULL))
    {
        SoAd_ActiveConfig = *ConfigPtr;
    }
    else
    {
        SoAd_ActiveConfig.SoAdPduRouteConfigPtr = SoAd_DefaultPduRouteConfig;
        SoAd_ActiveConfig.SoAdPduRouteConfigCount =
            (uint16)(sizeof(SoAd_DefaultPduRouteConfig) / sizeof(SoAd_DefaultPduRouteConfig[0]));
        SoAd_ActiveConfig.SoAdSoConConfigPtr = SoAd_DefaultSoConConfig;
        SoAd_ActiveConfig.SoAdSoConConfigCount =
            (uint16)(sizeof(SoAd_DefaultSoConConfig) / sizeof(SoAd_DefaultSoConConfig[0]));
        SoAd_ActiveConfig.SoAdEnableUnauthApControl = SOAD_ENABLE_UNAUTH_AP_CONTROL;
    }
    SoAd_UnauthApControlEnabled = SoAd_ActiveConfig.SoAdEnableUnauthApControl;

    /* Initialize socket connection table */
    for (idx = 0U; idx < SOAD_MAX_SOCKET_CONNECTIONS; idx++)
    {
        SoAd_SoConStates[idx].SocketId    = TCPIP_SOCKET_ID_INVALID;
        SoAd_SoConStates[idx].Protocol    = TCPIP_IPPROTO_UDP;
        SoAd_SoConStates[idx].LocalAddrId = 0U;
        SoAd_SoConStates[idx].LocalPort   = 0U;
        SoAd_SoConStates[idx].Mode        = SOAD_SOCON_OFFLINE;
        SoAd_SoConStates[idx].IsAllocated = FALSE;
        SoAd_SoConStates[idx].TxPduId     = 0U;
        (void)memset(&SoAd_SoConStates[idx].RemoteAddr, 0, sizeof(TcpIp_SockAddrType));
    }

    for (idx = 0U;
         (idx < SoAd_ActiveConfig.SoAdSoConConfigCount) && (idx < SOAD_MAX_SOCKET_CONNECTIONS);
         idx++)
    {
        TcpIp_SocketIdType SockId = TCPIP_SOCKET_ID_INVALID;
        uint16 BindPort = SoAd_ActiveConfig.SoAdSoConConfigPtr[idx].SoAdLocalPort;

        if (TcpIp_GetSocketId(TCPIP_AF_INET,
                              SoAd_ActiveConfig.SoAdSoConConfigPtr[idx].SoAdProtocol,
                              &SockId) != E_OK)
        {
            continue;
        }
        if (TcpIp_Bind(SockId,
                       SoAd_ActiveConfig.SoAdSoConConfigPtr[idx].SoAdLocalAddrId,
                       &BindPort) != E_OK)
        {
            (void)TcpIp_Close(SockId, TRUE);
            continue;
        }

        SoAd_SoConStates[idx].SocketId = SockId;
        SoAd_SoConStates[idx].Protocol = SoAd_ActiveConfig.SoAdSoConConfigPtr[idx].SoAdProtocol;
        SoAd_SoConStates[idx].LocalAddrId = SoAd_ActiveConfig.SoAdSoConConfigPtr[idx].SoAdLocalAddrId;
        SoAd_SoConStates[idx].LocalPort = BindPort;
        SoAd_SoConStates[idx].Mode = SoAd_ActiveConfig.SoAdSoConConfigPtr[idx].SoAdInitialMode;
        SoAd_SoConStates[idx].IsAllocated = TRUE;
        SoAd_SoConStates[idx].TxPduId = SoAd_ActiveConfig.SoAdSoConConfigPtr[idx].SoAdTxPduId;
        SoAd_SoConStates[idx].RemoteAddr = SoAd_ActiveConfig.SoAdSoConConfigPtr[idx].SoAdRemoteAddr;
    }

    /* Initialize routing groups disabled until BswM arbitration opens paths. */
    for (idx = 0U; idx < SOAD_MAX_ROUTING_GROUPS; idx++)
    {
        SoAd_RoutingGroupStates[idx].Enabled = FALSE;
    }

    /* Initialize TP buffers */
    (void)memset(SoAdTp_Buffer, 0, sizeof(SoAdTp_Buffer));
    (void)memset(SoAdTp_Buffer_Rx, 0, sizeof(SoAdTp_Buffer_Rx));
    (void)memset(SoAdTp_Buffer_RxData, 0, sizeof(SoAdTp_Buffer_RxData));
    (void)memset(SoAdTp_RxQueue, 0, sizeof(SoAdTp_RxQueue));
    (void)memset(SoAdTp_RxQueueData, 0, sizeof(SoAdTp_RxQueueData));
    (void)memset(SoAdTp_RxQueueHead, 0, sizeof(SoAdTp_RxQueueHead));
    (void)memset(SoAdTp_RxQueueTail, 0, sizeof(SoAdTp_RxQueueTail));
    (void)memset(SoAdTp_RxQueueCount, 0, sizeof(SoAdTp_RxQueueCount));
    (void)memset(SoAdTp_Recieve_Counter, 0, sizeof(SoAdTp_Recieve_Counter));
    (void)memset(SoAdTp_Processed_Counter, 0, sizeof(SoAdTp_Processed_Counter));
    (void)memset(SoAdTp_ReceivedBytes, 0, sizeof(SoAdTp_ReceivedBytes));
    (void)memset(SoAdTp_ProcessedBytes, 0, sizeof(SoAdTp_ProcessedBytes));
    (void)memset(SoAdTp_secureLength_Recieve, 0, sizeof(SoAdTp_secureLength_Recieve));

#if (SOAD_TCPIP_PAYLOAD_BACKEND == SOAD_TCPIP_PAYLOAD_BACKEND_ETHIF)
    EthIf_SetRxIndicationCallback(SoAd_EthIfRxIndicationCbk);
#endif

    SoAd_Initialized = TRUE;
    SoAd_ApBridgeState = SOAD_AP_BRIDGE_NOT_READY;
    SoAd_ApBridgeExternalControl = FALSE;
    (void)BswM_RequestMode(BSWM_REQUESTER_ID_AP_READY, (BswM_ModeType)SOAD_AP_BRIDGE_NOT_READY);

    #ifdef SOAD_DEBUG
        printf("######## SoAd_Init completed\n");
    #endif
}

void SoAd_DeInit(void)
{
    SoAd_SoConIdType idx;

#if (SOAD_DEV_ERROR_DETECT == STD_ON)
    if (SoAd_Initialized == FALSE)
    {
        (void)Det_ReportError(SOAD_MODULE_ID, 0U, SOAD_SID_DEINIT, SOAD_E_NOTINIT);
        return;
    }
#endif

    for (idx = 0U; idx < SOAD_MAX_SOCKET_CONNECTIONS; idx++)
    {
        if ((SoAd_SoConStates[idx].IsAllocated == TRUE) &&
            (SoAd_SoConStates[idx].SocketId != TCPIP_SOCKET_ID_INVALID))
        {
            (void)TcpIp_Close(SoAd_SoConStates[idx].SocketId, TRUE);
        }
        SoAd_SoConStates[idx].SocketId = TCPIP_SOCKET_ID_INVALID;
        SoAd_SoConStates[idx].Mode = SOAD_SOCON_OFFLINE;
        SoAd_SoConStates[idx].IsAllocated = FALSE;
    }

    SoAd_ApBridgeState = SOAD_AP_BRIDGE_NOT_READY;
    SoAd_ApBridgeExternalControl = FALSE;
    (void)BswM_RequestMode(BSWM_REQUESTER_ID_AP_READY, (BswM_ModeType)SOAD_AP_BRIDGE_NOT_READY);
    SoAd_Initialized = FALSE;
}

/****************************************************
 *          * Function Info *                       *
 *                                                  *
 * Function_Name        : SoAd_IfTransmit           *
 * Function_Descripton  : Requests transmission     *
 *              of an IF PDU via TcpIp              *
 ***************************************************/
Std_ReturnType SoAd_IfTransmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)
{
    #ifdef SOAD_DEBUG
        printf("######## in SoAd_IfTransmit \n");
    #endif

    Std_ReturnType result = E_OK;
    const SoAd_PduRouteConfigType* RouteCfgPtr = NULL;

#if (SOAD_DEV_ERROR_DETECT == STD_ON)
    if (SoAd_Initialized == FALSE)
    {
        (void)Det_ReportError(SOAD_MODULE_ID, 0U, SOAD_SID_IF_TRANSMIT, SOAD_E_NOTINIT);
        return E_NOT_OK;
    }
    if (PduInfoPtr == NULL)
    {
        (void)Det_ReportError(SOAD_MODULE_ID, 0U, SOAD_SID_IF_TRANSMIT, SOAD_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    if (TxPduId >= SECOC_NUM_OF_TX_PDU_PROCESSING)
    {
        (void)Det_ReportError(SOAD_MODULE_ID, 0U, SOAD_SID_IF_TRANSMIT, SOAD_E_INV_PDUID);
        return E_NOT_OK;
    }
#else
    if ((PduInfoPtr == NULL) || (TxPduId >= SECOC_NUM_OF_TX_PDU_PROCESSING))
    {
        return E_NOT_OK;
    }
#endif

    if ((PduInfoPtr->SduLength > 0U) && (PduInfoPtr->SduDataPtr == NULL))
    {
        return E_NOT_OK;
    }

#if (SOAD_TCPIP_PAYLOAD_BACKEND == SOAD_TCPIP_PAYLOAD_BACKEND_SOCKETS)
    {
        SoAd_SoConIdType soConId = SoAd_FindOrCreateSoCon(TxPduId);
        if (soConId >= SOAD_MAX_SOCKET_CONNECTIONS)
        {
            return E_NOT_OK;
        }
    }
#endif
    SoAd_UpdateApReadinessStatus();
    if (SoAd_GetPduRouteConfig(TxPduId, &RouteCfgPtr) != E_OK)
    {
        return E_NOT_OK;
    }

    if (SoAd_IsRoutingAllowedForPdu(TxPduId) == FALSE)
    {
        return E_NOT_OK;
    }

    #ifdef SOAD_DEBUG
        printf("Secure PDU -->\n");
        {
            PduLengthType i;
            for (i = 0U; i < PduInfoPtr->SduLength; i++)
            {
                printf("%d ", PduInfoPtr->SduDataPtr[i]);
            }
            printf("\n");
        }
    #endif

    /* Transmit through the selected payload backend. */
#if (SOAD_TCPIP_PAYLOAD_BACKEND == SOAD_TCPIP_PAYLOAD_BACKEND_ETHIF)
    result = SoAd_EthIfTransmit(TxPduId, PduInfoPtr->SduDataPtr, (uint16)PduInfoPtr->SduLength);
#else
    result = SoAd_TcpIpTransmit(TxPduId, PduInfoPtr->SduDataPtr, (uint16)PduInfoPtr->SduLength);
#endif

    ApBridge_ReportServiceStatus((result == E_OK) ? TRUE : FALSE);

    /* Confirmation callbacks (preserved from original) */
    if (RouteCfgPtr->SoAdPduRouteType == SOAD_PDU_ROUTE_TP)
    {
        SoAdTp_TxConfirmation(TxPduId, result);
    }
    else if (RouteCfgPtr->SoAdPduRouteType == SOAD_PDU_ROUTE_IF)
    {
        PduR_SoAdIfTxConfirmation(TxPduId, result);
    }
    else
    {
        /* No action for other types */
    }

    return result;
}

Std_ReturnType SoAd_TpTransmit(PduIdType SoAdTxSduId, const PduInfoType* SoAdTxInfoPtr)
{
    #ifdef SOAD_DEBUG
        printf("######## in SoAd_TpTransmit\n");
    #endif

#if (SOAD_DEV_ERROR_DETECT == STD_ON)
    if (SoAd_Initialized == FALSE)
    {
        (void)Det_ReportError(SOAD_MODULE_ID, 0U, SOAD_SID_TP_TRANSMIT, SOAD_E_NOTINIT);
        return E_NOT_OK;
    }
    if (SoAdTxInfoPtr == NULL)
    {
        (void)Det_ReportError(SOAD_MODULE_ID, 0U, SOAD_SID_TP_TRANSMIT, SOAD_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    if (SoAdTxSduId >= SECOC_NUM_OF_TX_PDU_PROCESSING)
    {
        (void)Det_ReportError(SOAD_MODULE_ID, 0U, SOAD_SID_TP_TRANSMIT, SOAD_E_INV_PDUID);
        return E_NOT_OK;
    }
#else
    if ((SoAdTxInfoPtr == NULL) || (SoAdTxSduId >= SECOC_NUM_OF_TX_PDU_PROCESSING))
    {
        return E_NOT_OK;
    }
#endif

    if ((SoAdTxInfoPtr->SduLength > 0U) && (SoAdTxInfoPtr->SduDataPtr == NULL))
    {
#if (SOAD_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(SOAD_MODULE_ID, 0U, SOAD_SID_TP_TRANSMIT, SOAD_E_PARAM_POINTER);
#endif
        return E_NOT_OK;
    }

    SoAdTp_Buffer[SoAdTxSduId] = *SoAdTxInfoPtr;
    return E_OK;
}

Std_ReturnType SoAd_GetSoConId(PduIdType TxPduId, SoAd_SoConIdType* SoConIdPtr)
{
    SoAd_SoConIdType idx;

#if (SOAD_DEV_ERROR_DETECT == STD_ON)
    if (SoAd_Initialized == FALSE)
    {
        (void)Det_ReportError(SOAD_MODULE_ID, 0U, SOAD_SID_GET_SOCON_ID, SOAD_E_NOTINIT);
        return E_NOT_OK;
    }
    if (SoConIdPtr == NULL)
    {
        (void)Det_ReportError(SOAD_MODULE_ID, 0U, SOAD_SID_GET_SOCON_ID, SOAD_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#else
    if (SoConIdPtr == NULL)
    {
        return E_NOT_OK;
    }
#endif

    for (idx = 0U; idx < SOAD_MAX_SOCKET_CONNECTIONS; idx++)
    {
        if ((SoAd_SoConStates[idx].IsAllocated == TRUE) &&
            (SoAd_SoConStates[idx].TxPduId == TxPduId))
        {
            *SoConIdPtr = idx;
            return E_OK;
        }
    }

    return E_NOT_OK;
}

Std_ReturnType SoAd_OpenSoCon(SoAd_SoConIdType SoConId)
{
#if (SOAD_DEV_ERROR_DETECT == STD_ON)
    if (SoAd_Initialized == FALSE)
    {
        (void)Det_ReportError(SOAD_MODULE_ID, 0U, SOAD_SID_OPEN_SOCON, SOAD_E_NOTINIT);
        return E_NOT_OK;
    }
    if (SoConId >= SOAD_MAX_SOCKET_CONNECTIONS)
    {
        (void)Det_ReportError(SOAD_MODULE_ID, 0U, SOAD_SID_OPEN_SOCON, SOAD_E_INV_SOCON_ID);
        return E_NOT_OK;
    }
#else
    if (SoConId >= SOAD_MAX_SOCKET_CONNECTIONS)
    {
        return E_NOT_OK;
    }
#endif

    if (SoAd_SoConStates[SoConId].IsAllocated == FALSE)
    {
        return E_NOT_OK;
    }

    if (SoAd_SoConStates[SoConId].SocketId == TCPIP_SOCKET_ID_INVALID)
    {
        if (SoAd_RecreateSoConSocket(SoConId) != E_OK)
        {
            return E_NOT_OK;
        }
    }

    SoAd_SoConStates[SoConId].Mode = SOAD_SOCON_ONLINE;
    SoAd_UpdateApReadinessStatus();

    #ifdef SOAD_DEBUG
        printf("######## SoAd_OpenSoCon: SoConId=%u now ONLINE\n", SoConId);
    #endif

    return E_OK;
}

Std_ReturnType SoAd_CloseSoCon(SoAd_SoConIdType SoConId, boolean Abort)
{
#if (SOAD_DEV_ERROR_DETECT == STD_ON)
    if (SoAd_Initialized == FALSE)
    {
        (void)Det_ReportError(SOAD_MODULE_ID, 0U, SOAD_SID_CLOSE_SOCON, SOAD_E_NOTINIT);
        return E_NOT_OK;
    }
    if (SoConId >= SOAD_MAX_SOCKET_CONNECTIONS)
    {
        (void)Det_ReportError(SOAD_MODULE_ID, 0U, SOAD_SID_CLOSE_SOCON, SOAD_E_INV_SOCON_ID);
        return E_NOT_OK;
    }
#else
    if (SoConId >= SOAD_MAX_SOCKET_CONNECTIONS)
    {
        return E_NOT_OK;
    }
#endif

    if (SoAd_SoConStates[SoConId].IsAllocated == FALSE)
    {
        return E_NOT_OK;
    }

    /* Close the underlying TcpIp socket */
    if (SoAd_SoConStates[SoConId].SocketId != TCPIP_SOCKET_ID_INVALID)
    {
        (void)TcpIp_Close(SoAd_SoConStates[SoConId].SocketId, Abort);
        SoAd_SoConStates[SoConId].SocketId = TCPIP_SOCKET_ID_INVALID;
    }

    SoAd_SoConStates[SoConId].Mode = SOAD_SOCON_OFFLINE;
    SoAd_UpdateApReadinessStatus();

    #ifdef SOAD_DEBUG
        printf("######## SoAd_CloseSoCon: SoConId=%u now OFFLINE\n", SoConId);
    #endif

    return E_OK;
}

Std_ReturnType SoAd_GetLocalAddr(
    SoAd_SoConIdType SoConId,
    TcpIp_SockAddrType* LocalAddrPtr,
    uint8* NetmaskPtr,
    TcpIp_SockAddrType* DefaultRouterPtr)
{
#if (SOAD_DEV_ERROR_DETECT == STD_ON)
    if (SoAd_Initialized == FALSE)
    {
        (void)Det_ReportError(SOAD_MODULE_ID, 0U, SOAD_SID_GET_LOCAL_ADDR, SOAD_E_NOTINIT);
        return E_NOT_OK;
    }
    if ((LocalAddrPtr == NULL) || (NetmaskPtr == NULL) || (DefaultRouterPtr == NULL))
    {
        (void)Det_ReportError(SOAD_MODULE_ID, 0U, SOAD_SID_GET_LOCAL_ADDR, SOAD_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    if (SoConId >= SOAD_MAX_SOCKET_CONNECTIONS)
    {
        (void)Det_ReportError(SOAD_MODULE_ID, 0U, SOAD_SID_GET_LOCAL_ADDR, SOAD_E_INV_SOCON_ID);
        return E_NOT_OK;
    }
#else
    if ((SoConId >= SOAD_MAX_SOCKET_CONNECTIONS) ||
        (LocalAddrPtr == NULL) || (NetmaskPtr == NULL) || (DefaultRouterPtr == NULL))
    {
        return E_NOT_OK;
    }
#endif

    if (SoAd_SoConStates[SoConId].IsAllocated == FALSE)
    {
        return E_NOT_OK;
    }

    return TcpIp_GetIpAddr(
        SoAd_SoConStates[SoConId].LocalAddrId,
        LocalAddrPtr,
        NetmaskPtr,
        DefaultRouterPtr
    );
}

Std_ReturnType SoAd_GetRemoteAddr(SoAd_SoConIdType SoConId, TcpIp_SockAddrType* IpAddrPtr)
{
#if (SOAD_DEV_ERROR_DETECT == STD_ON)
    if (SoAd_Initialized == FALSE)
    {
        (void)Det_ReportError(SOAD_MODULE_ID, 0U, SOAD_SID_GET_REMOTE_ADDR, SOAD_E_NOTINIT);
        return E_NOT_OK;
    }
    if (IpAddrPtr == NULL)
    {
        (void)Det_ReportError(SOAD_MODULE_ID, 0U, SOAD_SID_GET_REMOTE_ADDR, SOAD_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    if (SoConId >= SOAD_MAX_SOCKET_CONNECTIONS)
    {
        (void)Det_ReportError(SOAD_MODULE_ID, 0U, SOAD_SID_GET_REMOTE_ADDR, SOAD_E_INV_SOCON_ID);
        return E_NOT_OK;
    }
#else
    if ((SoConId >= SOAD_MAX_SOCKET_CONNECTIONS) || (IpAddrPtr == NULL))
    {
        return E_NOT_OK;
    }
#endif

    if (SoAd_SoConStates[SoConId].IsAllocated == FALSE)
    {
        return E_NOT_OK;
    }

    *IpAddrPtr = SoAd_SoConStates[SoConId].RemoteAddr;
    return E_OK;
}

Std_ReturnType SoAd_SetRemoteAddr(SoAd_SoConIdType SoConId, const TcpIp_SockAddrType* RemoteAddrPtr)
{
#if (SOAD_DEV_ERROR_DETECT == STD_ON)
    if (SoAd_Initialized == FALSE)
    {
        (void)Det_ReportError(SOAD_MODULE_ID, 0U, SOAD_SID_SET_REMOTE_ADDR, SOAD_E_NOTINIT);
        return E_NOT_OK;
    }
    if (RemoteAddrPtr == NULL)
    {
        (void)Det_ReportError(SOAD_MODULE_ID, 0U, SOAD_SID_SET_REMOTE_ADDR, SOAD_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    if (SoConId >= SOAD_MAX_SOCKET_CONNECTIONS)
    {
        (void)Det_ReportError(SOAD_MODULE_ID, 0U, SOAD_SID_SET_REMOTE_ADDR, SOAD_E_INV_SOCON_ID);
        return E_NOT_OK;
    }
#else
    if ((SoConId >= SOAD_MAX_SOCKET_CONNECTIONS) || (RemoteAddrPtr == NULL))
    {
        return E_NOT_OK;
    }
#endif

    if (SoAd_SoConStates[SoConId].IsAllocated == FALSE)
    {
        return E_NOT_OK;
    }

    SoAd_SoConStates[SoConId].RemoteAddr = *RemoteAddrPtr;
    return E_OK;
}

Std_ReturnType SoAd_EnableRouting(SoAd_RoutingGroupIdType RoutingGroupId)
{
#if (SOAD_DEV_ERROR_DETECT == STD_ON)
    if (SoAd_Initialized == FALSE)
    {
        (void)Det_ReportError(SOAD_MODULE_ID, 0U, SOAD_SID_ENABLE_ROUTING, SOAD_E_NOTINIT);
        return E_NOT_OK;
    }
    if (RoutingGroupId >= SOAD_MAX_ROUTING_GROUPS)
    {
        (void)Det_ReportError(SOAD_MODULE_ID, 0U, SOAD_SID_ENABLE_ROUTING, SOAD_E_INV_ROUTING_GROUP_ID);
        return E_NOT_OK;
    }
#else
    if (RoutingGroupId >= SOAD_MAX_ROUTING_GROUPS)
    {
        return E_NOT_OK;
    }
#endif

    SoAd_RoutingGroupStates[RoutingGroupId].Enabled = TRUE;
    return E_OK;
}

Std_ReturnType SoAd_DisableRouting(SoAd_RoutingGroupIdType RoutingGroupId)
{
#if (SOAD_DEV_ERROR_DETECT == STD_ON)
    if (SoAd_Initialized == FALSE)
    {
        (void)Det_ReportError(SOAD_MODULE_ID, 0U, SOAD_SID_DISABLE_ROUTING, SOAD_E_NOTINIT);
        return E_NOT_OK;
    }
    if (RoutingGroupId >= SOAD_MAX_ROUTING_GROUPS)
    {
        (void)Det_ReportError(SOAD_MODULE_ID, 0U, SOAD_SID_DISABLE_ROUTING, SOAD_E_INV_ROUTING_GROUP_ID);
        return E_NOT_OK;
    }
#else
    if (RoutingGroupId >= SOAD_MAX_ROUTING_GROUPS)
    {
        return E_NOT_OK;
    }
#endif

    SoAd_RoutingGroupStates[RoutingGroupId].Enabled = FALSE;
    return E_OK;
}

void SoAd_GetVersionInfo(Std_VersionInfoType* VersionInfoPtr)
{
#if (SOAD_DEV_ERROR_DETECT == STD_ON)
    if (VersionInfoPtr == NULL)
    {
        (void)Det_ReportError(SOAD_MODULE_ID, 0U, SOAD_SID_GET_VERSION_INFO, SOAD_E_PARAM_POINTER);
        return;
    }
#else
    if (VersionInfoPtr == NULL)
    {
        return;
    }
#endif

    VersionInfoPtr->vendorID         = SOAD_VENDOR_ID;
    VersionInfoPtr->moduleID         = SOAD_MODULE_ID;
    VersionInfoPtr->sw_major_version = SOAD_SW_MAJOR_VERSION;
    VersionInfoPtr->sw_minor_version = SOAD_SW_MINOR_VERSION;
    VersionInfoPtr->sw_patch_version = SOAD_SW_PATCH_VERSION;
}

void SoAd_RxIndication(
    TcpIp_SocketIdType SocketId,
    const TcpIp_SockAddrType* RemoteAddrPtr,
    const uint8* BufPtr,
    uint16 Length
)
{
    PduIdType rxPduId = 0U;
    SoAd_SoConIdType soConId = 0U;
    PduInfoType pduInfo;
    const SoAd_PduRouteConfigType* RouteCfgPtr = NULL;

#if (SOAD_DEV_ERROR_DETECT == STD_ON)
    if (SoAd_Initialized == FALSE)
    {
        (void)Det_ReportError(SOAD_MODULE_ID, 0U, SOAD_SID_RX_INDICATION, SOAD_E_NOTINIT);
        return;
    }
    if ((RemoteAddrPtr == NULL) || (BufPtr == NULL))
    {
        (void)Det_ReportError(SOAD_MODULE_ID, 0U, SOAD_SID_RX_INDICATION, SOAD_E_PARAM_POINTER);
        return;
    }
#else
    if ((RemoteAddrPtr == NULL) || (BufPtr == NULL))
    {
        return;
    }
#endif

    if ((Length == 0U) || (Length > (uint16)SECOC_SECPDU_MAX_LENGTH))
    {
        return;
    }

    if (SoAd_PQC_HandleControlMessage(BufPtr, Length) == TRUE)
    {
        return;
    }

    if ((SoAd_UnauthApControlEnabled == TRUE) &&
        (SoAd_IsApControlPdu(BufPtr, Length) == TRUE))
    {
        SoAd_HandleApControlPdu(BufPtr);
        return;
    }

    if (SoAd_FindRxPduIdBySocket(SocketId, &rxPduId, &soConId) != E_OK)
    {
        return;
    }

    if (rxPduId >= SECOC_NUM_OF_RX_PDU_PROCESSING)
    {
        return;
    }
    if (SoAd_IsRoutingAllowedForPdu(rxPduId) == FALSE)
    {
        return;
    }
    if (SoAd_GetPduRouteConfig(rxPduId, &RouteCfgPtr) != E_OK)
    {
        return;
    }

    /* Keep latest remote peer associated with the socket connection. */
    SoAd_SoConStates[soConId].RemoteAddr = *RemoteAddrPtr;

    pduInfo.SduDataPtr = (uint8*)BufPtr;
    pduInfo.MetaDataPtr = NULL;
    pduInfo.SduLength = (PduLengthType)Length;

    if (RouteCfgPtr->SoAdPduRouteType == SOAD_PDU_ROUTE_TP)
    {
        SoAdTp_RxIndication(rxPduId, &pduInfo);
    }
    else if (RouteCfgPtr->SoAdPduRouteType == SOAD_PDU_ROUTE_IF)
    {
        PduR_SoAdIfRxIndication(rxPduId, &pduInfo);
    }
    else
    {
        /* Ignore non-SoAd routed types. */
    }
}

void SoAdTp_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{
    #ifdef SOAD_DEBUG
        printf("######## in SoAdTp_RxIndication\n");
    #endif

    if ((PduInfoPtr == NULL) || (RxPduId >= SECOC_NUM_OF_RX_PDU_PROCESSING))
    {
        return;
    }
    if ((PduInfoPtr->SduLength > 0U) && (PduInfoPtr->SduDataPtr == NULL))
    {
        return;
    }

    const SoAd_PduRouteConfigType* RouteCfgPtr = NULL;
    uint8 AuthHeadlen;

    if (SoAd_GetPduRouteConfig(RxPduId, &RouteCfgPtr) != E_OK)
    {
        return;
    }
    if (RouteCfgPtr->SoAdPduRouteType != SOAD_PDU_ROUTE_TP)
    {
        return;
    }

    if (PduInfoPtr->SduLength > (PduLengthType)SECOC_SECPDU_MAX_LENGTH)
    {
        SoAd_ResetTpReceptionState(RxPduId);
        return;
    }

    if (SoAdTp_RxQueueCount[RxPduId] >= SOAD_TP_RX_QUEUE_DEPTH)
    {
        SoAd_ResetTpReceptionState(RxPduId);
        return;
    }

    {
        uint8 TailIndex = SoAdTp_RxQueueTail[RxPduId];
        (void)memcpy(SoAdTp_RxQueueData[RxPduId][TailIndex], PduInfoPtr->SduDataPtr, PduInfoPtr->SduLength);
        SoAdTp_RxQueue[RxPduId][TailIndex].SduDataPtr = SoAdTp_RxQueueData[RxPduId][TailIndex];
        SoAdTp_RxQueue[RxPduId][TailIndex].MetaDataPtr = NULL;
        SoAdTp_RxQueue[RxPduId][TailIndex].SduLength = PduInfoPtr->SduLength;

        SoAdTp_RxQueueTail[RxPduId] = (uint8)((TailIndex + 1U) % SOAD_TP_RX_QUEUE_DEPTH);
        SoAdTp_RxQueueCount[RxPduId]++;
    }

    /*
     * First frame initializes expected secure length:
     * header + authentic payload + freshness + authenticator/signature.
     */
    if (SoAdTp_Recieve_Counter[RxPduId] == 0U)
    {
        PduLengthType authenticLength = 0U;
        PduLengthType authInfoLength;
        AuthHeadlen = RouteCfgPtr->SoAdAuthPduHeaderLength;

        if ((AuthHeadlen > 0U) &&
            ((AuthHeadlen > PduInfoPtr->SduLength) || (AuthHeadlen > (uint8)sizeof(PduLengthType))))
        {
            SoAd_ResetTpReceptionState(RxPduId);
            return;
        }

        if (AuthHeadlen > 0U)
        {
            (void)memcpy((uint8*)&authenticLength, PduInfoPtr->SduDataPtr, AuthHeadlen);
            if (authenticLength > (PduLengthType)SECOC_AUTHPDU_MAX_LENGTH)
            {
                SoAd_ResetTpReceptionState(RxPduId);
                return;
            }
        }
        else
        {
            authenticLength = RouteCfgPtr->SoAdAuthenticPduLength;
        }

#if (SECOC_USE_PQC_MODE == TRUE)
        authInfoLength = (PduLengthType)PQC_MLDSA_SIGNATURE_BYTES;
#else
        authInfoLength = BIT_TO_BYTES(RouteCfgPtr->SoAdAuthInfoLengthBits);
#endif

        SoAdTp_secureLength_Recieve[RxPduId] = (PduLengthType)AuthHeadlen
            + authenticLength
            + BIT_TO_BYTES(RouteCfgPtr->SoAdFreshnessValueTruncLength)
            + authInfoLength;

        if (SoAdTp_secureLength_Recieve[RxPduId] > (PduLengthType)SECOC_SECPDU_MAX_LENGTH)
        {
            SoAd_ResetTpReceptionState(RxPduId);
            return;
        }
    }

    SoAdTp_Recieve_Counter[RxPduId]++;
    SoAdTp_ReceivedBytes[RxPduId] = (PduLengthType)(SoAdTp_ReceivedBytes[RxPduId] + PduInfoPtr->SduLength);

    if (SoAdTp_ReceivedBytes[RxPduId] > SoAdTp_secureLength_Recieve[RxPduId])
    {
        SoAd_ResetTpReceptionState(RxPduId);
    }
}

void SoAd_MainFunctionTx(void)
{
    #ifdef SOAD_DEBUG
        printf("######## in SoAd_MainFunctionTx\n");
    #endif

    uint8 sdata[BUS_LENGTH] = {0};
    uint8 mdata[BUS_LENGTH] = {0};
    PduLengthType length = BUS_LENGTH;
    PduInfoType info = {sdata, mdata, length};

    TpDataStateType retrystate = TP_DATACONF;
    PduLengthType retrycout = BUS_LENGTH;
    RetryInfoType retry = {retrystate, retrycout};

    PduLengthType availableDataPtr = 0U;
    PduIdType TxPduId;

    for (TxPduId = 0U; TxPduId < SECOC_NUM_OF_TX_PDU_PROCESSING; TxPduId++)
    {
        if (SoAdTp_Buffer[TxPduId].SduLength > 0U)
        {
            uint8 lastFrameIndex = (uint8)(((SoAdTp_Buffer[TxPduId].SduLength % BUS_LENGTH) == 0U)
                ? (SoAdTp_Buffer[TxPduId].SduLength / BUS_LENGTH)
                : ((SoAdTp_Buffer[TxPduId].SduLength / BUS_LENGTH) + 1U));

            #ifdef SOAD_DEBUG
                printf("Start sending id = %d\n", TxPduId);
                printf("PDU length = %ld\n", SoAdTp_Buffer[TxPduId].SduLength);
                printf("All Data to be Sent: \n");
                {
                    PduLengthType i;
                    for (i = 0U; i < SoAdTp_Buffer[TxPduId].SduLength; i++)
                    {
                        printf("%d  ", SoAdTp_Buffer[TxPduId].SduDataPtr[i]);
                    }
                    printf("\n\n\n");
                }
            #endif

            int frameIndex;
            uint8 retryBudget = 3U;
            boolean txFailed = FALSE;
            for (frameIndex = 0; frameIndex < lastFrameIndex; frameIndex++)
            {
                if (frameIndex == (lastFrameIndex - 1))
                {
                    info.SduLength = ((SoAdTp_Buffer[TxPduId].SduLength % BUS_LENGTH) == 0U)
                        ? BUS_LENGTH
                        : (SoAdTp_Buffer[TxPduId].SduLength % BUS_LENGTH);

                    #ifdef SOAD_DEBUG
                        printf("last frame PDU length = %ld\n", SoAdTp_Buffer[TxPduId].SduLength);
                        printf("All Data to be Sent: \n");
                        {
                            PduLengthType i;
                            for (i = 0U; i < info.SduLength; i++)
                            {
                                printf("%d  ", info.SduDataPtr[i]);
                            }
                            printf("\n");
                        }
                    #endif
                }

                BufReq_ReturnType resultCopy = PduR_SoAdTpCopyTxData(TxPduId, &info, &retry, &availableDataPtr);
                Std_ReturnType resultTransmit = SoAd_IfTransmit(TxPduId, &info);

                if ((resultTransmit != E_OK) || (resultCopy != BUFREQ_OK))
                {
                    if (retryBudget == 0U)
                    {
                        txFailed = TRUE;
                        break;
                    }
                    retryBudget--;
                    retry.TpDataState = TP_DATARETRY;
                    frameIndex--; /* DEVIATION: Rule 14.2 - loop counter adjusted for TP segmentation */ // cppcheck-suppress misra-c2012-14.2
                }
                else
                {
                    retry.TpDataState = TP_DATACONF;
                    retryBudget = 3U;
                }

                #ifdef SOAD_DEBUG
                    printf("Transmit Result = %d\n", resultTransmit);
                #endif
            }

            if (txFailed == FALSE)
            {
                PduR_SoAdTpTxConfirmation(TxPduId, E_OK);
            }
            else
            {
                PduR_SoAdTpTxConfirmation(TxPduId, E_NOT_OK);
            }
            SoAdTp_Buffer[TxPduId].SduLength = 0U;
        }
    }

    SoAd_UpdateApReadinessStatus();
}

void SoAdTp_TxConfirmation(PduIdType TxPduId, Std_ReturnType result)
{
    (void)TxPduId;
    (void)result;

    #ifdef SOAD_DEBUG
        printf("######## in SoAd_TxConfirmation \n");
    #endif
}

void SoAd_MainFunctionRx(void)
{
    #ifdef SOAD_DEBUG
        printf("######## in SoAd_MainFunctionRx\n");
    #endif

    /* Poll backend for incoming data. */
#if (SOAD_TCPIP_PAYLOAD_BACKEND == SOAD_TCPIP_PAYLOAD_BACKEND_ETHIF)
    /* EthIf polling is handled by EcuM main path; keep SoAd side callback-driven. */
#else
    TcpIp_MainFunction();
#endif

    PduIdType RxPduId;
    for (RxPduId = 0U; RxPduId < SECOC_NUM_OF_RX_PDU_PROCESSING; RxPduId++)
    {
        BufReq_ReturnType result = BUFREQ_OK;
        while ((SoAdTp_RxQueueCount[RxPduId] > 0U) && (SoAdTp_Recieve_Counter[RxPduId] > 0U))
        {
            uint8 HeadIndex = SoAdTp_RxQueueHead[RxPduId];
            PduLengthType RemainingBytes;
            boolean IsLastFrame;
            PduLengthType bufferSizePtr;

            SoAdTp_Buffer_Rx[RxPduId] = SoAdTp_RxQueue[RxPduId][HeadIndex];
            SoAdTp_RxQueueHead[RxPduId] = (uint8)((HeadIndex + 1U) % SOAD_TP_RX_QUEUE_DEPTH);
            SoAdTp_RxQueueCount[RxPduId]--;
            SoAdTp_Processed_Counter[RxPduId]++;

            if (SoAdTp_ProcessedBytes[RxPduId] >= SoAdTp_secureLength_Recieve[RxPduId])
            {
                SoAd_ResetTpReceptionState(RxPduId);
                break;
            }

            RemainingBytes = (PduLengthType)(SoAdTp_secureLength_Recieve[RxPduId] - SoAdTp_ProcessedBytes[RxPduId]);
            if (SoAdTp_Buffer_Rx[RxPduId].SduLength > RemainingBytes)
            {
                SoAd_ResetTpReceptionState(RxPduId);
                break;
            }
            IsLastFrame = (boolean)(SoAdTp_Buffer_Rx[RxPduId].SduLength == RemainingBytes);

            #ifdef SOAD_DEBUG
                printf("######## in main Soad Rx  in id : %d\n", RxPduId);
                printf("for id %d :", RxPduId);
                {
                    PduLengthType l;
                    for (l = 0U; l < SoAdTp_Buffer_Rx[RxPduId].SduLength; l++)
                    {
                        printf("%d ", SoAdTp_Buffer_Rx[RxPduId].SduDataPtr[l]);
                    }
                    printf("\n");
                }
            #endif

            if (SoAdTp_Processed_Counter[RxPduId] == 1U)
            {
                result = PduR_SoAdStartOfReception(RxPduId, &SoAdTp_Buffer_Rx[RxPduId],
                    SoAdTp_secureLength_Recieve[RxPduId], &bufferSizePtr);
                if (result == BUFREQ_OK)
                {
                    result = PduR_SoAdTpCopyRxData(RxPduId, &SoAdTp_Buffer_Rx[RxPduId], &bufferSizePtr);
                }
                else
                {
                    SoAd_ResetTpReceptionState(RxPduId);
                }
            }
            else
            {
                result = PduR_SoAdTpCopyRxData(RxPduId, &SoAdTp_Buffer_Rx[RxPduId], &bufferSizePtr);
            }

            SoAdTp_ProcessedBytes[RxPduId] = (PduLengthType)(SoAdTp_ProcessedBytes[RxPduId] + SoAdTp_Buffer_Rx[RxPduId].SduLength);
            SoAdTp_Buffer_Rx[RxPduId].SduLength = 0U;

            if ((result != BUFREQ_OK) ||
                (SoAdTp_ProcessedBytes[RxPduId] > SoAdTp_secureLength_Recieve[RxPduId]))
            {
                PduR_SoAdTpRxIndication(RxPduId, E_NOT_OK);
                SoAd_ResetTpReceptionState(RxPduId);
                ApBridge_ReportServiceStatus(FALSE);
                break;
            }

            if (IsLastFrame == TRUE)
            {
                PduR_SoAdTpRxIndication(RxPduId, E_OK);
                SoAd_ResetTpReceptionState(RxPduId);
            }

            ApBridge_ReportServiceStatus((result == BUFREQ_OK) ? TRUE : FALSE);

            #ifdef SCHEDULER_ON
                pthread_mutex_unlock(&lock);
            #endif
        }
    }

    SoAd_UpdateApReadinessStatus();
}

Std_ReturnType SoAd_SetApBridgeState(SoAd_ApBridgeStateType State)
{
#if (SOAD_DEV_ERROR_DETECT == STD_ON)
    if (SoAd_Initialized == FALSE)
    {
        (void)Det_ReportError(SOAD_MODULE_ID, 0U, SOAD_SID_SET_AP_BRIDGE_STATE, SOAD_E_NOTINIT);
        return E_NOT_OK;
    }
#endif

    if (State > SOAD_AP_BRIDGE_DEGRADED)
    {
        return E_NOT_OK;
    }

    SoAd_ApBridgeExternalControl = TRUE;

    if (SoAd_ApBridgeState != State)
    {
        SoAd_ApBridgeState = State;
        (void)BswM_RequestMode(BSWM_REQUESTER_ID_AP_READY, (BswM_ModeType)State);
    }

    return E_OK;
}
