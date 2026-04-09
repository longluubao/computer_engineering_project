/********************************************************************************************************/
/*****************************************SoAdPqcTests.cpp***********************************************/
/********************************************************************************************************/
/**
 * @file SoAdPqcTests.cpp
 * @brief Unit tests for SoAd PQC Integration (Ethernet key exchange layer)
 * @details Validates init/deinit, state machine, control message parsing,
 *          and rekeying timer behavior.
 */

#include <gtest/gtest.h>
#include <cstring>

extern "C" {
#include "SoAd/SoAd_PQC.h"
#include "Csm/Csm.h"
#include "PQC/PQC.h"
}

/*===========================================================================*/
/*  Test Fixture                                                             */
/*===========================================================================*/
class SoAdPqcTests : public ::testing::Test {
protected:
    void SetUp() override {
        (void)PQC_Init();
        Csm_ConfigType cfg;
        cfg.CsmMldsaBootstrapMode = CSM_MLDSA_BOOTSTRAP_DEMO_FILE_AUTO;
        cfg.CsmLoadProvisionedMldsaKeysFct = NULL;
        Csm_Init(&cfg);
        (void)SoAd_PQC_Init();
    }

    void TearDown() override {
        SoAd_PQC_DeInit();
    }
};

/*===========================================================================*/
/*  Initialization Tests                                                     */
/*===========================================================================*/
TEST_F(SoAdPqcTests, Init_ReturnsOk) {
    SoAd_PQC_DeInit();
    Std_ReturnType result = SoAd_PQC_Init();
    EXPECT_EQ(result, E_OK);
}

TEST_F(SoAdPqcTests, Init_Idempotent) {
    Std_ReturnType result = SoAd_PQC_Init();
    EXPECT_EQ(result, E_OK);
}

TEST_F(SoAdPqcTests, DeInit_ThenReinit) {
    SoAd_PQC_DeInit();
    Std_ReturnType result = SoAd_PQC_Init();
    EXPECT_EQ(result, E_OK);
}

/*===========================================================================*/
/*  State Machine Tests                                                      */
/*===========================================================================*/
TEST_F(SoAdPqcTests, InitialState_AllPeersIdle) {
    for (Csm_PeerIdType peerId = 0U; peerId < SOAD_PQC_MAX_PEERS; peerId++) {
        EXPECT_EQ(SoAd_PQC_GetState(peerId), SOAD_PQC_STATE_IDLE);
    }
}

TEST_F(SoAdPqcTests, GetState_InvalidPeerId) {
    SoAd_PQC_StateType state = SoAd_PQC_GetState(SOAD_PQC_MAX_PEERS + 1);
    EXPECT_EQ(state, SOAD_PQC_STATE_FAILED);
}

/*===========================================================================*/
/*  Reset Session Tests                                                      */
/*===========================================================================*/
TEST_F(SoAdPqcTests, ResetSession_IdlePeer) {
    Std_ReturnType result = SoAd_PQC_ResetSession(0U);
    EXPECT_EQ(result, E_OK);
    EXPECT_EQ(SoAd_PQC_GetState(0U), SOAD_PQC_STATE_IDLE);
}

TEST_F(SoAdPqcTests, ResetSession_InvalidPeerId) {
    Std_ReturnType result = SoAd_PQC_ResetSession(SOAD_PQC_MAX_PEERS);
    EXPECT_EQ(result, E_NOT_OK);
}

/*===========================================================================*/
/*  Control Message Parsing Tests                                            */
/*===========================================================================*/
TEST_F(SoAdPqcTests, HandleControlMessage_NullBuffer) {
    boolean result = SoAd_PQC_HandleControlMessage(NULL, 100U);
    EXPECT_EQ(result, FALSE);
}

TEST_F(SoAdPqcTests, HandleControlMessage_TooShort) {
    uint8 buf[3] = {0x51, 0x43, 0x01};
    boolean result = SoAd_PQC_HandleControlMessage(buf, 3U);
    EXPECT_EQ(result, FALSE);
}

TEST_F(SoAdPqcTests, HandleControlMessage_InvalidMagic) {
    uint8 buf[7] = {0xFF, 0xFF, 0x01, 0x01, 0x00, 0x00, 0x00};
    boolean result = SoAd_PQC_HandleControlMessage(buf, 7U);
    EXPECT_EQ(result, FALSE);
}

TEST_F(SoAdPqcTests, HandleControlMessage_InvalidVersion) {
    uint8 buf[7] = {0x51, 0x43, 0xFF, 0x01, 0x00, 0x00, 0x00};
    boolean result = SoAd_PQC_HandleControlMessage(buf, 7U);
    EXPECT_EQ(result, FALSE);
}

TEST_F(SoAdPqcTests, HandleControlMessage_InvalidPeerId) {
    /* Build a valid header but with out-of-range peer ID */
    uint8 buf[8];
    buf[0] = 0x51U;  /* Magic0 */
    buf[1] = 0x43U;  /* Magic1 */
    buf[2] = 0x01U;  /* Version */
    buf[3] = 0x01U;  /* Type: PUBKEY */
    buf[4] = (uint8)(SOAD_PQC_MAX_PEERS + 1U); /* Invalid peer ID */
    buf[5] = 0x01U;  /* Payload length low byte */
    buf[6] = 0x00U;  /* Payload length high byte */
    buf[7] = 0x00U;  /* Minimal payload */

    boolean result = SoAd_PQC_HandleControlMessage(buf, 8U);
    EXPECT_EQ(result, FALSE);
}

TEST_F(SoAdPqcTests, HandleControlMessage_LengthMismatch) {
    /* Header says 100 bytes payload but total length doesn't match */
    uint8 buf[8];
    buf[0] = 0x51U;
    buf[1] = 0x43U;
    buf[2] = 0x01U;
    buf[3] = 0x01U;
    buf[4] = 0x00U;
    buf[5] = 0x64U;  /* 100 bytes claimed */
    buf[6] = 0x00U;
    buf[7] = 0x00U;

    /* Only 8 bytes total, but header claims 7+100=107 */
    boolean result = SoAd_PQC_HandleControlMessage(buf, 8U);
    EXPECT_EQ(result, FALSE);
}

TEST_F(SoAdPqcTests, HandleControlMessage_UnknownMessageType) {
    /* Build valid header with unknown type 0xFF */
    uint8 buf[7];
    buf[0] = 0x51U;
    buf[1] = 0x43U;
    buf[2] = 0x01U;
    buf[3] = 0xFFU; /* Unknown type */
    buf[4] = 0x00U;
    buf[5] = 0x00U; /* 0 bytes payload */
    buf[6] = 0x00U;

    boolean result = SoAd_PQC_HandleControlMessage(buf, 7U);
    EXPECT_EQ(result, FALSE);
}

/*===========================================================================*/
/*  MainFunction Tests                                                       */
/*===========================================================================*/
TEST_F(SoAdPqcTests, MainFunction_NoEstablishedPeers_NoCrash) {
    /* Should be safe to call MainFunction with all peers idle */
    SoAd_PQC_MainFunction();
    SUCCEED();
}

TEST_F(SoAdPqcTests, MainFunction_MultipleCalls_NoCrash) {
    /* Simulate many cycles without any established sessions */
    for (int i = 0; i < 1000; i++) {
        SoAd_PQC_MainFunction();
    }
    SUCCEED();
}

/*===========================================================================*/
/*  Key Exchange Trigger Tests                                               */
/*===========================================================================*/
TEST_F(SoAdPqcTests, KeyExchange_NotInitialized) {
    SoAd_PQC_DeInit();

    /* Should fail because module not initialized */
    Std_ReturnType result = SoAd_PQC_KeyExchange(0U, TRUE);
    EXPECT_NE(result, E_OK);
}

TEST_F(SoAdPqcTests, KeyExchange_InvalidPeerId) {
    Std_ReturnType result = SoAd_PQC_KeyExchange(SOAD_PQC_MAX_PEERS, TRUE);
    EXPECT_NE(result, E_OK);
}
