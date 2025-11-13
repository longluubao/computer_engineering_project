/********************************************************************************************************/
/***********************************************PQC.h*****************************************************/
/********************************************************************************************************/
/**
 * @file PQC.h
 * @brief Post-Quantum Cryptography wrapper for AUTOSAR SecOC
 * @details Provides AUTOSAR-compliant interface to liboqs ML-KEM and ML-DSA algorithms
 *
 * This module wraps the liboqs library to provide:
 * - ML-KEM (Module-Lattice-Based Key-Encapsulation Mechanism) - NIST FIPS 203
 * - ML-DSA (Module-Lattice-Based Digital Signature Algorithm) - NIST FIPS 204
 */

#ifndef PQC_H
#define PQC_H

/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/
#include "Std_Types.h"

/********************************************************************************************************/
/***********************************************DEFINES***************************************************/
/********************************************************************************************************/

/* ML-KEM-768 parameters (NIST Security Level 3 - recommended) */
#define PQC_MLKEM_PUBLIC_KEY_BYTES      1184U
#define PQC_MLKEM_SECRET_KEY_BYTES      2400U
#define PQC_MLKEM_CIPHERTEXT_BYTES      1088U
#define PQC_MLKEM_SHARED_SECRET_BYTES   32U

/* ML-DSA-65 parameters (NIST Security Level 3 - recommended) */
#define PQC_MLDSA_PUBLIC_KEY_BYTES      1952U
#define PQC_MLDSA_SECRET_KEY_BYTES      4032U
#define PQC_MLDSA_SIGNATURE_BYTES       3309U  /* Actual size from liboqs */

/* Maximum signature size for buffer allocation */
#define PQC_MAX_SIGNATURE_LENGTH        PQC_MLDSA_SIGNATURE_BYTES

/* Error codes */
#define PQC_E_OK                        ((Std_ReturnType)0x00U)
#define PQC_E_NOT_OK                    ((Std_ReturnType)0x01U)
#define PQC_E_INVALID_PARAM             ((Std_ReturnType)0x02U)
#define PQC_E_VERIFY_FAILED             ((Std_ReturnType)0x03U)

/********************************************************************************************************/
/**********************************************TYPEDEFS***************************************************/
/********************************************************************************************************/

/**
 * @brief ML-KEM key pair structure
 */
typedef struct {
    uint8 PublicKey[PQC_MLKEM_PUBLIC_KEY_BYTES];
    uint8 SecretKey[PQC_MLKEM_SECRET_KEY_BYTES];
} PQC_MLKEM_KeyPairType;

/**
 * @brief ML-DSA key pair structure
 */
typedef struct {
    uint8 PublicKey[PQC_MLDSA_PUBLIC_KEY_BYTES];
    uint8 SecretKey[PQC_MLDSA_SECRET_KEY_BYTES];
} PQC_MLDSA_KeyPairType;

/**
 * @brief ML-KEM shared secret structure
 */
typedef struct {
    uint8 SharedSecret[PQC_MLKEM_SHARED_SECRET_BYTES];
    uint8 Ciphertext[PQC_MLKEM_CIPHERTEXT_BYTES];
} PQC_MLKEM_SharedSecretType;

/********************************************************************************************************/
/***************************************FUNCTION DECLARATIONS********************************************/
/********************************************************************************************************/

/**
 * @brief Initialize the PQC module
 * @details Must be called before any other PQC functions
 * @return PQC_E_OK if successful, PQC_E_NOT_OK otherwise
 */
Std_ReturnType PQC_Init(void);

/**
 * @brief Generate ML-KEM-768 key pair
 * @param[out] KeyPair Pointer to store generated key pair
 * @return PQC_E_OK if successful, PQC_E_NOT_OK otherwise
 */
Std_ReturnType PQC_MLKEM_KeyGen(PQC_MLKEM_KeyPairType* KeyPair);

/**
 * @brief ML-KEM-768 encapsulation (generate shared secret)
 * @param[in] PublicKey Public key for encapsulation
 * @param[out] SharedSecret Pointer to store shared secret and ciphertext
 * @return PQC_E_OK if successful, PQC_E_NOT_OK otherwise
 */
Std_ReturnType PQC_MLKEM_Encapsulate(
    const uint8* PublicKey,
    PQC_MLKEM_SharedSecretType* SharedSecret
);

/**
 * @brief ML-KEM-768 decapsulation (extract shared secret)
 * @param[in] Ciphertext Ciphertext to decapsulate
 * @param[in] SecretKey Secret key for decapsulation
 * @param[out] SharedSecret Pointer to store extracted shared secret
 * @return PQC_E_OK if successful, PQC_E_NOT_OK otherwise
 */
Std_ReturnType PQC_MLKEM_Decapsulate(
    const uint8* Ciphertext,
    const uint8* SecretKey,
    uint8* SharedSecret
);

/**
 * @brief Generate ML-DSA-65 key pair
 * @param[out] KeyPair Pointer to store generated key pair
 * @return PQC_E_OK if successful, PQC_E_NOT_OK otherwise
 */
Std_ReturnType PQC_MLDSA_KeyGen(PQC_MLDSA_KeyPairType* KeyPair);

/**
 * @brief Generate ML-DSA-65 digital signature
 * @param[in] Message Pointer to message data
 * @param[in] MessageLength Length of message in bytes
 * @param[in] SecretKey Secret key for signing
 * @param[out] Signature Pointer to store signature (must be at least PQC_MLDSA_SIGNATURE_BYTES)
 * @param[out] SignatureLength Actual signature length
 * @return PQC_E_OK if successful, PQC_E_NOT_OK otherwise
 */
Std_ReturnType PQC_MLDSA_Sign(
    const uint8* Message,
    uint32 MessageLength,
    const uint8* SecretKey,
    uint8* Signature,
    uint32* SignatureLength
);

/**
 * @brief Verify ML-DSA-65 digital signature
 * @param[in] Message Pointer to message data
 * @param[in] MessageLength Length of message in bytes
 * @param[in] Signature Pointer to signature data
 * @param[in] SignatureLength Length of signature in bytes
 * @param[in] PublicKey Public key for verification
 * @return PQC_E_OK if signature valid, PQC_E_VERIFY_FAILED if invalid, PQC_E_NOT_OK on error
 */
Std_ReturnType PQC_MLDSA_Verify(
    const uint8* Message,
    uint32 MessageLength,
    const uint8* Signature,
    uint32 SignatureLength,
    const uint8* PublicKey
);

#endif /* PQC_H */
