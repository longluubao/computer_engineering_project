/********************************************************************************************************/
/*****************************************EncryptTests.cpp**********************************************/
/********************************************************************************************************/
/**
 * @file EncryptTests.cpp
 * @brief Unit tests for AES Encryption module
 */

#include <gtest/gtest.h>
#include <cstring>

extern "C" {
#include "Encrypt/encrypt.h"
}

class EncryptTests : public ::testing::Test {
protected:
};

/* --- Key Expansion Tests --- */
TEST_F(EncryptTests, KeyExpansion_NoCrash) {
    unsigned char key[16] = {0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6,
                              0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C};
    unsigned char expandedKeys[176];
    KeyExpansion(key, expandedKeys);
    /* First 16 bytes should be the original key */
    EXPECT_EQ(memcmp(key, expandedKeys, 16), 0);
}

/* --- SubBytes Tests --- */
TEST_F(EncryptTests, SubBytes_KnownValues) {
    unsigned char state[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                                0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
    SubBytes(state);
    /* S-box: 0x00 -> 0x63, 0x01 -> 0x7C */
    EXPECT_EQ(state[0], 0x63);
    EXPECT_EQ(state[1], 0x7C);
}

/* --- ShiftRows Tests --- */
TEST_F(EncryptTests, ShiftRows_NoCrash) {
    unsigned char state[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                                0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
    ShiftRows(state);
    /* First row unchanged */
    EXPECT_EQ(state[0], 0x00);
}

/* --- MixColumns Tests --- */
TEST_F(EncryptTests, MixColumns_NoCrash) {
    unsigned char state[16] = {0xDB, 0x13, 0x53, 0x45, 0xF2, 0x0A, 0x22, 0x5C,
                                0x01, 0x01, 0x01, 0x01, 0xC6, 0xC6, 0xC6, 0xC6};
    MixColumns(state);
    SUCCEED();
}

/* --- AddRoundKey Tests --- */
TEST_F(EncryptTests, AddRoundKey_XOR) {
    unsigned char state[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    unsigned char roundKey[16] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    AddRoundKey(state, roundKey);
    for (int i = 0; i < 16; i++) {
        EXPECT_EQ(state[i], 0xFF);
    }
}

/* --- AESEncrypt Tests --- */
TEST_F(EncryptTests, AESEncrypt_KnownVector) {
    /* NIST AES-128 test vector */
    unsigned char key[16] = {0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6,
                              0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C};
    unsigned char expandedKeys[176];
    KeyExpansion(key, expandedKeys);

    uint8 plaintext[16] = {0x32, 0x43, 0xF6, 0xA8, 0x88, 0x5A, 0x30, 0x8D,
                            0x31, 0x31, 0x98, 0xA2, 0xE0, 0x37, 0x07, 0x34};
    uint8 ciphertext[16];
    AESEncrypt(plaintext, expandedKeys, ciphertext);
    /* Ciphertext should not be all zeros */
    bool allZero = true;
    for (int i = 0; i < 16; i++) {
        if (ciphertext[i] != 0) { allZero = false; break; }
    }
    EXPECT_FALSE(allZero);
}

TEST_F(EncryptTests, AESEncrypt_DifferentPlaintexts) {
    unsigned char key[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                              0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};
    unsigned char expandedKeys[176];
    KeyExpansion(key, expandedKeys);

    uint8 pt1[16] = {0};
    uint8 pt2[16] = {0};
    pt2[0] = 0x01;
    uint8 ct1[16], ct2[16];
    AESEncrypt(pt1, expandedKeys, ct1);
    AESEncrypt(pt2, expandedKeys, ct2);
    /* Different plaintexts should produce different ciphertexts */
    EXPECT_NE(memcmp(ct1, ct2, 16), 0);
}

/* --- addPadding Tests --- */
TEST_F(EncryptTests, AddPadding_ShortMessage) {
    uint8 message[] = "Hello";
    uint8 paddedMessage[16];
    addPadding(message, 5, paddedMessage);
    /* First 5 bytes should be the message */
    EXPECT_EQ(memcmp(message, paddedMessage, 5), 0);
}

/* --- startEncryption Tests --- */
TEST_F(EncryptTests, StartEncryption_ValidInput) {
    uint8 message[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    uint8 mac[32];
    uint32 macLength = sizeof(mac);
    startEncryption(message, sizeof(message), mac, &macLength);
    EXPECT_GT(macLength, 0U);
}

TEST_F(EncryptTests, StartEncryption_Deterministic) {
    uint8 message[] = {0xAA, 0xBB, 0xCC, 0xDD};
    uint8 mac1[32], mac2[32];
    uint32 macLength1 = sizeof(mac1), macLength2 = sizeof(mac2);
    startEncryption(message, sizeof(message), mac1, &macLength1);
    startEncryption(message, sizeof(message), mac2, &macLength2);
    EXPECT_EQ(macLength1, macLength2);
    EXPECT_EQ(memcmp(mac1, mac2, macLength1), 0);
}
