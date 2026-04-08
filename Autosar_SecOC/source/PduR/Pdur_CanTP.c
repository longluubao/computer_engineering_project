/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "Pdur_CanTP.h"
#include "SecOC.h"
#include "SecOC_Debug.h"

/********************************************************************************************************/
/**************************************ForwardDeclarations***********************************************/
/********************************************************************************************************/

extern BufReq_ReturnType PduR_CanTpCopyTxData(PduIdType id,
                                              const PduInfoType* info,
                                              const RetryInfoType* retry,
                                              PduLengthType* availableDataPtr);
extern void PduR_CanTpTxConfirmation(PduIdType TxPduId, Std_ReturnType result);
extern BufReq_ReturnType PduR_CanTpCopyRxData(PduIdType id,
                                              const PduInfoType* info,
                                              PduLengthType* bufferSizePtr);
extern BufReq_ReturnType PduR_CanTpStartOfReception(PduIdType id,
                                                    const PduInfoType* info,
                                                    PduLengthType TpSduLength,
                                                    PduLengthType* bufferSizePtr);
extern void PduR_CanTpRxIndication(PduIdType id, Std_ReturnType result);

/********************************************************************************************************/
/********************************************Functions***************************************************/
/********************************************************************************************************/

BufReq_ReturnType PduR_CanTpCopyTxData(PduIdType id,
                                       const PduInfoType* info,
                                       const RetryInfoType* retry,
                                       PduLengthType* availableDataPtr)
{
    #ifdef PDUR_DEBUG
        (void)printf("######## in PduR_CanTpCopyTxData \n");
    #endif
    if ((id >= (PduIdType)SECOC_NUM_OF_TX_PDU_PROCESSING) || (availableDataPtr == NULL))
    {
        return BUFREQ_E_NOT_OK;
    }
    return SecOC_CopyTxData(id, info, retry, availableDataPtr);
}

void PduR_CanTpTxConfirmation(PduIdType TxPduId, Std_ReturnType result)
{
    #ifdef PDUR_DEBUG
        (void)printf("######## in PduR_CanTpTxConfirmation \n");
    #endif
    if (TxPduId >= (PduIdType)SECOC_NUM_OF_TX_PDU_PROCESSING)
    {
        return;
    }
    SecOC_TpTxConfirmation(TxPduId, result);
}

BufReq_ReturnType PduR_CanTpCopyRxData(PduIdType id,
                                       const PduInfoType* info,
                                       PduLengthType* bufferSizePtr)
{
    #ifdef PDUR_DEBUG
        (void)printf("######## in PduR_CanTpCopyRxData \n");
    #endif
    if ((id >= (PduIdType)SECOC_NUM_OF_RX_PDU_PROCESSING) || (bufferSizePtr == NULL))
    {
        return BUFREQ_E_NOT_OK;
    }
    return SecOC_CopyRxData(id, info, bufferSizePtr);
}

BufReq_ReturnType PduR_CanTpStartOfReception(PduIdType id,
                                             const PduInfoType* info,
                                             PduLengthType TpSduLength,
                                             PduLengthType* bufferSizePtr)
{
    #ifdef PDUR_DEBUG
        (void)printf("######## in PduR_CanTpStartOfReception \n");
    #endif
    if ((id >= (PduIdType)SECOC_NUM_OF_RX_PDU_PROCESSING) || (bufferSizePtr == NULL))
    {
        return BUFREQ_E_NOT_OK;
    }
    return SecOC_StartOfReception(id, info, TpSduLength, bufferSizePtr);
}

void PduR_CanTpRxIndication(PduIdType id, Std_ReturnType result)
{
    #ifdef PDUR_DEBUG
        (void)printf("######## in PduR_CanTpRxIndication \n");
    #endif
    if (id >= (PduIdType)SECOC_NUM_OF_RX_PDU_PROCESSING)
    {
        return;
    }
    SecOC_TpRxIndication(id, result);
}
