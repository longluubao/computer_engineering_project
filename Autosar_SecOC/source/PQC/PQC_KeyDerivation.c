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
#include <openssl/evp.h>
#include <openssl/core_names.h>
#include <openssl/params.h>

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
 * @brief Compute HMAC-SHA256 using OpenSSL 3.0 EVP_MAC API
 * @param[in]  key      HMAC key
 * @param[in]  key_len  Key length in bytes
 * @param[in]  data     Data to authenticate
 * @param[in]  data_len Data length in bytes
 * @param[out] out      Output buffer (must be >= 32 bytes)
 * @param[out] out_len  Actual output length
 */
static void PQC_HMAC_SHA256(
    const uint8* key, uint32 key_len,
    const uint8* data, uint32 data_len,
    uint8* out, size_t* out_len)
{
    EVP_MAC* mac = EVP_MAC_fetch(NULL, "HMAC", NULL);
    EVP_MAC_CTX* ctx = EVP_MAC_CTX_new(mac);
    OSSL_PARAM params[2];

    params[0] = OSSL_PARAM_construct_utf8_string(OSSL_MAC_PARAM_DIGEST, "SHA256", 0);
    params[1] = OSSL_PARAM_construct_end();

    (void)EVP_MAC_init(ctx, key, (size_t)key_len, params);
    (void)EVP_MAC_update(ctx, data, (size_t)data_len);
    (void)EVP_MAC_final(ctx, out, out_len, 32U);

    EVP_MAC_CTX_free(ctx);
    EVP_MAC_free(mac);
}

/**
 * @brief HKDF-Extract: Extract pseudorandom key from input key material
 * @details RFC 5869 Section 2.2: PRK = HMAC-SHA256(salt, IKM)
 */
static void HKDF_Extract(
    const uint8* salt,
    uint32 salt_len,
    const uint8* ikm,      /* Input key material (shared secret) */
    uint32 ikm_len,
    uint8* prk)            /* Output: Pseudorandom key (32 bytes) */
{
    size_t prk_len = 32U;

    /* RFC 5869 Section 2.2: PRK = HMAC-Hash(salt, IKM) */
    PQC_HMAC_SHA256(salt, salt_len, ikm, ikm_len, prk, &prk_len);
}

/**
 * @brief HKDF-Expand: Expand pseudorandom key to desired length
 * @details RFC 5869 Section 2.3:
 *          T(0) = empty string
 *          T(i) = HMAC-SHA256(PRK, T(i-1) || info || i)
 *          OKM  = first okm_len octets of T(1) || T(2) || ...
 */
static void HKDF_Expand(
    const uint8* prk,      /* Pseudorandom key from Extract */
    uint32 prk_len,
    const uint8* info,     /* Context/purpose string */
    uint32 info_len,
    uint8* okm,            /* Output key material */
    uint32 okm_len)        /* Desired output length */
{
    uint8 t_prev[32];      /* T(i-1) - previous HMAC output */
    uint32 t_prev_len = 0; /* 0 for first iteration (T(0) = empty) */
    uint8 counter = 1U;
    uint32 offset = 0U;
    /* N = ceil(okm_len / 32) */
    uint32 n = (okm_len + 31U) / 32U;
    uint32 i;

    for (i = 0U; i < n; i++)
    {
        EVP_MAC* mac = EVP_MAC_fetch(NULL, "HMAC", NULL);
        EVP_MAC_CTX* ctx = EVP_MAC_CTX_new(mac);
        OSSL_PARAM params[2];
        uint8 t_out[32];
        size_t t_out_len = 32U;
        uint32 copy_len;

        params[0] = OSSL_PARAM_construct_utf8_string(OSSL_MAC_PARAM_DIGEST, "SHA256", 0);
        params[1] = OSSL_PARAM_construct_end();

        (void)EVP_MAC_init(ctx, prk, (size_t)prk_len, params);

        /* T(i-1) - empty for first iteration per RFC 5869 */
        if (t_prev_len > 0U)
        {
            (void)EVP_MAC_update(ctx, t_prev, (size_t)t_prev_len);
        }

        /* info */
        (void)EVP_MAC_update(ctx, info, (size_t)info_len);

        /* counter byte (1-indexed) */
        (void)EVP_MAC_update(ctx, &counter, 1U);

        (void)EVP_MAC_final(ctx, t_out, &t_out_len, 32U);
        EVP_MAC_CTX_free(ctx);
        EVP_MAC_free(mac);

        /* Copy to output, clamping last block */
        copy_len = (okm_len - offset < 32U) ? (okm_len - offset) : 32U;
        (void)memcpy(&okm[offset], t_out, copy_len);

        /* Save T(i) for next iteration */
        (void)memcpy(t_prev, t_out, 32U);
        t_prev_len = 32U;

        offset += copy_len;
        counter++;
    }
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
        (void)memset(&PQC_SessionKeys[i], 0, sizeof(PQC_SessionKeysType));
        PQC_SessionKeys[i].IsValid = FALSE;
    }

    PQC_KeyDerivation_Initialized = TRUE;
    (void)printf("PQC Key Derivation Module initialized (%u session slots)\n", PQC_SESSION_KEYS_MAX);

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
        (void)printf("ERROR: Key Derivation Module not initialized\n");
        return E_NOT_OK;
    }

    if ((SharedSecret == NULL) || (SessionKeys == NULL))
    {
        return E_NOT_OK;
    }

    if (PeerId >= PQC_SESSION_KEYS_MAX)
    {
        (void)printf("ERROR: Invalid peer ID %u\n", PeerId);
        return E_NOT_OK;
    }

    (void)printf("Deriving session keys for peer %u from ML-KEM shared secret...\n", PeerId);

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
    (void)memcpy(&PQC_SessionKeys[PeerId], SessionKeys, sizeof(PQC_SessionKeysType));

    (void)printf("Session keys derived successfully:\n");
    (void)printf("  Encryption Key:     %u bytes\n", PQC_DERIVED_KEY_LENGTH);
    (void)printf("  Authentication Key: %u bytes\n", PQC_DERIVED_KEY_LENGTH);

    /* Clear sensitive PRK */
    (void)memset(prk, 0, sizeof(prk));

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
        (void)printf("ERROR: No valid session keys for peer %u\n", PeerId);
        return E_NOT_OK;
    }

    (void)memcpy(SessionKeys, &PQC_SessionKeys[PeerId], sizeof(PQC_SessionKeysType));

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

    (void)memset(&PQC_SessionKeys[PeerId], 0, sizeof(PQC_SessionKeysType));
    PQC_SessionKeys[PeerId].IsValid = FALSE;

    (void)printf("Session keys cleared for peer %u\n", PeerId);

    return E_OK;
}
