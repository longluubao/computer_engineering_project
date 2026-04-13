/********************************************************************************************************/
/*****************************************BswMTests.cpp*************************************************/
/********************************************************************************************************/
/**
 * @file BswMTests.cpp
 * @brief Unit tests for Basic Software Mode Manager (BswM) module
 */

#include <gtest/gtest.h>

extern "C" {
#include "BswM/BswM.h"
}

class BswMTests : public ::testing::Test {
protected:
    void SetUp() override {
        BswM_Init(NULL);
    }
    void TearDown() override {
        BswM_Deinit();
    }
};

/* --- Initialization Tests --- */
TEST_F(BswMTests, Init_NullConfig) {
    BswM_Deinit();
    BswM_Init(NULL);
    SUCCEED();
}

TEST_F(BswMTests, Init_WithConfig) {
    BswM_ConfigType cfg;
    cfg.Dummy = 0;
    BswM_Deinit();
    BswM_Init(&cfg);
    SUCCEED();
}

TEST_F(BswMTests, Deinit_NoCrash) {
    BswM_Deinit();
    SUCCEED();
}

TEST_F(BswMTests, Deinit_ThenReinit) {
    BswM_Deinit();
    BswM_Init(NULL);
    SUCCEED();
}

/* --- Mode Request Tests --- */
TEST_F(BswMTests, RequestMode_Valid) {
    Std_ReturnType result = BswM_RequestMode(BSWM_REQUESTER_ID_COMM_DESIRED, 1U);
    EXPECT_EQ(result, E_OK);
}

TEST_F(BswMTests, RequestMode_ApReady) {
    Std_ReturnType result = BswM_RequestMode(BSWM_REQUESTER_ID_AP_READY, BSWM_AP_STATE_READY);
    EXPECT_EQ(result, E_OK);
}

TEST_F(BswMTests, GetCurrentMode_Valid) {
    BswM_RequestMode(BSWM_REQUESTER_ID_COMM_DESIRED, 2U);
    BswM_ModeType mode = BswM_GetCurrentMode(BSWM_REQUESTER_ID_COMM_DESIRED);
    EXPECT_EQ(mode, 2U);
}

TEST_F(BswMTests, GetCurrentMode_InvalidRequester) {
    BswM_ModeType mode = BswM_GetCurrentMode(0xFFFFU);
    /* Should return 0 or error mode for unknown requester */
    (void)mode;
    SUCCEED();
}

/* --- Gateway Profile Tests --- */
TEST_F(BswMTests, GetGatewayProfile_Initial) {
    BswM_GatewayProfileType profile = BswM_GetGatewayProfile();
    EXPECT_EQ(profile, BSWM_GATEWAY_PROFILE_NORMAL);
}

/* --- Gateway Health Tests --- */
TEST_F(BswMTests, GetGatewayHealth_NullPtr) {
    Std_ReturnType result = BswM_GetGatewayHealth(NULL);
    EXPECT_NE(result, E_OK);
}

TEST_F(BswMTests, GetGatewayHealth_Valid) {
    BswM_GatewayHealthType health;
    Std_ReturnType result = BswM_GetGatewayHealth(&health);
    EXPECT_EQ(result, E_OK);
}

TEST_F(BswMTests, SetGatewayHealth_Valid) {
    BswM_GatewayHealthType health;
    health.GatewayProfile = BSWM_GATEWAY_PROFILE_DEGRADED;
    health.CanFaultCounter = 1U;
    health.EthFaultCounter = 0U;
    health.SecOCFailCounter = 0U;
    health.RecoveryCounter = 0U;
    Std_ReturnType result = BswM_SetGatewayHealth(&health);
    EXPECT_EQ(result, E_OK);
}

TEST_F(BswMTests, SetGatewayHealth_NullPtr) {
    Std_ReturnType result = BswM_SetGatewayHealth(NULL);
    EXPECT_NE(result, E_OK);
}

TEST_F(BswMTests, SetGetGatewayHealth_Roundtrip) {
    BswM_GatewayHealthType set_health, get_health;
    set_health.GatewayProfile = BSWM_GATEWAY_PROFILE_DIAG_ONLY;
    set_health.CanFaultCounter = 5U;
    set_health.EthFaultCounter = 3U;
    set_health.SecOCFailCounter = 2U;
    set_health.RecoveryCounter = 1U;
    ASSERT_EQ(BswM_SetGatewayHealth(&set_health), E_OK);
    ASSERT_EQ(BswM_GetGatewayHealth(&get_health), E_OK);
    EXPECT_EQ(get_health.GatewayProfile, BSWM_GATEWAY_PROFILE_DIAG_ONLY);
    EXPECT_EQ(get_health.CanFaultCounter, 5U);
}

/* --- MainFunction Tests --- */
TEST_F(BswMTests, MainFunction_NoCrash) {
    BswM_MainFunction();
    SUCCEED();
}

TEST_F(BswMTests, MainFunction_MultipleCalls) {
    for (int i = 0; i < 100; i++) {
        BswM_MainFunction();
    }
    SUCCEED();
}
