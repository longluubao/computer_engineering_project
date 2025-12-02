/********************************************************************************************************/
/*******************************************PQC_KeyDerivation.h*******************************************/
/********************************************************************************************************/
/**
 * @file PQC_KeyDerivation.h
 * @brief Key Derivation Functions for PQC Shared Secrets
 * @details Implements HKDF (HMAC-based Key Derivation Function) to derive
 *          session keys from ML-KEM shared secrets for AUTOSAR SecOC
 */

#ifndef PQC_KEYDERIVATION_H
#define PQC_KEYDERIVATION_H

/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/
#include "Std_Types.h"
#include "PQC.h"

/********************************************************************************************************/
/***********************************************DEFINES***************************************************/
/********************************************************************************************************/

#define PQC_DERIVED_KEY_LENGTH      32U     /* 256 bits for AES-256 */
#define PQC_SESSION_KEYS_MAX        16U     /* Maximum number of session key pairs */

/********************************************************************************************************/
/**********************************************TYPEDEFS***************************************************/
/********************************************************************************************************/

/**
 * @brief Session keys derived from ML-KEM shared secret
 */
typedef struct {
    uint8 EncryptionKey[PQC_DERIVED_KEY_LENGTH];      /* AES-256-GCM encryption key */
    uint8 AuthenticationKey[PQC_DERIVED_KEY_LENGTH];  /* HMAC-SHA256 authentication key */
    boolean IsValid;                                   /* Key pair valid flag */
} PQC_SessionKeysType;

/********************************************************************************************************/
/***************************************FUNCTION DECLARATIONS********************************************/
/********************************************************************************************************/

/**
 * @brief Initialize Key Derivation Module
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType PQC_KeyDerivation_Init(void);

/**
 * @brief Derive session keys from ML-KEM shared secret using HKDF
 * @details Uses HKDF-Extract and HKDF-Expand to derive two independent keys
 *          from a single shared secret
 *
 * @param[in] SharedSecret ML-KEM shared secret (32 bytes)
 * @param[in] PeerId Peer identifier for key storage
 * @param[out] SessionKeys Derived encryption and authentication keys
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType PQC_DeriveSessionKeys(
    const uint8* SharedSecret,
    uint8 PeerId,
    PQC_SessionKeysType* SessionKeys
);

/**
 * @brief Get stored session keys for a peer
 * @param[in] PeerId Peer identifier
 * @param[out] SessionKeys Retrieved session keys
 * @return E_OK if keys exist, E_NOT_OK otherwise
 */
Std_ReturnType PQC_GetSessionKeys(
    uint8 PeerId,
    PQC_SessionKeysType* SessionKeys
);

/**
 * @brief Clear session keys for a peer
 * @param[in] PeerId Peer identifier
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType PQC_ClearSessionKeys(uint8 PeerId);

#endif /* PQC_KEYDERIVATION_H */
