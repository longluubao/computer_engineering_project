/********************************************************************************************************/
/*****************************************CanNmTests.cpp************************************************/
/********************************************************************************************************/
/**
 * @file CanNmTests.cpp
 * @brief Unit tests for CAN Network Management (CanNm) module
 */

#include <gtest/gtest.h>
#include <cstring>

extern "C" {
#include "CanNm/CanNm.h"
}

class CanNmTests : public ::testing::Test {
protected:
    void SetUp() override {
        CanNm_Init();
    }
    void TearDown() override {
        CanNm_DeInit();
    }
};

/* --- Initialization Tests --- */
TEST_F(CanNmTests, Init_NoCrash) {
    CanNm_DeInit();
    CanNm_Init();
    SUCCEED();
}

TEST_F(CanNmTests, DeInit_NoCrash) {
    CanNm_DeInit();
    SUCCEED();
}

/* --- Network Request/Release Tests --- */
TEST_F(CanNmTests, NetworkRequest_Valid) {
    Std_ReturnType result = CanNm_NetworkRequest(0U);
    EXPECT_EQ(result, E_OK);
}

TEST_F(CanNmTests, NetworkRelease_Valid) {
    CanNm_NetworkRequest(0U);
    Std_ReturnType result = CanNm_NetworkRelease(0U);
    EXPECT_EQ(result, E_OK);
}

/* --- GetState Tests --- */
TEST_F(CanNmTests, GetState_InitialBusSleep) {
    CanNm_NmStateType state;
    Std_ReturnType result = CanNm_GetState(0U, &state);
    EXPECT_EQ(result, E_OK);
    EXPECT_EQ(state, CANNM_STATE_BUS_SLEEP);
}

TEST_F(CanNmTests, GetState_AfterNetworkRequest) {
    CanNm_NetworkRequest(0U);
    CanNm_NmStateType state;
    CanNm_GetState(0U, &state);
    EXPECT_NE(state, CANNM_STATE_BUS_SLEEP);
}

TEST_F(CanNmTests, GetState_NullPtr) {
    Std_ReturnType result = CanNm_GetState(0U, NULL);
    EXPECT_NE(result, E_OK);
}

/* --- Callback Tests --- */
TEST_F(CanNmTests, TxConfirmation_NoCrash) {
    CanNm_TxConfirmation(CANNM_TX_PDU_ID, E_OK);
    SUCCEED();
}

TEST_F(CanNmTests, RxIndication_Valid) {
    uint8 data[CANNM_PDU_LENGTH] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    PduInfoType pduInfo;
    pduInfo.SduDataPtr = data;
    pduInfo.SduLength = CANNM_PDU_LENGTH;
    CanNm_RxIndication(CANNM_RX_PDU_ID, &pduInfo);
    SUCCEED();
}

TEST_F(CanNmTests, RxIndication_NullPtr) {
    CanNm_RxIndication(CANNM_RX_PDU_ID, NULL);
    SUCCEED();
}

/* --- MainFunction Tests --- */
TEST_F(CanNmTests, MainFunction_NoCrash) {
    CanNm_MainFunction();
    SUCCEED();
}

TEST_F(CanNmTests, MainFunction_MultipleCycles) {
    CanNm_NetworkRequest(0U);
    for (int i = 0; i < 200; i++) {
        CanNm_MainFunction();
    }
    SUCCEED();
}

TEST_F(CanNmTests, MainFunction_StateTransitions) {
    CanNm_NetworkRequest(0U);
    /* Run enough cycles to progress through repeat message state */
    for (int i = 0; i < CANNM_REPEAT_MESSAGE_TIME + 10; i++) {
        CanNm_MainFunction();
    }
    CanNm_NmStateType state;
    CanNm_GetState(0U, &state);
    /* Should have moved beyond repeat message */
    EXPECT_NE(state, CANNM_STATE_BUS_SLEEP);
}
