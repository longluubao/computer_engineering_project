#ifndef INCLUDE_PDUR_CANTP_H_
#define INCLUDE_PDUR_CANTP_H_

/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "Std_Types.h"
#include "Com/ComStack_Types.h"

/********************************************************************************************************/
/*****************************************FunctionPrototype**********************************************/
/********************************************************************************************************/

BufReq_ReturnType PduR_CanTpCopyTxData(PduIdType id,
                                       const PduInfoType* info,
                                       const RetryInfoType* retry,
                                       PduLengthType* availableDataPtr);

void PduR_CanTpTxConfirmation(PduIdType TxPduId, Std_ReturnType result);

BufReq_ReturnType PduR_CanTpCopyRxData (PduIdType id,const PduInfoType* info,PduLengthType* bufferSizePtr);

BufReq_ReturnType PduR_CanTpStartOfReception(PduIdType id, const PduInfoType* info, PduLengthType TpSduLength, PduLengthType* bufferSizePtr);

void PduR_CanTpRxIndication (PduIdType id, Std_ReturnType result);

#endif /* INCLUDE_PDUR_CANTP_H_ */