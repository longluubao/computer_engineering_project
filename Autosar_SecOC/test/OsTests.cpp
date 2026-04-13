/********************************************************************************************************/
/*****************************************OsTests.cpp***************************************************/
/********************************************************************************************************/
/**
 * @file OsTests.cpp
 * @brief Unit tests for AUTOSAR OS (OSEK) module
 */

#include <gtest/gtest.h>

extern "C" {
#include "Os/Os.h"
}

static bool hookCalled = false;
static void TestHook(void) { hookCalled = true; }

class OsTests : public ::testing::Test {
protected:
    void SetUp() override {
        hookCalled = false;
        Os_Init(&Os_Config);
    }
    void TearDown() override {
        ShutdownOS(E_OS_OK);
    }
};

/* --- Initialization Tests --- */
TEST_F(OsTests, Init_NoCrash) {
    Os_Init(&Os_Config);
    SUCCEED();
}

TEST_F(OsTests, Init_NullConfig) {
    Os_Init(NULL);
    SUCCEED();
}

/* --- StartOS / ShutdownOS --- */
TEST_F(OsTests, StartOS_DefaultMode) {
    StartOS(OSDEFAULTAPPMODE);
    SUCCEED();
}

TEST_F(OsTests, ShutdownOS_Ok) {
    StartOS(OSDEFAULTAPPMODE);
    ShutdownOS(E_OS_OK);
    SUCCEED();
}

TEST_F(OsTests, GetActiveApplicationMode) {
    StartOS(OSDEFAULTAPPMODE);
    AppModeType mode = GetActiveApplicationMode();
    EXPECT_EQ(mode, OSDEFAULTAPPMODE);
}

/* --- Task Management Tests --- */
TEST_F(OsTests, ActivateTask_InvalidId) {
    StatusType result = ActivateTask(OS_MAX_TASKS);
    EXPECT_NE(result, E_OS_OK);
}

TEST_F(OsTests, GetTaskID) {
    StartOS(OSDEFAULTAPPMODE);
    TaskType taskId = INVALID_TASK;
    StatusType result = GetTaskID(&taskId);
    EXPECT_EQ(result, E_OS_OK);
}

TEST_F(OsTests, GetTaskState_InvalidId) {
    Os_TaskStateType state;
    StatusType result = GetTaskState(OS_MAX_TASKS, &state);
    EXPECT_NE(result, E_OS_OK);
}

TEST_F(OsTests, TerminateTask_NoRunningTask) {
    StatusType result = TerminateTask();
    EXPECT_NE(result, E_OS_OK);
}

TEST_F(OsTests, ChainTask_InvalidId) {
    StatusType result = ChainTask(OS_MAX_TASKS);
    EXPECT_NE(result, E_OS_OK);
}

/* --- Alarm and Counter Tests --- */
TEST_F(OsTests, IncrementCounter_SystemCounter) {
    StartOS(OSDEFAULTAPPMODE);
    StatusType result = IncrementCounter(OS_SYSTEM_COUNTER_ID);
    EXPECT_EQ(result, E_OS_OK);
}

TEST_F(OsTests, IncrementCounter_InvalidId) {
    StatusType result = IncrementCounter(OS_MAX_COUNTERS);
    EXPECT_NE(result, E_OS_OK);
}

TEST_F(OsTests, GetCounterValue_Valid) {
    StartOS(OSDEFAULTAPPMODE);
    TickType value = 0U;
    StatusType result = Os_GetCounterValue(OS_SYSTEM_COUNTER_ID, &value);
    EXPECT_EQ(result, E_OS_OK);
}

TEST_F(OsTests, GetCounterValue_InvalidId) {
    TickType value = 0U;
    StatusType result = Os_GetCounterValue(OS_MAX_COUNTERS, &value);
    EXPECT_NE(result, E_OS_OK);
}

TEST_F(OsTests, SetRelAlarm_InvalidId) {
    StatusType result = SetRelAlarm(OS_MAX_ALARMS, 10U, 0U);
    EXPECT_NE(result, E_OS_OK);
}

TEST_F(OsTests, SetAbsAlarm_InvalidId) {
    StatusType result = SetAbsAlarm(OS_MAX_ALARMS, 10U, 0U);
    EXPECT_NE(result, E_OS_OK);
}

TEST_F(OsTests, CancelAlarm_InvalidId) {
    StatusType result = CancelAlarm(OS_MAX_ALARMS);
    EXPECT_NE(result, E_OS_OK);
}

TEST_F(OsTests, GetAlarm_InvalidId) {
    TickType tick = 0U;
    StatusType result = GetAlarm(OS_MAX_ALARMS, &tick);
    EXPECT_NE(result, E_OS_OK);
}

TEST_F(OsTests, GetAlarmBase_InvalidId) {
    AlarmBaseType info;
    StatusType result = GetAlarmBase(OS_MAX_ALARMS, &info);
    EXPECT_NE(result, E_OS_OK);
}

/* --- Resource Tests --- */
TEST_F(OsTests, GetResource_InvalidId) {
    StatusType result = GetResource(OS_MAX_RESOURCES);
    EXPECT_NE(result, E_OS_OK);
}

TEST_F(OsTests, ReleaseResource_InvalidId) {
    StatusType result = ReleaseResource(OS_MAX_RESOURCES);
    EXPECT_NE(result, E_OS_OK);
}

/* --- Event Tests --- */
TEST_F(OsTests, SetEvent_InvalidTask) {
    StatusType result = Os_SetEvent(OS_MAX_TASKS, 0x01U);
    EXPECT_NE(result, E_OS_OK);
}

TEST_F(OsTests, ClearEvent_NoCrash) {
    StatusType result = Os_ClearEvent(0x01U);
    /* May fail if no running task */
    (void)result;
    SUCCEED();
}

TEST_F(OsTests, GetEvent_InvalidTask) {
    EventMaskType mask = 0U;
    StatusType result = Os_GetEvent(OS_MAX_TASKS, &mask);
    EXPECT_NE(result, E_OS_OK);
}

/* --- Schedule Table Tests --- */
TEST_F(OsTests, StartScheduleTableRel_InvalidId) {
    StatusType result = StartScheduleTableRel(OS_MAX_SCHEDULE_TABLES, 10U);
    EXPECT_NE(result, E_OS_OK);
}

TEST_F(OsTests, StartScheduleTableAbs_InvalidId) {
    StatusType result = StartScheduleTableAbs(OS_MAX_SCHEDULE_TABLES, 10U);
    EXPECT_NE(result, E_OS_OK);
}

TEST_F(OsTests, StopScheduleTable_InvalidId) {
    StatusType result = StopScheduleTable(OS_MAX_SCHEDULE_TABLES);
    EXPECT_NE(result, E_OS_OK);
}

TEST_F(OsTests, NextScheduleTable_InvalidId) {
    StatusType result = NextScheduleTable(OS_MAX_SCHEDULE_TABLES, 0U);
    EXPECT_NE(result, E_OS_OK);
}

/* --- Interrupt Control Tests --- */
TEST_F(OsTests, InterruptControl_NoCrash) {
    DisableAllInterrupts();
    EnableAllInterrupts();
    SUCCEED();
}

TEST_F(OsTests, SuspendResumeAll_NoCrash) {
    SuspendAllInterrupts();
    ResumeAllInterrupts();
    SUCCEED();
}

TEST_F(OsTests, SuspendResumeOS_NoCrash) {
    SuspendOSInterrupts();
    ResumeOSInterrupts();
    SUCCEED();
}

/* --- ISR / Application Services --- */
TEST_F(OsTests, GetISRID_NoActiveISR) {
    ISRType isrId = GetISRID();
    EXPECT_EQ(isrId, INVALID_ISR);
}

TEST_F(OsTests, GetApplicationID) {
    ApplicationType appId = GetApplicationID();
    (void)appId;
    SUCCEED();
}

TEST_F(OsTests, CheckObjectAccess_InvalidApp) {
    StatusType result = CheckObjectAccess(OS_MAX_APPLICATIONS, OS_OBJECT_TASK, 0U);
    EXPECT_NE(result, E_OS_OK);
}

TEST_F(OsTests, TerminateApplication_InvalidApp) {
    StatusType result = TerminateApplication(OS_MAX_APPLICATIONS, NO_RESTART);
    EXPECT_NE(result, E_OS_OK);
}

/* --- Hook Tests --- */
TEST_F(OsTests, SetStartupHook) {
    Os_SetStartupHook(TestHook);
    SUCCEED();
}

TEST_F(OsTests, SetShutdownHook) {
    Os_SetShutdownHook(TestHook);
    SUCCEED();
}

TEST_F(OsTests, SetErrorHook) {
    Os_SetErrorHook(TestHook);
    SUCCEED();
}

TEST_F(OsTests, SetPreTaskHook) {
    Os_SetPreTaskHook(TestHook);
    SUCCEED();
}

TEST_F(OsTests, SetPostTaskHook) {
    Os_SetPostTaskHook(TestHook);
    SUCCEED();
}

/* --- Schedule --- */
TEST_F(OsTests, Schedule_NoCrash) {
    StartOS(OSDEFAULTAPPMODE);
    StatusType result = Schedule();
    (void)result;
    SUCCEED();
}

/* --- MainFunction --- */
TEST_F(OsTests, MainFunction_NoCrash) {
    StartOS(OSDEFAULTAPPMODE);
    Os_MainFunction();
    SUCCEED();
}

TEST_F(OsTests, MainFunction_MultipleCalls) {
    StartOS(OSDEFAULTAPPMODE);
    for (int i = 0; i < 100; i++) {
        Os_MainFunction();
    }
    SUCCEED();
}

/* --- Trusted Functions --- */
TEST_F(OsTests, RegisterTrustedFunction_Invalid) {
    StatusType result = Os_RegisterTrustedFunction(OS_MAX_TRUSTED_FUNCTIONS, NULL);
    EXPECT_NE(result, E_OS_OK);
}

TEST_F(OsTests, CallTrustedFunction_Unregistered) {
    StatusType result = CallTrustedFunction(0U, NULL);
    EXPECT_NE(result, E_OS_OK);
}

/* --- Enter/Exit ISR --- */
TEST_F(OsTests, EnterISR_InvalidId) {
    StatusType result = Os_EnterISR(INVALID_ISR, 0U);
    EXPECT_NE(result, E_OS_OK);
}

TEST_F(OsTests, ExitISR_NoActiveISR) {
    Os_ExitISR();
    SUCCEED();
}

/* --- AssignObjectToApplication --- */
TEST_F(OsTests, AssignObjectToApplication_InvalidApp) {
    StatusType result = Os_AssignObjectToApplication(OS_OBJECT_TASK, 0U, OS_MAX_APPLICATIONS);
    EXPECT_NE(result, E_OS_OK);
}
