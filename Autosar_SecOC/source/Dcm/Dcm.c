/********************************************************************************************************/
/************************************************INCLUDES***********************************************/
/********************************************************************************************************/

#include "Dcm/Dcm.h"
#include "Dem/Dem.h"
#include "Det/Det.h"
#include <stdio.h>
#include <string.h>

/* MISRA C:2012 Rule 17.3 - Cross-module forward declarations */
extern Std_ReturnType Dem_ClearDTC(uint32 DTC);
extern Std_ReturnType Dem_GetNextFilteredDTC(uint32* DTC, uint8* DTCStatus);

/* External API declarations (MISRA 8.4 visibility). */
void Dcm_Init(const Dcm_ConfigType* ConfigPtr);
void Dcm_DeInit(void);
void Dcm_MainFunction(void);
Std_ReturnType Dcm_ProcessRequest(const uint8* RequestData, uint16 RequestLength,
                                  uint8* ResponseData, uint16* ResponseLength);
void Dcm_TpTxConfirmation(PduIdType TxPduId, Std_ReturnType result);
BufReq_ReturnType Dcm_TpStartOfReception(PduIdType RxPduId,
                                         const PduInfoType* PduInfoPtr,
                                         PduLengthType TpSduLength,
                                         PduLengthType* RxBufferSizePtr);
BufReq_ReturnType Dcm_TpCopyRxData(PduIdType RxPduId,
                                   const PduInfoType* PduInfoPtr,
                                   PduLengthType* RxBufferSizePtr);
void Dcm_TpRxIndication(PduIdType RxPduId, Std_ReturnType result);

/********************************************************************************************************/
/******************************************GlobalVariables**********************************************/
/********************************************************************************************************/

static uint8 Dcm_Initialized = FALSE;
static uint8 Dcm_RxTpBuffer[DCM_RESPONSE_BUFFER_SIZE];
static uint16 Dcm_RxTpLength = 0U;
static uint8 Dcm_TxTpBuffer[DCM_RESPONSE_BUFFER_SIZE];
static uint16 Dcm_TxTpLength = 0U;

/********************************************************************************************************/
/********************************************Functions***************************************************/
/********************************************************************************************************/

void Dcm_Init(const Dcm_ConfigType* ConfigPtr)
{
    (void)ConfigPtr;
    Dcm_Initialized = TRUE;
    (void)printf("[DCM] Initialized\n");
}

void Dcm_MainFunction(void)
{
    if (Dcm_Initialized == FALSE)
    {
        return;
    }
}

void Dcm_DeInit(void)
{
    Dcm_Initialized = FALSE;
    Dcm_RxTpLength = 0U;
    Dcm_TxTpLength = 0U;
}

Std_ReturnType Dcm_ProcessRequest(const uint8* RequestData, uint16 RequestLength,
                                  uint8* ResponseData, uint16* ResponseLength)
{
    uint8 serviceId;
    uint8 subFunction;

    if (Dcm_Initialized == FALSE)
    {
        (void)Det_ReportError(DCM_MODULE_ID, DCM_INSTANCE_ID, DCM_SID_PROCESS_REQUEST, DCM_E_UNINIT);
        return E_NOT_OK;
    }

    if ((RequestData == NULL) || (ResponseData == NULL) || (ResponseLength == NULL))
    {
        (void)Det_ReportError(DCM_MODULE_ID, DCM_INSTANCE_ID, DCM_SID_PROCESS_REQUEST, DCM_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    if (RequestLength < 1U)
    {
        return E_NOT_OK;
    }

    serviceId = RequestData[0];

    switch (serviceId)
    {
        case DCM_UDS_SID_CLEAR_DTC:
        {
            /* UDS 0x14 ClearDiagnosticInformation */
            /* Request: [0x14] [DTC_HighByte] [DTC_MidByte] [DTC_LowByte] */
            uint32 dtcValue;

            if (RequestLength < 4U)
            {
                ResponseData[0] = 0x7FU;
                ResponseData[1] = serviceId;
                ResponseData[2] = DCM_NRC_CONDITIONS_NOT_CORRECT;
                *ResponseLength = 3U;
                return E_OK;
            }

            dtcValue = ((uint32)RequestData[1] << 16U)
                     | ((uint32)RequestData[2] << 8U)
                     | ((uint32)RequestData[3]);

            if (Dem_ClearDTC(dtcValue) == E_OK)
            {
                ResponseData[0] = (uint8)(serviceId + 0x40U);
                *ResponseLength = 1U;
            }
            else
            {
                ResponseData[0] = 0x7FU;
                ResponseData[1] = serviceId;
                ResponseData[2] = DCM_NRC_CONDITIONS_NOT_CORRECT;
                *ResponseLength = 3U;
            }
            break;
        }

        case DCM_UDS_SID_READ_DTC_INFORMATION:
        {
            /* UDS 0x19 ReadDTCInformation */
            if (RequestLength < 2U)
            {
                ResponseData[0] = 0x7FU;
                ResponseData[1] = serviceId;
                ResponseData[2] = DCM_NRC_CONDITIONS_NOT_CORRECT;
                *ResponseLength = 3U;
                return E_OK;
            }

            subFunction = RequestData[1];

            if (subFunction == DCM_UDS_SUBFUNC_REPORT_NUMBER_OF_DTC_BY_STATUS)
            {
                /* Sub-function 0x01: reportNumberOfDTCByStatusMask */
                uint8 statusMask;
                uint16 numDtcs = 0U;

                if (RequestLength < 3U)
                {
                    ResponseData[0] = 0x7FU;
                    ResponseData[1] = serviceId;
                    ResponseData[2] = DCM_NRC_CONDITIONS_NOT_CORRECT;
                    *ResponseLength = 3U;
                    return E_OK;
                }

                statusMask = RequestData[2];
                (void)Dem_GetNumberOfFilteredDTC(statusMask, &numDtcs);

                ResponseData[0] = (uint8)(serviceId + 0x40U);
                ResponseData[1] = subFunction;
                ResponseData[2] = statusMask;      /* StatusAvailabilityMask */
                ResponseData[3] = 0x00U;            /* DTCFormatIdentifier (ISO 14229) */
                ResponseData[4] = (uint8)(numDtcs >> 8U);
                ResponseData[5] = (uint8)(numDtcs & 0xFFU);
                *ResponseLength = 6U;
            }
            else if (subFunction == DCM_UDS_SUBFUNC_REPORT_DTC_BY_STATUS_MASK)
            {
                /* Sub-function 0x02: reportDTCByStatusMask */
                uint8 statusMask;
                uint16 offset;
                uint32 dtcVal;
                uint8 dtcStatus;

                if (RequestLength < 3U)
                {
                    ResponseData[0] = 0x7FU;
                    ResponseData[1] = serviceId;
                    ResponseData[2] = DCM_NRC_CONDITIONS_NOT_CORRECT;
                    *ResponseLength = 3U;
                    return E_OK;
                }

                statusMask = RequestData[2];
                (void)Dem_SetDTCFilter(statusMask);

                ResponseData[0] = (uint8)(serviceId + 0x40U);
                ResponseData[1] = subFunction;
                ResponseData[2] = statusMask;
                offset = 3U;

                while ((offset + 4U) <= DCM_RESPONSE_BUFFER_SIZE)
                {
                    if (Dem_GetNextFilteredDTC(&dtcVal, &dtcStatus) != E_OK)
                    {
                        break;
                    }
                    ResponseData[offset]      = (uint8)(dtcVal >> 16U);
                    ResponseData[offset + 1U] = (uint8)(dtcVal >> 8U);
                    ResponseData[offset + 2U] = (uint8)(dtcVal & 0xFFU);
                    ResponseData[offset + 3U] = dtcStatus;
                    offset += 4U;
                }

                *ResponseLength = offset;
            }
            else
            {
                ResponseData[0] = 0x7FU;
                ResponseData[1] = serviceId;
                ResponseData[2] = DCM_NRC_SUB_FUNCTION_NOT_SUPPORTED;
                *ResponseLength = 3U;
            }
            break;
        }

        default:
        {
            /* Service not supported */
            ResponseData[0] = 0x7FU;
            ResponseData[1] = serviceId;
            ResponseData[2] = DCM_NRC_SERVICE_NOT_SUPPORTED;
            *ResponseLength = 3U;
            break;
        }
    }

    return E_OK;
}

void Dcm_TpTxConfirmation(PduIdType TxPduId, Std_ReturnType result)
{
    (void)TxPduId;

    #ifdef DCM_DEBUG
        printf("######## in Dcm_TpTxConfirmation\n");
    #endif

    if (result == E_OK)
    {
        #ifdef DCM_DEBUG
        printf("DCM received data with status E_OK\n");
        #endif
    }
    else
    {
        #ifdef DCM_DEBUG
        printf("DCM received data with status E_NOT_OK\n");
        #endif
    }
}

BufReq_ReturnType Dcm_TpStartOfReception(PduIdType RxPduId,
                                         const PduInfoType* PduInfoPtr,
                                         PduLengthType TpSduLength,
                                         PduLengthType* RxBufferSizePtr)
{
    (void)RxPduId;
    (void)PduInfoPtr;

    if (Dcm_Initialized == FALSE)
    {
        (void)Det_ReportError(DCM_MODULE_ID, DCM_INSTANCE_ID, DCM_SID_TP_START_OF_RECEPTION, DCM_E_UNINIT);
        return BUFREQ_E_NOT_OK;
    }
    if (RxBufferSizePtr == NULL)
    {
        (void)Det_ReportError(DCM_MODULE_ID, DCM_INSTANCE_ID, DCM_SID_TP_START_OF_RECEPTION, DCM_E_PARAM_POINTER);
        return BUFREQ_E_NOT_OK;
    }
    if (TpSduLength > DCM_RESPONSE_BUFFER_SIZE)
    {
        *RxBufferSizePtr = DCM_RESPONSE_BUFFER_SIZE;
        return BUFREQ_E_OVFL;
    }

    Dcm_RxTpLength = 0U;
    *RxBufferSizePtr = (PduLengthType)DCM_RESPONSE_BUFFER_SIZE;
    return BUFREQ_OK;
}

BufReq_ReturnType Dcm_TpCopyRxData(PduIdType RxPduId,
                                   const PduInfoType* PduInfoPtr,
                                   PduLengthType* RxBufferSizePtr)
{
    uint16 copyLength;
    uint16 idx;
    (void)RxPduId;

    if (Dcm_Initialized == FALSE)
    {
        (void)Det_ReportError(DCM_MODULE_ID, DCM_INSTANCE_ID, DCM_SID_TP_COPY_RX_DATA, DCM_E_UNINIT);
        return BUFREQ_E_NOT_OK;
    }
    if ((PduInfoPtr == NULL) || (RxBufferSizePtr == NULL))
    {
        (void)Det_ReportError(DCM_MODULE_ID, DCM_INSTANCE_ID, DCM_SID_TP_COPY_RX_DATA, DCM_E_PARAM_POINTER);
        return BUFREQ_E_NOT_OK;
    }
    if ((PduInfoPtr->SduLength > 0U) && (PduInfoPtr->SduDataPtr == NULL))
    {
        return BUFREQ_E_NOT_OK;
    }
    if ((uint32)Dcm_RxTpLength + (uint32)PduInfoPtr->SduLength > (uint32)DCM_RESPONSE_BUFFER_SIZE)
    {
        *RxBufferSizePtr = 0U;
        return BUFREQ_E_OVFL;
    }

    copyLength = (uint16)PduInfoPtr->SduLength;
    for (idx = 0U; idx < copyLength; idx++)
    {
        Dcm_RxTpBuffer[Dcm_RxTpLength + idx] = PduInfoPtr->SduDataPtr[idx];
    }
    Dcm_RxTpLength = (uint16)(Dcm_RxTpLength + copyLength);
    *RxBufferSizePtr = (PduLengthType)((uint16)DCM_RESPONSE_BUFFER_SIZE - Dcm_RxTpLength);

    return BUFREQ_OK;
}

void Dcm_TpRxIndication(PduIdType RxPduId, Std_ReturnType result)
{
    (void)RxPduId;

    if ((Dcm_Initialized == FALSE) || (result != E_OK))
    {
        Dcm_RxTpLength = 0U;
        Dcm_TxTpLength = 0U;
        return;
    }

    if (Dcm_ProcessRequest(Dcm_RxTpBuffer, Dcm_RxTpLength, Dcm_TxTpBuffer, &Dcm_TxTpLength) != E_OK)
    {
        Dcm_TxTpLength = 0U;
    }

    Dcm_RxTpLength = 0U;
}
