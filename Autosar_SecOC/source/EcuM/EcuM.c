/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "EcuM.h"
#include "Det.h"
#include "Dem.h"
#include "Can.h"
#include "CanSM.h"
#include "CanNm.h"
#include "BswM.h"
#include "ComM.h"
#include "SecOC.h"
#include "SecOC_Lcfg.h"

#if defined(LINUX)
#include "ethernet.h"
#elif defined(WINDOWS)
#include "ethernet_windows.h"
#endif

/********************************************************************************************************/
/******************************************GlobalVaribles************************************************/
/********************************************************************************************************/

extern SecOC_ConfigType SecOC_Config;
static EcuM_StateType EcuM_State = ECUM_STATE_UNINIT;
static EcuM_ShutdownTargetType EcuM_ShutdownTarget = ECUM_SHUTDOWN_TARGET_OFF;
static EcuM_WakeupSourceType EcuM_PendingWakeupEvents = 0U;

/********************************************************************************************************/
/********************************************Functions***************************************************/
/********************************************************************************************************/

void EcuM_Init(const EcuM_ConfigType *ConfigPtr)
{
    (void)ConfigPtr;

    /* Phase 1: Basic SW initialization per AUTOSAR SWS_EcuM_02811 */
    Det_Init(NULL);
    Det_Start();
    Dem_Init();

    /* Phase 2: Driver initialization */
    Can_Init(NULL);
    CanSM_Init();
    CanNm_Init();

    /* Phase 3: BSW mode and communication managers */
    BswM_Init(NULL);
    ComM_Init();

    EcuM_State = ECUM_STATE_STARTUP_ONE;
}

Std_ReturnType EcuM_StartupTwo(void)
{
    if (EcuM_State != ECUM_STATE_STARTUP_ONE)
    {
        (void)Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_SID_STARTUP_TWO, ECUM_E_INVALID_STATE);
        return E_NOT_OK;
    }

#if defined(LINUX) || defined(WINDOWS)
    ethernet_init();
#endif

    if (ComM_RequestComMode(0U, COMM_FULL_COMMUNICATION) != E_OK)
    {
        return E_NOT_OK;
    }

    SecOC_Init(&SecOC_Config);

    EcuM_State = ECUM_STATE_RUN;
    return E_OK;
}

Std_ReturnType EcuM_Shutdown(void)
{
    if ((EcuM_State == ECUM_STATE_UNINIT) || (EcuM_State == ECUM_STATE_SHUTDOWN))
    {
        (void)Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_SID_SHUTDOWN, ECUM_E_UNINIT);
        return E_NOT_OK;
    }

    EcuM_State = ECUM_STATE_SHUTDOWN;

    (void)ComM_RequestComMode(0U, COMM_NO_COMMUNICATION);
    SecOC_DeInit();
    BswM_Deinit();
    Can_DeInit();

    if (EcuM_ShutdownTarget == ECUM_SHUTDOWN_TARGET_SLEEP)
    {
        EcuM_State = ECUM_STATE_SLEEP;
    }
    else
    {
        EcuM_State = ECUM_STATE_OFF;
    }

    return E_OK;
}

EcuM_StateType EcuM_GetState(void)
{
    return EcuM_State;
}

Std_ReturnType EcuM_SelectShutdownTarget(EcuM_ShutdownTargetType Target)
{
    if (EcuM_State == ECUM_STATE_UNINIT)
    {
        (void)Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_SID_SELECT_SHUTDOWN, ECUM_E_UNINIT);
        return E_NOT_OK;
    }

    if (Target > ECUM_SHUTDOWN_TARGET_OFF)
    {
        (void)Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_SID_SELECT_SHUTDOWN, ECUM_E_PARAM_INVALID);
        return E_NOT_OK;
    }

    EcuM_ShutdownTarget = Target;
    return E_OK;
}

EcuM_ShutdownTargetType EcuM_GetShutdownTarget(void)
{
    return EcuM_ShutdownTarget;
}

void EcuM_SetWakeupEvent(EcuM_WakeupSourceType WakeupSource)
{
    if (EcuM_State == ECUM_STATE_UNINIT)
    {
        (void)Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_SID_SET_WAKEUP_EVENT, ECUM_E_UNINIT);
        return;
    }

    EcuM_PendingWakeupEvents |= WakeupSource;
}

EcuM_WakeupSourceType EcuM_GetPendingWakeupEvents(void)
{
    return EcuM_PendingWakeupEvents;
}

void EcuM_MainFunction(void)
{
    if (EcuM_State == ECUM_STATE_UNINIT)
    {
        return;
    }

    /* In RUN state, delegate to BswM for periodic mode arbitration */
    if (EcuM_State == ECUM_STATE_RUN)
    {
        BswM_MainFunction();
    }
}
