/********************************************************************************************************/
/*****************************************DetTests.cpp**************************************************/
/********************************************************************************************************/
/**
 * @file DetTests.cpp
 * @brief Unit tests for Default Error Tracer (DET) module
 */

#include <gtest/gtest.h>

extern "C" {
#include "Det/Det.h"
}

class DetTests : public ::testing::Test {
protected:
    void SetUp() override {
        Det_Init(NULL);
        Det_Start();
    }
};

TEST_F(DetTests, Init_NullConfig) {
    Det_Init(NULL);
    EXPECT_EQ(Det_GetErrorCount(), 0U);
}

TEST_F(DetTests, Init_WithConfig) {
    Det_ConfigType cfg;
    cfg.dummy = 0;
    Det_Init(&cfg);
    EXPECT_EQ(Det_GetErrorCount(), 0U);
}

TEST_F(DetTests, ReportError_ReturnsOk) {
    Std_ReturnType result = Det_ReportError(10U, 0U, 0x01U, 0x01U);
    EXPECT_EQ(result, E_OK);
}

TEST_F(DetTests, ReportError_IncreasesCount) {
    uint8 before = Det_GetErrorCount();
    Det_ReportError(10U, 0U, 0x01U, 0x02U);
    EXPECT_EQ(Det_GetErrorCount(), before + 1U);
}

TEST_F(DetTests, ReportRuntimeError_ReturnsOk) {
    Std_ReturnType result = Det_ReportRuntimeError(20U, 0U, 0x02U, 0x01U);
    EXPECT_EQ(result, E_OK);
}

TEST_F(DetTests, ReportTransientFault_ReturnsOk) {
    Std_ReturnType result = Det_ReportTransientFault(30U, 0U, 0x03U, 0x01U);
    EXPECT_EQ(result, E_OK);
}

TEST_F(DetTests, GetErrorEntry_ValidIndex) {
    Det_ReportError(10U, 0U, 0x01U, 0x05U);
    Det_ErrorEntryType entry;
    Std_ReturnType result = Det_GetErrorEntry(0U, &entry);
    EXPECT_EQ(result, E_OK);
    EXPECT_EQ(entry.ModuleId, 10U);
    EXPECT_EQ(entry.ErrorId, 0x05U);
    EXPECT_EQ(entry.ErrorType, DET_ERROR_TYPE_DEVELOPMENT);
}

TEST_F(DetTests, GetErrorEntry_RuntimeType) {
    Det_Init(NULL);
    Det_ReportRuntimeError(20U, 1U, 0x02U, 0x0AU);
    Det_ErrorEntryType entry;
    Std_ReturnType result = Det_GetErrorEntry(0U, &entry);
    EXPECT_EQ(result, E_OK);
    EXPECT_EQ(entry.ErrorType, DET_ERROR_TYPE_RUNTIME);
}

TEST_F(DetTests, GetErrorEntry_TransientType) {
    Det_Init(NULL);
    Det_ReportTransientFault(30U, 0U, 0x03U, 0x0BU);
    Det_ErrorEntryType entry;
    Std_ReturnType result = Det_GetErrorEntry(0U, &entry);
    EXPECT_EQ(result, E_OK);
    EXPECT_EQ(entry.ErrorType, DET_ERROR_TYPE_TRANSIENT);
}

TEST_F(DetTests, GetErrorEntry_InvalidIndex) {
    Det_ErrorEntryType entry;
    Std_ReturnType result = Det_GetErrorEntry(255U, &entry);
    EXPECT_NE(result, E_OK);
}

TEST_F(DetTests, GetVersionInfo) {
    Std_VersionInfoType versionInfo;
    Det_GetVersionInfo(&versionInfo);
    EXPECT_EQ(versionInfo.moduleID, DET_MODULE_ID);
    EXPECT_EQ(versionInfo.sw_major_version, DET_SW_MAJOR_VERSION);
}

TEST_F(DetTests, MultipleErrors_CircularBuffer) {
    Det_Init(NULL);
    for (uint8 i = 0; i < DET_ERROR_LOG_SIZE + 5U; i++) {
        Det_ReportError(i, 0U, 0x01U, i);
    }
    /* Buffer should wrap around, count capped at max */
    EXPECT_LE(Det_GetErrorCount(), DET_ERROR_LOG_SIZE);
}
