/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "BswM.h"
#include "Det.h"
#include "ComM.h"
#include "EcuM.h"
#include "CanSM.h"
#include "CanNm.h"
#include "EthSM.h"
#include "SoAd.h"
#include "SecOC.h"
#include "Dem.h"

#define BSWM_CHANNEL_ID_DEFAULT            ((uint8)0)
#define BSWM_MODE_INVALID                  ((BswM_ModeType)0xFFFFU)
#define BSWM_ROUTING_GROUP_NORMAL          ((SoAd_RoutingGroupIdType)0U)
#define BSWM_ROUTING_GROUP_DIAG            ((SoAd_RoutingGroupIdType)1U)

#define BSWM_GATEWAY_CAN_FAULT_THRESHOLD   ((uint8)3U)
#define BSWM_GATEWAY_ETH_FAULT_THRESHOLD   ((uint8)3U)
#define BSWM_GATEWAY_SECOC_FAIL_THRESHOLD  ((uint8)5U)
#define BSWM_GATEWAY_RECOVERY_THRESHOLD    ((uint8)5U)

#define BSWM_SECOC_STATUS_OK               ((BswM_ModeType)0U)
#define BSWM_SECOC_STATUS_FAILURE          ((BswM_ModeType)1U)

#define BSWM_DEM_EVENT_GATEWAY_DEGRADED    ((Dem_EventIdType)1U)
#define BSWM_DEM_EVENT_GATEWAY_DIAG_ONLY   ((Dem_EventIdType)2U)

#define BSWM_RULE_ID_ECUM_STARTUP          ((uint8)0)
#define BSWM_RULE_ID_ECUM_SHUTDOWN_PATH    ((uint8)1)
#define BSWM_RULE_ID_COMM_FULL_REQ         ((uint8)2)
#define BSWM_RULE_ID_COMM_NO_REQ           ((uint8)3)
#define BSWM_RULE_ID_GATEWAY_PATH_DOWN     ((uint8)4)
#define BSWM_RULE_COUNT                    ((uint8)5)

typedef enum
{
    BSWM_GATEWAY_COMMAND_NONE = 0,
    BSWM_GATEWAY_COMMAND_STARTUP_HOLD,
    BSWM_GATEWAY_COMMAND_NO_COMM,
    BSWM_GATEWAY_COMMAND_PROFILE_NORMAL,
    BSWM_GATEWAY_COMMAND_PROFILE_DEGRADED,
    BSWM_GATEWAY_COMMAND_PROFILE_DIAG_ONLY
} BswM_GatewayCommandType;

/********************************************************************************************************/
/******************************************GlobalVaribles************************************************/
/********************************************************************************************************/

static BswM_StateType BswM_State = BSWM_UNINIT;
static BswM_ModeRequestType BswM_ModeRequests[BSWM_MAX_MODE_REQUESTS];
static uint8 BswM_PendingRequestCount = 0U;
static BswM_RuleResultType BswM_LastRuleResults[BSWM_MAX_RULES];
static boolean BswM_IsLastRuleResultValid[BSWM_MAX_RULES];
static BswM_GatewayProfileType BswM_GatewayProfile = BSWM_GATEWAY_PROFILE_NORMAL;
static uint8 BswM_CanFaultCounter = 0U;
static uint8 BswM_EthFaultCounter = 0U;
static uint8 BswM_SecOCFailCounter = 0U;
static uint8 BswM_RecoveryCounter = 0U;
static BswM_GatewayCommandType BswM_LastGatewayCommand = BSWM_GATEWAY_COMMAND_NONE;

static BswM_RuleResultType BswM_EvaluateRule_EcuMStartup(void);
static BswM_RuleResultType BswM_EvaluateRule_EcuMShutdownPath(void);
static BswM_RuleResultType BswM_EvaluateRule_ComMFullReq(void);
static BswM_RuleResultType BswM_EvaluateRule_ComMNoReq(void);
static BswM_RuleResultType BswM_EvaluateRule_GatewayPathDown(void);
static void BswM_Action_AllowCommFull(void);
static void BswM_Action_AllowCommNo(void);
static void BswM_Action_GatewayComFull(void);
static void BswM_Action_GatewayComNo(void);
static void BswM_Action_Startup(void);
static void BswM_Action_Shutdown(void);
static boolean BswM_GetRequesterMode(uint16 RequesterId, BswM_ModeType *ModePtr);
static boolean BswM_IsModeValueValid(uint16 RequesterId, BswM_ModeType RequestedMode);
static void BswM_ExecuteActionList(BswM_ActionListIdType ActionListId);
static void BswM_UpdateGatewayHealth(void);
static void BswM_ApplyGatewayProfile(BswM_GatewayProfileType TargetProfile);
static void BswM_Action_ProfileNormal(void);
static void BswM_Action_ProfileDegraded(void);
static void BswM_Action_ProfileDiagOnly(void);
static BswM_GatewayCommandType BswM_ComputeGatewayCommand(void);
static boolean BswM_IsApReady(void);

static const BswM_RuleType BswM_Rules[BSWM_RULE_COUNT] =
{
    {BswM_EvaluateRule_EcuMStartup, BSWM_ACTIONLIST_STARTUP, TRUE},
    {BswM_EvaluateRule_EcuMShutdownPath, BSWM_ACTIONLIST_SHUTDOWN, TRUE},
    {BswM_EvaluateRule_ComMFullReq, BSWM_ACTIONLIST_COMM_FULL, TRUE},
    {BswM_EvaluateRule_ComMNoReq, BSWM_ACTIONLIST_COMM_NO, TRUE},
    {BswM_EvaluateRule_GatewayPathDown, BSWM_ACTIONLIST_COMM_NO, TRUE}
};

static const BswM_ActionListType BswM_ActionLists[BSWM_MAX_ACTION_LISTS] =
{
    [BSWM_ACTIONLIST_COMM_FULL] = {{BswM_Action_AllowCommFull, BswM_Action_GatewayComFull, (BswM_ActionFuncType)0, (BswM_ActionFuncType)0}, 2U},
    [BSWM_ACTIONLIST_COMM_NO] = {{BswM_Action_AllowCommNo, BswM_Action_GatewayComNo, (BswM_ActionFuncType)0, (BswM_ActionFuncType)0}, 2U},
    [BSWM_ACTIONLIST_STARTUP] = {{BswM_Action_Startup, BswM_Action_GatewayComFull, (BswM_ActionFuncType)0, (BswM_ActionFuncType)0}, 2U},
    [BSWM_ACTIONLIST_SHUTDOWN] = {{BswM_Action_GatewayComNo, BswM_Action_Shutdown, (BswM_ActionFuncType)0, (BswM_ActionFuncType)0}, 2U}
};

static boolean BswM_GetRequesterMode(uint16 RequesterId, BswM_ModeType *ModePtr)
{
    uint8 idx;

    if (ModePtr == NULL)
    {
        (void)Det_ReportError(BSWM_MODULE_ID, BSWM_INSTANCE_ID, BSWM_SID_MAIN_FUNCTION, BSWM_E_NULL_POINTER);
        return FALSE;
    }

    for (idx = 0U; idx < BSWM_MAX_MODE_REQUESTS; idx++)
    {
        if ((BswM_ModeRequests[idx].IsValid == TRUE) &&
            (BswM_ModeRequests[idx].RequesterId == RequesterId))
        {
            *ModePtr = BswM_ModeRequests[idx].RequestedMode;
            return TRUE;
        }
    }

    return FALSE;
}

static boolean BswM_IsModeValueValid(uint16 RequesterId, BswM_ModeType RequestedMode)
{
    if (RequesterId == (uint16)ECUM_MODULE_ID)
    {
        return (RequestedMode <= (BswM_ModeType)ECUM_STATE_OFF) ? TRUE : FALSE;
    }

    if (RequesterId == (uint16)COMM_MODULE_ID)
    {
        return (RequestedMode <= (BswM_ModeType)COMM_FULL_COMMUNICATION) ? TRUE : FALSE;
    }

    if (RequesterId == (uint16)CANSM_MODULE_ID)
    {
        return (RequestedMode <= (BswM_ModeType)CANSM_FULL_COMMUNICATION) ? TRUE : FALSE;
    }

    if (RequesterId == (uint16)CANNM_MODULE_ID)
    {
        return (RequestedMode <= (BswM_ModeType)CANNM_MODE_NETWORK) ? TRUE : FALSE;
    }

    if (RequesterId == (uint16)ETHSM_MODULE_ID)
    {
        return (RequestedMode <= (BswM_ModeType)ETHSM_STATE_ONLINE) ? TRUE : FALSE;
    }

    if (RequesterId == (uint16)SECOC_MODULE_ID)
    {
        return (RequestedMode <= BSWM_SECOC_STATUS_FAILURE) ? TRUE : FALSE;
    }

    if (RequesterId == BSWM_REQUESTER_ID_COMM_DESIRED)
    {
        return (RequestedMode <= (BswM_ModeType)COMM_FULL_COMMUNICATION) ? TRUE : FALSE;
    }

    if (RequesterId == BSWM_REQUESTER_ID_AP_READY)
    {
        return (RequestedMode <= BSWM_AP_STATE_DEGRADED) ? TRUE : FALSE;
    }

    return FALSE;
}

static boolean BswM_IsApReady(void)
{
    BswM_ModeType ApReadyMode = BSWM_AP_STATE_NOT_READY;

    if (BswM_GetRequesterMode(BSWM_REQUESTER_ID_AP_READY, &ApReadyMode) == FALSE)
    {
        return FALSE;
    }

    return (ApReadyMode == BSWM_AP_STATE_READY) ? TRUE : FALSE;
}

static BswM_GatewayCommandType BswM_ComputeGatewayCommand(void)
{
    BswM_ModeType EcuMMode = BSWM_MODE_INVALID;
    BswM_ModeType ComMDesiredMode = BSWM_MODE_INVALID;
    BswM_ModeType ComMMode = BSWM_MODE_INVALID;
    BswM_ModeType ApBridgeMode = BSWM_AP_STATE_NOT_READY;

    if (BswM_GetRequesterMode((uint16)ECUM_MODULE_ID, &EcuMMode) == FALSE)
    {
        return BSWM_GATEWAY_COMMAND_NONE;
    }

    if (BswM_GetRequesterMode(BSWM_REQUESTER_ID_COMM_DESIRED, &ComMDesiredMode) == FALSE)
    {
        if (BswM_GetRequesterMode((uint16)COMM_MODULE_ID, &ComMMode) == TRUE)
        {
            ComMDesiredMode = ComMMode;
        }
    }

    if (((EcuM_StateType)EcuMMode == ECUM_STATE_SHUTDOWN) ||
        ((EcuM_StateType)EcuMMode == ECUM_STATE_SLEEP) ||
        ((EcuM_StateType)EcuMMode == ECUM_STATE_OFF))
    {
        return BSWM_GATEWAY_COMMAND_NO_COMM;
    }

    if (((EcuM_StateType)EcuMMode == ECUM_STATE_STARTUP_ONE) ||
        ((EcuM_StateType)EcuMMode == ECUM_STATE_STARTUP_TWO))
    {
        return BSWM_GATEWAY_COMMAND_STARTUP_HOLD;
    }

    if ((EcuM_StateType)EcuMMode != ECUM_STATE_RUN)
    {
        return BSWM_GATEWAY_COMMAND_NO_COMM;
    }

    if (((ComM_ModeType)ComMDesiredMode == COMM_NO_COMMUNICATION) ||
        ((ComM_ModeType)ComMDesiredMode == COMM_SILENT_COMMUNICATION))
    {
        return BSWM_GATEWAY_COMMAND_NO_COMM;
    }

    if (BswM_GetRequesterMode(BSWM_REQUESTER_ID_AP_READY, &ApBridgeMode) == TRUE)
    {
        if (ApBridgeMode == BSWM_AP_STATE_NOT_READY)
        {
            return BSWM_GATEWAY_COMMAND_NO_COMM;
        }
        if (ApBridgeMode == BSWM_AP_STATE_DEGRADED)
        {
            return BSWM_GATEWAY_COMMAND_PROFILE_DIAG_ONLY;
        }
    }

    if (BswM_GatewayProfile == BSWM_GATEWAY_PROFILE_DIAG_ONLY)
    {
        return BSWM_GATEWAY_COMMAND_PROFILE_DIAG_ONLY;
    }

    if (BswM_GatewayProfile == BSWM_GATEWAY_PROFILE_DEGRADED)
    {
        return BSWM_GATEWAY_COMMAND_PROFILE_DEGRADED;
    }

    return BSWM_GATEWAY_COMMAND_PROFILE_NORMAL;
}

static BswM_RuleResultType BswM_EvaluateRule_EcuMStartup(void)
{
    BswM_ModeType EcuMMode = BSWM_MODE_INVALID;

    if (BswM_GetRequesterMode((uint16)ECUM_MODULE_ID, &EcuMMode) == FALSE)
    {
        return BSWM_CONDITION_FALSE;
    }

    if (((EcuM_StateType)EcuMMode == ECUM_STATE_STARTUP_TWO) ||
        ((EcuM_StateType)EcuMMode == ECUM_STATE_RUN))
    {
        return BSWM_CONDITION_TRUE;
    }

    return BSWM_CONDITION_FALSE;
}

static BswM_RuleResultType BswM_EvaluateRule_EcuMShutdownPath(void)
{
    BswM_ModeType EcuMMode = BSWM_MODE_INVALID;

    if (BswM_GetRequesterMode((uint16)ECUM_MODULE_ID, &EcuMMode) == FALSE)
    {
        return BSWM_CONDITION_FALSE;
    }

    if (((EcuM_StateType)EcuMMode == ECUM_STATE_SHUTDOWN) ||
        ((EcuM_StateType)EcuMMode == ECUM_STATE_SLEEP) ||
        ((EcuM_StateType)EcuMMode == ECUM_STATE_OFF))
    {
        return BSWM_CONDITION_TRUE;
    }

    return BSWM_CONDITION_FALSE;
}

static BswM_RuleResultType BswM_EvaluateRule_ComMFullReq(void)
{
    BswM_ModeType ComMMode = BSWM_MODE_INVALID;
    BswM_ModeType EcuMMode = BSWM_MODE_INVALID;

    if (BswM_GetRequesterMode((uint16)COMM_MODULE_ID, &ComMMode) == FALSE)
    {
        return BSWM_CONDITION_FALSE;
    }

    if (BswM_GetRequesterMode((uint16)ECUM_MODULE_ID, &EcuMMode) == FALSE)
    {
        return BSWM_CONDITION_FALSE;
    }

    if ((((EcuM_StateType)EcuMMode == ECUM_STATE_STARTUP_TWO) ||
         ((EcuM_StateType)EcuMMode == ECUM_STATE_RUN)) &&
        ((ComM_ModeType)ComMMode == COMM_FULL_COMMUNICATION))
    {
        return BSWM_CONDITION_TRUE;
    }

    return BSWM_CONDITION_FALSE;
}

static BswM_RuleResultType BswM_EvaluateRule_ComMNoReq(void)
{
    BswM_ModeType ComMMode = BSWM_MODE_INVALID;
    BswM_ModeType EcuMMode = BSWM_MODE_INVALID;

    if (BswM_GetRequesterMode((uint16)COMM_MODULE_ID, &ComMMode) == FALSE)
    {
        return BSWM_CONDITION_FALSE;
    }

    if (BswM_GetRequesterMode((uint16)ECUM_MODULE_ID, &EcuMMode) == FALSE)
    {
        return BSWM_CONDITION_FALSE;
    }

    if ((((ComM_ModeType)ComMMode == COMM_NO_COMMUNICATION) ||
         ((ComM_ModeType)ComMMode == COMM_SILENT_COMMUNICATION)) &&
        ((EcuM_StateType)EcuMMode == ECUM_STATE_RUN))
    {
        return BSWM_CONDITION_TRUE;
    }

    return BSWM_CONDITION_FALSE;
}

static BswM_RuleResultType BswM_EvaluateRule_GatewayPathDown(void)
{
    BswM_ModeType EcuMMode = BSWM_MODE_INVALID;
    BswM_ModeType ComMMode = BSWM_MODE_INVALID;
    BswM_ModeType CanSmMode = BSWM_MODE_INVALID;
    BswM_ModeType CanNmMode = BSWM_MODE_INVALID;
    BswM_ModeType EthSmState = BSWM_MODE_INVALID;

    if ((BswM_GetRequesterMode((uint16)ECUM_MODULE_ID, &EcuMMode) == FALSE) ||
        (BswM_GetRequesterMode((uint16)COMM_MODULE_ID, &ComMMode) == FALSE))
    {
        return BSWM_CONDITION_FALSE;
    }

    if (((EcuM_StateType)EcuMMode != ECUM_STATE_RUN) ||
        ((ComM_ModeType)ComMMode != COMM_FULL_COMMUNICATION))
    {
        return BSWM_CONDITION_FALSE;
    }

    if ((BswM_GetRequesterMode((uint16)CANSM_MODULE_ID, &CanSmMode) == TRUE) &&
        ((CanSM_ComModeType)CanSmMode != CANSM_FULL_COMMUNICATION))
    {
        return BSWM_CONDITION_TRUE;
    }

    if ((BswM_GetRequesterMode((uint16)CANNM_MODULE_ID, &CanNmMode) == TRUE) &&
        ((CanNm_ModeType)CanNmMode != CANNM_MODE_NETWORK))
    {
        return BSWM_CONDITION_TRUE;
    }

    if ((BswM_GetRequesterMode((uint16)ETHSM_MODULE_ID, &EthSmState) == TRUE) &&
        ((EthSM_NetworkModeStateType)EthSmState == ETHSM_STATE_OFFLINE))
    {
        return BSWM_CONDITION_TRUE;
    }

    return BSWM_CONDITION_FALSE;
}

static void BswM_Action_AllowCommFull(void)
{
    if (ComM_RequestComMode(BSWM_CHANNEL_ID_DEFAULT, COMM_FULL_COMMUNICATION) != E_OK)
    {
        (void)Det_ReportError(BSWM_MODULE_ID, BSWM_INSTANCE_ID, BSWM_SID_MAIN_FUNCTION, BSWM_E_PARAM_INVALID);
    }
}

static void BswM_Action_AllowCommNo(void)
{
    if (ComM_RequestComMode(BSWM_CHANNEL_ID_DEFAULT, COMM_NO_COMMUNICATION) != E_OK)
    {
        (void)Det_ReportError(BSWM_MODULE_ID, BSWM_INSTANCE_ID, BSWM_SID_MAIN_FUNCTION, BSWM_E_PARAM_INVALID);
    }
}

static void BswM_Action_GatewayComFull(void)
{
    if (BswM_IsApReady() == TRUE)
    {
        if (SoAd_EnableRouting(BSWM_ROUTING_GROUP_NORMAL) != E_OK)
        {
            (void)Det_ReportError(BSWM_MODULE_ID, BSWM_INSTANCE_ID, BSWM_SID_MAIN_FUNCTION, BSWM_E_PARAM_INVALID);
        }
    }
    else
    {
        (void)SoAd_DisableRouting(BSWM_ROUTING_GROUP_NORMAL);
    }
    (void)SoAd_DisableRouting(BSWM_ROUTING_GROUP_DIAG);
    (void)EthSM_RequestComMode(BSWM_CHANNEL_ID_DEFAULT, COMM_FULL_COMMUNICATION);
}

static void BswM_Action_GatewayComNo(void)
{
    if (SoAd_DisableRouting(BSWM_ROUTING_GROUP_NORMAL) != E_OK)
    {
        (void)Det_ReportError(BSWM_MODULE_ID, BSWM_INSTANCE_ID, BSWM_SID_MAIN_FUNCTION, BSWM_E_PARAM_INVALID);
    }
    (void)SoAd_DisableRouting(BSWM_ROUTING_GROUP_DIAG);
    (void)EthSM_RequestComMode(BSWM_CHANNEL_ID_DEFAULT, COMM_NO_COMMUNICATION);
}

static void BswM_Action_Startup(void)
{
    BswM_Action_AllowCommFull();
}

static void BswM_Action_Shutdown(void)
{
    BswM_Action_AllowCommNo();
}

static void BswM_Action_ProfileNormal(void)
{
    BswM_Action_AllowCommFull();
    (void)EthSM_RequestComMode(BSWM_CHANNEL_ID_DEFAULT, COMM_FULL_COMMUNICATION);
    if (BswM_IsApReady() == TRUE)
    {
        (void)SoAd_EnableRouting(BSWM_ROUTING_GROUP_NORMAL);
    }
    else
    {
        (void)SoAd_DisableRouting(BSWM_ROUTING_GROUP_NORMAL);
    }
    (void)SoAd_DisableRouting(BSWM_ROUTING_GROUP_DIAG);
}

static void BswM_Action_ProfileDegraded(void)
{
    BswM_Action_AllowCommFull();
    (void)EthSM_RequestComMode(BSWM_CHANNEL_ID_DEFAULT, COMM_FULL_COMMUNICATION);
    if (BswM_IsApReady() == TRUE)
    {
        (void)SoAd_EnableRouting(BSWM_ROUTING_GROUP_NORMAL);
    }
    else
    {
        (void)SoAd_DisableRouting(BSWM_ROUTING_GROUP_NORMAL);
    }
    (void)SoAd_EnableRouting(BSWM_ROUTING_GROUP_DIAG);
}

static void BswM_Action_ProfileDiagOnly(void)
{
    BswM_Action_AllowCommFull();
    (void)EthSM_RequestComMode(BSWM_CHANNEL_ID_DEFAULT, COMM_FULL_COMMUNICATION);
    (void)SoAd_DisableRouting(BSWM_ROUTING_GROUP_NORMAL);
    (void)SoAd_EnableRouting(BSWM_ROUTING_GROUP_DIAG);
}

static void BswM_ApplyGatewayProfile(BswM_GatewayProfileType TargetProfile)
{
    if (TargetProfile == BswM_GatewayProfile)
    {
        return;
    }

    BswM_GatewayProfile = TargetProfile;

    if (BswM_GatewayProfile == BSWM_GATEWAY_PROFILE_NORMAL)
    {
        BswM_Action_ProfileNormal();
        (void)Dem_SetEventStatus(BSWM_DEM_EVENT_GATEWAY_DEGRADED, DEM_EVENT_STATUS_PASSED);
        (void)Dem_SetEventStatus(BSWM_DEM_EVENT_GATEWAY_DIAG_ONLY, DEM_EVENT_STATUS_PASSED);
    }
    else if (BswM_GatewayProfile == BSWM_GATEWAY_PROFILE_DEGRADED)
    {
        BswM_Action_ProfileDegraded();
        (void)Dem_SetEventStatus(BSWM_DEM_EVENT_GATEWAY_DEGRADED, DEM_EVENT_STATUS_FAILED);
        (void)Dem_SetEventStatus(BSWM_DEM_EVENT_GATEWAY_DIAG_ONLY, DEM_EVENT_STATUS_PASSED);
    }
    else
    {
        BswM_Action_ProfileDiagOnly();
        (void)Dem_SetEventStatus(BSWM_DEM_EVENT_GATEWAY_DIAG_ONLY, DEM_EVENT_STATUS_FAILED);
    }
}

static void BswM_UpdateGatewayHealth(void)
{
    BswM_ModeType EcuMMode = BSWM_MODE_INVALID;
    BswM_ModeType ComMMode = BSWM_MODE_INVALID;
    BswM_ModeType CanSmMode = BSWM_MODE_INVALID;
    BswM_ModeType EthSmState = BSWM_MODE_INVALID;
    BswM_ModeType SecOCStatus = BSWM_SECOC_STATUS_OK;
    boolean IsGatewayDemanded = FALSE;
    boolean IsCanFaultActive = FALSE;
    boolean IsEthFaultActive = FALSE;
    boolean IsSecOCFaultActive = FALSE;
    BswM_GatewayProfileType TargetProfile = BSWM_GATEWAY_PROFILE_NORMAL;

    if ((BswM_GetRequesterMode((uint16)ECUM_MODULE_ID, &EcuMMode) == TRUE) &&
        (BswM_GetRequesterMode((uint16)COMM_MODULE_ID, &ComMMode) == TRUE))
    {
        if (((EcuM_StateType)EcuMMode == ECUM_STATE_RUN) &&
            ((ComM_ModeType)ComMMode == COMM_FULL_COMMUNICATION))
        {
            IsGatewayDemanded = TRUE;
        }
    }

    if ((BswM_GetRequesterMode((uint16)CANSM_MODULE_ID, &CanSmMode) == TRUE) &&
        ((CanSM_ComModeType)CanSmMode != CANSM_FULL_COMMUNICATION))
    {
        IsCanFaultActive = TRUE;
    }

    if ((BswM_GetRequesterMode((uint16)ETHSM_MODULE_ID, &EthSmState) == TRUE) &&
        ((EthSM_NetworkModeStateType)EthSmState == ETHSM_STATE_OFFLINE))
    {
        IsEthFaultActive = TRUE;
    }

    if ((BswM_GetRequesterMode((uint16)SECOC_MODULE_ID, &SecOCStatus) == TRUE) &&
        (SecOCStatus == BSWM_SECOC_STATUS_FAILURE))
    {
        IsSecOCFaultActive = TRUE;
    }

    if (IsGatewayDemanded == TRUE)
    {
        if (IsCanFaultActive == TRUE)
        {
            if (BswM_CanFaultCounter < 255U)
            {
                BswM_CanFaultCounter++;
            }
        }
        else
        {
            BswM_CanFaultCounter = 0U;
        }

        if (IsEthFaultActive == TRUE)
        {
            if (BswM_EthFaultCounter < 255U)
            {
                BswM_EthFaultCounter++;
            }
        }
        else
        {
            BswM_EthFaultCounter = 0U;
        }

        if (IsSecOCFaultActive == TRUE)
        {
            if (BswM_SecOCFailCounter < 255U)
            {
                BswM_SecOCFailCounter++;
            }
        }
        else
        {
            BswM_SecOCFailCounter = 0U;
        }

        if ((BswM_SecOCFailCounter >= BSWM_GATEWAY_SECOC_FAIL_THRESHOLD) ||
            ((BswM_CanFaultCounter >= BSWM_GATEWAY_CAN_FAULT_THRESHOLD) &&
             (BswM_EthFaultCounter >= BSWM_GATEWAY_ETH_FAULT_THRESHOLD)))
        {
            TargetProfile = BSWM_GATEWAY_PROFILE_DIAG_ONLY;
            BswM_RecoveryCounter = 0U;
        }
        else if ((BswM_CanFaultCounter >= BSWM_GATEWAY_CAN_FAULT_THRESHOLD) ||
                 (BswM_EthFaultCounter >= BSWM_GATEWAY_ETH_FAULT_THRESHOLD))
        {
            TargetProfile = BSWM_GATEWAY_PROFILE_DEGRADED;
            BswM_RecoveryCounter = 0U;
        }
        else
        {
            if (BswM_RecoveryCounter < 255U)
            {
                BswM_RecoveryCounter++;
            }
            if (BswM_RecoveryCounter >= BSWM_GATEWAY_RECOVERY_THRESHOLD)
            {
                TargetProfile = BSWM_GATEWAY_PROFILE_NORMAL;
            }
            else
            {
                TargetProfile = BswM_GatewayProfile;
            }
        }
    }
    else
    {
        BswM_CanFaultCounter = 0U;
        BswM_EthFaultCounter = 0U;
        BswM_SecOCFailCounter = 0U;
        BswM_RecoveryCounter = 0U;
        TargetProfile = BswM_GatewayProfile;
    }

    BswM_ApplyGatewayProfile(TargetProfile);
}

static void BswM_ExecuteActionList(BswM_ActionListIdType ActionListId)
{
    uint8 ActionIdx;
    const BswM_ActionListType *ActionListPtr;

    if (ActionListId >= BSWM_MAX_ACTION_LISTS)
    {
        (void)Det_ReportError(BSWM_MODULE_ID, BSWM_INSTANCE_ID, BSWM_SID_MAIN_FUNCTION, BSWM_E_PARAM_INVALID);
        return;
    }

    ActionListPtr = &BswM_ActionLists[(uint8)ActionListId];

    for (ActionIdx = 0U; ActionIdx < ActionListPtr->ActionCount; ActionIdx++)
    {
        if (ActionListPtr->Actions[ActionIdx] != (BswM_ActionFuncType)0)
        {
            ActionListPtr->Actions[ActionIdx]();
        }
    }
}

/********************************************************************************************************/
/********************************************Functions***************************************************/
/********************************************************************************************************/

void BswM_Init(const BswM_ConfigType *ConfigPtr)
{
    uint8 idx;

    (void)ConfigPtr;

    for (idx = 0U; idx < BSWM_MAX_MODE_REQUESTS; idx++)
    {
        BswM_ModeRequests[idx].RequesterId = 0U;
        BswM_ModeRequests[idx].RequestedMode = 0U;
        BswM_ModeRequests[idx].IsValid = FALSE;
    }

    for (idx = 0U; idx < BSWM_MAX_RULES; idx++)
    {
        BswM_LastRuleResults[idx] = BSWM_CONDITION_FALSE;
        BswM_IsLastRuleResultValid[idx] = FALSE;
    }

    BswM_GatewayProfile = BSWM_GATEWAY_PROFILE_NORMAL;
    BswM_CanFaultCounter = 0U;
    BswM_EthFaultCounter = 0U;
    BswM_SecOCFailCounter = 0U;
    BswM_RecoveryCounter = 0U;
    BswM_LastGatewayCommand = BSWM_GATEWAY_COMMAND_NONE;

    BswM_PendingRequestCount = 0U;
    BswM_State = BSWM_INIT;
    (void)BswM_RequestMode(BSWM_REQUESTER_ID_AP_READY, BSWM_AP_READY_FALSE);
    (void)BswM_RequestMode(BSWM_REQUESTER_ID_COMM_DESIRED, (BswM_ModeType)COMM_NO_COMMUNICATION);
}

void BswM_Deinit(void)
{
    uint8 idx;

    if (BswM_State != BSWM_INIT)
    {
        (void)Det_ReportError(BSWM_MODULE_ID, BSWM_INSTANCE_ID, BSWM_SID_DEINIT, BSWM_E_UNINIT);
        return;
    }

    for (idx = 0U; idx < BSWM_MAX_MODE_REQUESTS; idx++)
    {
        BswM_ModeRequests[idx].RequesterId = 0U;
        BswM_ModeRequests[idx].RequestedMode = 0U;
        BswM_ModeRequests[idx].IsValid = FALSE;
    }

    for (idx = 0U; idx < BSWM_MAX_RULES; idx++)
    {
        BswM_LastRuleResults[idx] = BSWM_CONDITION_FALSE;
        BswM_IsLastRuleResultValid[idx] = FALSE;
    }

    BswM_GatewayProfile = BSWM_GATEWAY_PROFILE_NORMAL;
    BswM_CanFaultCounter = 0U;
    BswM_EthFaultCounter = 0U;
    BswM_SecOCFailCounter = 0U;
    BswM_RecoveryCounter = 0U;
    BswM_LastGatewayCommand = BSWM_GATEWAY_COMMAND_NONE;

    BswM_PendingRequestCount = 0U;
    BswM_State = BSWM_UNINIT;
}

Std_ReturnType BswM_RequestMode(uint16 RequesterId, BswM_ModeType RequestedMode)
{
    uint8 idx;

    if (BswM_State != BSWM_INIT)
    {
        (void)Det_ReportError(BSWM_MODULE_ID, BSWM_INSTANCE_ID, BSWM_SID_REQUEST_MODE, BSWM_E_UNINIT);
        return E_NOT_OK;
    }

    if (BswM_IsModeValueValid(RequesterId, RequestedMode) == FALSE)
    {
        (void)Det_ReportError(BSWM_MODULE_ID, BSWM_INSTANCE_ID, BSWM_SID_REQUEST_MODE, BSWM_E_PARAM_INVALID);
        return E_NOT_OK;
    }

    /* Look for existing request from this requester */
    for (idx = 0U; idx < BSWM_MAX_MODE_REQUESTS; idx++)
    {
        if ((BswM_ModeRequests[idx].IsValid == TRUE) &&
            (BswM_ModeRequests[idx].RequesterId == RequesterId))
        {
            BswM_ModeRequests[idx].RequestedMode = RequestedMode;
            return E_OK;
        }
    }

    /* Add new request */
    if (BswM_PendingRequestCount >= BSWM_MAX_MODE_REQUESTS)
    {
        (void)Det_ReportError(BSWM_MODULE_ID, BSWM_INSTANCE_ID, BSWM_SID_REQUEST_MODE, BSWM_E_REQ_MODE_OUT_OF_RANGE);
        return E_NOT_OK;
    }

    for (idx = 0U; idx < BSWM_MAX_MODE_REQUESTS; idx++)
    {
        if (BswM_ModeRequests[idx].IsValid == FALSE)
        {
            BswM_ModeRequests[idx].RequesterId = RequesterId;
            BswM_ModeRequests[idx].RequestedMode = RequestedMode;
            BswM_ModeRequests[idx].IsValid = TRUE;
            BswM_PendingRequestCount++;
            return E_OK;
        }
    }

    return E_NOT_OK;
}

BswM_ModeType BswM_GetCurrentMode(uint16 RequesterId)
{
    uint8 idx;

    if (BswM_State != BSWM_INIT)
    {
        (void)Det_ReportError(BSWM_MODULE_ID, BSWM_INSTANCE_ID, BSWM_SID_GET_CURRENT_MODE, BSWM_E_UNINIT);
        return (BswM_ModeType)0U;
    }

    if ((RequesterId != (uint16)ECUM_MODULE_ID) &&
        (RequesterId != (uint16)COMM_MODULE_ID) &&
        (RequesterId != (uint16)CANSM_MODULE_ID) &&
        (RequesterId != (uint16)CANNM_MODULE_ID) &&
        (RequesterId != (uint16)ETHSM_MODULE_ID) &&
        (RequesterId != (uint16)SECOC_MODULE_ID) &&
        (RequesterId != BSWM_REQUESTER_ID_COMM_DESIRED) &&
        (RequesterId != BSWM_REQUESTER_ID_AP_READY))
    {
        (void)Det_ReportError(BSWM_MODULE_ID, BSWM_INSTANCE_ID, BSWM_SID_GET_CURRENT_MODE, BSWM_E_PARAM_INVALID);
        return (BswM_ModeType)0U;
    }

    for (idx = 0U; idx < BSWM_MAX_MODE_REQUESTS; idx++)
    {
        if ((BswM_ModeRequests[idx].IsValid == TRUE) &&
            (BswM_ModeRequests[idx].RequesterId == RequesterId))
        {
            return BswM_ModeRequests[idx].RequestedMode;
        }
    }

    return (BswM_ModeType)0U;
}

void BswM_MainFunction(void)
{
    BswM_GatewayCommandType GatewayCommand;

    if (BswM_State != BSWM_INIT)
    {
        (void)Det_ReportError(BSWM_MODULE_ID, BSWM_INSTANCE_ID, BSWM_SID_MAIN_FUNCTION, BSWM_E_UNINIT);
        return;
    }

    BswM_UpdateGatewayHealth();
    GatewayCommand = BswM_ComputeGatewayCommand();

    if (GatewayCommand == BswM_LastGatewayCommand)
    {
        return;
    }

    switch (GatewayCommand)
    {
        case BSWM_GATEWAY_COMMAND_STARTUP_HOLD:
            BswM_Action_GatewayComNo();
            break;
        case BSWM_GATEWAY_COMMAND_NO_COMM:
            BswM_Action_AllowCommNo();
            BswM_Action_GatewayComNo();
            break;
        case BSWM_GATEWAY_COMMAND_PROFILE_NORMAL:
            BswM_Action_ProfileNormal();
            break;
        case BSWM_GATEWAY_COMMAND_PROFILE_DEGRADED:
            BswM_Action_ProfileDegraded();
            break;
        case BSWM_GATEWAY_COMMAND_PROFILE_DIAG_ONLY:
            BswM_Action_ProfileDiagOnly();
            break;
        default:
            break;
    }

    BswM_LastGatewayCommand = GatewayCommand;
}

BswM_GatewayProfileType BswM_GetGatewayProfile(void)
{
    if (BswM_State != BSWM_INIT)
    {
        (void)Det_ReportError(BSWM_MODULE_ID, BSWM_INSTANCE_ID, BSWM_SID_GET_GATEWAY_PROFILE, BSWM_E_UNINIT);
        return BSWM_GATEWAY_PROFILE_NORMAL;
    }

    return BswM_GatewayProfile;
}

Std_ReturnType BswM_GetGatewayHealth(BswM_GatewayHealthType *GatewayHealthPtr)
{
    if (BswM_State != BSWM_INIT)
    {
        (void)Det_ReportError(BSWM_MODULE_ID, BSWM_INSTANCE_ID, BSWM_SID_GET_GATEWAY_HEALTH, BSWM_E_UNINIT);
        return E_NOT_OK;
    }

    if (GatewayHealthPtr == NULL)
    {
        (void)Det_ReportError(BSWM_MODULE_ID, BSWM_INSTANCE_ID, BSWM_SID_GET_GATEWAY_HEALTH, BSWM_E_NULL_POINTER);
        return E_NOT_OK;
    }

    GatewayHealthPtr->GatewayProfile = BswM_GatewayProfile;
    GatewayHealthPtr->CanFaultCounter = BswM_CanFaultCounter;
    GatewayHealthPtr->EthFaultCounter = BswM_EthFaultCounter;
    GatewayHealthPtr->SecOCFailCounter = BswM_SecOCFailCounter;
    GatewayHealthPtr->RecoveryCounter = BswM_RecoveryCounter;

    return E_OK;
}

Std_ReturnType BswM_SetGatewayHealth(const BswM_GatewayHealthType *GatewayHealthPtr)
{
    if (BswM_State != BSWM_INIT)
    {
        (void)Det_ReportError(BSWM_MODULE_ID, BSWM_INSTANCE_ID, BSWM_SID_GET_GATEWAY_HEALTH, BSWM_E_UNINIT);
        return E_NOT_OK;
    }

    if (GatewayHealthPtr == NULL)
    {
        (void)Det_ReportError(BSWM_MODULE_ID, BSWM_INSTANCE_ID, BSWM_SID_GET_GATEWAY_HEALTH, BSWM_E_NULL_POINTER);
        return E_NOT_OK;
    }

    BswM_GatewayProfile = GatewayHealthPtr->GatewayProfile;
    BswM_CanFaultCounter = GatewayHealthPtr->CanFaultCounter;
    BswM_EthFaultCounter = GatewayHealthPtr->EthFaultCounter;
    BswM_SecOCFailCounter = GatewayHealthPtr->SecOCFailCounter;
    BswM_RecoveryCounter = GatewayHealthPtr->RecoveryCounter;

    return E_OK;
}
