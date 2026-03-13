/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "PduR_SoAd.h"
#include "SecOC.h"
#ifdef SCHEDULER_ON
    #include <pthread.h>
#endif 

/********************************************************************************************************/
/******************************************GlobalVaribles************************************************/
/********************************************************************************************************/

#ifdef SCHEDULER_ON
    extern pthread_mutex_t lock;
#endif 

/********************************************************************************************************/
/********************************************Functions***************************************************/
/********************************************************************************************************/

void PduR_SoAdIfTxConfirmation(PduIdType TxPduId, Std_ReturnType result)
{
    if (TxPduId >= (PduIdType)SECOC_NUM_OF_PDU_COLLECTION)
    {
        return;
    }
	SecOC_TxConfirmation( TxPduId,  result );
}


void PduR_SoAdIfRxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{
    if ((PduInfoPtr == NULL) || (RxPduId >= (PduIdType)SECOC_NUM_OF_PDU_COLLECTION))
    {
        return;
    }
    #ifdef SCHEDULER_ON
        pthread_mutex_unlock(&lock);
    #endif 
	SecOC_RxIndication(RxPduId, PduInfoPtr);
}




BufReq_ReturnType PduR_SoAdTpCopyTxData (PduIdType id,const PduInfoType* info,const RetryInfoType* retry,
PduLengthType* availableDataPtr)
{
    #ifdef SOAD_DEBUG
        printf("######## in PduR_SoAdTpCopyTxData \n");
    #endif
    if ((id >= (PduIdType)SECOC_NUM_OF_TX_PDU_PROCESSING) || (availableDataPtr == NULL))
    {
        return BUFREQ_E_NOT_OK;
    }
   return SecOC_CopyTxData(id, info, retry, availableDataPtr);
}



void PduR_SoAdTpTxConfirmation(PduIdType TxPduId, Std_ReturnType result)
{
    #ifdef SOAD_DEBUG
        printf("######## in PduR_SoAdTpTxConfirmation \n");
    #endif
    if (TxPduId >= (PduIdType)SECOC_NUM_OF_TX_PDU_PROCESSING)
    {
        return;
    }
    // forward result to SecOC
    SecOC_TpTxConfirmation(TxPduId, result);
}


BufReq_ReturnType PduR_SoAdTpCopyRxData (PduIdType id,const PduInfoType* info,PduLengthType* bufferSizePtr)
{
    #ifdef SOAD_DEBUG
        printf("######## in PduR_SoAdTpCopyRxData \n");
    #endif
    if ((id >= (PduIdType)SECOC_NUM_OF_RX_PDU_PROCESSING) || (bufferSizePtr == NULL))
    {
        return BUFREQ_E_NOT_OK;
    }
    /* SWS_PduR_00428 */
    return SecOC_CopyRxData(id, info, bufferSizePtr);
}

BufReq_ReturnType PduR_SoAdStartOfReception(PduIdType id, const PduInfoType* info, PduLengthType TpSduLength, PduLengthType* bufferSizePtr)
{
    #ifdef SOAD_DEBUG
        printf("######## in PduR_SoAdStartOfReception \n");
    #endif
    if ((id >= (PduIdType)SECOC_NUM_OF_RX_PDU_PROCESSING) || (bufferSizePtr == NULL))
    {
        return BUFREQ_E_NOT_OK;
    }
    /* SWS_PduR_00549 */
    return SecOC_StartOfReception(id, info, TpSduLength, bufferSizePtr);
}

void PduR_SoAdTpRxIndication (PduIdType id, Std_ReturnType result)
{
    #ifdef SOAD_DEBUG
        printf("######## in PduR_SoAdTpRxIndication \n");
    #endif
    if (id >= (PduIdType)SECOC_NUM_OF_RX_PDU_PROCESSING)
    {
        return;
    }
    /* SWS_PduR_00207 */
    SecOC_TpRxIndication(id, result);
}


