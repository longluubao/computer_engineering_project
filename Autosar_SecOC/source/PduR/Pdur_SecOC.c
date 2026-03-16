/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "PduR_SecOC.h"
#include "Com.h"
#include "CanIF.h"
#include "CanTP.h"
#include "Dcm.h"
#include "SoAd.h"
#include "SecOC_Lcfg.h"

/********************************************************************************************************/
/******************************************GlobalVaribles************************************************/
/********************************************************************************************************/

extern SecOC_PduCollection PdusCollections[];

/********************************************************************************************************/
/********************************************Functions***************************************************/
/********************************************************************************************************/

Std_ReturnType PduR_SecOCTransmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)
{
    if (PduInfoPtr == NULL)
    {
        return E_NOT_OK;
    }
    if (TxPduId >= (PduIdType)SECOC_NUM_OF_PDU_COLLECTION)
    {
        return E_NOT_OK;
    }

    switch (PdusCollections[TxPduId].Type)
    {
        case SECOC_SECURED_PDU_CANIF:
            return CanIf_Transmit(TxPduId, PduInfoPtr);
        case SECOC_SECURED_PDU_CANTP:
            return CanTp_Transmit(TxPduId, PduInfoPtr);
        case SECOC_SECURED_PDU_SOADTP:
            return SoAd_TpTransmit(TxPduId, PduInfoPtr);
        case SECOC_SECURED_PDU_SOADIF:
            return SoAd_IfTransmit(TxPduId, PduInfoPtr);
        case SECOC_AUTH_COLLECTON_PDU:
        case SECOC_CRYPTO_COLLECTON_PDU:
            return CanIf_Transmit(TxPduId, PduInfoPtr);
        default:
            return E_NOT_OK;
    }
}

void PduR_SecOCIfTxConfirmation(PduIdType TxPduId, Std_ReturnType result)
{
    if (TxPduId >= (PduIdType)SECOC_NUM_OF_TX_PDU_PROCESSING)
    {
        return;
    }
    Com_TxConfirmation(TxPduId, result);
}

void PduR_SecOCIfRxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{
    if ((PduInfoPtr == NULL) || (RxPduId >= (PduIdType)SECOC_NUM_OF_RX_PDU_PROCESSING))
    {
        return;
    }
    Com_RxIndication(RxPduId, PduInfoPtr);
}

void PduR_SecOCTpTxConfirmation(PduIdType TxPduId, Std_ReturnType result)
{
    if (TxPduId < (PduIdType)SECOC_NUM_OF_TX_PDU_PROCESSING)
    {
        Com_TpTxConfirmation(TxPduId, result);
    }
    else
    {
        Dcm_TpTxConfirmation(TxPduId, result);
    }
}

BufReq_ReturnType PduR_SecOCTpStartOfReception(PduIdType RxPduId,
                                               const PduInfoType* PduInfoPtr,
                                               PduLengthType TpSduLength,
                                               PduLengthType* RxBufferSizePtr)
{
    if (RxBufferSizePtr == NULL)
    {
        return BUFREQ_E_NOT_OK;
    }
    if (RxPduId < (PduIdType)SECOC_NUM_OF_RX_PDU_PROCESSING)
    {
        return Com_StartOfReception(RxPduId, PduInfoPtr, TpSduLength, RxBufferSizePtr);
    }
    return Dcm_TpStartOfReception(RxPduId, PduInfoPtr, TpSduLength, RxBufferSizePtr);
}

BufReq_ReturnType PduR_SecOCTpCopyRxData(PduIdType RxPduId,
                                         const PduInfoType* PduInfoPtr,
                                         PduLengthType* RxBufferSizePtr)
{
    if (RxBufferSizePtr == NULL)
    {
        return BUFREQ_E_NOT_OK;
    }
    if (RxPduId < (PduIdType)SECOC_NUM_OF_RX_PDU_PROCESSING)
    {
        return Com_CopyRxData(RxPduId, PduInfoPtr, RxBufferSizePtr);
    }
    return Dcm_TpCopyRxData(RxPduId, PduInfoPtr, RxBufferSizePtr);
}

void PduR_SecOCTpRxIndication(PduIdType RxPduId, Std_ReturnType result)
{
    if (RxPduId < (PduIdType)SECOC_NUM_OF_RX_PDU_PROCESSING)
    {
        Com_TpRxIndication(RxPduId, result);
    }
    else
    {
        Dcm_TpRxIndication(RxPduId, result);
    }
}
