/********************************************************************************************************/
/*****************************************CryIfTests.cpp************************************************/
/********************************************************************************************************/
/**
 * @file CryIfTests.cpp
 * @brief Unit tests for Crypto Interface (CryIf) module
 */

#include <gtest/gtest.h>
#include <cstring>

extern "C" {
#include "CryIf/CryIf.h"
#include "PQC/PQC.h"
#include "Csm/Csm.h"
}

class CryIfTests : public ::testing::Test {
protected:
    void SetUp() override {
        (void)PQC_Init();
        Csm_ConfigType cfg;
        cfg.CsmMldsaBootstrapMode = CSM_MLDSA_BOOTSTRAP_DEMO_FILE_AUTO;
        cfg.CsmLoadProvisionedMldsaKeysFct = NULL;
        Csm_Init(&cfg);
        CryIf_Init();
    }
};

/* --- Initialization Tests --- */
TEST_F(CryIfTests, Init_NoCrash) {
    CryIf_Init();
    SUCCEED();
}

/* --- MAC Generation Tests --- */
TEST_F(CryIfTests, MacGenerate_Classic) {
    uint8 data[] = {0x01, 0x02, 0x03, 0x04};
    uint8 mac[32];
    uint32 macLength = sizeof(mac);
    Std_ReturnType result = CryIf_MacGenerate(CRYIF_PROVIDER_CLASSIC, data, sizeof(data), mac, &macLength);
    EXPECT_EQ(result, E_OK);
    EXPECT_GT(macLength, 0U);
}

TEST_F(CryIfTests, MacGenerate_NullData) {
    uint8 mac[32];
    uint32 macLength = sizeof(mac);
    Std_ReturnType result = CryIf_MacGenerate(CRYIF_PROVIDER_CLASSIC, NULL, 4U, mac, &macLength);
    EXPECT_NE(result, E_OK);
}

/* --- Key Exchange Tests --- */
TEST_F(CryIfTests, KeyExchangeInitiate_Valid) {
    uint8 publicKey[PQC_MLKEM_PUBLIC_KEY_BYTES];
    Std_ReturnType result = CryIf_KeyExchangeInitiate(0U, publicKey);
    EXPECT_EQ(result, E_OK);
}

TEST_F(CryIfTests, KeyExchangeReset_Valid) {
    Std_ReturnType result = CryIf_KeyExchangeReset(0U);
    EXPECT_EQ(result, E_OK);
}

TEST_F(CryIfTests, KeyExchangeFullHandshake) {
    uint8 publicKey[PQC_MLKEM_PUBLIC_KEY_BYTES];
    uint8 ciphertext[PQC_MLKEM_CIPHERTEXT_BYTES];

    ASSERT_EQ(CryIf_KeyExchangeInitiate(0U, publicKey), E_OK);
    ASSERT_EQ(CryIf_KeyExchangeRespond(1U, publicKey, ciphertext), E_OK);
    ASSERT_EQ(CryIf_KeyExchangeComplete(0U, ciphertext), E_OK);

    uint8 secret[PQC_MLKEM_SHARED_SECRET_BYTES];
    EXPECT_EQ(CryIf_KeyExchangeGetSharedSecret(0U, secret), E_OK);
}

/* --- Session Key Tests --- */
TEST_F(CryIfTests, DeriveSessionKeys_Valid) {
    uint8 sharedSecret[PQC_MLKEM_SHARED_SECRET_BYTES];
    memset(sharedSecret, 0x42, sizeof(sharedSecret));
    PQC_SessionKeysType sessionKeys;
    Std_ReturnType result = CryIf_DeriveSessionKeys(sharedSecret, 0U, &sessionKeys);
    EXPECT_EQ(result, E_OK);
}

TEST_F(CryIfTests, GetSessionKeys_AfterDerivation) {
    uint8 sharedSecret[PQC_MLKEM_SHARED_SECRET_BYTES];
    memset(sharedSecret, 0x42, sizeof(sharedSecret));
    PQC_SessionKeysType derived, retrieved;
    ASSERT_EQ(CryIf_DeriveSessionKeys(sharedSecret, 0U, &derived), E_OK);
    Std_ReturnType result = CryIf_GetSessionKeys(0U, &retrieved);
    EXPECT_EQ(result, E_OK);
}

TEST_F(CryIfTests, ClearSessionKeys_Valid) {
    uint8 sharedSecret[PQC_MLKEM_SHARED_SECRET_BYTES];
    memset(sharedSecret, 0x42, sizeof(sharedSecret));
    PQC_SessionKeysType sessionKeys;
    CryIf_DeriveSessionKeys(sharedSecret, 2U, &sessionKeys);
    Std_ReturnType result = CryIf_ClearSessionKeys(2U);
    EXPECT_EQ(result, E_OK);
}

/* --- ML-DSA Key Management Tests --- */
TEST_F(CryIfTests, MldsaGenerateKeyPair) {
    PQC_MLDSA_KeyPairType keyPair;
    Std_ReturnType result = CryIf_MldsaGenerateKeyPair(&keyPair);
    if (result == E_OK) {
        EXPECT_NE(keyPair.PublicKey[0] + keyPair.PublicKey[1], 0);
    }
}
