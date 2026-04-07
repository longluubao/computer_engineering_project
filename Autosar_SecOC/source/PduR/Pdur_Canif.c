/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "PduR_CanIf.h"
#include "SecOC.h"
#include "SecOC_Debug.h"
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
/**************************************ForwardDeclarations***********************************************/
/********************************************************************************************************/

extern void PduR_CanIfTxConfirmation(PduIdType TxPduId, Std_ReturnType result);
extern void PduR_CanIfRxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);

/********************************************************************************************************/
/********************************************Functions***************************************************/
/********************************************************************************************************/

void PduR_CanIfTxConfirmation(PduIdType TxPduId, Std_ReturnType result)
{
    #ifdef PDUR_DEBUG
        printf("######## in PduR_CanIfTxConfirmation \n");
    #endif
    if (TxPduId >= (PduIdType)SECOC_NUM_OF_PDU_COLLECTION)
    {
        return;
    }
    SecOC_TxConfirmation(TxPduId, result);
}

void PduR_CanIfRxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{
    #ifdef PDUR_DEBUG
        printf("######## in PduR_CanIfRxIndication\n");
    #endif
    if ((PduInfoPtr == NULL) || (RxPduId >= (PduIdType)SECOC_NUM_OF_PDU_COLLECTION))
    {
        return;
    }
    #ifdef SCHEDULER_ON
        pthread_mutex_unlock(&lock);
    #endif
    SecOC_RxIndication(RxPduId, PduInfoPtr);
}
