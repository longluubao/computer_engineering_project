/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "PduR_UdpNm.h"
#include "UdpNm.h"

/********************************************************************************************************/
/**************************************ForwardDeclarations***********************************************/
/********************************************************************************************************/

extern void PduR_UdpNmTxConfirmation(PduIdType TxPduId, Std_ReturnType result);
extern void PduR_UdpNmRxIndication(PduIdType RxPduId, const PduInfoType *PduInfoPtr);

/********************************************************************************************************/
/********************************************Functions***************************************************/
/********************************************************************************************************/

void PduR_UdpNmTxConfirmation(PduIdType TxPduId, Std_ReturnType result)
{
    UdpNm_TxConfirmation(TxPduId, result);
}

void PduR_UdpNmRxIndication(PduIdType RxPduId, const PduInfoType *PduInfoPtr)
{
    if (PduInfoPtr == NULL)
    {
        return;
    }
    UdpNm_RxIndication(RxPduId, PduInfoPtr);
}
