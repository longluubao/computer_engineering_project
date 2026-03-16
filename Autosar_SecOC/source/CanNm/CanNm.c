/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "CanNm.h"
#include "Det.h"
#include "CanIF.h"
#include "BswM.h"

/********************************************************************************************************/
/******************************************GlobalVaribles************************************************/
/********************************************************************************************************/

static CanNm_InitStateType CanNm_InitState = CANNM_UNINIT;
static CanNm_NmStateType CanNm_NmState = CANNM_STATE_BUS_SLEEP;
static CanNm_ModeType CanNm_Mode = CANNM_MODE_BUS_SLEEP;
static boolean CanNm_NetworkRequested = FALSE;

/* Timers (count MainFunction cycles) */
static uint16 CanNm_NmTimeoutTimer = 0;
static uint16 CanNm_RepeatMsgTimer = 0;
static uint16 CanNm_WaitBusSleepTimer = 0;
static uint16 CanNm_MsgCycleTimer = 0;

/* NM PDU data */
static uint8 CanNm_TxPduData[CANNM_PDU_LENGTH];
static boolean CanNm_MsgTxEnabled = FALSE;

/********************************************************************************************************/
/******************************************InternalFunctions**********************************************/
/********************************************************************************************************/

static void CanNm_TransmitNmMessage(void)
{
    if (CanNm_MsgTxEnabled == FALSE)
    {
        return;
    }

    PduInfoType pduInfo;
    pduInfo.SduDataPtr = CanNm_TxPduData;
    pduInfo.SduLength = CANNM_PDU_LENGTH;
    pduInfo.MetaDataPtr = NULL;

    (void)CanIf_Transmit(CANNM_TX_PDU_ID, &pduInfo);
}

static void CanNm_EnterRepeatMessage(void)
{
    CanNm_NmState = CANNM_STATE_REPEAT_MESSAGE;
    CanNm_Mode = CANNM_MODE_NETWORK;
    CanNm_RepeatMsgTimer = 0;
    CanNm_NmTimeoutTimer = 0;
    CanNm_MsgTxEnabled = TRUE;
    CanNm_MsgCycleTimer = 0;
    CanNm_TxPduData[CANNM_NM_PDU_CBV_POSITION] |= CANNM_CBV_REPEAT_MSG_REQUEST;
    (void)BswM_RequestMode((uint16)CANNM_MODULE_ID, (BswM_ModeType)CanNm_Mode);
}

static void CanNm_EnterNormalOperation(void)
{
    CanNm_NmState = CANNM_STATE_NORMAL_OPERATION;
    CanNm_Mode = CANNM_MODE_NETWORK;
    CanNm_NmTimeoutTimer = 0;
    CanNm_MsgTxEnabled = TRUE;
    CanNm_TxPduData[CANNM_NM_PDU_CBV_POSITION] &= (uint8)(~CANNM_CBV_REPEAT_MSG_REQUEST);
    (void)BswM_RequestMode((uint16)CANNM_MODULE_ID, (BswM_ModeType)CanNm_Mode);
}

static void CanNm_EnterReadySleep(void)
{
    CanNm_NmState = CANNM_STATE_READY_SLEEP;
    CanNm_Mode = CANNM_MODE_NETWORK;
    CanNm_NmTimeoutTimer = 0;
    CanNm_MsgTxEnabled = FALSE;
    (void)BswM_RequestMode((uint16)CANNM_MODULE_ID, (BswM_ModeType)CanNm_Mode);
}

static void CanNm_EnterPrepareBusSleep(void)
{
    CanNm_NmState = CANNM_STATE_PREPARE_BUS_SLEEP;
    CanNm_Mode = CANNM_MODE_PREPARE_BUS_SLEEP;
    CanNm_WaitBusSleepTimer = 0;
    CanNm_MsgTxEnabled = FALSE;
    (void)BswM_RequestMode((uint16)CANNM_MODULE_ID, (BswM_ModeType)CanNm_Mode);
}

static void CanNm_EnterBusSleep(void)
{
    CanNm_NmState = CANNM_STATE_BUS_SLEEP;
    CanNm_Mode = CANNM_MODE_BUS_SLEEP;
    CanNm_MsgTxEnabled = FALSE;
    (void)BswM_RequestMode((uint16)CANNM_MODULE_ID, (BswM_ModeType)CanNm_Mode);
}

/********************************************************************************************************/
/********************************************Functions***************************************************/
/********************************************************************************************************/

void CanNm_Init(void)
{
    CanNm_InitState = CANNM_INIT;
    CanNm_NetworkRequested = FALSE;
    CanNm_NmTimeoutTimer = 0;
    CanNm_RepeatMsgTimer = 0;
    CanNm_WaitBusSleepTimer = 0;
    CanNm_MsgCycleTimer = 0;
    CanNm_MsgTxEnabled = FALSE;

    for (uint8 i = 0; i < CANNM_PDU_LENGTH; i++)
    {
        CanNm_TxPduData[i] = 0x00U;
    }

    CanNm_EnterBusSleep();
}

void CanNm_DeInit(void)
{
    if (CanNm_InitState != CANNM_INIT)
    {
        (void)Det_ReportError(CANNM_MODULE_ID, CANNM_INSTANCE_ID, CANNM_SID_DEINIT, CANNM_E_UNINIT);
        return;
    }

    CanNm_InitState = CANNM_UNINIT;
    CanNm_EnterBusSleep();
}

Std_ReturnType CanNm_NetworkRequest(uint8 NetworkHandle)
{
    if (CanNm_InitState != CANNM_INIT)
    {
        (void)Det_ReportError(CANNM_MODULE_ID, CANNM_INSTANCE_ID, CANNM_SID_NETWORK_REQUEST, CANNM_E_UNINIT);
        return E_NOT_OK;
    }

    if (NetworkHandle != 0U)
    {
        (void)Det_ReportError(CANNM_MODULE_ID, CANNM_INSTANCE_ID, CANNM_SID_NETWORK_REQUEST, CANNM_E_PARAM_NETWORK);
        return E_NOT_OK;
    }

    CanNm_NetworkRequested = TRUE;

    switch (CanNm_NmState)
    {
    case CANNM_STATE_BUS_SLEEP:
    case CANNM_STATE_PREPARE_BUS_SLEEP:
        /* Wake-up: enter Repeat Message state */
        CanNm_TxPduData[CANNM_NM_PDU_CBV_POSITION] |= CANNM_CBV_ACTIVE_WAKEUP;
        CanNm_EnterRepeatMessage();
        break;
    case CANNM_STATE_READY_SLEEP:
        /* Re-enter Normal Operation */
        CanNm_EnterNormalOperation();
        break;
    case CANNM_STATE_REPEAT_MESSAGE:
    case CANNM_STATE_NORMAL_OPERATION:
        /* Already in network mode, reset NM timeout */
        CanNm_NmTimeoutTimer = 0;
        break;
    default:
        break;
    }

    return E_OK;
}

Std_ReturnType CanNm_NetworkRelease(uint8 NetworkHandle)
{
    if (CanNm_InitState != CANNM_INIT)
    {
        (void)Det_ReportError(CANNM_MODULE_ID, CANNM_INSTANCE_ID, CANNM_SID_NETWORK_RELEASE, CANNM_E_UNINIT);
        return E_NOT_OK;
    }

    if (NetworkHandle != 0U)
    {
        (void)Det_ReportError(CANNM_MODULE_ID, CANNM_INSTANCE_ID, CANNM_SID_NETWORK_RELEASE, CANNM_E_PARAM_NETWORK);
        return E_NOT_OK;
    }

    CanNm_NetworkRequested = FALSE;

    if (CanNm_NmState == CANNM_STATE_NORMAL_OPERATION)
    {
        CanNm_EnterReadySleep();
    }

    return E_OK;
}

Std_ReturnType CanNm_GetState(uint8 NetworkHandle, CanNm_NmStateType *NmStatePtr)
{
    if (CanNm_InitState != CANNM_INIT)
    {
        (void)Det_ReportError(CANNM_MODULE_ID, CANNM_INSTANCE_ID, CANNM_SID_GET_STATE, CANNM_E_UNINIT);
        return E_NOT_OK;
    }

    if (NetworkHandle != 0U)
    {
        (void)Det_ReportError(CANNM_MODULE_ID, CANNM_INSTANCE_ID, CANNM_SID_GET_STATE, CANNM_E_PARAM_NETWORK);
        return E_NOT_OK;
    }

    if (NmStatePtr == NULL)
    {
        (void)Det_ReportError(CANNM_MODULE_ID, CANNM_INSTANCE_ID, CANNM_SID_GET_STATE, CANNM_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    *NmStatePtr = CanNm_NmState;
    return E_OK;
}

void CanNm_TxConfirmation(PduIdType TxPduId, Std_ReturnType result)
{
    if (CanNm_InitState != CANNM_INIT)
    {
        (void)Det_ReportError(CANNM_MODULE_ID, CANNM_INSTANCE_ID, CANNM_SID_TX_CONFIRMATION, CANNM_E_UNINIT);
        return;
    }

    (void)TxPduId;
    (void)result;
}

void CanNm_RxIndication(PduIdType RxPduId, const PduInfoType *PduInfoPtr)
{
    if (CanNm_InitState != CANNM_INIT)
    {
        (void)Det_ReportError(CANNM_MODULE_ID, CANNM_INSTANCE_ID, CANNM_SID_RX_INDICATION, CANNM_E_UNINIT);
        return;
    }

    if (PduInfoPtr == NULL)
    {
        (void)Det_ReportError(CANNM_MODULE_ID, CANNM_INSTANCE_ID, CANNM_SID_RX_INDICATION, CANNM_E_PARAM_POINTER);
        return;
    }

    (void)RxPduId;

    /* Reception of NM message: reset NM timeout timer */
    CanNm_NmTimeoutTimer = 0;

    /* If in BusSleep or PrepareBusSleep, wake up */
    if ((CanNm_NmState == CANNM_STATE_BUS_SLEEP) ||
        (CanNm_NmState == CANNM_STATE_PREPARE_BUS_SLEEP))
    {
        CanNm_EnterRepeatMessage();
    }
}

void CanNm_MainFunction(void)
{
    if (CanNm_InitState != CANNM_INIT)
    {
        return;
    }

    switch (CanNm_NmState)
    {
    case CANNM_STATE_BUS_SLEEP:
        /* Nothing to do, waiting for NetworkRequest or RxIndication */
        break;

    case CANNM_STATE_REPEAT_MESSAGE:
        CanNm_RepeatMsgTimer++;
        CanNm_NmTimeoutTimer++;
        CanNm_MsgCycleTimer++;

        /* Transmit NM message on cycle */
        if (CanNm_MsgCycleTimer >= CANNM_MSG_CYCLE_TIME)
        {
            CanNm_TransmitNmMessage();
            CanNm_MsgCycleTimer = 0;
        }

        /* Repeat Message timer expired */
        if (CanNm_RepeatMsgTimer >= CANNM_REPEAT_MESSAGE_TIME)
        {
            if (CanNm_NetworkRequested == TRUE)
            {
                CanNm_EnterNormalOperation();
            }
            else
            {
                CanNm_EnterReadySleep();
            }
        }
        break;

    case CANNM_STATE_NORMAL_OPERATION:
        CanNm_NmTimeoutTimer++;
        CanNm_MsgCycleTimer++;

        /* Transmit NM message on cycle */
        if (CanNm_MsgCycleTimer >= CANNM_MSG_CYCLE_TIME)
        {
            CanNm_TransmitNmMessage();
            CanNm_MsgCycleTimer = 0;
        }

        /* NM timeout: transition to Repeat Message */
        if (CanNm_NmTimeoutTimer >= CANNM_TIMEOUT_TIME)
        {
            CanNm_EnterRepeatMessage();
        }
        break;

    case CANNM_STATE_READY_SLEEP:
        CanNm_NmTimeoutTimer++;

        /* NM timeout: transition to Prepare Bus Sleep */
        if (CanNm_NmTimeoutTimer >= CANNM_TIMEOUT_TIME)
        {
            CanNm_EnterPrepareBusSleep();
        }
        break;

    case CANNM_STATE_PREPARE_BUS_SLEEP:
        CanNm_WaitBusSleepTimer++;

        /* Wait Bus Sleep timer expired: go to Bus Sleep */
        if (CanNm_WaitBusSleepTimer >= CANNM_WAIT_BUS_SLEEP_TIME)
        {
            CanNm_EnterBusSleep();
        }
        break;

    default:
        break;
    }
}
