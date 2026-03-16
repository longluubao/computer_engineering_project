/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "Csm.h"

#include <string.h>
#include <stdio.h>

#include "encrypt.h"
#include "PQC.h"
#include "PQC_KeyExchange.h"
#include "PQC_KeyDerivation.h"

/********************************************************************************************************/
/*********************************************Defines****************************************************/
/********************************************************************************************************/

#define CSM_VENDOR_ID                         ((uint16)0xFFFFU)
#define CSM_MODULE_ID                         ((uint16)0x0110U)
#define CSM_SW_MAJOR_VERSION                  ((uint8)1U)
#define CSM_SW_MINOR_VERSION                  ((uint8)0U)
#define CSM_SW_PATCH_VERSION                  ((uint8)0U)

#define CSM_KEYID_IS_SESSION(id)              (((id) >= CSM_KEYID_SESSION_BASE) && ((id) < (CSM_KEYID_SESSION_BASE + (uint32)PQC_MAX_PEERS)))
#define CSM_SESSION_PEER_FROM_KEYID(id)       ((uint8)((id) - CSM_KEYID_SESSION_BASE))

/********************************************************************************************************/
/*******************************************TypeDefinitions**********************************************/
/********************************************************************************************************/

typedef struct
{
    boolean CsmInUse;
    uint32 CsmJobId;
    Csm_JobStateType CsmJobState;
    uint32 CsmStreamLength;
    uint8 CsmStreamBuffer[CSM_MAX_STREAM_BUFFER];
} Csm_JobContextType;

typedef struct
{
    boolean CsmMldsaPublicKeyValid;
    boolean CsmMldsaSecretKeyValid;
    uint8 CsmMldsaPublicKey[PQC_MLDSA_PUBLIC_KEY_BYTES];
    uint8 CsmMldsaSecretKey[PQC_MLDSA_SECRET_KEY_BYTES];
} Csm_KeyStoreType;

/********************************************************************************************************/
/******************************************GlobalVariables***********************************************/
/********************************************************************************************************/

static boolean Csm_IsInitialized = FALSE;
static Csm_JobContextType Csm_JobContexts[CSM_MAX_JOB_CONTEXTS];
static uint32 Csm_JobQueue[CSM_MAX_JOB_QUEUE];
static uint8 Csm_JobQueueHead = 0U;
static uint8 Csm_JobQueueTail = 0U;
static uint8 Csm_JobQueueCount = 0U;
static Csm_KeyStoreType Csm_KeyStore;
static PQC_SessionKeysType Csm_SessionKeys[PQC_MAX_PEERS];
static PQC_MLDSA_KeyPairType Csm_MLDSA_KeyPair;
static boolean Csm_PqcReady = FALSE;

/********************************************************************************************************/
/****************************************StaticFunctions*************************************************/
/********************************************************************************************************/

static void Csm_ResetState(void)
{
    (void)memset(Csm_JobContexts, 0, sizeof(Csm_JobContexts));
    (void)memset(Csm_JobQueue, 0, sizeof(Csm_JobQueue));
    Csm_JobQueueHead = 0U;
    Csm_JobQueueTail = 0U;
    Csm_JobQueueCount = 0U;
    (void)memset(&Csm_KeyStore, 0, sizeof(Csm_KeyStore));
    (void)memset(Csm_SessionKeys, 0, sizeof(Csm_SessionKeys));
    (void)memset(&Csm_MLDSA_KeyPair, 0, sizeof(Csm_MLDSA_KeyPair));
    Csm_PqcReady = FALSE;
}

static Std_ReturnType Csm_EnqueueJob(uint32 jobId)
{
    uint8 idx;

    for (idx = 0U; idx < Csm_JobQueueCount; idx++)
    {
        uint8 queueIdx = (uint8)((Csm_JobQueueHead + idx) % CSM_MAX_JOB_QUEUE);
        if (Csm_JobQueue[queueIdx] == jobId)
        {
            return E_OK;
        }
    }

    if (Csm_JobQueueCount >= CSM_MAX_JOB_QUEUE)
    {
        return CRYPTO_E_QUEUE_FULL;
    }

    Csm_JobQueue[Csm_JobQueueTail] = jobId;
    Csm_JobQueueTail = (uint8)((Csm_JobQueueTail + 1U) % CSM_MAX_JOB_QUEUE);
    Csm_JobQueueCount++;
    return E_OK;
}

static Std_ReturnType Csm_DequeueJob(uint32* jobIdPtr)
{
    if ((jobIdPtr == NULL) || (Csm_JobQueueCount == 0U))
    {
        return E_NOT_OK;
    }

    *jobIdPtr = Csm_JobQueue[Csm_JobQueueHead];
    Csm_JobQueueHead = (uint8)((Csm_JobQueueHead + 1U) % CSM_MAX_JOB_QUEUE);
    Csm_JobQueueCount--;
    return E_OK;
}

static Csm_JobContextType* Csm_GetJobContext(uint32 jobId)
{
    uint8 idx;

    for (idx = 0U; idx < CSM_MAX_JOB_CONTEXTS; idx++)
    {
        if ((Csm_JobContexts[idx].CsmInUse == TRUE) && (Csm_JobContexts[idx].CsmJobId == jobId))
        {
            return &Csm_JobContexts[idx];
        }
    }

    for (idx = 0U; idx < CSM_MAX_JOB_CONTEXTS; idx++)
    {
        if (Csm_JobContexts[idx].CsmInUse == FALSE)
        {
            Csm_JobContexts[idx].CsmInUse = TRUE;
            Csm_JobContexts[idx].CsmJobId = jobId;
            Csm_JobContexts[idx].CsmJobState = CSM_JOBSTATE_IDLE;
            Csm_JobContexts[idx].CsmStreamLength = 0U;
            return &Csm_JobContexts[idx];
        }
    }

    return NULL;
}

static Std_ReturnType Csm_AppendStreamData(Csm_JobContextType* contextPtr, const uint8* dataPtr, uint32 dataLength)
{
    if ((contextPtr == NULL) || ((dataPtr == NULL) && (dataLength > 0U)))
    {
        return E_NOT_OK;
    }

    if ((contextPtr->CsmStreamLength + dataLength) > CSM_MAX_STREAM_BUFFER)
    {
        return E_NOT_OK;
    }

    if (dataLength > 0U)
    {
        (void)memcpy(&contextPtr->CsmStreamBuffer[contextPtr->CsmStreamLength], dataPtr, dataLength);
        contextPtr->CsmStreamLength += dataLength;
    }

    return E_OK;
}

static Std_ReturnType Csm_CheckAndHandleBusy(Csm_JobContextType* contextPtr)
{
    Std_ReturnType queueResult;

    if ((contextPtr == NULL) || (contextPtr->CsmJobState != CSM_JOBSTATE_ACTIVE))
    {
        return E_OK;
    }

    queueResult = Csm_EnqueueJob(contextPtr->CsmJobId);
    if (queueResult == E_OK)
    {
        contextPtr->CsmJobState = CSM_JOBSTATE_QUEUED;
        return E_BUSY;
    }

    return CRYPTO_E_QUEUE_FULL;
}

static Std_ReturnType Csm_PQC_EnsureReady(void)
{
    Std_ReturnType result;

    if (Csm_PqcReady == TRUE)
    {
        return E_OK;
    }

    result = PQC_Init();
    if (result != PQC_E_OK)
    {
        printf("ERROR: CSM PQC init failed\n");
        return E_NOT_OK;
    }

    if (PQC_KeyExchange_Init() != E_OK)
    {
        printf("ERROR: CSM key-exchange manager init failed\n");
        return E_NOT_OK;
    }

    if (PQC_KeyDerivation_Init() != E_OK)
    {
        printf("ERROR: CSM key-derivation init failed\n");
        return E_NOT_OK;
    }

    result = PQC_MLDSA_LoadKeys(&Csm_MLDSA_KeyPair, "mldsa_secoc");
    if (result != PQC_E_OK)
    {
        result = PQC_MLDSA_KeyGen(&Csm_MLDSA_KeyPair);
        if (result != PQC_E_OK)
        {
            printf("ERROR: CSM ML-DSA key generation failed\n");
            return E_NOT_OK;
        }
        (void)PQC_MLDSA_SaveKeys(&Csm_MLDSA_KeyPair, "mldsa_secoc");
    }

    (void)memcpy(Csm_KeyStore.CsmMldsaPublicKey, Csm_MLDSA_KeyPair.PublicKey, PQC_MLDSA_PUBLIC_KEY_BYTES);
    (void)memcpy(Csm_KeyStore.CsmMldsaSecretKey, Csm_MLDSA_KeyPair.SecretKey, PQC_MLDSA_SECRET_KEY_BYTES);
    Csm_KeyStore.CsmMldsaPublicKeyValid = TRUE;
    Csm_KeyStore.CsmMldsaSecretKeyValid = TRUE;
    Csm_PqcReady = TRUE;
    return E_OK;
}

static Std_ReturnType Csm_PrepareOperationInput(
    Csm_JobContextType* contextPtr,
    Crypto_OperationModeType mode,
    const uint8* dataPtr,
    uint32 dataLength,
    const uint8** preparedDataPtr,
    uint32* preparedLengthPtr)
{
    if ((contextPtr == NULL) || (preparedDataPtr == NULL) || (preparedLengthPtr == NULL))
    {
        return E_NOT_OK;
    }

    if (mode == CRYPTO_OPERATIONMODE_SINGLECALL)
    {
        *preparedDataPtr = dataPtr;
        *preparedLengthPtr = dataLength;
        return E_OK;
    }

    if (mode == CRYPTO_OPERATIONMODE_START)
    {
        contextPtr->CsmStreamLength = 0U;
        contextPtr->CsmJobState = CSM_JOBSTATE_ACTIVE;
        return Csm_AppendStreamData(contextPtr, dataPtr, dataLength);
    }

    if (mode == CRYPTO_OPERATIONMODE_UPDATE)
    {
        if (contextPtr->CsmJobState != CSM_JOBSTATE_ACTIVE)
        {
            return E_NOT_OK;
        }
        return Csm_AppendStreamData(contextPtr, dataPtr, dataLength);
    }

    if (mode == CRYPTO_OPERATIONMODE_FINISH)
    {
        if (contextPtr->CsmJobState != CSM_JOBSTATE_ACTIVE)
        {
            return E_NOT_OK;
        }

        if (Csm_AppendStreamData(contextPtr, dataPtr, dataLength) != E_OK)
        {
            return E_NOT_OK;
        }

        *preparedDataPtr = contextPtr->CsmStreamBuffer;
        *preparedLengthPtr = contextPtr->CsmStreamLength;
        contextPtr->CsmStreamLength = 0U;
        contextPtr->CsmJobState = CSM_JOBSTATE_IDLE;
        return E_OK;
    }

    return E_NOT_OK;
}

/********************************************************************************************************/
/********************************************PublicFunctions*********************************************/
/********************************************************************************************************/

void Csm_Init(const Csm_ConfigType* configPtr)
{
    (void)configPtr;
    Csm_ResetState();
    if (Csm_PQC_EnsureReady() == E_OK)
    {
        Csm_IsInitialized = TRUE;
    }
    else
    {
        Csm_IsInitialized = FALSE;
    }
}

void Csm_DeInit(void)
{
    uint8 peerId;

    for (peerId = 0U; peerId < PQC_MAX_PEERS; peerId++)
    {
        (void)PQC_KeyExchange_Reset(peerId);
        (void)PQC_ClearSessionKeys(peerId);
    }

    Csm_ResetState();
    Csm_IsInitialized = FALSE;
}

void Csm_MainFunction(void)
{
    uint32 queuedJobId = 0U;
    Csm_JobContextType* contextPtr;

    if (Csm_IsInitialized == FALSE)
    {
        return;
    }

    if (Csm_DequeueJob(&queuedJobId) == E_OK)
    {
        contextPtr = Csm_GetJobContext(queuedJobId);
        if (contextPtr != NULL)
        {
            contextPtr->CsmJobState = CSM_JOBSTATE_IDLE;
        }
    }
}

void Csm_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    if (versioninfo == NULL)
    {
        return;
    }

    versioninfo->vendorID = CSM_VENDOR_ID;
    versioninfo->moduleID = CSM_MODULE_ID;
    versioninfo->sw_major_version = CSM_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = CSM_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = CSM_SW_PATCH_VERSION;
}

Std_ReturnType Csm_MacGenerate(
    uint32 jobId,
    Crypto_OperationModeType mode,
    const uint8* dataPtr,
    uint32 dataLength,
    uint8* macPtr,
    uint32* macLengthPtr)
{
    Csm_JobContextType* contextPtr;
    const uint8* preparedDataPtr = NULL;
    uint32 preparedLength = 0U;
    Std_ReturnType prepareResult;
    Std_ReturnType busyResult;

    if ((Csm_IsInitialized == FALSE) || (macPtr == NULL) || (macLengthPtr == NULL))
    {
        return E_NOT_OK;
    }

    contextPtr = Csm_GetJobContext(jobId);
    if (contextPtr == NULL)
    {
        return CRYPTO_E_QUEUE_FULL;
    }

    if ((mode == CRYPTO_OPERATIONMODE_SINGLECALL) && (contextPtr->CsmJobState == CSM_JOBSTATE_ACTIVE))
    {
        busyResult = Csm_CheckAndHandleBusy(contextPtr);
        return busyResult;
    }

    prepareResult = Csm_PrepareOperationInput(
        contextPtr,
        mode,
        dataPtr,
        dataLength,
        &preparedDataPtr,
        &preparedLength);

    if ((mode == CRYPTO_OPERATIONMODE_START) || (mode == CRYPTO_OPERATIONMODE_UPDATE))
    {
        return prepareResult;
    }

    if ((prepareResult != E_OK) || (preparedDataPtr == NULL))
    {
        return E_NOT_OK;
    }

    startEncryption(preparedDataPtr, preparedLength, macPtr, macLengthPtr);
    return E_OK;
}

Std_ReturnType Csm_MacVerify(
    uint32 jobId,
    Crypto_OperationModeType mode,
    const uint8* dataPtr,
    uint32 dataLength,
    const uint8* macPtr,
    const uint32 macLength,
    Crypto_VerifyResultType* verifyPtr)
{
    uint32 generatedMacLength = BIT_TO_BYTES(macLength);
    uint8 generatedMacBuffer[32];
    Std_ReturnType generateResult;

    if ((verifyPtr == NULL) || (macPtr == NULL))
    {
        return E_NOT_OK;
    }

    generateResult = Csm_MacGenerate(
        jobId,
        mode,
        dataPtr,
        dataLength,
        generatedMacBuffer,
        &generatedMacLength);

    if (generateResult != E_OK)
    {
        *verifyPtr = CRYPTO_E_VER_NOT_OK;
        return generateResult;
    }

    if ((generatedMacLength == BIT_TO_BYTES(macLength)) &&
        (memcmp(generatedMacBuffer, macPtr, generatedMacLength) == 0))
    {
        *verifyPtr = CRYPTO_E_VER_OK;
        return E_OK;
    }

    *verifyPtr = CRYPTO_E_VER_NOT_OK;
    return E_NOT_OK;
}

Std_ReturnType Csm_SignatureGenerate(
    uint32 jobId,
    Crypto_OperationModeType mode,
    const uint8* dataPtr,
    uint32 dataLength,
    uint8* signaturePtr,
    uint32* signatureLengthPtr)
{
    Csm_JobContextType* contextPtr;
    const uint8* preparedDataPtr = NULL;
    uint32 preparedLength = 0U;
    Std_ReturnType prepareResult;
    Std_ReturnType busyResult;

    if ((Csm_IsInitialized == FALSE) || (signaturePtr == NULL) || (signatureLengthPtr == NULL))
    {
        return E_NOT_OK;
    }

    if ((Csm_PQC_EnsureReady() != E_OK) || (Csm_KeyStore.CsmMldsaSecretKeyValid == FALSE))
    {
        return CRYPTO_E_KEY_NOT_VALID;
    }

    contextPtr = Csm_GetJobContext(jobId);
    if (contextPtr == NULL)
    {
        return CRYPTO_E_QUEUE_FULL;
    }

    if ((mode == CRYPTO_OPERATIONMODE_SINGLECALL) && (contextPtr->CsmJobState == CSM_JOBSTATE_ACTIVE))
    {
        busyResult = Csm_CheckAndHandleBusy(contextPtr);
        return busyResult;
    }

    prepareResult = Csm_PrepareOperationInput(
        contextPtr,
        mode,
        dataPtr,
        dataLength,
        &preparedDataPtr,
        &preparedLength);

    if ((mode == CRYPTO_OPERATIONMODE_START) || (mode == CRYPTO_OPERATIONMODE_UPDATE))
    {
        return prepareResult;
    }

    if ((prepareResult != E_OK) || (preparedDataPtr == NULL))
    {
        return E_NOT_OK;
    }

    if (PQC_MLDSA_Sign(
            preparedDataPtr,
            preparedLength,
            Csm_KeyStore.CsmMldsaSecretKey,
            signaturePtr,
            signatureLengthPtr) != PQC_E_OK)
    {
        return E_NOT_OK;
    }

    return E_OK;
}

Std_ReturnType Csm_SignatureVerify(
    uint32 jobId,
    Crypto_OperationModeType mode,
    const uint8* dataPtr,
    uint32 dataLength,
    const uint8* signaturePtr,
    uint32 signatureLength,
    Crypto_VerifyResultType* verifyPtr)
{
    Csm_JobContextType* contextPtr;
    const uint8* preparedDataPtr = NULL;
    uint32 preparedLength = 0U;
    Std_ReturnType prepareResult;
    Std_ReturnType verifyResult;

    if ((Csm_IsInitialized == FALSE) || (verifyPtr == NULL) || (signaturePtr == NULL))
    {
        return E_NOT_OK;
    }

    if ((Csm_PQC_EnsureReady() != E_OK) || (Csm_KeyStore.CsmMldsaPublicKeyValid == FALSE))
    {
        *verifyPtr = CRYPTO_E_VER_NOT_OK;
        return CRYPTO_E_KEY_NOT_VALID;
    }

    contextPtr = Csm_GetJobContext(jobId);
    if (contextPtr == NULL)
    {
        *verifyPtr = CRYPTO_E_VER_NOT_OK;
        return CRYPTO_E_QUEUE_FULL;
    }

    prepareResult = Csm_PrepareOperationInput(
        contextPtr,
        mode,
        dataPtr,
        dataLength,
        &preparedDataPtr,
        &preparedLength);

    if ((mode == CRYPTO_OPERATIONMODE_START) || (mode == CRYPTO_OPERATIONMODE_UPDATE))
    {
        return prepareResult;
    }

    if ((prepareResult != E_OK) || (preparedDataPtr == NULL))
    {
        *verifyPtr = CRYPTO_E_VER_NOT_OK;
        return E_NOT_OK;
    }

    verifyResult = PQC_MLDSA_Verify(
        preparedDataPtr,
        preparedLength,
        signaturePtr,
        signatureLength,
        Csm_KeyStore.CsmMldsaPublicKey);

    if (verifyResult == PQC_E_OK)
    {
        *verifyPtr = CRYPTO_E_VER_OK;
        return E_OK;
    }

    *verifyPtr = CRYPTO_E_VER_NOT_OK;
    return E_NOT_OK;
}

Std_ReturnType Csm_KeyElementSet(
    uint32 keyId,
    uint32 keyElementId,
    const uint8* keyPtr,
    uint32 keyLength)
{
    uint8 peerId;
    PQC_SessionKeysType sessionKeys;

    if ((Csm_IsInitialized == FALSE) || (keyPtr == NULL))
    {
        return E_NOT_OK;
    }

    if (keyId == CSM_KEYID_MLDSA_LOCAL)
    {
        if ((keyElementId == CSM_KEYELEMENT_MLDSA_PUBLIC) && (keyLength == PQC_MLDSA_PUBLIC_KEY_BYTES))
        {
            (void)memcpy(Csm_KeyStore.CsmMldsaPublicKey, keyPtr, PQC_MLDSA_PUBLIC_KEY_BYTES);
            Csm_KeyStore.CsmMldsaPublicKeyValid = FALSE;
            return E_OK;
        }

        if ((keyElementId == CSM_KEYELEMENT_MLDSA_SECRET) && (keyLength == PQC_MLDSA_SECRET_KEY_BYTES))
        {
            (void)memcpy(Csm_KeyStore.CsmMldsaSecretKey, keyPtr, PQC_MLDSA_SECRET_KEY_BYTES);
            Csm_KeyStore.CsmMldsaSecretKeyValid = FALSE;
            return E_OK;
        }
        return CRYPTO_E_KEY_SIZE_MISMATCH;
    }

    if (CSM_KEYID_IS_SESSION(keyId) == TRUE)
    {
        peerId = CSM_SESSION_PEER_FROM_KEYID(keyId);
        if (peerId >= PQC_MAX_PEERS)
        {
            return E_NOT_OK;
        }

        if (PQC_GetSessionKeys(peerId, &sessionKeys) != E_OK)
        {
            (void)memset(&sessionKeys, 0, sizeof(sessionKeys));
        }

        if ((keyElementId == CSM_KEYELEMENT_SESSION_ENC) && (keyLength == PQC_DERIVED_KEY_LENGTH))
        {
            (void)memcpy(sessionKeys.EncryptionKey, keyPtr, PQC_DERIVED_KEY_LENGTH);
        }
        else if ((keyElementId == CSM_KEYELEMENT_SESSION_AUTH) && (keyLength == PQC_DERIVED_KEY_LENGTH))
        {
            (void)memcpy(sessionKeys.AuthenticationKey, keyPtr, PQC_DERIVED_KEY_LENGTH);
        }
        else
        {
            return CRYPTO_E_KEY_SIZE_MISMATCH;
        }

        sessionKeys.IsValid = FALSE;
        Csm_SessionKeys[peerId] = sessionKeys;
        return E_OK;
    }

    return E_NOT_OK;
}

Std_ReturnType Csm_KeySetValid(uint32 keyId)
{
    uint8 peerId;
    PQC_SessionKeysType sessionKeys;

    if (Csm_IsInitialized == FALSE)
    {
        return E_NOT_OK;
    }

    if (keyId == CSM_KEYID_MLDSA_LOCAL)
    {
        Csm_KeyStore.CsmMldsaPublicKeyValid = TRUE;
        Csm_KeyStore.CsmMldsaSecretKeyValid = TRUE;
        return E_OK;
    }

    if (CSM_KEYID_IS_SESSION(keyId) == TRUE)
    {
        peerId = CSM_SESSION_PEER_FROM_KEYID(keyId);
        if (peerId >= PQC_MAX_PEERS)
        {
            return E_NOT_OK;
        }

        sessionKeys = Csm_SessionKeys[peerId];
        sessionKeys.IsValid = TRUE;
        Csm_SessionKeys[peerId] = sessionKeys;
        return E_OK;
    }

    return E_NOT_OK;
}

Std_ReturnType Csm_KeyExchangeInitiate(
    uint32 jobId,
    uint8 peerId,
    uint8* publicValuePtr,
    uint32* publicValueLengthPtr)
{
    (void)jobId;

    if ((Csm_IsInitialized == FALSE) || (publicValuePtr == NULL) || (publicValueLengthPtr == NULL))
    {
        return E_NOT_OK;
    }

    if (*publicValueLengthPtr < PQC_MLKEM_PUBLIC_KEY_BYTES)
    {
        return CRYPTO_E_KEY_SIZE_MISMATCH;
    }

    if (PQC_KeyExchange_Initiate(peerId, publicValuePtr) != E_OK)
    {
        return E_NOT_OK;
    }

    *publicValueLengthPtr = PQC_MLKEM_PUBLIC_KEY_BYTES;
    return E_OK;
}

Std_ReturnType Csm_KeyExchangeRespond(
    uint32 jobId,
    uint8 peerId,
    const uint8* partnerPublicValuePtr,
    uint32 partnerPublicValueLength,
    uint8* ciphertextPtr,
    uint32* ciphertextLengthPtr)
{
    (void)jobId;

    if ((Csm_IsInitialized == FALSE) || (partnerPublicValuePtr == NULL) || (ciphertextPtr == NULL) || (ciphertextLengthPtr == NULL))
    {
        return E_NOT_OK;
    }

    if ((partnerPublicValueLength != PQC_MLKEM_PUBLIC_KEY_BYTES) || (*ciphertextLengthPtr < PQC_MLKEM_CIPHERTEXT_BYTES))
    {
        return CRYPTO_E_KEY_SIZE_MISMATCH;
    }

    if (PQC_KeyExchange_Respond(peerId, partnerPublicValuePtr, ciphertextPtr) != E_OK)
    {
        return E_NOT_OK;
    }

    *ciphertextLengthPtr = PQC_MLKEM_CIPHERTEXT_BYTES;
    return E_OK;
}

Std_ReturnType Csm_KeyExchangeComplete(
    uint32 jobId,
    uint8 peerId,
    const uint8* ciphertextPtr,
    uint32 ciphertextLength)
{
    (void)jobId;

    if ((Csm_IsInitialized == FALSE) || (ciphertextPtr == NULL))
    {
        return E_NOT_OK;
    }

    if (ciphertextLength != PQC_MLKEM_CIPHERTEXT_BYTES)
    {
        return CRYPTO_E_KEY_SIZE_MISMATCH;
    }

    return PQC_KeyExchange_Complete(peerId, ciphertextPtr);
}

Std_ReturnType Csm_KeyExchangeReset(uint8 peerId)
{
    if ((Csm_IsInitialized == FALSE) || (peerId >= PQC_MAX_PEERS))
    {
        return E_NOT_OK;
    }

    return PQC_KeyExchange_Reset(peerId);
}

Std_ReturnType Csm_KeyExchangeGetSharedSecret(
    uint32 jobId,
    uint8 peerId,
    uint8* sharedSecretPtr,
    uint32* sharedSecretLengthPtr)
{
    (void)jobId;

    if ((Csm_IsInitialized == FALSE) || (sharedSecretPtr == NULL) || (sharedSecretLengthPtr == NULL))
    {
        return E_NOT_OK;
    }

    if (*sharedSecretLengthPtr < PQC_MLKEM_SHARED_SECRET_BYTES)
    {
        return CRYPTO_E_KEY_SIZE_MISMATCH;
    }

    if (PQC_KeyExchange_GetSharedSecret(peerId, sharedSecretPtr) != E_OK)
    {
        return E_NOT_OK;
    }

    *sharedSecretLengthPtr = PQC_MLKEM_SHARED_SECRET_BYTES;
    return E_OK;
}

Std_ReturnType Csm_DeriveSessionKeys(
    uint8 peerId,
    const uint8* sharedSecretPtr,
    uint32 sharedSecretLength)
{
    PQC_SessionKeysType sessionKeys;

    if ((Csm_IsInitialized == FALSE) || (sharedSecretPtr == NULL) || (sharedSecretLength != PQC_MLKEM_SHARED_SECRET_BYTES))
    {
        return E_NOT_OK;
    }

    if (PQC_DeriveSessionKeys(sharedSecretPtr, peerId, &sessionKeys) != E_OK)
    {
        return E_NOT_OK;
    }

    Csm_SessionKeys[peerId] = sessionKeys;
    return E_OK;
}

Std_ReturnType Csm_GetSessionKeys(
    uint8 peerId,
    PQC_SessionKeysType* sessionKeysPtr)
{
    if ((Csm_IsInitialized == FALSE) || (sessionKeysPtr == NULL) || (peerId >= PQC_MAX_PEERS))
    {
        return E_NOT_OK;
    }

    if (PQC_GetSessionKeys(peerId, sessionKeysPtr) == E_OK)
    {
        return E_OK;
    }

    if (Csm_SessionKeys[peerId].IsValid == TRUE)
    {
        *sessionKeysPtr = Csm_SessionKeys[peerId];
        return E_OK;
    }

    return E_NOT_OK;
}

Std_ReturnType Csm_ClearSessionKeys(uint8 peerId)
{
    if ((Csm_IsInitialized == FALSE) || (peerId >= PQC_MAX_PEERS))
    {
        return E_NOT_OK;
    }

    (void)memset(&Csm_SessionKeys[peerId], 0, sizeof(PQC_SessionKeysType));
    return PQC_ClearSessionKeys(peerId);
}
