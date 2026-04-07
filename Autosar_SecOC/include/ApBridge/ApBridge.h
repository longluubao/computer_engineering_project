#ifndef INCLUDE_APBRIDGE_H_
#define INCLUDE_APBRIDGE_H_

/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "Std_Types.h"
#include "SoAd/SoAd.h"

/********************************************************************************************************/
/************************************************Defines*************************************************/
/********************************************************************************************************/

#define APBRIDGE_MODULE_ID                     ((uint16)0x900U)
#define APBRIDGE_INSTANCE_ID                   ((uint8)0U)

#define APBRIDGE_SID_INIT                      ((uint8)0x01U)
#define APBRIDGE_SID_DEINIT                    ((uint8)0x02U)
#define APBRIDGE_SID_MAIN_FUNCTION             ((uint8)0x03U)
#define APBRIDGE_SID_REPORT_HEARTBEAT          ((uint8)0x04U)
#define APBRIDGE_SID_REPORT_SERVICE_STATUS     ((uint8)0x05U)
#define APBRIDGE_SID_SET_FORCE_STATE           ((uint8)0x06U)

#define APBRIDGE_HEARTBEAT_TIMEOUT_CYCLES      ((uint16)50U)
#define APBRIDGE_SERVICE_FAIL_THRESHOLD        ((uint8)3U)
#define APBRIDGE_SERVICE_RECOVERY_THRESHOLD    ((uint8)5U)

/********************************************************************************************************/
/*******************************************TypeDefinitions**********************************************/
/********************************************************************************************************/

typedef enum
{
    APBRIDGE_STATE_UNINIT = 0U,
    APBRIDGE_STATE_INIT
} ApBridge_StateType;

typedef struct
{
    SoAd_ApBridgeStateType ApBridgeState;
    uint16 HeartbeatAgeCycles;
    uint8 ServiceFailCounter;
    uint8 ServiceRecoveryCounter;
} ApBridge_StatusType;

/********************************************************************************************************/
/*****************************************FunctionPrototype**********************************************/
/********************************************************************************************************/

void ApBridge_Init(void);
void ApBridge_DeInit(void);
void ApBridge_MainFunction(void);
void ApBridge_ReportHeartbeat(boolean Success);
void ApBridge_ReportServiceStatus(boolean ServiceOk);
Std_ReturnType ApBridge_SetForcedState(SoAd_ApBridgeStateType State);
Std_ReturnType ApBridge_GetStatus(ApBridge_StatusType* StatusPtr);

#endif /* INCLUDE_APBRIDGE_H_ */
