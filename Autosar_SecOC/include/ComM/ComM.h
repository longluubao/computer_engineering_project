#ifndef INCLUDE_COMM_H_
#define INCLUDE_COMM_H_

/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "Std_Types.h"

/********************************************************************************************************/
/************************************************Defines*************************************************/
/********************************************************************************************************/

#define COMM_MODULE_ID                        ((uint16)12)
#define COMM_INSTANCE_ID                      ((uint8)0)

#define COMM_SID_INIT                         ((uint8)0x00)
#define COMM_SID_DEINIT                       ((uint8)0x01)
#define COMM_SID_REQUEST_COM_MODE             ((uint8)0x05)
#define COMM_SID_GET_CURRENT_COM_MODE         ((uint8)0x08)
#define COMM_SID_BUSSM_MODE_INDICATION        ((uint8)0x33)
#define COMM_SID_MAIN_FUNCTION                ((uint8)0x60)
#define COMM_SID_GET_STATE                    ((uint8)0x34)

#define COMM_E_UNINIT                         ((uint8)0x01)
#define COMM_E_PARAM_POINTER                  ((uint8)0x02)
#define COMM_E_PARAM_CHANNEL                  ((uint8)0x03)
#define COMM_E_PARAM_MODE                     ((uint8)0x04)

#define COMM_MAX_CHANNELS                     ((uint8)1)
#define COMM_MAX_USERS                        ((uint8)4)

/********************************************************************************************************/
/*******************************************StructAndEnums***********************************************/
/********************************************************************************************************/

typedef enum
{
    COMM_UNINIT = 0,
    COMM_INIT
} ComM_StateType;

typedef enum
{
    COMM_NO_COMMUNICATION = 0,
    COMM_SILENT_COMMUNICATION,
    COMM_FULL_COMMUNICATION
} ComM_ModeType;

typedef enum
{
    COMM_NO_COM_NO_PENDING_REQUEST = 0,
    COMM_NO_COM_REQUEST_PENDING,
    COMM_FULL_COM_NETWORK_REQUESTED,
    COMM_FULL_COM_READY_SLEEP,
    COMM_SILENT_COM
} ComM_SubStateType;

typedef struct
{
    ComM_ModeType RequestedMode;
    boolean IsActive;
} ComM_UserRequestType;

/********************************************************************************************************/
/*****************************************FunctionPrototype**********************************************/
/********************************************************************************************************/

void ComM_Init(void);
void ComM_DeInit(void);
Std_ReturnType ComM_RequestComMode(uint8 Channel, ComM_ModeType ComMode);
Std_ReturnType ComM_GetCurrentComMode(uint8 Channel, ComM_ModeType *ComModePtr);
Std_ReturnType ComM_GetState(uint8 Channel, ComM_SubStateType *StatePtr);
Std_ReturnType ComM_BusSM_ModeIndication(uint8 Channel, ComM_ModeType ComMode);
void ComM_MainFunction(void);

#endif /* INCLUDE_COMM_H_ */
