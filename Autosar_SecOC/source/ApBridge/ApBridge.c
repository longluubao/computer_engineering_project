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

/* External API declarations (MISRA 8.4 visibility). */
void ApBridge_Init(void);
void ApBridge_DeInit(void);
void ApBridge_MainFunction(void);
void ApBridge_ReportHeartbeat(boolean Success);
void ApBridge_ReportServiceStatus(boolean ServiceOk);
Std_ReturnType ApBridge_SetForcedState(SoAd_ApBridgeStateType State);
Std_ReturnType ApBridge_GetStatus(ApBridge_StatusType* StatusPtr);

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
    boolean IsActive = FALSE;
    boolean IsForced = FALSE;

    if (ApBridge_State != APBRIDGE_STATE_INIT)
    {
        IsActive = FALSE;
    }
    else
    {
        IsActive = TRUE;
    }

    if (IsActive == TRUE)
    {
        if (ApBridge_HeartbeatAgeCycles < 0xFFFFU)
        {
            ApBridge_HeartbeatAgeCycles++;
        }

        if (ApBridge_ForcedStateValid == TRUE)
        {
            IsForced = TRUE;
            ApBridge_UpdateSoAdState(ApBridge_ForcedState);
        }

        if (IsForced == FALSE)
        {
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
    }
}

void ApBridge_ReportHeartbeat(boolean Success)
{
    boolean IsActive = FALSE;

    if (ApBridge_State == APBRIDGE_STATE_INIT)
    {
        IsActive = TRUE;
    }

    if ((IsActive == TRUE) && (Success == TRUE))
    {
        ApBridge_HasRecentHeartbeat = TRUE;
        ApBridge_HeartbeatAgeCycles = 0U;
    }
}

void ApBridge_ReportServiceStatus(boolean ServiceOk)
{
    if (ApBridge_State == APBRIDGE_STATE_INIT)
    {
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
}

Std_ReturnType ApBridge_SetForcedState(SoAd_ApBridgeStateType State)
{
    Std_ReturnType RetVal = E_OK;

    if (ApBridge_State != APBRIDGE_STATE_INIT)
    {
        RetVal = E_NOT_OK;
    }
    else if (State > SOAD_AP_BRIDGE_DEGRADED)
    {
        RetVal = E_NOT_OK;
    }
    else
    {
        ApBridge_ForcedState = State;
        ApBridge_ForcedStateValid = TRUE;
        ApBridge_UpdateSoAdState(State);
    }

    return RetVal;
}

Std_ReturnType ApBridge_GetStatus(ApBridge_StatusType* StatusPtr)
{
    Std_ReturnType RetVal = E_OK;

    if ((ApBridge_State != APBRIDGE_STATE_INIT) || (StatusPtr == NULL))
    {
        RetVal = E_NOT_OK;
    }
    else
    {
        StatusPtr->ApBridgeState = ApBridge_CurrentState;
        StatusPtr->HeartbeatAgeCycles = ApBridge_HeartbeatAgeCycles;
        StatusPtr->ServiceFailCounter = ApBridge_ServiceFailCounter;
        StatusPtr->ServiceRecoveryCounter = ApBridge_ServiceRecoveryCounter;
    }
    return RetVal;
}
