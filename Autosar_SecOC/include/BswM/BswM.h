#ifndef INCLUDE_BSWM_H_
#define INCLUDE_BSWM_H_

/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "Std_Types.h"

/********************************************************************************************************/
/************************************************Defines*************************************************/
/********************************************************************************************************/

#define BSWM_MODULE_ID                  ((uint16)42)
#define BSWM_INSTANCE_ID                ((uint8)0)

#define BSWM_SID_INIT                   ((uint8)0x00)
#define BSWM_SID_DEINIT                 ((uint8)0x04)
#define BSWM_SID_MAIN_FUNCTION          ((uint8)0x03)
#define BSWM_SID_REQUEST_MODE           ((uint8)0x02)
#define BSWM_SID_GET_CURRENT_MODE       ((uint8)0x05)
#define BSWM_SID_GET_GATEWAY_PROFILE    ((uint8)0x06)
#define BSWM_SID_GET_GATEWAY_HEALTH     ((uint8)0x07)
#define BSWM_SID_GET_VERSION_INFO       ((uint8)0x01)

#define BSWM_E_UNINIT                   ((uint8)0x01)
#define BSWM_E_NULL_POINTER             ((uint8)0x02)
#define BSWM_E_PARAM_INVALID            ((uint8)0x03)
#define BSWM_E_REQ_MODE_OUT_OF_RANGE    ((uint8)0x04)

#define BSWM_MAX_MODE_REQUESTS          ((uint8)8)
#define BSWM_MAX_RULES                  ((uint8)8)
#define BSWM_MAX_ACTION_LISTS           ((uint8)8)

#define BSWM_REQUESTER_ID_COMM_DESIRED  ((uint16)0x8001U)
#define BSWM_REQUESTER_ID_AP_READY      ((uint16)0x8002U)

#define BSWM_AP_STATE_NOT_READY         ((BswM_ModeType)0U)
#define BSWM_AP_STATE_READY             ((BswM_ModeType)1U)
#define BSWM_AP_STATE_DEGRADED          ((BswM_ModeType)2U)

#define BSWM_AP_READY_FALSE             BSWM_AP_STATE_NOT_READY
#define BSWM_AP_READY_TRUE              BSWM_AP_STATE_READY

/********************************************************************************************************/
/*******************************************StructAndEnums***********************************************/
/********************************************************************************************************/

typedef enum
{
    BSWM_UNINIT = 0,
    BSWM_INIT
} BswM_StateType;

typedef uint16 BswM_ModeType;

typedef enum
{
    BSWM_CONDITION_TRUE = 0,
    BSWM_CONDITION_FALSE
} BswM_RuleResultType;

typedef enum
{
    BSWM_ACTIONLIST_COMM_FULL = 0,
    BSWM_ACTIONLIST_COMM_NO,
    BSWM_ACTIONLIST_STARTUP,
    BSWM_ACTIONLIST_SHUTDOWN
} BswM_ActionListIdType;

typedef struct
{
    uint16 RequesterId;
    BswM_ModeType RequestedMode;
    boolean IsValid;
} BswM_ModeRequestType;

typedef BswM_RuleResultType (*BswM_RuleEvalFuncType)(void);
typedef void (*BswM_ActionFuncType)(void);

typedef struct
{
    BswM_RuleEvalFuncType EvalFunc;
    BswM_ActionListIdType TrueActionListId;
    boolean IsEnabled;
} BswM_RuleType;

typedef struct
{
    BswM_ActionFuncType Actions[4];
    uint8 ActionCount;
} BswM_ActionListType;

typedef enum
{
    BSWM_GATEWAY_PROFILE_NORMAL = 0,
    BSWM_GATEWAY_PROFILE_DEGRADED,
    BSWM_GATEWAY_PROFILE_DIAG_ONLY
} BswM_GatewayProfileType;

typedef struct
{
    BswM_GatewayProfileType GatewayProfile;
    uint8 CanFaultCounter;
    uint8 EthFaultCounter;
    uint8 SecOCFailCounter;
    uint8 RecoveryCounter;
} BswM_GatewayHealthType;

typedef struct
{
    uint8 Dummy;
} BswM_ConfigType;

/********************************************************************************************************/
/*****************************************FunctionPrototype**********************************************/
/********************************************************************************************************/

void BswM_Init(const BswM_ConfigType *ConfigPtr);
void BswM_Deinit(void);
void BswM_MainFunction(void);
Std_ReturnType BswM_RequestMode(uint16 RequesterId, BswM_ModeType RequestedMode);
BswM_ModeType BswM_GetCurrentMode(uint16 RequesterId);
BswM_GatewayProfileType BswM_GetGatewayProfile(void);
Std_ReturnType BswM_GetGatewayHealth(BswM_GatewayHealthType *GatewayHealthPtr);
Std_ReturnType BswM_SetGatewayHealth(const BswM_GatewayHealthType *GatewayHealthPtr);

#endif /* INCLUDE_BSWM_H_ */
