/********************************************************************************************************/
/*****************************************PQC_KeyExchange.c***********************************************/
/********************************************************************************************************/
/**
 * @file PQC_KeyExchange.c
 * @brief ML-KEM Key Exchange Manager implementation
 */

/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/
#include "PQC_KeyExchange.h"
#include <string.h>
#include <stdio.h>

/********************************************************************************************************/
/******************************************GLOBAL VARIABLES**********************************************/
/********************************************************************************************************/
static boolean PQC_KeyExchange_Initialized = FALSE;
static PQC_KeyExchangeSessionType PQC_Sessions[PQC_MAX_PEERS];

/********************************************************************************************************/
/**********************************************FUNCTIONS**************************************************/
/********************************************************************************************************/

/**
 * @brief Initialize the Key Exchange Manager
 */
Std_ReturnType PQC_KeyExchange_Init(void)
{
    uint8 i;

    if (PQC_KeyExchange_Initialized == TRUE)
    {
        return E_OK;
    }

    /* Initialize all sessions */
    for (i = 0; i < PQC_MAX_PEERS; i++)
    {
        PQC_Sessions[i].PeerId = i;
        PQC_Sessions[i].State = PQC_KE_STATE_IDLE;
        PQC_Sessions[i].IsInitiator = FALSE;
        PQC_Sessions[i].Timestamp = 0;

        /* Clear sensitive data */
        (void)memset(&PQC_Sessions[i].LocalKeyPair, 0, sizeof(PQC_MLKEM_KeyPairType));
        (void)memset(PQC_Sessions[i].PeerPublicKey, 0, PQC_MLKEM_PUBLIC_KEY_BYTES);
        (void)memset(PQC_Sessions[i].SharedSecret, 0, PQC_MLKEM_SHARED_SECRET_BYTES);
        (void)memset(PQC_Sessions[i].Ciphertext, 0, PQC_MLKEM_CIPHERTEXT_BYTES);
    }

    PQC_KeyExchange_Initialized = TRUE;
    (void)printf("PQC Key Exchange Manager initialized (%u peer slots)\n", PQC_MAX_PEERS);

    return E_OK;
}

/**
 * @brief Initiate key exchange as initiator (Alice)
 */
Std_ReturnType PQC_KeyExchange_Initiate(
    PQC_PeerIdType PeerId,
    uint8* PublicKey)
{
    Std_ReturnType result;

    if (PQC_KeyExchange_Initialized == FALSE)
    {
        (void)printf("ERROR: Key Exchange Manager not initialized\n");
        return E_NOT_OK;
    }

    if (PeerId >= PQC_MAX_PEERS)
    {
        (void)printf("ERROR: Invalid peer ID %u\n", PeerId);
        return E_NOT_OK;
    }

    if (PublicKey == NULL)
    {
        return E_NOT_OK;
    }

    PQC_KeyExchangeSessionType* session = &PQC_Sessions[PeerId];

    /* Generate keypair */
    result = PQC_MLKEM_KeyGen(&session->LocalKeyPair);
    if (result != PQC_E_OK)
    {
        (void)printf("ERROR: Failed to generate ML-KEM keypair for peer %u\n", PeerId);
        session->State = PQC_KE_STATE_FAILED;
        return E_NOT_OK;
    }

    /* Copy public key to output */
    (void)memcpy(PublicKey, session->LocalKeyPair.PublicKey, PQC_MLKEM_PUBLIC_KEY_BYTES);

    /* Update session state */
    session->State = PQC_KE_STATE_INITIATED;
    session->IsInitiator = TRUE;

    (void)printf("KE: Initiated key exchange with peer %u (sent %u byte public key)\n",
                 PeerId, PQC_MLKEM_PUBLIC_KEY_BYTES);

    return E_OK;
}

/**
 * @brief Respond to key exchange as responder (Bob)
 */
Std_ReturnType PQC_KeyExchange_Respond(
    PQC_PeerIdType PeerId,
    const uint8* PeerPublicKey,
    uint8* Ciphertext)
{
    Std_ReturnType result;
    PQC_MLKEM_SharedSecretType shared_secret_data;

    if (PQC_KeyExchange_Initialized == FALSE)
    {
        return E_NOT_OK;
    }

    if ((PeerId >= PQC_MAX_PEERS) || (PeerPublicKey == NULL) || (Ciphertext == NULL))
    {
        return E_NOT_OK;
    }

    PQC_KeyExchangeSessionType* session = &PQC_Sessions[PeerId];

    /* Store peer's public key */
    (void)memcpy(session->PeerPublicKey, PeerPublicKey, PQC_MLKEM_PUBLIC_KEY_BYTES);

    /* Encapsulate to create shared secret and ciphertext */
    result = PQC_MLKEM_Encapsulate(PeerPublicKey, &shared_secret_data);
    if (result != PQC_E_OK)
    {
        (void)printf("ERROR: Failed to encapsulate for peer %u\n", PeerId);
        session->State = PQC_KE_STATE_FAILED;
        return E_NOT_OK;
    }

    /* Store shared secret and ciphertext */
    (void)memcpy(session->SharedSecret, shared_secret_data.SharedSecret, PQC_MLKEM_SHARED_SECRET_BYTES);
    (void)memcpy(session->Ciphertext, shared_secret_data.Ciphertext, PQC_MLKEM_CIPHERTEXT_BYTES);
    (void)memcpy(Ciphertext, shared_secret_data.Ciphertext, PQC_MLKEM_CIPHERTEXT_BYTES);

    /* Update session state */
    session->State = PQC_KE_STATE_ESTABLISHED;
    session->IsInitiator = FALSE;

    (void)printf("KE: Responded to peer %u (sent %u byte ciphertext, established %u byte shared secret)\n",
                 PeerId, PQC_MLKEM_CIPHERTEXT_BYTES, PQC_MLKEM_SHARED_SECRET_BYTES);

    return E_OK;
}

/**
 * @brief Complete key exchange as initiator (Alice receives ciphertext)
 */
Std_ReturnType PQC_KeyExchange_Complete(
    PQC_PeerIdType PeerId,
    const uint8* Ciphertext)
{
    Std_ReturnType result;

    if (PQC_KeyExchange_Initialized == FALSE)
    {
        return E_NOT_OK;
    }

    if ((PeerId >= PQC_MAX_PEERS) || (Ciphertext == NULL))
    {
        return E_NOT_OK;
    }

    PQC_KeyExchangeSessionType* session = &PQC_Sessions[PeerId];

    /* Verify we initiated this exchange */
    if ((session->State != PQC_KE_STATE_INITIATED) || (session->IsInitiator == FALSE))
    {
        (void)printf("ERROR: Invalid state for completing key exchange with peer %u\n", PeerId);
        return E_NOT_OK;
    }

    /* Decapsulate to extract shared secret */
    result = PQC_MLKEM_Decapsulate(
        Ciphertext,
        session->LocalKeyPair.SecretKey,
        session->SharedSecret
    );

    if (result != PQC_E_OK)
    {
        (void)printf("ERROR: Failed to decapsulate for peer %u\n", PeerId);
        session->State = PQC_KE_STATE_FAILED;
        return E_NOT_OK;
    }

    /* Store ciphertext for reference */
    (void)memcpy(session->Ciphertext, Ciphertext, PQC_MLKEM_CIPHERTEXT_BYTES);

    /* Update session state */
    session->State = PQC_KE_STATE_ESTABLISHED;

    (void)printf("KE: Completed key exchange with peer %u (established %u byte shared secret)\n",
                 PeerId, PQC_MLKEM_SHARED_SECRET_BYTES);

    /* Clear local secret key for security */
    (void)memset(session->LocalKeyPair.SecretKey, 0, PQC_MLKEM_SECRET_KEY_BYTES);

    return E_OK;
}

/**
 * @brief Get established shared secret with a peer
 */
Std_ReturnType PQC_KeyExchange_GetSharedSecret(
    PQC_PeerIdType PeerId,
    uint8* SharedSecret)
{
    if (PQC_KeyExchange_Initialized == FALSE)
    {
        return E_NOT_OK;
    }

    if ((PeerId >= PQC_MAX_PEERS) || (SharedSecret == NULL))
    {
        return E_NOT_OK;
    }

    PQC_KeyExchangeSessionType* session = &PQC_Sessions[PeerId];

    /* Verify key is established */
    if (session->State != PQC_KE_STATE_ESTABLISHED)
    {
        (void)printf("ERROR: No established key with peer %u (state=%u)\n", PeerId, session->State);
        return E_NOT_OK;
    }

    /* Copy shared secret */
    (void)memcpy(SharedSecret, session->SharedSecret, PQC_MLKEM_SHARED_SECRET_BYTES);

    return E_OK;
}

/**
 * @brief Get key exchange state for a peer
 */
uint8 PQC_KeyExchange_GetState(PQC_PeerIdType PeerId)
{
    if (PeerId >= PQC_MAX_PEERS)
    {
        return PQC_KE_STATE_FAILED;
    }

    return PQC_Sessions[PeerId].State;
}

/**
 * @brief Reset key exchange session with a peer
 */
Std_ReturnType PQC_KeyExchange_Reset(PQC_PeerIdType PeerId)
{
    if (PeerId >= PQC_MAX_PEERS)
    {
        return E_NOT_OK;
    }

    PQC_KeyExchangeSessionType* session = &PQC_Sessions[PeerId];

    /* Clear all sensitive data */
    (void)memset(&session->LocalKeyPair, 0, sizeof(PQC_MLKEM_KeyPairType));
    (void)memset(session->PeerPublicKey, 0, PQC_MLKEM_PUBLIC_KEY_BYTES);
    (void)memset(session->SharedSecret, 0, PQC_MLKEM_SHARED_SECRET_BYTES);
    (void)memset(session->Ciphertext, 0, PQC_MLKEM_CIPHERTEXT_BYTES);

    session->State = PQC_KE_STATE_IDLE;
    session->IsInitiator = FALSE;
    session->Timestamp = 0;

    (void)printf("KE: Reset session with peer %u\n", PeerId);

    return E_OK;
}
