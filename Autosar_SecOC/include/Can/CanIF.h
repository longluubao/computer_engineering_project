#ifndef INCLUDE_CANIF_H_
#define INCLUDE_CANIF_H_

/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "Std_Types.h"
#include "ComStack_Types.h"


/********************************************************************************************************/
/************************************************Defines*************************************************/
/********************************************************************************************************/
#define CANIF_SUCCESS     (1u)
#define CANIF_FAILED      (0u)
/* Backward-compatible aliases */
#ifndef SUCCESS
#define SUCCESS           CANIF_SUCCESS
#endif
#ifndef FAILED
#define FAILED            CANIF_FAILED
#endif

#define CANIF_BUFFERLENGTH  ((uint32)100)

#define CANIF_MODULE_ID                     ((uint16)60)
#define CANIF_INSTANCE_ID                   ((uint8)0)

/* Service IDs */
#define CANIF_SID_INIT                      ((uint8)0x01)
#define CANIF_SID_SET_CONTROLLER_MODE       ((uint8)0x03)
#define CANIF_SID_GET_CONTROLLER_MODE       ((uint8)0x04)
#define CANIF_SID_TRANSMIT                  ((uint8)0x05)
#define CANIF_SID_RX_INDICATION             ((uint8)0x14)
#define CANIF_SID_TX_CONFIRMATION           ((uint8)0x13)
#define CANIF_SID_SET_PDU_MODE              ((uint8)0x09)
#define CANIF_SID_GET_PDU_MODE              ((uint8)0x0A)

/* DET Error Codes */
#define CANIF_E_PARAM_CANID                 ((uint8)0x10)
#define CANIF_E_PARAM_HOH                   ((uint8)0x12)
#define CANIF_E_PARAM_LPDU                  ((uint8)0x13)
#define CANIF_E_PARAM_CONTROLLER            ((uint8)0x14)
#define CANIF_E_PARAM_POINTER               ((uint8)0x20)
#define CANIF_E_UNINIT                      ((uint8)0x30)

/********************************************************************************************************/
/*******************************************StructAndEnums***********************************************/
/********************************************************************************************************/

typedef enum
{
    CANIF_CS_UNINIT = 0,
    CANIF_CS_STOPPED,
    CANIF_CS_STARTED,
    CANIF_CS_SLEEP
} CanIf_ControllerModeType;

typedef enum
{
    CANIF_OFFLINE = 0,
    CANIF_TX_OFFLINE,
    CANIF_TX_OFFLINE_ACTIVE,
    CANIF_ONLINE
} CanIf_PduModeType;

/********************************************************************************************************/
/*****************************************FunctionPrototype**********************************************/
/********************************************************************************************************/

void CanIf_Init(void);
Std_ReturnType CanIf_SetControllerMode(uint8 ControllerId, CanIf_ControllerModeType ControllerMode);
Std_ReturnType CanIf_GetControllerMode(uint8 ControllerId, CanIf_ControllerModeType *ControllerModePtr);
Std_ReturnType CanIf_Transmit(PduIdType TxPduId, const PduInfoType *PduInfoPtr);
void CanIf_TxConfirmation(PduIdType TxPduId, Std_ReturnType result);
void CanIf_RxIndication(PduIdType RxPduId, const PduInfoType *PduInfoPtr);
void CanIf_ControllerModeIndication(uint8 ControllerId, CanIf_ControllerModeType ControllerMode);
Std_ReturnType CanIf_SetPduMode(uint8 ControllerId, CanIf_PduModeType PduModeRequest);
Std_ReturnType CanIf_GetPduMode(uint8 ControllerId, CanIf_PduModeType *PduModePtr);

#endif
