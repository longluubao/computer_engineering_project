/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "ApBridge/ApBridge.h"
#include "BswM/BswM.h"
#include "Can/Can.h"
#include "Can/CanIF.h"
#include "Can/CanTP.h"
#include "CanNm/CanNm.h"
#include "CanSM/CanSM.h"
#include "Com/Com.h"
#include "ComM/ComM.h"
#include "Csm/Csm.h"
#include "Dcm/Dcm.h"
#include "Dem/Dem.h"
#include "Det/Det.h"
#include "EcuM/EcuM.h"
#include "EthIf/EthIf.h"
#include "EthSM/EthSM.h"
#include "MemIf/MemIf.h"
#include "NvM/NvM.h"
#include "SecOC/SecOC.h"
#include "SecOC/SecOC_Lcfg.h"
#include "SoAd/SoAd.h"
#include "SoAd/SoAd_PQC.h"
#include "TcpIp/TcpIp.h"
#include <string.h>

/********************************************************************************************************/
/******************************************GlobalVaribles************************************************/
/********************************************************************************************************/

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
/* cppcheck-suppress misra-c2012-11.4 */
static EcuM_ResetCalloutType EcuM_ResetCallout = (EcuM_ResetCalloutType)0;
/* cppcheck-suppress misra-c2012-11.4 */
static EcuM_OffCalloutType EcuM_OffCallout = (EcuM_OffCalloutType)0;
/* cppcheck-suppress misra-c2012-11.4 */
static EcuM_WakeupValidationCalloutType EcuM_WakeupValidationCallout = (EcuM_WakeupValidationCalloutType)0;
static BswM_GatewayHealthType EcuM_LastGatewayHealthSnapshot = {BSWM_GATEWAY_PROFILE_NORMAL, 0U, 0U, 0U, 0U};
static boolean EcuM_HasGatewayHealthSnapshot = FALSE;

static Std_ReturnType EcuM_InitEthCommunicationPath(void);
static void EcuM_DeInitEthCommunicationPath(void);
static void EcuM_MainFunctionEthCommunicationPath(void);
static void EcuM_StartComCommunicationPath(void);
static void EcuM_StopComCommunicationPath(void);
static Std_ReturnType EcuM_ExecuteResetCalloutHook(void);
static Std_ReturnType EcuM_ExecuteOffCalloutHook(void);
static boolean EcuM_IsWakeupSourceValidFromHardware(EcuM_WakeupSourceType WakeupSource);
static void EcuM_NotifyBswMState(EcuM_StateType State);
static void EcuM_ExecuteNvMReadAllPhase(void);
static void EcuM_ExecuteNvMWriteAllPhase(void);
static Std_ReturnType EcuM_WaitNvMBlockCompletion(NvM_BlockIdType BlockId, uint16 TimeoutCycles, NvM_RequestResultType* ResultPtr);
static uint32 EcuM_EncodePersistentStateWord(void);
static void EcuM_DecodePersistentStateWord(uint32 PackedState);
static void EcuM_LoadPersistentStateFromNvM(void);
static void EcuM_PersistStateToNvM(void);
static void EcuM_PersistGatewayHealthToNvM(void);

/********************************************************************************************************/
/********************************************Functions***************************************************/
/********************************************************************************************************/

static Std_ReturnType EcuM_InitEthCommunicationPath(void)
{
#if (SOAD_TCPIP_PAYLOAD_BACKEND == SOAD_TCPIP_PAYLOAD_BACKEND_ETHIF)
    EthIf_Init(NULL);

    if (EthIf_SetControllerMode(0U, ETH_MODE_ACTIVE) != E_OK)
    {
        return E_NOT_OK;
    }
#endif

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

    SoAd_DeInit();
    TcpIp_Shutdown();
#if (SOAD_TCPIP_PAYLOAD_BACKEND == SOAD_TCPIP_PAYLOAD_BACKEND_ETHIF)
    EthIf_DeInit();
#endif
    EcuM_EthPathStarted = FALSE;
}

static void EcuM_MainFunctionEthCommunicationPath(void)
{
    if (EcuM_EthPathStarted == FALSE)
    {
        return;
    }

#if (SOAD_TCPIP_PAYLOAD_BACKEND == SOAD_TCPIP_PAYLOAD_BACKEND_ETHIF)
    EthIf_MainFunctionRx();
    EthIf_MainFunctionTx();
#endif
    SoAd_MainFunctionTx();
    SoAd_MainFunctionRx();
}

static void EcuM_StartComCommunicationPath(void)
{
    Com_IpduGroupIdType GroupId;

    for (GroupId = 0U; GroupId < (Com_IpduGroupIdType)COM_NUM_OF_IPDU_GROUPS; GroupId++)
    {
        Com_IpduGroupStart(GroupId, TRUE);
    }
}

static void EcuM_StopComCommunicationPath(void)
{
    Com_IpduGroupIdType GroupId;

    for (GroupId = 0U; GroupId < (Com_IpduGroupIdType)COM_NUM_OF_IPDU_GROUPS; GroupId++)
    {
        Com_IpduGroupStop(GroupId);
    }
}

static Std_ReturnType EcuM_ExecuteResetCalloutHook(void)
{
    /* cppcheck-suppress misra-c2012-11.4 */
    if (EcuM_ResetCallout != (EcuM_ResetCalloutType)0)
    {
        return EcuM_ResetCallout();
    }

    return E_OK;
}

static Std_ReturnType EcuM_ExecuteOffCalloutHook(void)
{
    /* cppcheck-suppress misra-c2012-11.4 */
    if (EcuM_OffCallout != (EcuM_OffCalloutType)0)
    {
        return EcuM_OffCallout();
    }

    return E_OK;
}

static boolean EcuM_IsWakeupSourceValidFromHardware(EcuM_WakeupSourceType WakeupSource)
{
    /* cppcheck-suppress misra-c2012-11.4 */
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

static void EcuM_ExecuteNvMReadAllPhase(void)
{
    NvM_RequestResultType NvMResult = NVM_REQ_PENDING;
    uint16 WaitCounter = 0U;

    if (NvM_ReadAll() != E_OK)
    {
        return;
    }

    while (WaitCounter < 256U)
    {
        NvM_MainFunction();
        if ((NvM_GetErrorStatus(NVM_BLOCK_ID_DEM_DTC_STORAGE, &NvMResult) == E_OK) &&
            (NvMResult != NVM_REQ_PENDING))
        {
            break;
        }
        WaitCounter++;
    }
}

static void EcuM_ExecuteNvMWriteAllPhase(void)
{
    NvM_RequestResultType NvMResult = NVM_REQ_PENDING;
    uint16 WaitCounter = 0U;

    while ((NvM_WriteAll() != E_OK) && (WaitCounter < 64U))
    {
        NvM_MainFunction();
        WaitCounter++;
    }

    if (WaitCounter >= 64U)
    {
        return;
    }

    WaitCounter = 0U;
    while (WaitCounter < 256U)
    {
        NvM_MainFunction();
        if ((NvM_GetErrorStatus(NVM_BLOCK_ID_DEM_DTC_STORAGE, &NvMResult) == E_OK) &&
            (NvMResult != NVM_REQ_PENDING))
        {
            break;
        }
        WaitCounter++;
    }
}

static Std_ReturnType EcuM_WaitNvMBlockCompletion(NvM_BlockIdType BlockId, uint16 TimeoutCycles, NvM_RequestResultType* ResultPtr)
{
    uint16 WaitCounter = 0U;
    NvM_RequestResultType LocalResult = NVM_REQ_PENDING;

    while (WaitCounter < TimeoutCycles)
    {
        NvM_MainFunction();
        if (NvM_GetErrorStatus(BlockId, &LocalResult) != E_OK)
        {
            return E_NOT_OK;
        }
        if (LocalResult != NVM_REQ_PENDING)
        {
            if (ResultPtr != NULL)
            {
                *ResultPtr = LocalResult;
            }
            return E_OK;
        }
        WaitCounter++;
    }

    return E_NOT_OK;
}

static uint32 EcuM_EncodePersistentStateWord(void)
{
    uint32 PackedState = 0UL;

    PackedState |= ((uint32)EcuM_ShutdownTarget & 0xFFUL);
    PackedState |= (((uint32)EcuM_BootTarget & 0xFFUL) << 8U);
    PackedState |= (((uint32)EcuM_SleepMode & 0xFFUL) << 16U);

    return PackedState;
}

static void EcuM_DecodePersistentStateWord(uint32 PackedState)
{
    /* cppcheck-suppress misra-c2012-10.8 */
    EcuM_ShutdownTargetType PersistedShutdownTarget = (EcuM_ShutdownTargetType)(PackedState & 0xFFUL);
    /* cppcheck-suppress misra-c2012-10.8 */
    EcuM_BootTargetType PersistedBootTarget = (EcuM_BootTargetType)((PackedState >> 8U) & 0xFFUL);
    /* cppcheck-suppress misra-c2012-10.8 */
    EcuM_SleepModeType PersistedSleepMode = (EcuM_SleepModeType)((PackedState >> 16U) & 0xFFUL);

    if (PersistedShutdownTarget <= ECUM_SHUTDOWN_TARGET_OFF)
    {
        EcuM_ShutdownTarget = PersistedShutdownTarget;
    }

    if (PersistedBootTarget <= ECUM_BOOT_TARGET_BOOTLOADER)
    {
        EcuM_BootTarget = PersistedBootTarget;
    }

    if (PersistedSleepMode <= ECUM_SLEEP_MODE_POLL)
    {
        EcuM_SleepMode = PersistedSleepMode;
    }
}

static void EcuM_LoadPersistentStateFromNvM(void)
{
    uint32 PersistedStateWord = 0UL;
    uint32 PersistedHistoryWord = 0UL;
    BswM_GatewayHealthType PersistedGatewayHealth;
    NvM_RequestResultType NvMResult = NVM_REQ_PENDING;

    if ((NvM_ReadBlock(NVM_BLOCK_ID_ECUM_DATASET, &PersistedStateWord) == E_OK) &&
        (EcuM_WaitNvMBlockCompletion(NVM_BLOCK_ID_ECUM_DATASET, 128U, &NvMResult) == E_OK) &&
        (NvMResult == NVM_REQ_OK))
    {
        EcuM_DecodePersistentStateWord(PersistedStateWord);
    }

    (void)NvM_SetDataIndex(NVM_BLOCK_ID_ECUM_DATASET, 1U);
    if ((NvM_ReadBlock(NVM_BLOCK_ID_ECUM_DATASET, &PersistedHistoryWord) == E_OK) &&
        (EcuM_WaitNvMBlockCompletion(NVM_BLOCK_ID_ECUM_DATASET, 128U, &NvMResult) == E_OK) &&
        (NvMResult == NVM_REQ_OK))
    {
        /* cppcheck-suppress misra-c2012-10.8 */
        EcuM_LastShutdownTarget = (EcuM_ShutdownTargetType)(PersistedHistoryWord & 0xFFUL);
    }
    (void)NvM_SetDataIndex(NVM_BLOCK_ID_ECUM_DATASET, 0U);

    if ((NvM_ReadBlock(NVM_BLOCK_ID_GATEWAY_HEALTH, &PersistedGatewayHealth) == E_OK) &&
        (EcuM_WaitNvMBlockCompletion(NVM_BLOCK_ID_GATEWAY_HEALTH, 128U, &NvMResult) == E_OK) &&
        (NvMResult == NVM_REQ_OK))
    {
        (void)BswM_SetGatewayHealth(&PersistedGatewayHealth);
        EcuM_LastGatewayHealthSnapshot = PersistedGatewayHealth;
        EcuM_HasGatewayHealthSnapshot = TRUE;
    }
}

static void EcuM_PersistStateToNvM(void)
{
    uint32 PersistedStateWord = EcuM_EncodePersistentStateWord();
    uint32 PersistedHistoryWord = ((uint32)EcuM_LastShutdownTarget & 0xFFUL);

    (void)NvM_SetDataIndex(NVM_BLOCK_ID_ECUM_DATASET, 0U);
    (void)NvM_SetRamBlockStatus(NVM_BLOCK_ID_ECUM_DATASET, TRUE);
    (void)NvM_WriteBlock(NVM_BLOCK_ID_ECUM_DATASET, &PersistedStateWord);
    (void)NvM_SetDataIndex(NVM_BLOCK_ID_ECUM_DATASET, 1U);
    (void)NvM_SetRamBlockStatus(NVM_BLOCK_ID_ECUM_DATASET, TRUE);
    (void)NvM_WriteBlock(NVM_BLOCK_ID_ECUM_DATASET, &PersistedHistoryWord);
    (void)NvM_SetDataIndex(NVM_BLOCK_ID_ECUM_DATASET, 0U);
}

static void EcuM_PersistGatewayHealthToNvM(void)
{
    BswM_GatewayHealthType CurrentGatewayHealth;

    if (BswM_GetGatewayHealth(&CurrentGatewayHealth) != E_OK)
    {
        return;
    }

    /* cppcheck-suppress misra-c2012-21.16 */
    if ((EcuM_HasGatewayHealthSnapshot == FALSE) ||
        (memcmp(&CurrentGatewayHealth, &EcuM_LastGatewayHealthSnapshot, sizeof(BswM_GatewayHealthType)) != 0))
    {
        (void)NvM_SetRamBlockStatus(NVM_BLOCK_ID_GATEWAY_HEALTH, TRUE);
        (void)NvM_WriteBlock(NVM_BLOCK_ID_GATEWAY_HEALTH, &CurrentGatewayHealth);
        EcuM_LastGatewayHealthSnapshot = CurrentGatewayHealth;
        EcuM_HasGatewayHealthSnapshot = TRUE;
    }
}

void EcuM_Init(const EcuM_ConfigType *ConfigPtr)
{
    /* cppcheck-suppress misra-c2012-11.4 */
    if (ConfigPtr != (const EcuM_ConfigType*)0)
    {
        EcuM_ResetCallout = ConfigPtr->EcuMResetCallout;
        EcuM_OffCallout = ConfigPtr->EcuMOffCallout;
        EcuM_WakeupValidationCallout = ConfigPtr->EcuMWakeupValidationCallout;
    }

    /* Phase 1: Basic SW initialization per AUTOSAR SWS_EcuM_02811 */
    Det_Init(NULL);
    Det_Start();
    MemIf_Init();
    NvM_Init();
    EcuM_ExecuteNvMReadAllPhase();
    Dem_Init();
    Dcm_Init(NULL);
    Csm_Init(NULL);

    /* Phase 2: Driver and mode-manager initialization */
    Can_Init(NULL);
    BswM_Init(NULL);
    ComM_Init();
    EthSM_Init(NULL);
    CanSM_Init();
    CanNm_Init();

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
    EcuM_HasGatewayHealthSnapshot = FALSE;
    EcuM_LoadPersistentStateFromNvM();
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
        EcuM_NotifyBswMState(EcuM_State);
        return E_NOT_OK;
    }
#endif

    /* Ensure CanIf callbacks are registered before communication starts. */
    CanIf_Init();
    CanTp_Init();

    if (ComM_RequestComMode(0U, COMM_FULL_COMMUNICATION) != E_OK)
    {
#if defined(LINUX) || defined(WINDOWS)
        EcuM_DeInitEthCommunicationPath();
#endif
        EcuM_State = ECUM_STATE_STARTUP_ONE;
        EcuM_NotifyBswMState(EcuM_State);
        return E_NOT_OK;
    }

    SecOC_Init(&SecOC_Config);
    Com_Init();
    ApBridge_Init();
    if (SoAd_PQC_Init() != E_OK)
    {
        ApBridge_ReportServiceStatus(FALSE);
    }
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
    EcuM_PersistGatewayHealthToNvM();
    EcuM_PersistStateToNvM();

    if (EcuM_ShutdownTarget == ECUM_SHUTDOWN_TARGET_SLEEP)
    {
        EcuM_ExecuteNvMWriteAllPhase();
        (void)ComM_RequestComMode(0U, COMM_NO_COMMUNICATION);
        EcuM_StopComCommunicationPath();
        EcuM_State = ECUM_STATE_SLEEP;
        EcuM_WakeupValidationCounter = 0U;
        EcuM_NotifyBswMState(EcuM_State);
    }
    else if (EcuM_ShutdownTarget == ECUM_SHUTDOWN_TARGET_RESET)
    {
        EcuM_ExecuteNvMWriteAllPhase();
        (void)ComM_RequestComMode(0U, COMM_NO_COMMUNICATION);
        EcuM_StopComCommunicationPath();
        Dem_Shutdown();
        Dcm_DeInit();
        Csm_DeInit();
        ApBridge_DeInit();
        SoAd_PQC_DeInit();
        SecOC_DeInit();
        Com_DeInit();
        ComM_DeInit();
        BswM_Deinit();
        CanNm_DeInit();
        CanSM_DeInit();
        CanTp_Shutdown();
        Can_DeInit();
#if defined(LINUX) || defined(WINDOWS)
        EcuM_DeInitEthCommunicationPath();
#endif
        EthSM_DeInit();
        (void)EcuM_ExecuteResetCalloutHook();
        EcuM_State = ECUM_STATE_OFF;
    }
    else
    {
        EcuM_ExecuteNvMWriteAllPhase();
        (void)ComM_RequestComMode(0U, COMM_NO_COMMUNICATION);
        EcuM_StopComCommunicationPath();
        Dem_Shutdown();
        Dcm_DeInit();
        Csm_DeInit();
        ApBridge_DeInit();
        SoAd_PQC_DeInit();
        SecOC_DeInit();
        Com_DeInit();
        ComM_DeInit();
        BswM_Deinit();
        CanNm_DeInit();
        CanSM_DeInit();
        CanTp_Shutdown();
        Can_DeInit();
#if defined(LINUX) || defined(WINDOWS)
        EcuM_DeInitEthCommunicationPath();
#endif
        EthSM_DeInit();
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
    EcuM_PersistStateToNvM();
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
    EcuM_PersistStateToNvM();
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
    EcuM_PersistStateToNvM();
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

void EcuM_MainFunctionState(void)
{
    if (EcuM_State == ECUM_STATE_UNINIT)
    {
        return;
    }

    if (EcuM_State == ECUM_STATE_SLEEP)
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

void EcuM_MainFunctionComStack(void)
{
    if (EcuM_State != ECUM_STATE_RUN)
    {
        return;
    }

    Can_MainFunction_Write();
    Can_MainFunction_Read();
    CanTp_MainFunctionTx();
    CanTp_MainFunctionRx();
    Com_MainFunctionTx();
    Com_MainFunctionRx();
}

void EcuM_MainFunctionSecurityStack(void)
{
    if (EcuM_State != ECUM_STATE_RUN)
    {
        return;
    }

    SecOC_MainFunctionRx();
    SecOC_MainFunctionTx();
    Csm_MainFunction();
    SoAd_PQC_MainFunction();
}

void EcuM_MainFunctionNetworkStack(void)
{
    if (EcuM_State != ECUM_STATE_RUN)
    {
        return;
    }

    ComM_MainFunction();
    CanSM_MainFunction();
    CanNm_MainFunction();
    ApBridge_MainFunction();
#if defined(LINUX) || defined(WINDOWS)
    EcuM_MainFunctionEthCommunicationPath();
#endif
    EthSM_MainFunction();
    BswM_MainFunction();
}

void EcuM_MainFunctionDiagnosticsStack(void)
{
    if (EcuM_State != ECUM_STATE_RUN)
    {
        return;
    }

    Dcm_MainFunction();
    Dem_MainFunction();
}

void EcuM_MainFunctionNvStack(void)
{
    if (EcuM_State != ECUM_STATE_RUN)
    {
        return;
    }

    NvM_MainFunction();
    EcuM_PersistGatewayHealthToNvM();
}

void EcuM_MainFunction(void)
{
    EcuM_MainFunctionState();
}
