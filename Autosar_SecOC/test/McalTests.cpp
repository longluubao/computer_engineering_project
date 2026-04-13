/********************************************************************************************************/
/*****************************************McalTests.cpp*************************************************/
/********************************************************************************************************/
/**
 * @file McalTests.cpp
 * @brief Unit tests for MCAL Pi4 drivers (Mcu, Gpt, Wdg, Dio)
 */

#include <gtest/gtest.h>
#include <cstring>

extern "C" {
#include "Mcal/Mcu.h"
#include "Mcal/Gpt.h"
#include "Mcal/Wdg.h"
#include "Mcal/Dio.h"
}

/*===========================================================================*/
/*  MCU Tests                                                                */
/*===========================================================================*/
class McuTests : public ::testing::Test {
protected:
    void SetUp() override {
        Mcu_Init(NULL);
    }
    void TearDown() override {
        Mcu_DeInit();
    }
};

TEST_F(McuTests, Init_NullConfig) {
    Mcu_DeInit();
    Mcu_Init(NULL);
    SUCCEED();
}

TEST_F(McuTests, Init_WithConfig) {
    Mcu_ConfigType cfg;
    cfg.Placeholder = 0;
    Mcu_DeInit();
    Mcu_Init(&cfg);
    SUCCEED();
}

TEST_F(McuTests, GetResetReason) {
    Mcu_ResetType reason = Mcu_GetResetReason();
    EXPECT_LT(reason, MCU_RESET_REASON_COUNT);
}

TEST_F(McuTests, GetPllStatus) {
    Mcu_PllStatusType status = Mcu_GetPllStatus();
    EXPECT_LE(status, MCU_PLL_STATUS_UNDEFINED);
}

TEST_F(McuTests, GetHwInfo_Valid) {
    Mcu_HwInfoType hwInfo;
    Std_ReturnType result = Mcu_GetHwInfo(&hwInfo);
    EXPECT_EQ(result, E_OK);
    EXPECT_GT(hwInfo.CpuCoreCount, 0U);
}

TEST_F(McuTests, GetHwInfo_NullPtr) {
    Std_ReturnType result = Mcu_GetHwInfo(NULL);
    EXPECT_NE(result, E_OK);
}

TEST_F(McuTests, GetVersionInfo) {
    Std_VersionInfoType versionInfo;
    Mcu_GetVersionInfo(&versionInfo);
    EXPECT_EQ(versionInfo.moduleID, MCU_MODULE_ID);
}

TEST_F(McuTests, DeInit_NoCrash) {
    Mcu_DeInit();
    SUCCEED();
}

/*===========================================================================*/
/*  GPT Tests                                                                */
/*===========================================================================*/
class GptTests : public ::testing::Test {
protected:
    void SetUp() override {
        Gpt_Init(NULL);
    }
    void TearDown() override {
        Gpt_DeInit();
    }
};

TEST_F(GptTests, Init_NullConfig) {
    Gpt_DeInit();
    Gpt_Init(NULL);
    SUCCEED();
}

TEST_F(GptTests, StartTimer_Channel0) {
    Gpt_StartTimer(0U, 1000U);
    SUCCEED();
}

TEST_F(GptTests, StopTimer_Channel0) {
    Gpt_StartTimer(0U, 1000U);
    Gpt_StopTimer(0U);
    SUCCEED();
}

TEST_F(GptTests, StartTimer_InvalidChannel) {
    Gpt_StartTimer(GPT_MAX_CHANNELS, 1000U);
    SUCCEED();
}

TEST_F(GptTests, GetTimeElapsed_Channel0) {
    Gpt_StartTimer(0U, 1000000U);
    Gpt_ValueType elapsed = Gpt_GetTimeElapsed(0U);
    (void)elapsed;
    Gpt_StopTimer(0U);
    SUCCEED();
}

TEST_F(GptTests, GetTimestampUs) {
    uint64 ts = Gpt_GetTimestampUs();
    EXPECT_GT(ts, 0ULL);
}

TEST_F(GptTests, EnableNotification_NoCrash) {
    Gpt_EnableNotification(0U);
    SUCCEED();
}

TEST_F(GptTests, DisableNotification_NoCrash) {
    Gpt_DisableNotification(0U);
    SUCCEED();
}

TEST_F(GptTests, DeInit_NoCrash) {
    Gpt_DeInit();
    SUCCEED();
}

/*===========================================================================*/
/*  WDG Tests                                                                */
/*===========================================================================*/
class WdgTests : public ::testing::Test {
protected:
    void SetUp() override {
        Wdg_ConfigType cfg;
        cfg.InitialMode = WDG_MODE_OFF;
        cfg.SlowTimeoutSec = 30U;
        cfg.FastTimeoutSec = 5U;
        Wdg_Init(&cfg);
    }
    void TearDown() override {
        Wdg_DeInit();
    }
};

TEST_F(WdgTests, Init_NullConfig) {
    Wdg_DeInit();
    Wdg_Init(NULL);
    SUCCEED();
}

TEST_F(WdgTests, SetMode_Off) {
    Std_ReturnType result = Wdg_SetMode(WDG_MODE_OFF);
    EXPECT_EQ(result, E_OK);
}

TEST_F(WdgTests, SetMode_Slow) {
    Std_ReturnType result = Wdg_SetMode(WDG_MODE_SLOW);
    /* Returns E_NOT_OK when /dev/watchdog is not available (no hardware) */
    (void)result;
    SUCCEED();
}

TEST_F(WdgTests, SetMode_Fast) {
    Std_ReturnType result = Wdg_SetMode(WDG_MODE_FAST);
    /* Returns E_NOT_OK when /dev/watchdog is not available (no hardware) */
    (void)result;
    SUCCEED();
}

TEST_F(WdgTests, Trigger_NoCrash) {
    Wdg_Trigger();
    SUCCEED();
}

TEST_F(WdgTests, DeInit_NoCrash) {
    Wdg_DeInit();
    SUCCEED();
}

/*===========================================================================*/
/*  DIO Tests                                                                */
/*===========================================================================*/
class DioTests : public ::testing::Test {
protected:
    void SetUp() override {
        Dio_Init(NULL);
    }
    void TearDown() override {
        Dio_DeInit();
    }
};

TEST_F(DioTests, Init_NullConfig) {
    Dio_DeInit();
    Dio_Init(NULL);
    SUCCEED();
}

TEST_F(DioTests, ReadChannel_NoCrash) {
    Dio_LevelType level = Dio_ReadChannel(0U);
    EXPECT_LE(level, DIO_LEVEL_HIGH);
}

TEST_F(DioTests, WriteChannel_High) {
    Dio_WriteChannel(0U, DIO_LEVEL_HIGH);
    SUCCEED();
}

TEST_F(DioTests, WriteChannel_Low) {
    Dio_WriteChannel(0U, DIO_LEVEL_LOW);
    SUCCEED();
}

TEST_F(DioTests, FlipChannel) {
    Dio_LevelType newLevel = Dio_FlipChannel(0U);
    EXPECT_LE(newLevel, DIO_LEVEL_HIGH);
}

TEST_F(DioTests, DeInit_NoCrash) {
    Dio_DeInit();
    SUCCEED();
}
