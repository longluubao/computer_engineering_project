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
#define COM_SID_DEINIT                        ((uint8)0x04U)
#define COM_SID_SEND_SIGNAL_GROUP             ((uint8)0x0CU)
#define COM_SID_RECEIVE_SIGNAL_GROUP          ((uint8)0x0DU)
#define COM_SID_SEND_DYN_SIGNAL               ((uint8)0x21U)
#define COM_SID_RECEIVE_DYN_SIGNAL            ((uint8)0x22U)
#define COM_SID_INVALIDATE_SIGNAL             ((uint8)0x1FU)
#define COM_SID_START_OF_RECEPTION            ((uint8)0x40U)
#define COM_SID_COPY_RX_DATA                  ((uint8)0x41U)
#define COM_SID_TP_RX_INDICATION              ((uint8)0x42U)
#define COM_SID_TP_TX_CONFIRMATION            ((uint8)0x43U)

#define COM_E_UNINIT                          ((uint8)0x01U)
#define COM_E_PARAM                           ((uint8)0x02U)
#define COM_E_PARAM_POINTER                   ((uint8)0x03U)
#define COM_E_SKIPPED_TRANSMISSION            ((uint8)0x04U)

#define COM_NUM_OF_IPDU_GROUPS                ((uint8)2U)
#define COM_NUM_OF_SIGNAL_GROUPS              ((uint8)2U)

/********************************************************************************************************/
/*******************************************TypeDefinitions**********************************************/
/********************************************************************************************************/

typedef uint16 Com_SignalIdType;
typedef uint16 Com_IpduGroupIdType;
typedef uint16 Com_SignalGroupIdType;

/********************************************************************************************************/
/*****************************************FunctionPrototype**********************************************/
/********************************************************************************************************/

void Com_Init(void);

void Com_DeInit(void);

Std_ReturnType Com_SendSignal(Com_SignalIdType SignalId, const uint8* SignalDataPtr);

Std_ReturnType Com_ReceiveSignal(Com_SignalIdType SignalId, uint8* SignalDataPtr);

Std_ReturnType Com_SendDynSignal(Com_SignalIdType SignalId, const uint8* SignalDataPtr, uint16 Length);

Std_ReturnType Com_ReceiveDynSignal(Com_SignalIdType SignalId, uint8* SignalDataPtr, uint16* LengthPtr);

Std_ReturnType Com_InvalidateSignal(Com_SignalIdType SignalId);

Std_ReturnType Com_SendSignalGroup(Com_SignalGroupIdType SignalGroupId);

Std_ReturnType Com_ReceiveSignalGroup(Com_SignalGroupIdType SignalGroupId);

void Com_IpduGroupStart(Com_IpduGroupIdType IpduGroupId, boolean Initialize);

void Com_IpduGroupStop(Com_IpduGroupIdType IpduGroupId);

Std_ReturnType Com_TriggerIPDUSend(PduIdType TxPduId);

void Com_TxConfirmation(PduIdType TxPduId, Std_ReturnType result);

void Com_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);

BufReq_ReturnType Com_StartOfReception(PduIdType ComRxPduId,
                                       const PduInfoType* PduInfoPtr,
                                       PduLengthType TpSduLength,
                                       PduLengthType* RxBufferSizePtr);

BufReq_ReturnType Com_CopyRxData(PduIdType ComRxPduId,
                                 const PduInfoType* PduInfoPtr,
                                 PduLengthType* RxBufferSizePtr);

void Com_TpRxIndication(PduIdType ComRxPduId, Std_ReturnType Result);

void Com_TpTxConfirmation(PduIdType ComTxPduId, Std_ReturnType Result);

void Com_MainFunctionTx(void);

void Com_MainFunctionRx(void);

/* Backward-compatible API used by scheduler. */
void Com_MainTx(void);

#endif