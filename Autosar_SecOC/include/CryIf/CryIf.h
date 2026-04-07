#ifndef INCLUDE_CRYIF_H_
#define INCLUDE_CRYIF_H_

/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "Std_Types.h"
#include "PQC/PQC.h"
#include "PQC/PQC_KeyExchange.h"
#include "PQC/PQC_KeyDerivation.h"

/********************************************************************************************************/
/*********************************************DefinesAndEnums********************************************/
/********************************************************************************************************/

#define CRYIF_MAX_PEERS                 PQC_MAX_PEERS
#define CRYIF_MLDSA_PUBLIC_KEY_BYTES    PQC_MLDSA_PUBLIC_KEY_BYTES
#define CRYIF_MLDSA_SECRET_KEY_BYTES    PQC_MLDSA_SECRET_KEY_BYTES
#define CRYIF_MLDSA_SIGNATURE_BYTES     PQC_MLDSA_SIGNATURE_BYTES
#define CRYIF_MLKEM_PUBLIC_KEY_BYTES    PQC_MLKEM_PUBLIC_KEY_BYTES
#define CRYIF_MLKEM_CIPHERTEXT_BYTES    PQC_MLKEM_CIPHERTEXT_BYTES
#define CRYIF_MLKEM_SHARED_SECRET_BYTES PQC_MLKEM_SHARED_SECRET_BYTES
#define CRYIF_DERIVED_KEY_LENGTH        PQC_DERIVED_KEY_LENGTH

typedef enum
{
    CRYIF_PROVIDER_CLASSIC = 0U,
    CRYIF_PROVIDER_PQC = 1U
} CryIf_ProviderType;

/********************************************************************************************************/
/*****************************************FunctionPrototype**********************************************/
/********************************************************************************************************/

Std_ReturnType CryIf_Init(void);
void CryIf_DeInit(void);

Std_ReturnType CryIf_MacGenerate(
    CryIf_ProviderType provider,
    const uint8* dataPtr,
    uint32 dataLength,
    uint8* macPtr,
    uint32* macLengthPtr);

Std_ReturnType CryIf_SignatureGenerate(
    CryIf_ProviderType provider,
    const uint8* dataPtr,
    uint32 dataLength,
    const uint8* secretKeyPtr,
    uint8* signaturePtr,
    uint32* signatureLengthPtr);

Std_ReturnType CryIf_SignatureVerify(
    CryIf_ProviderType provider,
    const uint8* dataPtr,
    uint32 dataLength,
    const uint8* signaturePtr,
    uint32 signatureLength,
    const uint8* publicKeyPtr);

Std_ReturnType CryIf_KeyExchangeInitiate(uint8 peerId, uint8* publicValuePtr);
Std_ReturnType CryIf_KeyExchangeRespond(uint8 peerId, const uint8* partnerPublicValuePtr, uint8* ciphertextPtr);
Std_ReturnType CryIf_KeyExchangeComplete(uint8 peerId, const uint8* ciphertextPtr);
Std_ReturnType CryIf_KeyExchangeReset(uint8 peerId);
Std_ReturnType CryIf_KeyExchangeGetSharedSecret(uint8 peerId, uint8* sharedSecretPtr);

Std_ReturnType CryIf_DeriveSessionKeys(const uint8* sharedSecretPtr, uint8 peerId, PQC_SessionKeysType* sessionKeysPtr);
Std_ReturnType CryIf_GetSessionKeys(uint8 peerId, PQC_SessionKeysType* sessionKeysPtr);
Std_ReturnType CryIf_ClearSessionKeys(uint8 peerId);

Std_ReturnType CryIf_MldsaLoadKeys(PQC_MLDSA_KeyPairType* keyPairPtr, const char* filePrefixPtr);
Std_ReturnType CryIf_MldsaGenerateKeyPair(PQC_MLDSA_KeyPairType* keyPairPtr);
Std_ReturnType CryIf_MldsaSaveKeys(const PQC_MLDSA_KeyPairType* keyPairPtr, const char* filePrefixPtr);

#endif
