/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "Com/Com.h"
#include "PduR/PduR_Com.h"
#include "SecOC/SecOC_Debug.h"
#include "SecOC/SecOC_Lcfg.h"
#include <string.h>
#include <stdio.h>

/* MISRA C:2012 Rule 17.3 - Cross-module forward declarations */
extern Std_ReturnType PduR_ComTransmit(PduIdType PduID, const PduInfoType *PduInfo);

/********************************************************************************************************/
/************************************************Defines*************************************************/
/********************************************************************************************************/

#define COM_NUM_OF_TX_IPDU                    ((PduIdType)SECOC_NUM_OF_TX_PDU_PROCESSING)
#define COM_NUM_OF_RX_IPDU                    ((PduIdType)SECOC_NUM_OF_RX_PDU_PROCESSING)
#define COM_NUM_OF_SIGNALS                    ((Com_SignalIdType)SECOC_NUM_OF_TX_PDU_PROCESSING)
#define COM_SIGNAL_MAX_LENGTH                 ((uint16)SECOC_AUTHPDU_MAX_LENGTH)

/********************************************************************************************************/
/*******************************************TypeDefinitions**********************************************/
/********************************************************************************************************/

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

typedef struct
{
    boolean ComShadowUpdated;
    uint16 ComShadowLength;
    uint8 ComShadowData[COM_SIGNAL_MAX_LENGTH];
} Com_SignalGroupRuntimeType;

/********************************************************************************************************/
/******************************************GlobalVaribles************************************************/
/********************************************************************************************************/

static boolean Com_Initialized = FALSE;
static const Com_ConfigType* Com_ConfigPtr = NULL;

static Com_SignalRuntimeType Com_SignalRuntime[COM_NUM_OF_SIGNALS];
static Com_TxIpduRuntimeType Com_TxIpduRuntime[COM_NUM_OF_TX_IPDU];
static Com_RxIpduRuntimeType Com_RxIpduRuntime[COM_NUM_OF_RX_IPDU];
static Com_IpduGroupRuntimeType Com_IpduGroupRuntime[COM_NUM_OF_IPDU_GROUPS];
static Com_SignalGroupRuntimeType Com_SignalGroupRuntime[COM_NUM_OF_SIGNAL_GROUPS];

/********************************************************************************************************/
/**************************************ForwardDeclarations***********************************************/
/********************************************************************************************************/

extern void Com_Init(void);
extern void Com_InitWithConfig(const Com_ConfigType* ConfigPtr);
extern void Com_DeInit(void);
extern Std_ReturnType Com_SendSignal(Com_SignalIdType SignalId, const uint8* SignalDataPtr);
extern Std_ReturnType Com_ReceiveSignal(Com_SignalIdType SignalId, uint8* SignalDataPtr);
extern Std_ReturnType Com_SendDynSignal(Com_SignalIdType SignalId, const uint8* SignalDataPtr, uint16 Length);
extern Std_ReturnType Com_ReceiveDynSignal(Com_SignalIdType SignalId, uint8* SignalDataPtr, uint16* LengthPtr);
extern Std_ReturnType Com_InvalidateSignal(Com_SignalIdType SignalId);
extern Std_ReturnType Com_SendSignalGroup(Com_SignalGroupIdType SignalGroupId);
extern Std_ReturnType Com_ReceiveSignalGroup(Com_SignalGroupIdType SignalGroupId);
extern Std_ReturnType Com_UpdateShadowSignal(Com_SignalIdType SignalId, const uint8* SignalDataPtr);
extern Std_ReturnType Com_ReceiveShadowSignal(Com_SignalIdType SignalId, uint8* SignalDataPtr);
extern Std_ReturnType Com_InvalidateSignalGroup(Com_SignalGroupIdType SignalGroupId);
extern Std_ReturnType Com_SendSignalGroupArray(Com_SignalGroupIdType SignalGroupId,
                                               const uint8* SignalGroupArrayPtr,
                                               uint16 Length);
extern Std_ReturnType Com_ReceiveSignalGroupArray(Com_SignalGroupIdType SignalGroupId,
                                                  uint8* SignalGroupArrayPtr,
                                                  uint16* LengthPtr);
extern void Com_IpduGroupStart(Com_IpduGroupIdType IpduGroupId, boolean Initialize);
extern void Com_IpduGroupStop(Com_IpduGroupIdType IpduGroupId);
extern Std_ReturnType Com_TriggerIPDUSend(PduIdType TxPduId);
extern void Com_TxConfirmation(PduIdType TxPduId, Std_ReturnType result);
extern void Com_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);
extern BufReq_ReturnType Com_StartOfReception(PduIdType ComRxPduId,
                                              const PduInfoType* PduInfoPtr,
                                              PduLengthType TpSduLength,
                                              PduLengthType* RxBufferSizePtr);
extern BufReq_ReturnType Com_CopyRxData(PduIdType ComRxPduId,
                                        const PduInfoType* PduInfoPtr,
                                        PduLengthType* RxBufferSizePtr);
extern void Com_TpRxIndication(PduIdType ComRxPduId, Std_ReturnType Result);
extern void Com_TpTxConfirmation(PduIdType ComTxPduId, Std_ReturnType Result);
extern void Com_MainFunctionTx(void);
extern void Com_MainFunctionRx(void);
extern void Com_MainTx(void);

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
    if (Com_ConfigPtr == NULL)
    {
        return FALSE;
    }
    return (SignalId < Com_ConfigPtr->ComNumOfSignals) ? TRUE : FALSE;
}

static boolean Com_IsValidSignalGroupId(Com_SignalGroupIdType SignalGroupId)
{
    if (Com_ConfigPtr == NULL)
    {
        return FALSE;
    }
    return (SignalGroupId < Com_ConfigPtr->ComNumOfSignalGroups) ? TRUE : FALSE;
}

static boolean Com_IsValidTxPduId(PduIdType TxPduId)
{
    if (Com_ConfigPtr == NULL)
    {
        return FALSE;
    }
    return (TxPduId < Com_ConfigPtr->ComNumOfTxIpdu) ? TRUE : FALSE;
}

static boolean Com_IsValidRxPduId(PduIdType RxPduId)
{
    if (Com_ConfigPtr == NULL)
    {
        return FALSE;
    }
    return (RxPduId < Com_ConfigPtr->ComNumOfRxIpdu) ? TRUE : FALSE;
}

static boolean Com_IsSignalGroupActive(Com_SignalGroupIdType SignalGroupId)
{
    Com_IpduGroupIdType GroupId;
    if (Com_IsValidSignalGroupId(SignalGroupId) == FALSE)
    {
        return FALSE;
    }
    GroupId = (Com_IpduGroupIdType)Com_ConfigPtr->ComSignalGroupConfigPtr[SignalGroupId].ComIpduGroupId;
    if (GroupId >= COM_NUM_OF_IPDU_GROUPS)
    {
        return FALSE;
    }
    return Com_IpduGroupRuntime[GroupId].ComIpduGroupStarted;
}

static boolean Com_FindSignalGroupForSignal(Com_SignalIdType SignalId,
                                            Com_SignalGroupIdType* SignalGroupIdPtr,
                                            uint16* OffsetPtr)
{
    Com_SignalGroupIdType SignalGroupId;
    Com_SignalIdType FirstId;
    uint16 Count;

    if ((SignalGroupIdPtr == NULL) || (OffsetPtr == NULL))
    {
        return FALSE;
    }
    if (Com_IsValidSignalId(SignalId) == FALSE)
    {
        return FALSE;
    }

    for (SignalGroupId = 0U; SignalGroupId < Com_ConfigPtr->ComNumOfSignalGroups; SignalGroupId++)
    {
        FirstId = (Com_SignalIdType)Com_ConfigPtr->ComSignalGroupConfigPtr[SignalGroupId].ComFirstSignalId;
        Count = Com_ConfigPtr->ComSignalGroupConfigPtr[SignalGroupId].ComSignalCount;
        if ((SignalId >= FirstId) && (SignalId < (Com_SignalIdType)(FirstId + Count)))
        {
            *SignalGroupIdPtr = SignalGroupId;
            *OffsetPtr = (uint16)(SignalId - FirstId);
            return TRUE;
        }
    }

    return FALSE;
}

static uint16 Com_GetSignalGroupArrayCapacity(Com_SignalGroupIdType SignalGroupId)
{
    return Com_MinLength(Com_ConfigPtr->ComSignalGroupConfigPtr[SignalGroupId].ComSignalGroupArrayLength,
                         COM_SIGNAL_MAX_LENGTH);
}

static boolean Com_GetSignalGroupSignalLayout(Com_SignalGroupIdType SignalGroupId,
                                              Com_SignalIdType SignalId,
                                              uint16* RelativePositionPtr,
                                              uint16* SignalLengthPtr)
{
    Com_SignalIdType FirstId;
    uint16 Count;
    uint16 GroupArrayLength;
    uint16 GroupMinPosition = 0xFFFFU;
    Com_SignalIdType MemberId;
    uint16 SignalPosition;
    uint16 SignalLength;

    if ((RelativePositionPtr == NULL) || (SignalLengthPtr == NULL))
    {
        return FALSE;
    }
    if ((Com_IsValidSignalGroupId(SignalGroupId) == FALSE) || (Com_IsValidSignalId(SignalId) == FALSE))
    {
        return FALSE;
    }

    FirstId = (Com_SignalIdType)Com_ConfigPtr->ComSignalGroupConfigPtr[SignalGroupId].ComFirstSignalId;
    Count = Com_ConfigPtr->ComSignalGroupConfigPtr[SignalGroupId].ComSignalCount;
    if ((SignalId < FirstId) || (SignalId >= (Com_SignalIdType)(FirstId + Count)))
    {
        return FALSE;
    }

    GroupArrayLength = Com_GetSignalGroupArrayCapacity(SignalGroupId);
    if (GroupArrayLength == 0U)
    {
        return FALSE;
    }

    for (MemberId = FirstId; MemberId < (Com_SignalIdType)(FirstId + Count); MemberId++)
    {
        if (Com_IsValidSignalId(MemberId) == FALSE)
        {
            continue;
        }
        if (Com_ConfigPtr->ComSignalConfigPtr[MemberId].ComSignalPosition < GroupMinPosition)
        {
            GroupMinPosition = Com_ConfigPtr->ComSignalConfigPtr[MemberId].ComSignalPosition;
        }
    }
    if (GroupMinPosition == 0xFFFFU)
    {
        return FALSE;
    }

    SignalPosition = Com_ConfigPtr->ComSignalConfigPtr[SignalId].ComSignalPosition;
    SignalLength = Com_MinLength(Com_ConfigPtr->ComSignalConfigPtr[SignalId].ComSignalLength,
                                 Com_ConfigPtr->ComSignalConfigPtr[SignalId].ComSignalMaxLength);
    if ((SignalLength == 0U) || (SignalPosition < GroupMinPosition))
    {
        return FALSE;
    }

    *RelativePositionPtr = (uint16)(SignalPosition - GroupMinPosition);
    if ((uint32)(*RelativePositionPtr) + (uint32)SignalLength > (uint32)GroupArrayLength)
    {
        return FALSE;
    }
    *SignalLengthPtr = SignalLength;
    return TRUE;
}

static uint16 Com_GetSignalGroupUsedLength(Com_SignalGroupIdType SignalGroupId)
{
    Com_SignalIdType FirstId;
    uint16 Count;
    Com_SignalIdType SignalId;
    uint16 RelativePosition;
    uint16 SignalLength;
    uint16 UsedLength = 0U;

    if (Com_IsValidSignalGroupId(SignalGroupId) == FALSE)
    {
        return 0U;
    }

    FirstId = (Com_SignalIdType)Com_ConfigPtr->ComSignalGroupConfigPtr[SignalGroupId].ComFirstSignalId;
    Count = Com_ConfigPtr->ComSignalGroupConfigPtr[SignalGroupId].ComSignalCount;

    for (SignalId = FirstId; SignalId < (Com_SignalIdType)(FirstId + Count); SignalId++)
    {
        if (Com_GetSignalGroupSignalLayout(SignalGroupId, SignalId, &RelativePosition, &SignalLength) == TRUE)
        {
            uint16 EndPosition = (uint16)(RelativePosition + SignalLength);
            if (UsedLength < EndPosition)
            {
                UsedLength = EndPosition;
            }
        }
    }

    return UsedLength;
}

static boolean Com_IsConfigValid(const Com_ConfigType* ConfigPtr)
{
    if (ConfigPtr == NULL)
    {
        return FALSE;
    }
    if ((ConfigPtr->ComSignalConfigPtr == NULL) ||
        (ConfigPtr->ComSignalGroupConfigPtr == NULL) ||
        (ConfigPtr->ComTxIpduConfigPtr == NULL) ||
        (ConfigPtr->ComRxIpduConfigPtr == NULL))
    {
        return FALSE;
    }
    if ((ConfigPtr->ComNumOfSignals > COM_NUM_OF_SIGNALS) ||
        (ConfigPtr->ComNumOfSignalGroups > COM_NUM_OF_SIGNAL_GROUPS) ||
        (ConfigPtr->ComNumOfTxIpdu > COM_NUM_OF_TX_IPDU) ||
        (ConfigPtr->ComNumOfRxIpdu > COM_NUM_OF_RX_IPDU))
    {
        return FALSE;
    }
    return TRUE;
}

static void Com_ResetRuntime(boolean IsInit)
{
    PduIdType pduId;
    Com_SignalIdType signalId;
    Com_SignalGroupIdType signalGroupId;
    (void)IsInit;

    for (signalId = 0U; signalId < Com_ConfigPtr->ComNumOfSignals; signalId++)
    {
        Com_SignalRuntime[signalId].ComSignalUpdated = FALSE;
        Com_SignalRuntime[signalId].ComSignalInvalid = FALSE;
        Com_SignalRuntime[signalId].ComSignalLength = Com_ConfigPtr->ComSignalConfigPtr[signalId].ComSignalLength;
        Com_SignalRuntime[signalId].ComSignalLastDeliveredValue = 0U;
        (void)memset(Com_SignalRuntime[signalId].ComSignalValue, 0, COM_SIGNAL_MAX_LENGTH);
    }

    for (pduId = 0U; pduId < Com_ConfigPtr->ComNumOfTxIpdu; pduId++)
    {
        Com_TxIpduRuntime[pduId].ComTxPending = FALSE;
        Com_TxIpduRuntime[pduId].ComTxConfirmed = TRUE;
        Com_TxIpduRuntime[pduId].ComTxDeadlineTimeout = FALSE;
        Com_TxIpduRuntime[pduId].ComTxDeadlineCounter = 0U;
        Com_TxIpduRuntime[pduId].ComTxDeadlineLimit = Com_ConfigPtr->ComTxIpduConfigPtr[pduId].ComTxDeadlineLimit;
        Com_TxIpduRuntime[pduId].ComTxLength = 0U;
        (void)memset(Com_TxIpduRuntime[pduId].ComTxBuffer, 0, COM_SIGNAL_MAX_LENGTH);
    }

    for (pduId = 0U; pduId < Com_ConfigPtr->ComNumOfRxIpdu; pduId++)
    {
        Com_RxIpduRuntime[pduId].ComRxAvailable = FALSE;
        Com_RxIpduRuntime[pduId].ComRxTpInProgress = FALSE;
        Com_RxIpduRuntime[pduId].ComRxDeadlineTimeout = FALSE;
        Com_RxIpduRuntime[pduId].ComRxDeadlineCounter = 0U;
        Com_RxIpduRuntime[pduId].ComRxDeadlineLimit = Com_ConfigPtr->ComRxIpduConfigPtr[pduId].ComRxDeadlineLimit;
        Com_RxIpduRuntime[pduId].ComRxLength = 0U;
        (void)memset(Com_RxIpduRuntime[pduId].ComRxBuffer, 0, COM_SIGNAL_MAX_LENGTH);
    }

    for (pduId = 0U; pduId < COM_NUM_OF_IPDU_GROUPS; pduId++)
    {
        Com_IpduGroupRuntime[pduId].ComIpduGroupStarted = FALSE;
    }

    for (signalGroupId = 0U; signalGroupId < Com_ConfigPtr->ComNumOfSignalGroups; signalGroupId++)
    {
        Com_SignalGroupRuntime[signalGroupId].ComShadowUpdated = FALSE;
        Com_SignalGroupRuntime[signalGroupId].ComShadowLength = 0U;
        (void)memset(Com_SignalGroupRuntime[signalGroupId].ComShadowData, 0, COM_SIGNAL_MAX_LENGTH);
    }
}

static boolean Com_IsIpduGroupActiveForTxPdu(PduIdType TxPduId)
{
    Com_IpduGroupIdType GroupId;
    if (Com_IsValidTxPduId(TxPduId) == FALSE)
    {
        return FALSE;
    }
    GroupId = (Com_IpduGroupIdType)Com_ConfigPtr->ComTxIpduConfigPtr[TxPduId].ComIpduGroupId;
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
    GroupId = (Com_IpduGroupIdType)Com_ConfigPtr->ComRxIpduConfigPtr[RxPduId].ComIpduGroupId;
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

    TxPduId = Com_ConfigPtr->ComSignalConfigPtr[SignalId].ComTxPduId;
    if (Com_IsValidTxPduId(TxPduId) == FALSE)
    {
        return E_NOT_OK;
    }

    SignalPosition = Com_ConfigPtr->ComSignalConfigPtr[SignalId].ComSignalPosition;
    if (SignalPosition >= COM_SIGNAL_MAX_LENGTH)
    {
        return E_NOT_OK;
    }

    CopyLength = Com_MinLength(SignalLength, Com_ConfigPtr->ComSignalConfigPtr[SignalId].ComSignalMaxLength);
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
        if (Com_ConfigPtr->ComSignalConfigPtr[SignalId].ComRxPduId != RxPduId)
        {
            continue;
        }

        SignalPosition = Com_ConfigPtr->ComSignalConfigPtr[SignalId].ComSignalPosition;
        if (SignalPosition >= Com_RxIpduRuntime[RxPduId].ComRxLength)
        {
            continue;
        }

        CopyLength = Com_MinLength(Com_ConfigPtr->ComSignalConfigPtr[SignalId].ComSignalLength,
                                   (uint16)(Com_RxIpduRuntime[RxPduId].ComRxLength - SignalPosition));
        if (CopyLength == 0U)
        {
            continue;
        }

        RxFilteredValue = (uint8)(Com_RxIpduRuntime[RxPduId].ComRxBuffer[SignalPosition] &
                                  Com_ConfigPtr->ComSignalConfigPtr[SignalId].ComSignalFilterMask);
        if (RxFilteredValue != (uint8)(Com_SignalRuntime[SignalId].ComSignalLastDeliveredValue &
                                       Com_ConfigPtr->ComSignalConfigPtr[SignalId].ComSignalFilterMask))
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
    if (Com_IsConfigValid(&Com_Config) == FALSE)
    {
        Com_ConfigPtr = NULL;
        Com_Initialized = FALSE;
        return;
    }

    Com_ConfigPtr = &Com_Config;
    Com_ResetRuntime(TRUE);
    Com_Initialized = TRUE;
}

void Com_InitWithConfig(const Com_ConfigType* ConfigPtr)
{
    if (ConfigPtr == NULL)
    {
        Com_Init();
        return;
    }
    if (Com_IsConfigValid(ConfigPtr) == FALSE)
    {
        Com_ConfigPtr = NULL;
        Com_Initialized = FALSE;
        return;
    }
    Com_ConfigPtr = ConfigPtr;
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
                             Com_ConfigPtr->ComSignalConfigPtr[SignalId].ComSignalLength : 0U);
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
    if ((Length == 0U) || (Length > Com_ConfigPtr->ComSignalConfigPtr[SignalId].ComSignalMaxLength))
    {
        return E_NOT_OK;
    }

    CopyLength = Com_MinLength(Length, Com_ConfigPtr->ComSignalConfigPtr[SignalId].ComSignalMaxLength);
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

    SignalLength = Com_ConfigPtr->ComSignalConfigPtr[SignalId].ComSignalLength;
    for (Index = 0U; Index < SignalLength; Index++)
    {
        Com_SignalRuntime[SignalId].ComSignalValue[Index] = Com_ConfigPtr->ComSignalConfigPtr[SignalId].ComInvalidValue;
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

    if ((Com_Initialized == FALSE) || (Com_IsValidSignalGroupId(SignalGroupId) == FALSE))
    {
        return E_NOT_OK;
    }

    if (Com_IsSignalGroupActive(SignalGroupId) == FALSE)
    {
        return E_NOT_OK;
    }

    FirstId = (Com_SignalIdType)Com_ConfigPtr->ComSignalGroupConfigPtr[SignalGroupId].ComFirstSignalId;
    Count = Com_ConfigPtr->ComSignalGroupConfigPtr[SignalGroupId].ComSignalCount;

    if (Com_SignalGroupRuntime[SignalGroupId].ComShadowUpdated == TRUE)
    {
        for (SignalId = FirstId; SignalId < (Com_SignalIdType)(FirstId + Count); SignalId++)
        {
            uint16 RelativePosition;
            uint16 SignalLength;
            if (Com_GetSignalGroupSignalLayout(SignalGroupId, SignalId, &RelativePosition, &SignalLength) == FALSE)
            {
                continue;
            }
            if ((uint32)RelativePosition + (uint32)SignalLength >
                (uint32)Com_SignalGroupRuntime[SignalGroupId].ComShadowLength)
            {
                continue;
            }

            (void)memcpy(Com_SignalRuntime[SignalId].ComSignalValue,
                         &Com_SignalGroupRuntime[SignalGroupId].ComShadowData[RelativePosition],
                         SignalLength);
            Com_SignalRuntime[SignalId].ComSignalLength = SignalLength;
            Com_SignalRuntime[SignalId].ComSignalUpdated = TRUE;
            Com_SignalRuntime[SignalId].ComSignalInvalid = FALSE;
        }
        Com_SignalGroupRuntime[SignalGroupId].ComShadowUpdated = FALSE;
    }

    for (SignalId = FirstId; SignalId < (Com_SignalIdType)(FirstId + Count); SignalId++)
    {
        if (SignalId < Com_ConfigPtr->ComNumOfSignals)
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

    if ((Com_Initialized == FALSE) || (Com_IsValidSignalGroupId(SignalGroupId) == FALSE))
    {
        return E_NOT_OK;
    }

    if (Com_IsSignalGroupActive(SignalGroupId) == FALSE)
    {
        return E_NOT_OK;
    }

    FirstId = (Com_SignalIdType)Com_ConfigPtr->ComSignalGroupConfigPtr[SignalGroupId].ComFirstSignalId;
    Count = Com_ConfigPtr->ComSignalGroupConfigPtr[SignalGroupId].ComSignalCount;

    for (SignalId = FirstId; SignalId < (Com_SignalIdType)(FirstId + Count); SignalId++)
    {
        if ((SignalId < Com_ConfigPtr->ComNumOfSignals) && (Com_SignalRuntime[SignalId].ComSignalUpdated == TRUE))
        {
            AnyUpdated = TRUE;
        }
    }

    if (AnyUpdated == TRUE)
    {
        uint16 GroupLength = Com_GetSignalGroupUsedLength(SignalGroupId);
        (void)memset(Com_SignalGroupRuntime[SignalGroupId].ComShadowData, 0, GroupLength);

        for (SignalId = FirstId; SignalId < (Com_SignalIdType)(FirstId + Count); SignalId++)
        {
            uint16 RelativePosition;
            uint16 SignalLength;
            uint16 RuntimeLength;
            uint16 CopyLength;

            if (Com_GetSignalGroupSignalLayout(SignalGroupId, SignalId, &RelativePosition, &SignalLength) == FALSE)
            {
                continue;
            }
            RuntimeLength = Com_SignalRuntime[SignalId].ComSignalLength;
            CopyLength = Com_MinLength(SignalLength, RuntimeLength);
            if (CopyLength > 0U)
            {
                (void)memcpy(&Com_SignalGroupRuntime[SignalGroupId].ComShadowData[RelativePosition],
                             Com_SignalRuntime[SignalId].ComSignalValue,
                             CopyLength);
            }
        }
        Com_SignalGroupRuntime[SignalGroupId].ComShadowLength = GroupLength;
        Com_SignalGroupRuntime[SignalGroupId].ComShadowUpdated = TRUE;
    }

    return (AnyUpdated == TRUE) ? E_OK : E_NOT_OK;
}

Std_ReturnType Com_UpdateShadowSignal(Com_SignalIdType SignalId, const uint8* SignalDataPtr)
{
    Com_SignalGroupIdType SignalGroupId;
    uint16 Offset;
    uint16 SignalLength;

    if ((Com_Initialized == FALSE) || (SignalDataPtr == NULL) || (Com_IsValidSignalId(SignalId) == FALSE))
    {
        return E_NOT_OK;
    }

    if (Com_FindSignalGroupForSignal(SignalId, &SignalGroupId, &Offset) == FALSE)
    {
        return E_NOT_OK;
    }

    if (Com_GetSignalGroupSignalLayout(SignalGroupId, SignalId, &Offset, &SignalLength) == FALSE)
    {
        return E_NOT_OK;
    }

    (void)memcpy(&Com_SignalGroupRuntime[SignalGroupId].ComShadowData[Offset], SignalDataPtr, SignalLength);
    if (Com_SignalGroupRuntime[SignalGroupId].ComShadowLength < (uint16)(Offset + SignalLength))
    {
        Com_SignalGroupRuntime[SignalGroupId].ComShadowLength = (uint16)(Offset + SignalLength);
    }
    Com_SignalGroupRuntime[SignalGroupId].ComShadowUpdated = TRUE;

    return E_OK;
}

Std_ReturnType Com_ReceiveShadowSignal(Com_SignalIdType SignalId, uint8* SignalDataPtr)
{
    Com_SignalGroupIdType SignalGroupId;
    uint16 Offset;
    uint16 SignalLength;

    if ((Com_Initialized == FALSE) || (SignalDataPtr == NULL) || (Com_IsValidSignalId(SignalId) == FALSE))
    {
        return E_NOT_OK;
    }

    if (Com_FindSignalGroupForSignal(SignalId, &SignalGroupId, &Offset) == FALSE)
    {
        return E_NOT_OK;
    }

    if (Com_GetSignalGroupSignalLayout(SignalGroupId, SignalId, &Offset, &SignalLength) == FALSE)
    {
        return E_NOT_OK;
    }

    if ((Com_SignalGroupRuntime[SignalGroupId].ComShadowUpdated == FALSE) ||
        (SignalLength == 0U) ||
        ((uint32)Offset + (uint32)SignalLength > (uint32)Com_SignalGroupRuntime[SignalGroupId].ComShadowLength))
    {
        return E_NOT_OK;
    }

    (void)memcpy(SignalDataPtr, &Com_SignalGroupRuntime[SignalGroupId].ComShadowData[Offset], SignalLength);
    return E_OK;
}

Std_ReturnType Com_InvalidateSignalGroup(Com_SignalGroupIdType SignalGroupId)
{
    Com_SignalIdType SignalId;
    Com_SignalIdType FirstId;
    uint16 Count;
    uint16 Offset;
    uint16 SignalLength;
    uint16 FillIndex;

    if ((Com_Initialized == FALSE) || (Com_IsValidSignalGroupId(SignalGroupId) == FALSE))
    {
        return E_NOT_OK;
    }

    FirstId = (Com_SignalIdType)Com_ConfigPtr->ComSignalGroupConfigPtr[SignalGroupId].ComFirstSignalId;
    Count = Com_ConfigPtr->ComSignalGroupConfigPtr[SignalGroupId].ComSignalCount;
    Com_SignalGroupRuntime[SignalGroupId].ComShadowLength = 0U;

    for (SignalId = FirstId; SignalId < (Com_SignalIdType)(FirstId + Count); SignalId++)
    {
        if (SignalId < Com_ConfigPtr->ComNumOfSignals)
        {
            (void)Com_InvalidateSignal(SignalId);
            if (Com_GetSignalGroupSignalLayout(SignalGroupId, SignalId, &Offset, &SignalLength) == TRUE)
            {
                for (FillIndex = 0U; FillIndex < SignalLength; FillIndex++)
                {
                    Com_SignalGroupRuntime[SignalGroupId].ComShadowData[Offset + FillIndex] =
                        Com_ConfigPtr->ComSignalConfigPtr[SignalId].ComInvalidValue;
                }
                if (Com_SignalGroupRuntime[SignalGroupId].ComShadowLength < (uint16)(Offset + SignalLength))
                {
                    Com_SignalGroupRuntime[SignalGroupId].ComShadowLength = (uint16)(Offset + SignalLength);
                }
            }
        }
    }

    Com_SignalGroupRuntime[SignalGroupId].ComShadowUpdated = TRUE;
    return E_OK;
}

Std_ReturnType Com_SendSignalGroupArray(Com_SignalGroupIdType SignalGroupId,
                                        const uint8* SignalGroupArrayPtr,
                                        uint16 Length)
{
    uint16 GroupLength;

    if ((Com_Initialized == FALSE) || (SignalGroupArrayPtr == NULL) ||
        (Com_IsValidSignalGroupId(SignalGroupId) == FALSE))
    {
        return E_NOT_OK;
    }
    if (Com_IsSignalGroupActive(SignalGroupId) == FALSE)
    {
        return E_NOT_OK;
    }

    GroupLength = Com_ConfigPtr->ComSignalGroupConfigPtr[SignalGroupId].ComSignalGroupArrayLength;
    GroupLength = Com_MinLength(GroupLength, COM_SIGNAL_MAX_LENGTH);
    if ((Length == 0U) || (Length > GroupLength))
    {
        return E_NOT_OK;
    }

    (void)memcpy(Com_SignalGroupRuntime[SignalGroupId].ComShadowData, SignalGroupArrayPtr, Length);
    Com_SignalGroupRuntime[SignalGroupId].ComShadowLength = Length;
    Com_SignalGroupRuntime[SignalGroupId].ComShadowUpdated = TRUE;

    return Com_SendSignalGroup(SignalGroupId);
}

Std_ReturnType Com_ReceiveSignalGroupArray(Com_SignalGroupIdType SignalGroupId,
                                           uint8* SignalGroupArrayPtr,
                                           uint16* LengthPtr)
{
    uint16 MaxCopy;

    if ((Com_Initialized == FALSE) || (SignalGroupArrayPtr == NULL) || (LengthPtr == NULL) ||
        (Com_IsValidSignalGroupId(SignalGroupId) == FALSE))
    {
        return E_NOT_OK;
    }
    if (Com_IsSignalGroupActive(SignalGroupId) == FALSE)
    {
        return E_NOT_OK;
    }

    if ((Com_SignalGroupRuntime[SignalGroupId].ComShadowUpdated == FALSE) && (Com_ReceiveSignalGroup(SignalGroupId) != E_OK))
    {
        return E_NOT_OK;
    }

    MaxCopy = Com_MinLength(*LengthPtr, Com_SignalGroupRuntime[SignalGroupId].ComShadowLength);
    if (MaxCopy > 0U)
    {
        (void)memcpy(SignalGroupArrayPtr, Com_SignalGroupRuntime[SignalGroupId].ComShadowData, MaxCopy);
    }

    *LengthPtr = MaxCopy;
    return E_OK;
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
            if (Com_ConfigPtr->ComTxIpduConfigPtr[pduId].ComIpduGroupId == IpduGroupId)
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
    (void)printf("######## in Com_TxConfirmation\n");
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
    (void)printf("######## in Com_RxIndication\n");
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
        else
        {
            /* No action required */
        }
    }
}

void Com_MainTx(void)
{
#ifdef COM_DEBUG
    (void)printf("######## in Com_MainTx\n");
#endif
    Com_MainFunctionTx();
}
