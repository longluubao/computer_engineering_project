/********************************************************************************************************/
/*****************************************ComTests.cpp**************************************************/
/********************************************************************************************************/
/**
 * @file ComTests.cpp
 * @brief Unit tests for Communication (Com) module
 */

#include <gtest/gtest.h>
#include <cstring>

extern "C" {
#include "Com/Com.h"
}

class ComTests : public ::testing::Test {
protected:
    void SetUp() override {
        Com_Init();
    }
    void TearDown() override {
        Com_DeInit();
    }
};

/* --- Initialization Tests --- */
TEST_F(ComTests, Init_NoCrash) {
    Com_DeInit();
    Com_Init();
    SUCCEED();
}

TEST_F(ComTests, DeInit_NoCrash) {
    Com_DeInit();
    SUCCEED();
}

TEST_F(ComTests, DeInit_ThenReinit) {
    Com_DeInit();
    Com_Init();
    SUCCEED();
}

/* --- Signal Send/Receive Tests --- */
TEST_F(ComTests, SendSignal_Valid) {
    uint8 data[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    Std_ReturnType result = Com_SendSignal(0U, data);
    EXPECT_EQ(result, E_OK);
}

TEST_F(ComTests, SendSignal_NullPtr) {
    Std_ReturnType result = Com_SendSignal(0U, NULL);
    EXPECT_NE(result, E_OK);
}

TEST_F(ComTests, ReceiveSignal_Valid) {
    uint8 sendData[8] = {0xAA, 0xBB, 0xCC, 0xDD, 0x00, 0x00, 0x00, 0x00};
    uint8 recvData[8];
    Com_SendSignal(0U, sendData);
    Std_ReturnType result = Com_ReceiveSignal(0U, recvData);
    EXPECT_EQ(result, E_OK);
}

TEST_F(ComTests, ReceiveSignal_NullPtr) {
    Std_ReturnType result = Com_ReceiveSignal(0U, NULL);
    EXPECT_NE(result, E_OK);
}

/* --- Dynamic Signal Tests --- */
TEST_F(ComTests, SendDynSignal_Valid) {
    uint8 data[16] = {0};
    Std_ReturnType result = Com_SendDynSignal(0U, data, 16U);
    EXPECT_EQ(result, E_OK);
}

TEST_F(ComTests, SendDynSignal_NullPtr) {
    Std_ReturnType result = Com_SendDynSignal(0U, NULL, 8U);
    EXPECT_NE(result, E_OK);
}

TEST_F(ComTests, ReceiveDynSignal_Valid) {
    uint8 data[64];
    uint16 length = sizeof(data);
    Std_ReturnType result = Com_ReceiveDynSignal(0U, data, &length);
    EXPECT_EQ(result, E_OK);
}

/* --- InvalidateSignal Tests --- */
TEST_F(ComTests, InvalidateSignal) {
    Std_ReturnType result = Com_InvalidateSignal(0U);
    EXPECT_EQ(result, E_OK);
}

/* --- Signal Group Tests --- */
TEST_F(ComTests, SendSignalGroup) {
    Std_ReturnType result = Com_SendSignalGroup(0U);
    (void)result;
    SUCCEED();
}

TEST_F(ComTests, ReceiveSignalGroup) {
    Std_ReturnType result = Com_ReceiveSignalGroup(0U);
    (void)result;
    SUCCEED();
}

TEST_F(ComTests, UpdateShadowSignal) {
    uint8 data[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    Std_ReturnType result = Com_UpdateShadowSignal(0U, data);
    (void)result;
    SUCCEED();
}

TEST_F(ComTests, ReceiveShadowSignal) {
    uint8 data[8];
    Std_ReturnType result = Com_ReceiveShadowSignal(0U, data);
    (void)result;
    SUCCEED();
}

TEST_F(ComTests, InvalidateSignalGroup) {
    Std_ReturnType result = Com_InvalidateSignalGroup(0U);
    (void)result;
    SUCCEED();
}

TEST_F(ComTests, SendSignalGroupArray) {
    uint8 data[32] = {0};
    Std_ReturnType result = Com_SendSignalGroupArray(0U, data, 32U);
    (void)result;
    SUCCEED();
}

TEST_F(ComTests, ReceiveSignalGroupArray) {
    uint8 data[64];
    uint16 length = sizeof(data);
    Std_ReturnType result = Com_ReceiveSignalGroupArray(0U, data, &length);
    (void)result;
    SUCCEED();
}

/* --- IPdu Group Tests --- */
TEST_F(ComTests, IpduGroupStart) {
    Com_IpduGroupStart(0U, TRUE);
    SUCCEED();
}

TEST_F(ComTests, IpduGroupStop) {
    Com_IpduGroupStart(0U, TRUE);
    Com_IpduGroupStop(0U);
    SUCCEED();
}

/* --- Trigger IPdu Send --- */
TEST_F(ComTests, TriggerIPDUSend) {
    Std_ReturnType result = Com_TriggerIPDUSend(0U);
    (void)result;
    SUCCEED();
}

/* --- Indication/Confirmation Callbacks --- */
TEST_F(ComTests, TxConfirmation_NoCrash) {
    Com_TxConfirmation(0U, E_OK);
    SUCCEED();
}

TEST_F(ComTests, RxIndication_NoCrash) {
    uint8 data[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    PduInfoType pduInfo;
    pduInfo.SduDataPtr = data;
    pduInfo.SduLength = 8U;
    Com_RxIndication(0U, &pduInfo);
    SUCCEED();
}

TEST_F(ComTests, RxIndication_NullPdu) {
    Com_RxIndication(0U, NULL);
    SUCCEED();
}

/* --- TP Callbacks --- */
TEST_F(ComTests, StartOfReception_Valid) {
    PduLengthType rxBufferSize = 0U;
    BufReq_ReturnType result = Com_StartOfReception(0U, NULL, 64U, &rxBufferSize);
    (void)result;
    SUCCEED();
}

TEST_F(ComTests, CopyRxData_NoCrash) {
    PduLengthType rxBufferSize = 0U;
    Com_StartOfReception(0U, NULL, 64U, &rxBufferSize);
    uint8 data[8] = {0};
    PduInfoType pduInfo;
    pduInfo.SduDataPtr = data;
    pduInfo.SduLength = 8U;
    PduLengthType remaining = 0U;
    Com_CopyRxData(0U, &pduInfo, &remaining);
    SUCCEED();
}

TEST_F(ComTests, TpRxIndication_NoCrash) {
    Com_TpRxIndication(0U, E_OK);
    SUCCEED();
}

TEST_F(ComTests, TpTxConfirmation_NoCrash) {
    Com_TpTxConfirmation(0U, E_OK);
    SUCCEED();
}

/* --- MainFunction Tests --- */
TEST_F(ComTests, MainFunctionTx_NoCrash) {
    Com_MainFunctionTx();
    SUCCEED();
}

TEST_F(ComTests, MainFunctionRx_NoCrash) {
    Com_MainFunctionRx();
    SUCCEED();
}
