#ifndef INCLUDE_CSM_H_
#define INCLUDE_CSM_H_

/********************************************************************************************************/
/************************************************INCULDES************************************************/
/********************************************************************************************************/

#include "Std_Types.h"
#include "SecOC/SecOC_Debug.h"


/********************************************************************************************************/
/*********************************************DefinesAndEnums********************************************/
/********************************************************************************************************/

#define CRYPTO_E_KEY_NOT_VALID      ((Std_ReturnType)0x09)
#define CRYPTO_E_KEY_SIZE_MISMATCH  ((Std_ReturnType)0x0A)
#define CRYPTO_E_KEY_EMPTY          ((Std_ReturnType)0x0D)

/* Crypto operation modes (AUTOSAR SWS_Csm) */
typedef enum
{
    CRYPTO_OPERATIONMODE_SINGLECALL = 0x01,
    CRYPTO_OPERATIONMODE_START = 0x02,
    CRYPTO_OPERATIONMODE_UPDATE = 0x03,
    CRYPTO_OPERATIONMODE_FINISH = 0x04
}Crypto_OperationModeType;

typedef enum
{
    CRYPTO_E_VER_OK = 0x00,
    CRYPTO_E_VER_NOT_OK = 0x01
}Crypto_VerifyResultType;


/********************************************************************************************************/
/*****************************************FunctionPrototype**********************************************/
/********************************************************************************************************/

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

#endif