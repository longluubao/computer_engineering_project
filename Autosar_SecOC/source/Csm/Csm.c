
/********************************************************************************************************/
/************************************************INCULDES************************************************/
/********************************************************************************************************/

#include "Csm.h"

#include <string.h>
#include "encrypt.h"
#include "PQC.h"
#include <stdio.h>

/********************************************************************************************************/
/*********************************************Defines****************************************************/
/********************************************************************************************************/

#define MAC_DATA_LEN ((uint8)100) /* Maximum number of stored MACs*/
#define MAC_LEN      ((uint8)16)      /* Length of MAC*/
#define MAX_MAC_BUFFER  32


/********************************************************************************************************/
/******************************************GlobalVaribles************************************************/
/********************************************************************************************************/

static uint8 MacData[MAC_DATA_LEN][MAC_LEN]; /* An array to store generated macs, uses jobId as an index*/

/* PQC Key Pairs - In production, these should be securely stored and managed */
static PQC_MLDSA_KeyPairType Csm_MLDSA_KeyPair;
static boolean Csm_PQC_Initialized = FALSE;



/********************************************************************************************************/
/********************************************Functions***************************************************/
/********************************************************************************************************/

/* A stub function to generate a MAC for the authentic-PDU*/
extern Std_ReturnType Csm_MacGenerate ( 
    uint32 jobId, 
    Crypto_OperationModeType mode,
    const uint8* dataPtr,
    uint32 dataLength,
    uint8* macPtr,
    uint32* macLengthPtr )
    {
        #ifdef MAC_DEBUG
            printf("######## in Csm_MacGenerate\n");
        #endif
        startEncryption(dataPtr , dataLength , macPtr ,macLengthPtr);
        #ifdef MAC_DEBUG
        printf("\nMac ");
        for(int i = 0; i < *macLengthPtr; i++)
        {
            printf("%d ", macPtr[i]);
        }
        printf("\n");
        #endif
        return E_OK;
    }



Std_ReturnType Csm_MacVerify(uint32 jobId, Crypto_OperationModeType mode, const uint8* dataPtr, uint32 dataLength,
const uint8* macPtr, const uint32 macLength, Crypto_VerifyResultType* verifyPtr)
{
    #ifdef MAC_DEBUG
        printf("######## in Csm_MacVerify\n");
    #endif
    uint32 Maclen = BIT_TO_BYTES(macLength);
    uint8 Mac_from_data[MAX_MAC_BUFFER];

    Std_ReturnType Mac_status;
    Std_ReturnType result;

    Mac_status = Csm_MacGenerate(jobId, mode, dataPtr, dataLength, Mac_from_data, &Maclen);
    #ifdef MAC_DEBUG
        printf("data : ");
        for(int i = 0; i < dataLength; i++)
        {
            printf("%d ", dataPtr[i]);
        }
        printf("\nMac ");
        for(int i = 0; i < Maclen; i++)
        {
            printf("%d  --  %d || ", Mac_from_data[i], macPtr[i]);
        }
        printf("\n");
    #endif
    if ((Mac_status == E_OK) && (Maclen == (BIT_TO_BYTES(macLength)))) 
    {
        if ((memcmp(Mac_from_data, macPtr, Maclen)) == 0) 
        {
            result = E_OK;
            *verifyPtr = CRYPTO_E_VER_OK;
        }
        else 
        {
            result = E_NOT_OK;
            *verifyPtr = CRYPTO_E_VER_NOT_OK;
        }
    } 
    else 
    {
        result = E_NOT_OK;
        *verifyPtr = CRYPTO_E_VER_NOT_OK;
    }
    return result;
}

/********************************************************************************************************/
/**********************************PQC SIGNATURE FUNCTIONS***********************************************/
/********************************************************************************************************/

/**
 * @brief Initialize PQC keys if not already done
 */
static Std_ReturnType Csm_PQC_EnsureInit(void)
{
    Std_ReturnType result;

    if (Csm_PQC_Initialized == TRUE)
    {
        return E_OK;
    }

    /* Initialize PQC module */
    result = PQC_Init();
    if (result != PQC_E_OK)
    {
        printf("ERROR: Failed to initialize PQC module\n");
        return E_NOT_OK;
    }

    /* Generate ML-DSA key pair for signature operations */
    result = PQC_MLDSA_KeyGen(&Csm_MLDSA_KeyPair);
    if (result != PQC_E_OK)
    {
        printf("ERROR: Failed to generate ML-DSA key pair\n");
        return E_NOT_OK;
    }

    printf("CSM: ML-DSA-65 keys generated successfully\n");
    Csm_PQC_Initialized = TRUE;

    return E_OK;
}

/**
 * @brief Generate digital signature using ML-DSA-65
 */
Std_ReturnType Csm_SignatureGenerate(
    uint32 jobId,
    Crypto_OperationModeType mode,
    const uint8* dataPtr,
    uint32 dataLength,
    uint8* signaturePtr,
    uint32* signatureLengthPtr)
{
    Std_ReturnType result;

    /* Ensure PQC is initialized */
    result = Csm_PQC_EnsureInit();
    if (result != E_OK)
    {
        return E_NOT_OK;
    }

    /* Validate parameters */
    if (dataPtr == NULL || signaturePtr == NULL || signatureLengthPtr == NULL)
    {
        printf("ERROR: Csm_SignatureGenerate - Invalid parameters\n");
        return E_NOT_OK;
    }

    #ifdef MAC_DEBUG
        printf("######## in Csm_SignatureGenerate (ML-DSA-65)\n");
        printf("Data length: %u bytes\n", dataLength);
    #endif

    /* Generate signature using ML-DSA-65 */
    result = PQC_MLDSA_Sign(
        dataPtr,
        dataLength,
        Csm_MLDSA_KeyPair.SecretKey,
        signaturePtr,
        signatureLengthPtr
    );

    if (result != PQC_E_OK)
    {
        printf("ERROR: ML-DSA signature generation failed\n");
        return E_NOT_OK;
    }

    #ifdef MAC_DEBUG
        printf("Signature generated: %u bytes\n", *signatureLengthPtr);
        printf("First 16 bytes: ");
        for(uint32 i = 0; i < 16 && i < *signatureLengthPtr; i++)
        {
            printf("%02x ", signaturePtr[i]);
        }
        printf("\n");
    #endif

    return E_OK;
}

/**
 * @brief Verify digital signature using ML-DSA-65
 */
Std_ReturnType Csm_SignatureVerify(
    uint32 jobId,
    Crypto_OperationModeType mode,
    const uint8* dataPtr,
    uint32 dataLength,
    const uint8* signaturePtr,
    uint32 signatureLength,
    Crypto_VerifyResultType* verifyPtr)
{
    Std_ReturnType result;

    /* Ensure PQC is initialized */
    result = Csm_PQC_EnsureInit();
    if (result != E_OK)
    {
        return E_NOT_OK;
    }

    /* Validate parameters */
    if (dataPtr == NULL || signaturePtr == NULL || verifyPtr == NULL)
    {
        printf("ERROR: Csm_SignatureVerify - Invalid parameters\n");
        return E_NOT_OK;
    }

    #ifdef MAC_DEBUG
        printf("######## in Csm_SignatureVerify (ML-DSA-65)\n");
        printf("Data length: %u bytes, Signature length: %u bytes\n",
               dataLength, signatureLength);
    #endif

    /* Verify signature using ML-DSA-65 */
    result = PQC_MLDSA_Verify(
        dataPtr,
        dataLength,
        signaturePtr,
        signatureLength,
        Csm_MLDSA_KeyPair.PublicKey
    );

    if (result == PQC_E_OK)
    {
        /* Signature is valid */
        *verifyPtr = CRYPTO_E_VER_OK;
        #ifdef MAC_DEBUG
            printf("Signature verification: PASSED\n");
        #endif
        return E_OK;
    }
    else if (result == PQC_E_VERIFY_FAILED)
    {
        /* Signature is invalid */
        *verifyPtr = CRYPTO_E_VER_NOT_OK;
        #ifdef MAC_DEBUG
            printf("Signature verification: FAILED\n");
        #endif
        return E_NOT_OK;
    }
    else
    {
        /* Error during verification */
        *verifyPtr = CRYPTO_E_VER_NOT_OK;
        printf("ERROR: ML-DSA signature verification error\n");
        return E_NOT_OK;
    }
}
