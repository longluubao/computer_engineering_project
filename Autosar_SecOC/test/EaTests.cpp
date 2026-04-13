/********************************************************************************************************/
/*****************************************EaTests.cpp***************************************************/
/********************************************************************************************************/
/**
 * @file EaTests.cpp
 * @brief Unit tests for EEPROM Abstraction (Ea) module
 */

#include <gtest/gtest.h>
#include <cstring>

extern "C" {
#include "Ea/Ea.h"
}

class EaTests : public ::testing::Test {
protected:
    void SetUp() override {
        Ea_Init();
    }
};

/* --- Initialization Tests --- */
TEST_F(EaTests, Init_NoCrash) {
    Ea_Init();
    SUCCEED();
}

/* --- Read/Write Tests --- */
TEST_F(EaTests, Write_ValidBlock) {
    uint8 data[64];
    memset(data, 0xAA, sizeof(data));
    Std_ReturnType result = Ea_Write(1U, data, 64U);
    EXPECT_EQ(result, E_OK);
}

TEST_F(EaTests, Write_InvalidBlock) {
    uint8 data[64];
    Std_ReturnType result = Ea_Write(0U, data, 64U);
    (void)result;
    SUCCEED();
}

TEST_F(EaTests, Write_NullPtr) {
    Std_ReturnType result = Ea_Write(1U, NULL, 64U);
    EXPECT_NE(result, E_OK);
}

TEST_F(EaTests, Read_ValidBlock) {
    uint8 writeData[32];
    uint8 readData[32];
    memset(writeData, 0x55, sizeof(writeData));
    Ea_Write(1U, writeData, 32U);
    Ea_MainFunction();
    Std_ReturnType result = Ea_Read(1U, 0U, readData, 32U);
    EXPECT_EQ(result, E_OK);
}

TEST_F(EaTests, Read_NullPtr) {
    Std_ReturnType result = Ea_Read(1U, 0U, NULL, 32U);
    EXPECT_NE(result, E_OK);
}

/* --- InvalidateBlock Tests --- */
TEST_F(EaTests, InvalidateBlock_Valid) {
    Std_ReturnType result = Ea_InvalidateBlock(1U);
    EXPECT_EQ(result, E_OK);
}

/* --- EraseImmediateBlock Tests --- */
TEST_F(EaTests, EraseImmediateBlock_Valid) {
    Std_ReturnType result = Ea_EraseImmediateBlock(1U);
    EXPECT_EQ(result, E_OK);
}

/* --- Status Tests --- */
TEST_F(EaTests, GetStatus_AfterInit) {
    MemIf_StatusType status = Ea_GetStatus();
    EXPECT_EQ(status, MEMIF_IDLE);
}

TEST_F(EaTests, GetJobResult_AfterInit) {
    MemIf_JobResultType result = Ea_GetJobResult();
    EXPECT_EQ(result, MEMIF_JOB_OK);
}

/* --- SetMode Tests --- */
TEST_F(EaTests, SetMode_Fast) {
    Ea_SetMode(MEMIF_MODE_FAST);
    SUCCEED();
}

TEST_F(EaTests, SetMode_Slow) {
    Ea_SetMode(MEMIF_MODE_SLOW);
    SUCCEED();
}

/* --- Notification Tests --- */
TEST_F(EaTests, SetJobEndNotification_Null) {
    Ea_SetJobEndNotification(NULL);
    SUCCEED();
}

TEST_F(EaTests, SetJobErrorNotification_Null) {
    Ea_SetJobErrorNotification(NULL);
    SUCCEED();
}

/* --- MainFunction Tests --- */
TEST_F(EaTests, MainFunction_NoCrash) {
    Ea_MainFunction();
    SUCCEED();
}

TEST_F(EaTests, MainFunction_ProcessWrite) {
    uint8 data[32];
    memset(data, 0xBB, sizeof(data));
    Ea_Write(1U, data, 32U);
    Ea_MainFunction();
    MemIf_JobResultType result = Ea_GetJobResult();
    EXPECT_EQ(result, MEMIF_JOB_OK);
}
