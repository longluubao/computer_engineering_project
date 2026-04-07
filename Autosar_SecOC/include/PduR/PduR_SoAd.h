#ifndef INCLUDE_PDUR_SOAD_H_
#define INCLUDE_PDUR_SOAD_H_


/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "Std_Types.h"
#include "Com/ComStack_Types.h"



/********************************************************************************************************/
/*****************************************FunctionPrototype**********************************************/
/********************************************************************************************************/

void PduR_SoAdIfTxConfirmation(PduIdType TxPduId, Std_ReturnType result);
void PduR_SoAdIfRxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);

BufReq_ReturnType PduR_SoAdTpCopyTxData(
    PduIdType id,
    const PduInfoType* info,
    const RetryInfoType* retry,
    PduLengthType* availableDataPtr
     );

void PduR_SoAdTpTxConfirmation(PduIdType TxPduId, Std_ReturnType result);

BufReq_ReturnType PduR_SoAdTpCopyRxData(
    PduIdType id,
    const PduInfoType* info,
    PduLengthType* bufferSizePtr
);

BufReq_ReturnType PduR_SoAdStartOfReception(
    PduIdType id,
    const PduInfoType* info,
    PduLengthType TpSduLength,
    PduLengthType* bufferSizePtr
);

void PduR_SoAdTpRxIndication(PduIdType id, Std_ReturnType result);

#endif /* INCLUDE_PDUR_SOAD_H_ */