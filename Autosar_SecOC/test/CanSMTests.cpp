/********************************************************************************************************/
/*****************************************CanSMTests.cpp************************************************/
/********************************************************************************************************/
/**
 * @file CanSMTests.cpp
 * @brief Unit tests for CAN State Manager (CanSM) module
 */

#include <gtest/gtest.h>

extern "C" {
#include "CanSM/CanSM.h"
}

class CanSMTests : public ::testing::Test {
protected:
    void SetUp() override {
        CanSM_Init();
    }
    void TearDown() override {
        CanSM_DeInit();
    }
};

/* --- Initialization Tests --- */
TEST_F(CanSMTests, Init_NoCrash) {
    CanSM_DeInit();
    CanSM_Init();
    SUCCEED();
}

TEST_F(CanSMTests, DeInit_NoCrash) {
    CanSM_DeInit();
    SUCCEED();
}

/* --- RequestComMode Tests --- */
TEST_F(CanSMTests, RequestFullCom) {
    Std_ReturnType result = CanSM_RequestComMode(0U, CANSM_FULL_COMMUNICATION);
    (void)result; SUCCEED();
}

TEST_F(CanSMTests, RequestNoCom) {
    Std_ReturnType result = CanSM_RequestComMode(0U, CANSM_NO_COMMUNICATION);
    (void)result; SUCCEED();
}

/* --- GetCurrentComMode Tests --- */
TEST_F(CanSMTests, GetCurrentComMode_Initial) {
    CanSM_ComModeType mode;
    Std_ReturnType result = CanSM_GetCurrentComMode(0U, &mode);
    EXPECT_EQ(result, E_OK);
    EXPECT_EQ(mode, CANSM_NO_COMMUNICATION);
}

TEST_F(CanSMTests, GetCurrentComMode_AfterFullCom) {
    CanSM_RequestComMode(0U, CANSM_FULL_COMMUNICATION);
    /* Run main function to process state transitions */
    for (int i = 0; i < 20; i++) {
        CanSM_MainFunction();
    }
    CanSM_ComModeType mode;
    CanSM_GetCurrentComMode(0U, &mode);
    SUCCEED();
}

TEST_F(CanSMTests, GetCurrentComMode_NullPtr) {
    Std_ReturnType result = CanSM_GetCurrentComMode(0U, NULL);
    EXPECT_NE(result, E_OK);
}

/* --- BSM State Tests --- */
TEST_F(CanSMTests, GetBsmState_Initial) {
    CanSM_BsmStateType bsmState;
    Std_ReturnType result = CanSM_GetBsmState(0U, &bsmState);
    EXPECT_EQ(result, E_OK);
}

TEST_F(CanSMTests, GetBsmState_NullPtr) {
    Std_ReturnType result = CanSM_GetBsmState(0U, NULL);
    EXPECT_NE(result, E_OK);
}

/* --- Bus-Off Recovery Tests --- */
TEST_F(CanSMTests, ControllerBusOff_NoCrash) {
    CanSM_RequestComMode(0U, CANSM_FULL_COMMUNICATION);
    CanSM_ControllerBusOff(0U);
    SUCCEED();
}

TEST_F(CanSMTests, ControllerBusOff_RecoveryInMainFunction) {
    CanSM_RequestComMode(0U, CANSM_FULL_COMMUNICATION);
    CanSM_ControllerBusOff(0U);
    for (int i = 0; i < CANSM_BUSOFF_RECOVERY_CYCLES + 5; i++) {
        CanSM_MainFunction();
    }
    SUCCEED();
}

/* --- MainFunction Tests --- */
TEST_F(CanSMTests, MainFunction_NoCrash) {
    CanSM_MainFunction();
    SUCCEED();
}

TEST_F(CanSMTests, MainFunction_MultipleCalls) {
    for (int i = 0; i < 100; i++) {
        CanSM_MainFunction();
    }
    SUCCEED();
}
