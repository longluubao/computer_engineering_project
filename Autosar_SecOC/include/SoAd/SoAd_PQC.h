/********************************************************************************************************/
/**********************************************SoAd_PQC.h*************************************************/
/********************************************************************************************************/
/**
 * @file SoAd_PQC.h
 * @brief PQC Integration for Socket Adapter (Ethernet Gateway)
 * @details Provides ML-KEM key exchange functionality for establishing
 *          quantum-resistant secure channels between Ethernet peers
 */

#ifndef SOAD_PQC_H
#define SOAD_PQC_H

/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/
#include "Std_Types.h"
#include "PQC.h"
#include "PQC_KeyExchange.h"
#include "PQC_KeyDerivation.h"

/********************************************************************************************************/
/***********************************************DEFINES***************************************************/
/********************************************************************************************************/

#define SOAD_PQC_MAX_PEERS      PQC_MAX_PEERS

/********************************************************************************************************/
/**********************************************TYPEDEFS***************************************************/
/********************************************************************************************************/

/**
 * @brief SoAd PQC session state
 */
typedef enum {
    SOAD_PQC_STATE_IDLE = 0,
    SOAD_PQC_STATE_KEY_EXCHANGE_INITIATED,
    SOAD_PQC_STATE_KEY_EXCHANGE_COMPLETED,
    SOAD_PQC_STATE_SESSION_ESTABLISHED,
    SOAD_PQC_STATE_FAILED
} SoAd_PQC_StateType;

/********************************************************************************************************/
/***************************************FUNCTION DECLARATIONS********************************************/
/********************************************************************************************************/

/**
 * @brief Initialize SoAd PQC Integration
 * @details Must be called before any PQC operations
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType SoAd_PQC_Init(void);

/**
 * @brief Perform ML-KEM key exchange with Ethernet peer
 * @details Executes 3-way ML-KEM handshake:
 *          1. Generate keypair, send public key
 *          2. Receive ciphertext from peer
 *          3. Decapsulate to derive shared secret
 *          4. Derive session keys using HKDF
 *
 * @param[in] PeerId Peer identifier (0-15)
 * @param[in] IsInitiator TRUE if this node initiates, FALSE if responding
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType SoAd_PQC_KeyExchange(
    PQC_PeerIdType PeerId,
    boolean IsInitiator
);

/**
 * @brief Get current PQC session state for a peer
 * @param[in] PeerId Peer identifier
 * @return Current session state
 */
SoAd_PQC_StateType SoAd_PQC_GetState(PQC_PeerIdType PeerId);

/**
 * @brief Reset PQC session with a peer
 * @param[in] PeerId Peer identifier
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType SoAd_PQC_ResetSession(PQC_PeerIdType PeerId);

#endif /* SOAD_PQC_H */
