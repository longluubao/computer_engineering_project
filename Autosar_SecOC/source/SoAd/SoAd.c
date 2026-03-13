/**
 * @file SoAd.c
 * @brief AUTOSAR Socket Adapter Module Implementation
 * @details Routes PDU transmissions through TcpIp module instead of raw ethernet.
 *          Manages socket connections and routing groups per AUTOSAR SWS_SoAd.
 */

/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "SoAd.h"
#include "PduR_SoAd.h"
#include "SecOC.h"
#include "Std_Types.h"
#include "SecOC_Debug.h"
#include "SecOC_Cfg.h"
#include "TcpIp.h"
#include "Det.h"
#if (SOAD_TCPIP_PAYLOAD_BACKEND == SOAD_TCPIP_PAYLOAD_BACKEND_ETHIF)
    #include "EthIf.h"
#endif

#ifdef SCHEDULER_ON
    #include <pthread.h>
#endif

#include <string.h>

#if (SOAD_TCPIP_PAYLOAD_BACKEND == SOAD_TCPIP_PAYLOAD_BACKEND_ETHIF)
#define SOAD_ETHIF_CTRL_IDX                  ((uint8)0U)
#define SOAD_ETHIF_FRAME_TYPE_BASE           ((Eth_FrameType)0x9000U)
#endif

/********************************************************************************************************/
/******************************************GlobalVaribles************************************************/
/********************************************************************************************************/

static boolean SoAd_Initialized = FALSE;

/* TP buffers (preserved from original) */
static PduInfoType SoAdTp_Buffer[SOAD_BUFFERLENGTH];
static PduInfoType SoAdTp_Buffer_Rx[SECOC_NUM_OF_RX_PDU_PROCESSING];
static uint8 SoAdTp_Recieve_Counter[SECOC_NUM_OF_RX_PDU_PROCESSING] = {0};
static PduLengthType SoAdTp_secureLength_Recieve[SECOC_NUM_OF_RX_PDU_PROCESSING] = {0};

/* Socket connection table */
static SoAd_SoConStateType SoAd_SoConStates[SOAD_MAX_SOCKET_CONNECTIONS];

/* Routing group table */
static SoAd_RoutingGroupStateType SoAd_RoutingGroupStates[SOAD_MAX_ROUTING_GROUPS];

extern const SecOC_RxPduProcessingType     *SecOCRxPduProcessing;
extern SecOC_PduCollection PdusCollections[];

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

    /* Search for existing mapping */
    for (idx = 0U; idx < SOAD_MAX_SOCKET_CONNECTIONS; idx++)
    {
        if ((SoAd_SoConStates[idx].IsAllocated == TRUE) &&
            (SoAd_SoConStates[idx].TxPduId == TxPduId))
        {
            return idx;
        }
    }

    /* Auto-allocate a connection for this PDU (default UDP, port = 50000 + TxPduId) */
    for (idx = 0U; idx < SOAD_MAX_SOCKET_CONNECTIONS; idx++)
    {
        if (SoAd_SoConStates[idx].IsAllocated == FALSE)
        {
            TcpIp_SocketIdType sockId = TCPIP_SOCKET_ID_INVALID;
            Std_ReturnType res;

            res = TcpIp_GetSocketId(TCPIP_AF_INET, TCPIP_IPPROTO_UDP, &sockId);
            if (res != E_OK)
            {
                return SOAD_MAX_SOCKET_CONNECTIONS; /* invalid sentinel */
            }

            SoAd_SoConStates[idx].SocketId    = sockId;
            SoAd_SoConStates[idx].Protocol     = TCPIP_IPPROTO_UDP;
            SoAd_SoConStates[idx].LocalAddrId  = 0U;
            SoAd_SoConStates[idx].LocalPort    = (uint16)(50000U + TxPduId);
            SoAd_SoConStates[idx].Mode         = SOAD_SOCON_ONLINE;
            SoAd_SoConStates[idx].IsAllocated  = TRUE;
            SoAd_SoConStates[idx].TxPduId      = TxPduId;

            /* Set default remote: 127.0.0.1 with port based on PDU */
            SoAd_SoConStates[idx].RemoteAddr.domain  = TCPIP_AF_INET;
            SoAd_SoConStates[idx].RemoteAddr.addr[0] = 127U;
            SoAd_SoConStates[idx].RemoteAddr.addr[1] = 0U;
            SoAd_SoConStates[idx].RemoteAddr.addr[2] = 0U;
            SoAd_SoConStates[idx].RemoteAddr.addr[3] = 1U;
            SoAd_SoConStates[idx].RemoteAddr.port    = (uint16)(50000U + TxPduId);

            /* Bind the socket */
            {
                uint16 port = SoAd_SoConStates[idx].LocalPort;
                (void)TcpIp_Bind(sockId, SoAd_SoConStates[idx].LocalAddrId, &port);
                SoAd_SoConStates[idx].LocalPort = port;
            }

            return idx;
        }
    }

    return SOAD_MAX_SOCKET_CONNECTIONS; /* no free slot */
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
    pduInfo.SduDataPtr = (uint8*)DataPtr;
    pduInfo.MetaDataPtr = NULL;
    pduInfo.SduLength = (PduLengthType)LenByte;

    if (PdusCollections[RxPduId].Type == SECOC_SECURED_PDU_SOADTP)
    {
        SoAdTp_RxIndication(RxPduId, &pduInfo);
    }
    else if (PdusCollections[RxPduId].Type == SECOC_SECURED_PDU_SOADIF)
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

/********************************************************************************************************/
/********************************************Functions***************************************************/
/********************************************************************************************************/

void SoAd_Init(const SoAd_ConfigType* ConfigPtr)
{
    uint16 idx;

    (void)ConfigPtr;

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

    /* Initialize routing groups (all enabled by default) */
    for (idx = 0U; idx < SOAD_MAX_ROUTING_GROUPS; idx++)
    {
        SoAd_RoutingGroupStates[idx].Enabled = TRUE;
    }

    /* Initialize TP buffers */
    (void)memset(SoAdTp_Buffer, 0, sizeof(SoAdTp_Buffer));
    (void)memset(SoAdTp_Buffer_Rx, 0, sizeof(SoAdTp_Buffer_Rx));
    (void)memset(SoAdTp_Recieve_Counter, 0, sizeof(SoAdTp_Recieve_Counter));
    (void)memset(SoAdTp_secureLength_Recieve, 0, sizeof(SoAdTp_secureLength_Recieve));

#if (SOAD_TCPIP_PAYLOAD_BACKEND == SOAD_TCPIP_PAYLOAD_BACKEND_ETHIF)
    EthIf_SetRxIndicationCallback(SoAd_EthIfRxIndicationCbk);
#endif

    SoAd_Initialized = TRUE;

    #ifdef SOAD_DEBUG
        printf("######## SoAd_Init completed\n");
    #endif
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

    /* Confirmation callbacks (preserved from original) */
    if (PdusCollections[TxPduId].Type == SECOC_SECURED_PDU_SOADTP)
    {
        SoAdTp_TxConfirmation(TxPduId, result);
    }
    else if (PdusCollections[TxPduId].Type == SECOC_SECURED_PDU_SOADIF)
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

    SoAd_SoConStates[SoConId].Mode = SOAD_SOCON_ONLINE;

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

    if (SoAd_FindRxPduIdBySocket(SocketId, &rxPduId, &soConId) != E_OK)
    {
        return;
    }

    if (rxPduId >= SECOC_NUM_OF_RX_PDU_PROCESSING)
    {
        return;
    }

    /* Keep latest remote peer associated with the socket connection. */
    SoAd_SoConStates[soConId].RemoteAddr = *RemoteAddrPtr;

    pduInfo.SduDataPtr = (uint8*)BufPtr;
    pduInfo.MetaDataPtr = NULL;
    pduInfo.SduLength = (PduLengthType)Length;

    if (PdusCollections[rxPduId].Type == SECOC_SECURED_PDU_SOADTP)
    {
        SoAdTp_RxIndication(rxPduId, &pduInfo);
    }
    else if (PdusCollections[rxPduId].Type == SECOC_SECURED_PDU_SOADIF)
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

    /* Copy to SoAdTp buffer */
    SoAdTp_Buffer_Rx[RxPduId] = *PduInfoPtr;

    /*
     * Check if first frame:
     *   If there is a header -> get auth length from the frame
     *   Else -> get the config length of data
     * Then add Freshness, Mac and Header length for the whole Secure Frame Length
     */
    if (SoAdTp_Recieve_Counter[RxPduId] == 0U)
    {
        uint8 AuthHeadlen = SecOCRxPduProcessing[RxPduId].SecOCRxSecuredPduLayer->SecOCRxSecuredPdu->SecOCAuthPduHeaderLength;
        PduLengthType SecureDataframe = (PduLengthType)AuthHeadlen
            + BIT_TO_BYTES(SecOCRxPduProcessing[RxPduId].SecOCFreshnessValueTruncLength)
            + BIT_TO_BYTES(SecOCRxPduProcessing[RxPduId].SecOCAuthInfoTruncLength);
        if ((AuthHeadlen > 0U) &&
            ((AuthHeadlen > PduInfoPtr->SduLength) || (AuthHeadlen > (uint8)sizeof(PduLengthType))))
        {
            SoAdTp_Recieve_Counter[RxPduId] = 0U;
            return;
        }

        if (AuthHeadlen > 0U)
        {
            (void)memcpy((uint8*)&SoAdTp_secureLength_Recieve[RxPduId], PduInfoPtr->SduDataPtr, AuthHeadlen);
        }
        else
        {
            SoAdTp_secureLength_Recieve[RxPduId] = SecOCRxPduProcessing[RxPduId].SecOCRxAuthenticPduLayer->SecOCRxAuthenticLayerPduRef.SduLength;
        }
        SoAdTp_secureLength_Recieve[RxPduId] += SecureDataframe;
    }
    SoAdTp_Recieve_Counter[RxPduId]++;
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
            uint8 lastFrameIndex = (uint8)((SoAdTp_Buffer[TxPduId].SduLength % BUS_LENGTH == 0U)
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
            for (frameIndex = 0; frameIndex < lastFrameIndex; frameIndex++)
            {
                if (frameIndex == (lastFrameIndex - 1))
                {
                    info.SduLength = (SoAdTp_Buffer[TxPduId].SduLength % BUS_LENGTH == 0U)
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
                    retry.TpDataState = TP_DATARETRY;
                    frameIndex--;
                }
                else
                {
                    retry.TpDataState = TP_DATACONF;
                }

                #ifdef SOAD_DEBUG
                    printf("Transmit Result = %d\n", resultTransmit);
                #endif
            }

            PduR_SoAdTpTxConfirmation(TxPduId, E_OK);
            SoAdTp_Buffer[TxPduId].SduLength = 0U;
        }
    }
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
        if ((SoAdTp_Recieve_Counter[RxPduId] > 0U) && (SoAdTp_Buffer_Rx[RxPduId].SduLength > 0U))
        {
            uint8 lastFrameIndex = (uint8)((SoAdTp_secureLength_Recieve[RxPduId] % BUS_LENGTH == 0U)
                ? (SoAdTp_secureLength_Recieve[RxPduId] / BUS_LENGTH)
                : ((SoAdTp_secureLength_Recieve[RxPduId] / BUS_LENGTH) + 1U));

            PduLengthType bufferSizePtr;

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

            if (SoAdTp_Recieve_Counter[RxPduId] == 1U)
            {
                result = PduR_SoAdStartOfReception(RxPduId, &SoAdTp_Buffer_Rx[RxPduId],
                    SoAdTp_secureLength_Recieve[RxPduId], &bufferSizePtr);
                if (result == BUFREQ_OK)
                {
                    result = PduR_SoAdTpCopyRxData(RxPduId, &SoAdTp_Buffer_Rx[RxPduId], &bufferSizePtr);
                }
                else
                {
                    SoAdTp_Recieve_Counter[RxPduId] = 0U;
                }
                SoAdTp_Buffer_Rx[RxPduId].SduLength = 0U;
            }
            else if (SoAdTp_Recieve_Counter[RxPduId] == lastFrameIndex)
            {
                SoAdTp_Buffer_Rx[RxPduId].SduLength = (SoAdTp_secureLength_Recieve[RxPduId] % BUS_LENGTH == 0U)
                    ? BUS_LENGTH
                    : (SoAdTp_secureLength_Recieve[RxPduId] % BUS_LENGTH);
                result = PduR_SoAdTpCopyRxData(RxPduId, &SoAdTp_Buffer_Rx[RxPduId], &bufferSizePtr);
                PduR_SoAdTpRxIndication(RxPduId, result);
                SoAdTp_Recieve_Counter[RxPduId] = 0U;
            }
            else
            {
                result = PduR_SoAdTpCopyRxData(RxPduId, &SoAdTp_Buffer_Rx[RxPduId], &bufferSizePtr);
            }

            #ifdef SCHEDULER_ON
                pthread_mutex_unlock(&lock);
            #endif
        }
    }
}
