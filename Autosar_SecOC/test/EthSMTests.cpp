/********************************************************************************************************/
/*****************************************EthSMTests.cpp************************************************/
/********************************************************************************************************/
/**
 * @file EthSMTests.cpp
 * @brief Unit tests for Ethernet State Manager (EthSM) module
 */

#include <gtest/gtest.h>

extern "C" {
#include "EthSM/EthSM.h"
}

class EthSMTests : public ::testing::Test {
protected:
    void SetUp() override {
        EthSM_Init(NULL);
    }
    void TearDown() override {
        EthSM_DeInit();
    }
};

/* --- Initialization Tests --- */
TEST_F(EthSMTests, Init_NullConfig) {
    EthSM_DeInit();
    EthSM_Init(NULL);
    SUCCEED();
}

TEST_F(EthSMTests, DeInit_NoCrash) {
    EthSM_DeInit();
    SUCCEED();
}

/* --- RequestComMode Tests --- */
TEST_F(EthSMTests, RequestFullCom) {
    Std_ReturnType result = EthSM_RequestComMode(0U, COMM_FULL_COMMUNICATION);
    EXPECT_EQ(result, E_OK);
}

TEST_F(EthSMTests, RequestNoCom) {
    Std_ReturnType result = EthSM_RequestComMode(0U, COMM_NO_COMMUNICATION);
    EXPECT_EQ(result, E_OK);
}

TEST_F(EthSMTests, RequestComMode_InvalidHandle) {
    Std_ReturnType result = EthSM_RequestComMode(ETHSM_MAX_NETWORKS, COMM_FULL_COMMUNICATION);
    EXPECT_NE(result, E_OK);
}

/* --- GetCurrentComMode Tests --- */
TEST_F(EthSMTests, GetCurrentComMode_Initial) {
    ComM_ModeType mode;
    Std_ReturnType result = EthSM_GetCurrentComMode(0U, &mode);
    EXPECT_EQ(result, E_OK);
    EXPECT_EQ(mode, COMM_NO_COMMUNICATION);
}

TEST_F(EthSMTests, GetCurrentComMode_NullPtr) {
    Std_ReturnType result = EthSM_GetCurrentComMode(0U, NULL);
    EXPECT_NE(result, E_OK);
}

TEST_F(EthSMTests, GetCurrentComMode_InvalidHandle) {
    ComM_ModeType mode;
    Std_ReturnType result = EthSM_GetCurrentComMode(ETHSM_MAX_NETWORKS, &mode);
    EXPECT_NE(result, E_OK);
}

/* --- GetCurrentInternalMode Tests --- */
TEST_F(EthSMTests, GetCurrentInternalMode_Initial) {
    EthSM_NetworkModeStateType internalMode;
    Std_ReturnType result = EthSM_GetCurrentInternalMode(0U, &internalMode);
    EXPECT_EQ(result, E_OK);
    EXPECT_EQ(internalMode, ETHSM_STATE_OFFLINE);
}

TEST_F(EthSMTests, GetCurrentInternalMode_NullPtr) {
    Std_ReturnType result = EthSM_GetCurrentInternalMode(0U, NULL);
    EXPECT_NE(result, E_OK);
}

/* --- TcpIp Mode Indication Tests --- */
TEST_F(EthSMTests, TcpIpModeIndication_Online) {
    EthSM_RequestComMode(0U, COMM_FULL_COMMUNICATION);
    EthSM_TcpIpModeIndication(0U, TCPIP_STATE_ONLINE);
    SUCCEED();
}

TEST_F(EthSMTests, TcpIpModeIndication_Offline) {
    EthSM_TcpIpModeIndication(0U, TCPIP_STATE_OFFLINE);
    SUCCEED();
}

/* --- MainFunction Tests --- */
TEST_F(EthSMTests, MainFunction_NoCrash) {
    EthSM_MainFunction();
    SUCCEED();
}

TEST_F(EthSMTests, MainFunction_StateTransition) {
    EthSM_RequestComMode(0U, COMM_FULL_COMMUNICATION);
    for (int i = 0; i < 20; i++) {
        EthSM_MainFunction();
    }
    SUCCEED();
}

/* --- GetVersionInfo Tests --- */
TEST_F(EthSMTests, GetVersionInfo) {
    Std_VersionInfoType versionInfo;
    EthSM_GetVersionInfo(&versionInfo);
    EXPECT_EQ(versionInfo.moduleID, ETHSM_MODULE_ID);
}
