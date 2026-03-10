#ifndef INCLUDE_CANSM_H_
#define INCLUDE_CANSM_H_

/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "Std_Types.h"

/********************************************************************************************************/
/************************************************Defines*************************************************/
/********************************************************************************************************/

#define CANSM_MODULE_ID                  ((uint16)140)
#define CANSM_INSTANCE_ID                ((uint8)0)
#define CANSM_SID_INIT                   ((uint8)0x00)
#define CANSM_SID_DEINIT                 ((uint8)0x10)
#define CANSM_SID_REQUEST_COM_MODE       ((uint8)0x01)
#define CANSM_SID_GET_CURRENT_COM_MODE   ((uint8)0x02)
#define CANSM_SID_MAINFUNCTION           ((uint8)0x05)
#define CANSM_SID_CONTROLLER_BUSOFF      ((uint8)0x04)

#define CANSM_E_UNINIT                   ((uint8)0x01)
#define CANSM_E_PARAM_POINTER            ((uint8)0x02)
#define CANSM_E_PARAM_NETWORK            ((uint8)0x03)
#define CANSM_E_PARAM_CONTROLLER         ((uint8)0x04)

/* Bus-off recovery configuration */
#define CANSM_BUSOFF_RECOVERY_CYCLES     ((uint16)10)
#define CANSM_MAX_BUSOFF_RECOVERY        ((uint8)5)

/********************************************************************************************************/
/*******************************************StructAndEnums***********************************************/
/********************************************************************************************************/

typedef enum
{
    CANSM_UNINIT = 0,
    CANSM_INIT
} CanSM_StateType;

typedef enum
{
    CANSM_NO_COMMUNICATION = 0,
    CANSM_FULL_COMMUNICATION
} CanSM_ComModeType;

/* BSM sub-states per AUTOSAR CanSM SWS */
typedef enum
{
    CANSM_BSM_S_NOT_INITIALIZED = 0,
    CANSM_BSM_S_PRE_NOCOM,
    CANSM_BSM_S_NOCOM,
    CANSM_BSM_S_PRE_FULLCOM,
    CANSM_BSM_S_FULLCOM,
    CANSM_BSM_S_CHANGE_BAUDRATE,
    CANSM_BSM_S_BUSOFF_RECOVERY
} CanSM_BsmStateType;

/********************************************************************************************************/
/*****************************************FunctionPrototype**********************************************/
/********************************************************************************************************/

void CanSM_Init(void);
void CanSM_DeInit(void);
Std_ReturnType CanSM_RequestComMode(uint8 NetworkHandle, CanSM_ComModeType ComM_Mode);
Std_ReturnType CanSM_GetCurrentComMode(uint8 NetworkHandle, CanSM_ComModeType *ComM_ModePtr);
Std_ReturnType CanSM_GetBsmState(uint8 NetworkHandle, CanSM_BsmStateType *BsmStatePtr);
void CanSM_ControllerBusOff(uint8 ControllerId);
void CanSM_MainFunction(void);

#endif /* INCLUDE_CANSM_H_ */
