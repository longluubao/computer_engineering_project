/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "Can.h"
#include "Det.h"
#include "SecOC_Debug.h"

/********************************************************************************************************/
/******************************************GlobalVaribles************************************************/
/********************************************************************************************************/

typedef struct
{
    PduIdType TxPduId;
} Can_TxQueueEntryType;

#define CAN_RX_QUEUE_LENGTH ((uint8)32)

typedef struct
{
    PduIdType  RxPduId;
    PduInfoType PduInfo;
    uint8 SduData[8];
} Can_RxQueueEntryType;

static uint8 Can_Initialized = FALSE;
static Can_ControllerStateType Can_ControllerStates[CAN_MAX_CONTROLLERS];

/* Tx queue */
static Can_TxQueueEntryType Can_TxQueue[CAN_TX_QUEUE_LENGTH];
static uint8 Can_TxQueueHead = 0;
static uint8 Can_TxQueueTail = 0;
static uint8 Can_TxQueueCount = 0;
static Can_TxConfirmationCallbackType Can_TxConfirmationCallback = NULL;

/* Rx queue */
static Can_RxQueueEntryType Can_RxQueue[CAN_RX_QUEUE_LENGTH];
static uint8 Can_RxQueueHead = 0;
static uint8 Can_RxQueueTail = 0;
static uint8 Can_RxQueueCount = 0;
static Can_RxIndicationCallbackType Can_RxIndicationCallback = NULL;

/********************************************************************************************************/
/******************************************InternalFunctions**********************************************/
/********************************************************************************************************/

static void Can_ResetQueue(void)
{
    Can_TxQueueHead = 0;
    Can_TxQueueTail = 0;
    Can_TxQueueCount = 0;
    Can_RxQueueHead = 0;
    Can_RxQueueTail = 0;
    Can_RxQueueCount = 0;
}

/********************************************************************************************************/
/********************************************Functions***************************************************/
/********************************************************************************************************/

/* External API declarations (MISRA 8.4 visibility). */
void Can_Init(const Can_ConfigType *ConfigPtr);
void Can_DeInit(void);
Std_ReturnType Can_SetControllerMode(uint8 Controller, Can_ControllerStateType Transition);
Std_ReturnType Can_GetControllerMode(uint8 Controller, Can_ControllerStateType *ControllerModePtr);
Std_ReturnType Can_Write(PduIdType Hth, const Can_PduType *PduInfo);
void Can_MainFunction_Write(void);
void Can_MainFunction_Read(void);
void Can_RegisterTxConfirmation(Can_TxConfirmationCallbackType Callback);
void Can_RegisterRxIndication(Can_RxIndicationCallbackType Callback);
void Can_SimulateReception(PduIdType RxPduId, const PduInfoType *PduInfoPtr);

void Can_Init(const Can_ConfigType *ConfigPtr)
{
    (void)ConfigPtr;

    for (uint8 controller = 0; controller < CAN_MAX_CONTROLLERS; controller++)
    {
        Can_ControllerStates[controller] = CAN_CS_STOPPED;
    }

    Can_ResetQueue();
    Can_Initialized = TRUE;
}

void Can_DeInit(void)
{
    if (Can_Initialized == FALSE)
    {
        (void)Det_ReportError(CAN_MODULE_ID, CAN_INSTANCE_ID, CAN_SID_DEINIT, CAN_E_UNINIT);
        return;
    }

    Can_ResetQueue();
    Can_TxConfirmationCallback = NULL;
    Can_RxIndicationCallback = NULL;
    Can_Initialized = FALSE;
}

Std_ReturnType Can_SetControllerMode(uint8 Controller, Can_ControllerStateType Transition)
{
    Std_ReturnType RetVal = E_OK;
    Can_ControllerStateType CurrentState = CAN_CS_UNINIT;

    if (Can_Initialized == FALSE)
    {
        (void)Det_ReportError(CAN_MODULE_ID, CAN_INSTANCE_ID, CAN_SID_SET_CONTROLLER_MODE, CAN_E_UNINIT);
        RetVal = E_NOT_OK;
    }
    else if (Controller >= CAN_MAX_CONTROLLERS)
    {
        (void)Det_ReportError(CAN_MODULE_ID, CAN_INSTANCE_ID, CAN_SID_SET_CONTROLLER_MODE, CAN_E_PARAM_CONTROLLER);
        RetVal = E_NOT_OK;
    }
    else
    {
        CurrentState = Can_ControllerStates[Controller];
        switch (Transition)
        {
            case CAN_CS_STARTED:
                if ((CurrentState != CAN_CS_STOPPED) && (CurrentState != CAN_CS_STARTED))
                {
                    (void)Det_ReportError(CAN_MODULE_ID, CAN_INSTANCE_ID, CAN_SID_SET_CONTROLLER_MODE, CAN_E_TRANSITION);
                    RetVal = E_NOT_OK;
                }
                break;
            case CAN_CS_STOPPED:
                /* STOPPED can be reached from any initialized state. */
                break;
            case CAN_CS_SLEEP:
                if ((CurrentState != CAN_CS_STOPPED) && (CurrentState != CAN_CS_SLEEP))
                {
                    (void)Det_ReportError(CAN_MODULE_ID, CAN_INSTANCE_ID, CAN_SID_SET_CONTROLLER_MODE, CAN_E_TRANSITION);
                    RetVal = E_NOT_OK;
                }
                break;
            default:
                (void)Det_ReportError(CAN_MODULE_ID, CAN_INSTANCE_ID, CAN_SID_SET_CONTROLLER_MODE, CAN_E_TRANSITION);
                RetVal = E_NOT_OK;
                break;
        }
    }

    if (RetVal == E_OK)
    {
        Can_ControllerStates[Controller] = Transition;
    }
    return RetVal;
}

Std_ReturnType Can_GetControllerMode(uint8 Controller, Can_ControllerStateType *ControllerModePtr)
{
    if (Can_Initialized == FALSE)
    {
        (void)Det_ReportError(CAN_MODULE_ID, CAN_INSTANCE_ID, CAN_SID_GET_CONTROLLER_MODE, CAN_E_UNINIT);
        return E_NOT_OK;
    }

    if (Controller >= CAN_MAX_CONTROLLERS)
    {
        (void)Det_ReportError(CAN_MODULE_ID, CAN_INSTANCE_ID, CAN_SID_GET_CONTROLLER_MODE, CAN_E_PARAM_CONTROLLER);
        return E_NOT_OK;
    }

    if (ControllerModePtr == NULL)
    {
        (void)Det_ReportError(CAN_MODULE_ID, CAN_INSTANCE_ID, CAN_SID_GET_CONTROLLER_MODE, CAN_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    *ControllerModePtr = Can_ControllerStates[Controller];
    return E_OK;
}

Std_ReturnType Can_Write(PduIdType Hth, const Can_PduType *PduInfo)
{
    Std_ReturnType RetVal = E_OK;

    if (Can_Initialized == FALSE)
    {
        (void)Det_ReportError(CAN_MODULE_ID, CAN_INSTANCE_ID, CAN_SID_WRITE, CAN_E_UNINIT);
        RetVal = E_NOT_OK;
    }
    else if (PduInfo == NULL)
    {
        (void)Det_ReportError(CAN_MODULE_ID, CAN_INSTANCE_ID, CAN_SID_WRITE, CAN_E_PARAM_POINTER);
        RetVal = E_NOT_OK;
    }
    else if (Hth >= CAN_MAX_CONTROLLERS)
    {
        (void)Det_ReportError(CAN_MODULE_ID, CAN_INSTANCE_ID, CAN_SID_WRITE, CAN_E_PARAM_HANDLE);
        RetVal = E_NOT_OK;
    }
    else if ((PduInfo->sdu == NULL) && (PduInfo->length > 0U))
    {
        (void)Det_ReportError(CAN_MODULE_ID, CAN_INSTANCE_ID, CAN_SID_WRITE, CAN_E_PARAM_POINTER);
        RetVal = E_NOT_OK;
    }
    else if (Can_ControllerStates[Hth] != CAN_CS_STARTED)
    {
        RetVal = E_NOT_OK;
    }
    else if (Can_TxQueueCount >= CAN_TX_QUEUE_LENGTH)
    {
        RetVal = E_BUSY;
    }
    else
    {
        Can_TxQueue[Can_TxQueueTail].TxPduId = PduInfo->swPduHandle;
        Can_TxQueueTail = (uint8)((Can_TxQueueTail + 1U) % CAN_TX_QUEUE_LENGTH);
        Can_TxQueueCount++;
    }

    return RetVal;
}

void Can_MainFunction_Write(void)
{
    if (Can_Initialized == FALSE)
    {
        return;
    }

    while (Can_TxQueueCount > 0)
    {
        PduIdType txPduId = Can_TxQueue[Can_TxQueueHead].TxPduId;
        Can_TxQueueHead = (uint8)((Can_TxQueueHead + 1U) % CAN_TX_QUEUE_LENGTH);
        Can_TxQueueCount--;

        if (Can_TxConfirmationCallback != NULL)
        {
            Can_TxConfirmationCallback(txPduId, E_OK);
        }
    }
}

void Can_MainFunction_Read(void)
{
    if (Can_Initialized == FALSE)
    {
        return;
    }

    while (Can_RxQueueCount > 0)
    {
        Can_RxQueueEntryType *entry = &Can_RxQueue[Can_RxQueueHead];
        Can_RxQueueHead = (uint8)((Can_RxQueueHead + 1U) % CAN_RX_QUEUE_LENGTH);
        Can_RxQueueCount--;

        if (Can_RxIndicationCallback != NULL)
        {
            Can_RxIndicationCallback(entry->RxPduId, &entry->PduInfo);
        }
    }
}

void Can_RegisterTxConfirmation(Can_TxConfirmationCallbackType Callback)
{
    Can_TxConfirmationCallback = Callback;
}

void Can_RegisterRxIndication(Can_RxIndicationCallbackType Callback)
{
    Can_RxIndicationCallback = Callback;
}

void Can_SimulateReception(PduIdType RxPduId, const PduInfoType *PduInfoPtr)
{
    if ((Can_Initialized == FALSE) || (PduInfoPtr == NULL))
    {
        return;
    }

    if (Can_RxQueueCount >= CAN_RX_QUEUE_LENGTH)
    {
        return;
    }

    Can_RxQueueEntryType *entry = &Can_RxQueue[Can_RxQueueTail];
    entry->RxPduId = RxPduId;
    PduLengthType copyLen = (PduInfoPtr->SduLength <= 8U) ? PduInfoPtr->SduLength : 8U;
    if ((PduInfoPtr->SduDataPtr != NULL) && (copyLen > 0U))
    {
        for (PduLengthType i = 0; i < copyLen; i++)
        {
            entry->SduData[i] = PduInfoPtr->SduDataPtr[i];
        }
    }
    entry->PduInfo.SduDataPtr = entry->SduData;
    entry->PduInfo.SduLength = copyLen;
    entry->PduInfo.MetaDataPtr = NULL;

    Can_RxQueueTail = (uint8)((Can_RxQueueTail + 1U) % CAN_RX_QUEUE_LENGTH);
    Can_RxQueueCount++;
}
