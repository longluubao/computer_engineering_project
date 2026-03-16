/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "ApBridge.h"
#include "Det.h"

/********************************************************************************************************/
/******************************************GlobalVaribles************************************************/
/********************************************************************************************************/

static ApBridge_StateType ApBridge_State = APBRIDGE_STATE_UNINIT;
static SoAd_ApBridgeStateType ApBridge_CurrentState = SOAD_AP_BRIDGE_NOT_READY;
static SoAd_ApBridgeStateType ApBridge_ForcedState = SOAD_AP_BRIDGE_NOT_READY;
static boolean ApBridge_ForcedStateValid = FALSE;
static uint16 ApBridge_HeartbeatAgeCycles = 0U;
static uint8 ApBridge_ServiceFailCounter = 0U;
static uint8 ApBridge_ServiceRecoveryCounter = 0U;
static boolean ApBridge_HasRecentHeartbeat = FALSE;

/********************************************************************************************************/
/*****************************************StaticFunctions************************************************/
/********************************************************************************************************/

static void ApBridge_UpdateSoAdState(SoAd_ApBridgeStateType State)
{
    if (ApBridge_CurrentState != State)
    {
        ApBridge_CurrentState = State;
        (void)SoAd_SetApBridgeState(State);
    }
}

/********************************************************************************************************/
/********************************************Functions***************************************************/
/********************************************************************************************************/

void ApBridge_Init(void)
{
    ApBridge_State = APBRIDGE_STATE_INIT;
    ApBridge_CurrentState = SOAD_AP_BRIDGE_NOT_READY;
    ApBridge_ForcedState = SOAD_AP_BRIDGE_NOT_READY;
    ApBridge_ForcedStateValid = FALSE;
    ApBridge_HeartbeatAgeCycles = 0U;
    ApBridge_ServiceFailCounter = 0U;
    ApBridge_ServiceRecoveryCounter = 0U;
    ApBridge_HasRecentHeartbeat = FALSE;
    (void)SoAd_SetApBridgeState(SOAD_AP_BRIDGE_NOT_READY);
}

void ApBridge_DeInit(void)
{
    ApBridge_State = APBRIDGE_STATE_UNINIT;
    ApBridge_CurrentState = SOAD_AP_BRIDGE_NOT_READY;
    ApBridge_ForcedStateValid = FALSE;
    ApBridge_HeartbeatAgeCycles = 0U;
    ApBridge_ServiceFailCounter = 0U;
    ApBridge_ServiceRecoveryCounter = 0U;
    ApBridge_HasRecentHeartbeat = FALSE;
}

void ApBridge_MainFunction(void)
{
    SoAd_ApBridgeStateType TargetState;

    if (ApBridge_State != APBRIDGE_STATE_INIT)
    {
        return;
    }

    if (ApBridge_HeartbeatAgeCycles < 0xFFFFU)
    {
        ApBridge_HeartbeatAgeCycles++;
    }

    if (ApBridge_ForcedStateValid == TRUE)
    {
        ApBridge_UpdateSoAdState(ApBridge_ForcedState);
        return;
    }

    if ((ApBridge_HasRecentHeartbeat == FALSE) ||
        (ApBridge_HeartbeatAgeCycles >= APBRIDGE_HEARTBEAT_TIMEOUT_CYCLES))
    {
        TargetState = SOAD_AP_BRIDGE_NOT_READY;
    }
    else if (ApBridge_ServiceFailCounter >= APBRIDGE_SERVICE_FAIL_THRESHOLD)
    {
        TargetState = SOAD_AP_BRIDGE_DEGRADED;
    }
    else
    {
        TargetState = SOAD_AP_BRIDGE_READY;
    }

    ApBridge_UpdateSoAdState(TargetState);
}

void ApBridge_ReportHeartbeat(boolean Success)
{
    if (ApBridge_State != APBRIDGE_STATE_INIT)
    {
        return;
    }

    if (Success == TRUE)
    {
        ApBridge_HasRecentHeartbeat = TRUE;
        ApBridge_HeartbeatAgeCycles = 0U;
    }
}

void ApBridge_ReportServiceStatus(boolean ServiceOk)
{
    if (ApBridge_State != APBRIDGE_STATE_INIT)
    {
        return;
    }

    if (ServiceOk == TRUE)
    {
        ApBridge_ServiceFailCounter = 0U;
        if (ApBridge_ServiceRecoveryCounter < 0xFFU)
        {
            ApBridge_ServiceRecoveryCounter++;
        }
    }
    else
    {
        ApBridge_ServiceRecoveryCounter = 0U;
        if (ApBridge_ServiceFailCounter < 0xFFU)
        {
            ApBridge_ServiceFailCounter++;
        }
    }

    if (ApBridge_ServiceRecoveryCounter >= APBRIDGE_SERVICE_RECOVERY_THRESHOLD)
    {
        ApBridge_ServiceFailCounter = 0U;
    }
}

Std_ReturnType ApBridge_SetForcedState(SoAd_ApBridgeStateType State)
{
    if (ApBridge_State != APBRIDGE_STATE_INIT)
    {
        return E_NOT_OK;
    }

    if (State > SOAD_AP_BRIDGE_DEGRADED)
    {
        return E_NOT_OK;
    }

    ApBridge_ForcedState = State;
    ApBridge_ForcedStateValid = TRUE;
    ApBridge_UpdateSoAdState(State);
    return E_OK;
}

Std_ReturnType ApBridge_GetStatus(ApBridge_StatusType* StatusPtr)
{
    if ((ApBridge_State != APBRIDGE_STATE_INIT) || (StatusPtr == NULL))
    {
        return E_NOT_OK;
    }

    StatusPtr->ApBridgeState = ApBridge_CurrentState;
    StatusPtr->HeartbeatAgeCycles = ApBridge_HeartbeatAgeCycles;
    StatusPtr->ServiceFailCounter = ApBridge_ServiceFailCounter;
    StatusPtr->ServiceRecoveryCounter = ApBridge_ServiceRecoveryCounter;
    return E_OK;
}
