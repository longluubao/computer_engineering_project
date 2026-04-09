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
#include "SoAd.h"
#include "TcpIp.h"
#include "Csm.h"
#include "SecOC/SecOC_PQC_Cfg.h"
#include <string.h>
#include <stdio.h>

/* MISRA C:2012 Rule 8.4 - Forward declarations for external linkage functions */
extern Std_ReturnType SoAd_PQC_Init(void);
extern void SoAd_PQC_DeInit(void);
extern void SoAd_PQC_MainFunction(void);
extern Std_ReturnType SoAd_PQC_KeyExchange(Csm_PeerIdType PeerId, boolean IsInitiator);
extern boolean SoAd_PQC_HandleControlMessage(const uint8* BufPtr, uint16 Length);
extern SoAd_PQC_StateType SoAd_PQC_GetState(Csm_PeerIdType PeerId);
extern Std_ReturnType SoAd_PQC_ResetSession(Csm_PeerIdType PeerId);

#define SOAD_PQC_CTRL_MAGIC0               ((uint8)0x51U)
#define SOAD_PQC_CTRL_MAGIC1               ((uint8)0x43U)
#define SOAD_PQC_CTRL_VERSION              ((uint8)0x01U)
#define SOAD_PQC_CTRL_TYPE_PUBKEY          ((uint8)0x01U)
#define SOAD_PQC_CTRL_TYPE_CIPHERTEXT      ((uint8)0x02U)
#define SOAD_PQC_CTRL_HEADER_LENGTH        ((uint16)7U)
#define SOAD_PQC_CTRL_MAX_PAYLOAD          ((uint16)CSM_MLKEM_PUBLIC_KEY_BYTES)

/********************************************************************************************************/
/******************************************GLOBAL VARIABLES**********************************************/
/********************************************************************************************************/
static boolean SoAd_PQC_Initialized = FALSE;
static SoAd_PQC_StateType SoAd_PQC_States[SOAD_PQC_MAX_PEERS];
static uint32 SoAd_PQC_RekeyCycles[SOAD_PQC_MAX_PEERS];

/* Forward declarations for static helpers */
static Std_ReturnType SoAd_PQC_KeyExchange_Initiator(Csm_PeerIdType PeerId);

static Std_ReturnType SoAd_PQC_DeriveSessionFromPeer(Csm_PeerIdType PeerId)
{
    uint8 sharedSecret[CSM_MLKEM_SHARED_SECRET_BYTES];
    uint32 actualSize = CSM_MLKEM_SHARED_SECRET_BYTES;
    Std_ReturnType result;

    result = Csm_KeyExchangeGetSharedSecret(0U, PeerId, sharedSecret, &actualSize);
    if ((result != E_OK) || (actualSize != CSM_MLKEM_SHARED_SECRET_BYTES))
    {
        return E_NOT_OK;
    }

    result = Csm_DeriveSessionKeys(PeerId, sharedSecret, CSM_MLKEM_SHARED_SECRET_BYTES);
    (void)memset(sharedSecret, 0, sizeof(sharedSecret));
    return result;
}

static Std_ReturnType SoAd_PQC_SendControlMessage(Csm_PeerIdType PeerId,
                                                  uint8 MessageType,
                                                  const uint8* PayloadPtr,
                                                  uint16 PayloadLength)
{
    uint8 frame[SOAD_PQC_CTRL_HEADER_LENGTH + SOAD_PQC_CTRL_MAX_PAYLOAD];
    TcpIp_SockAddrType remoteAddr;
    SoAd_SoConIdType soConId;
    TcpIp_SocketIdType socketId = TCPIP_SOCKET_ID_INVALID;
    Std_ReturnType result;
    uint16 totalLength;

    if ((PayloadPtr == NULL) || (PayloadLength > SOAD_PQC_CTRL_MAX_PAYLOAD))
    {
        return E_NOT_OK;
    }

    frame[0] = SOAD_PQC_CTRL_MAGIC0;
    frame[1] = SOAD_PQC_CTRL_MAGIC1;
    frame[2] = SOAD_PQC_CTRL_VERSION;
    frame[3] = MessageType;
    frame[4] = (uint8)PeerId;
    frame[5] = (uint8)(PayloadLength & 0xFFU);
    frame[6] = (uint8)((PayloadLength >> 8U) & 0xFFU);
    (void)memcpy(&frame[SOAD_PQC_CTRL_HEADER_LENGTH], PayloadPtr, PayloadLength);
    totalLength = (uint16)(SOAD_PQC_CTRL_HEADER_LENGTH + PayloadLength);

    result = SoAd_GetSoConId((PduIdType)PeerId, &soConId);
    if (result == E_OK)
    {
        (void)SoAd_GetRemoteAddr(soConId, &remoteAddr);
    }
    else
    {
        remoteAddr.domain = TCPIP_AF_INET;
        remoteAddr.addr[0] = 127U;
        remoteAddr.addr[1] = 0U;
        remoteAddr.addr[2] = 0U;
        remoteAddr.addr[3] = 1U;
        remoteAddr.port = (uint16)(60000U + PeerId);
    }

    result = TcpIp_GetSocketId(TCPIP_AF_INET, TCPIP_IPPROTO_UDP, &socketId);
    if (result != E_OK)
    {
        return result;
    }

    result = TcpIp_UdpTransmit(socketId, frame, &remoteAddr, totalLength);
    (void)TcpIp_Close(socketId, FALSE);
    return result;
}

/********************************************************************************************************/
/**********************************************FUNCTIONS**************************************************/
/********************************************************************************************************/

/**
 * @brief Initialize SoAd PQC Integration
 */
Std_ReturnType SoAd_PQC_Init(void)
{
    uint8 i;

    if (SoAd_PQC_Initialized == TRUE)
    {
        return E_OK;
    }

    /* Initialize session states and rekey counters */
    for (i = 0; i < SOAD_PQC_MAX_PEERS; i++)
    {
        SoAd_PQC_States[i] = SOAD_PQC_STATE_IDLE;
        SoAd_PQC_RekeyCycles[i] = 0U;
    }

    SoAd_PQC_Initialized = TRUE;
    (void)printf("SoAd PQC Integration initialized successfully\n");
    (void)printf("  ML-KEM-768: Enabled for key exchange\n");
    (void)printf("  ML-DSA-65: Enabled for signatures\n");
    (void)printf("  Max Peers: %u\n", SOAD_PQC_MAX_PEERS);

    return E_OK;
}

void SoAd_PQC_DeInit(void)
{
    Csm_PeerIdType peerId;

    if (SoAd_PQC_Initialized == FALSE)
    {
        return;
    }

    for (peerId = 0U; peerId < SOAD_PQC_MAX_PEERS; peerId++)
    {
        (void)Csm_KeyExchangeReset(peerId);
        (void)Csm_ClearSessionKeys(peerId);
        SoAd_PQC_States[peerId] = SOAD_PQC_STATE_IDLE;
    }

    SoAd_PQC_Initialized = FALSE;
}

/**
 * @brief Periodic main function for SoAd PQC
 * @details Called cyclically by the scheduler. Tracks session age per peer
 *          and triggers automatic rekeying when SOAD_PQC_REKEY_INTERVAL_CYCLES
 *          is reached (set to 0 to disable). Also handles asynchronous
 *          completion via SoAd_PQC_HandleControlMessage().
 */
void SoAd_PQC_MainFunction(void)
{
#if (SOAD_PQC_REKEY_INTERVAL_CYCLES > 0U)
    Csm_PeerIdType peerId;

    if (SoAd_PQC_Initialized == FALSE)
    {
        return;
    }

    for (peerId = 0U; peerId < SOAD_PQC_MAX_PEERS; peerId++)
    {
        if (SoAd_PQC_States[peerId] == SOAD_PQC_STATE_SESSION_ESTABLISHED)
        {
            SoAd_PQC_RekeyCycles[peerId]++;

            if (SoAd_PQC_RekeyCycles[peerId] >= SOAD_PQC_REKEY_INTERVAL_CYCLES)
            {
                (void)printf("SoAd PQC: Rekeying triggered for peer %u (after %u cycles)\n",
                             peerId, SoAd_PQC_RekeyCycles[peerId]);

                /* Reset the current session */
                (void)Csm_KeyExchangeReset(peerId);
                (void)Csm_ClearSessionKeys(peerId);
                SoAd_PQC_RekeyCycles[peerId] = 0U;

                /* Re-initiate key exchange as initiator */
                if (SoAd_PQC_KeyExchange_Initiator(peerId) != E_OK)
                {
                    (void)printf("SoAd PQC: Rekey initiation failed for peer %u\n", peerId);
                    SoAd_PQC_States[peerId] = SOAD_PQC_STATE_FAILED;
                }
            }
        }
        else
        {
            /* Reset counter for non-established sessions */
            SoAd_PQC_RekeyCycles[peerId] = 0U;
        }
    }
#endif
}

/**
 * @brief Perform ML-KEM key exchange as INITIATOR (Alice)
 */
static Std_ReturnType SoAd_PQC_KeyExchange_Initiator(Csm_PeerIdType PeerId)
{
    Std_ReturnType result;
    uint8 publicKey[CSM_MLKEM_PUBLIC_KEY_BYTES];
    uint32 actualSize = CSM_MLKEM_PUBLIC_KEY_BYTES;

    SoAd_PQC_States[PeerId] = SOAD_PQC_STATE_KEY_EXCHANGE_INITIATED;
    result = Csm_KeyExchangeInitiate(0U, PeerId, publicKey, &actualSize);
    if (result != E_OK)
    {
        SoAd_PQC_States[PeerId] = SOAD_PQC_STATE_FAILED;
        return E_NOT_OK;
    }

    result = SoAd_PQC_SendControlMessage(PeerId, SOAD_PQC_CTRL_TYPE_PUBKEY, publicKey, (uint16)actualSize);
    if (result != E_OK)
    {
        SoAd_PQC_States[PeerId] = SOAD_PQC_STATE_FAILED;
        return E_NOT_OK;
    }

    return E_OK;
}

/**
 * @brief Perform ML-KEM key exchange as RESPONDER (Bob)
 */
static Std_ReturnType SoAd_PQC_KeyExchange_Responder(Csm_PeerIdType PeerId)
{
    SoAd_PQC_States[PeerId] = SOAD_PQC_STATE_KEY_EXCHANGE_INITIATED;
    return E_OK;
}

/**
 * @brief Perform ML-KEM key exchange with Ethernet peer
 */
Std_ReturnType SoAd_PQC_KeyExchange(
    Csm_PeerIdType PeerId,
    boolean IsInitiator)
{
    if (SoAd_PQC_Initialized == FALSE)
    {
        (void)printf("ERROR: SoAd PQC not initialized\n");
        return E_NOT_OK;
    }

    if (PeerId >= SOAD_PQC_MAX_PEERS)
    {
        (void)printf("ERROR: Invalid peer ID %u\n", PeerId);
        return E_NOT_OK;
    }

    if (IsInitiator == TRUE)
    {
        return SoAd_PQC_KeyExchange_Initiator(PeerId);
    }
    else
    {
        return SoAd_PQC_KeyExchange_Responder(PeerId);
    }
}

boolean SoAd_PQC_HandleControlMessage(const uint8* BufPtr, uint16 Length)
{
    Csm_PeerIdType peerId;
    uint16 payloadLength;
    uint8 messageType;
    const uint8* payloadPtr;
    Std_ReturnType result;

    if ((SoAd_PQC_Initialized == FALSE) || (BufPtr == NULL) || (Length < SOAD_PQC_CTRL_HEADER_LENGTH))
    {
        return FALSE;
    }

    if ((BufPtr[0] != SOAD_PQC_CTRL_MAGIC0) ||
        (BufPtr[1] != SOAD_PQC_CTRL_MAGIC1) ||
        (BufPtr[2] != SOAD_PQC_CTRL_VERSION))
    {
        return FALSE;
    }

    messageType = BufPtr[3];
    peerId = (Csm_PeerIdType)BufPtr[4];
    payloadLength = (uint16)BufPtr[5] | ((uint16)BufPtr[6] << 8U);
    payloadPtr = &BufPtr[SOAD_PQC_CTRL_HEADER_LENGTH];

    if ((peerId >= SOAD_PQC_MAX_PEERS) ||
        (payloadLength > SOAD_PQC_CTRL_MAX_PAYLOAD) ||
        ((uint16)(SOAD_PQC_CTRL_HEADER_LENGTH + payloadLength) != Length))
    {
        return FALSE;
    }

    if (messageType == SOAD_PQC_CTRL_TYPE_PUBKEY)
    {
        uint8 ciphertext[CSM_MLKEM_CIPHERTEXT_BYTES];
        uint32 ciphertextLength = CSM_MLKEM_CIPHERTEXT_BYTES;

        if ((SoAd_PQC_States[peerId] != SOAD_PQC_STATE_KEY_EXCHANGE_INITIATED) &&
            (SoAd_PQC_States[peerId] != SOAD_PQC_STATE_IDLE))
        {
            return TRUE;
        }

        result = Csm_KeyExchangeRespond(0U,
                                        peerId,
                                        payloadPtr,
                                        payloadLength,
                                        ciphertext,
                                        &ciphertextLength);
        if ((result != E_OK) ||
            (SoAd_PQC_SendControlMessage(peerId,
                                         SOAD_PQC_CTRL_TYPE_CIPHERTEXT,
                                         ciphertext,
                                         (uint16)ciphertextLength) != E_OK) ||
            (SoAd_PQC_DeriveSessionFromPeer(peerId) != E_OK))
        {
            SoAd_PQC_States[peerId] = SOAD_PQC_STATE_FAILED;
            return TRUE;
        }

        SoAd_PQC_States[peerId] = SOAD_PQC_STATE_SESSION_ESTABLISHED;
        SoAd_PQC_RekeyCycles[peerId] = 0U;
        return TRUE;
    }

    if (messageType == SOAD_PQC_CTRL_TYPE_CIPHERTEXT)
    {
        if (SoAd_PQC_States[peerId] != SOAD_PQC_STATE_KEY_EXCHANGE_INITIATED)
        {
            return TRUE;
        }

        result = Csm_KeyExchangeComplete(0U, peerId, payloadPtr, payloadLength);
        if ((result != E_OK) || (SoAd_PQC_DeriveSessionFromPeer(peerId) != E_OK))
        {
            SoAd_PQC_States[peerId] = SOAD_PQC_STATE_FAILED;
            return TRUE;
        }

        SoAd_PQC_States[peerId] = SOAD_PQC_STATE_SESSION_ESTABLISHED;
        SoAd_PQC_RekeyCycles[peerId] = 0U;
        return TRUE;
    }

    return FALSE;
}

/**
 * @brief Get current PQC session state for a peer
 */
SoAd_PQC_StateType SoAd_PQC_GetState(Csm_PeerIdType PeerId)
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
Std_ReturnType SoAd_PQC_ResetSession(Csm_PeerIdType PeerId)
{
    Std_ReturnType result;

    if (PeerId >= SOAD_PQC_MAX_PEERS)
    {
        return E_NOT_OK;
    }

    /* Reset key exchange session */
    result = Csm_KeyExchangeReset(PeerId);
    if (result != E_OK)
    {
        return E_NOT_OK;
    }

    /* Clear session keys */
    result = Csm_ClearSessionKeys(PeerId);
    if (result != E_OK)
    {
        return E_NOT_OK;
    }

    SoAd_PQC_States[PeerId] = SOAD_PQC_STATE_IDLE;
    SoAd_PQC_RekeyCycles[PeerId] = 0U;

    (void)printf("SoAd PQC session reset for peer %u\n", PeerId);

    return E_OK;
}
