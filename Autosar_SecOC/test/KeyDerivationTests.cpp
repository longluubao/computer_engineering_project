/********************************************************************************************************/
/*****************************************KeyDerivationTests.cpp*****************************************/
/********************************************************************************************************/
/**
 * @file KeyDerivationTests.cpp
 * @brief Unit tests for PQC Key Derivation (HKDF RFC 5869) module
 * @details Validates HKDF-Extract/Expand, session key derivation, storage,
 *          and clearing. Ensures RFC 5869 compliance and key separation.
 */

#include <gtest/gtest.h>
#include <cstring>

extern "C" {
#include "PQC/PQC.h"
#include "PQC_KeyDerivation.h"
}

/*===========================================================================*/
/*  Test Fixture                                                             */
/*===========================================================================*/
class KeyDerivationTests : public ::testing::Test {
protected:
    void SetUp() override {
        (void)PQC_Init();
        (void)PQC_KeyDerivation_Init();
    }
};

/*===========================================================================*/
/*  Initialization Tests                                                     */
/*===========================================================================*/
TEST_F(KeyDerivationTests, Init_ReturnsOk) {
    Std_ReturnType result = PQC_KeyDerivation_Init();
    EXPECT_EQ(result, E_OK);
}

TEST_F(KeyDerivationTests, Init_Idempotent) {
    EXPECT_EQ(PQC_KeyDerivation_Init(), E_OK);
    EXPECT_EQ(PQC_KeyDerivation_Init(), E_OK);
}

/*===========================================================================*/
/*  Key Derivation Tests                                                     */
/*===========================================================================*/
TEST_F(KeyDerivationTests, DeriveSessionKeys_ValidInput) {
    uint8 sharedSecret[PQC_MLKEM_SHARED_SECRET_BYTES];
    PQC_SessionKeysType sessionKeys;
    memset(sharedSecret, 0x42, sizeof(sharedSecret));

    Std_ReturnType result = PQC_DeriveSessionKeys(sharedSecret, 0U, &sessionKeys);
    EXPECT_EQ(result, E_OK);
    EXPECT_EQ(sessionKeys.IsValid, TRUE);
}

TEST_F(KeyDerivationTests, DeriveSessionKeys_NullSecret) {
    PQC_SessionKeysType sessionKeys;

    Std_ReturnType result = PQC_DeriveSessionKeys(NULL, 0U, &sessionKeys);
    EXPECT_NE(result, E_OK);
}

TEST_F(KeyDerivationTests, DeriveSessionKeys_NullOutputKeys) {
    uint8 sharedSecret[PQC_MLKEM_SHARED_SECRET_BYTES];
    memset(sharedSecret, 0x42, sizeof(sharedSecret));

    Std_ReturnType result = PQC_DeriveSessionKeys(sharedSecret, 0U, NULL);
    EXPECT_NE(result, E_OK);
}

TEST_F(KeyDerivationTests, DeriveSessionKeys_InvalidPeerId) {
    uint8 sharedSecret[PQC_MLKEM_SHARED_SECRET_BYTES];
    PQC_SessionKeysType sessionKeys;
    memset(sharedSecret, 0x42, sizeof(sharedSecret));

    Std_ReturnType result = PQC_DeriveSessionKeys(sharedSecret, PQC_SESSION_KEYS_MAX, &sessionKeys);
    EXPECT_NE(result, E_OK);
}

TEST_F(KeyDerivationTests, DeriveSessionKeys_ProducesDifferentEncAndAuthKeys) {
    uint8 sharedSecret[PQC_MLKEM_SHARED_SECRET_BYTES];
    PQC_SessionKeysType sessionKeys;
    memset(sharedSecret, 0x55, sizeof(sharedSecret));

    ASSERT_EQ(PQC_DeriveSessionKeys(sharedSecret, 0U, &sessionKeys), E_OK);

    /* Encryption key and authentication key must be different (derived with different info) */
    EXPECT_NE(memcmp(sessionKeys.EncryptionKey, sessionKeys.AuthenticationKey,
                     PQC_DERIVED_KEY_LENGTH), 0);
}

TEST_F(KeyDerivationTests, DeriveSessionKeys_Deterministic) {
    uint8 sharedSecret[PQC_MLKEM_SHARED_SECRET_BYTES];
    PQC_SessionKeysType keys1, keys2;
    memset(sharedSecret, 0xAA, sizeof(sharedSecret));

    ASSERT_EQ(PQC_DeriveSessionKeys(sharedSecret, 0U, &keys1), E_OK);
    ASSERT_EQ(PQC_DeriveSessionKeys(sharedSecret, 1U, &keys2), E_OK);

    /* Same shared secret must produce same derived keys (HKDF is deterministic) */
    EXPECT_EQ(memcmp(keys1.EncryptionKey, keys2.EncryptionKey, PQC_DERIVED_KEY_LENGTH), 0);
    EXPECT_EQ(memcmp(keys1.AuthenticationKey, keys2.AuthenticationKey, PQC_DERIVED_KEY_LENGTH), 0);
}

TEST_F(KeyDerivationTests, DeriveSessionKeys_DifferentSecretsProduceDifferentKeys) {
    uint8 secret1[PQC_MLKEM_SHARED_SECRET_BYTES];
    uint8 secret2[PQC_MLKEM_SHARED_SECRET_BYTES];
    PQC_SessionKeysType keys1, keys2;
    memset(secret1, 0x11, sizeof(secret1));
    memset(secret2, 0x22, sizeof(secret2));

    ASSERT_EQ(PQC_DeriveSessionKeys(secret1, 0U, &keys1), E_OK);
    ASSERT_EQ(PQC_DeriveSessionKeys(secret2, 1U, &keys2), E_OK);

    EXPECT_NE(memcmp(keys1.EncryptionKey, keys2.EncryptionKey, PQC_DERIVED_KEY_LENGTH), 0);
}

TEST_F(KeyDerivationTests, DeriveSessionKeys_KeysNotAllZeros) {
    uint8 sharedSecret[PQC_MLKEM_SHARED_SECRET_BYTES];
    PQC_SessionKeysType sessionKeys;
    memset(sharedSecret, 0xCC, sizeof(sharedSecret));

    ASSERT_EQ(PQC_DeriveSessionKeys(sharedSecret, 0U, &sessionKeys), E_OK);

    /* Verify keys are not all zeros */
    uint8 zeros[PQC_DERIVED_KEY_LENGTH];
    memset(zeros, 0, sizeof(zeros));

    EXPECT_NE(memcmp(sessionKeys.EncryptionKey, zeros, PQC_DERIVED_KEY_LENGTH), 0);
    EXPECT_NE(memcmp(sessionKeys.AuthenticationKey, zeros, PQC_DERIVED_KEY_LENGTH), 0);
}

/*===========================================================================*/
/*  Session Key Storage / Retrieval Tests                                    */
/*===========================================================================*/
TEST_F(KeyDerivationTests, GetSessionKeys_AfterDeriving) {
    uint8 sharedSecret[PQC_MLKEM_SHARED_SECRET_BYTES];
    PQC_SessionKeysType derived, retrieved;
    memset(sharedSecret, 0xDD, sizeof(sharedSecret));

    ASSERT_EQ(PQC_DeriveSessionKeys(sharedSecret, 2U, &derived), E_OK);
    ASSERT_EQ(PQC_GetSessionKeys(2U, &retrieved), E_OK);

    EXPECT_EQ(memcmp(derived.EncryptionKey, retrieved.EncryptionKey, PQC_DERIVED_KEY_LENGTH), 0);
    EXPECT_EQ(memcmp(derived.AuthenticationKey, retrieved.AuthenticationKey, PQC_DERIVED_KEY_LENGTH), 0);
    EXPECT_EQ(retrieved.IsValid, TRUE);
}

TEST_F(KeyDerivationTests, GetSessionKeys_InvalidPeerId) {
    PQC_SessionKeysType sessionKeys;

    Std_ReturnType result = PQC_GetSessionKeys(PQC_SESSION_KEYS_MAX, &sessionKeys);
    EXPECT_NE(result, E_OK);
}

TEST_F(KeyDerivationTests, GetSessionKeys_BeforeDerivation) {
    PQC_SessionKeysType sessionKeys;

    /* Peer 15 should not have keys yet */
    Std_ReturnType result = PQC_GetSessionKeys(15U, &sessionKeys);
    EXPECT_NE(result, E_OK);
}

/*===========================================================================*/
/*  Session Key Clearing Tests                                               */
/*===========================================================================*/
TEST_F(KeyDerivationTests, ClearSessionKeys_MakesInvalid) {
    uint8 sharedSecret[PQC_MLKEM_SHARED_SECRET_BYTES];
    PQC_SessionKeysType sessionKeys;
    memset(sharedSecret, 0xEE, sizeof(sharedSecret));

    ASSERT_EQ(PQC_DeriveSessionKeys(sharedSecret, 3U, &sessionKeys), E_OK);
    ASSERT_EQ(PQC_ClearSessionKeys(3U), E_OK);

    /* Should fail to retrieve cleared keys */
    Std_ReturnType result = PQC_GetSessionKeys(3U, &sessionKeys);
    EXPECT_NE(result, E_OK);
}

TEST_F(KeyDerivationTests, ClearSessionKeys_InvalidPeerId) {
    Std_ReturnType result = PQC_ClearSessionKeys(PQC_SESSION_KEYS_MAX);
    EXPECT_NE(result, E_OK);
}

/*===========================================================================*/
/*  Multi-Peer Tests                                                         */
/*===========================================================================*/
TEST_F(KeyDerivationTests, MultiplePeers_IndependentStorage) {
    uint8 secret0[PQC_MLKEM_SHARED_SECRET_BYTES];
    uint8 secret1[PQC_MLKEM_SHARED_SECRET_BYTES];
    PQC_SessionKeysType keys0, keys1, retrieved0;
    memset(secret0, 0x11, sizeof(secret0));
    memset(secret1, 0x22, sizeof(secret1));

    /* Derive for two peers with different secrets */
    ASSERT_EQ(PQC_DeriveSessionKeys(secret0, 10U, &keys0), E_OK);
    ASSERT_EQ(PQC_DeriveSessionKeys(secret1, 11U, &keys1), E_OK);

    /* Clearing peer 11 should not affect peer 10 */
    ASSERT_EQ(PQC_ClearSessionKeys(11U), E_OK);
    ASSERT_EQ(PQC_GetSessionKeys(10U, &retrieved0), E_OK);
    EXPECT_EQ(memcmp(keys0.EncryptionKey, retrieved0.EncryptionKey, PQC_DERIVED_KEY_LENGTH), 0);
}
