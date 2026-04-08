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
#include "PduR_SoAd.h"
#include "SecOC_PQC_Cfg.h"

// Include appropriate Ethernet header based on platform
#ifdef LINUX
#include "ethernet.h"
#elif defined(WINDOWS) || defined(_WIN32)
#include "ethernet_windows.h"
#endif

#include <string.h>
#include <stdio.h>

extern SecOC_ConfigType SecOC_Config;
extern const SecOC_TxPduProcessingType     *SecOCTxPduProcessing;
extern const SecOC_RxPduProcessingType     *SecOCRxPduProcessing;
extern const SecOC_GeneralType             *SecOCGeneral;

/* Classical mode functions */
extern Std_ReturnType authenticate(const PduIdType TxPduId, PduInfoType* authPdu, PduInfoType* SecPdu);
extern Std_ReturnType verify(PduIdType RxPduId, PduInfoType* SecPdu, SecOC_VerificationResultType *verification_result);

/* PQC mode functions */
extern Std_ReturnType authenticate_PQC(const PduIdType TxPduId, PduInfoType* authPdu, PduInfoType* SecPdu);
extern Std_ReturnType verify_PQC(PduIdType RxPduId, PduInfoType* SecPdu, SecOC_VerificationResultType *verification_result);

extern SecOC_PduCollection PdusCollections[];
extern SecOC_TxCountersType         SecOC_TxCounters[SECOC_NUM_OF_TX_PDU_PROCESSING];
extern SecOC_RxCountersType         SecOC_RxCounters[SECOC_NUM_OF_RX_PDU_PROCESSING];

extern PduLengthType                authRecieveLength[SECOC_NUM_OF_RX_PDU_PROCESSING];

}


/**
 * @brief Direct Reception Test with Ethernet (Dual Mode Support)
 *
 * This test validates Ethernet reception and verification flow:
 * 1. Receives secured PDU via Ethernet (ethernet_receive)
 * 2. Routes through PduR to SecOC
 * 3. Verifies signature/MAC based on mode:
 *    - Classical Mode: Uses AES-CMAC (4 bytes)
 *    - PQC Mode: Uses ML-DSA-65 signature (~3309 bytes)
 * 4. Extracts authentic PDU
 *
 * DUAL MODE SUPPORT:
 * - Classical: Header(2) + Auth(2) + Fresh(1) + MAC(4) = 8 bytes
 * - PQC: Header(2) + Auth(2) + Fresh(8) + Sig(~3309) = ~3320 bytes
 */
// Enable DirectRx test on both Linux and Windows platforms
#if defined(__linux__) || defined(LINUX) || defined(WINDOWS) || defined(_WIN32)
TEST(AuthenticationTests, directRx)
{
    printf("\n========================================\n");
    printf("DirectRx Test: Ethernet Reception + Verification\n");
#if SECOC_USE_PQC_MODE == TRUE
    printf("Mode: PQC (ML-DSA-65 Signature via Ethernet)\n");
#else
    printf("Mode: Classical (AES-CMAC via Ethernet)\n");
#endif
    printf("========================================\n");

    SecOC_Init(&SecOC_Config);

    Std_ReturnType result;
    uint8 authPduCheck[] = {100, 200};

#if SECOC_USE_PQC_MODE == TRUE
    /* PQC Mode: Large buffer for ML-DSA signature */
    #define BUS_LENGTH_RECEIVE 4096
    static uint8 dataRecieve[BUS_LENGTH_RECEIVE];

    printf("Expected: Large secured PDU with ML-DSA-65 signature\n");
    printf("  Structure: [Header(2)] + [Auth(2)] + [Fresh(8)] + [Sig(~3309)]\n");
#else
    /* Classical Mode: Small buffer for MAC */
    #define BUS_LENGTH_RECEIVE 8
    static uint8 dataRecieve[BUS_LENGTH_RECEIVE];
    uint8 securedPduCheck[] = {2, 100, 200, 1, 196, 200, 222, 153};

    printf("Expected: Small secured PDU with AES-CMAC\n");
    printf("  Structure: [Header(2)] + [Auth(2)] + [Fresh(1)] + [MAC(4)]\n");
#endif

    /* Receive data from Ethernet */
    uint16 id;
    uint16 actualReceivedSize;
    printf("Receiving data via Ethernet...\n");
    EthDrv_Receive(dataRecieve, BUS_LENGTH_RECEIVE, &id, &actualReceivedSize);

#if SECOC_USE_PQC_MODE == FALSE
    /* Only verify exact match in Classical mode (PQC signatures vary) */
    ASSERT_EQ(memcmp(dataRecieve, securedPduCheck, BUS_LENGTH_RECEIVE), 0);
#endif

    /* PDU ID 2 is configured for SoAd (Ethernet) */
    ASSERT_EQ(id, 2);
    printf("Received PDU with ID: %u (SoAd Ethernet)\n", id);

    PduInfoType PduInfoPtr = {
        .SduDataPtr = dataRecieve,
        .MetaDataPtr = (uint8*) &PdusCollections[id],
        .SduLength = actualReceivedSize,  /* Use actual received size, not buffer size */
    };

    printf("Received %u bytes from Ethernet (PDU size)\n", actualReceivedSize);

    /* Ethernet uses SoAd (Socket Adapter), not CanIF */
    ASSERT_EQ(PdusCollections[id].Type, SECOC_SECURED_PDU_SOADIF);

    /* Route through SoAd → PduR → SecOC (correct AUTOSAR path) */
    printf("Routing PDU through PduR_SoAdIfRxIndication (Ethernet)...\n");
    PduR_SoAdIfRxIndication(id, &PduInfoPtr);

    /* Verification - Use PDU processing index 2 for Ethernet/SoAd */
    PduIdType idx = 2;
    SecOC_VerificationResultType result_ver;

    PduInfoType *authPdu = &(SecOCRxPduProcessing[idx].SecOCRxAuthenticPduLayer->SecOCRxAuthenticLayerPduRef);
    PduInfoType *securedPdu = &(SecOCRxPduProcessing[idx].SecOCRxSecuredPduLayer->SecOCRxSecuredPdu->SecOCRxSecuredLayerPduRef);

    uint8 AuthHeadlen = SecOCRxPduProcessing[idx].SecOCRxSecuredPduLayer->SecOCRxSecuredPdu->SecOCAuthPduHeaderLength;
    PduLengthType securePduLength = AuthHeadlen + authRecieveLength[idx] +
                                    BIT_TO_BYTES(SecOCRxPduProcessing[idx].SecOCFreshnessValueTruncLength) +
                                    BIT_TO_BYTES(SecOCRxPduProcessing[idx].SecOCAuthInfoTruncLength);

    ASSERT_GE(securedPdu->SduLength, securePduLength);

#if SECOC_USE_PQC_MODE == FALSE
    /* Only verify exact content in Classical mode */
    ASSERT_EQ(memcmp(securedPdu->SduDataPtr, securedPduCheck, securedPdu->SduLength), 0);
#else
    printf("Secured PDU length: %u bytes (includes ~3309 byte signature)\n", securedPdu->SduLength);
#endif

    /* Verify signature/MAC based on mode */
    printf("Verifying %s...\n", SECOC_USE_PQC_MODE ? "ML-DSA-65 signature" : "AES-CMAC");

#if SECOC_USE_PQC_MODE == TRUE
    result = verify_PQC(idx, securedPdu, &result_ver);
#else
    result = verify(idx, securedPdu, &result_ver);
#endif

    ASSERT_EQ(result_ver, E_OK);
    ASSERT_EQ(result, E_OK);
    printf("Verification: SUCCESS\n");

    ASSERT_EQ(securedPdu->SduLength, 0);
    ASSERT_GT(authPdu->SduLength, 0);

    /* Verify extracted authentic PDU */
    ASSERT_EQ(memcmp(authPdu->SduDataPtr, authPduCheck, authPdu->SduLength), 0);
    printf("Authentic PDU extracted: ");
    for(uint8 i = 0; i < authPdu->SduLength; i++)
        printf("%d ", authPdu->SduDataPtr[i]);
    printf("\n");

    /* Forward to application layer */
    PduR_SecOCIfRxIndication(idx, authPdu);

    printf("PASS: Ethernet reception + %s verification successful\n",
           SECOC_USE_PQC_MODE ? "PQC" : "Classical");
    printf("========================================\n\n");
}
#endif
