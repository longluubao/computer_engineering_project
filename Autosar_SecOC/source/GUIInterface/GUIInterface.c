/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/
#include <stdio.h>
#include "SecOC.h"
#include "SecOC_Lcfg.h"
#include "Std_Types.h"
#include "FVM.h"
#include "PduR_SecOC.h"
#include "PduR_CanIf.h"
#include "CanTP.h"
#include "SoAd.h"
#include "EcuM.h"

#ifdef LINUX
#include "ethernet.h"
#elif defined(WINDOWS)
#include "ethernet_windows.h"
#endif


/********************************************************************************************************/
/******************************************GlobalVaribles************************************************/
/********************************************************************************************************/
extern SecOC_ConfigType SecOC_Config;

extern const SecOC_TxPduProcessingType     *SecOCTxPduProcessing;
extern const SecOC_RxPduProcessingType     *SecOCRxPduProcessing;
extern const SecOC_GeneralType             *SecOCGeneral;
extern SecOC_TxCountersType         SecOC_TxCounters[SECOC_NUM_OF_TX_PDU_PROCESSING];
extern PduLengthType                authRecieveLength[SECOC_NUM_OF_RX_PDU_PROCESSING];
extern SecOC_PduCollection PdusCollections[];

extern Std_ReturnType authenticate(const PduIdType TxPduId, PduInfoType* AuthPdu, PduInfoType* SecPdu);
extern Std_ReturnType authenticate_PQC(const PduIdType TxPduId, PduInfoType* AuthPdu, PduInfoType* SecPdu);
extern Std_ReturnType verify(PduIdType RxPduId, PduInfoType* SecPdu, SecOC_VerificationResultType *verification_result);
extern Std_ReturnType verify_PQC(PduIdType RxPduId, PduInfoType* SecPdu, SecOC_VerificationResultType *verification_result);
extern Std_ReturnType seperatePduCollectionTx(const PduIdType TxPduId,uint32 AuthPduLen , PduInfoType* securedPdu, PduInfoType* AuthPduCollection, PduInfoType* CryptoPduCollection, PduIdType* authPduId, PduIdType* cryptoPduId);

/********************************************************************************************************/
/********************************************Functions***************************************************/
/********************************************************************************************************/

// DLL export macro for Windows
#ifdef WINDOWS
    #define DLL_EXPORT __declspec(dllexport)
#else
    #define DLL_EXPORT
#endif

static char* errorString(Std_ReturnType error)
{
    switch(error)
    {
        case E_OK:
            return "E_OK";
        break;
        case E_NOT_OK:
            return "E_NOT_OK";
        break;
        case E_BUSY:
            return "E_BUSY";
        break;
        case QUEUE_FULL:
            return "QUEUE_FULL";
        break;
    }
}

DLL_EXPORT void GUIInterface_init()
{
    EcuM_Init(NULL);
    (void)EcuM_StartupTwo();
}

DLL_EXPORT char* GUIInterface_authenticate(uint8_t configId, uint8_t *data, uint8_t len)
{

    PduInfoType *authPdu = &(SecOCTxPduProcessing[configId].SecOCTxAuthenticPduLayer->SecOCTxAuthenticLayerPduRef);
    PduInfoType *securedPdu = &(SecOCTxPduProcessing[configId].SecOCTxSecuredPduLayer->SecOCTxSecuredPdu->SecOCTxSecuredLayerPduRef);
    
    /* [SWS_SecOC_00226], [SWS_SecOC_00225] */
    SecOC_TxCounters[configId].AuthenticationCounter = 0;


    // Creating te Authentic PDU
    memcpy(authPdu->SduDataPtr, data, len);
    authPdu->SduLength = len;

    Std_ReturnType result;
    result = authenticate(configId , authPdu , securedPdu);
    authPdu->SduLength = len;

    return errorString(result);

}

DLL_EXPORT char* GUIInterface_verify(uint8_t configId)
{
    PduInfoType *authPdu = &(SecOCRxPduProcessing[configId].SecOCRxAuthenticPduLayer->SecOCRxAuthenticLayerPduRef);
    PduInfoType *securedPdu = &(SecOCRxPduProcessing[configId].SecOCRxSecuredPduLayer->SecOCRxSecuredPdu->SecOCRxSecuredLayerPduRef);

    SecOC_VerificationResultType result_ver;
    Std_ReturnType result;
    result = verify(configId, securedPdu, &result_ver);
    securedPdu->SduLength = 0; 

    if(result_ver == SECOC_FRESHNESSFAILURE)
        return "Freshness Failed";

    if((result_ver == SECOC_AUTHENTICATIONBUILDFAILURE) || (result_ver == SECOC_VERIFICATIONFAILURE))
        return "PDU is not Authentic";

    return errorString(result);

}

DLL_EXPORT char* GUIInterface_getSecuredPDU(uint8_t configId, uint8_t *len)
{
    PduInfoType *securedPdu = &(SecOCTxPduProcessing[configId].SecOCTxSecuredPduLayer->SecOCTxSecuredPdu->SecOCTxSecuredLayerPduRef);
    *len = securedPdu->SduLength;

    static char securedStr[100]; /* Static to be passed to the python program*/
    memset(securedStr, 0, sizeof(securedStr)); /* Clear buffer before use */

    uint8_t headerIdx = SecOCTxPduProcessing[configId].SecOCTxSecuredPduLayer->SecOCTxSecuredPdu->SecOCAuthPduHeaderLength;
    uint8_t authIdx = *len - BIT_TO_BYTES(SecOCTxPduProcessing[configId].SecOCAuthInfoTruncLength);
    uint8_t freshIdx = authIdx - BIT_TO_BYTES(SecOCTxPduProcessing[configId].SecOCFreshnessValueTruncLength);

    int i, stri;
    for(i = 0, stri = 0; i < *len; i++)
    {
        if((headerIdx != 0 && i == headerIdx) || (i == authIdx) || ((SecOCTxPduProcessing[configId].SecOCFreshnessValueTruncLength != 0) && i == freshIdx))
        {
            stri += sprintf(&securedStr[stri], "%s", " - ");
        }
        stri += sprintf(&securedStr[stri], "%x ", securedPdu->SduDataPtr[i]);
    }
    securedStr[stri] = '\0';

    *len = stri; /* Updated the length to match the created string */
    
    return securedStr;
}


DLL_EXPORT char* GUIInterface_getSecuredRxPDU(uint8_t configId, uint8_t *len , uint8_t * Securedlen)
{
    if(PdusCollections[configId].Type == SECOC_AUTH_COLLECTON_PDU || PdusCollections[configId].Type == SECOC_CRYPTO_COLLECTON_PDU)
    {
        configId = PdusCollections[configId].CollectionId;
    }
    PduInfoType *securedPdu = &(SecOCRxPduProcessing[configId].SecOCRxSecuredPduLayer->SecOCRxSecuredPdu->SecOCRxSecuredLayerPduRef);
    *len = securedPdu->SduLength;
    *Securedlen = securedPdu->SduLength;

    static char securedStr[100]; /* Static to be passed to the python program*/
    memset(securedStr, 0, sizeof(securedStr)); /* Clear buffer before use */

    uint8_t headerIdx = SecOCRxPduProcessing[configId].SecOCRxSecuredPduLayer->SecOCRxSecuredPdu->SecOCAuthPduHeaderLength;
    uint8_t authIdx = *len - BIT_TO_BYTES(SecOCRxPduProcessing[configId].SecOCAuthInfoTruncLength);
    uint8_t freshIdx = authIdx - BIT_TO_BYTES(SecOCRxPduProcessing[configId].SecOCFreshnessValueTruncLength);

    int i, stri;
    for(i = 0, stri = 0; i < *len; i++)
    {
        if((headerIdx != 0 && i == headerIdx) || (i == authIdx) || ((SecOCRxPduProcessing[configId].SecOCFreshnessValueTruncLength != 0) && i == freshIdx))
        {
            stri += sprintf(&securedStr[stri], "%s", " - ");
        }
        stri += sprintf(&securedStr[stri], "%x ", securedPdu->SduDataPtr[i]);
    }
    securedStr[stri] = '\0';

    *len = stri; /* Updated the length to match the created string */
    

    
    return securedStr;
}


DLL_EXPORT char* GUIInterface_getAuthPdu(uint8_t configId, uint8_t *len)
{
    Std_ReturnType result;
    int stri = 0;
    PduInfoType *authPdu = &(SecOCRxPduProcessing[configId].SecOCRxAuthenticPduLayer->SecOCRxAuthenticLayerPduRef);

    stri = authPdu->SduLength;
    *len = stri;

    return authPdu->SduDataPtr;
}

DLL_EXPORT void GUIInterface_alterFreshness(uint8_t configId)
{
    uint32 FreshnesslenBytes = BIT_TO_BYTES(SecOCTxPduProcessing[configId].SecOCFreshnessValueTruncLength);
    PduInfoType *securedPdu = &(SecOCTxPduProcessing[configId].SecOCTxSecuredPduLayer->SecOCTxSecuredPdu->SecOCTxSecuredLayerPduRef);

    if(FreshnesslenBytes == 0 || securedPdu->SduLength == 0)
    {
        return;
    }


    uint8_t macLen = BIT_TO_BYTES(SecOCTxPduProcessing[configId].SecOCAuthInfoTruncLength);

    /* Get the first freshness byte */
    uint8_t freshness_offset = securedPdu->SduLength - macLen - FreshnesslenBytes;

    printf("DEBUG alterFreshness: PDU len=%d, MAC len=%d, Freshness len=%d, offset=%d\n",
           securedPdu->SduLength, macLen, FreshnesslenBytes, freshness_offset);
    printf("Before: byte[%d] = %d\n", freshness_offset, securedPdu->SduDataPtr[freshness_offset]);

    securedPdu->SduDataPtr[freshness_offset]++;  // INCREMENT instead of decrement

    printf("After: byte[%d] = %d\n", freshness_offset, securedPdu->SduDataPtr[freshness_offset]);

}

DLL_EXPORT void GUIInterface_alterAuthenticator(uint8_t configId)
{
    PduInfoType *securedPdu = &(SecOCTxPduProcessing[configId].SecOCTxSecuredPduLayer->SecOCTxSecuredPdu->SecOCTxSecuredLayerPduRef);

    if(securedPdu->SduLength == 0)
    {
        return;
    }

    uint8_t mac_offset = securedPdu->SduLength - 1;
    printf("DEBUG alterAuthenticator: PDU len=%d, offset=%d (last byte)\n",
           securedPdu->SduLength, mac_offset);
    printf("Before: byte[%d] = %d\n", mac_offset, securedPdu->SduDataPtr[mac_offset]);

    securedPdu->SduDataPtr[securedPdu->SduLength - 1]++;

    printf("After: byte[%d] = %d\n", mac_offset, securedPdu->SduDataPtr[mac_offset]);

    
}

DLL_EXPORT char* GUIInterface_transmit(uint8_t configId)
{
    Std_ReturnType result;

    /* TO BE IMPLEMENTED*/
    PduInfoType *authPdu = &(SecOCTxPduProcessing[configId].SecOCTxAuthenticPduLayer->SecOCTxAuthenticLayerPduRef);
    PduInfoType *securedPdu = &(SecOCTxPduProcessing[configId].SecOCTxSecuredPduLayer->SecOCTxSecuredPdu->SecOCTxSecuredLayerPduRef);
    SecOC_TxSecuredPduCollectionType * securePduCollection = (SecOCTxPduProcessing[configId].SecOCTxSecuredPduLayer->SecOCTxSecuredPduCollection);
    PduInfoType *AuthPduCollection;
    PduInfoType *CryptoPduCollection;

    uint32 SecuredPduLen = securedPdu->SduLength;
    /* Check if there is data */
    if (authPdu->SduLength > 0)
    {
        uint32 AuthPduLen = authPdu->SduLength;

        PduIdType  authPduId , cryptoPduId;

        /* NOTE: Counter increment removed here because freshness is already embedded in the PDU
         * during GUIInterface_authenticate(). Incrementing again would cause the RX side to
         * see stale freshness values when attack simulations are performed. */
        /* FVM_IncreaseCounter(SecOCTxPduProcessing[configId].SecOCFreshnessValueId); */

        /* [SWS_SecOC_00201] */
        if(securePduCollection != NULL)
        {
            AuthPduCollection = &(SecOCTxPduProcessing[configId].SecOCTxSecuredPduLayer->SecOCTxSecuredPduCollection->SecOCTxAuthenticPdu->SecOCTxAuthenticPduRef);
            CryptoPduCollection = &(SecOCTxPduProcessing[configId].SecOCTxSecuredPduLayer->SecOCTxSecuredPduCollection->SecOCTxCryptographicPdu->SecOCTxCryptographicPduRef);
            seperatePduCollectionTx(configId, AuthPduLen , securedPdu , AuthPduCollection , CryptoPduCollection , &authPduId , &cryptoPduId);
            

            /* [SWS_SecOC_00062] */
            PduR_SecOCTransmit(authPduId , AuthPduCollection);
            PduR_SecOCTransmit(cryptoPduId , CryptoPduCollection);
        }
        else
        {
            /* [SWS_SecOC_00062] */
            PduR_SecOCTransmit(configId , securedPdu);
        }
        
    }

    CanTp_MainFunctionTx();
    SoAd_MainFunctionTx();
    securedPdu->SduLength = SecuredPduLen; // to reserve the length of auth for GUI

    return errorString(result);
}

DLL_EXPORT char* GUIInterface_receive(uint8_t* rxId , uint8_t* finalRxLen)
{
    Std_ReturnType result;

    /* TO BE IMPLEMENTED*/
    #if defined(__linux__) || defined(WINDOWS)
        static uint8 dataRecieve [BUS_LENGTH_RECEIVE];
        uint16 id;
        uint16 actualSize;
        result = ethernet_receive(dataRecieve , BUS_LENGTH_RECEIVE, &id, &actualSize);

        if (result != E_OK)
        {
            return errorString(result);
        }

        PduInfoType PduInfoPtr = {
            .SduDataPtr = dataRecieve,
            .MetaDataPtr = (uint8*)&PdusCollections[id],
            .SduLength = actualSize,  /* Use actual received size, not buffer size */
        };
        switch (PdusCollections[id].Type)
        {
        case SECOC_SECURED_PDU_CANIF:
            #ifdef ETHERNET_DEBUG
                printf("here in Direct \n");
            #endif
            PduR_CanIfRxIndication(id, &PduInfoPtr);
            break;
        case SECOC_SECURED_PDU_CANTP:
            #ifdef ETHERNET_DEBUG
                printf("here in CANTP \n");
            #endif
            CanTp_RxIndication(id, &PduInfoPtr);
            CanTp_MainFunctionRx();
            break;
        case SECOC_SECURED_PDU_SOADTP:
            #ifdef ETHERNET_DEBUG
                printf("here in Ethernet SOADTP \n");
            #endif
            SoAdTp_RxIndication(id, &PduInfoPtr);
            SoAd_MainFunctionRx();
            break;
        case SECOC_SECURED_PDU_SOADIF:
            #ifdef ETHERNET_DEBUG
                printf("here in Ethernet SOADIF \n");
            #endif
            PduR_SoAdIfRxIndication(id, &PduInfoPtr);
            break;

        case SECOC_AUTH_COLLECTON_PDU:
            #ifdef ETHERNET_DEBUG
                printf("here in Direct - pdu collection - auth\n");
            #endif
            PduR_CanIfRxIndication(id, &PduInfoPtr);
            break;
        case SECOC_CRYPTO_COLLECTON_PDU:
            #ifdef ETHERNET_DEBUG
                printf("here in Direct- pdu collection - crypto \n");
            #endif
            PduR_CanIfRxIndication(id, &PduInfoPtr);
            break;
        default:
            #ifdef ETHERNET_DEBUG
                printf("This is no type like it for ID : %d  type : %d \n", id, PdusCollections[id].Type);
            #endif
            break;
        }

        *rxId = (uint8_t) id;
        if(PdusCollections[id].Type == SECOC_AUTH_COLLECTON_PDU || PdusCollections[id].Type == SECOC_CRYPTO_COLLECTON_PDU)
        {
            *finalRxLen = 0;
        }
        else
        {
            uint8_t AuthHeadlen = SecOCRxPduProcessing[id].SecOCRxSecuredPduLayer->SecOCRxSecuredPdu->SecOCAuthPduHeaderLength;
            *finalRxLen = (uint8_t) AuthHeadlen + authRecieveLength[id] + BIT_TO_BYTES(SecOCRxPduProcessing[id].SecOCFreshnessValueTruncLength) + BIT_TO_BYTES(SecOCRxPduProcessing[id].SecOCAuthInfoTruncLength);
        }
    #endif

    return errorString(result);
}

/********************************************************************************************************/
/************************************PQC GUI INTERFACE FUNCTIONS*****************************************/
/********************************************************************************************************/

DLL_EXPORT char* GUIInterface_authenticate_PQC(uint8_t configId, uint8_t *data, uint8_t len)
{
    PduInfoType *authPdu = &(SecOCTxPduProcessing[configId].SecOCTxAuthenticPduLayer->SecOCTxAuthenticLayerPduRef);
    PduInfoType *securedPdu = &(SecOCTxPduProcessing[configId].SecOCTxSecuredPduLayer->SecOCTxSecuredPdu->SecOCTxSecuredLayerPduRef);

    /* Reset authentication counter */
    SecOC_TxCounters[configId].AuthenticationCounter = 0;

    /* Create Authentic PDU */
    memcpy(authPdu->SduDataPtr, data, len);
    authPdu->SduLength = len;

    /* Use PQC authentication (ML-DSA-65 signature) */
    Std_ReturnType result;
    result = authenticate_PQC(configId, authPdu, securedPdu);
    authPdu->SduLength = len;

    return errorString(result);
}

DLL_EXPORT char* GUIInterface_verify_PQC(uint8_t configId)
{
    PduInfoType *authPdu = &(SecOCRxPduProcessing[configId].SecOCRxAuthenticPduLayer->SecOCRxAuthenticLayerPduRef);
    PduInfoType *securedPdu = &(SecOCRxPduProcessing[configId].SecOCRxSecuredPduLayer->SecOCRxSecuredPdu->SecOCRxSecuredLayerPduRef);

    SecOC_VerificationResultType result_ver;
    Std_ReturnType result;

    /* Use PQC verification (ML-DSA-65 signature) */
    result = verify_PQC(configId, securedPdu, &result_ver);
    securedPdu->SduLength = 0;

    if(result_ver == SECOC_FRESHNESSFAILURE)
        return "Freshness Failed";

    if((result_ver == SECOC_AUTHENTICATIONBUILDFAILURE) || (result_ver == SECOC_VERIFICATIONFAILURE))
        return "PDU is not Authentic (PQC Sig Failed)";

    return errorString(result);
}
