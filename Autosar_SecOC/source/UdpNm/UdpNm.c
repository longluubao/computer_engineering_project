/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "UdpNm.h"
#include "Det.h"
#include "SoAd.h"
#include "BswM.h"

/********************************************************************************************************/
/******************************************GlobalVariables************************************************/
/********************************************************************************************************/

static UdpNm_InitStateType UdpNm_InitState       = UDPNM_UNINIT;
static UdpNm_NmStateType   UdpNm_NmState         = UDPNM_STATE_BUS_SLEEP;
static UdpNm_ModeType      UdpNm_Mode            = UDPNM_MODE_BUS_SLEEP;
static boolean             UdpNm_NetworkRequested = FALSE;
static boolean             UdpNm_PassiveMode      = FALSE;

/* Timers (count in MainFunction cycles) */
static uint16 UdpNm_NmTimeoutTimer    = 0U;
static uint16 UdpNm_RepeatMsgTimer    = 0U;
static uint16 UdpNm_WaitBusSleepTimer = 0U;
static uint16 UdpNm_MsgCycleTimer     = 0U;

/*
 * Counter for fast-cycle ("immediate") NM transmissions on Network Mode entry.
 * Counts down from UDPNM_IMMEDIATE_NM_TRANSMISSIONS to 0.
 * While > 0, use UDPNM_IMMEDIATE_NM_CYCLE_TIME instead of UDPNM_MSG_CYCLE_TIME.
 */
static uint8 UdpNm_ImmediateNmTxCounter = 0U;

/* NM PDU transmit buffer: [NID][CBV][UserData...] */
static uint8   UdpNm_TxPduData[UDPNM_PDU_LENGTH];
static boolean UdpNm_MsgTxEnabled = FALSE;

/* Last received NM PDU (for GetPduData / GetNodeIdentifier) */
static uint8 UdpNm_RxPduData[UDPNM_PDU_LENGTH];

/********************************************************************************************************/
/******************************************InternalFunctions**********************************************/
/********************************************************************************************************/

static void UdpNm_TransmitNmMessage(void)
{
    PduInfoType pduInfo;

    if (UdpNm_MsgTxEnabled == FALSE)
    {
        return;
    }

    if (UdpNm_PassiveMode == TRUE)
    {
        /* Passive mode: must not send NM messages */
        return;
    }

    pduInfo.SduDataPtr  = UdpNm_TxPduData;
    pduInfo.SduLength   = (PduLengthType)UDPNM_PDU_LENGTH;
    pduInfo.MetaDataPtr = NULL;

    (void)SoAd_IfTransmit(UDPNM_TX_PDU_ID, &pduInfo);
}

static void UdpNm_EnterRepeatMessage(void)
{
    UdpNm_NmState      = UDPNM_STATE_REPEAT_MESSAGE;
    UdpNm_Mode         = UDPNM_MODE_NETWORK;
    UdpNm_RepeatMsgTimer  = 0U;
    UdpNm_NmTimeoutTimer  = 0U;     /* SWS: reset timeout on state entry */
    UdpNm_MsgCycleTimer   = 0U;
    UdpNm_MsgTxEnabled    = TRUE;

    /* Activate fast-cycle immediate transmissions */
    UdpNm_ImmediateNmTxCounter = UDPNM_IMMEDIATE_NM_TRANSMISSIONS;

    UdpNm_TxPduData[UDPNM_PDU_CBV_POSITION] |= UDPNM_CBV_REPEAT_MSG_REQUEST;

    /* SWS: first NM PDU is sent immediately on entering Network Mode */
    UdpNm_TransmitNmMessage();

    (void)BswM_RequestMode((uint16)UDPNM_MODULE_ID, (BswM_ModeType)UdpNm_Mode);
}

static void UdpNm_EnterNormalOperation(void)
{
    UdpNm_NmState        = UDPNM_STATE_NORMAL_OPERATION;
    UdpNm_Mode           = UDPNM_MODE_NETWORK;
    UdpNm_NmTimeoutTimer   = 0U;
    UdpNm_MsgCycleTimer    = 0U;
    UdpNm_ImmediateNmTxCounter = 0U;
    UdpNm_MsgTxEnabled     = TRUE;

    /* Clear RMR bit: node is no longer in Repeat Message State */
    UdpNm_TxPduData[UDPNM_PDU_CBV_POSITION] &= (uint8)(~UDPNM_CBV_REPEAT_MSG_REQUEST);

    (void)BswM_RequestMode((uint16)UDPNM_MODULE_ID, (BswM_ModeType)UdpNm_Mode);
}

static void UdpNm_EnterReadySleep(void)
{
    UdpNm_NmState        = UDPNM_STATE_READY_SLEEP;
    UdpNm_Mode           = UDPNM_MODE_NETWORK;
    UdpNm_NmTimeoutTimer   = 0U;
    UdpNm_ImmediateNmTxCounter = 0U;
    /*
     * SWS: TX continues in Ready Sleep (passive mode = FALSE path).
     * UdpNm_MsgTxEnabled remains TRUE so UdpNm_TransmitNmMessage() keeps
     * sending. Only passive-mode nodes or PrepareBusSleep stop TX.
     */
    UdpNm_MsgTxEnabled     = TRUE;

    /* Clear RMR bit */
    UdpNm_TxPduData[UDPNM_PDU_CBV_POSITION] &= (uint8)(~UDPNM_CBV_REPEAT_MSG_REQUEST);

    (void)BswM_RequestMode((uint16)UDPNM_MODULE_ID, (BswM_ModeType)UdpNm_Mode);
}

static void UdpNm_EnterPrepareBusSleep(void)
{
    UdpNm_NmState           = UDPNM_STATE_PREPARE_BUS_SLEEP;
    UdpNm_Mode              = UDPNM_MODE_PREPARE_BUS_SLEEP;
    UdpNm_WaitBusSleepTimer   = 0U;
    UdpNm_MsgTxEnabled        = FALSE;   /* TX stops in Prepare-Bus-Sleep */
    (void)BswM_RequestMode((uint16)UDPNM_MODULE_ID, (BswM_ModeType)UdpNm_Mode);
}

static void UdpNm_EnterBusSleep(void)
{
    UdpNm_NmState    = UDPNM_STATE_BUS_SLEEP;
    UdpNm_Mode       = UDPNM_MODE_BUS_SLEEP;
    UdpNm_MsgTxEnabled = FALSE;
    UdpNm_TxPduData[UDPNM_PDU_CBV_POSITION] = 0x00U;
    (void)BswM_RequestMode((uint16)UDPNM_MODULE_ID, (BswM_ModeType)UdpNm_Mode);
}

/********************************************************************************************************/
/********************************************Functions***************************************************/
/********************************************************************************************************/

void UdpNm_Init(void)
{
    uint8 i;

    UdpNm_NetworkRequested      = FALSE;
    UdpNm_PassiveMode           = FALSE;
    UdpNm_NmTimeoutTimer        = 0U;
    UdpNm_RepeatMsgTimer        = 0U;
    UdpNm_WaitBusSleepTimer     = 0U;
    UdpNm_MsgCycleTimer         = 0U;
    UdpNm_ImmediateNmTxCounter  = 0U;
    UdpNm_MsgTxEnabled          = FALSE;

    for (i = 0U; i < UDPNM_PDU_LENGTH; i++)
    {
        UdpNm_TxPduData[i] = 0x00U;
        UdpNm_RxPduData[i] = 0x00U;
    }

    /* Write local node ID into transmit PDU */
    UdpNm_TxPduData[UDPNM_PDU_NID_POSITION] = UDPNM_NODE_ID;

    UdpNm_InitState = UDPNM_INIT;
    UdpNm_EnterBusSleep();
}

void UdpNm_DeInit(void)
{
    if (UdpNm_InitState != UDPNM_INIT)
    {
        (void)Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID,
                              UDPNM_SID_DEINIT, UDPNM_E_UNINIT);
        return;
    }

    UdpNm_InitState = UDPNM_UNINIT;
    UdpNm_EnterBusSleep();
}

Std_ReturnType UdpNm_PassiveStartUp(uint8 NetworkHandle)
{
    if (UdpNm_InitState != UDPNM_INIT)
    {
        (void)Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID,
                              UDPNM_SID_PASSIVE_STARTUP, UDPNM_E_UNINIT);
        return E_NOT_OK;
    }

    if (NetworkHandle != 0U)
    {
        (void)Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID,
                              UDPNM_SID_PASSIVE_STARTUP, UDPNM_E_PARAM_NETWORK);
        return E_NOT_OK;
    }

    /*
     * SWS: PassiveStartUp only acts from Bus-Sleep / Prepare-Bus-Sleep.
     * It does NOT set NetworkRequested = TRUE; a subsequent NetworkRequest()
     * is still required to stay in Normal Operation after Repeat Message.
     */
    if ((UdpNm_NmState == UDPNM_STATE_BUS_SLEEP) ||
        (UdpNm_NmState == UDPNM_STATE_PREPARE_BUS_SLEEP))
    {
        UdpNm_PassiveMode = TRUE;
        UdpNm_EnterRepeatMessage();
    }

    return E_OK;
}

Std_ReturnType UdpNm_NetworkRequest(uint8 NetworkHandle)
{
    if (UdpNm_InitState != UDPNM_INIT)
    {
        (void)Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID,
                              UDPNM_SID_NETWORK_REQUEST, UDPNM_E_UNINIT);
        return E_NOT_OK;
    }

    if (NetworkHandle != 0U)
    {
        (void)Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID,
                              UDPNM_SID_NETWORK_REQUEST, UDPNM_E_PARAM_NETWORK);
        return E_NOT_OK;
    }

    UdpNm_NetworkRequested = TRUE;
    UdpNm_PassiveMode      = FALSE;

    switch (UdpNm_NmState)
    {
    case UDPNM_STATE_BUS_SLEEP:
    case UDPNM_STATE_PREPARE_BUS_SLEEP:
        /* Wake-up: set Active Wakeup Bit and enter Repeat Message */
        UdpNm_TxPduData[UDPNM_PDU_CBV_POSITION] |= UDPNM_CBV_ACTIVE_WAKEUP;
        UdpNm_EnterRepeatMessage();
        break;

    case UDPNM_STATE_READY_SLEEP:
        /* Return to Normal Operation */
        UdpNm_EnterNormalOperation();
        break;

    case UDPNM_STATE_REPEAT_MESSAGE:
    case UDPNM_STATE_NORMAL_OPERATION:
        /* Already active: reset NM timeout timer */
        UdpNm_NmTimeoutTimer = 0U;
        break;

    default:
        break;
    }

    return E_OK;
}

Std_ReturnType UdpNm_NetworkRelease(uint8 NetworkHandle)
{
    if (UdpNm_InitState != UDPNM_INIT)
    {
        (void)Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID,
                              UDPNM_SID_NETWORK_RELEASE, UDPNM_E_UNINIT);
        return E_NOT_OK;
    }

    if (NetworkHandle != 0U)
    {
        (void)Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID,
                              UDPNM_SID_NETWORK_RELEASE, UDPNM_E_PARAM_NETWORK);
        return E_NOT_OK;
    }

    UdpNm_NetworkRequested = FALSE;

    if (UdpNm_NmState == UDPNM_STATE_NORMAL_OPERATION)
    {
        UdpNm_EnterReadySleep();
    }

    return E_OK;
}

Std_ReturnType UdpNm_SetUserData(uint8 NetworkHandle, const uint8 *NmUserDataPtr)
{
    uint8 i;

    if (UdpNm_InitState != UDPNM_INIT)
    {
        (void)Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID,
                              UDPNM_SID_SET_USER_DATA, UDPNM_E_UNINIT);
        return E_NOT_OK;
    }

    if (NetworkHandle != 0U)
    {
        (void)Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID,
                              UDPNM_SID_SET_USER_DATA, UDPNM_E_PARAM_NETWORK);
        return E_NOT_OK;
    }

    if (NmUserDataPtr == NULL)
    {
        (void)Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID,
                              UDPNM_SID_SET_USER_DATA, UDPNM_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    for (i = 0U; i < UDPNM_USER_DATA_LENGTH; i++)
    {
        UdpNm_TxPduData[UDPNM_PDU_USERDATA_OFFSET + i] = NmUserDataPtr[i];
    }

    return E_OK;
}

Std_ReturnType UdpNm_GetUserData(uint8 NetworkHandle, uint8 *NmUserDataPtr)
{
    uint8 i;

    if (UdpNm_InitState != UDPNM_INIT)
    {
        (void)Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID,
                              UDPNM_SID_GET_USER_DATA, UDPNM_E_UNINIT);
        return E_NOT_OK;
    }

    if (NetworkHandle != 0U)
    {
        (void)Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID,
                              UDPNM_SID_GET_USER_DATA, UDPNM_E_PARAM_NETWORK);
        return E_NOT_OK;
    }

    if (NmUserDataPtr == NULL)
    {
        (void)Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID,
                              UDPNM_SID_GET_USER_DATA, UDPNM_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    /* Return user data from the last received NM PDU */
    for (i = 0U; i < UDPNM_USER_DATA_LENGTH; i++)
    {
        NmUserDataPtr[i] = UdpNm_RxPduData[UDPNM_PDU_USERDATA_OFFSET + i];
    }

    return E_OK;
}

Std_ReturnType UdpNm_GetPduData(uint8 NetworkHandle, uint8 *NmPduDataPtr)
{
    uint8 i;

    if (UdpNm_InitState != UDPNM_INIT)
    {
        (void)Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID,
                              UDPNM_SID_GET_PDU_DATA, UDPNM_E_UNINIT);
        return E_NOT_OK;
    }

    if (NetworkHandle != 0U)
    {
        (void)Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID,
                              UDPNM_SID_GET_PDU_DATA, UDPNM_E_PARAM_NETWORK);
        return E_NOT_OK;
    }

    if (NmPduDataPtr == NULL)
    {
        (void)Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID,
                              UDPNM_SID_GET_PDU_DATA, UDPNM_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    for (i = 0U; i < UDPNM_PDU_LENGTH; i++)
    {
        NmPduDataPtr[i] = UdpNm_RxPduData[i];
    }

    return E_OK;
}

Std_ReturnType UdpNm_GetNodeIdentifier(uint8 NetworkHandle, uint8 *NmNodeIdPtr)
{
    if (UdpNm_InitState != UDPNM_INIT)
    {
        (void)Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID,
                              UDPNM_SID_GET_NODE_IDENTIFIER, UDPNM_E_UNINIT);
        return E_NOT_OK;
    }

    if (NetworkHandle != 0U)
    {
        (void)Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID,
                              UDPNM_SID_GET_NODE_IDENTIFIER, UDPNM_E_PARAM_NETWORK);
        return E_NOT_OK;
    }

    if (NmNodeIdPtr == NULL)
    {
        (void)Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID,
                              UDPNM_SID_GET_NODE_IDENTIFIER, UDPNM_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    /* Source node ID from the last received NM PDU */
    *NmNodeIdPtr = UdpNm_RxPduData[UDPNM_PDU_NID_POSITION];

    return E_OK;
}

Std_ReturnType UdpNm_GetLocalNodeIdentifier(uint8 NetworkHandle, uint8 *NmNodeIdPtr)
{
    if (UdpNm_InitState != UDPNM_INIT)
    {
        (void)Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID,
                              UDPNM_SID_GET_LOCAL_NODE_IDENTIFIER, UDPNM_E_UNINIT);
        return E_NOT_OK;
    }

    if (NetworkHandle != 0U)
    {
        (void)Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID,
                              UDPNM_SID_GET_LOCAL_NODE_IDENTIFIER, UDPNM_E_PARAM_NETWORK);
        return E_NOT_OK;
    }

    if (NmNodeIdPtr == NULL)
    {
        (void)Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID,
                              UDPNM_SID_GET_LOCAL_NODE_IDENTIFIER, UDPNM_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    *NmNodeIdPtr = UDPNM_NODE_ID;

    return E_OK;
}

Std_ReturnType UdpNm_GetState(uint8 NetworkHandle, UdpNm_NmStateType *NmStatePtr,
                               UdpNm_ModeType *NmModePtr)
{
    if (UdpNm_InitState != UDPNM_INIT)
    {
        (void)Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID,
                              UDPNM_SID_GET_STATE, UDPNM_E_UNINIT);
        return E_NOT_OK;
    }

    if (NetworkHandle != 0U)
    {
        (void)Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID,
                              UDPNM_SID_GET_STATE, UDPNM_E_PARAM_NETWORK);
        return E_NOT_OK;
    }

    if ((NmStatePtr == NULL) || (NmModePtr == NULL))
    {
        (void)Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID,
                              UDPNM_SID_GET_STATE, UDPNM_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    *NmStatePtr = UdpNm_NmState;
    *NmModePtr  = UdpNm_Mode;

    return E_OK;
}

Std_ReturnType UdpNm_RepeatMessageRequest(uint8 NetworkHandle)
{
    if (UdpNm_InitState != UDPNM_INIT)
    {
        (void)Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID,
                              UDPNM_SID_REPEAT_MESSAGE_REQUEST, UDPNM_E_UNINIT);
        return E_NOT_OK;
    }

    if (NetworkHandle != 0U)
    {
        (void)Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID,
                              UDPNM_SID_REPEAT_MESSAGE_REQUEST, UDPNM_E_PARAM_NETWORK);
        return E_NOT_OK;
    }

    /*
     * SWS: RepeatMessageRequest() is only valid when Node Detection is enabled.
     * In this project Node Detection is not configured, so reject the call.
     */
    (void)Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID,
                          UDPNM_SID_REPEAT_MESSAGE_REQUEST,
                          UDPNM_E_NODE_DETECTION_DISABLED);
    return E_NOT_OK;
}

Std_ReturnType UdpNm_Transmit(PduIdType UdpNmSrcPduId,
                               const PduInfoType *UdpNmSrcPduInfoPtr)
{
    if (UdpNm_InitState != UDPNM_INIT)
    {
        (void)Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID,
                              UDPNM_SID_TRANSMIT, UDPNM_E_UNINIT);
        return E_NOT_OK;
    }

    if (UdpNmSrcPduInfoPtr == NULL)
    {
        (void)Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID,
                              UDPNM_SID_TRANSMIT, UDPNM_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    /* Forward NM PDU from gateway/PduR to SoAd for transmission */
    return SoAd_IfTransmit(UdpNmSrcPduId, UdpNmSrcPduInfoPtr);
}

void UdpNm_TxConfirmation(PduIdType TxPduId, Std_ReturnType result)
{
    if (UdpNm_InitState != UDPNM_INIT)
    {
        (void)Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID,
                              UDPNM_SID_TX_CONFIRMATION, UDPNM_E_UNINIT);
        return;
    }

    (void)TxPduId;
    (void)result;
}

void UdpNm_RxIndication(PduIdType RxPduId, const PduInfoType *PduInfoPtr)
{
    uint8 copyLen;
    uint8 i;

    if (UdpNm_InitState != UDPNM_INIT)
    {
        (void)Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID,
                              UDPNM_SID_RX_INDICATION, UDPNM_E_UNINIT);
        return;
    }

    if (PduInfoPtr == NULL)
    {
        (void)Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID,
                              UDPNM_SID_RX_INDICATION, UDPNM_E_PARAM_POINTER);
        return;
    }

    (void)RxPduId;

    /* Copy received PDU into the RX shadow buffer */
    copyLen = (uint8)((PduInfoPtr->SduLength < (PduLengthType)UDPNM_PDU_LENGTH)
                      ? PduInfoPtr->SduLength
                      : (PduLengthType)UDPNM_PDU_LENGTH);

    for (i = 0U; i < copyLen; i++)
    {
        UdpNm_RxPduData[i] = PduInfoPtr->SduDataPtr[i];
    }

    /*
     * SWS: NmTimeoutTime watchdog is reset on reception of any NM PDU,
     * regardless of source node or current state.
     */
    UdpNm_NmTimeoutTimer = 0U;

    /* Wake up on reception while sleeping */
    if ((UdpNm_NmState == UDPNM_STATE_BUS_SLEEP) ||
        (UdpNm_NmState == UDPNM_STATE_PREPARE_BUS_SLEEP))
    {
        UdpNm_PassiveMode = TRUE;
        UdpNm_EnterRepeatMessage();
    }
}

void UdpNm_MainFunction(void)
{
    uint16 activeCycleTime;

    if (UdpNm_InitState != UDPNM_INIT)
    {
        return;
    }

    switch (UdpNm_NmState)
    {
    case UDPNM_STATE_BUS_SLEEP:
        /* Waiting for NetworkRequest() or RxIndication() */
        break;

    case UDPNM_STATE_REPEAT_MESSAGE:
        UdpNm_RepeatMsgTimer++;
        UdpNm_NmTimeoutTimer++;
        UdpNm_MsgCycleTimer++;

        /* Use fast cycle while immediate transmissions remain */
        activeCycleTime = (UdpNm_ImmediateNmTxCounter > 0U)
                          ? UDPNM_IMMEDIATE_NM_CYCLE_TIME
                          : UDPNM_MSG_CYCLE_TIME;

        if (UdpNm_MsgCycleTimer >= activeCycleTime)
        {
            UdpNm_TransmitNmMessage();
            UdpNm_MsgCycleTimer = 0U;
            if (UdpNm_ImmediateNmTxCounter > 0U)
            {
                UdpNm_ImmediateNmTxCounter--;
            }
        }

        /* Repeat Message timer expired: decide next state */
        if (UdpNm_RepeatMsgTimer >= UDPNM_REPEAT_MESSAGE_TIME)
        {
            UdpNm_PassiveMode = FALSE;
            if (UdpNm_NetworkRequested == TRUE)
            {
                UdpNm_EnterNormalOperation();
            }
            else
            {
                UdpNm_EnterReadySleep();
            }
        }
        break;

    case UDPNM_STATE_NORMAL_OPERATION:
        UdpNm_NmTimeoutTimer++;
        UdpNm_MsgCycleTimer++;

        if (UdpNm_MsgCycleTimer >= UDPNM_MSG_CYCLE_TIME)
        {
            UdpNm_TransmitNmMessage();
            UdpNm_MsgCycleTimer = 0U;
        }

        /*
         * SWS: if no NM PDU received within NmTimeoutTime, transition back
         * to Repeat Message State to attempt re-synchronisation.
         */
        if (UdpNm_NmTimeoutTimer >= UDPNM_TIMEOUT_TIME)
        {
            UdpNm_EnterRepeatMessage();
        }
        break;

    case UDPNM_STATE_READY_SLEEP:
        UdpNm_NmTimeoutTimer++;
        UdpNm_MsgCycleTimer++;

        /*
         * SWS: NM PDU transmission continues in Ready Sleep state
         * (as long as passive mode is not active).
         */
        if (UdpNm_MsgCycleTimer >= UDPNM_MSG_CYCLE_TIME)
        {
            UdpNm_TransmitNmMessage();
            UdpNm_MsgCycleTimer = 0U;
        }

        if (UdpNm_NmTimeoutTimer >= UDPNM_TIMEOUT_TIME)
        {
            UdpNm_EnterPrepareBusSleep();
        }
        break;

    case UDPNM_STATE_PREPARE_BUS_SLEEP:
        UdpNm_WaitBusSleepTimer++;

        if (UdpNm_WaitBusSleepTimer >= UDPNM_WAIT_BUS_SLEEP_TIME)
        {
            UdpNm_EnterBusSleep();
        }
        break;

    default:
        break;
    }
}
