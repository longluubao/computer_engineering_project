/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/

#include "CryIf/CryIf.h"
#include "Encrypt/encrypt.h"
#include "PQC/PQC.h"
#include "PQC/PQC_KeyDerivation.h"
#include "PQC/PQC_KeyExchange.h"

/********************************************************************************************************/
/**************************************ForwardDeclarations***********************************************/
/********************************************************************************************************/

extern Std_ReturnType CryIf_Init(void);
extern void CryIf_DeInit(void);
extern Std_ReturnType CryIf_MacGenerate(CryIf_ProviderType provider,
                                        const uint8* dataPtr, uint32 dataLength,
                                        uint8* macPtr, uint32* macLengthPtr);
extern Std_ReturnType CryIf_SignatureGenerate(CryIf_ProviderType provider,
                                             const uint8* dataPtr, uint32 dataLength,
                                             const uint8* secretKeyPtr,
                                             uint8* signaturePtr, uint32* signatureLengthPtr);
extern Std_ReturnType CryIf_SignatureVerify(CryIf_ProviderType provider,
                                           const uint8* dataPtr, uint32 dataLength,
                                           const uint8* signaturePtr, uint32 signatureLength,
                                           const uint8* publicKeyPtr);
extern Std_ReturnType CryIf_KeyExchangeInitiate(uint8 peerId, uint8* publicValuePtr);
extern Std_ReturnType CryIf_KeyExchangeRespond(uint8 peerId, const uint8* partnerPublicValuePtr,
                                               uint8* ciphertextPtr);
extern Std_ReturnType CryIf_KeyExchangeComplete(uint8 peerId, const uint8* ciphertextPtr);
extern Std_ReturnType CryIf_KeyExchangeReset(uint8 peerId);
extern Std_ReturnType CryIf_KeyExchangeGetSharedSecret(uint8 peerId, uint8* sharedSecretPtr);
extern Std_ReturnType CryIf_DeriveSessionKeys(const uint8* sharedSecretPtr, uint8 peerId,
                                              PQC_SessionKeysType* sessionKeysPtr);
extern Std_ReturnType CryIf_GetSessionKeys(uint8 peerId, PQC_SessionKeysType* sessionKeysPtr);
extern Std_ReturnType CryIf_ClearSessionKeys(uint8 peerId);
extern Std_ReturnType CryIf_MldsaLoadKeys(PQC_MLDSA_KeyPairType* keyPairPtr,
                                          const char* filePrefixPtr);
extern Std_ReturnType CryIf_MldsaGenerateKeyPair(PQC_MLDSA_KeyPairType* keyPairPtr);
extern Std_ReturnType CryIf_MldsaSaveKeys(const PQC_MLDSA_KeyPairType* keyPairPtr,
                                          const char* filePrefixPtr);

/* MISRA C:2012 Rule 17.3 - Cross-module forward declarations */
extern Std_ReturnType PQC_KeyExchange_Init(void);
extern Std_ReturnType PQC_KeyDerivation_Init(void);
extern Std_ReturnType PQC_MLDSA_Sign(const uint8* Message, uint32 MessageLength,
                                     const uint8* SecretKey, uint8* Signature, uint32* SignatureLength);
extern Std_ReturnType PQC_MLDSA_Verify(const uint8* Message, uint32 MessageLength,
                                       const uint8* Signature, uint32 SignatureLength, const uint8* PublicKey);
extern void startEncryption(const uint8* message, uint32 messageLen, uint8* macPtr, uint32* macLengthPtr);

/********************************************************************************************************/
/********************************************PublicFunctions*********************************************/
/********************************************************************************************************/

Std_ReturnType CryIf_Init(void)
{
    Std_ReturnType result;

    result = PQC_Init();
    if (result != PQC_E_OK)
    {
        return E_NOT_OK;
    }

    if (PQC_KeyExchange_Init() != E_OK)
    {
        return E_NOT_OK;
    }

    if (PQC_KeyDerivation_Init() != E_OK)
    {
        return E_NOT_OK;
    }

    return E_OK;
}

void CryIf_DeInit(void)
{
    uint8 peerId;

    for (peerId = 0U; peerId < CRYIF_MAX_PEERS; peerId++)
    {
        (void)PQC_KeyExchange_Reset(peerId);
        (void)PQC_ClearSessionKeys(peerId);
    }
}

Std_ReturnType CryIf_MacGenerate(
    CryIf_ProviderType provider,
    const uint8* dataPtr,
    uint32 dataLength,
    uint8* macPtr,
    uint32* macLengthPtr)
{
    if ((provider != CRYIF_PROVIDER_CLASSIC) || (dataPtr == NULL) || (macPtr == NULL) || (macLengthPtr == NULL))
    {
        return E_NOT_OK;
    }

    startEncryption(dataPtr, dataLength, macPtr, macLengthPtr);
    return E_OK;
}

Std_ReturnType CryIf_SignatureGenerate(
    CryIf_ProviderType provider,
    const uint8* dataPtr,
    uint32 dataLength,
    const uint8* secretKeyPtr,
    uint8* signaturePtr,
    uint32* signatureLengthPtr)
{
    if ((provider != CRYIF_PROVIDER_PQC) ||
        (dataPtr == NULL) ||
        (secretKeyPtr == NULL) ||
        (signaturePtr == NULL) ||
        (signatureLengthPtr == NULL))
    {
        return E_NOT_OK;
    }

    if (PQC_MLDSA_Sign(dataPtr, dataLength, secretKeyPtr, signaturePtr, signatureLengthPtr) != PQC_E_OK)
    {
        return E_NOT_OK;
    }

    return E_OK;
}

Std_ReturnType CryIf_SignatureVerify(
    CryIf_ProviderType provider,
    const uint8* dataPtr,
    uint32 dataLength,
    const uint8* signaturePtr,
    uint32 signatureLength,
    const uint8* publicKeyPtr)
{
    if ((provider != CRYIF_PROVIDER_PQC) ||
        (dataPtr == NULL) ||
        (signaturePtr == NULL) ||
        (publicKeyPtr == NULL))
    {
        return E_NOT_OK;
    }

    if (PQC_MLDSA_Verify(dataPtr, dataLength, signaturePtr, signatureLength, publicKeyPtr) != PQC_E_OK)
    {
        return E_NOT_OK;
    }

    return E_OK;
}

Std_ReturnType CryIf_KeyExchangeInitiate(uint8 peerId, uint8* publicValuePtr)
{
    return PQC_KeyExchange_Initiate(peerId, publicValuePtr);
}

Std_ReturnType CryIf_KeyExchangeRespond(uint8 peerId, const uint8* partnerPublicValuePtr, uint8* ciphertextPtr)
{
    return PQC_KeyExchange_Respond(peerId, partnerPublicValuePtr, ciphertextPtr);
}

Std_ReturnType CryIf_KeyExchangeComplete(uint8 peerId, const uint8* ciphertextPtr)
{
    return PQC_KeyExchange_Complete(peerId, ciphertextPtr);
}

Std_ReturnType CryIf_KeyExchangeReset(uint8 peerId)
{
    return PQC_KeyExchange_Reset(peerId);
}

Std_ReturnType CryIf_KeyExchangeGetSharedSecret(uint8 peerId, uint8* sharedSecretPtr)
{
    return PQC_KeyExchange_GetSharedSecret(peerId, sharedSecretPtr);
}

Std_ReturnType CryIf_DeriveSessionKeys(const uint8* sharedSecretPtr, uint8 peerId, PQC_SessionKeysType* sessionKeysPtr)
{
    return PQC_DeriveSessionKeys(sharedSecretPtr, peerId, sessionKeysPtr);
}

Std_ReturnType CryIf_GetSessionKeys(uint8 peerId, PQC_SessionKeysType* sessionKeysPtr)
{
    return PQC_GetSessionKeys(peerId, sessionKeysPtr);
}

Std_ReturnType CryIf_ClearSessionKeys(uint8 peerId)
{
    return PQC_ClearSessionKeys(peerId);
}

Std_ReturnType CryIf_MldsaLoadKeys(PQC_MLDSA_KeyPairType* keyPairPtr, const char* filePrefixPtr)
{
    return PQC_MLDSA_LoadKeys(keyPairPtr, filePrefixPtr);
}

Std_ReturnType CryIf_MldsaGenerateKeyPair(PQC_MLDSA_KeyPairType* keyPairPtr)
{
    return PQC_MLDSA_KeyGen(keyPairPtr);
}

Std_ReturnType CryIf_MldsaSaveKeys(const PQC_MLDSA_KeyPairType* keyPairPtr, const char* filePrefixPtr)
{
    return PQC_MLDSA_SaveKeys(keyPairPtr, filePrefixPtr);
}
