#ifndef INCLUDE_COM_H_
#define INCLUDE_COM_H_

/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "Std_Types.h"
#include "ComStack_Types.h"

/********************************************************************************************************/
/************************************************Defines*************************************************/
/********************************************************************************************************/

#define COM_MODULE_ID                         ((uint16)50U)
#define COM_INSTANCE_ID                       ((uint8)0U)

#define COM_SID_INIT                          ((uint8)0x01U)
#define COM_SID_SEND_SIGNAL                   ((uint8)0x0AU)
#define COM_SID_RECEIVE_SIGNAL                ((uint8)0x0BU)
#define COM_SID_IPDU_GROUP_START              ((uint8)0x1AU)
#define COM_SID_IPDU_GROUP_STOP               ((uint8)0x1BU)
#define COM_SID_TRIGGER_IPDU_SEND             ((uint8)0x1DU)
#define COM_SID_MAIN_FUNCTION_TX              ((uint8)0x02U)
#define COM_SID_MAIN_FUNCTION_RX              ((uint8)0x03U)

#define COM_E_UNINIT                          ((uint8)0x01U)
#define COM_E_PARAM                           ((uint8)0x02U)
#define COM_E_PARAM_POINTER                   ((uint8)0x03U)
#define COM_E_SKIPPED_TRANSMISSION            ((uint8)0x04U)

#define COM_NUM_OF_IPDU_GROUPS                ((uint8)1U)

/********************************************************************************************************/
/*******************************************TypeDefinitions**********************************************/
/********************************************************************************************************/

typedef uint16 Com_SignalIdType;
typedef uint16 Com_IpduGroupIdType;

/********************************************************************************************************/
/*****************************************FunctionPrototype**********************************************/
/********************************************************************************************************/

void Com_Init(void);

Std_ReturnType Com_SendSignal(Com_SignalIdType SignalId, const uint8* SignalDataPtr);

Std_ReturnType Com_ReceiveSignal(Com_SignalIdType SignalId, uint8* SignalDataPtr);

void Com_IpduGroupStart(Com_IpduGroupIdType IpduGroupId, boolean Initialize);

void Com_IpduGroupStop(Com_IpduGroupIdType IpduGroupId);

Std_ReturnType Com_TriggerIPDUSend(PduIdType TxPduId);

void Com_TxConfirmation(PduIdType TxPduId, Std_ReturnType result);

void Com_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);

void Com_MainFunctionTx(void);

void Com_MainFunctionRx(void);

/* Backward-compatible API used by scheduler. */
void Com_MainTx(void);

#endif