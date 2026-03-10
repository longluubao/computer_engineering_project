#ifndef INCLUDE_CAN_H_
#define INCLUDE_CAN_H_

/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "Std_Types.h"
#include "ComStack_Types.h"

/********************************************************************************************************/
/************************************************Defines*************************************************/
/********************************************************************************************************/

#define CAN_MODULE_ID                       ((uint16)80)
#define CAN_INSTANCE_ID                     ((uint8)0)
#define CAN_MAX_CONTROLLERS                 ((uint8)1)
#define CAN_TX_QUEUE_LENGTH                 ((uint8)32)

/* Service IDs */
#define CAN_SID_INIT                        ((uint8)0x00)
#define CAN_SID_DEINIT                      ((uint8)0x10)
#define CAN_SID_SET_CONTROLLER_MODE         ((uint8)0x03)
#define CAN_SID_GET_CONTROLLER_MODE         ((uint8)0x04)
#define CAN_SID_WRITE                       ((uint8)0x06)
#define CAN_SID_MAINFUNCTION_WRITE          ((uint8)0x01)
#define CAN_SID_MAINFUNCTION_READ           ((uint8)0x08)

/* DET Error Codes */
#define CAN_E_PARAM_POINTER                 ((uint8)0x01)
#define CAN_E_PARAM_HANDLE                  ((uint8)0x02)
#define CAN_E_PARAM_CONTROLLER              ((uint8)0x03)
#define CAN_E_UNINIT                        ((uint8)0x04)
#define CAN_E_TRANSITION                    ((uint8)0x05)

/********************************************************************************************************/
/*******************************************StructAndEnums***********************************************/
/********************************************************************************************************/

typedef enum
{
    CAN_CS_UNINIT = 0,
    CAN_CS_STOPPED,
    CAN_CS_STARTED,
    CAN_CS_SLEEP
} Can_ControllerStateType;

typedef struct
{
    uint32  id;
    uint8   length;
    uint8  *sdu;
    PduIdType swPduHandle;
} Can_PduType;

typedef struct
{
    uint8 CanControllerCount;
} Can_ConfigType;

typedef void (*Can_TxConfirmationCallbackType)(PduIdType TxPduId, Std_ReturnType result);
typedef void (*Can_RxIndicationCallbackType)(PduIdType RxPduId, const PduInfoType *PduInfoPtr);

/********************************************************************************************************/
/*****************************************FunctionPrototype**********************************************/
/********************************************************************************************************/

void Can_Init(const Can_ConfigType *ConfigPtr);
void Can_DeInit(void);

Std_ReturnType Can_SetControllerMode(uint8 Controller, Can_ControllerStateType Transition);
Std_ReturnType Can_GetControllerMode(uint8 Controller, Can_ControllerStateType *ControllerModePtr);

Std_ReturnType Can_Write(PduIdType Hth, const Can_PduType *PduInfo);
void Can_MainFunction_Write(void);
void Can_MainFunction_Read(void);

void Can_RegisterTxConfirmation(Can_TxConfirmationCallbackType Callback);
void Can_RegisterRxIndication(Can_RxIndicationCallbackType Callback);

/* Simulated Rx injection for testing */
void Can_SimulateReception(PduIdType RxPduId, const PduInfoType *PduInfoPtr);

#endif /* INCLUDE_CAN_H_ */
