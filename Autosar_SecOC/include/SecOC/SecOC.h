#ifndef INCLUDE_SECOC_H_
#define INCLUDE_SECOC_H_

/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "SecOC_Cfg.h"
#include "SecOC_Types.h"
#include "SecOC_Lcfg.h"
#include "SchM_SecOC.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/********************************************************************************************************/
/************************************************Defines*************************************************/
/********************************************************************************************************/

#define SECOC_E_UNINIT 					0x02

  /* [SWS_SecOC_00054] SECOC_MODULE_ID = 150, SECOC_SID_INIT = 0x01, SECOC_E_PARAM_POINTER = 0x01 */

#define SECOC_MODULE_ID                 150
#define SECOC_SID_INIT                  0x01
#define SECOC_E_PARAM_POINTER           0x01

/********************************************************************************************************/
/*****************************************FunctionPrototype**********************************************/
/********************************************************************************************************/


/********************************************************************************************************/
/******************************************Initialization************************************************/
/********************************************************************************************************/

/*******************************************************
 *          * Function Info *                           *
 *                                                      *
 * Function_Name        : SecOC_Init                    *
 * Function_Index       : 8.3.1  [SWS_SecOC_00106]      *
 * Function_File        : SWS of SecOC                  *
 * Function_Descripton  : Initializes the the SecOC     *
 * module. Successful initialization leads to state     *
 * SecOC_INIT. In configurations, in which SecOC is     *
 * assigned to more than one partition                  *
 *******************************************************/

void SecOC_Init(const SecOC_ConfigType *config);


/*******************************************************
 *          * Function Info *                           *
 *                                                      *
 * Function_Name        : SecOC_DeInit                  *
 * Function_Index       : 8.3.2  [SWS_SecOC_00161]      *
 * Function_File        : SWS of SecOC                  *
 * Function_Descripton  : This service stops the secure *
 * onboard communication. All buffered I-PDU are removed*
 *  and have to be obtained again, if needed, after     *
 * SecOC_Init has been called. By a call to SecOC_DeInit*
 *  the AUTOSAR SecOC module is put into a not          *
 * initialized state (SecOC_UNINIT).                    *
 *******************************************************/

void SecOC_DeInit (void);



/********************************************************************************************************/
/********************************************Transmission************************************************/
/**********************************The creation of a Secured I-PDU***************************************/
/********************************************************************************************************/


/********************************************************
 *          * Function Info *                           *
 *                                                      *
 * Function_Name        : SecOC_IfTransmit              *
 * Function_Index       : 8.3.4 [SWS_SecOC_00112]       *
 * Function_File        : SWS of SecOC                  *
 * Function_Descripton  : Requests transmission of a    *
 * PDU.                                                 *
 *******************************************************/

Std_ReturnType SecOC_IfTransmit(
    PduIdType                  TxPduId,
    const PduInfoType*         PduInfoPtr
);


/********************************************************
 *          * Function Info *                           *
 *                                                      *
 * Function_Name        : SecOC_GetTxFreshness          *
 * Function_Index       : 8.5.3 [SWS_SecOC_00126]       *
 * Function_File        : SWS of SecOC                  *
 * Function_Descripton  : This API returns the freshness*
 * value from the Most Significant Bits in the first    *
 * byte in the array (SecOCFreshnessValue),             *
 * in big endian format.                                *
 *******************************************************/

Std_ReturnType SecOC_GetTxFreshness(uint16 SecOCFreshnessValueID, uint8* SecOCFreshnessValue,
uint32* SecOCFreshnessValueLength);


/********************************************************
 *          * Function Info *                           *
 *                                                      *
 * Function_Name        : SecOC_GetTxFreshnessTruncData *
 * Function_Index       : 8.5.4 [SWS_SecOC_91003]       *
 * Function_File        : SWS of SecOC                  *
 * Function_Descripton  : This interface is used by the *
 * SecOC to obtain the current freshness value          *
 * The interface function provides also the truncated   *
 * freshness transmitted in the secured I-PDU.          *
 *******************************************************/

Std_ReturnType SecOC_GetTxFreshnessTruncData(
    uint16 SecOCFreshnessValueID,
    uint8* SecOCFreshnessValue,
    uint32* SecOCFreshnessValueLength,
    uint8* SecOCTruncatedFreshnessValue,
    uint32* SecOCTruncatedFreshnessValueLength
);


/********************************************************
 *          * Function Info *                           *
 *                                                      *
 * Function_Name        : SecOC_CopyTxData              *
 * Function_Index       : 8.4.7 [SWS_SecOC_00129]       *
 * Function_File        : SWS of SecOC                  *
 * Function_Descripton  : This function is called to    *
 * acquire the transmit data of an I-PDU segment (N-PDU)*
 * Each call to this function provides the next part of *
 * the I-PDU data unless retry->Tp DataState is         *
 * TP_DATARETRY. In this case the function restarts to  *
 * copy the data beginning at the offset from the       *
 * current position indicated by retry->TxTpDataCnt.    *
 * The size of the remaining data is written to the     *
 * position indicated by availableDataPtr.              *
 *******************************************************/

BufReq_ReturnType SecOC_CopyTxData (
    PduIdType id,
    const PduInfoType* info,
    const RetryInfoType* retry,
    PduLengthType* availableDataPtr
);


/********************************************************
 *          * Function Info *                           *
 *                                                      *
 * Function_Name        : SecOC_TxConfirmation          *
 * Function_Index       : 8.4.3  [SWS_SecOC_00126]      *
 * Function_File        : SWS of SecOC                  *
 * Function_Descripton  : The lower layer communication *
 * interface module confirms the transmission of a PDU, *
 * or the failure to transmit a PDU.                    *
 *******************************************************/

void SecOC_TxConfirmation(PduIdType TxPduId, Std_ReturnType result);


/********************************************************
 *          * Function Info *                           *
 *                                                      *
 * Function_Name        : SecOC_TpTxConfirmation        *
 * Function_Index       : 8.4.4                         *
 * Function_File        : SWS of SecOC                  *
 * Function_Descripton  : This function is called after *
 * the I-PDU has been transmitted on its network, the   *
 * result indicates whether the transmission was        *
 * successful or not.                                   *
 *******************************************************/

void SecOC_TpTxConfirmation(PduIdType id,Std_ReturnType result);


/********************************************************
 *          * Function Info *                           *
 *                                                      *
 * Function_Name        : SecOC_IfCancelTransmit        *
 * Function_Index       : 8.3.6                         *
 * Function_File        : SWS of SecOC                  *
 * Function_Descripton  : Requests cancellation of an   *
 * ongoing transmission of a PDU in a lower layer       *
 * communication module.                                *
 *******************************************************/

Std_ReturnType SecOC_IfCancelTransmit(
    PduIdType                  TxPduId
);


/********************************************************************************************************/
/*********************************************Reception**************************************************/
/********************************The verification of a Secured I-PDU*************************************/
/********************************************************************************************************/

/********************************************************
 *          * Function Info *                           *
 *                                                      *
 * Function_Name        : SecOC_RxIndication            *
 * Function_Index       : 8.4.1 [SWS_SecOC_00124]       *
 * Function_File        : SWS of secOC                  *
 * Function_Descripton  : Indication of a received PDU  *
 * from a lower layer communication interface module.   *
 *******************************************************/

void SecOC_RxIndication (PduIdType RxPduId, const PduInfoType* PduInfoPtr);


/********************************************************
 *          * Function Info *                           *
 *                                                      *
 * Function_Name        : SecOC_StartOfReception        *
 * Function_Index       : 8.4.8                         *
 * Function_File        : SWS of SecOC                  *
 * Function_Descripton  : This function is called at    *
 * the start of receiving an N-SDU. The N-SDU might be  *
 * fragmented into multiple N-PDUs                      *
 * (FF with one or more following CFs) or might consist *
 * of a single N-PDU (SF).                              *
 *******************************************************/

BufReq_ReturnType SecOC_StartOfReception ( 
    PduIdType id, 
    const PduInfoType* info, 
    PduLengthType TpSduLength, 
    PduLengthType* bufferSizePtr );


/********************************************************
 *          * Function Info *                           *
 *                                                      *
 * Function_Name        : SecOC_CopyRxData              *
 * Function_Index       : 8.4.6                         *
 * Function_File        : SWS of SecOC                  *
 * Function_Descripton  : This function is called to    *
 * provide the received data of an I-PDU segment        *
 * (N-PDU) to the upper layer. Each call to this        *
 * function provides the next part of the I-PDU data.   *
 * The size of the remaining buffer is written to the   *
 * position indicated by bufferSizePtr.                 *
 *******************************************************/

BufReq_ReturnType SecOC_CopyRxData (PduIdType id, const PduInfoType* info, PduLengthType* bufferSizePtr);


/********************************************************
 *          * Function Info *                           *
 *                                                      *
 * Function_Name        : SecOC_TpRxIndication          *
 * Function_Index       : 8.4.2                         *
 * Function_File        : SWS of SecOC                  *
 * Function_Descripton  : This function is Called after *
 * an I-PDU has been received via the TP API,           *
 * the result indicates whether                         *
 * the transmission was successful or not.              *
 *******************************************************/

void SecOC_TpRxIndication( PduIdType id, Std_ReturnType result );



/********************************************************
 *          * Function Info *                           *
 *                                                      *
 * Function_Name        : SecOC_GetRxFreshness          *
 * Function_Index       : 8.5.1 [SWS_SecOC_91007]       *
 * Function_File        : SWS of SecOC                  *
 * Function_Descripton  : This interface is used by the *
 * SecOC to obtain the current freshness value          *
 *******************************************************/

Std_ReturnType SecOC_GetRxFreshness(uint16 SecOCFreshnessValueID, const uint8* SecOCTruncatedFreshnessValue,
uint32 SecOCTruncatedFreshnessValueLength, uint16 SecOCAuthVerifyAttempts, uint8* SecOCFreshnessValue,
uint32* SecOCFreshnessValueLength);


/********************************************************
 *          * Function Info *                           *
 *                                                      *
 * Function_Name        : SecOC_TpTransmit              *
 * Function_Index       : 8.3.5 [SWS_SecOC_00113]       *
 * Function_File        : SWS of SecOC                  *
 * Function_Descripton  : Requests transmission of a    *
 * PDU via Transport Protocol.                          *
 *******************************************************/

Std_ReturnType SecOC_TpTransmit(
    PduIdType                  TxPduId,
    const PduInfoType*         PduInfoPtr
);


/********************************************************
 *          * Function Info *                           *
 *                                                      *
 * Function_Name        : SecOC_TpCancelTransmit        *
 * Function_Index       : 8.3.7 [SWS_SecOC_00119]       *
 * Function_File        : SWS of SecOC                  *
 * Function_Descripton  : Requests cancellation of an   *
 * ongoing TP transmission of a PDU.                    *
 *******************************************************/

Std_ReturnType SecOC_TpCancelTransmit(
    PduIdType                  TxPduId
);


/********************************************************
 *          * Function Info *                           *
 *                                                      *
 * Function_Name        : SecOC_GetVersionInfo          *
 * Function_Index       : 8.3.3 [SWS_SecOC_00107]       *
 * Function_File        : SWS of SecOC                  *
 * Function_Descripton  : Returns the version           *
 * information of the SecOC module.                     *
 *******************************************************/

#if (SECOC_VERSION_INFO_API == STD_ON)
void SecOC_GetVersionInfo(
    Std_VersionInfoType*       versioninfo
);
#endif


/********************************************************
 *          * Function Info *                           *
 *                                                      *
 * Function_Name        : SecOC_VerifyStatusOverride    *
 * Function_Index       : 8.3.8 [SWS_SecOC_91008]       *
 * Function_File        : SWS of SecOC                  *
 * Function_Descripton  : This interface is used by     *
 * the upper layer to override the verification status  *
 * for a specific Freshness Value ID.                   *
 *******************************************************/

Std_ReturnType SecOC_VerifyStatusOverride(
    uint16                     SecOCFreshnessValueID,
    uint8                      overrideStatus,
    uint8                      numberOfMessagesToOverride
);


#endif /* INCLUDE_SECOC_H_*/
