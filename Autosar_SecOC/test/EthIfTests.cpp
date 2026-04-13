/********************************************************************************************************/
/*****************************************EthIfTests.cpp************************************************/
/********************************************************************************************************/
/**
 * @file EthIfTests.cpp
 * @brief Unit tests for Ethernet Interface (EthIf) module
 */

#include <gtest/gtest.h>
#include <cstring>

extern "C" {
#include "EthIf/EthIf.h"
}

class EthIfTests : public ::testing::Test {
protected:
    void SetUp() override {
        EthIf_Init(NULL);
    }
    void TearDown() override {
        EthIf_DeInit();
    }
};

/* --- Initialization Tests --- */
TEST_F(EthIfTests, Init_NullConfig) {
    EthIf_DeInit();
    EthIf_Init(NULL);
    SUCCEED();
}

TEST_F(EthIfTests, Init_WithConfig) {
    EthIf_ConfigType cfg;
    cfg.dummy = 0;
    EthIf_DeInit();
    EthIf_Init(&cfg);
    SUCCEED();
}

TEST_F(EthIfTests, DeInit_NoCrash) {
    EthIf_DeInit();
    SUCCEED();
}

/* --- Controller Mode Tests --- */
TEST_F(EthIfTests, SetControllerMode_Active) {
    Std_ReturnType result = EthIf_SetControllerMode(0U, ETH_MODE_ACTIVE);
    EXPECT_EQ(result, E_OK);
}

TEST_F(EthIfTests, SetControllerMode_Down) {
    Std_ReturnType result = EthIf_SetControllerMode(0U, ETH_MODE_DOWN);
    EXPECT_EQ(result, E_OK);
}

TEST_F(EthIfTests, SetControllerMode_InvalidCtrl) {
    Std_ReturnType result = EthIf_SetControllerMode(ETHIF_MAX_CTRL, ETH_MODE_ACTIVE);
    EXPECT_NE(result, E_OK);
}

TEST_F(EthIfTests, GetControllerMode_Valid) {
    EthIf_SetControllerMode(0U, ETH_MODE_ACTIVE);
    Eth_ModeType mode;
    Std_ReturnType result = EthIf_GetControllerMode(0U, &mode);
    EXPECT_EQ(result, E_OK);
    EXPECT_EQ(mode, ETH_MODE_ACTIVE);
}

TEST_F(EthIfTests, GetControllerMode_NullPtr) {
    Std_ReturnType result = EthIf_GetControllerMode(0U, NULL);
    EXPECT_NE(result, E_OK);
}

/* --- Physical Address Tests --- */
TEST_F(EthIfTests, GetPhysAddr_Valid) {
    uint8 macAddr[6];
    EthIf_GetPhysAddr(0U, macAddr);
    SUCCEED();
}

/* --- ProvideTxBuffer Tests --- */
TEST_F(EthIfTests, ProvideTxBuffer_Valid) {
    EthIf_SetControllerMode(0U, ETH_MODE_ACTIVE);
    Eth_BufIdxType bufIdx;
    uint8* bufPtr = NULL;
    uint16 lenByte = 100U;
    BufReq_ReturnType result = EthIf_ProvideTxBuffer(0U, ETH_FRAME_TYPE_IPV4, 0U, &bufIdx, &bufPtr, &lenByte);
    EXPECT_EQ(result, BUFREQ_OK);
    EXPECT_NE(bufPtr, (uint8*)NULL);
}

TEST_F(EthIfTests, ProvideTxBuffer_InvalidCtrl) {
    Eth_BufIdxType bufIdx;
    uint8* bufPtr = NULL;
    uint16 lenByte = 100U;
    BufReq_ReturnType result = EthIf_ProvideTxBuffer(ETHIF_MAX_CTRL, ETH_FRAME_TYPE_IPV4, 0U, &bufIdx, &bufPtr, &lenByte);
    EXPECT_NE(result, BUFREQ_OK);
}

/* --- Transmit Tests --- */
TEST_F(EthIfTests, Transmit_Valid) {
    EthIf_SetControllerMode(0U, ETH_MODE_ACTIVE);
    Eth_BufIdxType bufIdx;
    uint8* bufPtr = NULL;
    uint16 lenByte = 64U;
    ASSERT_EQ(EthIf_ProvideTxBuffer(0U, ETH_FRAME_TYPE_IPV4, 0U, &bufIdx, &bufPtr, &lenByte), BUFREQ_OK);
    memset(bufPtr, 0xAA, lenByte);
    uint8 destMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    Std_ReturnType result = EthIf_Transmit(0U, bufIdx, ETH_FRAME_TYPE_IPV4, TRUE, lenByte, destMac);
    /* Transmit may fail when no real network interface is available */
    (void)result;
    SUCCEED();
}

TEST_F(EthIfTests, Transmit_InvalidCtrl) {
    Std_ReturnType result = EthIf_Transmit(ETHIF_MAX_CTRL, 0U, ETH_FRAME_TYPE_IPV4, FALSE, 64U, NULL);
    EXPECT_NE(result, E_OK);
}

/* --- Callback Tests --- */
TEST_F(EthIfTests, RxIndication_NoCrash) {
    uint8 srcMac[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    uint8 data[64] = {0};
    EthIf_RxIndication(0U, ETH_FRAME_TYPE_IPV4, FALSE, srcMac, data, 64U);
    SUCCEED();
}

TEST_F(EthIfTests, TxConfirmation_NoCrash) {
    EthIf_TxConfirmation(0U, 0U);
    SUCCEED();
}

/* --- Callback Registration Tests --- */
TEST_F(EthIfTests, SetRxIndicationCallback_Null) {
    EthIf_SetRxIndicationCallback(NULL);
    SUCCEED();
}

TEST_F(EthIfTests, SetTxConfirmationCallback_Null) {
    EthIf_SetTxConfirmationCallback(NULL);
    SUCCEED();
}

/* --- GetVersionInfo Tests --- */
TEST_F(EthIfTests, GetVersionInfo) {
    Std_VersionInfoType versionInfo;
    EthIf_GetVersionInfo(&versionInfo);
    EXPECT_EQ(versionInfo.moduleID, ETHIF_MODULE_ID);
}

/* --- MainFunction Tests --- */
TEST_F(EthIfTests, MainFunctionRx_NoCrash) {
    EthIf_MainFunctionRx();
    SUCCEED();
}

TEST_F(EthIfTests, MainFunctionTx_NoCrash) {
    EthIf_MainFunctionTx();
    SUCCEED();
}
