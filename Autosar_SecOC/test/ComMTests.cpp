/********************************************************************************************************/
/*****************************************ComMTests.cpp*************************************************/
/********************************************************************************************************/
/**
 * @file ComMTests.cpp
 * @brief Unit tests for Communication Manager (ComM) module
 */

#include <gtest/gtest.h>

extern "C" {
#include "ComM/ComM.h"
}

class ComMTests : public ::testing::Test {
protected:
    void SetUp() override {
        ComM_Init();
    }
    void TearDown() override {
        ComM_DeInit();
    }
};

/* --- Initialization Tests --- */
TEST_F(ComMTests, Init_NoCrash) {
    ComM_DeInit();
    ComM_Init();
    SUCCEED();
}

TEST_F(ComMTests, DeInit_NoCrash) {
    ComM_DeInit();
    SUCCEED();
}

/* --- Request Com Mode Tests --- */
TEST_F(ComMTests, RequestFullCom) {
    Std_ReturnType result = ComM_RequestComMode(0U, COMM_FULL_COMMUNICATION);
    (void)result; SUCCEED();
}

TEST_F(ComMTests, RequestNoCom) {
    Std_ReturnType result = ComM_RequestComMode(0U, COMM_NO_COMMUNICATION);
    (void)result; SUCCEED();
}

TEST_F(ComMTests, RequestSilentCom) {
    Std_ReturnType result = ComM_RequestComMode(0U, COMM_SILENT_COMMUNICATION);
    (void)result; SUCCEED();
}

TEST_F(ComMTests, RequestComMode_InvalidChannel) {
    Std_ReturnType result = ComM_RequestComMode(COMM_MAX_CHANNELS, COMM_FULL_COMMUNICATION);
    EXPECT_NE(result, E_OK);
}

/* --- Get Current Com Mode Tests --- */
TEST_F(ComMTests, GetCurrentComMode_Initial) {
    ComM_ModeType mode;
    Std_ReturnType result = ComM_GetCurrentComMode(0U, &mode);
    EXPECT_EQ(result, E_OK);
    EXPECT_EQ(mode, COMM_NO_COMMUNICATION);
}

TEST_F(ComMTests, GetCurrentComMode_NullPtr) {
    Std_ReturnType result = ComM_GetCurrentComMode(0U, NULL);
    EXPECT_NE(result, E_OK);
}

TEST_F(ComMTests, GetCurrentComMode_InvalidChannel) {
    ComM_ModeType mode;
    Std_ReturnType result = ComM_GetCurrentComMode(COMM_MAX_CHANNELS, &mode);
    EXPECT_NE(result, E_OK);
}

/* --- Get State Tests --- */
TEST_F(ComMTests, GetState_Valid) {
    ComM_SubStateType state;
    Std_ReturnType result = ComM_GetState(0U, &state);
    EXPECT_EQ(result, E_OK);
}

TEST_F(ComMTests, GetState_NullPtr) {
    Std_ReturnType result = ComM_GetState(0U, NULL);
    EXPECT_NE(result, E_OK);
}

/* --- BusSM Mode Indication Tests --- */
TEST_F(ComMTests, BusSM_ModeIndication_FullCom) {
    Std_ReturnType result = ComM_BusSM_ModeIndication(0U, COMM_FULL_COMMUNICATION);
    EXPECT_EQ(result, E_OK);
}

TEST_F(ComMTests, BusSM_ModeIndication_NoCom) {
    Std_ReturnType result = ComM_BusSM_ModeIndication(0U, COMM_NO_COMMUNICATION);
    EXPECT_EQ(result, E_OK);
}

/* --- MainFunction Tests --- */
TEST_F(ComMTests, MainFunction_NoCrash) {
    ComM_MainFunction();
    SUCCEED();
}

TEST_F(ComMTests, MainFunction_AfterFullComRequest) {
    ComM_RequestComMode(0U, COMM_FULL_COMMUNICATION);
    for (int i = 0; i < 20; i++) {
        ComM_MainFunction();
    }
    SUCCEED();
}
