/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "Can/Can.h"
#include "Can/CanIF.h"
#include "Can/CanTP.h"
#include "CanNm/CanNm.h"
#include "Det/Det.h"
#include "PduR/PduR_CanIf.h"
#include "SecOC/SecOC.h"
#include "SecOC/SecOC_Debug.h"
#include "SecOC/SecOC_Lcfg.h"

/* MISRA C:2012 Rule 17.3 - Cross-module forward declarations */
extern Std_ReturnType Can_GetControllerMode(uint8 Controller, Can_ControllerStateType *ControllerModePtr);

/********************************************************************************************************/
/******************************************GlobalVaribles************************************************/
/********************************************************************************************************/

static boolean CanIf_Initialized = FALSE;
static CanIf_ControllerModeType CanIf_ControllerModes[CAN_MAX_CONTROLLERS];
static CanIf_PduModeType CanIf_PduModes[CAN_MAX_CONTROLLERS];

static void CanIf_InternalTxConfirmation(PduIdType TxPduId, Std_ReturnType result)
{
    if (TxPduId == CANNM_TX_PDU_ID)
    {
        CanNm_TxConfirmation(TxPduId, result);
    }

    if (TxPduId >= (PduIdType)SECOC_NUM_OF_TX_PDU_PROCESSING)
    {
        return;
    }

    switch (PdusCollections[TxPduId].Type)
    {
    case SECOC_SECURED_PDU_CANIF:
    case SECOC_AUTH_COLLECTON_PDU:
    case SECOC_CRYPTO_COLLECTON_PDU:
        PduR_CanIfTxConfirmation(TxPduId, result);
        break;
    case SECOC_SECURED_PDU_CANTP:
        CanTp_TxConfirmation(TxPduId, result);
        break;
    default:
        break;
    }
}

/********************************************************************************************************/
/********************************************Functions***************************************************/
/********************************************************************************************************/

/* External API declarations (MISRA 8.4 visibility). */
void CanIf_Init(void);
Std_ReturnType CanIf_SetControllerMode(uint8 ControllerId, CanIf_ControllerModeType ControllerMode);
Std_ReturnType CanIf_Transmit(PduIdType TxPduId, const PduInfoType *PduInfoPtr);
void CanIf_TxConfirmation(PduIdType TxPduId, Std_ReturnType result);
void CanIf_RxIndication(PduIdType RxPduId, const PduInfoType *PduInfoPtr);
void CanIf_ControllerModeIndication(uint8 ControllerId, CanIf_ControllerModeType ControllerMode);
Std_ReturnType CanIf_GetControllerMode(uint8 ControllerId, CanIf_ControllerModeType *ControllerModePtr);
Std_ReturnType CanIf_SetPduMode(uint8 ControllerId, CanIf_PduModeType PduModeRequest);
Std_ReturnType CanIf_GetPduMode(uint8 ControllerId, CanIf_PduModeType *PduModePtr);

void CanIf_Init(void)
{
    Can_ControllerStateType controllerMode;

    if (CanIf_Initialized == TRUE)
    {
        return;
    }

    if (Can_GetControllerMode(0U, &controllerMode) != E_OK)
    {
        Can_Init(NULL);
        (void)Can_SetControllerMode(0U, CAN_CS_STARTED);
        CanIf_ControllerModes[0U] = CANIF_CS_STARTED;
    }
    else
    {
        switch (controllerMode)
        {
        case CAN_CS_STARTED:
            CanIf_ControllerModes[0U] = CANIF_CS_STARTED;
            break;
        case CAN_CS_SLEEP:
            CanIf_ControllerModes[0U] = CANIF_CS_SLEEP;
            break;
        case CAN_CS_STOPPED:
            CanIf_ControllerModes[0U] = CANIF_CS_STOPPED;
            break;
        default:
            CanIf_ControllerModes[0U] = CANIF_CS_UNINIT;
            break;
        }
    }

    for (uint8 i = 0; i < CAN_MAX_CONTROLLERS; i++)
    {
        CanIf_PduModes[i] = CANIF_ONLINE;
    }

    Can_RegisterTxConfirmation(CanIf_InternalTxConfirmation);
    Can_RegisterRxIndication(CanIf_RxIndication);
    CanIf_Initialized = TRUE;
}

Std_ReturnType CanIf_SetControllerMode(uint8 ControllerId, CanIf_ControllerModeType ControllerMode)
{
    Std_ReturnType RetVal = E_OK;
    Can_ControllerStateType CanMode = CAN_CS_STOPPED;

    if (CanIf_Initialized == FALSE)
    {
        (void)Det_ReportError(CANIF_MODULE_ID, CANIF_INSTANCE_ID, CANIF_SID_SET_CONTROLLER_MODE, CANIF_E_UNINIT);
        RetVal = E_NOT_OK;
    }
    else if (ControllerId >= CAN_MAX_CONTROLLERS)
    {
        (void)Det_ReportError(CANIF_MODULE_ID, CANIF_INSTANCE_ID, CANIF_SID_SET_CONTROLLER_MODE, CANIF_E_PARAM_CONTROLLER);
        RetVal = E_NOT_OK;
    }
    else
    {
        /* Map CanIf mode to Can driver mode and request transition */
        switch (ControllerMode)
        {
            case CANIF_CS_STARTED:
                CanMode = CAN_CS_STARTED;
                break;
            case CANIF_CS_STOPPED:
                CanMode = CAN_CS_STOPPED;
                break;
            case CANIF_CS_SLEEP:
                CanMode = CAN_CS_SLEEP;
                break;
            default:
                RetVal = E_NOT_OK;
                break;
        }
    }

    if (RetVal == E_OK)
    {
        RetVal = Can_SetControllerMode(ControllerId, CanMode);
        if (RetVal == E_OK)
        {
            CanIf_ControllerModes[ControllerId] = ControllerMode;
        }
    }

    return RetVal;
}

Std_ReturnType CanIf_Transmit(PduIdType TxPduId, const PduInfoType *PduInfoPtr)
{
    #ifdef CANIF_DEBUG
        printf("######## in CanIf_Transmit \n");
    #endif

    Std_ReturnType result;
    Can_PduType canPdu;

    if (CanIf_Initialized == FALSE)
    {
        (void)Det_ReportError(CANIF_MODULE_ID, CANIF_INSTANCE_ID, CANIF_SID_TRANSMIT, CANIF_E_UNINIT);
        /* Backward compatibility: auto-init */
        CanIf_Init();
    }

    if (PduInfoPtr == NULL)
    {
        (void)Det_ReportError(CANIF_MODULE_ID, CANIF_INSTANCE_ID, CANIF_SID_TRANSMIT, CANIF_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    if ((PduInfoPtr->SduLength > 0) && (PduInfoPtr->SduDataPtr == NULL))
    {
        (void)Det_ReportError(CANIF_MODULE_ID, CANIF_INSTANCE_ID, CANIF_SID_TRANSMIT, CANIF_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    /* Check PDU channel mode allows Tx */
    if ((CanIf_PduModes[0U] == CANIF_OFFLINE) || (CanIf_PduModes[0U] == CANIF_TX_OFFLINE))
    {
        return E_NOT_OK;
    }

    #ifdef CANIF_DEBUG
        printf("Secure PDU -->\n");
            for(int i = 0; i < PduInfoPtr->SduLength; i++)
            {
                printf("%d ", PduInfoPtr->SduDataPtr[i]);
            }
        printf("\n");
    #endif

    canPdu.id = TxPduId;
    canPdu.length = (uint8)MIN(PduInfoPtr->SduLength, (PduLengthType)255U);
    canPdu.sdu = PduInfoPtr->SduDataPtr;
    canPdu.swPduHandle = TxPduId;

    result = Can_Write(0, &canPdu);
    if (result == E_OK)
    {
        /* Polling mode simulation: process queued transmissions immediately. */
        Can_MainFunction_Write();
    }

    return result;
}

void CanIf_TxConfirmation(PduIdType TxPduId, Std_ReturnType result)
{
    if (CanIf_Initialized == FALSE)
    {
        (void)Det_ReportError(CANIF_MODULE_ID, CANIF_INSTANCE_ID, CANIF_SID_TX_CONFIRMATION, CANIF_E_UNINIT);
        return;
    }

    /* Forward to upper layer via internal callback */
    CanIf_InternalTxConfirmation(TxPduId, result);
}

void CanIf_RxIndication(PduIdType RxPduId, const PduInfoType *PduInfoPtr)
{
    if (CanIf_Initialized == FALSE)
    {
        (void)Det_ReportError(CANIF_MODULE_ID, CANIF_INSTANCE_ID, CANIF_SID_RX_INDICATION, CANIF_E_UNINIT);
        return;
    }

    if (PduInfoPtr == NULL)
    {
        (void)Det_ReportError(CANIF_MODULE_ID, CANIF_INSTANCE_ID, CANIF_SID_RX_INDICATION, CANIF_E_PARAM_POINTER);
        return;
    }

    /* Check PDU channel mode allows Rx */
    if (CanIf_PduModes[0U] == CANIF_OFFLINE)
    {
        return;
    }

    if (RxPduId == CANNM_RX_PDU_ID)
    {
        CanNm_RxIndication(RxPduId, PduInfoPtr);
    }

    if (RxPduId >= (PduIdType)SECOC_NUM_OF_RX_PDU_PROCESSING)
    {
        return;
    }

    switch (PdusCollections[RxPduId].Type)
    {
    case SECOC_SECURED_PDU_CANTP:
        CanTp_RxIndication(RxPduId, PduInfoPtr);
        break;
    case SECOC_SECURED_PDU_CANIF:
    case SECOC_AUTH_COLLECTON_PDU:
    case SECOC_CRYPTO_COLLECTON_PDU:
        PduR_CanIfRxIndication(RxPduId, PduInfoPtr);
        break;
    default:
        /* Keep legacy behavior for non-CAN routed types. */
        PduR_CanIfRxIndication(RxPduId, PduInfoPtr);
        break;
    }
}

void CanIf_ControllerModeIndication(uint8 ControllerId, CanIf_ControllerModeType ControllerMode)
{
    if (ControllerId >= CAN_MAX_CONTROLLERS)
    {
        (void)Det_ReportError(CANIF_MODULE_ID, CANIF_INSTANCE_ID, CANIF_SID_SET_CONTROLLER_MODE, CANIF_E_PARAM_CONTROLLER);
        return;
    }

    CanIf_ControllerModes[ControllerId] = ControllerMode;
}

Std_ReturnType CanIf_GetControllerMode(uint8 ControllerId, CanIf_ControllerModeType *ControllerModePtr)
{
    if (CanIf_Initialized == FALSE)
    {
        (void)Det_ReportError(CANIF_MODULE_ID, CANIF_INSTANCE_ID, CANIF_SID_GET_CONTROLLER_MODE, CANIF_E_UNINIT);
        return E_NOT_OK;
    }

    if (ControllerId >= CAN_MAX_CONTROLLERS)
    {
        (void)Det_ReportError(CANIF_MODULE_ID, CANIF_INSTANCE_ID, CANIF_SID_GET_CONTROLLER_MODE, CANIF_E_PARAM_CONTROLLER);
        return E_NOT_OK;
    }

    if (ControllerModePtr == NULL)
    {
        (void)Det_ReportError(CANIF_MODULE_ID, CANIF_INSTANCE_ID, CANIF_SID_GET_CONTROLLER_MODE, CANIF_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    *ControllerModePtr = CanIf_ControllerModes[ControllerId];
    return E_OK;
}

Std_ReturnType CanIf_SetPduMode(uint8 ControllerId, CanIf_PduModeType PduModeRequest)
{
    if (CanIf_Initialized == FALSE)
    {
        (void)Det_ReportError(CANIF_MODULE_ID, CANIF_INSTANCE_ID, CANIF_SID_SET_PDU_MODE, CANIF_E_UNINIT);
        return E_NOT_OK;
    }

    if (ControllerId >= CAN_MAX_CONTROLLERS)
    {
        (void)Det_ReportError(CANIF_MODULE_ID, CANIF_INSTANCE_ID, CANIF_SID_SET_PDU_MODE, CANIF_E_PARAM_CONTROLLER);
        return E_NOT_OK;
    }

    CanIf_PduModes[ControllerId] = PduModeRequest;
    return E_OK;
}

Std_ReturnType CanIf_GetPduMode(uint8 ControllerId, CanIf_PduModeType *PduModePtr)
{
    if (CanIf_Initialized == FALSE)
    {
        (void)Det_ReportError(CANIF_MODULE_ID, CANIF_INSTANCE_ID, CANIF_SID_GET_PDU_MODE, CANIF_E_UNINIT);
        return E_NOT_OK;
    }

    if (ControllerId >= CAN_MAX_CONTROLLERS)
    {
        (void)Det_ReportError(CANIF_MODULE_ID, CANIF_INSTANCE_ID, CANIF_SID_GET_PDU_MODE, CANIF_E_PARAM_CONTROLLER);
        return E_NOT_OK;
    }

    if (PduModePtr == NULL)
    {
        (void)Det_ReportError(CANIF_MODULE_ID, CANIF_INSTANCE_ID, CANIF_SID_GET_PDU_MODE, CANIF_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    *PduModePtr = CanIf_PduModes[ControllerId];
    return E_OK;
}
