#ifndef INCLUDE_CSM_H_
#define INCLUDE_CSM_H_

/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "Std_Types.h"
#include "SecOC/SecOC_Debug.h"
#include "PQC_KeyExchange.h"
#include "PQC_KeyDerivation.h"


/********************************************************************************************************/
/*********************************************DefinesAndEnums********************************************/
/********************************************************************************************************/

#define CRYPTO_E_KEY_NOT_VALID      ((Std_ReturnType)0x09)
#define CRYPTO_E_KEY_SIZE_MISMATCH  ((Std_ReturnType)0x0A)
#define CRYPTO_E_KEY_EMPTY          ((Std_ReturnType)0x0D)
#define CRYPTO_E_QUEUE_FULL         ((Std_ReturnType)0x05)

#define CSM_MAX_JOB_CONTEXTS        ((uint8)16U)
#define CSM_MAX_JOB_QUEUE           ((uint8)16U)
#define CSM_MAX_STREAM_BUFFER       ((uint32)4096U)
#define CSM_MAX_KEY_ELEMENTS        ((uint8)16U)

#define CSM_KEYID_MLDSA_LOCAL       ((uint32)1U)
#define CSM_KEYID_SESSION_BASE      ((uint32)100U)

#define CSM_KEYELEMENT_MLDSA_PUBLIC ((uint32)1U)
#define CSM_KEYELEMENT_MLDSA_SECRET ((uint32)2U)
#define CSM_KEYELEMENT_SESSION_ENC  ((uint32)3U)
#define CSM_KEYELEMENT_SESSION_AUTH ((uint32)4U)

/* Crypto operation modes (AUTOSAR SWS_Csm) */
typedef enum
{
    CRYPTO_OPERATIONMODE_START = 0x01,
    CRYPTO_OPERATIONMODE_UPDATE = 0x02,
    CRYPTO_OPERATIONMODE_FINISH = 0x04
    ,
    CRYPTO_OPERATIONMODE_SINGLECALL = 0x07
}Crypto_OperationModeType;

typedef enum
{
    CRYPTO_E_VER_OK = 0x00,
    CRYPTO_E_VER_NOT_OK = 0x01
}Crypto_VerifyResultType;

typedef struct
{
    uint8 CsmReserved;
} Csm_ConfigType;

typedef enum
{
    CSM_JOBSTATE_IDLE = 0,
    CSM_JOBSTATE_ACTIVE,
    CSM_JOBSTATE_QUEUED
} Csm_JobStateType;


/********************************************************************************************************/
/*****************************************FunctionPrototype**********************************************/
/********************************************************************************************************/

void Csm_Init(const Csm_ConfigType* configPtr);
void Csm_DeInit(void);
void Csm_MainFunction(void);
void Csm_GetVersionInfo(Std_VersionInfoType* versioninfo);

Std_ReturnType Csm_MacGenerate ( 
    uint32 jobId, 
    Crypto_OperationModeType mode,
    const uint8* dataPtr,
    uint32 dataLength,
    uint8* macPtr,
    uint32* macLengthPtr );

/*******************************************************
 *          * Function Info *                           *
 *                                                      *
 * Function_Name        : Csm_MacVerify                 *
 * Function_Index       : 8.3.3.2 [SWS_Csm_01050]       *
 * Function_File        : SWS of CSM                    *
 * Function_Descripton  : Verifies the given MAC by     *
 * SecOC to obtain the current freshness value          *
 * comparing if the MAC is generated with the given data*
 *******************************************************/

Std_ReturnType Csm_MacVerify(
uint32 jobId,
Crypto_OperationModeType mode,
const uint8* dataPtr,
uint32 dataLength,
const uint8* macPtr,
const uint32 macLength,
Crypto_VerifyResultType* verifyPtr
);

/*******************************************************
 *          * Function Info *                           *
 *                                                      *
 * Function_Name        : Csm_SignatureGenerate         *
 * Function_Descripton  : Generates a digital signature *
 * using ML-DSA-65 post-quantum algorithm               *
 *******************************************************/
Std_ReturnType Csm_SignatureGenerate(
    uint32 jobId,
    Crypto_OperationModeType mode,
    const uint8* dataPtr,
    uint32 dataLength,
    uint8* signaturePtr,
    uint32* signatureLengthPtr
);

/*******************************************************
 *          * Function Info *                           *
 *                                                      *
 * Function_Name        : Csm_SignatureVerify           *
 * Function_Descripton  : Verifies a digital signature  *
 * using ML-DSA-65 post-quantum algorithm               *
 *******************************************************/
Std_ReturnType Csm_SignatureVerify(
    uint32 jobId,
    Crypto_OperationModeType mode,
    const uint8* dataPtr,
    uint32 dataLength,
    const uint8* signaturePtr,
    uint32 signatureLength,
    Crypto_VerifyResultType* verifyPtr
);

/*******************************************************
 *          * Function Info *                           *
 *                                                      *
 * Function_Name        : Csm_KeyElementSet             *
 * Function_Index       : [SWS_Csm_01018]               *
 * Function_Descripton  : Sets a key element to the     *
 * key identified by keyId.                             *
 *******************************************************/
Std_ReturnType Csm_KeyElementSet(
    uint32 keyId,
    uint32 keyElementId,
    const uint8* keyPtr,
    uint32 keyLength
);

/*******************************************************
 *          * Function Info *                           *
 *                                                      *
 * Function_Name        : Csm_KeySetValid               *
 * Function_Index       : [SWS_Csm_01019]               *
 * Function_Descripton  : Sets the key state of the     *
 * key identified by keyId to valid.                    *
 *******************************************************/
Std_ReturnType Csm_KeySetValid(
    uint32 keyId
);

Std_ReturnType Csm_KeyExchangeInitiate(
    uint32 jobId,
    uint8 peerId,
    uint8* publicValuePtr,
    uint32* publicValueLengthPtr
);

Std_ReturnType Csm_KeyExchangeRespond(
    uint32 jobId,
    uint8 peerId,
    const uint8* partnerPublicValuePtr,
    uint32 partnerPublicValueLength,
    uint8* ciphertextPtr,
    uint32* ciphertextLengthPtr
);

Std_ReturnType Csm_KeyExchangeComplete(
    uint32 jobId,
    uint8 peerId,
    const uint8* ciphertextPtr,
    uint32 ciphertextLength
);

Std_ReturnType Csm_KeyExchangeReset(uint8 peerId);

Std_ReturnType Csm_KeyExchangeGetSharedSecret(
    uint32 jobId,
    uint8 peerId,
    uint8* sharedSecretPtr,
    uint32* sharedSecretLengthPtr
);

Std_ReturnType Csm_DeriveSessionKeys(
    uint8 peerId,
    const uint8* sharedSecretPtr,
    uint32 sharedSecretLength
);

Std_ReturnType Csm_GetSessionKeys(
    uint8 peerId,
    PQC_SessionKeysType* sessionKeysPtr
);

Std_ReturnType Csm_ClearSessionKeys(
    uint8 peerId
);

#endif