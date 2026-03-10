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
#define ECUM_SID_MAIN_FUNCTION      ((uint8)0x18)

#define ECUM_E_UNINIT               ((uint8)0x01)
#define ECUM_E_INVALID_STATE        ((uint8)0x02)
#define ECUM_E_PARAM_POINTER        ((uint8)0x03)
#define ECUM_E_PARAM_INVALID        ((uint8)0x04)

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

#define ECUM_WKSOURCE_POWER         ((EcuM_WakeupSourceType)0x01U)
#define ECUM_WKSOURCE_RESET         ((EcuM_WakeupSourceType)0x02U)
#define ECUM_WKSOURCE_INTERNAL_RESET ((EcuM_WakeupSourceType)0x04U)
#define ECUM_WKSOURCE_INTERNAL_WDG  ((EcuM_WakeupSourceType)0x08U)
#define ECUM_WKSOURCE_EXTERNAL_WDG  ((EcuM_WakeupSourceType)0x10U)
#define ECUM_WKSOURCE_CAN           ((EcuM_WakeupSourceType)0x20U)

typedef struct
{
    uint8 Dummy;
} EcuM_ConfigType;

/********************************************************************************************************/
/*****************************************FunctionPrototype**********************************************/
/********************************************************************************************************/

void EcuM_Init(const EcuM_ConfigType *ConfigPtr);
Std_ReturnType EcuM_StartupTwo(void);
Std_ReturnType EcuM_Shutdown(void);
EcuM_StateType EcuM_GetState(void);
Std_ReturnType EcuM_SelectShutdownTarget(EcuM_ShutdownTargetType Target);
EcuM_ShutdownTargetType EcuM_GetShutdownTarget(void);
void EcuM_SetWakeupEvent(EcuM_WakeupSourceType WakeupSource);
EcuM_WakeupSourceType EcuM_GetPendingWakeupEvents(void);
void EcuM_MainFunction(void);

#endif /* INCLUDE_ECUM_H_ */
