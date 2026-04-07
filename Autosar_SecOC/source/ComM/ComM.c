/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "BswM/BswM.h"
#include "CanNm/CanNm.h"
#include "CanSM/CanSM.h"
#include "ComM/ComM.h"
#include "Det/Det.h"

/********************************************************************************************************/
/**************************************ForwardDeclarations***********************************************/
/********************************************************************************************************/

extern void ComM_Init(void);
extern void ComM_DeInit(void);
extern Std_ReturnType ComM_RequestComMode(uint8 Channel, ComM_ModeType ComMode);
extern Std_ReturnType ComM_GetCurrentComMode(uint8 Channel, ComM_ModeType *ComModePtr);
extern Std_ReturnType ComM_GetState(uint8 Channel, ComM_SubStateType *StatePtr);
extern Std_ReturnType ComM_BusSM_ModeIndication(uint8 Channel, ComM_ModeType ComMode);
extern void ComM_MainFunction(void);

/* MISRA C:2012 Rule 17.3 - Cross-module forward declarations */
extern Std_ReturnType CanNm_NetworkRequest(uint8 NetworkHandle);
extern Std_ReturnType CanNm_NetworkRelease(uint8 NetworkHandle);
extern Std_ReturnType CanSM_RequestComMode(uint8 NetworkHandle, CanSM_ComModeType ComM_Mode);

/********************************************************************************************************/
/******************************************GlobalVaribles************************************************/
/********************************************************************************************************/

static ComM_StateType ComM_State = COMM_UNINIT;
static ComM_ModeType ComM_CurrentMode = COMM_NO_COMMUNICATION;
static ComM_SubStateType ComM_SubState = COMM_NO_COM_NO_PENDING_REQUEST;
static ComM_UserRequestType ComM_UserRequests[COMM_MAX_USERS];

/********************************************************************************************************/
/*****************************************StaticFunctions************************************************/
/********************************************************************************************************/

static ComM_ModeType ComM_EvaluateHighestRequest(void)
{
    uint8 idx;
    ComM_ModeType highest = COMM_NO_COMMUNICATION;

    for (idx = 0U; idx < COMM_MAX_USERS; idx++)
    {
        if (ComM_UserRequests[idx].IsActive == TRUE)
        {
            if (ComM_UserRequests[idx].RequestedMode > highest)
            {
                highest = ComM_UserRequests[idx].RequestedMode;
            }
        }
    }

    return highest;
}

static Std_ReturnType ComM_ApplyMode(uint8 Channel, ComM_ModeType TargetMode)
{
    if (TargetMode == COMM_FULL_COMMUNICATION)
    {
        if (CanNm_NetworkRequest(Channel) != E_OK)
        {
            return E_NOT_OK;
        }
        if (CanSM_RequestComMode(Channel, CANSM_FULL_COMMUNICATION) != E_OK)
        {
            return E_NOT_OK;
        }
        ComM_SubState = COMM_FULL_COM_NETWORK_REQUESTED;
    }
    else if (TargetMode == COMM_SILENT_COMMUNICATION)
    {
        if (CanSM_RequestComMode(Channel, CANSM_NO_COMMUNICATION) != E_OK)
        {
            return E_NOT_OK;
        }
        ComM_SubState = COMM_SILENT_COM;
    }
    else
    {
        if (CanSM_RequestComMode(Channel, CANSM_NO_COMMUNICATION) != E_OK)
        {
            return E_NOT_OK;
        }
        if (CanNm_NetworkRelease(Channel) != E_OK)
        {
            return E_NOT_OK;
        }
        ComM_SubState = COMM_NO_COM_NO_PENDING_REQUEST;
    }

    ComM_CurrentMode = TargetMode;
    (void)BswM_RequestMode((uint16)COMM_MODULE_ID, (BswM_ModeType)ComM_CurrentMode);
    return E_OK;
}

/********************************************************************************************************/
/********************************************Functions***************************************************/
/********************************************************************************************************/

void ComM_Init(void)
{
    uint8 idx;

    for (idx = 0U; idx < COMM_MAX_USERS; idx++)
    {
        ComM_UserRequests[idx].RequestedMode = COMM_NO_COMMUNICATION;
        ComM_UserRequests[idx].IsActive = FALSE;
    }

    ComM_CurrentMode = COMM_NO_COMMUNICATION;
    ComM_SubState = COMM_NO_COM_NO_PENDING_REQUEST;
    ComM_State = COMM_INIT;
    (void)BswM_RequestMode((uint16)COMM_MODULE_ID, (BswM_ModeType)COMM_NO_COMMUNICATION);
    (void)BswM_RequestMode(BSWM_REQUESTER_ID_COMM_DESIRED, (BswM_ModeType)COMM_NO_COMMUNICATION);
}

void ComM_DeInit(void)
{
    if (ComM_State != COMM_INIT)
    {
        (void)Det_ReportError(COMM_MODULE_ID, COMM_INSTANCE_ID, COMM_SID_DEINIT, COMM_E_UNINIT);
        return;
    }

    ComM_CurrentMode = COMM_NO_COMMUNICATION;
    ComM_SubState = COMM_NO_COM_NO_PENDING_REQUEST;
    ComM_State = COMM_UNINIT;
}

Std_ReturnType ComM_RequestComMode(uint8 Channel, ComM_ModeType ComMode)
{
    if (ComM_State != COMM_INIT)
    {
        (void)Det_ReportError(COMM_MODULE_ID, COMM_INSTANCE_ID, COMM_SID_REQUEST_COM_MODE, COMM_E_UNINIT);
        return E_NOT_OK;
    }

    if (Channel >= COMM_MAX_CHANNELS)
    {
        (void)Det_ReportError(COMM_MODULE_ID, COMM_INSTANCE_ID, COMM_SID_REQUEST_COM_MODE, COMM_E_PARAM_CHANNEL);
        return E_NOT_OK;
    }

    if (ComMode > COMM_FULL_COMMUNICATION)
    {
        (void)Det_ReportError(COMM_MODULE_ID, COMM_INSTANCE_ID, COMM_SID_REQUEST_COM_MODE, COMM_E_PARAM_MODE);
        return E_NOT_OK;
    }

    /* Store user request (use Channel as user index for simplified model) */
    if (Channel < COMM_MAX_USERS)
    {
        ComM_UserRequests[Channel].RequestedMode = ComMode;
        ComM_UserRequests[Channel].IsActive = TRUE;
    }

    (void)BswM_RequestMode(BSWM_REQUESTER_ID_COMM_DESIRED, (BswM_ModeType)ComMode);

    /* Apply immediately */
    return ComM_ApplyMode(Channel, ComMode);
}

Std_ReturnType ComM_GetCurrentComMode(uint8 Channel, ComM_ModeType *ComModePtr)
{
    if (ComM_State != COMM_INIT)
    {
        (void)Det_ReportError(COMM_MODULE_ID, COMM_INSTANCE_ID, COMM_SID_GET_CURRENT_COM_MODE, COMM_E_UNINIT);
        return E_NOT_OK;
    }

    if (Channel >= COMM_MAX_CHANNELS)
    {
        (void)Det_ReportError(COMM_MODULE_ID, COMM_INSTANCE_ID, COMM_SID_GET_CURRENT_COM_MODE, COMM_E_PARAM_CHANNEL);
        return E_NOT_OK;
    }

    if (ComModePtr == NULL)
    {
        (void)Det_ReportError(COMM_MODULE_ID, COMM_INSTANCE_ID, COMM_SID_GET_CURRENT_COM_MODE, COMM_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    *ComModePtr = ComM_CurrentMode;
    return E_OK;
}

Std_ReturnType ComM_GetState(uint8 Channel, ComM_SubStateType *StatePtr)
{
    if (ComM_State != COMM_INIT)
    {
        (void)Det_ReportError(COMM_MODULE_ID, COMM_INSTANCE_ID, COMM_SID_GET_STATE, COMM_E_UNINIT);
        return E_NOT_OK;
    }

    if (Channel >= COMM_MAX_CHANNELS)
    {
        (void)Det_ReportError(COMM_MODULE_ID, COMM_INSTANCE_ID, COMM_SID_GET_STATE, COMM_E_PARAM_CHANNEL);
        return E_NOT_OK;
    }

    if (StatePtr == NULL)
    {
        (void)Det_ReportError(COMM_MODULE_ID, COMM_INSTANCE_ID, COMM_SID_GET_STATE, COMM_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    *StatePtr = ComM_SubState;
    return E_OK;
}

Std_ReturnType ComM_BusSM_ModeIndication(uint8 Channel, ComM_ModeType ComMode)
{
    if (ComM_State != COMM_INIT)
    {
        (void)Det_ReportError(COMM_MODULE_ID, COMM_INSTANCE_ID, COMM_SID_BUSSM_MODE_INDICATION, COMM_E_UNINIT);
        return E_NOT_OK;
    }

    if (Channel >= COMM_MAX_CHANNELS)
    {
        (void)Det_ReportError(COMM_MODULE_ID, COMM_INSTANCE_ID, COMM_SID_BUSSM_MODE_INDICATION, COMM_E_PARAM_CHANNEL);
        return E_NOT_OK;
    }

    ComM_CurrentMode = ComMode;

    /* Notify BswM of mode change */
    (void)BswM_RequestMode((uint16)COMM_MODULE_ID, (BswM_ModeType)ComMode);

    return E_OK;
}

void ComM_MainFunction(void)
{
    ComM_ModeType highestRequest;

    if (ComM_State != COMM_INIT)
    {
        return;
    }

    /* Evaluate highest user request and transition if needed */
    highestRequest = ComM_EvaluateHighestRequest();

    if (highestRequest != ComM_CurrentMode)
    {
        (void)ComM_ApplyMode(0U, highestRequest);
    }
}
