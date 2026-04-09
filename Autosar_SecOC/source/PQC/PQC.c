/********************************************************************************************************/
/***********************************************PQC.c*****************************************************/
/********************************************************************************************************/
/**
 * @file PQC.c
 * @brief Post-Quantum Cryptography wrapper implementation
 * @details Wraps liboqs ML-KEM and ML-DSA for AUTOSAR SecOC
 */

/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/
#include "PQC/PQC.h"
#include "SecOC/SecOC_PQC_Cfg.h"
#include <string.h>
#include <stdio.h>

/* liboqs includes */
#include <oqs/oqs.h>

#if defined(LINUX)
#include <sys/stat.h>
#include <errno.h>
#endif

/* External API declarations (MISRA 8.4 visibility). */
Std_ReturnType PQC_Init(void);
Std_ReturnType PQC_MLKEM_KeyGen(PQC_MLKEM_KeyPairType* KeyPair);
Std_ReturnType PQC_MLKEM_Encapsulate(const uint8* PublicKey,
                                     PQC_MLKEM_SharedSecretType* SharedSecret);
Std_ReturnType PQC_MLKEM_Decapsulate(const uint8* Ciphertext,
                                     const uint8* SecretKey,
                                     uint8* SharedSecret);
Std_ReturnType PQC_MLDSA_KeyGen(PQC_MLDSA_KeyPairType* KeyPair);
Std_ReturnType PQC_MLDSA_SaveKeys(const PQC_MLDSA_KeyPairType* KeyPair,
                                  const char* filePrefix);
Std_ReturnType PQC_MLDSA_LoadKeys(PQC_MLDSA_KeyPairType* KeyPair,
                                  const char* filePrefix);
Std_ReturnType PQC_MLDSA_Sign(const uint8* Message, uint32 MessageLength,
                              const uint8* SecretKey, uint8* Signature,
                              uint32* SignatureLength);
Std_ReturnType PQC_MLDSA_Verify(const uint8* Message, uint32 MessageLength,
                                const uint8* Signature, uint32 SignatureLength,
                                const uint8* PublicKey);

/********************************************************************************************************/
/******************************************GLOBAL VARIABLES**********************************************/
/********************************************************************************************************/
static boolean PQC_Initialized = FALSE;

/********************************************************************************************************/
/**********************************************FUNCTIONS**************************************************/
/********************************************************************************************************/

/**
 * @brief Initialize the PQC module
 */
Std_ReturnType PQC_Init(void)
{
    if (PQC_Initialized == TRUE)
    {
        return PQC_E_OK;
    }

    /* liboqs is stateless and doesn't require explicit initialization */
    /* Just verify that the algorithms we need are enabled */

    if (OQS_KEM_alg_is_enabled(OQS_KEM_alg_ml_kem_768) != 1)
    {
        (void)printf("ERROR: ML-KEM-768 is not enabled in liboqs\n");
        return PQC_E_NOT_OK;
    }

    if (OQS_SIG_alg_is_enabled(OQS_SIG_alg_ml_dsa_65) != 1)
    {
        (void)printf("ERROR: ML-DSA-65 is not enabled in liboqs\n");
        return PQC_E_NOT_OK;
    }

    PQC_Initialized = TRUE;
    (void)printf("PQC Module initialized successfully\n");
    (void)printf("  ML-KEM-768: Public Key=%u bytes, Ciphertext=%u bytes\n",
                 PQC_MLKEM_PUBLIC_KEY_BYTES, PQC_MLKEM_CIPHERTEXT_BYTES);
    (void)printf("  ML-DSA-65: Public Key=%u bytes, Signature=%u bytes\n",
                 PQC_MLDSA_PUBLIC_KEY_BYTES, PQC_MLDSA_SIGNATURE_BYTES);

    return PQC_E_OK;
}

/**
 * @brief Generate ML-KEM-768 key pair
 */
Std_ReturnType PQC_MLKEM_KeyGen(PQC_MLKEM_KeyPairType* KeyPair)
{
    OQS_KEM* kem = NULL;
    OQS_STATUS status;

    if (PQC_Initialized == FALSE)
    {
        (void)printf("ERROR: PQC not initialized\n");
        return PQC_E_NOT_OK;
    }

    if (KeyPair == NULL)
    {
        return PQC_E_INVALID_PARAM;
    }

    /* Create ML-KEM-768 context */
    kem = OQS_KEM_new(OQS_KEM_alg_ml_kem_768);
    if (kem == NULL)
    {
        (void)printf("ERROR: Failed to create ML-KEM-768 context\n");
        return PQC_E_NOT_OK;
    }

    /* Verify key sizes match our constants */
    if ((kem->length_public_key != PQC_MLKEM_PUBLIC_KEY_BYTES) ||
        (kem->length_secret_key != PQC_MLKEM_SECRET_KEY_BYTES))
    {
        (void)printf("ERROR: ML-KEM key size mismatch\n");
        OQS_KEM_free(kem);
        return PQC_E_NOT_OK;
    }

    /* Generate keypair */
    status = OQS_KEM_keypair(kem, KeyPair->PublicKey, KeyPair->SecretKey);

    OQS_KEM_free(kem);

    if (status != OQS_SUCCESS)
    {
        (void)printf("ERROR: ML-KEM keypair generation failed\n");
        return PQC_E_NOT_OK;
    }

    return PQC_E_OK;
}

/**
 * @brief ML-KEM-768 encapsulation
 */
Std_ReturnType PQC_MLKEM_Encapsulate(
    const uint8* PublicKey,
    PQC_MLKEM_SharedSecretType* SharedSecret)
{
    OQS_KEM* kem = NULL;
    OQS_STATUS status;

    if (PQC_Initialized == FALSE)
    {
        return PQC_E_NOT_OK;
    }

    if ((PublicKey == NULL) || (SharedSecret == NULL))
    {
        return PQC_E_INVALID_PARAM;
    }

    kem = OQS_KEM_new(OQS_KEM_alg_ml_kem_768);
    if (kem == NULL)
    {
        return PQC_E_NOT_OK;
    }

    /* Encapsulate to create shared secret and ciphertext */
    status = OQS_KEM_encaps(kem,
                            SharedSecret->Ciphertext,
                            SharedSecret->SharedSecret,
                            PublicKey);

    OQS_KEM_free(kem);

    if (status != OQS_SUCCESS)
    {
        (void)printf("ERROR: ML-KEM encapsulation failed\n");
        return PQC_E_NOT_OK;
    }

    return PQC_E_OK;
}

/**
 * @brief ML-KEM-768 decapsulation
 */
Std_ReturnType PQC_MLKEM_Decapsulate(
    const uint8* Ciphertext,
    const uint8* SecretKey,
    uint8* SharedSecret)
{
    OQS_KEM* kem = NULL;
    OQS_STATUS status;

    if (PQC_Initialized == FALSE)
    {
        return PQC_E_NOT_OK;
    }

    if ((Ciphertext == NULL) || (SecretKey == NULL) || (SharedSecret == NULL))
    {
        return PQC_E_INVALID_PARAM;
    }

    kem = OQS_KEM_new(OQS_KEM_alg_ml_kem_768);
    if (kem == NULL)
    {
        return PQC_E_NOT_OK;
    }

    /* Decapsulate to extract shared secret */
    status = OQS_KEM_decaps(kem,
                            SharedSecret,
                            Ciphertext,
                            SecretKey);

    OQS_KEM_free(kem);

    if (status != OQS_SUCCESS)
    {
        (void)printf("ERROR: ML-KEM decapsulation failed\n");
        return PQC_E_NOT_OK;
    }

    return PQC_E_OK;
}

/**
 * @brief Generate ML-DSA-65 key pair
 */
Std_ReturnType PQC_MLDSA_KeyGen(PQC_MLDSA_KeyPairType* KeyPair)
{
    OQS_SIG* sig = NULL;
    OQS_STATUS status;

    if (PQC_Initialized == FALSE)
    {
        (void)printf("ERROR: PQC not initialized\n");
        return PQC_E_NOT_OK;
    }

    if (KeyPair == NULL)
    {
        return PQC_E_INVALID_PARAM;
    }

    /* Create ML-DSA-65 context */
    sig = OQS_SIG_new(OQS_SIG_alg_ml_dsa_65);
    if (sig == NULL)
    {
        (void)printf("ERROR: Failed to create ML-DSA-65 context\n");
        return PQC_E_NOT_OK;
    }

    /* Verify key sizes match our constants */
    if ((sig->length_public_key != PQC_MLDSA_PUBLIC_KEY_BYTES) ||
        (sig->length_secret_key != PQC_MLDSA_SECRET_KEY_BYTES))
    {
        (void)printf("ERROR: ML-DSA key size mismatch\n");
        OQS_SIG_free(sig);
        return PQC_E_NOT_OK;
    }

    /* Generate keypair */
    status = OQS_SIG_keypair(sig, KeyPair->PublicKey, KeyPair->SecretKey);

    OQS_SIG_free(sig);

    if (status != OQS_SUCCESS)
    {
        (void)printf("ERROR: ML-DSA keypair generation failed\n");
        return PQC_E_NOT_OK;
    }

    return PQC_E_OK;
}

/**
 * @brief Ensure the key storage directory exists (Linux only)
 */
static Std_ReturnType PQC_EnsureKeyDirectory(void)
{
#if defined(LINUX)
    struct stat st;
    if (stat(PQC_MLDSA_KEY_DIRECTORY, &st) != 0)
    {
        /* Attempt to create directory with owner-only permissions */
        if (mkdir(PQC_MLDSA_KEY_DIRECTORY, 0700) != 0)
        {
            (void)printf("WARNING: Cannot create key directory %s (errno=%d)\n",
                         PQC_MLDSA_KEY_DIRECTORY, errno);
            return PQC_E_NOT_OK;
        }
        (void)printf("Created key directory: %s\n", PQC_MLDSA_KEY_DIRECTORY);
    }
#endif
    return PQC_E_OK;
}

/**
 * @brief Save ML-DSA-65 key pair to files
 * @details Keys are stored in PQC_MLDSA_KEY_DIRECTORY (configurable).
 */
Std_ReturnType PQC_MLDSA_SaveKeys(const PQC_MLDSA_KeyPairType* KeyPair, const char* filePrefix)
{
    FILE* fp;
    char filename[512];
    size_t written;

    if ((KeyPair == NULL) || (filePrefix == NULL))
    {
        return PQC_E_INVALID_PARAM;
    }

    /* Ensure key directory exists */
    (void)PQC_EnsureKeyDirectory();

    /* Save public key */
    (void)snprintf(filename, sizeof(filename), "%s%s.pub",
                   PQC_MLDSA_KEY_DIRECTORY, filePrefix);
    fp = fopen(filename, "wb");
    if (fp == NULL)
    {
        (void)printf("ERROR: Failed to create public key file: %s\n", filename);
        return PQC_E_NOT_OK;
    }
    written = fwrite(KeyPair->PublicKey, 1, PQC_MLDSA_PUBLIC_KEY_BYTES, fp);
    (void)fclose(fp);
    if (written != PQC_MLDSA_PUBLIC_KEY_BYTES)
    {
        (void)printf("ERROR: Failed to write complete public key\n");
        return PQC_E_NOT_OK;
    }

    /* Save secret key */
    (void)snprintf(filename, sizeof(filename), "%s%s.key",
                   PQC_MLDSA_KEY_DIRECTORY, filePrefix);
    fp = fopen(filename, "wb");
    if (fp == NULL)
    {
        (void)printf("ERROR: Failed to create secret key file: %s\n", filename);
        return PQC_E_NOT_OK;
    }
    written = fwrite(KeyPair->SecretKey, 1, PQC_MLDSA_SECRET_KEY_BYTES, fp);
    (void)fclose(fp);
    if (written != PQC_MLDSA_SECRET_KEY_BYTES)
    {
        (void)printf("ERROR: Failed to write complete secret key\n");
        return PQC_E_NOT_OK;
    }

    (void)printf("ML-DSA keys saved to %s%s.pub and %s%s.key\n",
                 PQC_MLDSA_KEY_DIRECTORY, filePrefix,
                 PQC_MLDSA_KEY_DIRECTORY, filePrefix);
    return PQC_E_OK;
}

/**
 * @brief Load ML-DSA-65 key pair from files
 * @details Keys are loaded from PQC_MLDSA_KEY_DIRECTORY (configurable).
 */
Std_ReturnType PQC_MLDSA_LoadKeys(PQC_MLDSA_KeyPairType* KeyPair, const char* filePrefix)
{
    FILE* fp;
    char filename[512];
    size_t read_bytes;

    if ((KeyPair == NULL) || (filePrefix == NULL))
    {
        return PQC_E_INVALID_PARAM;
    }

    /* Load public key from configured directory */
    (void)snprintf(filename, sizeof(filename), "%s%s.pub",
                   PQC_MLDSA_KEY_DIRECTORY, filePrefix);
    fp = fopen(filename, "rb");
    if (fp == NULL)
    {
        /* File doesn't exist - not an error, caller will generate new keys */
        return PQC_E_NOT_OK;
    }
    read_bytes = fread(KeyPair->PublicKey, 1, PQC_MLDSA_PUBLIC_KEY_BYTES, fp);
    (void)fclose(fp);
    if (read_bytes != PQC_MLDSA_PUBLIC_KEY_BYTES)
    {
        (void)printf("ERROR: Failed to read complete public key from %s\n", filename);
        return PQC_E_NOT_OK;
    }

    /* Load secret key from configured directory */
    (void)snprintf(filename, sizeof(filename), "%s%s.key",
                   PQC_MLDSA_KEY_DIRECTORY, filePrefix);
    fp = fopen(filename, "rb");
    if (fp == NULL)
    {
        (void)printf("ERROR: Secret key file not found: %s\n", filename);
        return PQC_E_NOT_OK;
    }
    read_bytes = fread(KeyPair->SecretKey, 1, PQC_MLDSA_SECRET_KEY_BYTES, fp);
    (void)fclose(fp);
    if (read_bytes != PQC_MLDSA_SECRET_KEY_BYTES)
    {
        (void)printf("ERROR: Failed to read complete secret key from %s\n", filename);
        return PQC_E_NOT_OK;
    }

    (void)printf("ML-DSA keys loaded from %s%s.pub and %s%s.key\n",
                 PQC_MLDSA_KEY_DIRECTORY, filePrefix,
                 PQC_MLDSA_KEY_DIRECTORY, filePrefix);
    return PQC_E_OK;
}

/**
 * @brief Generate ML-DSA-65 digital signature
 */
Std_ReturnType PQC_MLDSA_Sign(
    const uint8* Message,
    uint32 MessageLength,
    const uint8* SecretKey,
    uint8* Signature,
    uint32* SignatureLength)
{
    OQS_SIG* sig = NULL;
    OQS_STATUS status;
    size_t sig_len = 0;

    if (PQC_Initialized == FALSE)
    {
        return PQC_E_NOT_OK;
    }

    if ((Message == NULL) || (SecretKey == NULL) || (Signature == NULL) || (SignatureLength == NULL))
    {
        return PQC_E_INVALID_PARAM;
    }

    sig = OQS_SIG_new(OQS_SIG_alg_ml_dsa_65);
    if (sig == NULL)
    {
        return PQC_E_NOT_OK;
    }

    /* Sign the message */
    status = OQS_SIG_sign(sig,
                          Signature,
                          &sig_len,
                          Message,
                          (size_t)MessageLength,
                          SecretKey);

    OQS_SIG_free(sig);

    if (status != OQS_SUCCESS)
    {
        (void)printf("ERROR: ML-DSA signature generation failed\n");
        return PQC_E_NOT_OK;
    }

    *SignatureLength = (uint32)sig_len;

    return PQC_E_OK;
}

/**
 * @brief Verify ML-DSA-65 digital signature
 */
Std_ReturnType PQC_MLDSA_Verify(
    const uint8* Message,
    uint32 MessageLength,
    const uint8* Signature,
    uint32 SignatureLength,
    const uint8* PublicKey)
{
    OQS_SIG* sig = NULL;
    OQS_STATUS status;

    if (PQC_Initialized == FALSE)
    {
        return PQC_E_NOT_OK;
    }

    if ((Message == NULL) || (Signature == NULL) || (PublicKey == NULL))
    {
        return PQC_E_INVALID_PARAM;
    }

    sig = OQS_SIG_new(OQS_SIG_alg_ml_dsa_65);
    if (sig == NULL)
    {
        return PQC_E_NOT_OK;
    }

    /* Verify the signature */
    status = OQS_SIG_verify(sig,
                            Message,
                            (size_t)MessageLength,
                            Signature,
                            (size_t)SignatureLength,
                            PublicKey);

    OQS_SIG_free(sig);

    if (status != OQS_SUCCESS)
    {
        /* Signature verification failed */
        return PQC_E_VERIFY_FAILED;
    }

    return PQC_E_OK;
}
