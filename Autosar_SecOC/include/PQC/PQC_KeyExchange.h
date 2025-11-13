/********************************************************************************************************/
/*****************************************PQC_KeyExchange.h***********************************************/
/********************************************************************************************************/
/**
 * @file PQC_KeyExchange.h
 * @brief ML-KEM Key Exchange Manager for AUTOSAR SecOC
 * @details Manages post-quantum key exchange between ECUs using ML-KEM-768
 */

#ifndef PQC_KEYEXCHANGE_H
#define PQC_KEYEXCHANGE_H

/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/
#include "Std_Types.h"
#include "PQC.h"

/********************************************************************************************************/
/***********************************************DEFINES***************************************************/
/********************************************************************************************************/

/* Maximum number of peer ECUs */
#define PQC_MAX_PEERS               8U

/* Key exchange states */
#define PQC_KE_STATE_IDLE           0U
#define PQC_KE_STATE_INITIATED      1U
#define PQC_KE_STATE_RESPONDED      2U
#define PQC_KE_STATE_ESTABLISHED    3U
#define PQC_KE_STATE_FAILED         4U

/********************************************************************************************************/
/**********************************************TYPEDEFS***************************************************/
/********************************************************************************************************/

/**
 * @brief Peer ECU identifier type
 */
typedef uint8 PQC_PeerIdType;

/**
 * @brief Key exchange state for a peer
 */
typedef struct {
    PQC_PeerIdType PeerId;
    uint8 State;
    PQC_MLKEM_KeyPairType LocalKeyPair;
    uint8 PeerPublicKey[PQC_MLKEM_PUBLIC_KEY_BYTES];
    uint8 SharedSecret[PQC_MLKEM_SHARED_SECRET_BYTES];
    uint8 Ciphertext[PQC_MLKEM_CIPHERTEXT_BYTES];
    boolean IsInitiator;
    uint32 Timestamp;
} PQC_KeyExchangeSessionType;

/********************************************************************************************************/
/***************************************FUNCTION DECLARATIONS********************************************/
/********************************************************************************************************/

/**
 * @brief Initialize the Key Exchange Manager
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType PQC_KeyExchange_Init(void);

/**
 * @brief Initiate key exchange as initiator (Alice)
 * @param[in] PeerId Identifier of peer ECU
 * @param[out] PublicKey Our public key to send to peer (1184 bytes)
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType PQC_KeyExchange_Initiate(
    PQC_PeerIdType PeerId,
    uint8* PublicKey
);

/**
 * @brief Respond to key exchange as responder (Bob)
 * @param[in] PeerId Identifier of peer ECU
 * @param[in] PeerPublicKey Public key received from peer (1184 bytes)
 * @param[out] Ciphertext Ciphertext to send back to peer (1088 bytes)
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType PQC_KeyExchange_Respond(
    PQC_PeerIdType PeerId,
    const uint8* PeerPublicKey,
    uint8* Ciphertext
);

/**
 * @brief Complete key exchange as initiator (Alice receives ciphertext)
 * @param[in] PeerId Identifier of peer ECU
 * @param[in] Ciphertext Ciphertext received from peer (1088 bytes)
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType PQC_KeyExchange_Complete(
    PQC_PeerIdType PeerId,
    const uint8* Ciphertext
);

/**
 * @brief Get established shared secret with a peer
 * @param[in] PeerId Identifier of peer ECU
 * @param[out] SharedSecret Buffer to store shared secret (32 bytes)
 * @return E_OK if key is established, E_NOT_OK otherwise
 */
Std_ReturnType PQC_KeyExchange_GetSharedSecret(
    PQC_PeerIdType PeerId,
    uint8* SharedSecret
);

/**
 * @brief Get key exchange state for a peer
 * @param[in] PeerId Identifier of peer ECU
 * @return Current state (PQC_KE_STATE_*)
 */
uint8 PQC_KeyExchange_GetState(PQC_PeerIdType PeerId);

/**
 * @brief Reset key exchange session with a peer
 * @param[in] PeerId Identifier of peer ECU
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType PQC_KeyExchange_Reset(PQC_PeerIdType PeerId);

#endif /* PQC_KEYEXCHANGE_H */
