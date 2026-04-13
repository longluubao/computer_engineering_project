/********************************************************************************************************/
/*****************************************DcmTests.cpp**************************************************/
/********************************************************************************************************/
/**
 * @file DcmTests.cpp
 * @brief Unit tests for Diagnostic Communication Manager (Dcm) module
 */

#include <gtest/gtest.h>
#include <cstring>

extern "C" {
#include "Dcm/Dcm.h"
#include "Dem/Dem.h"
}

class DcmTests : public ::testing::Test {
protected:
    void SetUp() override {
        Dem_Init();
        Dcm_Init(NULL);
    }
    void TearDown() override {
        Dcm_DeInit();
    }
};

/* --- Initialization Tests --- */
TEST_F(DcmTests, Init_NullConfig) {
    Dcm_DeInit();
    Dcm_Init(NULL);
    SUCCEED();
}

TEST_F(DcmTests, Init_WithConfig) {
    Dcm_ConfigType cfg;
    cfg.dummy = 0;
    Dcm_DeInit();
    Dcm_Init(&cfg);
    SUCCEED();
}

TEST_F(DcmTests, DeInit_NoCrash) {
    Dcm_DeInit();
    SUCCEED();
}

/* --- ProcessRequest Tests --- */
TEST_F(DcmTests, ProcessRequest_ClearDTC) {
    uint8 request[] = {DCM_UDS_SID_CLEAR_DTC, 0xFF, 0xFF, 0xFF};
    uint8 response[DCM_RESPONSE_BUFFER_SIZE];
    uint16 responseLength = 0U;
    Std_ReturnType result = Dcm_ProcessRequest(request, sizeof(request), response, &responseLength);
    EXPECT_EQ(result, E_OK);
    EXPECT_GT(responseLength, 0U);
}

TEST_F(DcmTests, ProcessRequest_ReadDTCByStatusMask) {
    /* First report some events */
    Dem_SetEventStatus(1U, DEM_EVENT_STATUS_FAILED);

    uint8 request[] = {DCM_UDS_SID_READ_DTC_INFORMATION,
                       DCM_UDS_SUBFUNC_REPORT_DTC_BY_STATUS_MASK,
                       DEM_DTC_STATUS_TEST_FAILED};
    uint8 response[DCM_RESPONSE_BUFFER_SIZE];
    uint16 responseLength = 0U;
    Std_ReturnType result = Dcm_ProcessRequest(request, sizeof(request), response, &responseLength);
    EXPECT_EQ(result, E_OK);
}

TEST_F(DcmTests, ProcessRequest_ReadNumberOfDTC) {
    uint8 request[] = {DCM_UDS_SID_READ_DTC_INFORMATION,
                       DCM_UDS_SUBFUNC_REPORT_NUMBER_OF_DTC_BY_STATUS,
                       0x01U};
    uint8 response[DCM_RESPONSE_BUFFER_SIZE];
    uint16 responseLength = 0U;
    Std_ReturnType result = Dcm_ProcessRequest(request, sizeof(request), response, &responseLength);
    EXPECT_EQ(result, E_OK);
}

TEST_F(DcmTests, ProcessRequest_UnsupportedService) {
    uint8 request[] = {0xFFU}; /* Unknown service */
    uint8 response[DCM_RESPONSE_BUFFER_SIZE];
    uint16 responseLength = 0U;
    Std_ReturnType result = Dcm_ProcessRequest(request, sizeof(request), response, &responseLength);
    EXPECT_EQ(result, E_OK);
    /* Should get NRC 0x11 (service not supported) */
    if (responseLength >= 3U) {
        EXPECT_EQ(response[0], 0x7FU); /* Negative response */
        EXPECT_EQ(response[2], DCM_NRC_SERVICE_NOT_SUPPORTED);
    }
}

TEST_F(DcmTests, ProcessRequest_NullRequest) {
    uint8 response[DCM_RESPONSE_BUFFER_SIZE];
    uint16 responseLength = 0U;
    Std_ReturnType result = Dcm_ProcessRequest(NULL, 4U, response, &responseLength);
    EXPECT_NE(result, E_OK);
}

TEST_F(DcmTests, ProcessRequest_NullResponse) {
    uint8 request[] = {0x14U, 0xFF, 0xFF, 0xFF};
    uint16 responseLength = 0U;
    Std_ReturnType result = Dcm_ProcessRequest(request, sizeof(request), NULL, &responseLength);
    EXPECT_NE(result, E_OK);
}

/* --- TP Callback Tests --- */
TEST_F(DcmTests, TpStartOfReception_NoCrash) {
    PduLengthType rxBufferSize = 0U;
    BufReq_ReturnType result = Dcm_TpStartOfReception(0U, NULL, 64U, &rxBufferSize);
    EXPECT_EQ(result, BUFREQ_OK);
}

TEST_F(DcmTests, TpCopyRxData_NoCrash) {
    PduLengthType rxBufferSize = 0U;
    Dcm_TpStartOfReception(0U, NULL, 64U, &rxBufferSize);
    uint8 data[8] = {0x14, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00};
    PduInfoType pduInfo;
    pduInfo.SduDataPtr = data;
    pduInfo.SduLength = 4U;
    PduLengthType remaining = 0U;
    Dcm_TpCopyRxData(0U, &pduInfo, &remaining);
    SUCCEED();
}

TEST_F(DcmTests, TpRxIndication_NoCrash) {
    Dcm_TpRxIndication(0U, E_OK);
    SUCCEED();
}

TEST_F(DcmTests, TpTxConfirmation_NoCrash) {
    Dcm_TpTxConfirmation(0U, E_OK);
    SUCCEED();
}

/* --- MainFunction Tests --- */
TEST_F(DcmTests, MainFunction_NoCrash) {
    Dcm_MainFunction();
    SUCCEED();
}
