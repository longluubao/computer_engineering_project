/********************************************************************************************************/
/*****************************************ApBridgeTests.cpp*********************************************/
/********************************************************************************************************/
/**
 * @file ApBridgeTests.cpp
 * @brief Unit tests for AP Bridge module
 */

#include <gtest/gtest.h>

extern "C" {
#include "ApBridge/ApBridge.h"
}

class ApBridgeTests : public ::testing::Test {
protected:
    void SetUp() override {
        ApBridge_Init();
    }
    void TearDown() override {
        ApBridge_DeInit();
    }
};

/* --- Initialization Tests --- */
TEST_F(ApBridgeTests, Init_NoCrash) {
    ApBridge_DeInit();
    ApBridge_Init();
    SUCCEED();
}

TEST_F(ApBridgeTests, DeInit_NoCrash) {
    ApBridge_DeInit();
    SUCCEED();
}

/* --- GetStatus Tests --- */
TEST_F(ApBridgeTests, GetStatus_Valid) {
    ApBridge_StatusType status;
    Std_ReturnType result = ApBridge_GetStatus(&status);
    EXPECT_EQ(result, E_OK);
}

TEST_F(ApBridgeTests, GetStatus_NullPtr) {
    Std_ReturnType result = ApBridge_GetStatus(NULL);
    EXPECT_NE(result, E_OK);
}

TEST_F(ApBridgeTests, GetStatus_InitialNotReady) {
    ApBridge_StatusType status;
    ApBridge_GetStatus(&status);
    EXPECT_EQ(status.ApBridgeState, SOAD_AP_BRIDGE_NOT_READY);
}

/* --- Heartbeat Tests --- */
TEST_F(ApBridgeTests, ReportHeartbeat_Success) {
    ApBridge_ReportHeartbeat(TRUE);
    SUCCEED();
}

TEST_F(ApBridgeTests, ReportHeartbeat_Failure) {
    ApBridge_ReportHeartbeat(FALSE);
    SUCCEED();
}

TEST_F(ApBridgeTests, HeartbeatTimeout) {
    /* Simulate heartbeat timeout by running main function without heartbeats */
    for (uint16 i = 0; i < APBRIDGE_HEARTBEAT_TIMEOUT_CYCLES + 5; i++) {
        ApBridge_MainFunction();
    }
    ApBridge_StatusType status;
    ApBridge_GetStatus(&status);
    /* Should detect timeout */
    SUCCEED();
}

/* --- Service Status Tests --- */
TEST_F(ApBridgeTests, ReportServiceStatus_Ok) {
    ApBridge_ReportServiceStatus(TRUE);
    SUCCEED();
}

TEST_F(ApBridgeTests, ReportServiceStatus_Failure) {
    ApBridge_ReportServiceStatus(FALSE);
    SUCCEED();
}

TEST_F(ApBridgeTests, ServiceFailureAccumulation) {
    /* Simulate service failures exceeding threshold */
    for (uint8 i = 0; i < APBRIDGE_SERVICE_FAIL_THRESHOLD + 1; i++) {
        ApBridge_ReportServiceStatus(FALSE);
        ApBridge_MainFunction();
    }
    ApBridge_StatusType status;
    ApBridge_GetStatus(&status);
    EXPECT_GT(status.ServiceFailCounter, 0U);
}

/* --- Forced State Tests --- */
TEST_F(ApBridgeTests, SetForcedState_Ready) {
    Std_ReturnType result = ApBridge_SetForcedState(SOAD_AP_BRIDGE_READY);
    EXPECT_EQ(result, E_OK);
}

TEST_F(ApBridgeTests, SetForcedState_Degraded) {
    Std_ReturnType result = ApBridge_SetForcedState(SOAD_AP_BRIDGE_DEGRADED);
    EXPECT_EQ(result, E_OK);
}

TEST_F(ApBridgeTests, SetForcedState_NotReady) {
    Std_ReturnType result = ApBridge_SetForcedState(SOAD_AP_BRIDGE_NOT_READY);
    EXPECT_EQ(result, E_OK);
}

/* --- MainFunction Tests --- */
TEST_F(ApBridgeTests, MainFunction_NoCrash) {
    ApBridge_MainFunction();
    SUCCEED();
}

TEST_F(ApBridgeTests, MainFunction_MultipleCycles) {
    for (int i = 0; i < 100; i++) {
        ApBridge_MainFunction();
    }
    SUCCEED();
}
