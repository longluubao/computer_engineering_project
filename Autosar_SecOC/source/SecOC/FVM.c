/* 
 * This is a compliant Freshness Value Manager (FVM) implementation 
 * based on AUTOSAR R19-11 [UC_SecOC_00200]
 */

#include "FVM.h"
#include <string.h>
#include "SecOC_Debug.h"

/********************************************************************************************************/
/******************************************Global Variables**********************************************/
/********************************************************************************************************/

/* Freshness Counter - Stored in Big Endian (Index 0 is MSB) */
static SecOC_FreshnessArrayType Freshness_Counter[SecOC_FreshnessValue_ID_MAX];
static uint32 Freshness_Counter_length_bits[SecOC_FreshnessValue_ID_MAX];

/********************************************************************************************************/
/********************************************Functions***************************************************/
/********************************************************************************************************/

/* [SWS_SecOC_91005] Equivalent utility to increment counter */
Std_ReturnType SecOC_IncreaseCounter(uint16 SecOCFreshnessValueID) {
    if (SecOCFreshnessValueID >= SecOC_FreshnessValue_ID_MAX) {
        return E_NOT_OK;
    }

    uint32 maxBytes = BIT_TO_BYTES(SECOC_MAX_FRESHNESS_SIZE);
    /* Big Endian Increment: Start from the last byte (LSB) */
    for (int i = (int)maxBytes - 1; i >= 0; i--) {
        Freshness_Counter[SecOCFreshnessValueID][i]++;
        if (Freshness_Counter[SecOCFreshnessValueID][i] != 0) {
            break; 
        }
    }
    return E_OK;
}

/* [SWS_SecOC_91006] Get Freshness for Transmission */
Std_ReturnType SecOC_GetTxFreshness(uint16 SecOCFreshnessValueID, uint8* SecOCFreshnessValue, uint32* SecOCFreshnessValueLength) {
    if (SecOCFreshnessValueID >= SecOC_FreshnessValue_ID_MAX) {
        return E_NOT_OK;
    }

    uint32 currentLen = Freshness_Counter_length_bits[SecOCFreshnessValueID];
    /* If length is not initialized, default to maximum allowed size */
    if (currentLen == 0) {
        currentLen = SECOC_MAX_FRESHNESS_SIZE;
    }

    (void)memcpy(SecOCFreshnessValue, Freshness_Counter[SecOCFreshnessValueID], BIT_TO_BYTES(currentLen));
    *SecOCFreshnessValueLength = currentLen;

    return E_OK;
}

/* [SWS_SecOC_91007] Get Freshness for Reception (Reconstruction Logic) */
Std_ReturnType SecOC_GetRxFreshness(
    uint16 SecOCFreshnessValueID, 
    const uint8* SecOCTruncatedFreshnessValue, 
    uint32 SecOCTruncatedFreshnessValueLength, 
    uint16 SecOCAuthVerifyAttempts, 
    uint8* SecOCFreshnessValue, 
    uint32* SecOCFreshnessValueLength
) {
    if (SecOCFreshnessValueID >= SecOC_FreshnessValue_ID_MAX) {
        return E_NOT_OK;
    }

    uint32 configFullLength = Freshness_Counter_length_bits[SecOCFreshnessValueID];
    if (configFullLength == 0) {
        configFullLength = SECOC_MAX_FRESHNESS_SIZE;
    }
    
    uint32 truncLen = SecOCTruncatedFreshnessValueLength;
    uint8* lastStored = Freshness_Counter[SecOCFreshnessValueID];

    /* Case 1: Full Freshness is transmitted [SWS_SecOC_00094] */
    if (truncLen == configFullLength) {
        (void)memcpy(SecOCFreshnessValue, SecOCTruncatedFreshnessValue, BIT_TO_BYTES(truncLen));
    } 
    /* Case 2: Truncated Freshness (Standard Reconstruction) [SWS_SecOC_91007] */
    else {
        uint32 msbLen = configFullLength - truncLen;
        uint32 msbBytes = BIT_TO_BYTES(msbLen);
        uint32 truncBytes = BIT_TO_BYTES(truncLen);
        uint32 fullBytes = BIT_TO_BYTES(configFullLength);

        /* 1. Copy MSB from the last successfully received/stored counter */
        (void)memcpy(SecOCFreshnessValue, lastStored, msbBytes);
        
        /* 2. Compare Truncated Part with stored LSB (last truncBytes of the stored counter) */
        if (memcmp(SecOCTruncatedFreshnessValue, &lastStored[fullBytes - truncBytes], truncBytes) > 0) {
            /* Received Truncated Value > Stored LSB: No rollover, use current MSB */
            (void)memcpy(&SecOCFreshnessValue[fullBytes - truncBytes], SecOCTruncatedFreshnessValue, truncBytes);
        } else {
            /* Received Truncated Value <= Stored LSB: Rollover detected, increment MSB */
            (void)memcpy(&SecOCFreshnessValue[fullBytes - truncBytes], SecOCTruncatedFreshnessValue, truncBytes);
            
            /* Increment MSB part of the reconstructed value */
            for (int i = (int)msbBytes - 1; i >= 0; i--) {
                SecOCFreshnessValue[i]++;
                if (SecOCFreshnessValue[i] != 0) {
                    break;
                }
            }
        }
    }

    *SecOCFreshnessValueLength = configFullLength;
    return E_OK;
}

/* [SWS_SecOC_91003] Get Tx Freshness and Truncated Data */
Std_ReturnType SecOC_GetTxFreshnessTruncData(
    uint16 SecOCFreshnessValueID, 
    uint8* SecOCFreshnessValue, 
    uint32* SecOCFreshnessValueLength, 
    uint8* SecOCTruncatedFreshnessValue, 
    uint32* SecOCTruncatedFreshnessValueLength
) {
    Std_ReturnType ret = SecOC_GetTxFreshness(SecOCFreshnessValueID, SecOCFreshnessValue, SecOCFreshnessValueLength);
    
    if (ret == E_OK && SecOCTruncatedFreshnessValue != (void*)0) {
        uint32 truncBits = *SecOCTruncatedFreshnessValueLength;
        uint32 fullBytes = BIT_TO_BYTES(*SecOCFreshnessValueLength);
        uint32 truncBytes = BIT_TO_BYTES(truncBits);
        
        /* Extract the Least Significant Bits (LSB) for truncation as per [SWS_SecOC_00095] */
        (void)memcpy(SecOCTruncatedFreshnessValue, &SecOCFreshnessValue[fullBytes - truncBytes], truncBytes);
    }
    return ret;
}

/* Internal utility to update the local counter notion */
Std_ReturnType SecOC_UpdateCounter(uint16 SecOCFreshnessValueID, uint8* SecOCFreshnessValue, uint32 SecOCFreshnessValueLength) {
    if (SecOCFreshnessValueID >= SecOC_FreshnessValue_ID_MAX) {
        return E_NOT_OK;
    }
    uint32 lengthBytes = BIT_TO_BYTES(SecOCFreshnessValueLength);
    (void)memcpy(Freshness_Counter[SecOCFreshnessValueID], SecOCFreshnessValue, lengthBytes);
    Freshness_Counter_length_bits[SecOCFreshnessValueID] = SecOCFreshnessValueLength;
    return E_OK;
}
