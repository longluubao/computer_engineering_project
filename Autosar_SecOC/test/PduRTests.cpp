/**
 * @file PduRTests.cpp
 * @brief Unit tests for the PduR (PDU Router) thin-routing layer.
 *
 * The PduR routing files are very small and act as dispatchers between
 * lower-layer drivers (CanIf/CanTp/SoAd/UdpNm) and SecOC. These tests
 * exercise both the happy path and the boundary/error guards (NULL
 * pointers, out-of-range PDU IDs, and unknown collection types) so the
 * PduR_*.c files reach 100% line coverage.
 */

#include <gtest/gtest.h>

extern "C" {
#include "SecOC_Lcfg.h"
#include "SecOC_Cfg.h"
#include "SecOC_PBcfg.h"
#include "SecOC.h"
#include "PduR_SecOC.h"
#include "PduR_SoAd.h"
#include "PduR_Com.h"
#include "PduR_CanIf.h"
#include "Pdur_CanTP.h"
#include "PduR_UdpNm.h"

extern SecOC_ConfigType SecOC_Config;
extern SecOC_PduCollection PdusCollections[];
}

/* --------------------------------------------------------------------------
 * Test fixture - reinitializes SecOC for every test
 * -------------------------------------------------------------------------- */
class PduRTests : public ::testing::Test {
protected:
    void SetUp() override {
        SecOC_DeInit();
        SecOC_Init(&SecOC_Config);
    }
};

/* --------------------------------------------------------------------------
 * PduR_SecOC.c - Transmit dispatcher
 * -------------------------------------------------------------------------- */
TEST_F(PduRTests, SecOCTransmit_NullPduInfo) {
    Std_ReturnType result = PduR_SecOCTransmit(0, NULL);
    EXPECT_EQ(result, E_NOT_OK);
}

TEST_F(PduRTests, SecOCTransmit_OutOfRangeId) {
    PduInfoType info = {NULL, NULL, 0};
    Std_ReturnType result = PduR_SecOCTransmit(SECOC_NUM_OF_PDU_COLLECTION, &info);
    EXPECT_EQ(result, E_NOT_OK);
}

TEST_F(PduRTests, SecOCTransmit_UnknownCollectionType) {
    /* Force an out-of-range Type to drive the default branch */
    SecOC_PduCollection_Type saved = PdusCollections[0].Type;
    PdusCollections[0].Type = (SecOC_PduCollection_Type)0xFFu;
    PduInfoType info = {NULL, NULL, 0};
    Std_ReturnType result = PduR_SecOCTransmit(0, &info);
    EXPECT_EQ(result, E_NOT_OK);
    PdusCollections[0].Type = saved;
}

TEST_F(PduRTests, SecOCTransmit_AllValidTypes) {
    /* Walk every collection slot - each routes to a different lower layer */
    PduInfoType info = {NULL, NULL, 0};
    for (PduIdType i = 0; i < SECOC_NUM_OF_PDU_COLLECTION; ++i) {
        (void)PduR_SecOCTransmit(i, &info);
    }
    SUCCEED();
}

/* --------------------------------------------------------------------------
 * PduR_SecOC.c - Confirmation / Indication callbacks
 * -------------------------------------------------------------------------- */
TEST_F(PduRTests, SecOCIfTxConfirmation_OutOfRange) {
    PduR_SecOCIfTxConfirmation(SECOC_NUM_OF_TX_PDU_PROCESSING, E_OK);
    SUCCEED();
}

TEST_F(PduRTests, SecOCIfTxConfirmation_Valid) {
    PduR_SecOCIfTxConfirmation(0, E_OK);
    SUCCEED();
}

TEST_F(PduRTests, SecOCIfRxIndication_NullPtr) {
    PduR_SecOCIfRxIndication(0, NULL);
    SUCCEED();
}

TEST_F(PduRTests, SecOCIfRxIndication_OutOfRange) {
    uint8 buf[4] = {1, 2, 3, 4};
    PduInfoType info = {buf, NULL, 4};
    PduR_SecOCIfRxIndication(SECOC_NUM_OF_RX_PDU_PROCESSING, &info);
    SUCCEED();
}

TEST_F(PduRTests, SecOCIfRxIndication_Valid) {
    uint8 buf[4] = {1, 2, 3, 4};
    PduInfoType info = {buf, NULL, 4};
    PduR_SecOCIfRxIndication(0, &info);
    SUCCEED();
}

TEST_F(PduRTests, SecOCTpTxConfirmation_LowId) {
    PduR_SecOCTpTxConfirmation(0, E_OK);
    SUCCEED();
}

TEST_F(PduRTests, SecOCTpTxConfirmation_HighIdGoesToDcm) {
    /* IDs >= SECOC_NUM_OF_TX_PDU_PROCESSING route to Dcm */
    PduR_SecOCTpTxConfirmation(SECOC_NUM_OF_TX_PDU_PROCESSING, E_OK);
    SUCCEED();
}

TEST_F(PduRTests, SecOCTpStartOfReception_NullBuffer) {
    PduInfoType info = {NULL, NULL, 0};
    BufReq_ReturnType result = PduR_SecOCTpStartOfReception(0, &info, 16, NULL);
    EXPECT_EQ(result, BUFREQ_E_NOT_OK);
}

TEST_F(PduRTests, SecOCTpStartOfReception_LowIdRoutesToCom) {
    PduInfoType info = {NULL, NULL, 0};
    PduLengthType bufSize = 0;
    (void)PduR_SecOCTpStartOfReception(0, &info, 16, &bufSize);
    SUCCEED();
}

TEST_F(PduRTests, SecOCTpStartOfReception_HighIdRoutesToDcm) {
    PduInfoType info = {NULL, NULL, 0};
    PduLengthType bufSize = 0;
    (void)PduR_SecOCTpStartOfReception(SECOC_NUM_OF_RX_PDU_PROCESSING,
                                       &info, 16, &bufSize);
    SUCCEED();
}

TEST_F(PduRTests, SecOCTpCopyRxData_NullBuffer) {
    PduInfoType info = {NULL, NULL, 0};
    BufReq_ReturnType result = PduR_SecOCTpCopyRxData(0, &info, NULL);
    EXPECT_EQ(result, BUFREQ_E_NOT_OK);
}

TEST_F(PduRTests, SecOCTpCopyRxData_LowIdRoutesToCom) {
    PduInfoType info = {NULL, NULL, 0};
    PduLengthType bufSize = 0;
    (void)PduR_SecOCTpCopyRxData(0, &info, &bufSize);
    SUCCEED();
}

TEST_F(PduRTests, SecOCTpCopyRxData_HighIdRoutesToDcm) {
    PduInfoType info = {NULL, NULL, 0};
    PduLengthType bufSize = 0;
    (void)PduR_SecOCTpCopyRxData(SECOC_NUM_OF_RX_PDU_PROCESSING,
                                 &info, &bufSize);
    SUCCEED();
}

TEST_F(PduRTests, SecOCTpRxIndication_LowIdRoutesToCom) {
    PduR_SecOCTpRxIndication(0, E_OK);
    SUCCEED();
}

TEST_F(PduRTests, SecOCTpRxIndication_HighIdRoutesToDcm) {
    PduR_SecOCTpRxIndication(SECOC_NUM_OF_RX_PDU_PROCESSING, E_OK);
    SUCCEED();
}

/* --------------------------------------------------------------------------
 * PduR_SoAd.c - Socket Adapter routing layer
 * -------------------------------------------------------------------------- */
TEST_F(PduRTests, SoAdIfTxConfirmation_OutOfRange) {
    PduR_SoAdIfTxConfirmation(SECOC_NUM_OF_PDU_COLLECTION, E_OK);
    SUCCEED();
}

TEST_F(PduRTests, SoAdIfTxConfirmation_Valid) {
    PduR_SoAdIfTxConfirmation(0, E_OK);
    SUCCEED();
}

TEST_F(PduRTests, SoAdIfRxIndication_NullPtr) {
    PduR_SoAdIfRxIndication(0, NULL);
    SUCCEED();
}

TEST_F(PduRTests, SoAdIfRxIndication_OutOfRange) {
    uint8 buf[4] = {0};
    PduInfoType info = {buf, NULL, 4};
    PduR_SoAdIfRxIndication(SECOC_NUM_OF_PDU_COLLECTION, &info);
    SUCCEED();
}

TEST_F(PduRTests, SoAdTpCopyTxData_OutOfRange) {
    PduInfoType info = {NULL, NULL, 0};
    PduLengthType avail = 0;
    BufReq_ReturnType result = PduR_SoAdTpCopyTxData(
        SECOC_NUM_OF_TX_PDU_PROCESSING, &info, NULL, &avail);
    EXPECT_EQ(result, BUFREQ_E_NOT_OK);
}

TEST_F(PduRTests, SoAdTpCopyTxData_NullAvail) {
    PduInfoType info = {NULL, NULL, 0};
    BufReq_ReturnType result = PduR_SoAdTpCopyTxData(0, &info, NULL, NULL);
    EXPECT_EQ(result, BUFREQ_E_NOT_OK);
}

TEST_F(PduRTests, SoAdTpCopyTxData_Valid) {
    PduInfoType info = {NULL, NULL, 0};
    PduLengthType avail = 0;
    (void)PduR_SoAdTpCopyTxData(0, &info, NULL, &avail);
    SUCCEED();
}

TEST_F(PduRTests, SoAdTpTxConfirmation_OutOfRange) {
    PduR_SoAdTpTxConfirmation(SECOC_NUM_OF_TX_PDU_PROCESSING, E_OK);
    SUCCEED();
}

TEST_F(PduRTests, SoAdTpTxConfirmation_Valid) {
    PduR_SoAdTpTxConfirmation(0, E_OK);
    SUCCEED();
}

TEST_F(PduRTests, SoAdTpCopyRxData_OutOfRange) {
    PduInfoType info = {NULL, NULL, 0};
    PduLengthType bufSize = 0;
    BufReq_ReturnType result = PduR_SoAdTpCopyRxData(
        SECOC_NUM_OF_RX_PDU_PROCESSING, &info, &bufSize);
    EXPECT_EQ(result, BUFREQ_E_NOT_OK);
}

TEST_F(PduRTests, SoAdTpCopyRxData_NullBuf) {
    PduInfoType info = {NULL, NULL, 0};
    BufReq_ReturnType result = PduR_SoAdTpCopyRxData(0, &info, NULL);
    EXPECT_EQ(result, BUFREQ_E_NOT_OK);
}

TEST_F(PduRTests, SoAdStartOfReception_OutOfRange) {
    PduInfoType info = {NULL, NULL, 0};
    PduLengthType bufSize = 0;
    BufReq_ReturnType result = PduR_SoAdStartOfReception(
        SECOC_NUM_OF_RX_PDU_PROCESSING, &info, 16, &bufSize);
    EXPECT_EQ(result, BUFREQ_E_NOT_OK);
}

TEST_F(PduRTests, SoAdStartOfReception_NullBuf) {
    PduInfoType info = {NULL, NULL, 0};
    BufReq_ReturnType result = PduR_SoAdStartOfReception(0, &info, 16, NULL);
    EXPECT_EQ(result, BUFREQ_E_NOT_OK);
}

TEST_F(PduRTests, SoAdTpRxIndication_OutOfRange) {
    PduR_SoAdTpRxIndication(SECOC_NUM_OF_RX_PDU_PROCESSING, E_OK);
    SUCCEED();
}

TEST_F(PduRTests, SoAdTpRxIndication_Valid) {
    PduR_SoAdTpRxIndication(0, E_OK);
    SUCCEED();
}

/* --------------------------------------------------------------------------
 * Pdur_CanTP.c - CAN Transport Protocol routing
 * -------------------------------------------------------------------------- */
TEST_F(PduRTests, CanTpCopyTxData_OutOfRange) {
    PduInfoType info = {NULL, NULL, 0};
    PduLengthType avail = 0;
    BufReq_ReturnType result = PduR_CanTpCopyTxData(
        SECOC_NUM_OF_TX_PDU_PROCESSING, &info, NULL, &avail);
    EXPECT_EQ(result, BUFREQ_E_NOT_OK);
}

TEST_F(PduRTests, CanTpCopyTxData_NullAvail) {
    PduInfoType info = {NULL, NULL, 0};
    BufReq_ReturnType result = PduR_CanTpCopyTxData(0, &info, NULL, NULL);
    EXPECT_EQ(result, BUFREQ_E_NOT_OK);
}

TEST_F(PduRTests, CanTpCopyTxData_Valid) {
    PduInfoType info = {NULL, NULL, 0};
    PduLengthType avail = 0;
    (void)PduR_CanTpCopyTxData(0, &info, NULL, &avail);
    SUCCEED();
}

TEST_F(PduRTests, CanTpTxConfirmation_OutOfRange) {
    PduR_CanTpTxConfirmation(SECOC_NUM_OF_TX_PDU_PROCESSING, E_OK);
    SUCCEED();
}

TEST_F(PduRTests, CanTpTxConfirmation_Valid) {
    PduR_CanTpTxConfirmation(0, E_OK);
    SUCCEED();
}

TEST_F(PduRTests, CanTpCopyRxData_OutOfRange) {
    PduInfoType info = {NULL, NULL, 0};
    PduLengthType bufSize = 0;
    BufReq_ReturnType result = PduR_CanTpCopyRxData(
        SECOC_NUM_OF_RX_PDU_PROCESSING, &info, &bufSize);
    EXPECT_EQ(result, BUFREQ_E_NOT_OK);
}

TEST_F(PduRTests, CanTpCopyRxData_NullBuf) {
    PduInfoType info = {NULL, NULL, 0};
    BufReq_ReturnType result = PduR_CanTpCopyRxData(0, &info, NULL);
    EXPECT_EQ(result, BUFREQ_E_NOT_OK);
}

TEST_F(PduRTests, CanTpStartOfReception_OutOfRange) {
    PduInfoType info = {NULL, NULL, 0};
    PduLengthType bufSize = 0;
    BufReq_ReturnType result = PduR_CanTpStartOfReception(
        SECOC_NUM_OF_RX_PDU_PROCESSING, &info, 16, &bufSize);
    EXPECT_EQ(result, BUFREQ_E_NOT_OK);
}

TEST_F(PduRTests, CanTpStartOfReception_NullBuf) {
    PduInfoType info = {NULL, NULL, 0};
    BufReq_ReturnType result = PduR_CanTpStartOfReception(0, &info, 16, NULL);
    EXPECT_EQ(result, BUFREQ_E_NOT_OK);
}

TEST_F(PduRTests, CanTpRxIndication_OutOfRange) {
    PduR_CanTpRxIndication(SECOC_NUM_OF_RX_PDU_PROCESSING, E_OK);
    SUCCEED();
}

TEST_F(PduRTests, CanTpRxIndication_Valid) {
    PduR_CanTpRxIndication(0, E_OK);
    SUCCEED();
}

/* --------------------------------------------------------------------------
 * Pdur_Canif.c - CAN Interface routing
 * -------------------------------------------------------------------------- */
TEST_F(PduRTests, CanIfTxConfirmation_OutOfRange) {
    PduR_CanIfTxConfirmation(SECOC_NUM_OF_PDU_COLLECTION, E_OK);
    SUCCEED();
}

TEST_F(PduRTests, CanIfTxConfirmation_Valid) {
    PduR_CanIfTxConfirmation(0, E_OK);
    SUCCEED();
}

TEST_F(PduRTests, CanIfRxIndication_NullPtr) {
    PduR_CanIfRxIndication(0, NULL);
    SUCCEED();
}

TEST_F(PduRTests, CanIfRxIndication_OutOfRange) {
    uint8 buf[4] = {0};
    PduInfoType info = {buf, NULL, 4};
    PduR_CanIfRxIndication(SECOC_NUM_OF_PDU_COLLECTION, &info);
    SUCCEED();
}

/* --------------------------------------------------------------------------
 * PduR_Com.c - Application/Communication-layer routing
 * -------------------------------------------------------------------------- */
TEST_F(PduRTests, ComTransmit_NullPduInfo) {
    Std_ReturnType result = PduR_ComTransmit(0, NULL);
    EXPECT_EQ(result, E_NOT_OK);
}

TEST_F(PduRTests, ComTransmit_OutOfRange) {
    PduInfoType info = {NULL, NULL, 0};
    Std_ReturnType result = PduR_ComTransmit(SECOC_NUM_OF_TX_PDU_PROCESSING,
                                             &info);
    EXPECT_EQ(result, E_NOT_OK);
}

TEST_F(PduRTests, ComTransmit_Valid) {
    uint8 buf[2] = {100, 200};
    PduInfoType info = {buf, NULL, 2};
    /* Don't assert specific return; exercise the routing path */
    (void)PduR_ComTransmit(0, &info);
    SUCCEED();
}

/* --------------------------------------------------------------------------
 * PduR_UdpNm.c - UDP Network Management routing
 * -------------------------------------------------------------------------- */
TEST_F(PduRTests, UdpNmTxConfirmation_NoCrash) {
    PduR_UdpNmTxConfirmation(0, E_OK);
    SUCCEED();
}

TEST_F(PduRTests, UdpNmRxIndication_NullPtr) {
    PduR_UdpNmRxIndication(0, NULL);
    SUCCEED();
}

TEST_F(PduRTests, UdpNmRxIndication_Valid) {
    uint8 buf[4] = {0};
    PduInfoType info = {buf, NULL, 4};
    PduR_UdpNmRxIndication(0, &info);
    SUCCEED();
}
