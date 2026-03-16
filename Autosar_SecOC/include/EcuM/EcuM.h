#ifndef INCLUDE_ECUM_H_
#define INCLUDE_ECUM_H_

/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "Std_Types.h"

/********************************************************************************************************/
/************************************************Defines*************************************************/
/********************************************************************************************************/

#define ECUM_MODULE_ID              ((uint16)10)
#define ECUM_INSTANCE_ID            ((uint8)0)

#define ECUM_SID_INIT               ((uint8)0x00)
#define ECUM_SID_STARTUP_TWO        ((uint8)0x03)
#define ECUM_SID_SHUTDOWN           ((uint8)0x04)
#define ECUM_SID_GET_STATE          ((uint8)0x07)
#define ECUM_SID_SELECT_SHUTDOWN    ((uint8)0x06)
#define ECUM_SID_SET_WAKEUP_EVENT   ((uint8)0x0C)
#define ECUM_SID_VALIDATE_WAKEUP_EVENT ((uint8)0x0D)
#define ECUM_SID_CLEAR_WAKEUP_EVENT ((uint8)0x0E)
#define ECUM_SID_CHECK_WAKEUP       ((uint8)0x0F)
#define ECUM_SID_MAIN_FUNCTION      ((uint8)0x18)
#define ECUM_SID_GET_SHUTDOWN_TARGET ((uint8)0x09)
#define ECUM_SID_GET_LAST_SHUTDOWN_TARGET ((uint8)0x0A)
#define ECUM_SID_SELECT_BOOT_TARGET ((uint8)0x1A)
#define ECUM_SID_GET_BOOT_TARGET    ((uint8)0x1B)
#define ECUM_SID_SELECT_SLEEP_MODE  ((uint8)0x10)
#define ECUM_SID_GO_HALT            ((uint8)0x11)
#define ECUM_SID_GO_POLL            ((uint8)0x12)
#define ECUM_SID_GO_DOWN            ((uint8)0x13)
#define ECUM_SID_KILL_ALL_RUN_REQUESTS ((uint8)0x14)
#define ECUM_SID_REQUEST_RUN        ((uint8)0x15)
#define ECUM_SID_RELEASE_RUN        ((uint8)0x16)
#define ECUM_SID_GET_WAKEUP_STATUS  ((uint8)0x17)
#define ECUM_SID_GET_EXPIRED_WAKEUP_EVENTS ((uint8)0x19)
#define ECUM_SID_REQUEST_POST_RUN   ((uint8)0x20)
#define ECUM_SID_RELEASE_POST_RUN   ((uint8)0x21)

#define ECUM_E_UNINIT               ((uint8)0x01)
#define ECUM_E_INVALID_STATE        ((uint8)0x02)
#define ECUM_E_PARAM_POINTER        ((uint8)0x03)
#define ECUM_E_PARAM_INVALID        ((uint8)0x04)
#define ECUM_E_UNKNOWN_WAKEUP_SOURCE ((uint8)0x05)
#define ECUM_E_MULTIPLE_RUN_REQUESTS ((uint8)0x06)
#define ECUM_E_MULTIPLE_POST_RUN_REQUESTS ((uint8)0x07)

#define ECUM_MAX_RUN_USERS          ((uint8)8)
#define ECUM_WAKEUP_VALIDATION_TIMEOUT_MAINCYCLES ((uint16)20)

/********************************************************************************************************/
/*******************************************StructAndEnums***********************************************/
/********************************************************************************************************/

typedef enum
{
    ECUM_STATE_UNINIT = 0,
    ECUM_STATE_STARTUP_ONE,
    ECUM_STATE_STARTUP_TWO,
    ECUM_STATE_RUN,
    ECUM_STATE_SHUTDOWN,
    ECUM_STATE_SLEEP,
    ECUM_STATE_OFF
} EcuM_StateType;

typedef enum
{
    ECUM_SHUTDOWN_TARGET_SLEEP = 0,
    ECUM_SHUTDOWN_TARGET_RESET,
    ECUM_SHUTDOWN_TARGET_OFF
} EcuM_ShutdownTargetType;

typedef uint32 EcuM_WakeupSourceType;
/* Optional callouts: when unset, EcuM keeps default behavior. */
typedef Std_ReturnType (*EcuM_ResetCalloutType)(void);
typedef Std_ReturnType (*EcuM_OffCalloutType)(void);
typedef boolean (*EcuM_WakeupValidationCalloutType)(EcuM_WakeupSourceType WakeupSource);

typedef enum
{
    ECUM_SLEEP_MODE_HALT = 0,
    ECUM_SLEEP_MODE_POLL
} EcuM_SleepModeType;

typedef enum
{
    ECUM_WAKEUP_NONE = 0,
    ECUM_WAKEUP_PENDING,
    ECUM_WAKEUP_VALIDATED,
    ECUM_WAKEUP_EXPIRED
} EcuM_WakeupStatusType;

typedef enum
{
    ECUM_BOOT_TARGET_APP = 0,
    ECUM_BOOT_TARGET_BOOTLOADER
} EcuM_BootTargetType;

#define ECUM_WKSOURCE_POWER         ((EcuM_WakeupSourceType)0x01U)
#define ECUM_WKSOURCE_RESET         ((EcuM_WakeupSourceType)0x02U)
#define ECUM_WKSOURCE_INTERNAL_RESET ((EcuM_WakeupSourceType)0x04U)
#define ECUM_WKSOURCE_INTERNAL_WDG  ((EcuM_WakeupSourceType)0x08U)
#define ECUM_WKSOURCE_EXTERNAL_WDG  ((EcuM_WakeupSourceType)0x10U)
#define ECUM_WKSOURCE_CAN           ((EcuM_WakeupSourceType)0x20U)
#define ECUM_WKSOURCE_ETH           ((EcuM_WakeupSourceType)0x40U)
#define ECUM_WKSOURCE_ALL           (ECUM_WKSOURCE_POWER | ECUM_WKSOURCE_RESET | \
                                     ECUM_WKSOURCE_INTERNAL_RESET | ECUM_WKSOURCE_INTERNAL_WDG | \
                                     ECUM_WKSOURCE_EXTERNAL_WDG | ECUM_WKSOURCE_CAN | ECUM_WKSOURCE_ETH)

typedef struct
{
    /* Optional reset/off hooks and HW wakeup validation callback. */
    EcuM_ResetCalloutType EcuMResetCallout;
    EcuM_OffCalloutType EcuMOffCallout;
    EcuM_WakeupValidationCalloutType EcuMWakeupValidationCallout;
} EcuM_ConfigType;

/********************************************************************************************************/
/*****************************************FunctionPrototype**********************************************/
/********************************************************************************************************/

void EcuM_Init(const EcuM_ConfigType *ConfigPtr);
void EcuM_SetResetCallout(EcuM_ResetCalloutType ResetCallout);
void EcuM_SetOffCallout(EcuM_OffCalloutType OffCallout);
void EcuM_SetWakeupValidationCallout(EcuM_WakeupValidationCalloutType WakeupValidationCallout);
Std_ReturnType EcuM_StartupTwo(void);
Std_ReturnType EcuM_Shutdown(void);
EcuM_StateType EcuM_GetState(void);
Std_ReturnType EcuM_SelectShutdownTarget(EcuM_ShutdownTargetType Target);
EcuM_ShutdownTargetType EcuM_GetShutdownTarget(void);
EcuM_ShutdownTargetType EcuM_GetLastShutdownTarget(void);
Std_ReturnType EcuM_SelectBootTarget(EcuM_BootTargetType Target);
EcuM_BootTargetType EcuM_GetBootTarget(void);
Std_ReturnType EcuM_SelectSleepMode(EcuM_SleepModeType SleepMode);
Std_ReturnType EcuM_GoHalt(void);
Std_ReturnType EcuM_GoPoll(void);
Std_ReturnType EcuM_GoDown(void);
Std_ReturnType EcuM_RequestRUN(uint8 User);
Std_ReturnType EcuM_ReleaseRUN(uint8 User);
Std_ReturnType EcuM_RequestPOST_RUN(uint8 User);
Std_ReturnType EcuM_ReleasePOST_RUN(uint8 User);
void EcuM_KillAllRUNRequests(void);
void EcuM_SetWakeupEvent(EcuM_WakeupSourceType WakeupSource);
void EcuM_ValidateWakeupEvent(EcuM_WakeupSourceType WakeupSource);
void EcuM_ClearWakeupEvent(EcuM_WakeupSourceType WakeupSource);
void EcuM_CheckWakeup(EcuM_WakeupSourceType WakeupSource);
EcuM_WakeupSourceType EcuM_GetPendingWakeupEvents(void);
EcuM_WakeupSourceType EcuM_GetValidatedWakeupEvents(void);
EcuM_WakeupSourceType EcuM_GetExpiredWakeupEvents(void);
EcuM_WakeupStatusType EcuM_GetWakeupStatus(EcuM_WakeupSourceType WakeupSource);
void EcuM_MainFunctionState(void);
void EcuM_MainFunctionComStack(void);
void EcuM_MainFunctionSecurityStack(void);
void EcuM_MainFunctionNetworkStack(void);
void EcuM_MainFunctionDiagnosticsStack(void);
void EcuM_MainFunctionNvStack(void);
void EcuM_MainFunction(void);

#endif /* INCLUDE_ECUM_H_ */
