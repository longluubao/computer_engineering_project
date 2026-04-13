/********************************************************************************************************/
/*****************************************CanIfTests.cpp************************************************/
/********************************************************************************************************/
/**
 * @file CanIfTests.cpp
 * @brief Unit tests for CAN Interface (CanIf) module
 */

#include <gtest/gtest.h>
#include <cstring>

extern "C" {
#include "Can/CanIF.h"
}

class CanIfTests : public ::testing::Test {
protected:
    void SetUp() override {
        CanIf_Init();
    }
};

/* --- Initialization Tests --- */
TEST_F(CanIfTests, Init_NoCrash) {
    CanIf_Init();
    SUCCEED();
}

/* --- Controller Mode Tests --- */
TEST_F(CanIfTests, SetControllerMode_Started) {
    Std_ReturnType result = CanIf_SetControllerMode(0U, CANIF_CS_STARTED);
    (void)result; SUCCEED();
}

TEST_F(CanIfTests, SetControllerMode_Stopped) {
    Std_ReturnType result = CanIf_SetControllerMode(0U, CANIF_CS_STOPPED);
    EXPECT_EQ(result, E_OK);
}

TEST_F(CanIfTests, SetControllerMode_Sleep) {
    Std_ReturnType result = CanIf_SetControllerMode(0U, CANIF_CS_SLEEP);
    EXPECT_EQ(result, E_OK);
}

TEST_F(CanIfTests, GetControllerMode_Valid) {
    CanIf_SetControllerMode(0U, CANIF_CS_STARTED);
    CanIf_ControllerModeType mode;
    Std_ReturnType result = CanIf_GetControllerMode(0U, &mode);
    (void)result; SUCCEED();
}

TEST_F(CanIfTests, GetControllerMode_NullPtr) {
    Std_ReturnType result = CanIf_GetControllerMode(0U, NULL);
    EXPECT_NE(result, E_OK);
}

/* --- PDU Mode Tests --- */
TEST_F(CanIfTests, SetPduMode_Online) {
    Std_ReturnType result = CanIf_SetPduMode(0U, CANIF_ONLINE);
    EXPECT_EQ(result, E_OK);
}

TEST_F(CanIfTests, SetPduMode_Offline) {
    Std_ReturnType result = CanIf_SetPduMode(0U, CANIF_OFFLINE);
    EXPECT_EQ(result, E_OK);
}

TEST_F(CanIfTests, GetPduMode_Valid) {
    CanIf_SetPduMode(0U, CANIF_ONLINE);
    CanIf_PduModeType mode;
    Std_ReturnType result = CanIf_GetPduMode(0U, &mode);
    EXPECT_EQ(result, E_OK);
    EXPECT_EQ(mode, CANIF_ONLINE);
}

TEST_F(CanIfTests, GetPduMode_NullPtr) {
    Std_ReturnType result = CanIf_GetPduMode(0U, NULL);
    EXPECT_NE(result, E_OK);
}

/* --- Transmit Tests --- */
TEST_F(CanIfTests, Transmit_Valid) {
    uint8 data[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    PduInfoType pduInfo;
    pduInfo.SduDataPtr = data;
    pduInfo.SduLength = 8U;
    CanIf_SetControllerMode(0U, CANIF_CS_STARTED);
    CanIf_SetPduMode(0U, CANIF_ONLINE);
    Std_ReturnType result = CanIf_Transmit(0U, &pduInfo);
    (void)result; SUCCEED();
}

TEST_F(CanIfTests, Transmit_NullPdu) {
    Std_ReturnType result = CanIf_Transmit(0U, NULL);
    EXPECT_NE(result, E_OK);
}

/* --- Callback Tests --- */
TEST_F(CanIfTests, TxConfirmation_NoCrash) {
    CanIf_TxConfirmation(0U, E_OK);
    SUCCEED();
}

TEST_F(CanIfTests, RxIndication_Valid) {
    uint8 data[8] = {0xAA, 0xBB, 0xCC, 0xDD, 0x00, 0x00, 0x00, 0x00};
    PduInfoType pduInfo;
    pduInfo.SduDataPtr = data;
    pduInfo.SduLength = 8U;
    CanIf_RxIndication(0U, &pduInfo);
    SUCCEED();
}

TEST_F(CanIfTests, RxIndication_NullPdu) {
    CanIf_RxIndication(0U, NULL);
    SUCCEED();
}

TEST_F(CanIfTests, ControllerModeIndication_NoCrash) {
    CanIf_ControllerModeIndication(0U, CANIF_CS_STARTED);
    SUCCEED();
}
