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
#define COM_SIGNAL_MAX_LENGTH                 ((uint16)SECOC_AUTHPDU_MAX_LENGTH)
#define COM_DEFAULT_TX_TIMEOUT_TICKS          ((uint16)20U)
#define COM_DEFAULT_RX_TIMEOUT_TICKS          ((uint16)20U)
#define COM_SIGNAL_FILTER_MASK                ((uint8)0xFFU)
#define COM_DEFAULT_INVALID_VALUE             ((uint8)0xFFU)

/********************************************************************************************************/
/*******************************************TypeDefinitions**********************************************/
/********************************************************************************************************/

typedef struct
{
    PduIdType ComTxPduId;
    PduIdType ComRxPduId;
    uint16 ComSignalPosition;
    uint16 ComSignalLength;
    uint16 ComSignalMaxLength;
    Com_IpduGroupIdType ComIpduGroupId;
    uint8 ComInvalidValue;
    uint8 ComSignalFilterMask;
} Com_SignalConfigType;

typedef struct
{
    Com_SignalIdType ComFirstSignalId;
    uint16 ComSignalCount;
    Com_IpduGroupIdType ComIpduGroupId;
} Com_SignalGroupConfigType;

typedef struct
{
    Com_IpduGroupIdType ComIpduGroupId;
    uint16 ComTxDeadlineLimit;
} Com_TxIpduConfigType;

typedef struct
{
    Com_IpduGroupIdType ComIpduGroupId;
    uint16 ComRxDeadlineLimit;
} Com_RxIpduConfigType;

typedef struct
{
    boolean ComSignalUpdated;
    boolean ComSignalInvalid;
    uint16 ComSignalLength;
    uint8 ComSignalValue[COM_SIGNAL_MAX_LENGTH];
    uint8 ComSignalLastDeliveredValue;
} Com_SignalRuntimeType;

typedef struct
{
    boolean ComTxPending;
    boolean ComTxConfirmed;
    boolean ComTxDeadlineTimeout;
    uint16 ComTxDeadlineCounter;
    uint16 ComTxDeadlineLimit;
    uint16 ComTxLength;
    uint8 ComTxBuffer[COM_SIGNAL_MAX_LENGTH];
} Com_TxIpduRuntimeType;

typedef struct
{
    boolean ComRxAvailable;
    boolean ComRxTpInProgress;
    boolean ComRxDeadlineTimeout;
    uint16 ComRxDeadlineCounter;
    uint16 ComRxDeadlineLimit;
    uint16 ComRxLength;
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
static Com_SignalConfigType Com_SignalConfig[COM_NUM_OF_SIGNALS];
static Com_SignalGroupConfigType Com_SignalGroupConfig[COM_NUM_OF_SIGNAL_GROUPS];
static Com_TxIpduConfigType Com_TxIpduConfig[COM_NUM_OF_TX_IPDU];
static Com_RxIpduConfigType Com_RxIpduConfig[COM_NUM_OF_RX_IPDU];

static Com_SignalRuntimeType Com_SignalRuntime[COM_NUM_OF_SIGNALS];
static Com_TxIpduRuntimeType Com_TxIpduRuntime[COM_NUM_OF_TX_IPDU];
static Com_RxIpduRuntimeType Com_RxIpduRuntime[COM_NUM_OF_RX_IPDU];
static Com_IpduGroupRuntimeType Com_IpduGroupRuntime[COM_NUM_OF_IPDU_GROUPS];

/********************************************************************************************************/
/**************************************Static Helper Functions*******************************************/
/********************************************************************************************************/

static uint16 Com_MinLength(uint16 Left, uint16 Right)
{
    uint16 Result;
    if (Left < Right)
    {
        Result = Left;
    }
    else
    {
        Result = Right;
    }
    return Result;
}

static boolean Com_IsValidSignalId(Com_SignalIdType SignalId)
{
    return (boolean)(SignalId < COM_NUM_OF_SIGNALS);
}

static boolean Com_IsValidTxPduId(PduIdType TxPduId)
{
    return (boolean)(TxPduId < COM_NUM_OF_TX_IPDU);
}

static boolean Com_IsValidRxPduId(PduIdType RxPduId)
{
    return (boolean)(RxPduId < COM_NUM_OF_RX_IPDU);
}

static void Com_LoadDefaultConfiguration(void)
{
    PduIdType pduId;
    Com_SignalIdType signalId;

    for (pduId = 0U; pduId < COM_NUM_OF_TX_IPDU; pduId++)
    {
        Com_TxIpduConfig[pduId].ComIpduGroupId = (Com_IpduGroupIdType)(pduId % COM_NUM_OF_IPDU_GROUPS);
        Com_TxIpduConfig[pduId].ComTxDeadlineLimit =
            (uint16)(COM_DEFAULT_TX_TIMEOUT_TICKS + (uint16)(pduId % 3U));
    }

    for (pduId = 0U; pduId < COM_NUM_OF_RX_IPDU; pduId++)
    {
        Com_RxIpduConfig[pduId].ComIpduGroupId = (Com_IpduGroupIdType)(pduId % COM_NUM_OF_IPDU_GROUPS);
        Com_RxIpduConfig[pduId].ComRxDeadlineLimit =
            (uint16)(COM_DEFAULT_RX_TIMEOUT_TICKS + (uint16)(pduId % 3U));
    }

    for (signalId = 0U; signalId < COM_NUM_OF_SIGNALS; signalId++)
    {
        Com_SignalConfig[signalId].ComTxPduId = (PduIdType)(signalId % COM_NUM_OF_TX_IPDU);
        Com_SignalConfig[signalId].ComRxPduId = (PduIdType)(signalId % COM_NUM_OF_RX_IPDU);
        Com_SignalConfig[signalId].ComSignalPosition = 0U;
        Com_SignalConfig[signalId].ComSignalLength = 1U;
        Com_SignalConfig[signalId].ComSignalMaxLength = COM_SIGNAL_MAX_LENGTH;
        Com_SignalConfig[signalId].ComIpduGroupId =
            Com_TxIpduConfig[Com_SignalConfig[signalId].ComTxPduId].ComIpduGroupId;
        Com_SignalConfig[signalId].ComInvalidValue = COM_DEFAULT_INVALID_VALUE;
        Com_SignalConfig[signalId].ComSignalFilterMask = COM_SIGNAL_FILTER_MASK;
    }

    Com_SignalGroupConfig[0U].ComFirstSignalId = 0U;
    Com_SignalGroupConfig[0U].ComSignalCount = (uint16)(COM_NUM_OF_SIGNALS / 2U);
    Com_SignalGroupConfig[0U].ComIpduGroupId = 0U;

    Com_SignalGroupConfig[1U].ComFirstSignalId = (Com_SignalIdType)(COM_NUM_OF_SIGNALS / 2U);
    Com_SignalGroupConfig[1U].ComSignalCount =
        (uint16)(COM_NUM_OF_SIGNALS - (Com_SignalIdType)(COM_NUM_OF_SIGNALS / 2U));
    Com_SignalGroupConfig[1U].ComIpduGroupId = (Com_IpduGroupIdType)((COM_NUM_OF_IPDU_GROUPS > 1U) ? 1U : 0U);
}

static void Com_ResetRuntime(boolean IsInit)
{
    PduIdType pduId;
    Com_SignalIdType signalId;

    for (signalId = 0U; signalId < COM_NUM_OF_SIGNALS; signalId++)
    {
        Com_SignalRuntime[signalId].ComSignalUpdated = FALSE;
        Com_SignalRuntime[signalId].ComSignalInvalid = FALSE;
        Com_SignalRuntime[signalId].ComSignalLength = Com_SignalConfig[signalId].ComSignalLength;
        Com_SignalRuntime[signalId].ComSignalLastDeliveredValue = 0U;
        (void)memset(Com_SignalRuntime[signalId].ComSignalValue, 0, COM_SIGNAL_MAX_LENGTH);
    }

    for (pduId = 0U; pduId < COM_NUM_OF_TX_IPDU; pduId++)
    {
        Com_TxIpduRuntime[pduId].ComTxPending = FALSE;
        Com_TxIpduRuntime[pduId].ComTxConfirmed = TRUE;
        Com_TxIpduRuntime[pduId].ComTxDeadlineTimeout = FALSE;
        Com_TxIpduRuntime[pduId].ComTxDeadlineCounter = 0U;
        Com_TxIpduRuntime[pduId].ComTxDeadlineLimit = Com_TxIpduConfig[pduId].ComTxDeadlineLimit;
        Com_TxIpduRuntime[pduId].ComTxLength = 0U;
        (void)memset(Com_TxIpduRuntime[pduId].ComTxBuffer, 0, COM_SIGNAL_MAX_LENGTH);
    }

    for (pduId = 0U; pduId < COM_NUM_OF_RX_IPDU; pduId++)
    {
        Com_RxIpduRuntime[pduId].ComRxAvailable = FALSE;
        Com_RxIpduRuntime[pduId].ComRxTpInProgress = FALSE;
        Com_RxIpduRuntime[pduId].ComRxDeadlineTimeout = FALSE;
        Com_RxIpduRuntime[pduId].ComRxDeadlineCounter = 0U;
        Com_RxIpduRuntime[pduId].ComRxDeadlineLimit = Com_RxIpduConfig[pduId].ComRxDeadlineLimit;
        Com_RxIpduRuntime[pduId].ComRxLength = 0U;
        (void)memset(Com_RxIpduRuntime[pduId].ComRxBuffer, 0, COM_SIGNAL_MAX_LENGTH);
    }

    for (pduId = 0U; pduId < COM_NUM_OF_IPDU_GROUPS; pduId++)
    {
        Com_IpduGroupRuntime[pduId].ComIpduGroupStarted = (boolean)(IsInit == TRUE);
    }
}

static boolean Com_IsIpduGroupActiveForTxPdu(PduIdType TxPduId)
{
    Com_IpduGroupIdType GroupId;
    if (Com_IsValidTxPduId(TxPduId) == FALSE)
    {
        return FALSE;
    }
    GroupId = Com_TxIpduConfig[TxPduId].ComIpduGroupId;
    if (GroupId >= COM_NUM_OF_IPDU_GROUPS)
    {
        return FALSE;
    }
    return Com_IpduGroupRuntime[GroupId].ComIpduGroupStarted;
}

static boolean Com_IsIpduGroupActiveForRxPdu(PduIdType RxPduId)
{
    Com_IpduGroupIdType GroupId;
    if (Com_IsValidRxPduId(RxPduId) == FALSE)
    {
        return FALSE;
    }
    GroupId = Com_RxIpduConfig[RxPduId].ComIpduGroupId;
    if (GroupId >= COM_NUM_OF_IPDU_GROUPS)
    {
        return FALSE;
    }
    return Com_IpduGroupRuntime[GroupId].ComIpduGroupStarted;
}

static Std_ReturnType Com_CopySignalToIpdu(Com_SignalIdType SignalId, uint16 SignalLength)
{
    PduIdType TxPduId;
    uint16 SignalPosition;
    uint16 CopyLength;
    uint16 NewTxLength;

    if (Com_IsValidSignalId(SignalId) == FALSE)
    {
        return E_NOT_OK;
    }

    TxPduId = Com_SignalConfig[SignalId].ComTxPduId;
    if (Com_IsValidTxPduId(TxPduId) == FALSE)
    {
        return E_NOT_OK;
    }

    SignalPosition = Com_SignalConfig[SignalId].ComSignalPosition;
    if (SignalPosition >= COM_SIGNAL_MAX_LENGTH)
    {
        return E_NOT_OK;
    }

    CopyLength = Com_MinLength(SignalLength, Com_SignalConfig[SignalId].ComSignalMaxLength);
    if ((uint32)SignalPosition + (uint32)CopyLength > (uint32)COM_SIGNAL_MAX_LENGTH)
    {
        return E_NOT_OK;
    }

    (void)memcpy(&Com_TxIpduRuntime[TxPduId].ComTxBuffer[SignalPosition],
                 Com_SignalRuntime[SignalId].ComSignalValue,
                 CopyLength);

    NewTxLength = (uint16)(SignalPosition + CopyLength);
    if (Com_TxIpduRuntime[TxPduId].ComTxLength < NewTxLength)
    {
        Com_TxIpduRuntime[TxPduId].ComTxLength = NewTxLength;
    }

    Com_TxIpduRuntime[TxPduId].ComTxPending = TRUE;
    Com_TxIpduRuntime[TxPduId].ComTxConfirmed = FALSE;
    Com_TxIpduRuntime[TxPduId].ComTxDeadlineTimeout = FALSE;
    Com_TxIpduRuntime[TxPduId].ComTxDeadlineCounter = 0U;

    return E_OK;
}

static void Com_UpdateMappedRxSignals(PduIdType RxPduId)
{
    Com_SignalIdType SignalId;
    uint16 SignalPosition;
    uint16 CopyLength;
    uint8 RxFilteredValue;

    for (SignalId = 0U; SignalId < COM_NUM_OF_SIGNALS; SignalId++)
    {
        if (Com_SignalConfig[SignalId].ComRxPduId != RxPduId)
        {
            continue;
        }

        SignalPosition = Com_SignalConfig[SignalId].ComSignalPosition;
        if (SignalPosition >= Com_RxIpduRuntime[RxPduId].ComRxLength)
        {
            continue;
        }

        CopyLength = Com_MinLength(Com_SignalConfig[SignalId].ComSignalLength,
                                   (uint16)(Com_RxIpduRuntime[RxPduId].ComRxLength - SignalPosition));
        if (CopyLength == 0U)
        {
            continue;
        }

        RxFilteredValue = (uint8)(Com_RxIpduRuntime[RxPduId].ComRxBuffer[SignalPosition] &
                                  Com_SignalConfig[SignalId].ComSignalFilterMask);
        if (RxFilteredValue != (uint8)(Com_SignalRuntime[SignalId].ComSignalLastDeliveredValue &
                                       Com_SignalConfig[SignalId].ComSignalFilterMask))
        {
            (void)memcpy(Com_SignalRuntime[SignalId].ComSignalValue,
                         &Com_RxIpduRuntime[RxPduId].ComRxBuffer[SignalPosition],
                         CopyLength);
            Com_SignalRuntime[SignalId].ComSignalLength = CopyLength;
            Com_SignalRuntime[SignalId].ComSignalLastDeliveredValue =
                Com_RxIpduRuntime[RxPduId].ComRxBuffer[SignalPosition];
            Com_SignalRuntime[SignalId].ComSignalUpdated = TRUE;
            Com_SignalRuntime[SignalId].ComSignalInvalid = FALSE;
        }
    }
}

/********************************************************************************************************/
/********************************************Functions***************************************************/
/********************************************************************************************************/

void Com_Init(void)
{
    Com_LoadDefaultConfiguration();
    Com_ResetRuntime(TRUE);
    Com_Initialized = TRUE;
}

void Com_DeInit(void)
{
    if (Com_Initialized == FALSE)
    {
        return;
    }

    Com_ResetRuntime(FALSE);
    Com_Initialized = FALSE;
}

Std_ReturnType Com_SendSignal(Com_SignalIdType SignalId, const uint8* SignalDataPtr)
{
    return Com_SendDynSignal(SignalId, SignalDataPtr,
                             (Com_IsValidSignalId(SignalId) == TRUE) ?
                             Com_SignalConfig[SignalId].ComSignalLength : 0U);
}

Std_ReturnType Com_ReceiveSignal(Com_SignalIdType SignalId, uint8* SignalDataPtr)
{
    if ((Com_Initialized == FALSE) || (SignalDataPtr == NULL) || (Com_IsValidSignalId(SignalId) == FALSE))
    {
        return E_NOT_OK;
    }

    *SignalDataPtr = Com_SignalRuntime[SignalId].ComSignalValue[0U];
    return E_OK;
}

Std_ReturnType Com_SendDynSignal(Com_SignalIdType SignalId, const uint8* SignalDataPtr, uint16 Length)
{
    uint16 CopyLength;

    if (Com_Initialized == FALSE)
    {
        return E_NOT_OK;
    }
    if ((SignalDataPtr == NULL) || (Com_IsValidSignalId(SignalId) == FALSE))
    {
        return E_NOT_OK;
    }
    if ((Length == 0U) || (Length > Com_SignalConfig[SignalId].ComSignalMaxLength))
    {
        return E_NOT_OK;
    }

    CopyLength = Com_MinLength(Length, Com_SignalConfig[SignalId].ComSignalMaxLength);
    (void)memcpy(Com_SignalRuntime[SignalId].ComSignalValue, SignalDataPtr, CopyLength);
    Com_SignalRuntime[SignalId].ComSignalLength = CopyLength;
    Com_SignalRuntime[SignalId].ComSignalUpdated = TRUE;
    Com_SignalRuntime[SignalId].ComSignalInvalid = FALSE;

    return Com_CopySignalToIpdu(SignalId, CopyLength);
}

Std_ReturnType Com_ReceiveDynSignal(Com_SignalIdType SignalId, uint8* SignalDataPtr, uint16* LengthPtr)
{
    uint16 CopyLength;

    if ((Com_Initialized == FALSE) || (SignalDataPtr == NULL) || (LengthPtr == NULL))
    {
        return E_NOT_OK;
    }
    if (Com_IsValidSignalId(SignalId) == FALSE)
    {
        return E_NOT_OK;
    }

    CopyLength = Com_MinLength(Com_SignalRuntime[SignalId].ComSignalLength, *LengthPtr);
    (void)memcpy(SignalDataPtr, Com_SignalRuntime[SignalId].ComSignalValue, CopyLength);
    *LengthPtr = CopyLength;

    return E_OK;
}

Std_ReturnType Com_InvalidateSignal(Com_SignalIdType SignalId)
{
    uint16 Index;
    uint16 SignalLength;

    if ((Com_Initialized == FALSE) || (Com_IsValidSignalId(SignalId) == FALSE))
    {
        return E_NOT_OK;
    }

    SignalLength = Com_SignalConfig[SignalId].ComSignalLength;
    for (Index = 0U; Index < SignalLength; Index++)
    {
        Com_SignalRuntime[SignalId].ComSignalValue[Index] = Com_SignalConfig[SignalId].ComInvalidValue;
    }
    Com_SignalRuntime[SignalId].ComSignalLength = SignalLength;
    Com_SignalRuntime[SignalId].ComSignalInvalid = TRUE;
    Com_SignalRuntime[SignalId].ComSignalUpdated = TRUE;

    return Com_CopySignalToIpdu(SignalId, SignalLength);
}

Std_ReturnType Com_SendSignalGroup(Com_SignalGroupIdType SignalGroupId)
{
    Com_SignalIdType SignalId;
    Com_SignalIdType FirstId;
    uint16 Count;

    if ((Com_Initialized == FALSE) || (SignalGroupId >= COM_NUM_OF_SIGNAL_GROUPS))
    {
        return E_NOT_OK;
    }

    if (Com_IpduGroupRuntime[Com_SignalGroupConfig[SignalGroupId].ComIpduGroupId].ComIpduGroupStarted == FALSE)
    {
        return E_NOT_OK;
    }

    FirstId = Com_SignalGroupConfig[SignalGroupId].ComFirstSignalId;
    Count = Com_SignalGroupConfig[SignalGroupId].ComSignalCount;

    for (SignalId = FirstId; SignalId < (Com_SignalIdType)(FirstId + Count); SignalId++)
    {
        if (SignalId < COM_NUM_OF_SIGNALS)
        {
            (void)Com_CopySignalToIpdu(SignalId, Com_SignalRuntime[SignalId].ComSignalLength);
        }
    }

    return E_OK;
}

Std_ReturnType Com_ReceiveSignalGroup(Com_SignalGroupIdType SignalGroupId)
{
    Com_SignalIdType SignalId;
    Com_SignalIdType FirstId;
    uint16 Count;
    boolean AnyUpdated = FALSE;

    if ((Com_Initialized == FALSE) || (SignalGroupId >= COM_NUM_OF_SIGNAL_GROUPS))
    {
        return E_NOT_OK;
    }

    if (Com_IpduGroupRuntime[Com_SignalGroupConfig[SignalGroupId].ComIpduGroupId].ComIpduGroupStarted == FALSE)
    {
        return E_NOT_OK;
    }

    FirstId = Com_SignalGroupConfig[SignalGroupId].ComFirstSignalId;
    Count = Com_SignalGroupConfig[SignalGroupId].ComSignalCount;

    for (SignalId = FirstId; SignalId < (Com_SignalIdType)(FirstId + Count); SignalId++)
    {
        if ((SignalId < COM_NUM_OF_SIGNALS) && (Com_SignalRuntime[SignalId].ComSignalUpdated == TRUE))
        {
            AnyUpdated = TRUE;
            break;
        }
    }

    return (AnyUpdated == TRUE) ? E_OK : E_NOT_OK;
}

void Com_IpduGroupStart(Com_IpduGroupIdType IpduGroupId, boolean Initialize)
{
    PduIdType pduId;

    if ((Com_Initialized == FALSE) || (IpduGroupId >= COM_NUM_OF_IPDU_GROUPS))
    {
        return;
    }

    Com_IpduGroupRuntime[IpduGroupId].ComIpduGroupStarted = TRUE;

    if (Initialize == TRUE)
    {
        for (pduId = 0U; pduId < COM_NUM_OF_TX_IPDU; pduId++)
        {
            if (Com_TxIpduConfig[pduId].ComIpduGroupId == IpduGroupId)
            {
                Com_TxIpduRuntime[pduId].ComTxPending = FALSE;
                Com_TxIpduRuntime[pduId].ComTxConfirmed = TRUE;
                Com_TxIpduRuntime[pduId].ComTxDeadlineCounter = 0U;
                Com_TxIpduRuntime[pduId].ComTxDeadlineTimeout = FALSE;
                Com_TxIpduRuntime[pduId].ComTxLength = 0U;
                (void)memset(Com_TxIpduRuntime[pduId].ComTxBuffer, 0, COM_SIGNAL_MAX_LENGTH);
            }
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
    PduInfoType ComPduInfo;

    if ((Com_Initialized == FALSE) || (Com_IsValidTxPduId(TxPduId) == FALSE))
    {
        return E_NOT_OK;
    }
    if (Com_IsIpduGroupActiveForTxPdu(TxPduId) == FALSE)
    {
        return E_NOT_OK;
    }

    ComPduInfo.MetaDataPtr = NULL;
    ComPduInfo.SduDataPtr = Com_TxIpduRuntime[TxPduId].ComTxBuffer;
    ComPduInfo.SduLength = Com_TxIpduRuntime[TxPduId].ComTxLength;

    Com_TxIpduRuntime[TxPduId].ComTxConfirmed = FALSE;
    Com_TxIpduRuntime[TxPduId].ComTxPending = FALSE;
    Com_TxIpduRuntime[TxPduId].ComTxDeadlineCounter = 0U;
    Com_TxIpduRuntime[TxPduId].ComTxDeadlineTimeout = FALSE;

    if (PduR_ComTransmit(TxPduId, &ComPduInfo) != E_OK)
    {
        Com_TxIpduRuntime[TxPduId].ComTxPending = TRUE;
        return E_NOT_OK;
    }

    return E_OK;
}

void Com_TxConfirmation(PduIdType TxPduId, Std_ReturnType result)
{
#ifdef COM_DEBUG
    printf("######## in Com_TxConfirmation\n");
#endif
    if ((Com_Initialized == FALSE) || (Com_IsValidTxPduId(TxPduId) == FALSE))
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
    uint16 CopyLength;

#ifdef COM_DEBUG
    printf("######## in Com_RxIndication\n");
#endif

    if ((Com_Initialized == FALSE) || (PduInfoPtr == NULL) || (Com_IsValidRxPduId(RxPduId) == FALSE))
    {
        return;
    }
    if ((PduInfoPtr->SduDataPtr == NULL) || (PduInfoPtr->SduLength == 0U))
    {
        return;
    }
    if (Com_IsIpduGroupActiveForRxPdu(RxPduId) == FALSE)
    {
        return;
    }

    CopyLength = Com_MinLength((uint16)PduInfoPtr->SduLength, COM_SIGNAL_MAX_LENGTH);
    Com_RxIpduRuntime[RxPduId].ComRxLength = CopyLength;
    (void)memcpy(Com_RxIpduRuntime[RxPduId].ComRxBuffer, PduInfoPtr->SduDataPtr, CopyLength);
    Com_RxIpduRuntime[RxPduId].ComRxAvailable = TRUE;
    Com_RxIpduRuntime[RxPduId].ComRxTpInProgress = FALSE;
    Com_RxIpduRuntime[RxPduId].ComRxDeadlineCounter = 0U;
    Com_RxIpduRuntime[RxPduId].ComRxDeadlineTimeout = FALSE;

    Com_UpdateMappedRxSignals(RxPduId);
}

BufReq_ReturnType Com_StartOfReception(PduIdType ComRxPduId,
                                       const PduInfoType* PduInfoPtr,
                                       PduLengthType TpSduLength,
                                       PduLengthType* RxBufferSizePtr)
{
    if ((Com_Initialized == FALSE) || (RxBufferSizePtr == NULL) || (Com_IsValidRxPduId(ComRxPduId) == FALSE))
    {
        return BUFREQ_E_NOT_OK;
    }
    if (Com_IsIpduGroupActiveForRxPdu(ComRxPduId) == FALSE)
    {
        return BUFREQ_E_NOT_OK;
    }
    if (TpSduLength > COM_SIGNAL_MAX_LENGTH)
    {
        return BUFREQ_E_OVFL;
    }

    Com_RxIpduRuntime[ComRxPduId].ComRxLength = 0U;
    Com_RxIpduRuntime[ComRxPduId].ComRxAvailable = FALSE;
    Com_RxIpduRuntime[ComRxPduId].ComRxTpInProgress = TRUE;
    Com_RxIpduRuntime[ComRxPduId].ComRxDeadlineCounter = 0U;
    Com_RxIpduRuntime[ComRxPduId].ComRxDeadlineTimeout = FALSE;
    (void)memset(Com_RxIpduRuntime[ComRxPduId].ComRxBuffer, 0, COM_SIGNAL_MAX_LENGTH);
    *RxBufferSizePtr = COM_SIGNAL_MAX_LENGTH;

    if ((PduInfoPtr != NULL) && (PduInfoPtr->SduDataPtr != NULL) && (PduInfoPtr->SduLength > 0U))
    {
        (void)Com_CopyRxData(ComRxPduId, PduInfoPtr, RxBufferSizePtr);
    }

    return BUFREQ_OK;
}

BufReq_ReturnType Com_CopyRxData(PduIdType ComRxPduId,
                                 const PduInfoType* PduInfoPtr,
                                 PduLengthType* RxBufferSizePtr)
{
    uint16 RemainingLength;
    uint16 CopyLength;

    if ((Com_Initialized == FALSE) || (PduInfoPtr == NULL) || (RxBufferSizePtr == NULL))
    {
        return BUFREQ_E_NOT_OK;
    }
    if ((Com_IsValidRxPduId(ComRxPduId) == FALSE) || (PduInfoPtr->SduDataPtr == NULL))
    {
        return BUFREQ_E_NOT_OK;
    }
    if (Com_RxIpduRuntime[ComRxPduId].ComRxTpInProgress == FALSE)
    {
        return BUFREQ_E_NOT_OK;
    }

    RemainingLength = (uint16)(COM_SIGNAL_MAX_LENGTH - Com_RxIpduRuntime[ComRxPduId].ComRxLength);
    if ((uint16)PduInfoPtr->SduLength > RemainingLength)
    {
        return BUFREQ_E_OVFL;
    }

    CopyLength = (uint16)PduInfoPtr->SduLength;
    (void)memcpy(&Com_RxIpduRuntime[ComRxPduId].ComRxBuffer[Com_RxIpduRuntime[ComRxPduId].ComRxLength],
                 PduInfoPtr->SduDataPtr,
                 CopyLength);
    Com_RxIpduRuntime[ComRxPduId].ComRxLength += CopyLength;
    *RxBufferSizePtr = (PduLengthType)(COM_SIGNAL_MAX_LENGTH - Com_RxIpduRuntime[ComRxPduId].ComRxLength);

    return BUFREQ_OK;
}

void Com_TpRxIndication(PduIdType ComRxPduId, Std_ReturnType Result)
{
    PduInfoType TpPduInfo;

    if ((Com_Initialized == FALSE) || (Com_IsValidRxPduId(ComRxPduId) == FALSE))
    {
        return;
    }

    Com_RxIpduRuntime[ComRxPduId].ComRxTpInProgress = FALSE;
    if (Result != E_OK)
    {
        Com_RxIpduRuntime[ComRxPduId].ComRxLength = 0U;
        Com_RxIpduRuntime[ComRxPduId].ComRxAvailable = FALSE;
        return;
    }

    TpPduInfo.MetaDataPtr = NULL;
    TpPduInfo.SduDataPtr = Com_RxIpduRuntime[ComRxPduId].ComRxBuffer;
    TpPduInfo.SduLength = Com_RxIpduRuntime[ComRxPduId].ComRxLength;
    Com_RxIndication(ComRxPduId, &TpPduInfo);
}

void Com_TpTxConfirmation(PduIdType ComTxPduId, Std_ReturnType Result)
{
    Com_TxConfirmation(ComTxPduId, Result);
}

void Com_MainFunctionTx(void)
{
    PduIdType TxPduId;

    if (Com_Initialized == FALSE)
    {
        return;
    }

    for (TxPduId = 0U; TxPduId < COM_NUM_OF_TX_IPDU; TxPduId++)
    {
        if ((Com_TxIpduRuntime[TxPduId].ComTxPending == TRUE) &&
            (Com_IsIpduGroupActiveForTxPdu(TxPduId) == TRUE))
        {
            (void)Com_TriggerIPDUSend(TxPduId);
        }

        if (Com_TxIpduRuntime[TxPduId].ComTxConfirmed == FALSE)
        {
            if (Com_TxIpduRuntime[TxPduId].ComTxDeadlineCounter < 0xFFFFU)
            {
                Com_TxIpduRuntime[TxPduId].ComTxDeadlineCounter++;
            }
            if (Com_TxIpduRuntime[TxPduId].ComTxDeadlineCounter >
                Com_TxIpduRuntime[TxPduId].ComTxDeadlineLimit)
            {
                Com_TxIpduRuntime[TxPduId].ComTxDeadlineTimeout = TRUE;
                Com_TxIpduRuntime[TxPduId].ComTxPending = TRUE;
            }
        }
    }
}

void Com_MainFunctionRx(void)
{
    PduIdType RxPduId;

    if (Com_Initialized == FALSE)
    {
        return;
    }

    for (RxPduId = 0U; RxPduId < COM_NUM_OF_RX_IPDU; RxPduId++)
    {
        if (Com_RxIpduRuntime[RxPduId].ComRxAvailable == TRUE)
        {
            Com_RxIpduRuntime[RxPduId].ComRxAvailable = FALSE;
        }
        else if (Com_RxIpduRuntime[RxPduId].ComRxTpInProgress == FALSE)
        {
            if (Com_RxIpduRuntime[RxPduId].ComRxDeadlineCounter < 0xFFFFU)
            {
                Com_RxIpduRuntime[RxPduId].ComRxDeadlineCounter++;
            }
            if (Com_RxIpduRuntime[RxPduId].ComRxDeadlineCounter >
                Com_RxIpduRuntime[RxPduId].ComRxDeadlineLimit)
            {
                Com_RxIpduRuntime[RxPduId].ComRxDeadlineTimeout = TRUE;
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
