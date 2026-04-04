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
        /* Passive mode: do not send NM messages */
        return;
    }

    pduInfo.SduDataPtr  = UdpNm_TxPduData;
    pduInfo.SduLength   = (PduLengthType)UDPNM_PDU_LENGTH;
    pduInfo.MetaDataPtr = NULL;

    (void)SoAd_IfTransmit(UDPNM_TX_PDU_ID, &pduInfo);
}

static void UdpNm_EnterRepeatMessage(void)
{
    UdpNm_NmState       = UDPNM_STATE_REPEAT_MESSAGE;
    UdpNm_Mode          = UDPNM_MODE_NETWORK;
    UdpNm_RepeatMsgTimer  = 0U;
    UdpNm_NmTimeoutTimer  = 0U;
    UdpNm_MsgCycleTimer   = 0U;
    UdpNm_MsgTxEnabled    = TRUE;
    UdpNm_TxPduData[UDPNM_PDU_CBV_POSITION] |= UDPNM_CBV_REPEAT_MSG_REQUEST;
    (void)BswM_RequestMode((uint16)UDPNM_MODULE_ID, (BswM_ModeType)UdpNm_Mode);
}

static void UdpNm_EnterNormalOperation(void)
{
    UdpNm_NmState     = UDPNM_STATE_NORMAL_OPERATION;
    UdpNm_Mode        = UDPNM_MODE_NETWORK;
    UdpNm_NmTimeoutTimer = 0U;
    UdpNm_MsgCycleTimer  = 0U;
    UdpNm_MsgTxEnabled   = TRUE;
    UdpNm_TxPduData[UDPNM_PDU_CBV_POSITION] &= (uint8)(~UDPNM_CBV_REPEAT_MSG_REQUEST);
    (void)BswM_RequestMode((uint16)UDPNM_MODULE_ID, (BswM_ModeType)UdpNm_Mode);
}

static void UdpNm_EnterReadySleep(void)
{
    UdpNm_NmState       = UDPNM_STATE_READY_SLEEP;
    UdpNm_Mode          = UDPNM_MODE_NETWORK;
    UdpNm_NmTimeoutTimer  = 0U;
    UdpNm_MsgTxEnabled    = FALSE;
    (void)BswM_RequestMode((uint16)UDPNM_MODULE_ID, (BswM_ModeType)UdpNm_Mode);
}

static void UdpNm_EnterPrepareBusSleep(void)
{
    UdpNm_NmState         = UDPNM_STATE_PREPARE_BUS_SLEEP;
    UdpNm_Mode            = UDPNM_MODE_PREPARE_BUS_SLEEP;
    UdpNm_WaitBusSleepTimer = 0U;
    UdpNm_MsgTxEnabled      = FALSE;
    (void)BswM_RequestMode((uint16)UDPNM_MODULE_ID, (BswM_ModeType)UdpNm_Mode);
}

static void UdpNm_EnterBusSleep(void)
{
    UdpNm_NmState      = UDPNM_STATE_BUS_SLEEP;
    UdpNm_Mode         = UDPNM_MODE_BUS_SLEEP;
    UdpNm_MsgTxEnabled   = FALSE;
    UdpNm_TxPduData[UDPNM_PDU_CBV_POSITION] = 0x00U;
    (void)BswM_RequestMode((uint16)UDPNM_MODULE_ID, (BswM_ModeType)UdpNm_Mode);
}

/********************************************************************************************************/
/********************************************Functions***************************************************/
/********************************************************************************************************/

void UdpNm_Init(void)
{
    uint8 i;

    UdpNm_NetworkRequested  = FALSE;
    UdpNm_PassiveMode       = FALSE;
    UdpNm_NmTimeoutTimer    = 0U;
    UdpNm_RepeatMsgTimer    = 0U;
    UdpNm_WaitBusSleepTimer = 0U;
    UdpNm_MsgCycleTimer     = 0U;
    UdpNm_MsgTxEnabled      = FALSE;

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

    /* Passive startup: enter Repeat Message without sending */
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
        /* Wake-up: set active wakeup bit and enter Repeat Message */
        UdpNm_TxPduData[UDPNM_PDU_CBV_POSITION] |= UDPNM_CBV_ACTIVE_WAKEUP;
        UdpNm_EnterRepeatMessage();
        break;

    case UDPNM_STATE_READY_SLEEP:
        /* Return to Normal Operation */
        UdpNm_EnterNormalOperation();
        break;

    case UDPNM_STATE_REPEAT_MESSAGE:
    case UDPNM_STATE_NORMAL_OPERATION:
        /* Already active: reset NM timeout */
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

    /* Return user data from last received NM PDU */
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

    /* Return source node ID from the last received NM PDU */
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

Std_ReturnType UdpNm_GetState(uint8 NetworkHandle, UdpNm_NmStateType *NmStatePtr)
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

    if (NmStatePtr == NULL)
    {
        (void)Det_ReportError(UDPNM_MODULE_ID, UDPNM_INSTANCE_ID,
                              UDPNM_SID_GET_STATE, UDPNM_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    *NmStatePtr = UdpNm_NmState;

    return E_OK;
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

    /* Copy received PDU into local buffer (up to PDU length) */
    copyLen = (uint8)((PduInfoPtr->SduLength < (PduLengthType)UDPNM_PDU_LENGTH)
                      ? PduInfoPtr->SduLength
                      : (PduLengthType)UDPNM_PDU_LENGTH);

    for (i = 0U; i < copyLen; i++)
    {
        UdpNm_RxPduData[i] = PduInfoPtr->SduDataPtr[i];
    }

    /* Reset NM timeout: a remote node is active */
    UdpNm_NmTimeoutTimer = 0U;

    /* Wake up from sleep states on reception */
    if ((UdpNm_NmState == UDPNM_STATE_BUS_SLEEP) ||
        (UdpNm_NmState == UDPNM_STATE_PREPARE_BUS_SLEEP))
    {
        UdpNm_PassiveMode = TRUE;
        UdpNm_EnterRepeatMessage();
    }
}

void UdpNm_MainFunction(void)
{
    if (UdpNm_InitState != UDPNM_INIT)
    {
        return;
    }

    switch (UdpNm_NmState)
    {
    case UDPNM_STATE_BUS_SLEEP:
        /* Waiting for NetworkRequest or RxIndication */
        break;

    case UDPNM_STATE_REPEAT_MESSAGE:
        UdpNm_RepeatMsgTimer++;
        UdpNm_NmTimeoutTimer++;
        UdpNm_MsgCycleTimer++;

        if (UdpNm_MsgCycleTimer >= UDPNM_MSG_CYCLE_TIME)
        {
            UdpNm_TransmitNmMessage();
            UdpNm_MsgCycleTimer = 0U;
        }

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

        /* NM timeout with no received messages: go back to Repeat Message */
        if (UdpNm_NmTimeoutTimer >= UDPNM_TIMEOUT_TIME)
        {
            UdpNm_EnterRepeatMessage();
        }
        break;

    case UDPNM_STATE_READY_SLEEP:
        UdpNm_NmTimeoutTimer++;

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
