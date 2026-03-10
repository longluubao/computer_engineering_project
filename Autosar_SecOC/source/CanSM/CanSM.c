/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "CanSM.h"
#include "Can.h"
#include "Det.h"
#include "CanIF.h"
#include "ComM.h"

/********************************************************************************************************/
/******************************************GlobalVaribles************************************************/
/********************************************************************************************************/

static CanSM_StateType CanSM_State = CANSM_UNINIT;
static CanSM_ComModeType CanSM_CurrentComMode = CANSM_NO_COMMUNICATION;
static CanSM_BsmStateType CanSM_BsmState = CANSM_BSM_S_NOT_INITIALIZED;
static boolean CanSM_BusOffPending = FALSE;
static uint8 CanSM_BusOffRecoveryCount = 0;
static uint16 CanSM_BusOffRecoveryTimer = 0;

/********************************************************************************************************/
/********************************************Functions***************************************************/
/********************************************************************************************************/

void CanSM_Init(void)
{
    CanSM_State = CANSM_INIT;
    CanSM_CurrentComMode = CANSM_NO_COMMUNICATION;
    CanSM_BsmState = CANSM_BSM_S_PRE_NOCOM;
    CanSM_BusOffPending = FALSE;
    CanSM_BusOffRecoveryCount = 0;
    CanSM_BusOffRecoveryTimer = 0;

    /* Transition PRE_NOCOM -> NOCOM: set controller to STOPPED */
    (void)Can_SetControllerMode(0U, CAN_CS_STOPPED);
    CanIf_ControllerModeIndication(0U, CANIF_CS_STOPPED);
    CanSM_BsmState = CANSM_BSM_S_NOCOM;
}

void CanSM_DeInit(void)
{
    if (CanSM_State != CANSM_INIT)
    {
        (void)Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID, CANSM_SID_DEINIT, CANSM_E_UNINIT);
        return;
    }

    CanSM_State = CANSM_UNINIT;
    CanSM_CurrentComMode = CANSM_NO_COMMUNICATION;
    CanSM_BsmState = CANSM_BSM_S_NOT_INITIALIZED;
}

Std_ReturnType CanSM_RequestComMode(uint8 NetworkHandle, CanSM_ComModeType ComM_Mode)
{
    if (CanSM_State != CANSM_INIT)
    {
        (void)Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID, CANSM_SID_REQUEST_COM_MODE, CANSM_E_UNINIT);
        return E_NOT_OK;
    }

    if (NetworkHandle != 0U)
    {
        (void)Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID, CANSM_SID_REQUEST_COM_MODE, CANSM_E_PARAM_NETWORK);
        return E_NOT_OK;
    }

    if (ComM_Mode == CANSM_FULL_COMMUNICATION)
    {
        /* BSM transition: NOCOM -> PRE_FULLCOM -> FULLCOM */
        CanSM_BsmState = CANSM_BSM_S_PRE_FULLCOM;

        if (Can_SetControllerMode(0U, CAN_CS_STARTED) != E_OK)
        {
            CanSM_BsmState = CANSM_BSM_S_NOCOM;
            return E_NOT_OK;
        }
        CanIf_ControllerModeIndication(0U, CANIF_CS_STARTED);

        CanSM_BsmState = CANSM_BSM_S_FULLCOM;
        CanSM_CurrentComMode = CANSM_FULL_COMMUNICATION;
        (void)ComM_BusSM_ModeIndication(NetworkHandle, COMM_FULL_COMMUNICATION);
    }
    else
    {
        /* BSM transition: FULLCOM -> PRE_NOCOM -> NOCOM */
        CanSM_BsmState = CANSM_BSM_S_PRE_NOCOM;

        if (Can_SetControllerMode(0U, CAN_CS_STOPPED) != E_OK)
        {
            return E_NOT_OK;
        }
        CanIf_ControllerModeIndication(0U, CANIF_CS_STOPPED);

        CanSM_BsmState = CANSM_BSM_S_NOCOM;
        CanSM_CurrentComMode = CANSM_NO_COMMUNICATION;
        (void)ComM_BusSM_ModeIndication(NetworkHandle, COMM_NO_COMMUNICATION);
    }

    return E_OK;
}

Std_ReturnType CanSM_GetCurrentComMode(uint8 NetworkHandle, CanSM_ComModeType *ComM_ModePtr)
{
    if (CanSM_State != CANSM_INIT)
    {
        (void)Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID, CANSM_SID_GET_CURRENT_COM_MODE, CANSM_E_UNINIT);
        return E_NOT_OK;
    }

    if (NetworkHandle != 0U)
    {
        (void)Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID, CANSM_SID_GET_CURRENT_COM_MODE, CANSM_E_PARAM_NETWORK);
        return E_NOT_OK;
    }

    if (ComM_ModePtr == NULL)
    {
        (void)Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID, CANSM_SID_GET_CURRENT_COM_MODE, CANSM_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    *ComM_ModePtr = CanSM_CurrentComMode;
    return E_OK;
}

Std_ReturnType CanSM_GetBsmState(uint8 NetworkHandle, CanSM_BsmStateType *BsmStatePtr)
{
    if (CanSM_State != CANSM_INIT)
    {
        (void)Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID, CANSM_SID_GET_CURRENT_COM_MODE, CANSM_E_UNINIT);
        return E_NOT_OK;
    }

    if (NetworkHandle != 0U)
    {
        (void)Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID, CANSM_SID_GET_CURRENT_COM_MODE, CANSM_E_PARAM_NETWORK);
        return E_NOT_OK;
    }

    if (BsmStatePtr == NULL)
    {
        (void)Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID, CANSM_SID_GET_CURRENT_COM_MODE, CANSM_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    *BsmStatePtr = CanSM_BsmState;
    return E_OK;
}

void CanSM_ControllerBusOff(uint8 ControllerId)
{
    if (CanSM_State != CANSM_INIT)
    {
        (void)Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID, CANSM_SID_CONTROLLER_BUSOFF, CANSM_E_UNINIT);
        return;
    }

    if (ControllerId >= CAN_MAX_CONTROLLERS)
    {
        (void)Det_ReportError(CANSM_MODULE_ID, CANSM_INSTANCE_ID, CANSM_SID_CONTROLLER_BUSOFF, CANSM_E_PARAM_CONTROLLER);
        return;
    }

    /* Enter bus-off recovery state */
    CanSM_BusOffPending = TRUE;
    CanSM_BusOffRecoveryTimer = 0;
    CanSM_BsmState = CANSM_BSM_S_BUSOFF_RECOVERY;

    /* Notify ComM of communication loss */
    CanSM_CurrentComMode = CANSM_NO_COMMUNICATION;
    (void)ComM_BusSM_ModeIndication(0U, COMM_NO_COMMUNICATION);
}

void CanSM_MainFunction(void)
{
    if (CanSM_State != CANSM_INIT)
    {
        return;
    }

    /* Handle bus-off recovery */
    if (CanSM_BsmState == CANSM_BSM_S_BUSOFF_RECOVERY)
    {
        CanSM_BusOffRecoveryTimer++;

        if (CanSM_BusOffRecoveryTimer >= CANSM_BUSOFF_RECOVERY_CYCLES)
        {
            CanSM_BusOffRecoveryCount++;

            if (CanSM_BusOffRecoveryCount > CANSM_MAX_BUSOFF_RECOVERY)
            {
                /* Max recovery attempts exceeded, stay in NOCOM */
                CanSM_BsmState = CANSM_BSM_S_NOCOM;
                CanSM_BusOffPending = FALSE;
                CanSM_BusOffRecoveryCount = 0;
                return;
            }

            /* Attempt recovery: restart controller */
            (void)Can_SetControllerMode(0U, CAN_CS_STOPPED);
            if (Can_SetControllerMode(0U, CAN_CS_STARTED) == E_OK)
            {
                /* Recovery successful */
                CanIf_ControllerModeIndication(0U, CANIF_CS_STARTED);
                CanSM_BsmState = CANSM_BSM_S_FULLCOM;
                CanSM_CurrentComMode = CANSM_FULL_COMMUNICATION;
                CanSM_BusOffPending = FALSE;
                CanSM_BusOffRecoveryCount = 0;
                (void)ComM_BusSM_ModeIndication(0U, COMM_FULL_COMMUNICATION);
            }
            else
            {
                /* Recovery failed, reset timer for next attempt */
                CanSM_BusOffRecoveryTimer = 0;
            }
        }
    }
}
