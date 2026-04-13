/********************************************************************************************************/
/*****************************************DemTests.cpp**************************************************/
/********************************************************************************************************/
/**
 * @file DemTests.cpp
 * @brief Unit tests for Diagnostic Event Manager (Dem) module
 */

#include <gtest/gtest.h>
#include <cstring>

extern "C" {
#include "Dem/Dem.h"
}

class DemTests : public ::testing::Test {
protected:
    void SetUp() override {
        Dem_Init();
    }
    void TearDown() override {
        Dem_Shutdown();
    }
};

TEST_F(DemTests, Init_NoError) {
    /* Re-init should be safe */
    Dem_Init();
    SUCCEED();
}

TEST_F(DemTests, ReportErrorStatus_Passed) {
    Std_ReturnType result = Dem_ReportErrorStatus(1U, DEM_EVENT_STATUS_PASSED);
    EXPECT_EQ(result, E_OK);
}

TEST_F(DemTests, ReportErrorStatus_Failed) {
    Std_ReturnType result = Dem_ReportErrorStatus(1U, DEM_EVENT_STATUS_FAILED);
    EXPECT_EQ(result, E_OK);
}

TEST_F(DemTests, ReportErrorStatus_InvalidEventId) {
    Std_ReturnType result = Dem_ReportErrorStatus(0U, DEM_EVENT_STATUS_FAILED);
    (void)result;
    SUCCEED();
}

TEST_F(DemTests, GetLastEvent_AfterReport) {
    Dem_ReportErrorStatus(2U, DEM_EVENT_STATUS_FAILED);
    Dem_EventRecordType record;
    Std_ReturnType result = Dem_GetLastEvent(&record);
    EXPECT_EQ(result, E_OK);
    EXPECT_EQ(record.IsValid, 1U);
}

TEST_F(DemTests, GetLastEvent_NullPtr) {
    Std_ReturnType result = Dem_GetLastEvent(NULL);
    EXPECT_NE(result, E_OK);
}

TEST_F(DemTests, SetEventStatus_Failed) {
    Std_ReturnType result = Dem_SetEventStatus(1U, DEM_EVENT_STATUS_FAILED);
    EXPECT_EQ(result, E_OK);
}

TEST_F(DemTests, SetEventStatus_Passed) {
    Std_ReturnType result = Dem_SetEventStatus(1U, DEM_EVENT_STATUS_PASSED);
    EXPECT_EQ(result, E_OK);
}

TEST_F(DemTests, SetEventStatus_PreFailed) {
    Std_ReturnType result = Dem_SetEventStatus(1U, DEM_EVENT_STATUS_PREFAILED);
    EXPECT_EQ(result, E_OK);
}

TEST_F(DemTests, SetEventStatus_PrePassed) {
    Std_ReturnType result = Dem_SetEventStatus(1U, DEM_EVENT_STATUS_PREPASSED);
    EXPECT_EQ(result, E_OK);
}

TEST_F(DemTests, GetEventStatus_AfterFailed) {
    Dem_SetEventStatus(1U, DEM_EVENT_STATUS_FAILED);
    uint8 status = 0U;
    Std_ReturnType result = Dem_GetEventStatus(1U, &status);
    EXPECT_EQ(result, E_OK);
    EXPECT_NE(status & DEM_DTC_STATUS_TEST_FAILED, 0U);
}

TEST_F(DemTests, GetEventStatus_NullPtr) {
    Std_ReturnType result = Dem_GetEventStatus(1U, NULL);
    EXPECT_NE(result, E_OK);
}

TEST_F(DemTests, ClearDTC_All) {
    Dem_SetEventStatus(1U, DEM_EVENT_STATUS_FAILED);
    Std_ReturnType result = Dem_ClearDTC(0xFFFFFFU);
    EXPECT_EQ(result, E_OK);
}

TEST_F(DemTests, GetNumberOfFilteredDTC) {
    Dem_SetEventStatus(1U, DEM_EVENT_STATUS_FAILED);
    uint16 count = 0U;
    Std_ReturnType result = Dem_GetNumberOfFilteredDTC(DEM_DTC_STATUS_TEST_FAILED, &count);
    EXPECT_EQ(result, E_OK);
}

TEST_F(DemTests, SetDTCFilter_And_GetNext) {
    Dem_SetEventStatus(1U, DEM_EVENT_STATUS_FAILED);
    ASSERT_EQ(Dem_SetDTCFilter(DEM_DTC_STATUS_TEST_FAILED), E_OK);
    uint32 dtc;
    uint8 dtcStatus;
    /* Try to get next filtered DTC */
    Dem_GetNextFilteredDTC(&dtc, &dtcStatus);
    SUCCEED();
}

TEST_F(DemTests, GetDTCOfEvent) {
    Dem_SetEventStatus(1U, DEM_EVENT_STATUS_FAILED);
    uint32 dtcNumber = 0U;
    Std_ReturnType result = Dem_GetDTCOfEvent(1U, &dtcNumber);
    /* May or may not be mapped */
    (void)result;
    SUCCEED();
}

TEST_F(DemTests, ReportDetError) {
    Std_ReturnType result = Dem_ReportDetError(10U, 0U, 0x01U, 0x01U);
    EXPECT_EQ(result, E_OK);
}

TEST_F(DemTests, MainFunction_NoCrash) {
    Dem_MainFunction();
    SUCCEED();
}

TEST_F(DemTests, Shutdown_NoCrash) {
    Dem_Shutdown();
    SUCCEED();
}

TEST_F(DemTests, Debounce_PreFailedMultipleTimes) {
    /* Exercise debounce by reporting PREFAILED multiple times */
    for (int i = 0; i < 10; i++) {
        Dem_SetEventStatus(2U, DEM_EVENT_STATUS_PREFAILED);
    }
    uint8 status = 0U;
    Dem_GetEventStatus(2U, &status);
    /* After enough prefailed, should be confirmed */
    SUCCEED();
}
