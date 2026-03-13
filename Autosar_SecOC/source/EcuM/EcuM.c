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
#include "EthIf.h"
#include "TcpIp.h"
#include "SoAd.h"
#include "SecOC.h"
#include "SecOC_Lcfg.h"
#include "Com.h"

/********************************************************************************************************/
/******************************************GlobalVaribles************************************************/
/********************************************************************************************************/

extern SecOC_ConfigType SecOC_Config;
static EcuM_StateType EcuM_State = ECUM_STATE_UNINIT;
static EcuM_ShutdownTargetType EcuM_ShutdownTarget = ECUM_SHUTDOWN_TARGET_OFF;
static EcuM_ShutdownTargetType EcuM_LastShutdownTarget = ECUM_SHUTDOWN_TARGET_OFF;
static EcuM_BootTargetType EcuM_BootTarget = ECUM_BOOT_TARGET_APP;
static EcuM_SleepModeType EcuM_SleepMode = ECUM_SLEEP_MODE_HALT;
static EcuM_WakeupSourceType EcuM_PendingWakeupEvents = 0U;
static EcuM_WakeupSourceType EcuM_ValidatedWakeupEvents = 0U;
static EcuM_WakeupSourceType EcuM_ExpiredWakeupEvents = 0U;
static uint16 EcuM_WakeupValidationCounter = 0U;
static uint8 EcuM_RunRequestCounter = 0U;
static uint8 EcuM_RunRequestMask = 0U;
static uint8 EcuM_PostRunRequestCounter = 0U;
static uint8 EcuM_PostRunRequestMask = 0U;
static EcuM_WakeupSourceType EcuM_WakeupEdgeMask = 0U;
static boolean EcuM_EthPathStarted = FALSE;
static EcuM_ResetCalloutType EcuM_ResetCallout = (EcuM_ResetCalloutType)0;
static EcuM_OffCalloutType EcuM_OffCallout = (EcuM_OffCalloutType)0;
static EcuM_WakeupValidationCalloutType EcuM_WakeupValidationCallout = (EcuM_WakeupValidationCalloutType)0;

static Std_ReturnType EcuM_InitEthCommunicationPath(void);
static void EcuM_DeInitEthCommunicationPath(void);
static void EcuM_MainFunctionEthCommunicationPath(void);
static void EcuM_StartComCommunicationPath(void);
static void EcuM_StopComCommunicationPath(void);
static Std_ReturnType EcuM_ExecuteResetCalloutHook(void);
static Std_ReturnType EcuM_ExecuteOffCalloutHook(void);
static boolean EcuM_IsWakeupSourceValidFromHardware(EcuM_WakeupSourceType WakeupSource);
static void EcuM_NotifyBswMState(EcuM_StateType State);

/********************************************************************************************************/
/********************************************Functions***************************************************/
/********************************************************************************************************/

static Std_ReturnType EcuM_InitEthCommunicationPath(void)
{
    EthIf_Init(NULL);

    if (EthIf_SetControllerMode(0U, ETH_MODE_ACTIVE) != E_OK)
    {
        return E_NOT_OK;
    }

    TcpIp_Init(NULL);
    SoAd_Init(NULL);
    EcuM_EthPathStarted = TRUE;

    return E_OK;
}

static void EcuM_DeInitEthCommunicationPath(void)
{
    if (EcuM_EthPathStarted == FALSE)
    {
        return;
    }

    /*
     * Extension point:
     * SoAd/EthIf currently expose no public de-init APIs in this codebase.
     * Keep shutdown deterministic using available public API only.
     */
    TcpIp_Shutdown();
    EcuM_EthPathStarted = FALSE;
}

static void EcuM_MainFunctionEthCommunicationPath(void)
{
    if (EcuM_EthPathStarted == FALSE)
    {
        return;
    }

    EthIf_MainFunctionRx();
    EthIf_MainFunctionTx();
    SoAd_MainFunctionTx();
    SoAd_MainFunctionRx();
}

static void EcuM_StartComCommunicationPath(void)
{
    Com_IpduGroupStart((Com_IpduGroupIdType)0U, TRUE);
}

static void EcuM_StopComCommunicationPath(void)
{
    Com_IpduGroupStop((Com_IpduGroupIdType)0U);
}

static Std_ReturnType EcuM_ExecuteResetCalloutHook(void)
{
    if (EcuM_ResetCallout != (EcuM_ResetCalloutType)0)
    {
        return EcuM_ResetCallout();
    }

    return E_OK;
}

static Std_ReturnType EcuM_ExecuteOffCalloutHook(void)
{
    if (EcuM_OffCallout != (EcuM_OffCalloutType)0)
    {
        return EcuM_OffCallout();
    }

    return E_OK;
}

static boolean EcuM_IsWakeupSourceValidFromHardware(EcuM_WakeupSourceType WakeupSource)
{
    if (EcuM_WakeupValidationCallout != (EcuM_WakeupValidationCalloutType)0)
    {
        return EcuM_WakeupValidationCallout(WakeupSource);
    }

    return TRUE;
}

static void EcuM_NotifyBswMState(EcuM_StateType State)
{
    (void)BswM_RequestMode((uint16)ECUM_MODULE_ID, (BswM_ModeType)State);
}

void EcuM_Init(const EcuM_ConfigType *ConfigPtr)
{
    if (ConfigPtr != (const EcuM_ConfigType*)0)
    {
        EcuM_ResetCallout = ConfigPtr->EcuMResetCallout;
        EcuM_OffCallout = ConfigPtr->EcuMOffCallout;
        EcuM_WakeupValidationCallout = ConfigPtr->EcuMWakeupValidationCallout;
    }

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
    EcuM_PendingWakeupEvents = 0U;
    EcuM_ValidatedWakeupEvents = 0U;
    EcuM_ExpiredWakeupEvents = 0U;
    EcuM_WakeupValidationCounter = 0U;
    EcuM_RunRequestCounter = 0U;
    EcuM_RunRequestMask = 0U;
    EcuM_PostRunRequestCounter = 0U;
    EcuM_PostRunRequestMask = 0U;
    EcuM_WakeupEdgeMask = 0U;
    EcuM_EthPathStarted = FALSE;
    EcuM_LastShutdownTarget = ECUM_SHUTDOWN_TARGET_OFF;
    EcuM_BootTarget = ECUM_BOOT_TARGET_APP;
    EcuM_SleepMode = ECUM_SLEEP_MODE_HALT;
    EcuM_NotifyBswMState(EcuM_State);
}

void EcuM_SetResetCallout(EcuM_ResetCalloutType ResetCallout)
{
    EcuM_ResetCallout = ResetCallout;
}

void EcuM_SetOffCallout(EcuM_OffCalloutType OffCallout)
{
    EcuM_OffCallout = OffCallout;
}

void EcuM_SetWakeupValidationCallout(EcuM_WakeupValidationCalloutType WakeupValidationCallout)
{
    EcuM_WakeupValidationCallout = WakeupValidationCallout;
}

Std_ReturnType EcuM_StartupTwo(void)
{
    if (EcuM_State != ECUM_STATE_STARTUP_ONE)
    {
        (void)Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_SID_STARTUP_TWO, ECUM_E_INVALID_STATE);
        return E_NOT_OK;
    }

    EcuM_State = ECUM_STATE_STARTUP_TWO;
    EcuM_NotifyBswMState(EcuM_State);

#if defined(LINUX) || defined(WINDOWS)
    if (EcuM_InitEthCommunicationPath() != E_OK)
    {
        EcuM_State = ECUM_STATE_STARTUP_ONE;
        return E_NOT_OK;
    }
#endif

    if (ComM_RequestComMode(0U, COMM_FULL_COMMUNICATION) != E_OK)
    {
#if defined(LINUX) || defined(WINDOWS)
        EcuM_DeInitEthCommunicationPath();
#endif
        EcuM_State = ECUM_STATE_STARTUP_ONE;
        return E_NOT_OK;
    }

    SecOC_Init(&SecOC_Config);
    Com_Init();
    EcuM_StartComCommunicationPath();

    EcuM_State = ECUM_STATE_RUN;
    EcuM_NotifyBswMState(EcuM_State);
    return E_OK;
}

Std_ReturnType EcuM_Shutdown(void)
{
    if ((EcuM_State == ECUM_STATE_UNINIT) || (EcuM_State == ECUM_STATE_SHUTDOWN))
    {
        (void)Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_SID_SHUTDOWN, ECUM_E_UNINIT);
        return E_NOT_OK;
    }

    if (EcuM_PostRunRequestCounter != 0U)
    {
        (void)Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_SID_SHUTDOWN, ECUM_E_INVALID_STATE);
        return E_NOT_OK;
    }

    EcuM_State = ECUM_STATE_SHUTDOWN;
    EcuM_NotifyBswMState(EcuM_State);
    EcuM_LastShutdownTarget = EcuM_ShutdownTarget;

    if (EcuM_ShutdownTarget == ECUM_SHUTDOWN_TARGET_SLEEP)
    {
        (void)ComM_RequestComMode(0U, COMM_NO_COMMUNICATION);
        EcuM_StopComCommunicationPath();
        EcuM_State = ECUM_STATE_SLEEP;
        EcuM_WakeupValidationCounter = 0U;
        EcuM_NotifyBswMState(EcuM_State);
    }
    else if (EcuM_ShutdownTarget == ECUM_SHUTDOWN_TARGET_RESET)
    {
        (void)ComM_RequestComMode(0U, COMM_NO_COMMUNICATION);
        EcuM_StopComCommunicationPath();
        SecOC_DeInit();
        Com_DeInit();
        ComM_DeInit();
        BswM_Deinit();
        CanNm_DeInit();
        CanSM_DeInit();
        Can_DeInit();
#if defined(LINUX) || defined(WINDOWS)
        EcuM_DeInitEthCommunicationPath();
#endif
        (void)EcuM_ExecuteResetCalloutHook();
        EcuM_State = ECUM_STATE_OFF;
    }
    else
    {
        (void)ComM_RequestComMode(0U, COMM_NO_COMMUNICATION);
        EcuM_StopComCommunicationPath();
        SecOC_DeInit();
        Com_DeInit();
        ComM_DeInit();
        BswM_Deinit();
        CanNm_DeInit();
        CanSM_DeInit();
        Can_DeInit();
#if defined(LINUX) || defined(WINDOWS)
        EcuM_DeInitEthCommunicationPath();
#endif
        (void)EcuM_ExecuteOffCalloutHook();
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

EcuM_ShutdownTargetType EcuM_GetLastShutdownTarget(void)
{
    return EcuM_LastShutdownTarget;
}

Std_ReturnType EcuM_SelectBootTarget(EcuM_BootTargetType Target)
{
    if (EcuM_State == ECUM_STATE_UNINIT)
    {
        (void)Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_SID_SELECT_BOOT_TARGET, ECUM_E_UNINIT);
        return E_NOT_OK;
    }

    if (Target > ECUM_BOOT_TARGET_BOOTLOADER)
    {
        (void)Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_SID_SELECT_BOOT_TARGET, ECUM_E_PARAM_INVALID);
        return E_NOT_OK;
    }

    EcuM_BootTarget = Target;
    return E_OK;
}

EcuM_BootTargetType EcuM_GetBootTarget(void)
{
    return EcuM_BootTarget;
}

Std_ReturnType EcuM_SelectSleepMode(EcuM_SleepModeType SleepMode)
{
    if (EcuM_State == ECUM_STATE_UNINIT)
    {
        (void)Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_SID_SELECT_SLEEP_MODE, ECUM_E_UNINIT);
        return E_NOT_OK;
    }

    if (SleepMode > ECUM_SLEEP_MODE_POLL)
    {
        (void)Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_SID_SELECT_SLEEP_MODE, ECUM_E_PARAM_INVALID);
        return E_NOT_OK;
    }

    EcuM_SleepMode = SleepMode;
    return E_OK;
}

Std_ReturnType EcuM_GoHalt(void)
{
    if (EcuM_SelectShutdownTarget(ECUM_SHUTDOWN_TARGET_SLEEP) != E_OK)
    {
        return E_NOT_OK;
    }

    if (EcuM_SelectSleepMode(ECUM_SLEEP_MODE_HALT) != E_OK)
    {
        return E_NOT_OK;
    }

    return EcuM_Shutdown();
}

Std_ReturnType EcuM_GoPoll(void)
{
    if (EcuM_SelectShutdownTarget(ECUM_SHUTDOWN_TARGET_SLEEP) != E_OK)
    {
        return E_NOT_OK;
    }

    if (EcuM_SelectSleepMode(ECUM_SLEEP_MODE_POLL) != E_OK)
    {
        return E_NOT_OK;
    }

    return EcuM_Shutdown();
}

Std_ReturnType EcuM_GoDown(void)
{
    if ((EcuM_State != ECUM_STATE_RUN) && (EcuM_State != ECUM_STATE_STARTUP_TWO))
    {
        (void)Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_SID_GO_DOWN, ECUM_E_INVALID_STATE);
        return E_NOT_OK;
    }

    if (EcuM_PostRunRequestCounter != 0U)
    {
        (void)Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_SID_GO_DOWN, ECUM_E_INVALID_STATE);
        return E_NOT_OK;
    }

    EcuM_KillAllRUNRequests();
    return EcuM_Shutdown();
}

Std_ReturnType EcuM_RequestRUN(uint8 User)
{
    if (EcuM_State == ECUM_STATE_UNINIT)
    {
        (void)Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_SID_REQUEST_RUN, ECUM_E_UNINIT);
        return E_NOT_OK;
    }

    if (User >= ECUM_MAX_RUN_USERS)
    {
        (void)Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_SID_REQUEST_RUN, ECUM_E_PARAM_INVALID);
        return E_NOT_OK;
    }

    if (EcuM_RunRequestCounter < 255U)
    {
        if ((EcuM_RunRequestMask & ((uint8)1U << User)) != 0U)
        {
            (void)Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_SID_REQUEST_RUN, ECUM_E_MULTIPLE_RUN_REQUESTS);
            return E_NOT_OK;
        }

        EcuM_RunRequestMask |= ((uint8)1U << User);
        EcuM_RunRequestCounter++;
    }

    return ComM_RequestComMode(0U, COMM_FULL_COMMUNICATION);
}

Std_ReturnType EcuM_ReleaseRUN(uint8 User)
{
    if (EcuM_State == ECUM_STATE_UNINIT)
    {
        (void)Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_SID_RELEASE_RUN, ECUM_E_UNINIT);
        return E_NOT_OK;
    }

    if (User >= ECUM_MAX_RUN_USERS)
    {
        (void)Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_SID_RELEASE_RUN, ECUM_E_PARAM_INVALID);
        return E_NOT_OK;
    }

    if (EcuM_RunRequestCounter == 0U)
    {
        (void)Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_SID_RELEASE_RUN, ECUM_E_MULTIPLE_RUN_REQUESTS);
        return E_NOT_OK;
    }

    if ((EcuM_RunRequestMask & ((uint8)1U << User)) == 0U)
    {
        (void)Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_SID_RELEASE_RUN, ECUM_E_MULTIPLE_RUN_REQUESTS);
        return E_NOT_OK;
    }

    EcuM_RunRequestMask &= (uint8)(~((uint8)1U << User));
    EcuM_RunRequestCounter--;
    if ((EcuM_RunRequestCounter == 0U) && (EcuM_PostRunRequestCounter == 0U))
    {
        return ComM_RequestComMode(0U, COMM_NO_COMMUNICATION);
    }

    return E_OK;
}

Std_ReturnType EcuM_RequestPOST_RUN(uint8 User)
{
    if (EcuM_State == ECUM_STATE_UNINIT)
    {
        (void)Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_SID_REQUEST_POST_RUN, ECUM_E_UNINIT);
        return E_NOT_OK;
    }

    if (User >= ECUM_MAX_RUN_USERS)
    {
        (void)Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_SID_REQUEST_POST_RUN, ECUM_E_PARAM_INVALID);
        return E_NOT_OK;
    }

    if ((EcuM_PostRunRequestMask & ((uint8)1U << User)) != 0U)
    {
        (void)Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_SID_REQUEST_POST_RUN, ECUM_E_MULTIPLE_POST_RUN_REQUESTS);
        return E_NOT_OK;
    }

    if (EcuM_PostRunRequestCounter < 255U)
    {
        EcuM_PostRunRequestMask |= ((uint8)1U << User);
        EcuM_PostRunRequestCounter++;
    }

    return ComM_RequestComMode(0U, COMM_FULL_COMMUNICATION);
}

Std_ReturnType EcuM_ReleasePOST_RUN(uint8 User)
{
    if (EcuM_State == ECUM_STATE_UNINIT)
    {
        (void)Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_SID_RELEASE_POST_RUN, ECUM_E_UNINIT);
        return E_NOT_OK;
    }

    if (User >= ECUM_MAX_RUN_USERS)
    {
        (void)Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_SID_RELEASE_POST_RUN, ECUM_E_PARAM_INVALID);
        return E_NOT_OK;
    }

    if ((EcuM_PostRunRequestCounter == 0U) ||
        ((EcuM_PostRunRequestMask & ((uint8)1U << User)) == 0U))
    {
        (void)Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_SID_RELEASE_POST_RUN, ECUM_E_MULTIPLE_POST_RUN_REQUESTS);
        return E_NOT_OK;
    }

    EcuM_PostRunRequestMask &= (uint8)(~((uint8)1U << User));
    EcuM_PostRunRequestCounter--;

    if ((EcuM_PostRunRequestCounter == 0U) && (EcuM_RunRequestCounter == 0U))
    {
        return ComM_RequestComMode(0U, COMM_NO_COMMUNICATION);
    }

    return E_OK;
}

void EcuM_KillAllRUNRequests(void)
{
    EcuM_RunRequestCounter = 0U;
    EcuM_RunRequestMask = 0U;
    if (EcuM_PostRunRequestCounter == 0U)
    {
        (void)ComM_RequestComMode(0U, COMM_NO_COMMUNICATION);
    }
}

void EcuM_SetWakeupEvent(EcuM_WakeupSourceType WakeupSource)
{
    EcuM_WakeupSourceType NewWakeupEdges = 0U;

    if (EcuM_State == ECUM_STATE_UNINIT)
    {
        (void)Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_SID_SET_WAKEUP_EVENT, ECUM_E_UNINIT);
        return;
    }

    if ((WakeupSource == 0U) || ((WakeupSource & (~ECUM_WKSOURCE_ALL)) != 0U))
    {
        (void)Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_SID_SET_WAKEUP_EVENT, ECUM_E_PARAM_INVALID);
        return;
    }

    NewWakeupEdges = WakeupSource & (~EcuM_WakeupEdgeMask);
    if (NewWakeupEdges == 0U)
    {
        return;
    }

    EcuM_WakeupEdgeMask |= NewWakeupEdges;
    EcuM_PendingWakeupEvents |= NewWakeupEdges;
    EcuM_ExpiredWakeupEvents &= (~NewWakeupEdges);
}

void EcuM_ValidateWakeupEvent(EcuM_WakeupSourceType WakeupSource)
{
    EcuM_WakeupSourceType ValidWakeupSources = 0U;

    if (EcuM_State == ECUM_STATE_UNINIT)
    {
        (void)Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_SID_VALIDATE_WAKEUP_EVENT, ECUM_E_UNINIT);
        return;
    }

    if ((WakeupSource == 0U) || ((WakeupSource & (~ECUM_WKSOURCE_ALL)) != 0U))
    {
        (void)Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_SID_VALIDATE_WAKEUP_EVENT, ECUM_E_PARAM_INVALID);
        return;
    }

    ValidWakeupSources = WakeupSource & EcuM_PendingWakeupEvents;

    if (ValidWakeupSources != 0U)
    {
        EcuM_ValidatedWakeupEvents |= ValidWakeupSources;
        EcuM_PendingWakeupEvents &= (~ValidWakeupSources);
    }
}

void EcuM_ClearWakeupEvent(EcuM_WakeupSourceType WakeupSource)
{
    if (EcuM_State == ECUM_STATE_UNINIT)
    {
        (void)Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_SID_CLEAR_WAKEUP_EVENT, ECUM_E_UNINIT);
        return;
    }

    if ((WakeupSource == 0U) || ((WakeupSource & (~ECUM_WKSOURCE_ALL)) != 0U))
    {
        (void)Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_SID_CLEAR_WAKEUP_EVENT, ECUM_E_PARAM_INVALID);
        return;
    }

    EcuM_PendingWakeupEvents &= (~WakeupSource);
    EcuM_ValidatedWakeupEvents &= (~WakeupSource);
    EcuM_ExpiredWakeupEvents &= (~WakeupSource);
    EcuM_WakeupEdgeMask &= (~WakeupSource);
}

void EcuM_CheckWakeup(EcuM_WakeupSourceType WakeupSource)
{
    EcuM_WakeupSourceType WakeupMask = ECUM_WKSOURCE_POWER;

    if (EcuM_State == ECUM_STATE_UNINIT)
    {
        (void)Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_SID_CHECK_WAKEUP, ECUM_E_UNINIT);
        return;
    }

    if ((WakeupSource == 0U) || ((WakeupSource & (~ECUM_WKSOURCE_ALL)) != 0U))
    {
        (void)Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_SID_CHECK_WAKEUP, ECUM_E_PARAM_INVALID);
        return;
    }

    while (WakeupMask != 0U)
    {
        if ((WakeupSource & WakeupMask) != 0U)
        {
            if (EcuM_IsWakeupSourceValidFromHardware(WakeupMask) == TRUE)
            {
                EcuM_ValidateWakeupEvent(WakeupMask);
            }
            else
            {
                EcuM_PendingWakeupEvents &= (~WakeupMask);
                EcuM_ExpiredWakeupEvents |= WakeupMask;
            }
        }

        WakeupMask <<= 1U;
    }
}

EcuM_WakeupSourceType EcuM_GetPendingWakeupEvents(void)
{
    return EcuM_PendingWakeupEvents;
}

EcuM_WakeupSourceType EcuM_GetValidatedWakeupEvents(void)
{
    return EcuM_ValidatedWakeupEvents;
}

EcuM_WakeupSourceType EcuM_GetExpiredWakeupEvents(void)
{
    return EcuM_ExpiredWakeupEvents;
}

EcuM_WakeupStatusType EcuM_GetWakeupStatus(EcuM_WakeupSourceType WakeupSource)
{
    if ((WakeupSource == 0U) || ((WakeupSource & (~ECUM_WKSOURCE_ALL)) != 0U))
    {
        (void)Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_SID_GET_WAKEUP_STATUS, ECUM_E_UNKNOWN_WAKEUP_SOURCE);
        return ECUM_WAKEUP_NONE;
    }

    if ((EcuM_ValidatedWakeupEvents & WakeupSource) != 0U)
    {
        return ECUM_WAKEUP_VALIDATED;
    }

    if ((EcuM_PendingWakeupEvents & WakeupSource) != 0U)
    {
        return ECUM_WAKEUP_PENDING;
    }

    if ((EcuM_ExpiredWakeupEvents & WakeupSource) != 0U)
    {
        return ECUM_WAKEUP_EXPIRED;
    }

    return ECUM_WAKEUP_NONE;
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
        Com_MainFunctionTx();
        Com_MainFunctionRx();
        ComM_MainFunction();
        CanSM_MainFunction();
        CanNm_MainFunction();
#if defined(LINUX) || defined(WINDOWS)
        EcuM_MainFunctionEthCommunicationPath();
#endif
        BswM_MainFunction();
    }
    else if (EcuM_State == ECUM_STATE_SLEEP)
    {
        if (EcuM_SleepMode == ECUM_SLEEP_MODE_POLL)
        {
            Can_MainFunction_Read();
#if defined(LINUX) || defined(WINDOWS)
            EthIf_MainFunctionRx();
#endif
        }

        if (EcuM_PendingWakeupEvents != 0U)
        {
            EcuM_CheckWakeup(EcuM_PendingWakeupEvents);
            if (EcuM_WakeupValidationCounter < 65535U)
            {
                EcuM_WakeupValidationCounter++;
            }
            if (EcuM_WakeupValidationCounter >= ECUM_WAKEUP_VALIDATION_TIMEOUT_MAINCYCLES)
            {
                EcuM_ExpiredWakeupEvents |= EcuM_PendingWakeupEvents;
                EcuM_PendingWakeupEvents = 0U;
                EcuM_WakeupValidationCounter = 0U;
            }
        }

        if (EcuM_ValidatedWakeupEvents != 0U)
        {
            if (ComM_RequestComMode(0U, COMM_FULL_COMMUNICATION) == E_OK)
            {
                EcuM_StartComCommunicationPath();
                EcuM_ClearWakeupEvent(EcuM_ValidatedWakeupEvents);
                EcuM_WakeupValidationCounter = 0U;
                EcuM_State = ECUM_STATE_RUN;
                EcuM_NotifyBswMState(EcuM_State);
            }
        }
    }
}
