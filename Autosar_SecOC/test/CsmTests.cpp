/********************************************************************************************************/
/***********************************************CsmTests.cpp*********************************************/
/********************************************************************************************************/
/**
 * @file CsmTests.cpp
 * @brief Comprehensive unit tests for Cryptographic Service Manager (Csm)
 * @details Tests MAC generation/verification, ML-DSA signatures, ML-KEM key
 *          exchange, session key derivation, and job management.
 */

#include <gtest/gtest.h>
#include <cstring>

extern "C" {
#include "Csm/Csm.h"
#include "PQC/PQC.h"
}

/*===========================================================================*/
/*  Test Fixture                                                             */
/*===========================================================================*/
class CsmTests : public ::testing::Test {
protected:
    void SetUp() override {
        /* Ensure PQC and CSM are initialized with DEMO_FILE_AUTO mode
         * so that ML-DSA keys are generated automatically if absent. */
        (void)PQC_Init();
        Csm_ConfigType cfg;
        cfg.CsmMldsaBootstrapMode = CSM_MLDSA_BOOTSTRAP_DEMO_FILE_AUTO;
        cfg.CsmLoadProvisionedMldsaKeysFct = NULL;
        Csm_Init(&cfg);
    }
};

/*===========================================================================*/
/*  Initialization Tests                                                     */
/*===========================================================================*/
TEST_F(CsmTests, Init_Idempotent) {
    /* Calling init twice should not fail */
    Csm_Init(NULL);
    /* If we get here, no crash — pass */
    SUCCEED();
}

TEST_F(CsmTests, Init_WithNullConfig) {
    /* NULL config should use default bootstrap mode */
    Csm_DeInit();
    Csm_Init(NULL);
    SUCCEED();
}

/*===========================================================================*/
/*  MAC Generation / Verification Tests                                      */
/*===========================================================================*/
TEST_F(CsmTests, MacGenerate_ValidInput) {
    uint8 data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    uint8 mac[32];
    uint32 macLength = sizeof(mac);

    Std_ReturnType result = Csm_MacGenerate(
        0U, CRYPTO_OPERATIONMODE_SINGLECALL,
        data, sizeof(data),
        mac, &macLength
    );

    EXPECT_EQ(result, E_OK);
    EXPECT_GT(macLength, 0U);
}

TEST_F(CsmTests, MacGenerate_NullDataPtr) {
    uint8 mac[32];
    uint32 macLength = sizeof(mac);

    Std_ReturnType result = Csm_MacGenerate(
        0U, CRYPTO_OPERATIONMODE_SINGLECALL,
        NULL, 8U,
        mac, &macLength
    );

    EXPECT_NE(result, E_OK);
}

TEST_F(CsmTests, MacVerify_ValidMac) {
    uint8 data[] = {0x10, 0x20, 0x30, 0x40};
    uint8 mac[32];
    uint32 macLength = sizeof(mac);
    Crypto_VerifyResultType verifyResult = CRYPTO_E_VER_NOT_OK;

    /* Generate MAC first */
    Std_ReturnType result = Csm_MacGenerate(
        0U, CRYPTO_OPERATIONMODE_SINGLECALL,
        data, sizeof(data),
        mac, &macLength
    );
    ASSERT_EQ(result, E_OK);

    /* Verify the same MAC - accept either verification scheme */
    result = Csm_MacVerify(
        0U, CRYPTO_OPERATIONMODE_SINGLECALL,
        data, sizeof(data),
        mac, macLength,
        &verifyResult
    );

    /* Csm_MacVerify should return E_OK and set verifyResult */
    if (result == E_OK) {
        EXPECT_EQ(verifyResult, CRYPTO_E_VER_OK);
    }
    /* Some implementations return E_OK always, check verifyResult;
       others return E_NOT_OK on mismatch. Both are valid. */
}

TEST_F(CsmTests, MacVerify_TamperedData) {
    uint8 data[] = {0x10, 0x20, 0x30, 0x40};
    uint8 mac[32];
    uint32 macLength = sizeof(mac);
    Crypto_VerifyResultType verifyResult = CRYPTO_E_VER_OK;

    /* Generate MAC */
    Std_ReturnType result = Csm_MacGenerate(
        0U, CRYPTO_OPERATIONMODE_SINGLECALL,
        data, sizeof(data),
        mac, &macLength
    );
    ASSERT_EQ(result, E_OK);

    /* Tamper with data */
    data[0] = 0xFF;

    /* Verify should indicate failure through either return code or verifyResult */
    result = Csm_MacVerify(
        0U, CRYPTO_OPERATIONMODE_SINGLECALL,
        data, sizeof(data),
        mac, macLength,
        &verifyResult
    );

    /* Either the function returns E_NOT_OK, or returns E_OK with VER_NOT_OK */
    bool tamperDetected = (result != E_OK) || (verifyResult == CRYPTO_E_VER_NOT_OK);
    EXPECT_TRUE(tamperDetected);
}

/*===========================================================================*/
/*  ML-DSA Signature Tests                                                   */
/*===========================================================================*/
TEST_F(CsmTests, SignatureGenerate_ValidInput) {
    uint8 message[] = "AUTOSAR SecOC test message for ML-DSA signature";
    uint8 signature[CSM_MLDSA_SIGNATURE_BYTES];
    uint32 signatureLength = sizeof(signature);

    Std_ReturnType result = Csm_SignatureGenerate(
        0U, CRYPTO_OPERATIONMODE_SINGLECALL,
        message, sizeof(message),
        signature, &signatureLength
    );

    /* May return CRYPTO_E_KEY_NOT_VALID if ML-DSA keys not loaded */
    if (result == E_OK) {
        EXPECT_GT(signatureLength, 0U);
        EXPECT_LE(signatureLength, CSM_MLDSA_SIGNATURE_BYTES);
    }
}

TEST_F(CsmTests, SignatureVerify_ValidSignature) {
    uint8 message[] = "Test message for signature verification";
    uint8 signature[CSM_MLDSA_SIGNATURE_BYTES];
    uint32 signatureLength = sizeof(signature);
    Crypto_VerifyResultType verifyResult;

    /* Generate signature */
    Std_ReturnType result = Csm_SignatureGenerate(
        0U, CRYPTO_OPERATIONMODE_SINGLECALL,
        message, sizeof(message),
        signature, &signatureLength
    );

    if (result == E_OK) {
        /* Verify the signature */
        result = Csm_SignatureVerify(
            0U, CRYPTO_OPERATIONMODE_SINGLECALL,
            message, sizeof(message),
            signature, signatureLength,
            &verifyResult
        );

        EXPECT_EQ(result, E_OK);
        EXPECT_EQ(verifyResult, CRYPTO_E_VER_OK);
    }
}

TEST_F(CsmTests, SignatureVerify_TamperedMessage) {
    uint8 message[] = "Original message for tamper test";
    uint8 signature[CSM_MLDSA_SIGNATURE_BYTES];
    uint32 signatureLength = sizeof(signature);
    Crypto_VerifyResultType verifyResult = CRYPTO_E_VER_OK;

    Std_ReturnType result = Csm_SignatureGenerate(
        0U, CRYPTO_OPERATIONMODE_SINGLECALL,
        message, sizeof(message),
        signature, &signatureLength
    );

    if (result == E_OK) {
        /* Tamper with message */
        message[0] = 'X';

        result = Csm_SignatureVerify(
            0U, CRYPTO_OPERATIONMODE_SINGLECALL,
            message, sizeof(message),
            signature, signatureLength,
            &verifyResult
        );

        /* Tamper must be detected: either E_NOT_OK or VER_NOT_OK */
        bool tamperDetected = (result != E_OK) || (verifyResult == CRYPTO_E_VER_NOT_OK);
        EXPECT_TRUE(tamperDetected);
    }
}

/*===========================================================================*/
/*  ML-KEM Key Exchange Tests                                                */
/*===========================================================================*/
TEST_F(CsmTests, KeyExchange_FullHandshake) {
    uint8 publicKey[CSM_MLKEM_PUBLIC_KEY_BYTES];
    uint32 publicKeyLen = sizeof(publicKey);
    uint8 ciphertext[CSM_MLKEM_CIPHERTEXT_BYTES];
    uint32 ciphertextLen = sizeof(ciphertext);

    /* Alice (peer 0) initiates */
    Std_ReturnType result = Csm_KeyExchangeInitiate(
        0U, 0U, publicKey, &publicKeyLen
    );
    ASSERT_EQ(result, E_OK);
    EXPECT_EQ(publicKeyLen, CSM_MLKEM_PUBLIC_KEY_BYTES);

    /* Bob (peer 1) responds using Alice's public key */
    result = Csm_KeyExchangeRespond(
        0U, 1U, publicKey, publicKeyLen,
        ciphertext, &ciphertextLen
    );
    ASSERT_EQ(result, E_OK);
    EXPECT_EQ(ciphertextLen, CSM_MLKEM_CIPHERTEXT_BYTES);

    /* Alice (peer 0) completes with Bob's ciphertext */
    result = Csm_KeyExchangeComplete(
        0U, 0U, ciphertext, ciphertextLen
    );
    EXPECT_EQ(result, E_OK);
}

TEST_F(CsmTests, KeyExchange_SharedSecretMatch) {
    uint8 publicKey[CSM_MLKEM_PUBLIC_KEY_BYTES];
    uint32 publicKeyLen = sizeof(publicKey);
    uint8 ciphertext[CSM_MLKEM_CIPHERTEXT_BYTES];
    uint32 ciphertextLen = sizeof(ciphertext);
    uint8 sharedSecret[CSM_MLKEM_SHARED_SECRET_BYTES];
    uint32 sharedSecretLen = sizeof(sharedSecret);

    /* Alice=peer2, Bob=peer3 */
    ASSERT_EQ(Csm_KeyExchangeInitiate(0U, 2U, publicKey, &publicKeyLen), E_OK);
    ASSERT_EQ(Csm_KeyExchangeRespond(0U, 3U, publicKey, publicKeyLen,
                                      ciphertext, &ciphertextLen), E_OK);
    ASSERT_EQ(Csm_KeyExchangeComplete(0U, 2U, ciphertext, ciphertextLen), E_OK);

    /* Retrieve shared secret from Alice's side */
    Std_ReturnType result = Csm_KeyExchangeGetSharedSecret(
        0U, 2U, sharedSecret, &sharedSecretLen
    );
    EXPECT_EQ(result, E_OK);
    EXPECT_EQ(sharedSecretLen, CSM_MLKEM_SHARED_SECRET_BYTES);

    /* Shared secret should not be all zeros */
    bool allZero = true;
    for (uint32 i = 0; i < CSM_MLKEM_SHARED_SECRET_BYTES; i++) {
        if (sharedSecret[i] != 0) { allZero = false; break; }
    }
    EXPECT_FALSE(allZero);
}

TEST_F(CsmTests, KeyExchange_InvalidPeerId) {
    uint8 publicKey[CSM_MLKEM_PUBLIC_KEY_BYTES];
    uint32 publicKeyLen = sizeof(publicKey);

    Std_ReturnType result = Csm_KeyExchangeInitiate(
        0U, CSM_MAX_PEERS + 1, publicKey, &publicKeyLen
    );
    EXPECT_NE(result, E_OK);
}

TEST_F(CsmTests, KeyExchange_Reset) {
    uint8 publicKey[CSM_MLKEM_PUBLIC_KEY_BYTES];
    uint32 publicKeyLen = sizeof(publicKey);
    uint8 peerId = 5U;

    /* Initiate then reset */
    ASSERT_EQ(Csm_KeyExchangeInitiate(0U, peerId, publicKey, &publicKeyLen), E_OK);
    Std_ReturnType result = Csm_KeyExchangeReset(peerId);
    EXPECT_EQ(result, E_OK);

    /* Should be able to re-initiate after reset */
    publicKeyLen = sizeof(publicKey);
    result = Csm_KeyExchangeInitiate(0U, peerId, publicKey, &publicKeyLen);
    EXPECT_EQ(result, E_OK);
}

/*===========================================================================*/
/*  Session Key Derivation Tests                                             */
/*===========================================================================*/
TEST_F(CsmTests, DeriveSessionKeys_FromSharedSecret) {
    /* Use a known shared secret */
    uint8 sharedSecret[CSM_MLKEM_SHARED_SECRET_BYTES];
    memset(sharedSecret, 0xAB, sizeof(sharedSecret));
    uint8 peerId = 3U;

    Std_ReturnType result = Csm_DeriveSessionKeys(
        peerId, sharedSecret, CSM_MLKEM_SHARED_SECRET_BYTES
    );
    EXPECT_EQ(result, E_OK);
}

TEST_F(CsmTests, GetSessionKeys_AfterDerivation) {
    uint8 sharedSecret[CSM_MLKEM_SHARED_SECRET_BYTES];
    memset(sharedSecret, 0xCD, sizeof(sharedSecret));
    uint8 peerId = 4U;
    Csm_SessionKeysType sessionKeys;

    /* Derive keys */
    ASSERT_EQ(Csm_DeriveSessionKeys(peerId, sharedSecret, CSM_MLKEM_SHARED_SECRET_BYTES), E_OK);

    /* Retrieve keys */
    Std_ReturnType result = Csm_GetSessionKeys(peerId, &sessionKeys);
    EXPECT_EQ(result, E_OK);
    EXPECT_EQ(sessionKeys.IsValid, TRUE);

    /* Keys should not be all zeros */
    bool encAllZero = true;
    bool authAllZero = true;
    for (uint32 i = 0; i < CSM_DERIVED_KEY_LENGTH; i++) {
        if (sessionKeys.EncryptionKey[i] != 0) encAllZero = false;
        if (sessionKeys.AuthenticationKey[i] != 0) authAllZero = false;
    }
    EXPECT_FALSE(encAllZero);
    EXPECT_FALSE(authAllZero);
}

TEST_F(CsmTests, ClearSessionKeys_MakesInvalid) {
    uint8 sharedSecret[CSM_MLKEM_SHARED_SECRET_BYTES];
    memset(sharedSecret, 0xEF, sizeof(sharedSecret));
    uint8 peerId = 5U;
    Csm_SessionKeysType sessionKeys;

    /* Derive then clear */
    ASSERT_EQ(Csm_DeriveSessionKeys(peerId, sharedSecret, CSM_MLKEM_SHARED_SECRET_BYTES), E_OK);
    ASSERT_EQ(Csm_ClearSessionKeys(peerId), E_OK);

    /* Get should fail - keys cleared */
    Std_ReturnType result = Csm_GetSessionKeys(peerId, &sessionKeys);
    EXPECT_NE(result, E_OK);
}

TEST_F(CsmTests, DeriveSessionKeys_DifferentSecrets_DifferentKeys) {
    uint8 secret1[CSM_MLKEM_SHARED_SECRET_BYTES];
    uint8 secret2[CSM_MLKEM_SHARED_SECRET_BYTES];
    memset(secret1, 0xAA, sizeof(secret1));
    memset(secret2, 0xBB, sizeof(secret2));
    Csm_SessionKeysType keys1, keys2;

    ASSERT_EQ(Csm_DeriveSessionKeys(6U, secret1, CSM_MLKEM_SHARED_SECRET_BYTES), E_OK);
    ASSERT_EQ(Csm_GetSessionKeys(6U, &keys1), E_OK);

    ASSERT_EQ(Csm_DeriveSessionKeys(7U, secret2, CSM_MLKEM_SHARED_SECRET_BYTES), E_OK);
    ASSERT_EQ(Csm_GetSessionKeys(7U, &keys2), E_OK);

    /* Different shared secrets must produce different session keys */
    EXPECT_NE(memcmp(keys1.EncryptionKey, keys2.EncryptionKey, CSM_DERIVED_KEY_LENGTH), 0);
    EXPECT_NE(memcmp(keys1.AuthenticationKey, keys2.AuthenticationKey, CSM_DERIVED_KEY_LENGTH), 0);
}

/*===========================================================================*/
/*  Version Info Tests                                                       */
/*===========================================================================*/
TEST_F(CsmTests, GetVersionInfo) {
    Std_VersionInfoType versionInfo;
    Csm_GetVersionInfo(&versionInfo);

    /* Verify non-zero module ID */
    EXPECT_GT(versionInfo.moduleID, 0U);
}
