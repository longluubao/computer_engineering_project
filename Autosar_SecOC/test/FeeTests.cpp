/********************************************************************************************************/
/*****************************************FeeTests.cpp**************************************************/
/********************************************************************************************************/
/**
 * @file FeeTests.cpp
 * @brief Unit tests for Flash EEPROM Emulation (Fee) module
 */

#include <gtest/gtest.h>
#include <cstring>

extern "C" {
#include "Fee/Fee.h"
}

class FeeTests : public ::testing::Test {
protected:
    void SetUp() override {
        Fee_Init();
    }
};

/* --- Initialization Tests --- */
TEST_F(FeeTests, Init_NoCrash) {
    Fee_Init();
    SUCCEED();
}

/* --- Read/Write Tests --- */
TEST_F(FeeTests, Write_ValidBlock) {
    uint8 data[64];
    memset(data, 0xCC, sizeof(data));
    Std_ReturnType result = Fee_Write(1U, data, 64U);
    EXPECT_EQ(result, E_OK);
}

TEST_F(FeeTests, Write_InvalidBlock) {
    uint8 data[64];
    Std_ReturnType result = Fee_Write(0U, data, 64U);
    (void)result;
    SUCCEED();
}

TEST_F(FeeTests, Write_NullPtr) {
    Std_ReturnType result = Fee_Write(1U, NULL, 64U);
    EXPECT_NE(result, E_OK);
}

TEST_F(FeeTests, Read_ValidBlock) {
    uint8 writeData[32];
    uint8 readData[32];
    memset(writeData, 0xDD, sizeof(writeData));
    Fee_Write(1U, writeData, 32U);
    Fee_MainFunction();
    Std_ReturnType result = Fee_Read(1U, 0U, readData, 32U);
    EXPECT_EQ(result, E_OK);
}

TEST_F(FeeTests, Read_NullPtr) {
    Std_ReturnType result = Fee_Read(1U, 0U, NULL, 32U);
    EXPECT_NE(result, E_OK);
}

/* --- InvalidateBlock Tests --- */
TEST_F(FeeTests, InvalidateBlock_Valid) {
    Std_ReturnType result = Fee_InvalidateBlock(1U);
    EXPECT_EQ(result, E_OK);
}

/* --- EraseImmediateBlock Tests --- */
TEST_F(FeeTests, EraseImmediateBlock_Valid) {
    Std_ReturnType result = Fee_EraseImmediateBlock(1U);
    EXPECT_EQ(result, E_OK);
}

/* --- Status Tests --- */
TEST_F(FeeTests, GetStatus_AfterInit) {
    MemIf_StatusType status = Fee_GetStatus();
    EXPECT_EQ(status, MEMIF_IDLE);
}

TEST_F(FeeTests, GetJobResult_AfterInit) {
    MemIf_JobResultType result = Fee_GetJobResult();
    EXPECT_EQ(result, MEMIF_JOB_OK);
}

/* --- SetMode Tests --- */
TEST_F(FeeTests, SetMode_Fast) {
    Fee_SetMode(MEMIF_MODE_FAST);
    SUCCEED();
}

/* --- MainFunction Tests --- */
TEST_F(FeeTests, MainFunction_NoCrash) {
    Fee_MainFunction();
    SUCCEED();
}
