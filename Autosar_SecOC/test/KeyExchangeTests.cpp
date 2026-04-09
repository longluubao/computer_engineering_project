/********************************************************************************************************/
/****************************************KeyExchangeTests.cpp********************************************/
/********************************************************************************************************/
/**
 * @file KeyExchangeTests.cpp
 * @brief Unit tests for PQC ML-KEM Key Exchange module
 * @details Validates the 3-way ML-KEM-768 handshake, state machine transitions,
 *          shared secret agreement, multi-peer support, and error handling.
 */

#include <gtest/gtest.h>
#include <cstring>

extern "C" {
#include "PQC/PQC.h"
#include "PQC/PQC_KeyExchange.h"
}

/*===========================================================================*/
/*  Test Fixture                                                             */
/*===========================================================================*/
class KeyExchangeTests : public ::testing::Test {
protected:
    void SetUp() override {
        (void)PQC_Init();
        (void)PQC_KeyExchange_Init();
    }
};

/*===========================================================================*/
/*  Initialization Tests                                                     */
/*===========================================================================*/
TEST_F(KeyExchangeTests, Init_ReturnsOk) {
    EXPECT_EQ(PQC_KeyExchange_Init(), E_OK);
}

TEST_F(KeyExchangeTests, Init_AllPeersIdle) {
    for (uint8 i = 0; i < PQC_MAX_PEERS; i++) {
        EXPECT_EQ(PQC_KeyExchange_GetState(i), PQC_KE_STATE_IDLE);
    }
}

/*===========================================================================*/
/*  Full Handshake Tests                                                     */
/*  Note: In a real system, Alice and Bob run on different ECUs.             */
/*  On a single device the same peer slot is shared, so Respond overwrites  */
/*  the Initiate state. We use separate peer IDs: 0=Alice, 1=Bob.           */
/*===========================================================================*/
TEST_F(KeyExchangeTests, FullHandshake_AliceBob) {
    uint8 alicePublicKey[PQC_MLKEM_PUBLIC_KEY_BYTES];
    uint8 bobCiphertext[PQC_MLKEM_CIPHERTEXT_BYTES];

    /* Alice (peer 0) initiates */
    Std_ReturnType result = PQC_KeyExchange_Initiate(0U, alicePublicKey);
    ASSERT_EQ(result, E_OK);
    EXPECT_EQ(PQC_KeyExchange_GetState(0U), PQC_KE_STATE_INITIATED);

    /* Bob (peer 1) responds using Alice's public key */
    result = PQC_KeyExchange_Respond(1U, alicePublicKey, bobCiphertext);
    ASSERT_EQ(result, E_OK);
    EXPECT_EQ(PQC_KeyExchange_GetState(1U), PQC_KE_STATE_ESTABLISHED);

    /* Alice (peer 0) completes with Bob's ciphertext */
    result = PQC_KeyExchange_Complete(0U, bobCiphertext);
    ASSERT_EQ(result, E_OK);
    EXPECT_EQ(PQC_KeyExchange_GetState(0U), PQC_KE_STATE_ESTABLISHED);
}

TEST_F(KeyExchangeTests, SharedSecret_RetrievableAfterExchange) {
    uint8 alicePublicKey[PQC_MLKEM_PUBLIC_KEY_BYTES];
    uint8 bobCiphertext[PQC_MLKEM_CIPHERTEXT_BYTES];
    uint8 sharedSecret[PQC_MLKEM_SHARED_SECRET_BYTES];

    /* Alice initiates, Bob responds, Alice completes */
    ASSERT_EQ(PQC_KeyExchange_Initiate(0U, alicePublicKey), E_OK);
    ASSERT_EQ(PQC_KeyExchange_Respond(1U, alicePublicKey, bobCiphertext), E_OK);
    ASSERT_EQ(PQC_KeyExchange_Complete(0U, bobCiphertext), E_OK);

    /* Both sides should have shared secrets */
    EXPECT_EQ(PQC_KeyExchange_GetSharedSecret(0U, sharedSecret), E_OK);

    /* Secret should not be all zeros */
    bool allZero = true;
    for (uint32 i = 0; i < PQC_MLKEM_SHARED_SECRET_BYTES; i++) {
        if (sharedSecret[i] != 0) { allZero = false; break; }
    }
    EXPECT_FALSE(allZero);
}

/*===========================================================================*/
/*  State Machine Tests                                                      */
/*===========================================================================*/
TEST_F(KeyExchangeTests, State_InitiallyIdle) {
    /* Use peer 7 (untouched by other tests) to verify idle state */
    PQC_KeyExchange_Reset(7U);
    EXPECT_EQ(PQC_KeyExchange_GetState(7U), PQC_KE_STATE_IDLE);
}

TEST_F(KeyExchangeTests, State_AfterInitiate) {
    uint8 publicKey[PQC_MLKEM_PUBLIC_KEY_BYTES];
    PQC_KeyExchange_Reset(6U);
    ASSERT_EQ(PQC_KeyExchange_Initiate(6U, publicKey), E_OK);
    EXPECT_EQ(PQC_KeyExchange_GetState(6U), PQC_KE_STATE_INITIATED);
}

TEST_F(KeyExchangeTests, State_AfterReset) {
    uint8 publicKey[PQC_MLKEM_PUBLIC_KEY_BYTES];
    PQC_KeyExchange_Reset(6U);
    ASSERT_EQ(PQC_KeyExchange_Initiate(6U, publicKey), E_OK);
    ASSERT_EQ(PQC_KeyExchange_Reset(6U), E_OK);
    EXPECT_EQ(PQC_KeyExchange_GetState(6U), PQC_KE_STATE_IDLE);
}

/*===========================================================================*/
/*  Error Handling Tests                                                     */
/*===========================================================================*/
TEST_F(KeyExchangeTests, Initiate_InvalidPeerId) {
    uint8 publicKey[PQC_MLKEM_PUBLIC_KEY_BYTES];
    EXPECT_NE(PQC_KeyExchange_Initiate(PQC_MAX_PEERS, publicKey), E_OK);
}

TEST_F(KeyExchangeTests, Initiate_NullPublicKey) {
    EXPECT_NE(PQC_KeyExchange_Initiate(0U, NULL), E_OK);
}

TEST_F(KeyExchangeTests, Respond_NullPeerPublicKey) {
    uint8 ciphertext[PQC_MLKEM_CIPHERTEXT_BYTES];
    EXPECT_NE(PQC_KeyExchange_Respond(0U, NULL, ciphertext), E_OK);
}

TEST_F(KeyExchangeTests, Complete_WithoutInitiate) {
    uint8 ciphertext[PQC_MLKEM_CIPHERTEXT_BYTES];
    memset(ciphertext, 0, sizeof(ciphertext));

    /* Use a fresh peer that hasn't been touched by other tests */
    PQC_KeyExchange_Reset(5U);

    /* Complete without initiating should fail (state is IDLE, not INITIATED) */
    Std_ReturnType result = PQC_KeyExchange_Complete(5U, ciphertext);
    EXPECT_NE(result, E_OK);
}

TEST_F(KeyExchangeTests, GetSharedSecret_BeforeExchange) {
    uint8 sharedSecret[PQC_MLKEM_SHARED_SECRET_BYTES];

    /* Reset to ensure IDLE state, then check that secret retrieval fails */
    PQC_KeyExchange_Reset(7U);
    Std_ReturnType result = PQC_KeyExchange_GetSharedSecret(7U, sharedSecret);
    EXPECT_NE(result, E_OK);
}

TEST_F(KeyExchangeTests, Reset_InvalidPeerId) {
    EXPECT_NE(PQC_KeyExchange_Reset(PQC_MAX_PEERS), E_OK);
}

/*===========================================================================*/
/*  Multi-Peer Tests                                                         */
/*===========================================================================*/
TEST_F(KeyExchangeTests, MultiplePeers_IndependentSessions) {
    uint8 publicKey0[PQC_MLKEM_PUBLIC_KEY_BYTES];
    uint8 publicKey1[PQC_MLKEM_PUBLIC_KEY_BYTES];

    /* Initiate for two different peers */
    ASSERT_EQ(PQC_KeyExchange_Initiate(0U, publicKey0), E_OK);
    ASSERT_EQ(PQC_KeyExchange_Initiate(1U, publicKey1), E_OK);

    /* Both should be INITIATED */
    EXPECT_EQ(PQC_KeyExchange_GetState(0U), PQC_KE_STATE_INITIATED);
    EXPECT_EQ(PQC_KeyExchange_GetState(1U), PQC_KE_STATE_INITIATED);

    /* Resetting peer 0 should not affect peer 1 */
    ASSERT_EQ(PQC_KeyExchange_Reset(0U), E_OK);
    EXPECT_EQ(PQC_KeyExchange_GetState(0U), PQC_KE_STATE_IDLE);
    EXPECT_EQ(PQC_KeyExchange_GetState(1U), PQC_KE_STATE_INITIATED);
}

TEST_F(KeyExchangeTests, MultiplePeers_DifferentSharedSecrets) {
    uint8 publicKey[PQC_MLKEM_PUBLIC_KEY_BYTES];
    uint8 ciphertext[PQC_MLKEM_CIPHERTEXT_BYTES];
    uint8 secretA[PQC_MLKEM_SHARED_SECRET_BYTES];
    uint8 secretB[PQC_MLKEM_SHARED_SECRET_BYTES];

    /* Exchange A: Alice=peer2, Bob=peer3 */
    ASSERT_EQ(PQC_KeyExchange_Initiate(2U, publicKey), E_OK);
    ASSERT_EQ(PQC_KeyExchange_Respond(3U, publicKey, ciphertext), E_OK);
    ASSERT_EQ(PQC_KeyExchange_Complete(2U, ciphertext), E_OK);
    ASSERT_EQ(PQC_KeyExchange_GetSharedSecret(2U, secretA), E_OK);

    /* Exchange B: Alice=peer4, Bob=peer5 */
    ASSERT_EQ(PQC_KeyExchange_Initiate(4U, publicKey), E_OK);
    ASSERT_EQ(PQC_KeyExchange_Respond(5U, publicKey, ciphertext), E_OK);
    ASSERT_EQ(PQC_KeyExchange_Complete(4U, ciphertext), E_OK);
    ASSERT_EQ(PQC_KeyExchange_GetSharedSecret(4U, secretB), E_OK);

    /* Different random keypairs = different shared secrets */
    EXPECT_NE(memcmp(secretA, secretB, PQC_MLKEM_SHARED_SECRET_BYTES), 0);
}

/*===========================================================================*/
/*  Re-exchange Tests                                                        */
/*===========================================================================*/
TEST_F(KeyExchangeTests, Rekey_AfterResetAndReInitiate) {
    uint8 publicKey[PQC_MLKEM_PUBLIC_KEY_BYTES];
    uint8 ciphertext[PQC_MLKEM_CIPHERTEXT_BYTES];
    uint8 secret1[PQC_MLKEM_SHARED_SECRET_BYTES];
    uint8 secret2[PQC_MLKEM_SHARED_SECRET_BYTES];

    /* First exchange: Alice=peer3, Bob=peer4 */
    ASSERT_EQ(PQC_KeyExchange_Initiate(3U, publicKey), E_OK);
    ASSERT_EQ(PQC_KeyExchange_Respond(4U, publicKey, ciphertext), E_OK);
    ASSERT_EQ(PQC_KeyExchange_Complete(3U, ciphertext), E_OK);
    ASSERT_EQ(PQC_KeyExchange_GetSharedSecret(3U, secret1), E_OK);

    /* Reset Alice and re-exchange */
    ASSERT_EQ(PQC_KeyExchange_Reset(3U), E_OK);
    ASSERT_EQ(PQC_KeyExchange_Reset(4U), E_OK);
    ASSERT_EQ(PQC_KeyExchange_Initiate(3U, publicKey), E_OK);
    ASSERT_EQ(PQC_KeyExchange_Respond(4U, publicKey, ciphertext), E_OK);
    ASSERT_EQ(PQC_KeyExchange_Complete(3U, ciphertext), E_OK);
    ASSERT_EQ(PQC_KeyExchange_GetSharedSecret(3U, secret2), E_OK);

    /* New exchange = new random keys = different shared secret */
    EXPECT_NE(memcmp(secret1, secret2, PQC_MLKEM_SHARED_SECRET_BYTES), 0);
}
