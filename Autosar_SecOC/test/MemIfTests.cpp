/********************************************************************************************************/
/*****************************************MemIfTests.cpp************************************************/
/********************************************************************************************************/
/**
 * @file MemIfTests.cpp
 * @brief Unit tests for Memory Interface Abstraction (MemIf) module
 */

#include <gtest/gtest.h>
#include <cstring>

extern "C" {
#include "MemIf/MemIf.h"
}

class MemIfTests : public ::testing::Test {
protected:
    void SetUp() override {
        MemIf_Init();
    }
};

/* --- Initialization Tests --- */
TEST_F(MemIfTests, Init_NoCrash) {
    MemIf_Init();
    SUCCEED();
}

/* --- Read/Write Tests --- */
TEST_F(MemIfTests, Write_FeeDevice) {
    uint8 data[32];
    memset(data, 0xEE, sizeof(data));
    Std_ReturnType result = MemIf_Write(MEMIF_FEE_DEVICE_INDEX, 1U, data, 32U);
    EXPECT_EQ(result, E_OK);
}

TEST_F(MemIfTests, Write_EaDevice) {
    uint8 data[32];
    memset(data, 0xFF, sizeof(data));
    Std_ReturnType result = MemIf_Write(MEMIF_EA_DEVICE_INDEX, 1U, data, 32U);
    EXPECT_EQ(result, E_OK);
}

TEST_F(MemIfTests, Write_InvalidDevice) {
    uint8 data[32];
    Std_ReturnType result = MemIf_Write(MEMIF_NUMBER_OF_DEVICES, 1U, data, 32U);
    EXPECT_NE(result, E_OK);
}

TEST_F(MemIfTests, Read_FeeDevice) {
    uint8 data[32];
    Std_ReturnType result = MemIf_Read(MEMIF_FEE_DEVICE_INDEX, 1U, 0U, data, 32U);
    EXPECT_EQ(result, E_OK);
}

TEST_F(MemIfTests, Read_InvalidDevice) {
    uint8 data[32];
    Std_ReturnType result = MemIf_Read(MEMIF_NUMBER_OF_DEVICES, 1U, 0U, data, 32U);
    EXPECT_NE(result, E_OK);
}

/* --- InvalidateBlock Tests --- */
TEST_F(MemIfTests, InvalidateBlock_Valid) {
    Std_ReturnType result = MemIf_InvalidateBlock(MEMIF_FEE_DEVICE_INDEX, 1U);
    EXPECT_EQ(result, E_OK);
}

/* --- EraseImmediateBlock Tests --- */
TEST_F(MemIfTests, EraseImmediateBlock_Valid) {
    Std_ReturnType result = MemIf_EraseImmediateBlock(MEMIF_FEE_DEVICE_INDEX, 1U);
    EXPECT_EQ(result, E_OK);
}

/* --- SetMode Tests --- */
TEST_F(MemIfTests, SetMode_Fast) {
    MemIf_SetMode(MEMIF_MODE_FAST);
    SUCCEED();
}

TEST_F(MemIfTests, SetMode_Slow) {
    MemIf_SetMode(MEMIF_MODE_SLOW);
    SUCCEED();
}

/* --- GetStatus Tests --- */
TEST_F(MemIfTests, GetStatus_FeeDevice) {
    MemIf_StatusType status = MemIf_GetStatus(MEMIF_FEE_DEVICE_INDEX);
    EXPECT_EQ(status, MEMIF_IDLE);
}

TEST_F(MemIfTests, GetStatus_EaDevice) {
    MemIf_StatusType status = MemIf_GetStatus(MEMIF_EA_DEVICE_INDEX);
    EXPECT_EQ(status, MEMIF_IDLE);
}

/* --- GetJobResult Tests --- */
TEST_F(MemIfTests, GetJobResult_FeeDevice) {
    MemIf_JobResultType result = MemIf_GetJobResult(MEMIF_FEE_DEVICE_INDEX);
    EXPECT_EQ(result, MEMIF_JOB_OK);
}

/* --- SetJobNotifications Tests --- */
TEST_F(MemIfTests, SetJobNotifications_Valid) {
    Std_ReturnType result = MemIf_SetJobNotifications(MEMIF_FEE_DEVICE_INDEX, NULL, NULL);
    EXPECT_EQ(result, E_OK);
}

TEST_F(MemIfTests, SetJobNotifications_InvalidDevice) {
    Std_ReturnType result = MemIf_SetJobNotifications(MEMIF_NUMBER_OF_DEVICES, NULL, NULL);
    EXPECT_NE(result, E_OK);
}

/* --- MainFunction Tests --- */
TEST_F(MemIfTests, MainFunction_NoCrash) {
    MemIf_MainFunction();
    SUCCEED();
}
