/********************************************************************************************************/
/*******************************************PQC_KeyDerivation.c*******************************************/
/********************************************************************************************************/
/**
 * @file PQC_KeyDerivation.c
 * @brief Key Derivation Functions implementation
 * @details Implements HKDF using liboqs SHA-256 for deriving session keys
 */

/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/
#include "PQC_KeyDerivation.h"
#include <string.h>
#include <stdio.h>
#include <oqs/oqs.h>
#include <oqs/sha2.h>

/********************************************************************************************************/
/******************************************GLOBAL VARIABLES**********************************************/
/********************************************************************************************************/
static boolean PQC_KeyDerivation_Initialized = FALSE;
static PQC_SessionKeysType PQC_SessionKeys[PQC_SESSION_KEYS_MAX];

/********************************************************************************************************/
/***********************************************CONSTANTS*************************************************/
/********************************************************************************************************/
static const char* HKDF_SALT = "AUTOSAR-SecOC-PQC-v1.0";
static const char* HKDF_INFO_ENCRYPTION = "Encryption-Key";
static const char* HKDF_INFO_AUTHENTICATION = "Authentication-Key";

/********************************************************************************************************/
/**********************************************FUNCTIONS**************************************************/
/********************************************************************************************************/

/**
 * @brief HKDF-Extract: Extract pseudorandom key from input key material
 */
static void HKDF_Extract(
    const uint8* salt,
    uint32 salt_len,
    const uint8* ikm,      /* Input key material (shared secret) */
    uint32 ikm_len,
    uint8* prk)            /* Output: Pseudorandom key (32 bytes) */
{
    /* HKDF-Extract: PRK = HMAC-SHA256(salt, IKM) */
    /* For simplicity, we use SHA-256(salt || IKM) as approximation */
    /* In production, should use proper HMAC-SHA256 from liboqs */

    uint8 input[1024];
    uint32 input_len = 0;

    /* Concatenate salt and IKM */
    memcpy(input, salt, salt_len);
    input_len += salt_len;
    memcpy(input + input_len, ikm, ikm_len);
    input_len += ikm_len;

    /* Hash using SHA-256 from liboqs */
    OQS_SHA2_sha256(prk, input, input_len);
}

/**
 * @brief HKDF-Expand: Expand pseudorandom key to desired length
 */
static void HKDF_Expand(
    const uint8* prk,      /* Pseudorandom key from Extract */
    uint32 prk_len,
    const uint8* info,     /* Context/purpose string */
    uint32 info_len,
    uint8* okm,            /* Output key material */
    uint32 okm_len)        /* Desired output length */
{
    /* HKDF-Expand: OKM = HMAC-SHA256(PRK, info || 0x01) */
    /* Simplified version - in production use proper HKDF */

    uint8 input[1024];
    uint32 input_len = 0;

    /* Concatenate PRK, info, and counter */
    memcpy(input, prk, prk_len);
    input_len += prk_len;
    memcpy(input + input_len, info, info_len);
    input_len += info_len;
    input[input_len++] = 0x01;  /* Counter byte */

    /* Hash using SHA-256 from liboqs */
    uint8 hash[32];
    OQS_SHA2_sha256(hash, input, input_len);

    /* Copy desired length */
    memcpy(okm, hash, okm_len < 32 ? okm_len : 32);
}

/**
 * @brief Initialize the Key Derivation Module
 */
Std_ReturnType PQC_KeyDerivation_Init(void)
{
    uint8 i;

    if (PQC_KeyDerivation_Initialized == TRUE)
    {
        return E_OK;
    }

    /* Initialize all session key storage */
    for (i = 0; i < PQC_SESSION_KEYS_MAX; i++)
    {
        memset(&PQC_SessionKeys[i], 0, sizeof(PQC_SessionKeysType));
        PQC_SessionKeys[i].IsValid = FALSE;
    }

    PQC_KeyDerivation_Initialized = TRUE;
    printf("PQC Key Derivation Module initialized (%u session slots)\n", PQC_SESSION_KEYS_MAX);

    return E_OK;
}

/**
 * @brief Derive session keys from ML-KEM shared secret
 */
Std_ReturnType PQC_DeriveSessionKeys(
    const uint8* SharedSecret,
    uint8 PeerId,
    PQC_SessionKeysType* SessionKeys)
{
    uint8 prk[32];  /* Pseudorandom key */

    if (PQC_KeyDerivation_Initialized == FALSE)
    {
        printf("ERROR: Key Derivation Module not initialized\n");
        return E_NOT_OK;
    }

    if (SharedSecret == NULL || SessionKeys == NULL)
    {
        return E_NOT_OK;
    }

    if (PeerId >= PQC_SESSION_KEYS_MAX)
    {
        printf("ERROR: Invalid peer ID %u\n", PeerId);
        return E_NOT_OK;
    }

    printf("Deriving session keys for peer %u from ML-KEM shared secret...\n", PeerId);

    /* Step 1: HKDF-Extract - derive pseudorandom key from shared secret */
    HKDF_Extract(
        (const uint8*)HKDF_SALT,
        strlen(HKDF_SALT),
        SharedSecret,
        PQC_MLKEM_SHARED_SECRET_BYTES,
        prk
    );

    /* Step 2: HKDF-Expand - derive encryption key */
    HKDF_Expand(
        prk,
        32,
        (const uint8*)HKDF_INFO_ENCRYPTION,
        strlen(HKDF_INFO_ENCRYPTION),
        SessionKeys->EncryptionKey,
        PQC_DERIVED_KEY_LENGTH
    );

    /* Step 3: HKDF-Expand - derive authentication key */
    HKDF_Expand(
        prk,
        32,
        (const uint8*)HKDF_INFO_AUTHENTICATION,
        strlen(HKDF_INFO_AUTHENTICATION),
        SessionKeys->AuthenticationKey,
        PQC_DERIVED_KEY_LENGTH
    );

    SessionKeys->IsValid = TRUE;

    /* Store in global session key array */
    memcpy(&PQC_SessionKeys[PeerId], SessionKeys, sizeof(PQC_SessionKeysType));

    printf("Session keys derived successfully:\n");
    printf("  Encryption Key:     %u bytes\n", PQC_DERIVED_KEY_LENGTH);
    printf("  Authentication Key: %u bytes\n", PQC_DERIVED_KEY_LENGTH);

    /* Clear sensitive PRK */
    memset(prk, 0, sizeof(prk));

    return E_OK;
}

/**
 * @brief Get stored session keys for a peer
 */
Std_ReturnType PQC_GetSessionKeys(
    uint8 PeerId,
    PQC_SessionKeysType* SessionKeys)
{
    if (PeerId >= PQC_SESSION_KEYS_MAX)
    {
        return E_NOT_OK;
    }

    if (PQC_SessionKeys[PeerId].IsValid == FALSE)
    {
        printf("ERROR: No valid session keys for peer %u\n", PeerId);
        return E_NOT_OK;
    }

    memcpy(SessionKeys, &PQC_SessionKeys[PeerId], sizeof(PQC_SessionKeysType));

    return E_OK;
}

/**
 * @brief Clear session keys for a peer
 */
Std_ReturnType PQC_ClearSessionKeys(uint8 PeerId)
{
    if (PeerId >= PQC_SESSION_KEYS_MAX)
    {
        return E_NOT_OK;
    }

    memset(&PQC_SessionKeys[PeerId], 0, sizeof(PQC_SessionKeysType));
    PQC_SessionKeys[PeerId].IsValid = FALSE;

    printf("Session keys cleared for peer %u\n", PeerId);

    return E_OK;
}
