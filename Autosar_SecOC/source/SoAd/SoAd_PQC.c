/********************************************************************************************************/
/**********************************************SoAd_PQC.c*************************************************/
/********************************************************************************************************/
/**
 * @file SoAd_PQC.c
 * @brief PQC Integration for Socket Adapter implementation
 */

/********************************************************************************************************/
/************************************************INCLUDES************************************************/
/********************************************************************************************************/
#include "SoAd_PQC.h"
#include <string.h>
#include <stdio.h>

#ifdef LINUX
#include "ethernet.h"
#elif defined(WINDOWS)
#include "ethernet_windows.h"
#endif

/********************************************************************************************************/
/******************************************GLOBAL VARIABLES**********************************************/
/********************************************************************************************************/
static boolean SoAd_PQC_Initialized = FALSE;
static SoAd_PQC_StateType SoAd_PQC_States[SOAD_PQC_MAX_PEERS];

/********************************************************************************************************/
/**********************************************FUNCTIONS**************************************************/
/********************************************************************************************************/

/**
 * @brief Initialize SoAd PQC Integration
 */
Std_ReturnType SoAd_PQC_Init(void)
{
    Std_ReturnType result;
    uint8 i;

    if (SoAd_PQC_Initialized == TRUE)
    {
        return E_OK;
    }

    /* Initialize PQC module */
    result = PQC_Init();
    if (result != PQC_E_OK)
    {
        printf("ERROR: Failed to initialize PQC module in SoAd\n");
        return E_NOT_OK;
    }

    /* Initialize ML-KEM Key Exchange Manager */
    result = PQC_KeyExchange_Init();
    if (result != E_OK)
    {
        printf("ERROR: Failed to initialize PQC Key Exchange\n");
        return E_NOT_OK;
    }

    /* Initialize Key Derivation Module */
    result = PQC_KeyDerivation_Init();
    if (result != E_OK)
    {
        printf("ERROR: Failed to initialize PQC Key Derivation\n");
        return E_NOT_OK;
    }

    /* Initialize session states */
    for (i = 0; i < SOAD_PQC_MAX_PEERS; i++)
    {
        SoAd_PQC_States[i] = SOAD_PQC_STATE_IDLE;
    }

    SoAd_PQC_Initialized = TRUE;
    printf("SoAd PQC Integration initialized successfully\n");
    printf("  ML-KEM-768: Enabled for key exchange\n");
    printf("  ML-DSA-65: Enabled for signatures\n");
    printf("  Max Peers: %u\n", SOAD_PQC_MAX_PEERS);

    return E_OK;
}

/**
 * @brief Perform ML-KEM key exchange as INITIATOR (Alice)
 */
static Std_ReturnType SoAd_PQC_KeyExchange_Initiator(PQC_PeerIdType PeerId)
{
    Std_ReturnType result;
    uint8 publicKey[PQC_MLKEM_PUBLIC_KEY_BYTES];
    uint8 ciphertext[PQC_MLKEM_CIPHERTEXT_BYTES];
    uint8 sharedSecret[PQC_MLKEM_SHARED_SECRET_BYTES];
    PQC_SessionKeysType sessionKeys;
    uint16 actualSize;

    printf("[SoAd-PQC] Initiating ML-KEM key exchange with peer %u...\n", PeerId);

    /* Step 1: Generate ML-KEM keypair and get public key */
    SoAd_PQC_States[PeerId] = SOAD_PQC_STATE_KEY_EXCHANGE_INITIATED;

    result = PQC_KeyExchange_Initiate(PeerId, publicKey);
    if (result != E_OK)
    {
        printf("ERROR: ML-KEM keypair generation failed\n");
        SoAd_PQC_States[PeerId] = SOAD_PQC_STATE_FAILED;
        return E_NOT_OK;
    }

    printf("  [Step 1/3] Generated ML-KEM keypair, sending public key (%u bytes)...\n",
           PQC_MLKEM_PUBLIC_KEY_BYTES);

#if defined(__linux__) || defined(WINDOWS)
    /* Step 2: Send public key to peer via Ethernet */
    result = ethernet_send(PeerId, publicKey, PQC_MLKEM_PUBLIC_KEY_BYTES);
    if (result != E_OK)
    {
        printf("ERROR: Failed to send public key\n");
        SoAd_PQC_States[PeerId] = SOAD_PQC_STATE_FAILED;
        return E_NOT_OK;
    }

    printf("  [Step 2/3] Waiting for ciphertext from peer...\n");

    /* Step 3: Receive ciphertext from peer */
    unsigned short rxPeerId;
    result = ethernet_receive(ciphertext, PQC_MLKEM_CIPHERTEXT_BYTES, &rxPeerId, &actualSize);
    if (result != E_OK || actualSize != PQC_MLKEM_CIPHERTEXT_BYTES)
    {
        printf("ERROR: Failed to receive ciphertext (received %u bytes, expected %u)\n",
               actualSize, PQC_MLKEM_CIPHERTEXT_BYTES);
        SoAd_PQC_States[PeerId] = SOAD_PQC_STATE_FAILED;
        return E_NOT_OK;
    }
#else
    /* Simulation mode - for testing without actual Ethernet */
    printf("  [SIMULATION] Skipping actual Ethernet transmission\n");
    return E_OK;
#endif

    printf("  [Step 3/3] Received ciphertext (%u bytes), decapsulating...\n",
           PQC_MLKEM_CIPHERTEXT_BYTES);

    /* Step 4: Complete key exchange (decapsulate ciphertext) */
    result = PQC_KeyExchange_Complete(PeerId, ciphertext);
    if (result != E_OK)
    {
        printf("ERROR: ML-KEM decapsulation failed\n");
        SoAd_PQC_States[PeerId] = SOAD_PQC_STATE_FAILED;
        return E_NOT_OK;
    }

    SoAd_PQC_States[PeerId] = SOAD_PQC_STATE_KEY_EXCHANGE_COMPLETED;

    /* Step 5: Retrieve shared secret */
    result = PQC_KeyExchange_GetSharedSecret(PeerId, sharedSecret);
    if (result != E_OK)
    {
        printf("ERROR: Failed to get shared secret\n");
        SoAd_PQC_States[PeerId] = SOAD_PQC_STATE_FAILED;
        return E_NOT_OK;
    }

    printf("  [SUCCESS] Shared secret established (%u bytes)\n", PQC_MLKEM_SHARED_SECRET_BYTES);

    /* Step 6: Derive session keys from shared secret */
    printf("  [HKDF] Deriving session keys from shared secret...\n");
    result = PQC_DeriveSessionKeys(sharedSecret, PeerId, &sessionKeys);
    if (result != E_OK)
    {
        printf("ERROR: Session key derivation failed\n");
        SoAd_PQC_States[PeerId] = SOAD_PQC_STATE_FAILED;
        return E_NOT_OK;
    }

    SoAd_PQC_States[PeerId] = SOAD_PQC_STATE_SESSION_ESTABLISHED;

    printf("[SoAd-PQC] ML-KEM key exchange completed successfully!\n");
    printf("            Encryption key:     32 bytes\n");
    printf("            Authentication key: 32 bytes\n");
    printf("            Session state:      ESTABLISHED\n");

    /* Clear sensitive shared secret */
    memset(sharedSecret, 0, sizeof(sharedSecret));

    return E_OK;
}

/**
 * @brief Perform ML-KEM key exchange as RESPONDER (Bob)
 */
static Std_ReturnType SoAd_PQC_KeyExchange_Responder(PQC_PeerIdType PeerId)
{
    Std_ReturnType result;
    uint8 peerPublicKey[PQC_MLKEM_PUBLIC_KEY_BYTES];
    uint8 ciphertext[PQC_MLKEM_CIPHERTEXT_BYTES];
    uint8 sharedSecret[PQC_MLKEM_SHARED_SECRET_BYTES];
    PQC_SessionKeysType sessionKeys;
    uint16 actualSize;
    unsigned short rxPeerId;

    printf("[SoAd-PQC] Responding to ML-KEM key exchange from peer %u...\n", PeerId);

#if defined(__linux__) || defined(WINDOWS)
    /* Step 1: Receive public key from peer */
    printf("  [Step 1/2] Waiting for public key from peer...\n");

    result = ethernet_receive(peerPublicKey, PQC_MLKEM_PUBLIC_KEY_BYTES, &rxPeerId, &actualSize);
    if (result != E_OK || actualSize != PQC_MLKEM_PUBLIC_KEY_BYTES)
    {
        printf("ERROR: Failed to receive public key (received %u bytes, expected %u)\n",
               actualSize, PQC_MLKEM_PUBLIC_KEY_BYTES);
        SoAd_PQC_States[PeerId] = SOAD_PQC_STATE_FAILED;
        return E_NOT_OK;
    }

    printf("  Received public key (%u bytes)\n", PQC_MLKEM_PUBLIC_KEY_BYTES);
#else
    /* Simulation mode */
    printf("  [SIMULATION] Skipping actual Ethernet reception\n");
    return E_OK;
#endif

    /* Step 2: Respond (encapsulate to create ciphertext and shared secret) */
    SoAd_PQC_States[PeerId] = SOAD_PQC_STATE_KEY_EXCHANGE_INITIATED;

    result = PQC_KeyExchange_Respond(PeerId, peerPublicKey, ciphertext);
    if (result != E_OK)
    {
        printf("ERROR: ML-KEM encapsulation failed\n");
        SoAd_PQC_States[PeerId] = SOAD_PQC_STATE_FAILED;
        return E_NOT_OK;
    }

    printf("  [Step 2/2] Encapsulated shared secret, sending ciphertext (%u bytes)...\n",
           PQC_MLKEM_CIPHERTEXT_BYTES);

#if defined(__linux__) || defined(WINDOWS)
    /* Step 3: Send ciphertext to peer */
    result = ethernet_send(PeerId, ciphertext, PQC_MLKEM_CIPHERTEXT_BYTES);
    if (result != E_OK)
    {
        printf("ERROR: Failed to send ciphertext\n");
        SoAd_PQC_States[PeerId] = SOAD_PQC_STATE_FAILED;
        return E_NOT_OK;
    }
#endif

    SoAd_PQC_States[PeerId] = SOAD_PQC_STATE_KEY_EXCHANGE_COMPLETED;

    /* Step 4: Retrieve shared secret */
    result = PQC_KeyExchange_GetSharedSecret(PeerId, sharedSecret);
    if (result != E_OK)
    {
        printf("ERROR: Failed to get shared secret\n");
        SoAd_PQC_States[PeerId] = SOAD_PQC_STATE_FAILED;
        return E_NOT_OK;
    }

    printf("  [SUCCESS] Shared secret established (%u bytes)\n", PQC_MLKEM_SHARED_SECRET_BYTES);

    /* Step 5: Derive session keys from shared secret */
    printf("  [HKDF] Deriving session keys from shared secret...\n");
    result = PQC_DeriveSessionKeys(sharedSecret, PeerId, &sessionKeys);
    if (result != E_OK)
    {
        printf("ERROR: Session key derivation failed\n");
        SoAd_PQC_States[PeerId] = SOAD_PQC_STATE_FAILED;
        return E_NOT_OK;
    }

    SoAd_PQC_States[PeerId] = SOAD_PQC_STATE_SESSION_ESTABLISHED;

    printf("[SoAd-PQC] ML-KEM key exchange completed successfully!\n");
    printf("            Encryption key:     32 bytes\n");
    printf("            Authentication key: 32 bytes\n");
    printf("            Session state:      ESTABLISHED\n");

    /* Clear sensitive shared secret */
    memset(sharedSecret, 0, sizeof(sharedSecret));

    return E_OK;
}

/**
 * @brief Perform ML-KEM key exchange with Ethernet peer
 */
Std_ReturnType SoAd_PQC_KeyExchange(
    PQC_PeerIdType PeerId,
    boolean IsInitiator)
{
    if (SoAd_PQC_Initialized == FALSE)
    {
        printf("ERROR: SoAd PQC not initialized\n");
        return E_NOT_OK;
    }

    if (PeerId >= SOAD_PQC_MAX_PEERS)
    {
        printf("ERROR: Invalid peer ID %u\n", PeerId);
        return E_NOT_OK;
    }

    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║          ML-KEM-768 KEY EXCHANGE - ETHERNET GATEWAY        ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("  Peer ID: %u\n", PeerId);
    printf("  Role:    %s\n", IsInitiator ? "Initiator (Alice)" : "Responder (Bob)");
    printf("  Protocol: ML-KEM-768 (NIST FIPS 203)\n");
    printf("\n");

    if (IsInitiator)
    {
        return SoAd_PQC_KeyExchange_Initiator(PeerId);
    }
    else
    {
        return SoAd_PQC_KeyExchange_Responder(PeerId);
    }
}

/**
 * @brief Get current PQC session state for a peer
 */
SoAd_PQC_StateType SoAd_PQC_GetState(PQC_PeerIdType PeerId)
{
    if (PeerId >= SOAD_PQC_MAX_PEERS)
    {
        return SOAD_PQC_STATE_FAILED;
    }

    return SoAd_PQC_States[PeerId];
}

/**
 * @brief Reset PQC session with a peer
 */
Std_ReturnType SoAd_PQC_ResetSession(PQC_PeerIdType PeerId)
{
    Std_ReturnType result;

    if (PeerId >= SOAD_PQC_MAX_PEERS)
    {
        return E_NOT_OK;
    }

    /* Reset key exchange session */
    result = PQC_KeyExchange_Reset(PeerId);
    if (result != E_OK)
    {
        return E_NOT_OK;
    }

    /* Clear session keys */
    result = PQC_ClearSessionKeys(PeerId);
    if (result != E_OK)
    {
        return E_NOT_OK;
    }

    SoAd_PQC_States[PeerId] = SOAD_PQC_STATE_IDLE;

    printf("SoAd PQC session reset for peer %u\n", PeerId);

    return E_OK;
}
