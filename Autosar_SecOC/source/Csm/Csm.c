/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "CryIf/CryIf.h"
#include "Csm/Csm.h"
#include "SecOC/SecOC_Cfg.h"

#include <string.h>
#include <stdio.h>

/* MISRA C:2012 Rule 17.3 - Cross-module forward declarations */
extern Std_ReturnType CryIf_SignatureVerify(CryIf_ProviderType provider,
                                           const uint8* dataPtr, uint32 dataLength,
                                           const uint8* signaturePtr, uint32 signatureLength,
                                           const uint8* publicKeyPtr);
extern Std_ReturnType CryIf_GetSessionKeys(uint8 peerId, PQC_SessionKeysType* sessionKeysPtr);
extern Std_ReturnType CryIf_KeyExchangeInitiate(uint8 peerId, uint8* publicValuePtr);
extern Std_ReturnType CryIf_KeyExchangeRespond(uint8 peerId, const uint8* partnerPublicValuePtr,
                                               uint8* ciphertextPtr);
extern Std_ReturnType CryIf_KeyExchangeGetSharedSecret(uint8 peerId, uint8* sharedSecretPtr);
extern Std_ReturnType CryIf_DeriveSessionKeys(const uint8* sharedSecretPtr, uint8 peerId,
                                              PQC_SessionKeysType* sessionKeysPtr);

/********************************************************************************************************/
/*********************************************Defines****************************************************/
/********************************************************************************************************/

#define CSM_VENDOR_ID                         ((uint16)0xFFFFU)
#define CSM_MODULE_ID                         ((uint16)0x0110U)
#define CSM_SW_MAJOR_VERSION                  ((uint8)1U)
#define CSM_SW_MINOR_VERSION                  ((uint8)0U)
#define CSM_SW_PATCH_VERSION                  ((uint8)0U)

#define CSM_KEYID_IS_SESSION(id)              (((id) >= CSM_KEYID_SESSION_BASE) && ((id) < (CSM_KEYID_SESSION_BASE + (uint32)CSM_MAX_PEERS)))
#define CSM_SESSION_PEER_FROM_KEYID(id)       ((uint8)((id) - CSM_KEYID_SESSION_BASE))

typedef enum
{
    CSM_PENDING_NONE = 0U,
    CSM_PENDING_MAC_GENERATE,
    CSM_PENDING_SIGNATURE_GENERATE,
    CSM_PENDING_SIGNATURE_VERIFY
} Csm_PendingOperationType;

typedef struct
{
    Csm_JobCallbackType CsmJobCallbackFct;
    void* CsmJobCallbackContextPtr;
} Csm_JobCallbackConfigType;

typedef struct
{
    uint32 CsmJobId;
    CryIf_ProviderType CsmMacProvider;
    CryIf_ProviderType CsmSignatureProvider;
    CryIf_ProviderType CsmKeyExchangeProvider;
} Csm_JobRoutingType;

/********************************************************************************************************/
/**************************************ForwardDeclarations***********************************************/
/********************************************************************************************************/

extern void Csm_Init(const Csm_ConfigType* configPtr);
extern void Csm_DeInit(void);
extern void Csm_MainFunction(void);
extern Std_ReturnType Csm_RegisterJobCallback(uint32 jobId, Csm_JobCallbackType callbackFct, void* callbackContextPtr);
extern Std_ReturnType Csm_GetJobResult(uint32 jobId, Std_ReturnType* resultPtr, boolean* completedPtr);
extern void Csm_GetVersionInfo(Std_VersionInfoType* versioninfo);
extern Std_ReturnType Csm_MacGenerate(uint32 jobId, Crypto_OperationModeType mode,
                                      const uint8* dataPtr, uint32 dataLength,
                                      uint8* macPtr, uint32* macLengthPtr);
extern Std_ReturnType Csm_MacVerify(uint32 jobId, Crypto_OperationModeType mode,
                                    const uint8* dataPtr, uint32 dataLength,
                                    const uint8* macPtr, const uint32 macLength,
                                    Crypto_VerifyResultType* verifyPtr);
extern Std_ReturnType Csm_SignatureGenerate(uint32 jobId, Crypto_OperationModeType mode,
                                           const uint8* dataPtr, uint32 dataLength,
                                           uint8* signaturePtr, uint32* signatureLengthPtr);
extern Std_ReturnType Csm_SignatureVerify(uint32 jobId, Crypto_OperationModeType mode,
                                         const uint8* dataPtr, uint32 dataLength,
                                         const uint8* signaturePtr, uint32 signatureLength,
                                         Crypto_VerifyResultType* verifyPtr);
extern Std_ReturnType Csm_KeyElementSet(uint32 keyId, uint32 keyElementId,
                                        const uint8* keyPtr, uint32 keyLength);
extern Std_ReturnType Csm_KeySetValid(uint32 keyId);
extern Std_ReturnType Csm_KeyExchangeInitiate(uint32 jobId, uint8 peerId,
                                              uint8* publicValuePtr, uint32* publicValueLengthPtr);
extern Std_ReturnType Csm_KeyExchangeRespond(uint32 jobId, uint8 peerId,
                                             const uint8* partnerPublicValuePtr,
                                             uint32 partnerPublicValueLength,
                                             uint8* ciphertextPtr, uint32* ciphertextLengthPtr);
extern Std_ReturnType Csm_KeyExchangeComplete(uint32 jobId, uint8 peerId,
                                              const uint8* ciphertextPtr, uint32 ciphertextLength);
extern Std_ReturnType Csm_KeyExchangeReset(uint8 peerId);
extern Std_ReturnType Csm_KeyExchangeGetSharedSecret(uint32 jobId, uint8 peerId,
                                                     uint8* sharedSecretPtr,
                                                     uint32* sharedSecretLengthPtr);
extern Std_ReturnType Csm_DeriveSessionKeys(uint8 peerId, const uint8* sharedSecretPtr,
                                            uint32 sharedSecretLength);
extern Std_ReturnType Csm_GetSessionKeys(uint8 peerId, Csm_SessionKeysType* sessionKeysPtr);
extern Std_ReturnType Csm_ClearSessionKeys(uint8 peerId);

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
    Csm_PendingOperationType CsmPendingOperation;
    const uint8* CsmPendingDataPtr;
    uint32 CsmPendingDataLength;
    const uint8* CsmPendingInputAuthenticatorPtr;
    uint32 CsmPendingInputAuthenticatorLength;
    uint8* CsmPendingOutputPtr;
    uint32* CsmPendingOutputLengthPtr;
    Crypto_VerifyResultType* CsmPendingVerifyResultPtr;
    boolean CsmPendingValid;
    Std_ReturnType CsmLastResult;
    boolean CsmLastResultValid;
    Csm_JobCallbackConfigType CsmCallbackConfig;
} Csm_JobContextType;

typedef struct
{
    boolean CsmMldsaPublicKeyValid;
    boolean CsmMldsaSecretKeyValid;
    uint8 CsmMldsaPublicKey[CSM_MLDSA_PUBLIC_KEY_BYTES];
    uint8 CsmMldsaSecretKey[CSM_MLDSA_SECRET_KEY_BYTES];
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
static PQC_SessionKeysType Csm_SessionKeys[CSM_MAX_PEERS];
static PQC_MLDSA_KeyPairType Csm_MLDSA_KeyPair;
static boolean Csm_PqcReady = FALSE;
static Csm_ConfigType Csm_RuntimeConfig =
{
    CSM_MLDSA_BOOTSTRAP_DEMO_FILE_AUTO,
    NULL
};

static const Csm_JobRoutingType Csm_JobRoutingTable[] =
{
    { (uint32)CSM_JOBID, CRYIF_PROVIDER_CLASSIC, CRYIF_PROVIDER_PQC, CRYIF_PROVIDER_PQC }
};

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

static CryIf_ProviderType Csm_SelectProvider(uint32 jobId, Csm_PendingOperationType operationType)
{
    uint8 idx;
    CryIf_ProviderType defaultProvider = CRYIF_PROVIDER_PQC;

    if (operationType == CSM_PENDING_MAC_GENERATE)
    {
        defaultProvider = CRYIF_PROVIDER_CLASSIC;
    }

    for (idx = 0U; idx < (uint8)(sizeof(Csm_JobRoutingTable) / sizeof(Csm_JobRoutingTable[0])); idx++)
    {
        if (Csm_JobRoutingTable[idx].CsmJobId != jobId)
        {
            continue;
        }

        if (operationType == CSM_PENDING_MAC_GENERATE)
        {
            return Csm_JobRoutingTable[idx].CsmMacProvider;
        }

        if ((operationType == CSM_PENDING_SIGNATURE_GENERATE) || (operationType == CSM_PENDING_SIGNATURE_VERIFY))
        {
            return Csm_JobRoutingTable[idx].CsmSignatureProvider;
        }

        return Csm_JobRoutingTable[idx].CsmKeyExchangeProvider;
    }

    return defaultProvider;
}

static void Csm_ClearPendingOperation(Csm_JobContextType* contextPtr)
{
    if (contextPtr == NULL)
    {
        return;
    }

    contextPtr->CsmPendingOperation = CSM_PENDING_NONE;
    contextPtr->CsmPendingDataPtr = NULL;
    contextPtr->CsmPendingDataLength = 0U;
    contextPtr->CsmPendingInputAuthenticatorPtr = NULL;
    contextPtr->CsmPendingInputAuthenticatorLength = 0U;
    contextPtr->CsmPendingOutputPtr = NULL;
    contextPtr->CsmPendingOutputLengthPtr = NULL;
    contextPtr->CsmPendingVerifyResultPtr = NULL;
    contextPtr->CsmPendingValid = FALSE;
}

static Std_ReturnType Csm_QueuePendingOperation(
    Csm_JobContextType* contextPtr,
    Csm_PendingOperationType operationType,
    const uint8* dataPtr,
    uint32 dataLength,
    const uint8* inputAuthenticatorPtr,
    uint32 inputAuthenticatorLength,
    uint8* outputPtr,
    uint32* outputLengthPtr,
    Crypto_VerifyResultType* verifyResultPtr)
{
    Std_ReturnType queueResult;

    if (contextPtr == NULL)
    {
        return E_NOT_OK;
    }

    if (contextPtr->CsmPendingValid == TRUE)
    {
        return E_BUSY;
    }

    queueResult = Csm_EnqueueJob(contextPtr->CsmJobId);
    if (queueResult != E_OK)
    {
        return CRYPTO_E_QUEUE_FULL;
    }

    contextPtr->CsmPendingOperation = operationType;
    contextPtr->CsmPendingDataPtr = dataPtr;
    contextPtr->CsmPendingDataLength = dataLength;
    contextPtr->CsmPendingInputAuthenticatorPtr = inputAuthenticatorPtr;
    contextPtr->CsmPendingInputAuthenticatorLength = inputAuthenticatorLength;
    contextPtr->CsmPendingOutputPtr = outputPtr;
    contextPtr->CsmPendingOutputLengthPtr = outputLengthPtr;
    contextPtr->CsmPendingVerifyResultPtr = verifyResultPtr;
    contextPtr->CsmPendingValid = TRUE;
    contextPtr->CsmJobState = CSM_JOBSTATE_QUEUED;

    return E_BUSY;
}

static Std_ReturnType Csm_ExecutePendingOperation(Csm_JobContextType* contextPtr)
{
    CryIf_ProviderType provider;

    if ((contextPtr == NULL) || (contextPtr->CsmPendingValid == FALSE))
    {
        return E_NOT_OK;
    }

    provider = Csm_SelectProvider(contextPtr->CsmJobId, contextPtr->CsmPendingOperation);

    if (contextPtr->CsmPendingOperation == CSM_PENDING_MAC_GENERATE)
    {
        return CryIf_MacGenerate(
            provider,
            contextPtr->CsmPendingDataPtr,
            contextPtr->CsmPendingDataLength,
            contextPtr->CsmPendingOutputPtr,
            contextPtr->CsmPendingOutputLengthPtr);
    }

    if (contextPtr->CsmPendingOperation == CSM_PENDING_SIGNATURE_GENERATE)
    {
        return CryIf_SignatureGenerate(
            provider,
            contextPtr->CsmPendingDataPtr,
            contextPtr->CsmPendingDataLength,
            Csm_KeyStore.CsmMldsaSecretKey,
            contextPtr->CsmPendingOutputPtr,
            contextPtr->CsmPendingOutputLengthPtr);
    }

    if (contextPtr->CsmPendingOperation == CSM_PENDING_SIGNATURE_VERIFY)
    {
        if (contextPtr->CsmPendingVerifyResultPtr == NULL)
        {
            return E_NOT_OK;
        }

        if (CryIf_SignatureVerify(
                provider,
                contextPtr->CsmPendingDataPtr,
                contextPtr->CsmPendingDataLength,
                contextPtr->CsmPendingInputAuthenticatorPtr,
                contextPtr->CsmPendingInputAuthenticatorLength,
                Csm_KeyStore.CsmMldsaPublicKey) == E_OK)
        {
            *contextPtr->CsmPendingVerifyResultPtr = CRYPTO_E_VER_OK;
            return E_OK;
        }

        *contextPtr->CsmPendingVerifyResultPtr = CRYPTO_E_VER_NOT_OK;
        return E_NOT_OK;
    }

    return E_NOT_OK;
}

static Std_ReturnType Csm_PQC_EnsureReady(void)
{
    Std_ReturnType result;

    if (Csm_PqcReady == TRUE)
    {
        return E_OK;
    }

    result = CryIf_Init();
    if (result != E_OK)
    {
        (void)printf("ERROR: CSM PQC init failed\n");
        return E_NOT_OK;
    }

    if (Csm_RuntimeConfig.CsmMldsaBootstrapMode == CSM_MLDSA_BOOTSTRAP_DEMO_FILE_AUTO)
    {
        result = CryIf_MldsaLoadKeys(&Csm_MLDSA_KeyPair, CSM_MLDSA_KEY_PREFIX);
        if (result != E_OK)
        {
            result = CryIf_MldsaGenerateKeyPair(&Csm_MLDSA_KeyPair);
            if (result != E_OK)
            {
                (void)printf("ERROR: CSM ML-DSA key generation failed\n");
                return E_NOT_OK;
            }

            (void)CryIf_MldsaSaveKeys(&Csm_MLDSA_KeyPair, CSM_MLDSA_KEY_PREFIX);
        }
    }
    else if (Csm_RuntimeConfig.CsmMldsaBootstrapMode == CSM_MLDSA_BOOTSTRAP_FILE_STRICT)
    {
        result = CryIf_MldsaLoadKeys(&Csm_MLDSA_KeyPair, CSM_MLDSA_KEY_PREFIX);
        if (result != E_OK)
        {
            (void)printf("ERROR: CSM strict key bootstrap failed (missing key files)\n");
            return E_NOT_OK;
        }
    }
    else
    {
        if (Csm_RuntimeConfig.CsmLoadProvisionedMldsaKeysFct == NULL)
        {
            (void)printf("ERROR: CSM provisioned/HSM bootstrap requires key loading callback\n");
            return E_NOT_OK;
        }

        result = Csm_RuntimeConfig.CsmLoadProvisionedMldsaKeysFct(
            Csm_MLDSA_KeyPair.PublicKey,
            CSM_MLDSA_PUBLIC_KEY_BYTES,
            Csm_MLDSA_KeyPair.SecretKey,
            CSM_MLDSA_SECRET_KEY_BYTES);
        if (result != E_OK)
        {
            (void)printf("ERROR: CSM provisioned/HSM key bootstrap failed\n");
            return E_NOT_OK;
        }
    }

    (void)memcpy(Csm_KeyStore.CsmMldsaPublicKey, Csm_MLDSA_KeyPair.PublicKey, CSM_MLDSA_PUBLIC_KEY_BYTES);
    (void)memcpy(Csm_KeyStore.CsmMldsaSecretKey, Csm_MLDSA_KeyPair.SecretKey, CSM_MLDSA_SECRET_KEY_BYTES);
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
    if (configPtr != NULL)
    {
        Csm_RuntimeConfig = *configPtr;
    }
    else
    {
        Csm_RuntimeConfig.CsmMldsaBootstrapMode = CSM_MLDSA_BOOTSTRAP_FILE_STRICT;
        Csm_RuntimeConfig.CsmLoadProvisionedMldsaKeysFct = NULL;
    }

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

    for (peerId = 0U; peerId < CSM_MAX_PEERS; peerId++)
    {
        (void)CryIf_KeyExchangeReset(peerId);
        (void)CryIf_ClearSessionKeys(peerId);
    }

    CryIf_DeInit();
    Csm_ResetState();
    Csm_IsInitialized = FALSE;
}

void Csm_MainFunction(void)
{
    uint32 queuedJobId = 0U;
    Csm_JobContextType* contextPtr;
    Std_ReturnType operationResult;

    if (Csm_IsInitialized == FALSE)
    {
        return;
    }

    if (Csm_DequeueJob(&queuedJobId) == E_OK)
    {
        contextPtr = Csm_GetJobContext(queuedJobId);
        if ((contextPtr != NULL) && (contextPtr->CsmPendingValid == TRUE))
        {
            operationResult = Csm_ExecutePendingOperation(contextPtr);
            contextPtr->CsmLastResult = operationResult;
            contextPtr->CsmLastResultValid = TRUE;
            contextPtr->CsmJobState = CSM_JOBSTATE_IDLE;
            Csm_ClearPendingOperation(contextPtr);

            if (contextPtr->CsmCallbackConfig.CsmJobCallbackFct != NULL)
            {
                contextPtr->CsmCallbackConfig.CsmJobCallbackFct(
                    contextPtr->CsmJobId,
                    operationResult,
                    contextPtr->CsmCallbackConfig.CsmJobCallbackContextPtr);
            }
        }
    }
}

Std_ReturnType Csm_RegisterJobCallback(uint32 jobId, Csm_JobCallbackType callbackFct, void* callbackContextPtr)
{
    Csm_JobContextType* contextPtr;

    if (Csm_IsInitialized == FALSE)
    {
        return E_NOT_OK;
    }

    contextPtr = Csm_GetJobContext(jobId);
    if (contextPtr == NULL)
    {
        return CRYPTO_E_QUEUE_FULL;
    }

    contextPtr->CsmCallbackConfig.CsmJobCallbackFct = callbackFct;
    contextPtr->CsmCallbackConfig.CsmJobCallbackContextPtr = callbackContextPtr;
    return E_OK;
}

Std_ReturnType Csm_GetJobResult(uint32 jobId, Std_ReturnType* resultPtr, boolean* completedPtr)
{
    Csm_JobContextType* contextPtr;

    if ((Csm_IsInitialized == FALSE) || (resultPtr == NULL) || (completedPtr == NULL))
    {
        return E_NOT_OK;
    }

    contextPtr = Csm_GetJobContext(jobId);
    if (contextPtr == NULL)
    {
        return E_NOT_OK;
    }

    if ((contextPtr->CsmPendingValid == TRUE) || (contextPtr->CsmLastResultValid == FALSE))
    {
        *completedPtr = FALSE;
        *resultPtr = E_BUSY;
        return E_OK;
    }

    *completedPtr = TRUE;
    *resultPtr = contextPtr->CsmLastResult;
    return E_OK;
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
    CryIf_ProviderType provider;

    if ((Csm_IsInitialized == FALSE) || (macPtr == NULL) || (macLengthPtr == NULL))
    {
        return E_NOT_OK;
    }

    contextPtr = Csm_GetJobContext(jobId);
    if (contextPtr == NULL)
    {
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
        return E_NOT_OK;
    }

    if ((mode == CRYPTO_OPERATIONMODE_SINGLECALL) && (contextPtr->CsmJobState == CSM_JOBSTATE_ACTIVE))
    {
        return Csm_QueuePendingOperation(
            contextPtr,
            CSM_PENDING_MAC_GENERATE,
            preparedDataPtr,
            preparedLength,
            NULL,
            0U,
            macPtr,
            macLengthPtr,
            NULL);
    }

    provider = Csm_SelectProvider(jobId, CSM_PENDING_MAC_GENERATE);
    prepareResult = CryIf_MacGenerate(provider, preparedDataPtr, preparedLength, macPtr, macLengthPtr);
    contextPtr->CsmLastResult = prepareResult;
    contextPtr->CsmLastResultValid = TRUE;
    return prepareResult;
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
    CryIf_ProviderType provider;

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

    if ((mode == CRYPTO_OPERATIONMODE_SINGLECALL) && (contextPtr->CsmJobState == CSM_JOBSTATE_ACTIVE))
    {
        return Csm_QueuePendingOperation(
            contextPtr,
            CSM_PENDING_SIGNATURE_GENERATE,
            preparedDataPtr,
            preparedLength,
            NULL,
            0U,
            signaturePtr,
            signatureLengthPtr,
            NULL);
    }

    provider = Csm_SelectProvider(jobId, CSM_PENDING_SIGNATURE_GENERATE);
    prepareResult = CryIf_SignatureGenerate(
        provider,
        preparedDataPtr,
        preparedLength,
        Csm_KeyStore.CsmMldsaSecretKey,
        signaturePtr,
        signatureLengthPtr);
    contextPtr->CsmLastResult = prepareResult;
    contextPtr->CsmLastResultValid = TRUE;
    return prepareResult;
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
    CryIf_ProviderType provider;

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

    if ((mode == CRYPTO_OPERATIONMODE_SINGLECALL) && (contextPtr->CsmJobState == CSM_JOBSTATE_ACTIVE))
    {
        return Csm_QueuePendingOperation(
            contextPtr,
            CSM_PENDING_SIGNATURE_VERIFY,
            preparedDataPtr,
            preparedLength,
            signaturePtr,
            signatureLength,
            NULL,
            NULL,
            verifyPtr);
    }

    provider = Csm_SelectProvider(jobId, CSM_PENDING_SIGNATURE_VERIFY);
    if (CryIf_SignatureVerify(
            provider,
            preparedDataPtr,
            preparedLength,
            signaturePtr,
            signatureLength,
            Csm_KeyStore.CsmMldsaPublicKey) == E_OK)
    {
        *verifyPtr = CRYPTO_E_VER_OK;
        contextPtr->CsmLastResult = E_OK;
        contextPtr->CsmLastResultValid = TRUE;
        return E_OK;
    }

    *verifyPtr = CRYPTO_E_VER_NOT_OK;
    contextPtr->CsmLastResult = E_NOT_OK;
    contextPtr->CsmLastResultValid = TRUE;
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
        if ((keyElementId == CSM_KEYELEMENT_MLDSA_PUBLIC) && (keyLength == CSM_MLDSA_PUBLIC_KEY_BYTES))
        {
            (void)memcpy(Csm_KeyStore.CsmMldsaPublicKey, keyPtr, CSM_MLDSA_PUBLIC_KEY_BYTES);
            Csm_KeyStore.CsmMldsaPublicKeyValid = FALSE;
            return E_OK;
        }

        if ((keyElementId == CSM_KEYELEMENT_MLDSA_SECRET) && (keyLength == CSM_MLDSA_SECRET_KEY_BYTES))
        {
            (void)memcpy(Csm_KeyStore.CsmMldsaSecretKey, keyPtr, CSM_MLDSA_SECRET_KEY_BYTES);
            Csm_KeyStore.CsmMldsaSecretKeyValid = FALSE;
            return E_OK;
        }
        return CRYPTO_E_KEY_SIZE_MISMATCH;
    }

    /* cppcheck-suppress misra-c2012-10.4 */
    if (CSM_KEYID_IS_SESSION(keyId) != 0)
    {
        peerId = CSM_SESSION_PEER_FROM_KEYID(keyId);
        if (peerId >= CSM_MAX_PEERS)
        {
            return E_NOT_OK;
        }

        if (CryIf_GetSessionKeys(peerId, &sessionKeys) != E_OK)
        {
            (void)memset(&sessionKeys, 0, sizeof(sessionKeys));
        }

        if ((keyElementId == CSM_KEYELEMENT_SESSION_ENC) && (keyLength == CSM_DERIVED_KEY_LENGTH))
        {
            (void)memcpy(sessionKeys.EncryptionKey, keyPtr, CSM_DERIVED_KEY_LENGTH);
        }
        else if ((keyElementId == CSM_KEYELEMENT_SESSION_AUTH) && (keyLength == CSM_DERIVED_KEY_LENGTH))
        {
            (void)memcpy(sessionKeys.AuthenticationKey, keyPtr, CSM_DERIVED_KEY_LENGTH);
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

    /* cppcheck-suppress misra-c2012-10.4 */
    if (CSM_KEYID_IS_SESSION(keyId) != 0)
    {
        peerId = CSM_SESSION_PEER_FROM_KEYID(keyId);
        if (peerId >= CSM_MAX_PEERS)
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

    if (*publicValueLengthPtr < CSM_MLKEM_PUBLIC_KEY_BYTES)
    {
        return CRYPTO_E_KEY_SIZE_MISMATCH;
    }

    if (CryIf_KeyExchangeInitiate(peerId, publicValuePtr) != E_OK)
    {
        return E_NOT_OK;
    }

    *publicValueLengthPtr = CSM_MLKEM_PUBLIC_KEY_BYTES;
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

    if ((partnerPublicValueLength != CSM_MLKEM_PUBLIC_KEY_BYTES) || (*ciphertextLengthPtr < CSM_MLKEM_CIPHERTEXT_BYTES))
    {
        return CRYPTO_E_KEY_SIZE_MISMATCH;
    }

    if (CryIf_KeyExchangeRespond(peerId, partnerPublicValuePtr, ciphertextPtr) != E_OK)
    {
        return E_NOT_OK;
    }

    *ciphertextLengthPtr = CSM_MLKEM_CIPHERTEXT_BYTES;
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

    if (ciphertextLength != CSM_MLKEM_CIPHERTEXT_BYTES)
    {
        return CRYPTO_E_KEY_SIZE_MISMATCH;
    }

    return CryIf_KeyExchangeComplete(peerId, ciphertextPtr);
}

Std_ReturnType Csm_KeyExchangeReset(uint8 peerId)
{
    if ((Csm_IsInitialized == FALSE) || (peerId >= CSM_MAX_PEERS))
    {
        return E_NOT_OK;
    }

    return CryIf_KeyExchangeReset(peerId);
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

    if (*sharedSecretLengthPtr < CSM_MLKEM_SHARED_SECRET_BYTES)
    {
        return CRYPTO_E_KEY_SIZE_MISMATCH;
    }

    if (CryIf_KeyExchangeGetSharedSecret(peerId, sharedSecretPtr) != E_OK)
    {
        return E_NOT_OK;
    }

    *sharedSecretLengthPtr = CSM_MLKEM_SHARED_SECRET_BYTES;
    return E_OK;
}

Std_ReturnType Csm_DeriveSessionKeys(
    uint8 peerId,
    const uint8* sharedSecretPtr,
    uint32 sharedSecretLength)
{
    PQC_SessionKeysType sessionKeys;

    if ((Csm_IsInitialized == FALSE) || (sharedSecretPtr == NULL) || (sharedSecretLength != CSM_MLKEM_SHARED_SECRET_BYTES))
    {
        return E_NOT_OK;
    }

    if (CryIf_DeriveSessionKeys(sharedSecretPtr, peerId, &sessionKeys) != E_OK)
    {
        return E_NOT_OK;
    }

    Csm_SessionKeys[peerId] = sessionKeys;
    return E_OK;
}

Std_ReturnType Csm_GetSessionKeys(
    uint8 peerId,
    Csm_SessionKeysType* sessionKeysPtr)
{
    if ((Csm_IsInitialized == FALSE) || (sessionKeysPtr == NULL) || (peerId >= CSM_MAX_PEERS))
    {
        return E_NOT_OK;
    }

    if (CryIf_GetSessionKeys(peerId, sessionKeysPtr) == E_OK)
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
    if ((Csm_IsInitialized == FALSE) || (peerId >= CSM_MAX_PEERS))
    {
        return E_NOT_OK;
    }

    (void)memset(&Csm_SessionKeys[peerId], 0, sizeof(PQC_SessionKeysType));
    return CryIf_ClearSessionKeys(peerId);
}
