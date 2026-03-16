#ifndef INCLUDE_DCM_H_
#define INCLUDE_DCM_H_

/********************************************************************************************************/
/************************************************INCLUDES***********************************************/
/********************************************************************************************************/

#include "Std_Types.h"
#include "ComStack_Types.h"

/********************************************************************************************************/
/************************************************Defines*************************************************/
/********************************************************************************************************/

#define DCM_MODULE_ID                ((uint16)53U)
#define DCM_INSTANCE_ID              ((uint8)0U)

/* Service IDs */
#define DCM_SID_INIT                 ((uint8)0x01U)
#define DCM_SID_MAIN_FUNCTION        ((uint8)0x02U)
#define DCM_SID_PROCESS_REQUEST      ((uint8)0x03U)
#define DCM_SID_TP_TX_CONFIRMATION   ((uint8)0x04U)
#define DCM_SID_DEINIT               ((uint8)0x05U)
#define DCM_SID_TP_START_OF_RECEPTION ((uint8)0x06U)
#define DCM_SID_TP_COPY_RX_DATA      ((uint8)0x07U)
#define DCM_SID_TP_RX_INDICATION     ((uint8)0x08U)

/* DET Error Codes */
#define DCM_E_UNINIT                 ((uint8)0x01U)
#define DCM_E_PARAM_POINTER          ((uint8)0x02U)

/* UDS Service IDs */
#define DCM_UDS_SID_CLEAR_DTC              ((uint8)0x14U)
#define DCM_UDS_SID_READ_DTC_INFORMATION   ((uint8)0x19U)

/* UDS Sub-functions for 0x19 */
#define DCM_UDS_SUBFUNC_REPORT_NUMBER_OF_DTC_BY_STATUS     ((uint8)0x01U)
#define DCM_UDS_SUBFUNC_REPORT_DTC_BY_STATUS_MASK          ((uint8)0x02U)

/* Response buffer size */
#define DCM_RESPONSE_BUFFER_SIZE     (256U)

/* Negative response codes */
#define DCM_NRC_POSITIVE_RESPONSE              ((uint8)0x00U)
#define DCM_NRC_SERVICE_NOT_SUPPORTED          ((uint8)0x11U)
#define DCM_NRC_SUB_FUNCTION_NOT_SUPPORTED     ((uint8)0x12U)
#define DCM_NRC_CONDITIONS_NOT_CORRECT         ((uint8)0x22U)

/********************************************************************************************************/
/*******************************************StructAndEnums***********************************************/
/********************************************************************************************************/

typedef struct
{
    uint8 dummy;
} Dcm_ConfigType;

/********************************************************************************************************/
/*****************************************FunctionPrototype**********************************************/
/********************************************************************************************************/

void Dcm_Init(const Dcm_ConfigType* ConfigPtr);
void Dcm_DeInit(void);
void Dcm_MainFunction(void);

/**
 * @brief Process a UDS diagnostic request and produce a response.
 * @param[in]  RequestData   Pointer to UDS request bytes.
 * @param[in]  RequestLength Length of the request.
 * @param[out] ResponseData  Pointer to buffer for UDS response.
 * @param[out] ResponseLength Pointer to store response length.
 * @return E_OK on success, E_NOT_OK on failure.
 */
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

#endif /* INCLUDE_DCM_H_ */
