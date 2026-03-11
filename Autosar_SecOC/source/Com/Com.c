/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "Com.h"
#include "PduR_Com.h"
#include "SecOC_Lcfg.h"
#include "SecOC_Debug.h"
#include <string.h>
#include <stdio.h>

/********************************************************************************************************/
/************************************************Defines*************************************************/
/********************************************************************************************************/

#define COM_NUM_OF_TX_IPDU                    ((PduIdType)SECOC_NUM_OF_TX_PDU_PROCESSING)
#define COM_NUM_OF_RX_IPDU                    ((PduIdType)SECOC_NUM_OF_RX_PDU_PROCESSING)
#define COM_NUM_OF_SIGNALS                    ((Com_SignalIdType)SECOC_NUM_OF_TX_PDU_PROCESSING)
#define COM_SIGNAL_MAX_LENGTH                 ((uint8)SECOC_AUTHPDU_MAX_LENGTH)
#define COM_DEFAULT_TX_TIMEOUT_TICKS          ((uint16)20U)
#define COM_DEFAULT_RX_TIMEOUT_TICKS          ((uint16)20U)
#define COM_SIGNAL_FILTER_MASK                ((uint8)0xFFU)

/********************************************************************************************************/
/*******************************************TypeDefinitions**********************************************/
/********************************************************************************************************/

typedef struct
{
    boolean ComSignalUpdated;
    uint8 ComSignalValue;
    uint8 ComSignalLastDeliveredValue;
    uint8 ComSignalFilterMask;
} Com_SignalRuntimeType;

typedef struct
{
    boolean ComTxPending;
    boolean ComTxConfirmed;
    boolean ComTxDeadlineTimeout;
    uint16 ComTxDeadlineCounter;
    uint16 ComTxDeadlineLimit;
    uint8 ComTxLength;
    uint8 ComTxBuffer[COM_SIGNAL_MAX_LENGTH];
} Com_TxIpduRuntimeType;

typedef struct
{
    boolean ComRxAvailable;
    boolean ComRxDeadlineTimeout;
    uint16 ComRxDeadlineCounter;
    uint16 ComRxDeadlineLimit;
    uint8 ComRxLength;
    uint8 ComRxBuffer[COM_SIGNAL_MAX_LENGTH];
} Com_RxIpduRuntimeType;

typedef struct
{
    boolean ComIpduGroupStarted;
} Com_IpduGroupRuntimeType;

/********************************************************************************************************/
/******************************************GlobalVaribles************************************************/
/********************************************************************************************************/

static boolean Com_Initialized = FALSE;
static Com_SignalRuntimeType Com_SignalRuntime[COM_NUM_OF_SIGNALS];
static Com_TxIpduRuntimeType Com_TxIpduRuntime[COM_NUM_OF_TX_IPDU];
static Com_RxIpduRuntimeType Com_RxIpduRuntime[COM_NUM_OF_RX_IPDU];
static Com_IpduGroupRuntimeType Com_IpduGroupRuntime[COM_NUM_OF_IPDU_GROUPS];

/********************************************************************************************************/
/**************************************Static Helper Functions*******************************************/
/********************************************************************************************************/

static boolean Com_IsIpduGroupActiveForPdu(PduIdType PduId)
{
    (void)PduId;
    return Com_IpduGroupRuntime[0U].ComIpduGroupStarted;
}

static Std_ReturnType Com_CopySignalToIpdu(Com_SignalIdType SignalId)
{
    PduIdType txPduId = (PduIdType)SignalId;
    if ((SignalId >= COM_NUM_OF_SIGNALS) || (txPduId >= COM_NUM_OF_TX_IPDU))
    {
        return E_NOT_OK;
    }

    Com_TxIpduRuntime[txPduId].ComTxBuffer[0U] = Com_SignalRuntime[SignalId].ComSignalValue;
    if (Com_TxIpduRuntime[txPduId].ComTxLength < 1U)
    {
        Com_TxIpduRuntime[txPduId].ComTxLength = 1U;
    }
    Com_TxIpduRuntime[txPduId].ComTxPending = TRUE;
    Com_TxIpduRuntime[txPduId].ComTxConfirmed = FALSE;
    Com_TxIpduRuntime[txPduId].ComTxDeadlineTimeout = FALSE;
    Com_TxIpduRuntime[txPduId].ComTxDeadlineCounter = 0U;
    return E_OK;
}

/********************************************************************************************************/
/********************************************Functions***************************************************/
/********************************************************************************************************/

void Com_Init(void)
{
    PduIdType pduId;
    Com_SignalIdType signalId;

    for (signalId = 0U; signalId < COM_NUM_OF_SIGNALS; signalId++)
    {
        Com_SignalRuntime[signalId].ComSignalUpdated = FALSE;
        Com_SignalRuntime[signalId].ComSignalValue = 0U;
        Com_SignalRuntime[signalId].ComSignalLastDeliveredValue = 0U;
        Com_SignalRuntime[signalId].ComSignalFilterMask = COM_SIGNAL_FILTER_MASK;
    }

    for (pduId = 0U; pduId < COM_NUM_OF_TX_IPDU; pduId++)
    {
        Com_TxIpduRuntime[pduId].ComTxPending = FALSE;
        Com_TxIpduRuntime[pduId].ComTxConfirmed = TRUE;
        Com_TxIpduRuntime[pduId].ComTxDeadlineTimeout = FALSE;
        Com_TxIpduRuntime[pduId].ComTxDeadlineCounter = 0U;
        Com_TxIpduRuntime[pduId].ComTxDeadlineLimit = COM_DEFAULT_TX_TIMEOUT_TICKS;
        Com_TxIpduRuntime[pduId].ComTxLength = 1U;
        (void)memset(Com_TxIpduRuntime[pduId].ComTxBuffer, 0, COM_SIGNAL_MAX_LENGTH);
    }

    for (pduId = 0U; pduId < COM_NUM_OF_RX_IPDU; pduId++)
    {
        Com_RxIpduRuntime[pduId].ComRxAvailable = FALSE;
        Com_RxIpduRuntime[pduId].ComRxDeadlineTimeout = FALSE;
        Com_RxIpduRuntime[pduId].ComRxDeadlineCounter = 0U;
        Com_RxIpduRuntime[pduId].ComRxDeadlineLimit = COM_DEFAULT_RX_TIMEOUT_TICKS;
        Com_RxIpduRuntime[pduId].ComRxLength = 0U;
        (void)memset(Com_RxIpduRuntime[pduId].ComRxBuffer, 0, COM_SIGNAL_MAX_LENGTH);
    }

    Com_IpduGroupRuntime[0U].ComIpduGroupStarted = TRUE;
    Com_Initialized = TRUE;
}

Std_ReturnType Com_SendSignal(Com_SignalIdType SignalId, const uint8* SignalDataPtr)
{
    if (Com_Initialized == FALSE)
    {
        return E_NOT_OK;
    }
    if (SignalDataPtr == NULL)
    {
        return E_NOT_OK;
    }
    if (SignalId >= COM_NUM_OF_SIGNALS)
    {
        return E_NOT_OK;
    }

    Com_SignalRuntime[SignalId].ComSignalValue = *SignalDataPtr;
    Com_SignalRuntime[SignalId].ComSignalUpdated = TRUE;

    return Com_CopySignalToIpdu(SignalId);
}

Std_ReturnType Com_ReceiveSignal(Com_SignalIdType SignalId, uint8* SignalDataPtr)
{
    if (Com_Initialized == FALSE)
    {
        return E_NOT_OK;
    }
    if (SignalDataPtr == NULL)
    {
        return E_NOT_OK;
    }
    if (SignalId >= COM_NUM_OF_SIGNALS)
    {
        return E_NOT_OK;
    }

    *SignalDataPtr = Com_SignalRuntime[SignalId].ComSignalValue;
    return E_OK;
}

void Com_IpduGroupStart(Com_IpduGroupIdType IpduGroupId, boolean Initialize)
{
    if ((Com_Initialized == FALSE) || (IpduGroupId >= COM_NUM_OF_IPDU_GROUPS))
    {
        return;
    }

    Com_IpduGroupRuntime[IpduGroupId].ComIpduGroupStarted = TRUE;

    if (Initialize == TRUE)
    {
        PduIdType pduId;
        for (pduId = 0U; pduId < COM_NUM_OF_TX_IPDU; pduId++)
        {
            Com_TxIpduRuntime[pduId].ComTxPending = FALSE;
            Com_TxIpduRuntime[pduId].ComTxConfirmed = TRUE;
            Com_TxIpduRuntime[pduId].ComTxDeadlineCounter = 0U;
            Com_TxIpduRuntime[pduId].ComTxDeadlineTimeout = FALSE;
            (void)memset(Com_TxIpduRuntime[pduId].ComTxBuffer, 0, COM_SIGNAL_MAX_LENGTH);
        }
    }
}

void Com_IpduGroupStop(Com_IpduGroupIdType IpduGroupId)
{
    if ((Com_Initialized == FALSE) || (IpduGroupId >= COM_NUM_OF_IPDU_GROUPS))
    {
        return;
    }

    Com_IpduGroupRuntime[IpduGroupId].ComIpduGroupStarted = FALSE;
}

Std_ReturnType Com_TriggerIPDUSend(PduIdType TxPduId)
{
    PduInfoType comPduInfo;

    if (Com_Initialized == FALSE)
    {
        return E_NOT_OK;
    }
    if (TxPduId >= COM_NUM_OF_TX_IPDU)
    {
        return E_NOT_OK;
    }
    if (Com_IsIpduGroupActiveForPdu(TxPduId) == FALSE)
    {
        return E_NOT_OK;
    }

    comPduInfo.MetaDataPtr = NULL;
    comPduInfo.SduDataPtr = Com_TxIpduRuntime[TxPduId].ComTxBuffer;
    comPduInfo.SduLength = Com_TxIpduRuntime[TxPduId].ComTxLength;

    Com_TxIpduRuntime[TxPduId].ComTxConfirmed = FALSE;
    Com_TxIpduRuntime[TxPduId].ComTxPending = FALSE;
    Com_TxIpduRuntime[TxPduId].ComTxDeadlineCounter = 0U;
    Com_TxIpduRuntime[TxPduId].ComTxDeadlineTimeout = FALSE;

    if (PduR_ComTransmit(TxPduId, &comPduInfo) != E_OK)
    {
        Com_TxIpduRuntime[TxPduId].ComTxPending = TRUE;
        return E_NOT_OK;
    }

    return E_OK;
}

void Com_TxConfirmation(PduIdType TxPduId, Std_ReturnType result)
{
#ifdef COM_DEBUG
    printf("######## in Com_TxConfirmation \n");
#endif
    if ((Com_Initialized == FALSE) || (TxPduId >= COM_NUM_OF_TX_IPDU))
    {
        return;
    }

    if (result == E_OK)
    {
        Com_TxIpduRuntime[TxPduId].ComTxConfirmed = TRUE;
        Com_TxIpduRuntime[TxPduId].ComTxDeadlineCounter = 0U;
        Com_TxIpduRuntime[TxPduId].ComTxDeadlineTimeout = FALSE;
    }
    else
    {
        Com_TxIpduRuntime[TxPduId].ComTxPending = TRUE;
    }
}

void Com_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{
    uint8 rxValue;

#ifdef COM_DEBUG
    printf("######## in Com_RxIndication\n");
#endif

    if ((Com_Initialized == FALSE) || (PduInfoPtr == NULL) || (RxPduId >= COM_NUM_OF_RX_IPDU))
    {
        return;
    }
    if ((PduInfoPtr->SduDataPtr == NULL) || (PduInfoPtr->SduLength == 0U))
    {
        return;
    }

    Com_RxIpduRuntime[RxPduId].ComRxLength = (uint8)MIN(PduInfoPtr->SduLength, (PduLengthType)COM_SIGNAL_MAX_LENGTH);
    (void)memcpy(Com_RxIpduRuntime[RxPduId].ComRxBuffer,
                 PduInfoPtr->SduDataPtr,
                 Com_RxIpduRuntime[RxPduId].ComRxLength);
    Com_RxIpduRuntime[RxPduId].ComRxAvailable = TRUE;
    Com_RxIpduRuntime[RxPduId].ComRxDeadlineCounter = 0U;
    Com_RxIpduRuntime[RxPduId].ComRxDeadlineTimeout = FALSE;

    /* Apply a simple masked-value filter before updating application signal shadow. */
    rxValue = Com_RxIpduRuntime[RxPduId].ComRxBuffer[0U];
    if ((rxValue & Com_SignalRuntime[RxPduId].ComSignalFilterMask) !=
        (Com_SignalRuntime[RxPduId].ComSignalLastDeliveredValue & Com_SignalRuntime[RxPduId].ComSignalFilterMask))
    {
        Com_SignalRuntime[RxPduId].ComSignalValue = rxValue;
        Com_SignalRuntime[RxPduId].ComSignalLastDeliveredValue = rxValue;
        Com_SignalRuntime[RxPduId].ComSignalUpdated = TRUE;
    }
}

void Com_MainFunctionTx(void)
{
    PduIdType txPduId;

    if (Com_Initialized == FALSE)
    {
        return;
    }

    for (txPduId = 0U; txPduId < COM_NUM_OF_TX_IPDU; txPduId++)
    {
        if ((Com_TxIpduRuntime[txPduId].ComTxPending == TRUE) &&
            (Com_IsIpduGroupActiveForPdu(txPduId) == TRUE))
        {
            (void)Com_TriggerIPDUSend(txPduId);
        }

        if (Com_TxIpduRuntime[txPduId].ComTxConfirmed == FALSE)
        {
            if (Com_TxIpduRuntime[txPduId].ComTxDeadlineCounter < 0xFFFFU)
            {
                Com_TxIpduRuntime[txPduId].ComTxDeadlineCounter++;
            }
            if (Com_TxIpduRuntime[txPduId].ComTxDeadlineCounter > Com_TxIpduRuntime[txPduId].ComTxDeadlineLimit)
            {
                Com_TxIpduRuntime[txPduId].ComTxDeadlineTimeout = TRUE;
                Com_TxIpduRuntime[txPduId].ComTxPending = TRUE;
            }
        }
    }
}

void Com_MainFunctionRx(void)
{
    PduIdType rxPduId;

    if (Com_Initialized == FALSE)
    {
        return;
    }

    for (rxPduId = 0U; rxPduId < COM_NUM_OF_RX_IPDU; rxPduId++)
    {
        if (Com_RxIpduRuntime[rxPduId].ComRxAvailable == TRUE)
        {
            Com_RxIpduRuntime[rxPduId].ComRxAvailable = FALSE;
        }
        else
        {
            if (Com_RxIpduRuntime[rxPduId].ComRxDeadlineCounter < 0xFFFFU)
            {
                Com_RxIpduRuntime[rxPduId].ComRxDeadlineCounter++;
            }
            if (Com_RxIpduRuntime[rxPduId].ComRxDeadlineCounter > Com_RxIpduRuntime[rxPduId].ComRxDeadlineLimit)
            {
                Com_RxIpduRuntime[rxPduId].ComRxDeadlineTimeout = TRUE;
            }
        }
    }
}

void Com_MainTx(void)
{
#ifdef COM_DEBUG
    printf("######## in Com_MainTx\n");
#endif
    Com_MainFunctionTx();
}