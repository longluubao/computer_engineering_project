
/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "CanTP.h"
#include "PduR_CanTp.h"
#include "SecOC.h"
#include "CanIF.h"
#include "Det.h"
#include "Std_Types.h"
#include "SecOC_Debug.h"
#include "SecOC_Cfg.h"
#include "SecOC_Lcfg.h"
#ifdef SCHEDULER_ON
    #include <pthread.h>
#endif
#ifdef LINUX
#include "ethernet.h"
#endif

/********************************************************************************************************/
/******************************************GlobalVaribles************************************************/
/********************************************************************************************************/

#ifdef SCHEDULER_ON
    extern pthread_mutex_t lock;
#endif

static boolean CanTp_Initialized = FALSE;

static PduInfoType CanTp_Buffer[SECOC_NUM_OF_TX_PDU_PROCESSING];
static PduInfoType CanTp_Buffer_Rx[SECOC_NUM_OF_RX_PDU_PROCESSING];
static uint8 CanTp_Recieve_Counter[SECOC_NUM_OF_RX_PDU_PROCESSING];
static PduLengthType CanTp_secureLength_Recieve[SECOC_NUM_OF_RX_PDU_PROCESSING];

/* Timeout counters for Tx channels */
static uint16 CanTp_TxTimeoutCounter[SECOC_NUM_OF_TX_PDU_PROCESSING];
/* Timeout counters for Rx channels */
static uint16 CanTp_RxTimeoutCounter[SECOC_NUM_OF_RX_PDU_PROCESSING];

/* Channel states */
static CanTp_ChannelStateType CanTp_TxState[SECOC_NUM_OF_TX_PDU_PROCESSING];
static CanTp_ChannelStateType CanTp_RxState[SECOC_NUM_OF_RX_PDU_PROCESSING];

/********************************************************************************************************/
/*****************************External API declarations (MISRA 8.4)**************************************/
/********************************************************************************************************/

void CanTp_Init(void);
void CanTp_Shutdown(void);
Std_ReturnType CanTp_Transmit(PduIdType CanTpTxSduId, const PduInfoType* CanTpTxInfoPtr);
void CanTp_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);
void CanTp_MainFunctionTx(void);
void CanTp_TxConfirmation(PduIdType TxPduId, Std_ReturnType result);
void CanTp_MainFunctionRx(void);

/********************************************************************************************************/
/********************************************Functions***************************************************/
/********************************************************************************************************/

void CanTp_Init(void)
{
    for (PduIdType i = 0; i < SECOC_NUM_OF_TX_PDU_PROCESSING; i++)
    {
        CanTp_Buffer[i].SduLength = 0;
        CanTp_Buffer[i].SduDataPtr = NULL;
        CanTp_Buffer[i].MetaDataPtr = NULL;
        CanTp_TxTimeoutCounter[i] = 0;
        CanTp_TxState[i] = CANTP_STATE_IDLE;
    }

    for (PduIdType i = 0; i < SECOC_NUM_OF_RX_PDU_PROCESSING; i++)
    {
        CanTp_Buffer_Rx[i].SduLength = 0;
        CanTp_Buffer_Rx[i].SduDataPtr = NULL;
        CanTp_Buffer_Rx[i].MetaDataPtr = NULL;
        CanTp_Recieve_Counter[i] = 0;
        CanTp_secureLength_Recieve[i] = 0;
        CanTp_RxTimeoutCounter[i] = 0;
        CanTp_RxState[i] = CANTP_STATE_IDLE;
    }

    CanTp_Initialized = TRUE;
}

void CanTp_Shutdown(void)
{
    if (CanTp_Initialized == FALSE)
    {
        (void)Det_ReportError(CANTP_MODULE_ID, CANTP_INSTANCE_ID, CANTP_SID_SHUTDOWN, CANTP_E_UNINIT);
        return;
    }

    CanTp_Initialized = FALSE;
}

Std_ReturnType CanTp_Transmit(PduIdType CanTpTxSduId, const PduInfoType* CanTpTxInfoPtr)
{
    #ifdef CANTP_DEBUG
        printf("######## in CanTp_Transmit\n");
    #endif

    if (CanTp_Initialized == FALSE)
    {
        (void)Det_ReportError(CANTP_MODULE_ID, CANTP_INSTANCE_ID, CANTP_SID_TRANSMIT, CANTP_E_UNINIT);
        /* Backward compatibility: auto-init */
        CanTp_Init();
    }

    if (CanTpTxInfoPtr == NULL)
    {
        (void)Det_ReportError(CANTP_MODULE_ID, CANTP_INSTANCE_ID, CANTP_SID_TRANSMIT, CANTP_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    if (CanTpTxSduId >= SECOC_NUM_OF_TX_PDU_PROCESSING)
    {
        (void)Det_ReportError(CANTP_MODULE_ID, CANTP_INSTANCE_ID, CANTP_SID_TRANSMIT, CANTP_E_PARAM_ID);
        return E_NOT_OK;
    }

    if (CanTp_TxState[CanTpTxSduId] != CANTP_STATE_IDLE)
    {
        return E_NOT_OK;
    }

    CanTp_Buffer[CanTpTxSduId] = *CanTpTxInfoPtr;
    CanTp_TxTimeoutCounter[CanTpTxSduId] = 0;

    /* Determine if Single Frame or First Frame */
    if (CanTpTxInfoPtr->SduLength <= (PduLengthType)(BUS_LENGTH - 1U))
    {
        CanTp_TxState[CanTpTxSduId] = CANTP_STATE_TX_SF;
    }
    else
    {
        CanTp_TxState[CanTpTxSduId] = CANTP_STATE_TX_FF;
    }

    return E_OK;
}


void CanTp_RxIndication (PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{
    #ifdef CANTP_DEBUG
        printf("######## in CanTp_RxIndication\n");
    #endif

    if (CanTp_Initialized == FALSE)
    {
        (void)Det_ReportError(CANTP_MODULE_ID, CANTP_INSTANCE_ID, CANTP_SID_RX_INDICATION, CANTP_E_UNINIT);
        CanTp_Init();
    }

    if (PduInfoPtr == NULL)
    {
        (void)Det_ReportError(CANTP_MODULE_ID, CANTP_INSTANCE_ID, CANTP_SID_RX_INDICATION, CANTP_E_PARAM_POINTER);
        return;
    }
    if ((PduInfoPtr->SduLength > 0U) && (PduInfoPtr->SduDataPtr == NULL))
    {
        (void)Det_ReportError(CANTP_MODULE_ID, CANTP_INSTANCE_ID, CANTP_SID_RX_INDICATION, CANTP_E_PARAM_POINTER);
        return;
    }

    if (RxPduId >= SECOC_NUM_OF_RX_PDU_PROCESSING)
    {
        (void)Det_ReportError(CANTP_MODULE_ID, CANTP_INSTANCE_ID, CANTP_SID_RX_INDICATION, CANTP_E_PARAM_ID);
        return;
    }

    /* copy to CanTp buffer */
    CanTp_Buffer_Rx[RxPduId] = *PduInfoPtr;
    CanTp_RxTimeoutCounter[RxPduId] = 0;

    /* Check if it first frame :
        Check if there are a header of no
            if there are a header
                get the auth length from the frame
            else
                get the config length of data
        then add the Freshness , Mac and Header length
        to the the whole Secure Frame Length to recieve
    */
    if(CanTp_Recieve_Counter[RxPduId] == 0)
    {
        uint8 AuthHeadlen = SecOCRxPduProcessing[RxPduId].SecOCRxSecuredPduLayer->SecOCRxSecuredPdu->SecOCAuthPduHeaderLength;
        PduLengthType SecureDataframe = AuthHeadlen + BIT_TO_BYTES(SecOCRxPduProcessing[RxPduId].SecOCFreshnessValueTruncLength) + BIT_TO_BYTES(SecOCRxPduProcessing[RxPduId].SecOCAuthInfoTruncLength);
        if ((AuthHeadlen > 0U) &&
            ((AuthHeadlen > PduInfoPtr->SduLength) || (AuthHeadlen > (uint8)sizeof(PduLengthType))))
        {
            CanTp_Recieve_Counter[RxPduId] = 0U;
            CanTp_RxState[RxPduId] = CANTP_STATE_IDLE;
            return;
        }
        if(AuthHeadlen > 0)
        {
            (void)memcpy((uint8*)&CanTp_secureLength_Recieve[RxPduId], PduInfoPtr->SduDataPtr, AuthHeadlen );
        }
        else
        {
            CanTp_secureLength_Recieve[RxPduId] = SecOCRxPduProcessing[RxPduId].SecOCRxAuthenticPduLayer->SecOCRxAuthenticLayerPduRef.SduLength;
        }
        CanTp_secureLength_Recieve[RxPduId] += SecureDataframe;
        CanTp_RxState[RxPduId] = CANTP_STATE_RX_FF_RECEIVED;
    }
    else
    {
        CanTp_RxState[RxPduId] = CANTP_STATE_RX_CF;
    }
    CanTp_Recieve_Counter[RxPduId] ++;
}



void CanTp_MainFunctionTx(void)
{
    #ifdef CANTP_DEBUG
        printf("######## in CanTp_MainFunction\n");
    #endif

    if (CanTp_Initialized == FALSE)
    {
        return;
    }

    uint8 sdata[BUS_LENGTH] = {0};
    uint8 mdata[BUS_LENGTH] = {0};
    PduLengthType length = BUS_LENGTH;
    PduInfoType info = {sdata,mdata,length};

    TpDataStateType retrystate = TP_DATACONF;
    PduLengthType retrycout = BUS_LENGTH;
    RetryInfoType retry = {retrystate,retrycout};

    PduLengthType availableDataPtr = 0;
    for(PduIdType TxPduId = 0 ; TxPduId < SECOC_NUM_OF_TX_PDU_PROCESSING ; TxPduId++)
    {
        if (CanTp_TxState[TxPduId] == CANTP_STATE_IDLE)
        {
            continue;
        }

        /* Check for N_Cs timeout (sender side) */
        CanTp_TxTimeoutCounter[TxPduId]++;
        if (CanTp_TxTimeoutCounter[TxPduId] > CANTP_N_CS_TIMEOUT)
        {
            PduR_CanTpTxConfirmation(TxPduId, E_NOT_OK);
            CanTp_Buffer[TxPduId].SduLength = 0;
            CanTp_TxState[TxPduId] = CANTP_STATE_IDLE;
            CanTp_TxTimeoutCounter[TxPduId] = 0;
            continue;
        }

        if( CanTp_Buffer[TxPduId].SduLength > 0)
        {
            uint8 lastFrameIndex = ((CanTp_Buffer[TxPduId].SduLength % BUS_LENGTH) == 0)  ? (CanTp_Buffer[TxPduId].SduLength / BUS_LENGTH) : ((CanTp_Buffer[TxPduId].SduLength / BUS_LENGTH) + 1);
            #ifdef CANTP_DEBUG
                printf("Start sending id = %d\n" , TxPduId);
                printf("PDU length = %ld\n" , CanTp_Buffer[TxPduId].SduLength);
                printf("All Data to be Sent: \n");
                for(int i = 0 ; i < CanTp_Buffer[TxPduId].SduLength; i++)
                {
                    printf("%d  " , CanTp_Buffer[TxPduId].SduDataPtr[i]);
                }
                printf("\n\n\n");
            #endif
            for(int frameIndex = 0; frameIndex < lastFrameIndex ; frameIndex++)
            {
                if(frameIndex == (lastFrameIndex - 1))
                {
                    info.SduLength = ((CanTp_Buffer[TxPduId].SduLength % BUS_LENGTH) == 0)  ? (BUS_LENGTH) : (CanTp_Buffer[TxPduId].SduLength % BUS_LENGTH);
                    #ifdef CANTP_DEBUG
                    printf("last frame PDU length = %ld\n" , CanTp_Buffer[TxPduId].SduLength);
                    printf("All Data to be Sent: \n");
                    for(int i = 0 ; i < info.SduLength; i++)
                    {
                        printf("%d  " , info.SduDataPtr[i]);
                    }
                    printf("\n");
                    #endif
                }
                BufReq_ReturnType resultCopy = PduR_CanTpCopyTxData(TxPduId, &info, &retry, &availableDataPtr);
                Std_ReturnType resultTrasmit = CanIf_Transmit(TxPduId , &info);
                if((resultTrasmit != E_OK) || (resultCopy != BUFREQ_OK))
                {
                    retry.TpDataState = TP_DATARETRY;
                    frameIndex--; /* DEVIATION: Rule 14.2 - loop counter adjusted for TP segmentation */ // cppcheck-suppress misra-c2012-14.2
                }
                else if(resultTrasmit == E_OK)
                {
                    retry.TpDataState = TP_DATACONF;
                }
                else
                {
                    /* No action required */
                }

                #ifdef CANTP_DEBUG
                    printf("Transmit Result = %d\n" , resultTrasmit);
                #endif
            }

            PduR_CanTpTxConfirmation(TxPduId , E_OK);

            CanTp_Buffer[TxPduId].SduLength = 0;
            CanTp_TxState[TxPduId] = CANTP_STATE_IDLE;
            CanTp_TxTimeoutCounter[TxPduId] = 0;
        }
    }
}



void CanTp_TxConfirmation(PduIdType TxPduId, Std_ReturnType result)
{
    #ifdef CANTP_DEBUG
        printf("######## in CanTp_TxConfirmation \n");
    #endif

    if (CanTp_Initialized == FALSE)
    {
        (void)Det_ReportError(CANTP_MODULE_ID, CANTP_INSTANCE_ID, CANTP_SID_TX_CONFIRMATION, CANTP_E_UNINIT);
        return;
    }

    (void)TxPduId;
    (void)result;
}

void CanTp_MainFunctionRx(void)
{
    #ifdef CANTP_DEBUG
        printf("######## in CanTP_MainFunctionRx\n");
    #endif

    if (CanTp_Initialized == FALSE)
    {
        return;
    }

    for(PduIdType RxPduId = 0 ; RxPduId < SECOC_NUM_OF_RX_PDU_PROCESSING ; RxPduId++)
    {
        /* Check for N_Cr timeout on active Rx channels */
        if (CanTp_RxState[RxPduId] != CANTP_STATE_IDLE)
        {
            CanTp_RxTimeoutCounter[RxPduId]++;
            if (CanTp_RxTimeoutCounter[RxPduId] > CANTP_N_CR_TIMEOUT)
            {
                PduR_CanTpRxIndication(RxPduId, E_NOT_OK);
                CanTp_Recieve_Counter[RxPduId] = 0;
                CanTp_RxState[RxPduId] = CANTP_STATE_IDLE;
                CanTp_RxTimeoutCounter[RxPduId] = 0;
                continue;
            }
        }

        BufReq_ReturnType result = BUFREQ_OK;
        if((CanTp_Recieve_Counter[RxPduId] > 0) && (CanTp_Buffer_Rx[RxPduId].SduLength > 0))
        {
            uint8 lastFrameIndex = ((CanTp_secureLength_Recieve[RxPduId] % BUS_LENGTH) == 0)  ? (CanTp_secureLength_Recieve[RxPduId] / BUS_LENGTH) : ((CanTp_secureLength_Recieve[RxPduId] / BUS_LENGTH) + 1);
            PduLengthType bufferSizePtr;
            #ifdef CANTP_DEBUG
                printf("######## in main tp Rx  in id : %d\n", RxPduId);
                printf("for id %d :",RxPduId);
                for(int l = 0; l < CanTp_Buffer_Rx[RxPduId].SduLength; l++)
                {
                    printf("%d ", CanTp_Buffer_Rx[RxPduId].SduDataPtr[l]);
                }
                printf("\n");
            #endif
            if(CanTp_Recieve_Counter[RxPduId] == 1)
            {
                result = PduR_CanTpStartOfReception(RxPduId, &CanTp_Buffer_Rx[RxPduId], CanTp_secureLength_Recieve[RxPduId], &bufferSizePtr);
                if (result == BUFREQ_OK)
                {
                    result = PduR_CanTpCopyRxData(RxPduId, &CanTp_Buffer_Rx[RxPduId], &bufferSizePtr);
                }
                else
                {
                    CanTp_Recieve_Counter[RxPduId] = 0;
                    CanTp_RxState[RxPduId] = CANTP_STATE_IDLE;
                }
                CanTp_Buffer_Rx[RxPduId].SduLength = 0;
            }
            else if (CanTp_Recieve_Counter[RxPduId] == lastFrameIndex)
            {
                CanTp_Buffer_Rx[RxPduId].SduLength = ((CanTp_secureLength_Recieve[RxPduId] % BUS_LENGTH) == 0) ? (BUS_LENGTH) : (CanTp_secureLength_Recieve[RxPduId] % BUS_LENGTH);
                result = PduR_CanTpCopyRxData(RxPduId, &CanTp_Buffer_Rx[RxPduId], &bufferSizePtr);
                PduR_CanTpRxIndication(RxPduId, result);
                CanTp_Recieve_Counter[RxPduId] = 0;
                CanTp_RxState[RxPduId] = CANTP_STATE_IDLE;
                CanTp_RxTimeoutCounter[RxPduId] = 0;
            }
            else
            {
                result = PduR_CanTpCopyRxData(RxPduId, &CanTp_Buffer_Rx[RxPduId], &bufferSizePtr);
                CanTp_RxTimeoutCounter[RxPduId] = 0; /* Reset on each CF received */
            }
            #ifdef SCHEDULER_ON
                pthread_mutex_unlock(&lock);
            #endif
        }
    }

}
