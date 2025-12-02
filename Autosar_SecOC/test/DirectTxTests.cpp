#include <gtest/gtest.h>

extern "C" {

#include "SecOC_Lcfg.h"
#include "SecOC_Cfg.h"
#include "SecOC_PBcfg.h"
#include "SecOC_Cbk.h"
#include "ComStack_Types.h"
#include "Rte_SecOC.h"
#include "SecOC.h"
#include "PduR_SecOC.h"
#include "Csm.h"
#include "Rte_SecOC_Type.h"
#include "PduR_Com.h"
#include "PduR_SecOC.h"
#include "Pdur_CanTP.h"
#include "PduR_CanIf.h"
#include "SecOC_PQC_Cfg.h"

#include <string.h>
#include <stdio.h>

extern SecOC_ConfigType SecOC_Config;
extern const SecOC_TxPduProcessingType     *SecOCTxPduProcessing;
extern const SecOC_RxPduProcessingType     *SecOCRxPduProcessing;
extern const SecOC_GeneralType             *SecOCGeneral;

/* Classical mode functions */
extern Std_ReturnType authenticate(const PduIdType TxPduId, PduInfoType* authPdu, PduInfoType* SecPdu);

/* PQC mode functions */
extern Std_ReturnType authenticate_PQC(const PduIdType TxPduId, PduInfoType* authPdu, PduInfoType* SecPdu);

extern SecOC_TxCountersType         SecOC_TxCounters[SECOC_NUM_OF_TX_PDU_PROCESSING];

}


/**
 * @brief Direct Transmission Test via Ethernet (Dual Mode Support)
 *
 * This test validates Ethernet transmission and authentication flow:
 * 1. Application provides authentic PDU
 * 2. SecOC authenticates using MAC (Classical) or ML-DSA signature (PQC)
 * 3. Routes through PduR → SoAd → Ethernet
 * 4. Sends secured PDU via ethernet_send()
 *
 * DUAL MODE SUPPORT:
 * - Classical: Header(2) + Auth(2) + Fresh(1) + MAC(4) = ~9 bytes
 * - PQC: Header(1) + Auth(2) + Fresh(8) + Sig(~3309) = ~3320 bytes
 *
 * USES PDU ID 2 (SoAd/Ethernet), NOT ID 0 (CanIF)
 */
TEST(AuthenticationTests, directTx)
{
    printf("\n========================================\n");
    printf("DirectTx Test: Ethernet Transmission + Authentication\n");
#if SECOC_USE_PQC_MODE == TRUE
    printf("Mode: PQC (ML-DSA-65 Signature via Ethernet)\n");
#else
    printf("Mode: Classical (AES-CMAC via Ethernet)\n");
#endif
    printf("========================================\n");

    SecOC_Init(&SecOC_Config);

    /* Use PDU ID 2 - configured for SoAd (Ethernet) */
    PduIdType TxPduId = 2;
    printf("Using PDU ID %u (SoAd/Ethernet routing)\n", TxPduId);

    /* Application layer provides authentic PDU */
    uint8 test_meta_data = 0;
    PduInfoType sendPdu;
    uint8 authBuffer[100] = {100, 200};
    sendPdu.MetaDataPtr = &test_meta_data;
    sendPdu.SduDataPtr = authBuffer;
    sendPdu.SduLength = 2;

    printf("Application PDU: ");
    for(int i = 0; i < sendPdu.SduLength; i++)
        printf("%d ", sendPdu.SduDataPtr[i]);
    printf("\n");

    /* Transmit via COM layer */
    ASSERT_EQ(PduR_ComTransmit(TxPduId, &sendPdu), E_OK);

    /* Verify PDU was buffered */
    PduInfoType *authPduCheck = &(SecOCTxPduProcessing[TxPduId].SecOCTxAuthenticPduLayer->SecOCTxAuthenticLayerPduRef);
    ASSERT_EQ(memcmp(sendPdu.SduDataPtr, authPduCheck->SduDataPtr, sendPdu.SduLength), 0);

    /* Get PDU references */
    Std_ReturnType result;
    PduInfoType *authPdu = &(SecOCTxPduProcessing[TxPduId].SecOCTxAuthenticPduLayer->SecOCTxAuthenticLayerPduRef);
    PduInfoType *securedPdu = &(SecOCTxPduProcessing[TxPduId].SecOCTxSecuredPduLayer->SecOCTxSecuredPdu->SecOCTxSecuredLayerPduRef);

    /* Check if there is data */
    ASSERT_GT(authPdu->SduLength, 0);

    uint32 AuthPduLen = authPdu->SduLength;

    /* Authenticate based on mode */
#if SECOC_USE_PQC_MODE == TRUE
    printf("Generating ML-DSA-65 signature...\n");
    result = authenticate_PQC(TxPduId, authPdu, securedPdu);
#else
    printf("Generating AES-CMAC...\n");
    uint8 securedPduCheck[] = {1, 100, 200, 1, 196, 200, 222, 153};  // Header=1 for SoAd
    result = authenticate(TxPduId, authPdu, securedPdu);
#endif

    ASSERT_EQ(result, E_OK);
    ASSERT_EQ(authPdu->SduLength, 0);  // Auth PDU consumed

#if SECOC_USE_PQC_MODE == TRUE
    /* PQC mode: Expect large secured PDU */
    printf("Secured PDU length: %u bytes (with ML-DSA signature)\n", securedPdu->SduLength);
    ASSERT_GT(securedPdu->SduLength, 3000);  // Should be ~3320 bytes
#else
    /* Classical mode: Expect small secured PDU */
    printf("Secured PDU: ");
    for(int i = 0; i < securedPdu->SduLength; i++)
        printf("%d ", securedPdu->SduDataPtr[i]);
    printf("\n");
    ASSERT_EQ(memcmp(securedPdu->SduDataPtr, securedPduCheck, securedPdu->SduLength), 0);
#endif

    if(result == E_OK)
    {
        /* Increase freshness counter before transmission */
        FVM_IncreaseCounter(SecOCTxPduProcessing[TxPduId].SecOCFreshnessValueId);

        /* Transmit via PduR → SoAd → Ethernet */
        printf("Transmitting via PduR_SecOCTransmit → SoAd → Ethernet...\n");
        ASSERT_EQ(PduR_SecOCTransmit(TxPduId, securedPdu), E_OK);
        ASSERT_EQ(securedPdu->SduLength, 0);  // PDU transmitted

        printf("PASS: Ethernet transmission + %s authentication successful\n",
               SECOC_USE_PQC_MODE ? "PQC" : "Classical");
    }
    else if ((result == E_BUSY) || (result == QUEUE_FULL))
    {
        SecOC_TxCounters[TxPduId].AuthenticationCounter++;

        if(SecOC_TxCounters[TxPduId].AuthenticationCounter >= SecOCTxPduProcessing[TxPduId].SecOCAuthenticationBuildAttempts)
        {
            authPdu->SduLength = 0;
        }
    }
    else /* result == E_NOT_OK */
    {
        authPdu->SduLength = 0;
    }

    printf("========================================\n\n");
}
