/* "Copyright [2022/2023] <Tanta University>" */

/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "SecOC_Lcfg.h"
#include "SecOC_Cfg.h"
#include "SecOC_PBcfg.h"
#include "SecOC_PQC_Cfg.h"
#include "SecOC_Cbk.h"
#include "ComStack_Types.h"
#include "Rte_SecOC.h"
#include "SecOC.h"
#include "PduR_SecOC.h"
#include "Csm.h"
#include "Rte_SecOC_Type.h"
#include "PduR_Com.h"
#include "Pdur_CanTP.h"
#include "PduR_CanIf.h"
#include "CanTP.h"
#include "SecOC_Debug.h"
#include "Det.h"
#include "BswM.h"
#include "PQC.h"

#include <string.h>

#define SECOC_BSWM_STATUS_OK              ((BswM_ModeType)0U)
#define SECOC_BSWM_STATUS_FAILURE         ((BswM_ModeType)1U)



/********************************************************************************************************/
/******************************************GlobalVaribles************************************************/
/********************************************************************************************************/

#ifdef RELEASE
    #define STATIC static
#else
    #define STATIC
#endif

/* cppcheck-suppress misra-c2012-8.4 */
const SecOC_TxPduProcessingType     *SecOCTxPduProcessing;
/* cppcheck-suppress misra-c2012-8.4 */
const SecOC_RxPduProcessingType     *SecOCRxPduProcessing;
/* cppcheck-suppress misra-c2012-8.4 */
const SecOC_GeneralType             *SecOCGeneral;

static SecOC_StateType SecOCState = SECOC_UNINIT;
static PduLengthType bufferRemainIndex[SECOC_NUM_OF_TX_PDU_PROCESSING] = {0};
STATIC PduLengthType authRecieveLength[SECOC_NUM_OF_RX_PDU_PROCESSING] = {0};

static boolean SecOC_IsValidTxPduId(PduIdType TxPduId)
{
    return (TxPduId < (PduIdType)SECOC_NUM_OF_TX_PDU_PROCESSING) ? TRUE : FALSE;
}

static boolean SecOC_IsValidRxPduId(PduIdType RxPduId)
{
    return (RxPduId < (PduIdType)SECOC_NUM_OF_RX_PDU_PROCESSING) ? TRUE : FALSE;
}

static boolean SecOC_IsValidCollectionPduId(PduIdType PduId)
{
    return (PduId < (PduIdType)SECOC_NUM_OF_PDU_COLLECTION) ? TRUE : FALSE;
}

static uint32 SecOC_GetTxCsmJobId(PduIdType TxPduId)
{
    if ((SecOC_IsValidTxPduId(TxPduId) == TRUE) &&
        (SecOCTxPduProcessing[TxPduId].SecOCTxAuthServiceConfigRef != NULL) &&
        (SecOCTxPduProcessing[TxPduId].SecOCTxAuthServiceConfigRef->CsmJob != NULL))
    {
        return SecOCTxPduProcessing[TxPduId].SecOCTxAuthServiceConfigRef->CsmJob->CsmJobId;
    }

    return (uint32)CSM_JOBID;
}

static uint32 SecOC_GetRxCsmJobId(PduIdType RxPduId)
{
    if ((SecOC_IsValidRxPduId(RxPduId) == TRUE) &&
        (SecOCRxPduProcessing[RxPduId].SecOCRxAuthServiceConfigRef != NULL) &&
        (SecOCRxPduProcessing[RxPduId].SecOCRxAuthServiceConfigRef->CsmJob != NULL))
    {
        return SecOCRxPduProcessing[RxPduId].SecOCRxAuthServiceConfigRef->CsmJob->CsmJobId;
    }

    return (uint32)CSM_JOBID;
}



/********************************************************************************************************/
/************************************InternalFunctionsPrototype******************************************/
/********************************************************************************************************/

static Std_ReturnType prepareFreshnessTx(const PduIdType TxPduId, SecOC_TxIntermediateType *SecOCIntermediate);
static void constructDataToAuthenticatorTx(const PduIdType TxPduId, SecOC_TxIntermediateType *SecOCIntermediate, const PduInfoType* AuthPdu);
STATIC Std_ReturnType authenticate(const PduIdType TxPduId, PduInfoType* AuthPdu, PduInfoType* SecPdu);
STATIC Std_ReturnType authenticate_PQC(const PduIdType TxPduId, PduInfoType* AuthPdu, PduInfoType* SecPdu);

static void parseSecuredPdu(PduIdType RxPduId, PduInfoType* SecPdu, SecOC_RxIntermediateType *SecOCIntermediate);
static void constructDataToAuthenticatorRx(PduIdType RxPduId, SecOC_RxIntermediateType *SecOCIntermediate);
STATIC Std_ReturnType verify(PduIdType RxPduId, PduInfoType* SecPdu, SecOC_VerificationResultType *verification_result);
STATIC Std_ReturnType verify_PQC(PduIdType RxPduId, PduInfoType* SecPdu, SecOC_VerificationResultType *verification_result);
STATIC Std_ReturnType seperatePduCollectionTx(const PduIdType TxPduId,uint32 AuthPduLen , PduInfoType* securedPdu, PduInfoType* AuthPduCollection, PduInfoType* CryptoPduCollection, PduIdType* authPduId, PduIdType* cryptoPduId);


/********************************************************************************************************/
/******************************************Initialization************************************************/
/********************************************************************************************************/

void SecOC_Init(const SecOC_ConfigType *config)
{
    #ifdef SECOC_DEBUG
        (void)printf("######## in SecOC_Init \n");
    #endif

    if (config == NULL)
    {
        /* [SWS_SecOC_00054] SECOC_MODULE_ID = 150, SECOC_SID_INIT = 0x01, SECOC_E_PARAM_POINTER = 0x01 */
        (void)Det_ReportError(SECOC_MODULE_ID , 0U, SECOC_SID_INIT, SECOC_E_PARAM_POINTER);
        return;
    }

    /* [SWS_SecOC_00054] */
    SecOCGeneral = config->General;
    SecOCTxPduProcessing = config->SecOCTxPduProcessings;
    SecOCRxPduProcessing = config->SecOCRxPduProcessings;

    /* Increase all freshness counters to make it fresh */
    PduIdType idx;
    for (idx = 0U ; idx < SECOC_NUM_OF_TX_PDU_PROCESSING ; idx++)
    {      
        (void)FVM_IncreaseCounter(SecOCTxPduProcessing[idx].SecOCFreshnessValueId);
    }

    SecOCState = SECOC_INIT;
    
}                   


void SecOC_DeInit(void)
{

    #ifdef SECOC_DEBUG
        (void)printf("######## in SecOC_DeInit \n");
    #endif
    if(SecOCState != SECOC_INIT)
    {
        return;
    }

    SecOCState = SECOC_UNINIT;

    /* [SWS_SecOC_00157] */
    /* Emptying Tx/Rx buffers */
    PduIdType idx;
    for (idx = 0 ; idx < SECOC_NUM_OF_TX_PDU_PROCESSING; idx++) 
    {

        PduInfoType *authPdu = &(SecOCTxPduProcessing[idx].SecOCTxAuthenticPduLayer->SecOCTxAuthenticLayerPduRef);
        authPdu->SduLength = 0;
    }

    for (idx = 0 ; idx < SECOC_NUM_OF_RX_PDU_PROCESSING; idx++) 
    {

        PduInfoType *securedPdu = &(SecOCRxPduProcessing[idx].SecOCRxSecuredPduLayer->SecOCRxSecuredPdu->SecOCRxSecuredLayerPduRef);
        securedPdu->SduLength = 0;
    }

    SecOCGeneral = NULL;
    SecOCTxPduProcessing = NULL;
    SecOCRxPduProcessing = NULL;

}



/********************************************************************************************************/
/********************************************Transmission************************************************/
/**********************************The creation of a Secured I-PDU***************************************/
/********************************************************************************************************/

Std_ReturnType SecOC_IfTransmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr) 
{
    #ifdef SECOC_DEBUG
        (void)printf("######## in SecOC_IfTransmit \n");
    #endif
    Std_ReturnType result = E_OK;

    if (SecOCState != SECOC_INIT)
    {
        (void)Det_ReportError(SECOC_MODULE_ID, 0U, SECOC_SID_IF_TRANSMIT, SECOC_E_UNINIT);
        return E_NOT_OK;
    }

    if ((PduInfoPtr == NULL) || (PduInfoPtr->SduDataPtr == NULL))
    {
        (void)Det_ReportError(SECOC_MODULE_ID, 0U, SECOC_SID_IF_TRANSMIT, SECOC_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    if (SecOC_IsValidTxPduId(TxPduId) == FALSE)
    {
        (void)Det_ReportError(SECOC_MODULE_ID, 0U, SECOC_SID_IF_TRANSMIT, SECOC_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    PduInfoType *authpdu = &(SecOCTxPduProcessing[TxPduId].SecOCTxAuthenticPduLayer->SecOCTxAuthenticLayerPduRef);
    
    /* [SWS_SecOC_00252] The SecOC module shall copy the complete Authentic I-PDU to its internal memory before transmission */
    /* [SWS_SecOC_00110] Transmissions the upper layer shall overwrite the Authentic I-PDU without affecting the respective Secured I-PDU.*/
    (void)memcpy(authpdu->SduDataPtr, PduInfoPtr->SduDataPtr, PduInfoPtr->SduLength);
    authpdu->MetaDataPtr = PduInfoPtr->MetaDataPtr;
    authpdu->SduLength = PduInfoPtr->SduLength;

    /* [SWS_SecOC_00226], [SWS_SecOC_00225] */
    SecOC_TxCounters[TxPduId].AuthenticationCounter = 0;

    #ifdef SECOC_DEBUG
        (void)printf("###### Getting The Auth PDU \n");
        for(PduLengthType i = 0U; i < authpdu->SduLength; i++)
        {
            (void)printf("%d ", authpdu->SduDataPtr[i]);
        }
        (void)printf("\n");
    #endif

    return result;
}


/********************************************************
 *          * Function Info *                           *
 *                                                      *
 * Function_Name        : constructDataToAuthenticatorTx*
 * Function_Index       : SecOC internal                *
 * Parameter in         : TxPduId                       *
 * Parameter in         : DataToAuth                    *
 * Parameter in/out     : DataToAuthLen                 *
 * Parameter in         : AuthPdu                       *
 * Function_Descripton  : This function constructs the  *
 * DataToAuthenticator using Data Identifier, secured   *
 * part of the * Authentic I-PDU, and freshness value   *
 *******************************************************/
static void constructDataToAuthenticatorTx(const PduIdType TxPduId, SecOC_TxIntermediateType *SecOCIntermediate, const PduInfoType* AuthPdu)
{
    #ifdef SECOC_DEBUG
        (void)printf("######## in constructDataToAuthenticatorTx\n");
    #endif

    uint8  *DataToAuth    = SecOCIntermediate->DataToAuth;
    uint32 *DataToAuthLen = &SecOCIntermediate->DataToAuthLen;

    *DataToAuthLen = 0;
    /* DataToAuthenticator = Data Identifier | secured part of the Authentic I-PDU | Complete Freshness Value */
    /* Data Identifier "SecOC dataId" */
    (void)memcpy((void*)&DataToAuth[*DataToAuthLen], (const void*)&SecOCTxPduProcessing[TxPduId].SecOCDataId, sizeof(SecOCTxPduProcessing[TxPduId].SecOCDataId));
    *DataToAuthLen += sizeof(SecOCTxPduProcessing[TxPduId].SecOCDataId);

    /* secured part of the Authentic I-PDU */
    (void)memcpy(&DataToAuth[*DataToAuthLen], AuthPdu->SduDataPtr, AuthPdu->SduLength);
    *DataToAuthLen += AuthPdu->SduLength;

    uint32 FreshnesslenBytes = BIT_TO_BYTES(SecOCIntermediate->FreshnessLenBits);
    (void)memcpy(&DataToAuth[*DataToAuthLen], SecOCIntermediate->Freshness, FreshnesslenBytes);
    *DataToAuthLen += FreshnesslenBytes;
}


Std_ReturnType SecOC_GetTxFreshness(uint16 SecOCFreshnessValueID, uint8* SecOCFreshnessValue,
uint32* SecOCFreshnessValueLength) {
    #ifdef SECOC_DEBUG
        (void)printf("######## in SecOC_GetTxFreshness\n");
    #endif
    SecOC_GetTxFreshnessCalloutType PTR = (SecOC_GetTxFreshnessCalloutType)FVM_GetTxFreshness;
Std_ReturnType result = PTR(SecOCFreshnessValueID, SecOCFreshnessValue, SecOCFreshnessValueLength);
    return result;
}


Std_ReturnType SecOC_GetTxFreshnessTruncData (uint16 SecOCFreshnessValueID,uint8* SecOCFreshnessValue,
uint32* SecOCFreshnessValueLength,uint8* SecOCTruncatedFreshnessValue,uint32* SecOCTruncatedFreshnessValueLength) 
{
    #ifdef SECOC_DEBUG
        (void)printf("######## in SecOC_GetTxFreshnessTruncData\n");
    #endif
    Std_ReturnType result = FVM_GetTxFreshnessTruncData(SecOCFreshnessValueID, SecOCFreshnessValue , SecOCFreshnessValueLength,
    SecOCTruncatedFreshnessValue, SecOCTruncatedFreshnessValueLength);
    return result;
}


/************************************************************
*          * Function Info *                                *
*                                                           *
* Function_Name       : prepareFreshnessTx                  *
* Function_Index      : SecOC internal                      *
* Parameter in        : TxPduId, SecOCIntermediate          *
* Parameter in        : SecOCIntermediate                   *
* Function_Descripton : This function prepares the          *
* freshnes value for the secure communication. It           *
* will query freshness data using SecOC_GetTxFreshness or   *
* SecOC_GetTxFreshnessTruncData API based on configuration  *
* and store it in Intermediate structure                    *
************************************************************/                                         
static Std_ReturnType prepareFreshnessTx(const PduIdType TxPduId, SecOC_TxIntermediateType *SecOCIntermediate)
{
    #ifdef SECOC_DEBUG
        (void)printf("######## in prepareFreshnessTx \n");
    #endif
    Std_ReturnType result = E_OK;

    /* [SWS_SecOC_00220] */
    (void)memset(SecOCIntermediate->Freshness, 0, sizeof(SecOCIntermediate->Freshness));
    (void)memset(SecOCIntermediate->FreshnessTrunc, 0, sizeof(SecOCIntermediate->FreshnessTrunc));

    SecOCIntermediate->FreshnessLenBits = SecOCTxPduProcessing[TxPduId].SecOCFreshnessValueLength;
    SecOCIntermediate->FreshnessTruncLenBits = SecOCTxPduProcessing[TxPduId].SecOCFreshnessValueTruncLength;

    if(SecOCGeneral->SecOCQueryFreshnessValue == SECOC_CFUNC)
    {
        /* [SWS_SecOC_00221] */
        if(SecOCTxPduProcessing[TxPduId].SecOCProvideTxTruncatedFreshnessValue == TRUE)
        {

            /* [SWS_SecOC_00094] */
            result = SecOC_GetTxFreshnessTruncData(
                SecOCTxPduProcessing[TxPduId].SecOCFreshnessValueId,
                SecOCIntermediate->Freshness,
                &SecOCIntermediate->FreshnessLenBits,
                SecOCIntermediate->FreshnessTrunc,
                &SecOCIntermediate->FreshnessTruncLenBits
            );

        }
        /* [SWS_SecOC_00222] */
        else
        {

            result = SecOC_GetTxFreshness(
                SecOCTxPduProcessing[TxPduId].SecOCFreshnessValueId,
                SecOCIntermediate->Freshness,
                &SecOCIntermediate->FreshnessLenBits
            );

        }
    }

    return result;
}


/********************************************************
 *          * Function Info *                           *
 *                                                      *
 * Function_Name        : authenticate                  *
 * Function_Index       : SecOC internal                *
 * Parameter in         : TxPduId                       *
 * Parameter in         : AuthPdu                       *
 * Parameter out        : SecPdu                        * 
 * Function_Descripton  : This function generates the   *
 * secured PDU using authenticator, payload, freshness  * 
 *  value                                               *
 *******************************************************/
STATIC Std_ReturnType authenticate(const PduIdType TxPduId, PduInfoType* AuthPdu, PduInfoType* SecPdu)
{
    #ifdef SECOC_DEBUG
        (void)printf("######## in authenticate \n");
    #endif
    /* [SWS_SecOC_00031] authentication steps */
    Std_ReturnType result = E_NOT_OK;

    /* [SWS_SecOC_00033] */
    SecOC_TxIntermediateType SecOCIntermediate;

    result = prepareFreshnessTx(TxPduId, &SecOCIntermediate);
    /* [SWS_SecOC_00227] */
    if(( result == E_BUSY) ||  (result == E_NOT_OK))
    {
        return result;
    }

    /* [SWS_SecOC_00034] */
    constructDataToAuthenticatorTx(TxPduId, &SecOCIntermediate, AuthPdu);

    /* Authenticator generation */
    SecOCIntermediate.AuthenticatorLen = BIT_TO_BYTES(SecOCTxPduProcessing[TxPduId].SecOCAuthInfoTruncLength);

    /* [SWS_SecOC_00035], [SWS_SecOC_00036]  [SWS_SecOC_00012]*/
    result = Csm_MacGenerate(
        SecOC_GetTxCsmJobId(TxPduId),
        0,
        SecOCIntermediate.DataToAuth,
        SecOCIntermediate.DataToAuthLen,
        SecOCIntermediate.AuthenticatorPtr,
        &SecOCIntermediate.AuthenticatorLen
    );

    /* [SWS_SecOC_00227] */
    if( (result == E_NOT_OK) || (result == E_BUSY) || (result == QUEUE_FULL) )
    {
        return result;
    }

    /* [SWS_SecOC_00037] SECURED = HEADER(OPTIONAL) + AuthPdu + TruncatedFreshnessValue(OPTIONAL) + Authenticator */
    PduLengthType SecPduLen = 0;

    /* [SWS_SecOC_00262] Header */
    uint32 headerLen = SecOCTxPduProcessing[TxPduId].SecOCTxSecuredPduLayer->SecOCTxSecuredPdu->SecOCAuthPduHeaderLength;
    if(headerLen > 0U)
    {
        /* [SWS_SecOC_00261] The Secured I-PDU Header shall indicate the length of the Authentic I-PDU in bytes */
        (void)memcpy((void*)&SecPdu->SduDataPtr[SecPduLen], (const void*)&AuthPdu->SduLength, headerLen);
        SecPduLen += headerLen;
    }
    
    /* AuthPdu */
    (void)memcpy(&SecPdu->SduDataPtr[SecPduLen], AuthPdu->SduDataPtr, AuthPdu->SduLength);
    SecPduLen += AuthPdu->SduLength;

    /* [SWS_SecOC_00230], [SWS_SecOC_00231] */
    boolean IsFreshnessTruncated = (SecOCTxPduProcessing[TxPduId].SecOCProvideTxTruncatedFreshnessValue == TRUE) ? TRUE : FALSE;
    uint8 *MsgFreshness = (IsFreshnessTruncated) ? SecOCIntermediate.FreshnessTrunc : SecOCIntermediate.Freshness;
    uint32 FreshnesslenBytes = BIT_TO_BYTES(SecOCTxPduProcessing[TxPduId].SecOCFreshnessValueTruncLength);

    /* [SWS_SecOC_00094] TruncatedFreshnessValue */
    (void)memcpy(&SecPdu->SduDataPtr[SecPduLen], MsgFreshness, FreshnesslenBytes);
    SecPduLen += FreshnesslenBytes;

    /* Authenticator */
    (void)memcpy(&SecPdu->SduDataPtr[SecPduLen], SecOCIntermediate.AuthenticatorPtr, SecOCIntermediate.AuthenticatorLen);
    SecPduLen += SecOCIntermediate.AuthenticatorLen;

    SecPdu->SduLength = SecPduLen;

    /* [SWS_SecOC_00212] */
    SecPdu->MetaDataPtr = AuthPdu->MetaDataPtr;

    /* Clear Auth */
    AuthPdu->SduLength = 0;


    #ifdef SECOC_DEBUG
        (void)printf("result of authenticate is %d\n", result);

        /* ===== PDU STRUCTURE VISUALIZATION (Classical MAC Mode) ===== */
        (void)printf("\n");
        (void)printf("╔══════════════════════════════════════════════════════════════════════════════════╗\n");
        (void)printf("║                    SECURED I-PDU STRUCTURE (TX) - Classical Mode                 ║\n");
        (void)printf("╠═══════════════╦═══════════════════════╦═════════════════╦════════════════════════╣\n");
        (void)printf("║    Header     ║    Authentic PDU      ║   Freshness     ║   Authenticator (MAC)  ║\n");
        (void)printf("║   %2u bytes    ║      %3lu bytes        ║    %2u bytes     ║      %4u bytes        ║\n",
               headerLen, (unsigned long)(SecPduLen - headerLen - FreshnesslenBytes - SecOCIntermediate.AuthenticatorLen),
               FreshnesslenBytes, SecOCIntermediate.AuthenticatorLen);
        (void)printf("╠═══════════════╬═══════════════════════╬═════════════════╬════════════════════════╣\n");

        /* Print Header bytes */
        (void)printf("║ ");
        if(headerLen > 0U) {
            for(uint32 i = 0; i < headerLen && i < 4U; i++) { (void)printf("0x%02X ", SecPdu->SduDataPtr[i]); }
        } else {
            (void)printf("(none)       ");
        }
        (void)printf("║ ");

        /* Print first few AuthPdu bytes */
        uint32 authStart = headerLen;
        uint32 authLen = SecPduLen - headerLen - FreshnesslenBytes - SecOCIntermediate.AuthenticatorLen;
        for(uint32 i = 0; i < authLen && i < 4U; i++) { (void)printf("0x%02X ", SecPdu->SduDataPtr[authStart + i]); }
        if(authLen > 4U) { (void)printf("..."); }
        (void)printf("    ║ ");

        /* Print Freshness bytes */
        uint32 freshStart = headerLen + authLen;
        for(uint32 i = 0; i < FreshnesslenBytes && i < 2U; i++) { (void)printf("0x%02X ", SecPdu->SduDataPtr[freshStart + i]); }
        (void)printf("        ║ ");

        /* Print first few MAC bytes */
        uint32 macStart = freshStart + FreshnesslenBytes;
        for(uint32 i = 0; i < SecOCIntermediate.AuthenticatorLen && i < 4U; i++) { (void)printf("0x%02X ", SecPdu->SduDataPtr[macStart + i]); }
        (void)printf("  ║\n");

        (void)printf("╚═══════════════╩═══════════════════════╩═════════════════╩════════════════════════╝\n");
        (void)printf("Total Secured PDU: %lu bytes\n\n", (unsigned long)SecPduLen);
    #endif

    return result;
}

/********************************************************
 *          * Function Info *                           *
 *                                                      *
 * Function_Name        : seperatePduCollectionTx       *
 * Function_Index       : SecOC internal                *
 * Parameter in         : TxPduId                       *
 * Parameter in         : AuthPduLen                    *
 * Parameter in/out     : securedPdu                    *
 * Parameter in/out     : AuthPduCollection             *
 * Parameter in/out     : CryptoPduCollection           *
 * Parameter out         : authPduId                    *
 * Parameter out         : cryptoPduId                  *
 * Function_Descripton  : This function seperate the    *
 * secured pdu into two seperate pdus authentic and     *
 * crypto                                               *
 *******************************************************/
STATIC Std_ReturnType seperatePduCollectionTx(const PduIdType TxPduId,uint32 AuthPduLen , PduInfoType* securedPdu, PduInfoType* AuthPduCollection, PduInfoType* CryptoPduCollection, PduIdType* authPduId, PduIdType* cryptoPduId)
{
    uint32 headerLen = SecOCTxPduProcessing[TxPduId].SecOCTxSecuredPduLayer->SecOCTxSecuredPduCollection->SecOCTxAuthenticPdu->SecOCAuthPduHeaderLength;
    /* [SWS_SecOC_00210] */
    uint16 messageLinkLen = SecOCTxPduProcessing[TxPduId].SecOCTxSecuredPduLayer->SecOCTxSecuredPduCollection->SecOCUseMessageLink->SecOCMessageLinkLen;
    uint16 messageLinkPos = SecOCTxPduProcessing[TxPduId].SecOCTxSecuredPduLayer->SecOCTxSecuredPduCollection->SecOCUseMessageLink->SecOCMessageLinkPos;
    *authPduId = SecOCTxPduProcessing[TxPduId].SecOCTxSecuredPduLayer->SecOCTxSecuredPduCollection->SecOCTxAuthenticPdu->SecOCTxAuthenticPduId;
    *cryptoPduId = SecOCTxPduProcessing[TxPduId].SecOCTxSecuredPduLayer->SecOCTxSecuredPduCollection->SecOCTxCryptographicPdu->SecOCTxCryptographicPduId;
    uint32 AuthPduCollectionLen = AuthPduLen + headerLen;

    /* AuthPduCollection */
    (void)memcpy(AuthPduCollection->SduDataPtr, securedPdu->SduDataPtr, AuthPduCollectionLen);
    AuthPduCollection->MetaDataPtr =  securedPdu->MetaDataPtr;
    AuthPduCollection->SduLength = AuthPduCollectionLen;

    uint32 FreshnesslenBytes = BIT_TO_BYTES(SecOCTxPduProcessing[TxPduId].SecOCFreshnessValueTruncLength);
    uint32 AuthenticatorLen = BIT_TO_BYTES(SecOCTxPduProcessing[TxPduId].SecOCAuthInfoTruncLength);
    uint32 CryptoPduCollectionLen = FreshnesslenBytes+ AuthenticatorLen;

    /* CryptoPduCollection */
    (void)memcpy(CryptoPduCollection->SduDataPtr, &securedPdu->SduDataPtr[AuthPduCollectionLen], CryptoPduCollectionLen);


    /* MessageLink */
    /* [SWS_SecOC_00209] */
    (void)memcpy(&CryptoPduCollection->SduDataPtr[messageLinkPos], securedPdu->SduDataPtr, messageLinkLen);
    CryptoPduCollectionLen += messageLinkLen;

    CryptoPduCollection->MetaDataPtr = securedPdu->MetaDataPtr;
    CryptoPduCollection->SduLength = CryptoPduCollectionLen;

    /* Clear Secured Pdu*/
    securedPdu->SduLength = 0;

    return E_OK;
}

void SecOC_MainFunctionTx(void) 
{
    #ifdef SECOC_DEBUG
        (void)printf("######## in SecOCMainFunctionTx \n");
    #endif
    /* [SWS_SecOC_00177] */
    if(SecOCState == SECOC_UNINIT)
    {
        return;
    }
        
    PduIdType idx;
    PduIdType authPduId;
    PduIdType cryptoPduId;
    Std_ReturnType result;
    for (idx = 0 ; idx < SECOC_NUM_OF_TX_PDU_PROCESSING ; idx++) 
    {
        PduInfoType *authPdu = &(SecOCTxPduProcessing[idx].SecOCTxAuthenticPduLayer->SecOCTxAuthenticLayerPduRef);
        PduInfoType *securedPdu = &(SecOCTxPduProcessing[idx].SecOCTxSecuredPduLayer->SecOCTxSecuredPdu->SecOCTxSecuredLayerPduRef);
        SecOC_TxSecuredPduCollectionType * securePduCollection = (SecOCTxPduProcessing[idx].SecOCTxSecuredPduLayer->SecOCTxSecuredPduCollection);
        PduInfoType *AuthPduCollection;
        PduInfoType *CryptoPduCollection;

        /* Check if there is data */
        /* [SWS_SecOC_00179] */
        if (authPdu->SduLength > 0U) 
        {
            uint32 AuthPduLen = authPdu->SduLength;
            #ifdef SECOC_DEBUG
                (void)printf("send data to ID %d and data is ",idx);
                for(PduLengthType k = 0U; k < authPdu->SduLength; k++)
                {
                    (void)printf("%d ", authPdu->SduDataPtr[k] );
                }
                (void)printf("\n");
            #endif
            /* [SWS_SecOC_00060], [SWS_SecOC_00061] */
#if (SECOC_USE_PQC_MODE == TRUE)
            result = authenticate_PQC(idx , authPdu , securedPdu);
#else
            result = authenticate(idx , authPdu , securedPdu);
#endif

            if(result == E_OK )
            {
                #ifdef PDU_COLLECTION_DEBUG  
                    (void)printf("Secured data in pdu collection %d ",idx);
                    for(PduLengthType k = 0U; k < securedPdu->SduLength; k++)
                    {
                        (void)printf("%d ", securedPdu->SduDataPtr[k] );
                    }
                    (void)printf("\n");
                #endif
                /* Using Freshness Value Based on Single Freshness Counter we need to keep it synchronise 
                    increase counter before Broadcast as require */
                /*[SWS_SecOC_00031]*/
                (void)FVM_IncreaseCounter(SecOCTxPduProcessing[idx].SecOCFreshnessValueId);

                /* [SWS_SecOC_00201] */
                if(securePduCollection != NULL)
                {
                    AuthPduCollection = &(SecOCTxPduProcessing[idx].SecOCTxSecuredPduLayer->SecOCTxSecuredPduCollection->SecOCTxAuthenticPdu->SecOCTxAuthenticPduRef);
                    CryptoPduCollection = &(SecOCTxPduProcessing[idx].SecOCTxSecuredPduLayer->SecOCTxSecuredPduCollection->SecOCTxCryptographicPdu->SecOCTxCryptographicPduRef);
                    (void)seperatePduCollectionTx(idx, AuthPduLen , securedPdu , AuthPduCollection , CryptoPduCollection , &authPduId , &cryptoPduId);
                    
                    
                    /* [SWS_SecOC_00062] [SWS_SecOC_00202]*/
                    (void)PduR_SecOCTransmit(authPduId , AuthPduCollection);
                    (void)PduR_SecOCTransmit(cryptoPduId , CryptoPduCollection);
                }

                else
                {
                    /* [SWS_SecOC_00062] */
                    (void)PduR_SecOCTransmit(idx , securedPdu);
                }
            }
            else if ((result == E_BUSY) || (result == QUEUE_FULL))
            {
                /* [SWS_SecOC_00227] */
                SecOC_TxCounters[idx].AuthenticationCounter++;

                #ifdef COUNTERS_DEBUG
                (void)printf("SecOC_TxCounters[%d].AuthenticationCounter = %d\n",idx,SecOC_TxCounters[idx].AuthenticationCounter);
                (void)printf("SecOCTxPduProcessing[%d].SecOCAuthenticationBuildAttempts = %d\n",idx,SecOCTxPduProcessing[idx].SecOCAuthenticationBuildAttempts);
                #endif

                /* [SWS_SecOC_00228] */
                if( SecOC_TxCounters[idx].AuthenticationCounter >= SecOCTxPduProcessing[idx].SecOCAuthenticationBuildAttempts )
                {
                    authPdu->SduLength = 0;
                }
            }
            else /* result == E_NOT_OK */
            {
                authPdu->SduLength = 0;                
            }
        }
    }
}


BufReq_ReturnType SecOC_CopyTxData (PduIdType id,const PduInfoType* info,
const RetryInfoType* retry, PduLengthType* availableDataPtr)
{
    #ifdef SECOC_DEBUG
        (void)printf("######## in SecOC_CopyTxData \n");
    #endif
    if ((SecOCState != SECOC_INIT) || (SecOC_IsValidTxPduId(id) == FALSE) || (info == NULL) || (availableDataPtr == NULL))
    {
        return BUFREQ_E_NOT_OK;
    }

    if ((info->SduLength > 0U) && (info->SduDataPtr == NULL))
    {
        return BUFREQ_E_NOT_OK;
    }

    BufReq_ReturnType result = BUFREQ_OK;
    PduInfoType *securedPdu = &(SecOCTxPduProcessing[id].SecOCTxSecuredPduLayer->SecOCTxSecuredPdu->SecOCTxSecuredLayerPduRef);
    PduLengthType remainingBytes = securedPdu->SduLength - bufferRemainIndex[id];
    /* - Check if there is data in the buffer to be copy */
    if(securedPdu->SduLength > 0U)
    {
        /*  If not enough transmit data is available, no data is copied by the upper layer module 
        and BUFREQ_E_BUSY is returned */
        if(info->SduLength <= remainingBytes)
        {
            if(info->SduLength == 0U)
            {
                /* Querey amount of avalible data in upperlayer */
                *availableDataPtr = remainingBytes;
            }
            else
            {
                if(retry != NULL)
                {
                    switch (retry->TpDataState)
                    {
                        case TP_DATACONF:

                            /* indicates that all data that has been copied before this call is confirmed and 
                            can be removed from the TP buffer. Data copied by this API call is excluded and will be confirmed later */
                            (void)memcpy(info->SduDataPtr, &securedPdu->SduDataPtr[bufferRemainIndex[id]], info->SduLength);
                            bufferRemainIndex[id] += info->SduLength;
                            remainingBytes -= info->SduLength;
                            break;
                        case TP_CONFPENDING:
                            /* the previously copied data must remain in the TP buffer to be available for error recovery */
                            /* do nothing */
                            (void)memcpy(info->SduDataPtr, &securedPdu->SduDataPtr[bufferRemainIndex[id] - info->SduLength], info->SduLength);
                            break;
                        case TP_DATARETRY:
                            /* indicates that this API call shall copy previously copied data in order to recover from an error. 
                            In this case TxTpDataCnt specifies the offset in bytes from the current data copy position */
                            (void)memcpy(info->SduDataPtr, &securedPdu->SduDataPtr[bufferRemainIndex[id] - retry->TxTpDataCnt], info->SduLength);
                            break;
                        default:
                            result = BUFREQ_E_NOT_OK;
                        break;  
                    }
                }
                else
                {
                    /* Copy data then remove from the buffer */
                    (void)memcpy(info->SduDataPtr, &securedPdu->SduDataPtr[bufferRemainIndex[id]], info->SduLength);
                    bufferRemainIndex[id] += info->SduLength;
                    remainingBytes -= info->SduLength;
                }
                *availableDataPtr = remainingBytes;
            }
        }
        else
        {
            result = BUFREQ_E_BUSY;
        }
    }
    else
    {
        result = BUFREQ_E_NOT_OK;
    }
    #ifdef SECOC_DEBUG
        (void)printf("The result of SecOC_CopyTxData is %d \n and the info have : ",result );
        for(PduLengthType h = 0U; h < info->SduLength; h++)
        {
            (void)printf("%d ", info->SduDataPtr[h]);
        }
        (void)printf("\n");
    #endif
    return result;
}


void SecOC_TxConfirmation(PduIdType TxPduId, Std_ReturnType result) 
{
    #ifdef SECOC_DEBUG
        (void)printf("######## in SecOC_TxConfirmation \n");
    #endif
    
    if ((SecOCState != SECOC_INIT) || (SecOC_IsValidCollectionPduId(TxPduId) == FALSE))
    {
        return;
    }
    
    PduInfoType *securedPdu;
    PduInfoType *AuthPduCollection;
    PduInfoType *CryptoPduCollection;
    PduIdType pduCollectionId;
    PduIdType authCollectionId;
    PduIdType cryptoCollectionId;

    /* [SWS_SecOC_00220] */
    if((PdusCollections[TxPduId].Type == SECOC_AUTH_COLLECTON_PDU) || (PdusCollections[TxPduId].Type == SECOC_CRYPTO_COLLECTON_PDU))
    {
        PdusCollections[TxPduId].status = result;
        pduCollectionId = PdusCollections[TxPduId].CollectionId;
        authCollectionId = PdusCollections[TxPduId].AuthId;
        cryptoCollectionId = PdusCollections[TxPduId].CryptoId;

        AuthPduCollection = &(SecOCTxPduProcessing[pduCollectionId].SecOCTxSecuredPduLayer->SecOCTxSecuredPduCollection->SecOCTxAuthenticPdu->SecOCTxAuthenticPduRef);
        CryptoPduCollection = &(SecOCTxPduProcessing[pduCollectionId].SecOCTxSecuredPduLayer->SecOCTxSecuredPduCollection->SecOCTxCryptographicPdu->SecOCTxCryptographicPduRef);
        securedPdu = &(SecOCTxPduProcessing[pduCollectionId].SecOCTxSecuredPduLayer->SecOCTxSecuredPdu->SecOCTxSecuredLayerPduRef);

        if( (PdusCollections[authCollectionId].status == E_OK) && (PdusCollections[cryptoCollectionId].status == E_OK) )
        {
            PdusCollections[authCollectionId].status = 0x02;
            PdusCollections[cryptoCollectionId].status = 0x02;
            /* [SWS_SecOC_00064] */
            AuthPduCollection->SduLength = 0;
            CryptoPduCollection->SduLength = 0;
            securedPdu->SduLength = 0;
            /* [SWS_SecOC_00063] */
            PduR_SecOCIfTxConfirmation(pduCollectionId, E_OK);
        }
        else if( (PdusCollections[authCollectionId].status == E_NOT_OK) && (PdusCollections[cryptoCollectionId].status == E_OK) )
        {
            PdusCollections[authCollectionId].status = 0x02;
            PdusCollections[cryptoCollectionId].status = 0x02;
            /* [SWS_SecOC_00063] */
            PduR_SecOCIfTxConfirmation(pduCollectionId, E_NOT_OK);
        }
        else if( (PdusCollections[authCollectionId].status == E_OK) && (PdusCollections[cryptoCollectionId].status == E_NOT_OK) )
        {
            PdusCollections[authCollectionId].status = 0x02;
            PdusCollections[cryptoCollectionId].status = 0x02;
            /* [SWS_SecOC_00063] */
            PduR_SecOCIfTxConfirmation(pduCollectionId, E_NOT_OK);
        }
        else if( (PdusCollections[authCollectionId].status == E_NOT_OK) && (PdusCollections[cryptoCollectionId].status == E_NOT_OK) )
        {
            PdusCollections[authCollectionId].status = 0x02;
            PdusCollections[cryptoCollectionId].status = 0x02;
            /* [SWS_SecOC_00063] */
            PduR_SecOCIfTxConfirmation(pduCollectionId, E_NOT_OK);
        }
        else
        {
            /* Wait for both pdus to be confirmed */
        }
    }
    else
    {
        /* [SWS_SecOC_00064] */
        securedPdu = &(SecOCTxPduProcessing[TxPduId].SecOCTxSecuredPduLayer->SecOCTxSecuredPdu->SecOCTxSecuredLayerPduRef);
        if (result == E_OK) 
        {
            securedPdu->SduLength = 0;
        }
        /* [SWS_SecOC_00063] */
        PduR_SecOCIfTxConfirmation(TxPduId, result);
    }
}


void SecOC_TpTxConfirmation(PduIdType TxPduId,Std_ReturnType result)
{
    if ((SecOCState != SECOC_INIT) || (SecOC_IsValidTxPduId(TxPduId) == FALSE))
    {
        return;
    }

    PduInfoType *securedPdu = &(SecOCTxPduProcessing[TxPduId].SecOCTxSecuredPduLayer->SecOCTxSecuredPdu->SecOCTxSecuredLayerPduRef);
    #ifdef SECOC_DEBUG
        (void)printf("########  In SecOC_TpTxConfirmation \n result of SecOC_TpTxConfirmation is %d \n" , result);
    #endif
    if (result == E_OK) {
        /* Clear buffer */
        securedPdu->SduLength = 0;
        bufferRemainIndex[TxPduId] = 0;
    }

    /* [SWS_SecOC_00074] */
    if (SecOCTxPduProcessing[TxPduId].SecOCTxAuthenticPduLayer->SecOCPduType == SECOC_TPPDU)
    {
        /* [SWS_SecOC_00063] */
        PduR_SecOCTpTxConfirmation(TxPduId, result);
    }
    else if (SecOCTxPduProcessing[TxPduId].SecOCTxAuthenticPduLayer->SecOCPduType == SECOC_IFPDU)
    {
        /* [SWS_SecOC_00063] */
        PduR_SecOCIfTxConfirmation(TxPduId, result);
    }
    else
    {
        /* DET Report Error */
    }

}


Std_ReturnType SecOC_IfCancelTransmit(PduIdType TxPduId)
{
    #ifdef SECOC_DEBUG
        (void)printf("######## in SecOC_IfCancelTransmit \n");
    #endif
    Std_ReturnType result = E_OK;

    PduInfoType *authpdu = &(SecOCTxPduProcessing[TxPduId].SecOCTxAuthenticPduLayer->SecOCTxAuthenticLayerPduRef);
    
    authpdu->SduLength = 0;
    return result;
}



/********************************************************************************************************/
/*********************************************Reception**************************************************/
/********************************The verification of a Secured I-PDU*************************************/
/********************************************************************************************************/

void SecOC_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{   
    #ifdef SECOC_DEBUG
        (void)printf("######## in SecOC_RxIndication \n");
    #endif
    if ((SecOCState != SECOC_INIT) || (PduInfoPtr == NULL) || (PduInfoPtr->SduDataPtr == NULL))
    {
        return;
    }

    if (SecOC_IsValidCollectionPduId(RxPduId) == FALSE)
    {
        return;
    }

    /* [SWS_SecOC_00124] */
    

    uint8 headerLen;
    uint8 authLen;
    if((PdusCollections[RxPduId].Type == SECOC_AUTH_COLLECTON_PDU) || (PdusCollections[RxPduId].Type == SECOC_CRYPTO_COLLECTON_PDU))
    {
        PduInfoType *securedPdu;
        PduInfoType *AuthPduCollection;
        PduInfoType *CryptoPduCollection;
        PduIdType pduCollectionId;
        pduCollectionId = PdusCollections[RxPduId].CollectionId;
        
        AuthPduCollection = &(SecOCRxPduProcessing[pduCollectionId].SecOCRxSecuredPduLayer->SecOCRxSecuredPduCollection->SecOCRxAuthenticPdu->SecOCRxAuthenticPduRef);
        CryptoPduCollection = &(SecOCRxPduProcessing[pduCollectionId].SecOCRxSecuredPduLayer->SecOCRxSecuredPduCollection->SecOCRxCryptographicPdu->SecOCRxCryptographicPduRef);

        securedPdu = &(SecOCRxPduProcessing[pduCollectionId].SecOCRxSecuredPduLayer->SecOCRxSecuredPdu->SecOCRxSecuredLayerPduRef);
        
        if((PdusCollections[RxPduId].Type == SECOC_AUTH_COLLECTON_PDU) && (securedPdu->SduLength == 0U))
        {
            
            /* AuthPduCollection */
            headerLen = SecOCRxPduProcessing[pduCollectionId].SecOCRxSecuredPduLayer->SecOCRxSecuredPdu->SecOCAuthPduHeaderLength;
            (void)memcpy(&authLen, &PduInfoPtr->SduDataPtr[0] ,headerLen);
            authLen += headerLen;
            (void)memcpy(AuthPduCollection->SduDataPtr,PduInfoPtr->SduDataPtr, authLen); /* from pduinfoptr finished */
            AuthPduCollection->SduLength= authLen;
        }
        else if((PdusCollections[RxPduId].Type == SECOC_CRYPTO_COLLECTON_PDU) && (securedPdu->SduLength == 0U))
        {

            /* copy from pduinfoptr to crypto */
            (void)memcpy(CryptoPduCollection->SduDataPtr,PduInfoPtr->SduDataPtr,PduInfoPtr->SduLength);
            CryptoPduCollection->SduLength=PduInfoPtr->SduLength;
        }
        else
        {
            /* No action required */
        }

        /* [SWS_SecOC_00203] */
        /* if not secued and the length not >0 out of the crypto and in the pdu collection */
        if((AuthPduCollection->SduLength > 0U) && (CryptoPduCollection->SduLength > 0U))
        {
            uint16 messageLinkLen = SecOCRxPduProcessing[pduCollectionId].SecOCRxSecuredPduLayer->SecOCRxSecuredPduCollection->SecOCUseMessageLink->SecOCMessageLinkLen;
            uint16 messageLinkPos = SecOCRxPduProcessing[pduCollectionId].SecOCRxSecuredPduLayer->SecOCRxSecuredPduCollection->SecOCUseMessageLink->SecOCMessageLinkPos;
            
            /* AuthPduCollection */
            (void)memcpy(securedPdu->SduDataPtr,AuthPduCollection->SduDataPtr,AuthPduCollection->SduLength);
            securedPdu->SduLength+=AuthPduCollection->SduLength;

            /* CryptoPduCollection */
            (void)memcpy(&(securedPdu->SduDataPtr[securedPdu->SduLength]),CryptoPduCollection->SduDataPtr, (CryptoPduCollection->SduLength)-messageLinkLen);
            
            securedPdu->SduLength+=(CryptoPduCollection->SduLength)-messageLinkLen;
            AuthPduCollection->SduLength = 0;
            CryptoPduCollection->SduLength = 0;
            #ifdef PDU_COLLECTION_DEBUG
                (void)printf("########  both received and secured length = %lu\n" , securedPdu->SduLength);
                (void)printf("Data Recieve in secured pdu : ");
                for(uint8 i = 0; i < securedPdu->SduLength; i++)
                {
                    (void)printf("%d ",securedPdu->SduDataPtr[i]);
                }
                (void)printf("\n");
            #endif
        }
    }
    else
    {
        /* The SecOC copies the Authentic I-PDU to its own buffer */
        PduInfoType *securedPdu = &(SecOCRxPduProcessing[RxPduId].SecOCRxSecuredPduLayer->SecOCRxSecuredPdu->SecOCRxSecuredLayerPduRef);
        uint32 headerLen = SecOCRxPduProcessing[RxPduId].SecOCRxSecuredPduLayer->SecOCRxSecuredPdu->SecOCAuthPduHeaderLength;



        (void)memcpy(securedPdu->SduDataPtr, PduInfoPtr->SduDataPtr, PduInfoPtr->SduLength);
        securedPdu->MetaDataPtr = PduInfoPtr->MetaDataPtr;

        /* [SWS_SecOC_00078] */
        if(headerLen > 0U)
        {
            (void)memcpy(&authLen, &PduInfoPtr->SduDataPtr[0] ,headerLen);
            authRecieveLength[RxPduId] = authLen;
#if SECOC_USE_PQC_MODE == TRUE
            /* PQC Mode: Use actual received length (ML-DSA signature is 3309 bytes, not 4) */
            securedPdu->SduLength = MIN(PduInfoPtr->SduLength, SECOC_SECPDU_MAX_LENGTH);
            #ifdef SECOC_DEBUG
                (void)printf("PQC Mode: Setting securedPdu->SduLength = %u (from PduInfoPtr->SduLength = %u)\n",
                       securedPdu->SduLength, PduInfoPtr->SduLength);
            #endif
#else
            /* Classical Mode: Calculate from configuration (fixed MAC size) */
            PduLengthType securePduLength = headerLen + authRecieveLength[RxPduId] + BIT_TO_BYTES(SecOCRxPduProcessing[RxPduId].SecOCFreshnessValueTruncLength) + BIT_TO_BYTES(SecOCRxPduProcessing[RxPduId].SecOCAuthInfoTruncLength);
            securedPdu->SduLength = securePduLength;
            #ifdef SECOC_DEBUG
                (void)printf("Classical Mode: Setting securedPdu->SduLength = %u (calculated)\n", securedPdu->SduLength);
            #endif
#endif
        }
        else
        {
            securedPdu->SduLength = MIN(PduInfoPtr->SduLength, SECOC_SECPDU_MAX_LENGTH);
        }
        
        /* [SWS_SecOC_00234], [SWS_SecOC_00235] */
        SecOC_RxCounters[RxPduId].AuthenticationCounter = 0;
        SecOC_RxCounters[RxPduId].VerificationCounter = 0; 
    }
}


BufReq_ReturnType SecOC_StartOfReception ( PduIdType id, const PduInfoType* info, PduLengthType TpSduLength, PduLengthType* bufferSizePtr )
{
    /* [SWS_SecOC_00130] */
    #ifdef SECOC_DEBUG
        (void)printf("######## in SecOC_StartOfReception \n");
    #endif
    if ((SecOCState != SECOC_INIT) || (SecOC_IsValidRxPduId(id) == FALSE) || (bufferSizePtr == NULL))
    {
        return BUFREQ_E_NOT_OK;
    }

    if ((info != NULL) && (info->SduLength > 0U) && (info->SduDataPtr == NULL))
    {
        return BUFREQ_E_NOT_OK;
    }

	uint8 AuthHeadlen;
	AuthHeadlen=SecOCRxPduProcessing[id].SecOCRxSecuredPduLayer->SecOCRxSecuredPdu->SecOCAuthPduHeaderLength;
    /* [SWS_SecOC_00082] */
    PduInfoType *securedPdu = &(SecOCRxPduProcessing[id].SecOCRxSecuredPduLayer->SecOCRxSecuredPdu->SecOCRxSecuredLayerPduRef);

    /* Clear the buffer at the start of a new reception to prevent data accumulation */
    securedPdu->SduLength = 0;
    (void)memset(securedPdu->SduDataPtr, 0, SECOC_SECPDU_MAX_LENGTH);

    *bufferSizePtr = SECOC_SECPDU_MAX_LENGTH - securedPdu->SduLength;
    BufReq_ReturnType result = BUFREQ_OK;
    uint32 datalen=0;
    /* [SWS_SecOC_00130] */
    if(TpSduLength>*bufferSizePtr)
    {
        result = BUFREQ_E_OVFL;
        /*[SWS_SecOC_00215]*/
        if(SecOCRxPduProcessing[id].SecOCReceptionOverflowStrategy==SECOC_REJECT)
        {
            result = BUFREQ_E_NOT_OK;
        }
    }
    else if (TpSduLength == 0U)
    {
        /* [SWS_SecOC_00181] */
        result = BUFREQ_E_NOT_OK;
    }
    else
    {
        if((info->SduDataPtr != NULL))
        {
            /* [SWS_SecOC_00263] check if dynamic*/            
            if(AuthHeadlen > 0U)
            {
                (void)memcpy((uint8*)&datalen, info->SduDataPtr, AuthHeadlen );
                if(datalen > SECOC_AUTHPDU_MAX_LENGTH)
                {
                    result = BUFREQ_E_NOT_OK;
                }
                authRecieveLength[id] = datalen;
            }
            else
            {
                authRecieveLength[id] = SecOCRxPduProcessing[id].SecOCRxAuthenticPduLayer->SecOCRxAuthenticLayerPduRef.SduLength;
            }
        }
    }
        
    if(SecOCRxPduProcessing[id].SecOCRxAuthenticPduLayer->SecOCPduType==SECOC_TPPDU)
    {
        /* [SWS_SecOC_00082] */
		/*result = PduR_SecOCTpStartOfReception();*/
	}
    #ifdef SECOC_DEBUG
        (void)printf("result of SecOC_StartOfReception is %d\n", result);
    #endif
	return result;
}


BufReq_ReturnType SecOC_CopyRxData (PduIdType id, const PduInfoType* info, PduLengthType* bufferSizePtr)
{
    /* [SWS_SecOC_00128] */
    #ifdef SECOC_DEBUG
        (void)printf("######## in SecOC_CopyRxData \n");
    #endif
    if ((SecOCState != SECOC_INIT) || (SecOC_IsValidRxPduId(id) == FALSE) || (bufferSizePtr == NULL) || (info == NULL))
    {
        return BUFREQ_E_NOT_OK;
    }

    if ((info->SduLength > 0U) && (info->SduDataPtr == NULL))
    {
        return BUFREQ_E_NOT_OK;
    }

    BufReq_ReturnType result = BUFREQ_OK;
    
    /* Create a pointer to the secured I-PDU buffer that we will store the data into it */
    /* [SWS_SecOC_00082] */
    PduInfoType *securedPdu = &(SecOCRxPduProcessing[id].SecOCRxSecuredPduLayer->SecOCRxSecuredPdu->SecOCRxSecuredLayerPduRef);


    if(info->SduLength == 0U)
    {
        /* An SduLength of 0 can be used to query the
        * current amount of available buffer in the upper layer module. In this
        * case, the SduDataPtr may be a NULL_PTR.
        */
        *bufferSizePtr = SECOC_SECPDU_MAX_LENGTH - securedPdu->SduLength;
    }
    else if((info->SduLength > 0U) && (info->SduDataPtr != NULL))
    {
        /*[SWS_SecOC_00083]*/
        (void)memcpy(&securedPdu->SduDataPtr[securedPdu->SduLength], info->SduDataPtr, info->SduLength);
        securedPdu->SduLength += info->SduLength;

        *bufferSizePtr = SECOC_SECPDU_MAX_LENGTH - securedPdu->SduLength;
    }
    else
    {
        result = BUFREQ_E_NOT_OK;
    }


    return result;
}


void SecOC_TpRxIndication(PduIdType Id,Std_ReturnType result)
{
    #ifdef SECOC_DEBUG
        (void)printf("######## in SecOC_TpRxIndication \n");
    #endif
    /* [SWS_SecOC_00125] */
    PduInfoType *securedPdu = &(SecOCRxPduProcessing[Id].SecOCRxSecuredPduLayer->SecOCRxSecuredPdu->SecOCRxSecuredLayerPduRef);

    if (result==E_NOT_OK)
    {
        /* Clear buffer */
        securedPdu->SduLength = 0;

        if (SecOCRxPduProcessing[Id].SecOCRxAuthenticPduLayer->SecOCPduType == SECOC_TPPDU)
        {
            PduR_SecOCTpRxIndication(Id, E_NOT_OK);
        }
    }
}


Std_ReturnType SecOC_GetRxFreshness(uint16 SecOCFreshnessValueID,const uint8* SecOCTruncatedFreshnessValue,uint32 SecOCTruncatedFreshnessValueLength,
    uint16 SecOCAuthVerifyAttempts,uint8* SecOCFreshnessValue,uint32* SecOCFreshnessValueLength)
{
    #ifdef SECOC_DEBUG
        (void)printf("######## in SecOC_GetRxFreshness \n");
    #endif
    return FVM_GetRxFreshness(SecOCFreshnessValueID,SecOCTruncatedFreshnessValue,SecOCTruncatedFreshnessValueLength,
    SecOCAuthVerifyAttempts,SecOCFreshnessValue,SecOCFreshnessValueLength);
}


static void parseSecuredPdu(PduIdType RxPduId, PduInfoType* SecPdu, SecOC_RxIntermediateType *SecOCIntermediate)
{
    #ifdef SECOC_DEBUG
        (void)printf("######## in parseSecuredPdu \n");
    #endif
    /* Track the current byte of secured to be parsed */
    uint32 SecCursor = 0;  

    /* Get data length from configuration or header if found */
    uint32 headerLen = SecOCRxPduProcessing[RxPduId].SecOCRxSecuredPduLayer->SecOCRxSecuredPdu->SecOCAuthPduHeaderLength;
    SecOCIntermediate->authenticPduLen = 0;
    if(headerLen > 0U)
    {
        /* [SWS_SecOC_00259] */
        (void)memcpy((void*)&SecOCIntermediate->authenticPduLen, (const void*)&SecPdu->SduDataPtr[SecCursor], headerLen);
        SecCursor += headerLen;
    }
    else
    {
        /* [SWS_SecOC_00257] */
        SecOCIntermediate->authenticPduLen =  SecOCRxPduProcessing[RxPduId].SecOCRxAuthenticPduLayer->SecOCRxAuthenticLayerPduRef.SduLength;
    }

    /* Copy authenticPdu to intermediate */
    (void)memcpy(SecOCIntermediate->authenticPdu, &SecPdu->SduDataPtr[SecCursor], SecOCIntermediate->authenticPduLen);
    SecCursor += SecOCIntermediate->authenticPduLen;

    uint16 authVeriAttempts = 0;
    /* Get Rx freshness from FVM using the truncated freshness in SecPdu */ 
    const uint8* SecOCTruncatedFreshnessValue = &SecPdu->SduDataPtr[SecCursor];
    uint32 SecOCTruncatedFreshnessValueLength = SecOCRxPduProcessing[RxPduId].SecOCFreshnessValueTruncLength;
    SecOCIntermediate->freshnessLenBits = SecOCRxPduProcessing[RxPduId].SecOCFreshnessValueLength;
    
    /* init freshness in struct SecOCIntermediate with 0 */
    (void)memset(SecOCIntermediate->freshness, 0, sizeof(SecOCIntermediate->freshness));
    /* [SWS_SecOC_00250] */
    SecOCIntermediate->freshnessResult = SecOC_GetRxFreshness(
            SecOCRxPduProcessing[RxPduId].SecOCFreshnessValueId,
            SecOCTruncatedFreshnessValue, 
            SecOCTruncatedFreshnessValueLength, 
            authVeriAttempts,
            SecOCIntermediate->freshness, 
            &SecOCIntermediate->freshnessLenBits
    );

    SecCursor += BIT_TO_BYTES(SecOCTruncatedFreshnessValueLength);

    /* Copy Mac/Signature to intermediate */
#if SECOC_USE_PQC_MODE == TRUE
    /* PQC Mode: Signature is all remaining bytes (ML-DSA-65 = 3309 bytes) */
    uint32 actualSignatureLen = SecPdu->SduLength - SecCursor;
    SecOCIntermediate->macLenBits = actualSignatureLen * 8U;  /* Convert bytes to bits */
    (void)memcpy(SecOCIntermediate->mac, &SecPdu->SduDataPtr[SecCursor], actualSignatureLen);
    SecCursor += actualSignatureLen;
    #ifdef SECOC_DEBUG
        (void)printf("PQC Mode: Extracted signature, actualSignatureLen = %lu bytes (SecPdu->SduLength=%u, SecCursor was=%lu)\n",
               actualSignatureLen, SecPdu->SduLength, SecPdu->SduLength - actualSignatureLen);
    #endif
#else
    /* Classical Mode: MAC size from configuration (32 bits = 4 bytes) */
    SecOCIntermediate->macLenBits = SecOCRxPduProcessing[RxPduId].SecOCAuthInfoTruncLength;
    (void)memcpy(SecOCIntermediate->mac, &SecPdu->SduDataPtr[SecCursor], BIT_TO_BYTES(SecOCIntermediate->macLenBits));
    SecCursor += BIT_TO_BYTES(SecOCIntermediate->macLenBits);
#endif
    #ifdef SECOC_DEBUG
        /* ===== PDU STRUCTURE VISUALIZATION (RX - Parsing Received Secured PDU) ===== */
        uint32 freshLenBytes = BIT_TO_BYTES(SecOCRxPduProcessing[RxPduId].SecOCFreshnessValueTruncLength);
        uint32 macLenBytes = BIT_TO_BYTES(SecOCIntermediate->macLenBits);

        (void)printf("\n");
#if SECOC_USE_PQC_MODE == TRUE
        (void)printf("╔════════════════════════════════════════════════════════════════════════════════════════════╗\n");
        (void)printf("║                      SECURED I-PDU STRUCTURE (RX) - PQC Mode (ML-DSA-65)                   ║\n");
        (void)printf("╠═══════════════╦═══════════════════════╦═════════════════╦══════════════════════════════════╣\n");
        (void)printf("║    Header     ║    Authentic PDU      ║   Freshness     ║  Authenticator (ML-DSA-65 Sig)   ║\n");
        (void)printf("║   %2u bytes    ║      %3u bytes        ║    %2u bytes     ║        %4u bytes                ║\n",
               headerLen, SecOCIntermediate->authenticPduLen, freshLenBytes, macLenBytes);
        (void)printf("╠═══════════════╬═══════════════════════╬═════════════════╬══════════════════════════════════╣\n");
#else
        (void)printf("╔══════════════════════════════════════════════════════════════════════════════════╗\n");
        (void)printf("║                    SECURED I-PDU STRUCTURE (RX) - Classical Mode                 ║\n");
        (void)printf("╠═══════════════╦═══════════════════════╦═════════════════╦════════════════════════╣\n");
        (void)printf("║    Header     ║    Authentic PDU      ║   Freshness     ║   Authenticator (MAC)  ║\n");
        (void)printf("║   %2u bytes    ║      %3u bytes        ║    %2u bytes     ║      %4u bytes        ║\n",
               headerLen, SecOCIntermediate->authenticPduLen, freshLenBytes, macLenBytes);
        (void)printf("╠═══════════════╬═══════════════════════╬═════════════════╬════════════════════════╣\n");
#endif

        /* Print Header indicator */
        (void)printf("║ ");
        if(headerLen > 0U) {
            (void)printf("0x%02X         ", SecOCIntermediate->authenticPduLen);
        } else {
            (void)printf("(none)       ");
        }
        (void)printf("║ ");

        /* Print first few AuthPdu bytes */
        for(uint32 i = 0; i < SecOCIntermediate->authenticPduLen && i < 4U; i++) { (void)printf("0x%02X ", SecOCIntermediate->authenticPdu[i]); }
        if(SecOCIntermediate->authenticPduLen > 4U) { (void)printf("..."); }
        (void)printf("    ║ ");

        /* Print Freshness bytes */
        for(uint32 i = 0; i < freshLenBytes && i < 2U; i++) { (void)printf("0x%02X ", SecOCIntermediate->freshness[i]); }
        (void)printf("        ║ ");

        /* Print first few MAC/Signature bytes */
        for(uint32 i = 0; i < macLenBytes && i < 4U; i++) { (void)printf("0x%02X ", SecOCIntermediate->mac[i]); }
        if(macLenBytes > 4U) { (void)printf("..."); }
        (void)printf("  ║\n");

#if SECOC_USE_PQC_MODE == TRUE
        (void)printf("╚═══════════════╩═══════════════════════╩═════════════════╩══════════════════════════════════╝\n");
#else
        (void)printf("╚═══════════════╩═══════════════════════╩═════════════════╩════════════════════════════════════╝\n");
#endif
        (void)printf("Total Received PDU: %u bytes\n", SecPdu->SduLength);
        (void)printf("Parsed: Header=%u + Auth=%u + Fresh=%u + MAC/Sig=%u\n\n",
               headerLen, SecOCIntermediate->authenticPduLen, freshLenBytes, macLenBytes);
    #endif
    return;
}


static void constructDataToAuthenticatorRx(PduIdType RxPduId, SecOC_RxIntermediateType *SecOCIntermediate)
{
    #ifdef SECOC_DEBUG
        (void)printf("######## in constructDataToAuthenticatorRx \n");
    #endif

    uint8  *DataToAuth    = SecOCIntermediate->DataToAuth;
    uint32 *DataToAuthLen = &SecOCIntermediate->DataToAuthLen;

    *DataToAuthLen = 0;
	/* Copy the Id to buffer Data to Auth */
    (void)memcpy((void*)&DataToAuth[*DataToAuthLen], (const void*)&SecOCRxPduProcessing[RxPduId].SecOCDataId, sizeof(SecOCRxPduProcessing[RxPduId].SecOCDataId));
    *DataToAuthLen += sizeof(SecOCRxPduProcessing[RxPduId].SecOCDataId);	


    /* copy the authenticPdu to buffer DatatoAuth */
    (void)memcpy(&DataToAuth[*DataToAuthLen], SecOCIntermediate->authenticPdu, SecOCIntermediate->authenticPduLen);
    *DataToAuthLen += SecOCIntermediate->authenticPduLen;

    /* copy the freshness value to buffer Data to Auth */
    (void)memcpy(&DataToAuth[*DataToAuthLen], SecOCIntermediate->freshness, BIT_TO_BYTES(SecOCIntermediate->freshnessLenBits));
    *DataToAuthLen += (BIT_TO_BYTES(SecOCIntermediate->freshnessLenBits));

}


/* cppcheck-suppress misra-c2012-5.8 */
STATIC Std_ReturnType verify(PduIdType RxPduId, PduInfoType* SecPdu, SecOC_VerificationResultType *verification_result)
{
    #ifdef SECOC_DEBUG
        (void)printf("######## in verify \n");
    #endif
    /* [SWS_SecOC_00040] verifcation steps */

    SecOC_RxIntermediateType    SecOCIntermediate;

    /* [SWS_SecOC_00042] Parsing */
    parseSecuredPdu(RxPduId, SecPdu, &SecOCIntermediate);

    *verification_result = SECOC_NO_VERIFICATION;
    boolean SecOCSecuredRxPduVerification = TRUE;
    if((PdusCollections[RxPduId].Type == SECOC_AUTH_COLLECTON_PDU) || (PdusCollections[RxPduId].Type == SECOC_CRYPTO_COLLECTON_PDU))
    {
        SecOCSecuredRxPduVerification = SecOCRxPduProcessing[RxPduId].SecOCRxSecuredPduLayer->SecOCRxSecuredPduCollection->SecOCSecuredRxPduVerification;
    }
    else
    {
        SecOCSecuredRxPduVerification = SecOCRxPduProcessing[RxPduId].SecOCRxSecuredPduLayer->SecOCRxSecuredPdu->SecOCSecuredRxPduVerification;
    }

    /* [SWS_SecOC_00265] */
    if(SecOCSecuredRxPduVerification == TRUE)
    {
        if((SecOCIntermediate.freshnessResult == E_BUSY) || (SecOCIntermediate.freshnessResult == E_NOT_OK))
        {
            /* [SWS_SecOC_00256] */
            if( SecOCIntermediate.freshnessResult == E_NOT_OK)
            {
                *verification_result = SECOC_FRESHNESSFAILURE;
            }

            return SecOCIntermediate.freshnessResult;
        }

        /* [SWS_SecOC_00046] */
        constructDataToAuthenticatorRx(RxPduId, &SecOCIntermediate);

        Crypto_VerifyResultType verify_var;

        /* [SWS_SecOC_00047] */
        Std_ReturnType Mac_verify = Csm_MacVerify(
            SecOC_GetRxCsmJobId(RxPduId),
            CRYPTO_OPERATIONMODE_SINGLECALL,
            SecOCIntermediate.DataToAuth,
            SecOCIntermediate.DataToAuthLen,
            SecOCIntermediate.mac,
            SecOCIntermediate.macLenBits,
            &verify_var
        );

        if ( (Mac_verify == E_BUSY) || (Mac_verify == QUEUE_FULL) || (Mac_verify == E_NOT_OK))
        {

            /* [SWS_SecOC_00240] */
            if( SecOC_RxCounters[RxPduId].AuthenticationCounter == SecOCRxPduProcessing[RxPduId].SecOCAuthenticationBuildAttempts )
            {
                *verification_result = SECOC_AUTHENTICATIONBUILDFAILURE;
            }
            /* [SWS_SecOC_00241] */
            if(Mac_verify == E_NOT_OK)
            {
                *verification_result = SECOC_VERIFICATIONFAILURE;
            }

            return Mac_verify;

        }
        else if ( (Mac_verify == CRYPTO_E_KEY_NOT_VALID) || (Mac_verify == CRYPTO_E_KEY_EMPTY) )
        {
            /* [SWS_SecOC_00241], [SWS_SecOC_00121] */
            if( SecOC_RxCounters[RxPduId].VerificationCounter == SecOCRxPduProcessing[RxPduId].SecOCAuthenticationVerifyAttempts )
            {
                *verification_result = SECOC_VERIFICATIONFAILURE;
            }
            
            return Mac_verify;
        }
        else
        {
            /* No action required */
        }
    }
    /* [SWS_SecOC_00242] */
    *verification_result = SECOC_VERIFICATIONSUCCESS;

    PduInfoType *authPdu = &(SecOCRxPduProcessing[RxPduId].SecOCRxAuthenticPduLayer->SecOCRxAuthenticLayerPduRef);

    /* Copy authenticPdu from secured layer to the authentic layer */
    (void)memcpy(authPdu->SduDataPtr, SecOCIntermediate.authenticPdu, SecOCIntermediate.authenticPduLen);
    authPdu->SduLength = SecOCIntermediate.authenticPduLen;
    authPdu->MetaDataPtr = SecPdu->MetaDataPtr;

    /* [SWS_SecOC_00087] */
    SecPdu->SduLength = 0;

    (void)FVM_UpdateCounter(SecOCRxPduProcessing[RxPduId].SecOCFreshnessValueId, SecOCIntermediate.freshness, SecOCIntermediate.freshnessLenBits);

    #ifdef SECOC_DEBUG
    (void)printf("%d\n",*verification_result);
    #endif

    return E_OK;
}

/********************************************************************************************************/
/***********************************POST-QUANTUM CRYPTOGRAPHY FUNCTIONS**********************************/
/********************************************************************************************************/

/**
 * @brief PQC-enabled authentication using ML-DSA-65 digital signatures
 * @details Similar to authenticate() but uses Csm_SignatureGenerate instead of Csm_MacGenerate
 */
STATIC Std_ReturnType authenticate_PQC(const PduIdType TxPduId, PduInfoType* AuthPdu, PduInfoType* SecPdu)
{
    #ifdef SECOC_DEBUG
        (void)printf("######## in authenticate_PQC (ML-DSA-65)\n");
    #endif

    Std_ReturnType result = E_NOT_OK;
    SecOC_TxIntermediateType SecOCIntermediate;

    /* Prepare freshness value */
    result = prepareFreshnessTx(TxPduId, &SecOCIntermediate);
    if((result == E_BUSY) || (result == E_NOT_OK))
    {
        return result;
    }

    /* Construct data to be signed */
    constructDataToAuthenticatorTx(TxPduId, &SecOCIntermediate, AuthPdu);

    /* Signature generation (replaces MAC generation) */
    uint32 signatureLen = 0;

    result = Csm_SignatureGenerate(
        SecOC_GetTxCsmJobId(TxPduId),
        0,
        SecOCIntermediate.DataToAuth,
        SecOCIntermediate.DataToAuthLen,
        SecOCIntermediate.AuthenticatorPtr,
        &signatureLen
    );

    if((result == E_NOT_OK) || (result == E_BUSY) || (result == QUEUE_FULL))
    {
        (void)printf("ERROR: PQC signature generation failed\n");
        return result;
    }

    SecOCIntermediate.AuthenticatorLen = signatureLen;

    #ifdef SECOC_DEBUG
        (void)printf("PQC Signature generated: %u bytes (vs ~4 bytes for MAC)\n", signatureLen);
    #endif

    /* Build Secured PDU: HEADER(OPT) + AuthPdu + TruncatedFreshness(OPT) + Signature */
    PduLengthType SecPduLen = 0;

    /* Header */
    uint32 headerLen = SecOCTxPduProcessing[TxPduId].SecOCTxSecuredPduLayer->SecOCTxSecuredPdu->SecOCAuthPduHeaderLength;
    if(headerLen > 0U)
    {
        (void)memcpy((void*)&SecPdu->SduDataPtr[SecPduLen], (const void*)&AuthPdu->SduLength, headerLen);
        SecPduLen += headerLen;
    }

    /* Authentic PDU */
    (void)memcpy(&SecPdu->SduDataPtr[SecPduLen], AuthPdu->SduDataPtr, AuthPdu->SduLength);
    SecPduLen += AuthPdu->SduLength;

    /* Truncated Freshness Value */
    boolean IsFreshnessTruncated = (SecOCTxPduProcessing[TxPduId].SecOCProvideTxTruncatedFreshnessValue == TRUE) ? TRUE : FALSE;
    uint8 *MsgFreshness = (IsFreshnessTruncated) ? SecOCIntermediate.FreshnessTrunc : SecOCIntermediate.Freshness;
    uint32 FreshnesslenBytes = BIT_TO_BYTES(SecOCTxPduProcessing[TxPduId].SecOCFreshnessValueTruncLength);

    (void)memcpy(&SecPdu->SduDataPtr[SecPduLen], MsgFreshness, FreshnesslenBytes);
    SecPduLen += FreshnesslenBytes;

    /* Digital Signature (much larger than MAC!) */
    (void)memcpy(&SecPdu->SduDataPtr[SecPduLen], SecOCIntermediate.AuthenticatorPtr, SecOCIntermediate.AuthenticatorLen);
    SecPduLen += SecOCIntermediate.AuthenticatorLen;

    SecPdu->SduLength = SecPduLen;
    SecPdu->MetaDataPtr = AuthPdu->MetaDataPtr;

    /* Store authLen before clearing for debug output */
    uint32 authLen = SecPduLen - headerLen - FreshnesslenBytes - SecOCIntermediate.AuthenticatorLen;

    /* Clear Auth PDU */
    AuthPdu->SduLength = 0;

    #ifdef SECOC_DEBUG
        /* ===== PDU STRUCTURE VISUALIZATION (PQC ML-DSA-65 Mode) ===== */
        (void)printf("\n");
        (void)printf("╔════════════════════════════════════════════════════════════════════════════════════════════╗\n");
        (void)printf("║                      SECURED I-PDU STRUCTURE (TX) - PQC Mode (ML-DSA-65)                   ║\n");
        (void)printf("╠═══════════════╦═══════════════════════╦═════════════════╦══════════════════════════════════╣\n");
        (void)printf("║    Header     ║    Authentic PDU      ║   Freshness     ║  Authenticator (ML-DSA-65 Sig)   ║\n");
        (void)printf("║   %2u bytes    ║      %3u bytes        ║    %2u bytes     ║        %4u bytes                ║\n",
               headerLen, authLen, FreshnesslenBytes, SecOCIntermediate.AuthenticatorLen);
        (void)printf("╠═══════════════╬═══════════════════════╬═════════════════╬══════════════════════════════════╣\n");

        /* Print Header bytes */
        (void)printf("║ ");
        if(headerLen > 0U) {
            for(uint32 i = 0; i < headerLen && i < 4U; i++) { (void)printf("0x%02X ", SecPdu->SduDataPtr[i]); }
        } else {
            (void)printf("(none)       ");
        }
        (void)printf("║ ");

        /* Print first few AuthPdu bytes */
        uint32 authStart = headerLen;
        for(uint32 i = 0; i < authLen && i < 4U; i++) { (void)printf("0x%02X ", SecPdu->SduDataPtr[authStart + i]); }
        if(authLen > 4U) { (void)printf("..."); }
        (void)printf("    ║ ");

        /* Print Freshness bytes */
        uint32 freshStart = headerLen + authLen;
        for(uint32 i = 0; i < FreshnesslenBytes && i < 2U; i++) { (void)printf("0x%02X ", SecPdu->SduDataPtr[freshStart + i]); }
        (void)printf("        ║ ");

        /* Print first few Signature bytes */
        uint32 sigStart = freshStart + FreshnesslenBytes;
        for(uint32 i = 0; i < 4U; i++) { (void)printf("0x%02X ", SecPdu->SduDataPtr[sigStart + i]); }
        (void)printf("...   ║\n");

        (void)printf("╚═══════════════╩═══════════════════════╩═════════════════╩══════════════════════════════════╝\n");
        (void)printf("Total Secured PDU: %u bytes (Header=%u + Auth=%u + Fresh=%u + Signature=%u)\n\n",
               SecPduLen, headerLen, authLen, FreshnesslenBytes, SecOCIntermediate.AuthenticatorLen);
    #endif

    return result;
}

/**
 * @brief PQC-enabled verification using ML-DSA-65 digital signatures
 * @details Similar to verify() but uses Csm_SignatureVerify instead of Csm_MacVerify
 */
STATIC Std_ReturnType verify_PQC(PduIdType RxPduId, PduInfoType* SecPdu, SecOC_VerificationResultType *verification_result)
{
    #ifdef SECOC_DEBUG
        (void)printf("######## in verify_PQC (ML-DSA-65)\n");
    #endif

    SecOC_RxIntermediateType SecOCIntermediate;

    /* Parse secured PDU */
    parseSecuredPdu(RxPduId, SecPdu, &SecOCIntermediate);

    *verification_result = SECOC_NO_VERIFICATION;

    boolean SecOCSecuredRxPduVerification = TRUE;
    if((PdusCollections[RxPduId].Type == SECOC_AUTH_COLLECTON_PDU) ||
       (PdusCollections[RxPduId].Type == SECOC_CRYPTO_COLLECTON_PDU))
    {
        SecOCSecuredRxPduVerification = SecOCRxPduProcessing[RxPduId].SecOCRxSecuredPduLayer->SecOCRxSecuredPduCollection->SecOCSecuredRxPduVerification;
    }
    else
    {
        SecOCSecuredRxPduVerification = SecOCRxPduProcessing[RxPduId].SecOCRxSecuredPduLayer->SecOCRxSecuredPdu->SecOCSecuredRxPduVerification;
    }

    if(SecOCSecuredRxPduVerification == TRUE)
    {
        /* Check freshness result */
        if((SecOCIntermediate.freshnessResult == E_BUSY) || (SecOCIntermediate.freshnessResult == E_NOT_OK))
        {
            if(SecOCIntermediate.freshnessResult == E_NOT_OK)
            {
                *verification_result = SECOC_FRESHNESSFAILURE;
            }
            return SecOCIntermediate.freshnessResult;
        }

        /* Construct data to verify */
        constructDataToAuthenticatorRx(RxPduId, &SecOCIntermediate);

        Crypto_VerifyResultType verify_var;

        /* Signature verification (replaces MAC verification). */
#if (SECOC_USE_PQC_MODE == TRUE)
        uint32 signatureLen = (uint32)PQC_MLDSA_SIGNATURE_BYTES;
#else
        uint32 signatureLen = BIT_TO_BYTES(SecOCIntermediate.macLenBits);
#endif

        Std_ReturnType Sig_verify = Csm_SignatureVerify(
            SecOC_GetRxCsmJobId(RxPduId),
            CRYPTO_OPERATIONMODE_SINGLECALL,
            SecOCIntermediate.DataToAuth,
            SecOCIntermediate.DataToAuthLen,
            SecOCIntermediate.mac,  // Actually contains signature for PQC
            signatureLen,
            &verify_var
        );

        #ifdef SECOC_DEBUG
            (void)printf("PQC Signature verification: sig_len=%u bytes, result=%d\n", signatureLen, Sig_verify);
        #endif

        if((Sig_verify == E_BUSY) || (Sig_verify == QUEUE_FULL) || (Sig_verify == E_NOT_OK))
        {
            if(SecOC_RxCounters[RxPduId].AuthenticationCounter == SecOCRxPduProcessing[RxPduId].SecOCAuthenticationBuildAttempts)
            {
                *verification_result = SECOC_AUTHENTICATIONBUILDFAILURE;
            }
            if(Sig_verify == E_NOT_OK)
            {
                *verification_result = SECOC_VERIFICATIONFAILURE;
            }
            return Sig_verify;
        }
        else if((Sig_verify == CRYPTO_E_KEY_NOT_VALID) || (Sig_verify == CRYPTO_E_KEY_EMPTY))
        {
            if(SecOC_RxCounters[RxPduId].VerificationCounter == SecOCRxPduProcessing[RxPduId].SecOCAuthenticationVerifyAttempts)
            {
                *verification_result = SECOC_VERIFICATIONFAILURE;
            }
            return Sig_verify;
        }
        else
        {
            /* No action required */
        }
    }

    /* Verification success */
    *verification_result = SECOC_VERIFICATIONSUCCESS;

    PduInfoType *authPdu = &(SecOCRxPduProcessing[RxPduId].SecOCRxAuthenticPduLayer->SecOCRxAuthenticLayerPduRef);

    /* Copy authentic PDU */
    (void)memcpy(authPdu->SduDataPtr, SecOCIntermediate.authenticPdu, SecOCIntermediate.authenticPduLen);
    authPdu->SduLength = SecOCIntermediate.authenticPduLen;
    authPdu->MetaDataPtr = SecPdu->MetaDataPtr;

    SecPdu->SduLength = 0;

    /* Update freshness counter */
    (void)FVM_UpdateCounter(SecOCRxPduProcessing[RxPduId].SecOCFreshnessValueId, SecOCIntermediate.freshness, SecOCIntermediate.freshnessLenBits);

    return E_OK;
}


void SecOC_MainFunctionRx(void)
{
    #ifdef SECOC_DEBUG
        (void)printf("######## in SecOCMainFunctionRx \n");
    #endif
    /* [SWS_SecOC_00172] */
    if(SecOCState == SECOC_UNINIT)
    {
        return;
    }

    PduIdType idx = 0;
    SecOC_VerificationResultType result_ver;
    Std_ReturnType result;
    boolean SecOCAnyVerificationFailure = FALSE;
    boolean SecOCAnyVerificationSuccess = FALSE;
    for (idx = 0 ; idx < SECOC_NUM_OF_RX_PDU_PROCESSING; idx++) 
    {
        PduInfoType *authPdu = &(SecOCRxPduProcessing[idx].SecOCRxAuthenticPduLayer->SecOCRxAuthenticLayerPduRef);
        PduInfoType *securedPdu = &(SecOCRxPduProcessing[idx].SecOCRxSecuredPduLayer->SecOCRxSecuredPdu->SecOCRxSecuredLayerPduRef);
        boolean IsTpAuthenticPdu = (SecOCRxPduProcessing[idx].SecOCRxAuthenticPduLayer->SecOCPduType == SECOC_TPPDU) ? TRUE : FALSE;
        
        uint8 AuthHeadlen = SecOCRxPduProcessing[idx].SecOCRxSecuredPduLayer->SecOCRxSecuredPdu->SecOCAuthPduHeaderLength;
        PduLengthType authInfoLength;
#if (SECOC_USE_PQC_MODE == TRUE)
        authInfoLength = (PduLengthType)PQC_MLDSA_SIGNATURE_BYTES;
#else
        authInfoLength = BIT_TO_BYTES(SecOCRxPduProcessing[idx].SecOCAuthInfoTruncLength);
#endif
        PduLengthType securePduLength = AuthHeadlen + authRecieveLength[idx] + BIT_TO_BYTES(SecOCRxPduProcessing[idx].SecOCFreshnessValueTruncLength) + authInfoLength;

        /* Check if there is data */
        /* [SWS_SecOC_00174] */
        if ( securedPdu->SduLength >= securePduLength ) 
        {       
            /* [SWS_SecOC_00079] */
#if (SECOC_USE_PQC_MODE == TRUE)
            result = verify_PQC(idx, securedPdu, &result_ver);
#else
            result = verify(idx, securedPdu, &result_ver);
#endif
            if(result == E_OK)
            {
                SecOCAnyVerificationSuccess = TRUE;
                #ifdef SECOC_DEBUG
                (void)printf("Verify success for id: %d\n", idx);
                #endif

                if (IsTpAuthenticPdu == TRUE)
                {
                    PduLengthType RxBufferSize = 0U;
                    BufReq_ReturnType TpRxResult;

                    TpRxResult = PduR_SecOCTpStartOfReception(idx, authPdu, authPdu->SduLength, &RxBufferSize);
                    if (TpRxResult == BUFREQ_OK)
                    {
                        TpRxResult = PduR_SecOCTpCopyRxData(idx, authPdu, &RxBufferSize);
                    }

                    if (TpRxResult == BUFREQ_OK)
                    {
                        PduR_SecOCTpRxIndication(idx, E_OK);
                    }
                    else
                    {
                        PduR_SecOCTpRxIndication(idx, E_NOT_OK);
                    }
                }
                else
                {
                    /* [SWS_SecOC_00050], [SWS_SecOC_00080] */
                    PduR_SecOCIfRxIndication(idx,  authPdu);
                }
            }
            else if((result == E_BUSY) || (result == QUEUE_FULL))
            {
                /* [SWS_SecOC_00236], [SWS_SecOC_00237]*/
                SecOC_RxCounters[idx].AuthenticationCounter++;
            
                #ifdef COUNTERS_DEBUG
                (void)printf("SecOC_RxCounters[%d].AuthenticationCounter = %d\n",idx,SecOC_RxCounters[idx].AuthenticationCounter);
                (void)printf("SecOCRxPduProcessing[%d].SecOCAuthenticationBuildAttempts = %d\n",idx,SecOCRxPduProcessing[idx].SecOCAuthenticationBuildAttempts);
                #endif

                /* [SWS_SecOC_00240], [SWS_SecOC_00238], [SWS_SecOC_00151] */
                if( SecOC_RxCounters[idx].AuthenticationCounter >= SecOCRxPduProcessing[idx].SecOCAuthenticationBuildAttempts )
                {
                    securedPdu->SduLength = 0;
                    SecOCAnyVerificationFailure = TRUE;
                    if (IsTpAuthenticPdu == TRUE)
                    {
                        PduR_SecOCTpRxIndication(idx, E_NOT_OK);
                    }
                }

            }
            else if ( (result == CRYPTO_E_KEY_NOT_VALID) || (result == CRYPTO_E_KEY_EMPTY) )
            {
                /* [SWS_SecOC_00239] */
                SecOC_RxCounters[idx].AuthenticationCounter = 0; 
                SecOC_RxCounters[idx].VerificationCounter++;

                #ifdef COUNTERS_DEBUG
                (void)printf("SecOC_RxCounters[%d].VerificationCounter = %d\n",idx,SecOC_RxCounters[idx].VerificationCounter);
                (void)printf("SecOCRxPduProcessing[%d].SecOCAuthenticationVerifyAttempts = %d\n",idx,SecOCRxPduProcessing[idx].SecOCAuthenticationVerifyAttempts);
                #endif

                /* [SWS_SecOC_00241] */
                if( SecOC_RxCounters[idx].VerificationCounter >= SecOCRxPduProcessing[idx].SecOCAuthenticationVerifyAttempts )
                {
                    securedPdu->SduLength = 0;
                    SecOCAnyVerificationFailure = TRUE;
                    if (IsTpAuthenticPdu == TRUE)
                    {
                        PduR_SecOCTpRxIndication(idx, E_NOT_OK);
                    }
                }
            }
            else /* result == E_NOT_OK */
            {
                #ifdef SECOC_DEBUG
                (void)printf("Verify failed for id: %d\n", idx);
                (void)printf("Reason: %s\n", (result_ver == SECOC_FRESHNESSFAILURE) ? "FV" : "MAC");
                #endif
                /* [SWS_SecOC_00256] */
                securedPdu->SduLength = 0;
                SecOCAnyVerificationFailure = TRUE;
                if (IsTpAuthenticPdu == TRUE)
                {
                    PduR_SecOCTpRxIndication(idx, E_NOT_OK);
                }
            }
        }
    }

    if (SecOCAnyVerificationFailure == TRUE)
    {
        (void)BswM_RequestMode((uint16)SECOC_MODULE_ID, SECOC_BSWM_STATUS_FAILURE);
    }
    else if (SecOCAnyVerificationSuccess == TRUE)
    {
        (void)BswM_RequestMode((uint16)SECOC_MODULE_ID, SECOC_BSWM_STATUS_OK);
    }
    else
    {
        /* No action required */
    }

}



/********************************************************************************************************/
/******************************************Additional SWS APIs******************************************/
/********************************************************************************************************/

/* [SWS_SecOC_00113] */
Std_ReturnType SecOC_TpTransmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)
{
    #ifdef SECOC_DEBUG
        (void)printf("######## in SecOC_TpTransmit \n");
    #endif

    if(SecOCState != SECOC_INIT)
    {
        (void)Det_ReportError(SECOC_MODULE_ID, 0U, SECOC_SID_TP_TRANSMIT, SECOC_E_UNINIT);
        return E_NOT_OK;
    }

    if(PduInfoPtr == NULL)
    {
        (void)Det_ReportError(SECOC_MODULE_ID, 0U, SECOC_SID_TP_TRANSMIT, SECOC_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    if (SecOC_IsValidTxPduId(TxPduId) == FALSE)
    {
        return E_NOT_OK;
    }

    /* [SWS_SecOC_00252] Copy the Authentic I-PDU to internal memory */
    PduInfoType *authpdu = &(SecOCTxPduProcessing[TxPduId].SecOCTxAuthenticPduLayer->SecOCTxAuthenticLayerPduRef);

    (void)memcpy(authpdu->SduDataPtr, PduInfoPtr->SduDataPtr, PduInfoPtr->SduLength);
    authpdu->MetaDataPtr = PduInfoPtr->MetaDataPtr;
    authpdu->SduLength = PduInfoPtr->SduLength;

    SecOC_TxCounters[TxPduId].AuthenticationCounter = 0;

    return E_OK;
}


/* [SWS_SecOC_00119] */
Std_ReturnType SecOC_TpCancelTransmit(PduIdType TxPduId)
{
    #ifdef SECOC_DEBUG
        (void)printf("######## in SecOC_TpCancelTransmit \n");
    #endif

    if(SecOCState != SECOC_INIT)
    {
        return E_NOT_OK;
    }

    PduInfoType *authpdu = &(SecOCTxPduProcessing[TxPduId].SecOCTxAuthenticPduLayer->SecOCTxAuthenticLayerPduRef);
    PduInfoType *securedPdu = &(SecOCTxPduProcessing[TxPduId].SecOCTxSecuredPduLayer->SecOCTxSecuredPdu->SecOCTxSecuredLayerPduRef);

    authpdu->SduLength = 0;
    securedPdu->SduLength = 0;
    bufferRemainIndex[TxPduId] = 0;

    return E_OK;
}


/* [SWS_SecOC_00107] */
#if (SECOC_VERSION_INFO_API == STD_ON)
void SecOC_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    if(versioninfo == NULL)
    {
        (void)Det_ReportError(SECOC_MODULE_ID, 0U, SECOC_SID_GET_VERSION_INFO, SECOC_E_PARAM_POINTER);
        return;
    }

    versioninfo->vendorID = SECOC_VENDOR_ID;
    versioninfo->moduleID = SECOC_MODULE_ID;
    versioninfo->sw_major_version = SECOC_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = SECOC_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = SECOC_SW_PATCH_VERSION;
}
#endif


/* [SWS_SecOC_91008] Override verification status */
Std_ReturnType SecOC_VerifyStatusOverride(
    uint16 SecOCFreshnessValueID,
    uint8 overrideStatus,
    uint8 numberOfMessagesToOverride)
{
    #ifdef SECOC_DEBUG
        (void)printf("######## in SecOC_VerifyStatusOverride \n");
    #endif

    if(SecOCState != SECOC_INIT)
    {
        (void)Det_ReportError(SECOC_MODULE_ID, 0U, SECOC_SID_VERIFY_STATUS_OVERRIDE, SECOC_E_UNINIT);
        return E_NOT_OK;
    }

    /* [SWS_SecOC_91009] Override status values:
     * 0 = Cancel override (resume normal verification)
     * 1 = Pass override without performing verification (overrideStatus == SECOC_OVERRIDE_PASS)
     * 2 = Pass override and skip message authentication (overrideStatus == SECOC_OVERRIDE_SKIP)
     * 40/64 = Fail override (overrideStatus == SECOC_OVERRIDE_FAIL)
     */

    /* Store override in configuration - for now just return OK
     * Full implementation requires per-PDU override state tracking */
    (void)SecOCFreshnessValueID;
    (void)overrideStatus;
    (void)numberOfMessagesToOverride;

    return E_OK;
}


/********************************************************************************************************/
/*********************************************TestFunction***********************************************/
/********************************************************************************************************/

#ifdef DEBUG_ALL
/* cppcheck-suppress misra-c2012-8.4 */
void SecOC_test(void)
{
    SecOC_Init(&SecOC_Config);
    while (1)
    {
        #ifdef SECOC_DEBUG
            (void)printf("############### Starting Receive ###############\n");
        #endif
        #ifdef LINUX
        EthDrv_ReceiveMainFunction();
        #endif
        CanTp_MainFunctionRx();
        #ifdef LINUX
        SoAd_MainFunctionRx();
        #endif
        SecOC_MainFunctionRx();
    }

}
#endif


