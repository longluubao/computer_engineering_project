/********************************************************************************************************/
/*********************************************SecOC_PQC_Cfg.h*********************************************/
/********************************************************************************************************/
/**
 * @file SecOC_PQC_Cfg.h
 * @brief Post-Quantum Cryptography Configuration for SecOC
 */

#ifndef SECOC_PQC_CFG_H
#define SECOC_PQC_CFG_H

/********************************************************************************************************/
/***********************************************DEFINES***************************************************/
/********************************************************************************************************/

/**
 * @brief Enable/Disable PQC mode globally
 * TRUE  = Use ML-DSA signatures (Post-Quantum)
 * FALSE = Use MAC/HMAC (Classical)
 */
#define SECOC_USE_PQC_MODE              TRUE

/**
 * @brief Enable ML-KEM key exchange
 * Requires SECOC_USE_PQC_MODE = TRUE
 */
#define SECOC_USE_MLKEM_KEY_EXCHANGE    TRUE

/**
 * @brief Ethernet Gateway mode (removes CAN/FlexRay dependencies)
 */
#define SECOC_ETHERNET_GATEWAY_MODE     TRUE

/**
 * @brief Maximum PDU size for PQC signatures
 * ML-DSA-65 signature is 3309 bytes, so total PDU can be large
 */
#define SECOC_PQC_MAX_PDU_SIZE          8192U

#endif /* SECOC_PQC_CFG_H */
