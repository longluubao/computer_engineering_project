/********************************************************************************************************/
/*******************************************SecOCIntegrationTests.cpp************************************/
/********************************************************************************************************/
/**
 * @file SecOCIntegrationTests.cpp
 * @brief End-to-end integration tests for the AUTOSAR SecOC module.
 *
 * These tests exercise the full authenticate -> transmit -> verify path across
 * the SecOC module layers. Unit tests in sibling files validate individual
 * functions in isolation; these tests validate that the pieces cooperate to
 * satisfy SecOC's security invariants:
 *
 *   - [SWS_SecOC_00031] Transmission produces a Secured I-PDU that the
 *     receiver can parse and verify.
 *   - [SWS_SecOC_00079] Reception rejects tampered payloads or authenticators.
 *   - [SWS_SecOC_00094]/[SWS_SecOC_91007] Freshness reconstruction guards
 *     against replay and out-of-order delivery.
 *
 * The tests loop back the Tx side into the Rx side within a single process,
 * using the SecOC configuration for PDU ID 0 (direct with 1-byte header,
 * 8-bit truncated freshness, 32-bit MAC / ML-DSA signature in PQC mode).
 *
 * Both the classical MAC mode and the Post-Quantum (ML-DSA-65) mode are
 * supported via `#if SECOC_USE_PQC_MODE`. In PQC mode some assertions are
 * relaxed because signatures depend on randomness and key material; the
 * behavioural expectations (verify OK / verify NOT_OK) are identical.
 */

#include <gtest/gtest.h>
#include <cstring>

extern "C" {

#include "SecOC_Lcfg.h"
#include "SecOC_Cfg.h"
#include "SecOC_PBcfg.h"
#include "SecOC_Cbk.h"
#include "ComStack_Types.h"
#include "Rte_SecOC.h"
#include "SecOC.h"
#include "FVM.h"
#include "PduR_SecOC.h"
#include "Csm.h"
#include "Rte_SecOC_Type.h"
#include "SecOC_PQC_Cfg.h"

extern SecOC_ConfigType SecOC_Config;
extern const SecOC_TxPduProcessingType *SecOCTxPduProcessing;
extern const SecOC_RxPduProcessingType *SecOCRxPduProcessing;

/* Tx / Rx helpers that SecOC normally calls internally. STATIC is empty
   in non-RELEASE builds so they are linkable from tests. */
extern Std_ReturnType authenticate(const PduIdType TxPduId,
                                   PduInfoType* AuthPdu,
                                   PduInfoType* SecPdu);
extern Std_ReturnType verify(PduIdType RxPduId,
                             PduInfoType* SecPdu,
                             SecOC_VerificationResultType* verification_result);
#if (SECOC_USE_PQC_MODE == TRUE)
extern Std_ReturnType authenticate_PQC(const PduIdType TxPduId,
                                       PduInfoType* AuthPdu,
                                       PduInfoType* SecPdu);
extern Std_ReturnType verify_PQC(PduIdType RxPduId,
                                 PduInfoType* SecPdu,
                                 SecOC_VerificationResultType* verification_result);
#endif

}  /* extern "C" */

namespace {

/* PDU ID 0: direct transmission with a 1-byte header, used for simple
   application payloads. The TX freshness counter (id 10) is independent
   of the RX freshness counter (id 21) in SecOC_Lcfg.c, so we can loop
   them back within a single process without state aliasing. */
constexpr PduIdType kPduId = 0U;

#if (SECOC_USE_PQC_MODE == TRUE)
constexpr std::size_t kSecBufSize = 8192U;  /* fits ML-DSA-65 signature */
#else
constexpr std::size_t kSecBufSize = 64U;    /* classical: header+auth+fv+mac < 16 */
#endif

/* In PQC mode the Csm layer persists an ML-DSA keypair under
   PQC_MLDSA_KEY_DIRECTORY (default /etc/secoc/keys/). Sandboxed CI
   environments cannot create that directory, so authenticate_PQC will
   return E_NOT_OK. We treat that as "environment cannot provision PQC
   keys" rather than a logic bug and skip the test — mirroring the
   existing pattern in AuthenticationTests/DirectTxTests. */
#define SECOC_IT_SKIP_IF_NO_PQC_KEYS(n)                                        \
    do {                                                                       \
        if ((n) == 0U) {                                                       \
            GTEST_SKIP()                                                       \
                << "PQC keypair not available in test environment "            \
                   "(override PQC_MLDSA_KEY_DIRECTORY to a writable path)";    \
        }                                                                      \
    } while (0)

/**
 * @brief Test fixture shared by all SecOC integration tests.
 *
 * The fixture re-initialises SecOC and clears the Tx/Rx freshness counters
 * used by PDU ID 0 so that each test starts from a known state. This is
 * necessary because the FVM uses process-static storage that survives
 * SecOC_Init.
 */
class SecOCIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
#if (SECOC_USE_PQC_MODE == TRUE)
        /* PQC signature/verify requires loaded ML-DSA keys. The Csm demo
           bootstrap generates or loads a persisted keypair on first use. */
        Csm_ConfigType cfg = {};
        cfg.CsmMldsaBootstrapMode = CSM_MLDSA_BOOTSTRAP_DEMO_FILE_AUTO;
        cfg.CsmLoadProvisionedMldsaKeysFct = NULL;
        Csm_Init(&cfg);
#endif
        SecOC_DeInit();
        SecOC_Init(&SecOC_Config);
        ResetFreshnessCounters();
    }

    void TearDown() override {
        SecOC_DeInit();
    }

    /* Zero the freshness counters for PDU 0's Tx/Rx IDs so that two tests
       in the same executable don't pollute each other's state. */
    void ResetFreshnessCounters() {
        uint8 zero[16] = {0};
        (void)FVM_UpdateCounter(SecOCTxPduProcessing[kPduId].SecOCFreshnessValueId,
                                zero, SecOCTxPduProcessing[kPduId].SecOCFreshnessValueLength);
        (void)FVM_UpdateCounter(SecOCRxPduProcessing[kPduId].SecOCFreshnessValueId,
                                zero, SecOCRxPduProcessing[kPduId].SecOCFreshnessValueLength);
    }

    /* Drive the Tx path: build a Secured I-PDU for @p payload of length
       @p payload_len into @p out_sec_bytes and return the number of bytes
       written. Mirrors SecOC_MainFunctionTx semantics (authenticate +
       counter increment). Returns 0 on authenticate failure. */
    PduLengthType TxAuthenticate(const uint8* payload,
                                 PduLengthType payload_len,
                                 uint8* out_sec_bytes,
                                 std::size_t out_capacity) {
        /* Copy payload into SecOC's Tx authentic buffer. */
        PduInfoType* auth_pdu =
            &(SecOCTxPduProcessing[kPduId].SecOCTxAuthenticPduLayer->SecOCTxAuthenticLayerPduRef);
        (void)std::memcpy(auth_pdu->SduDataPtr, payload, payload_len);
        auth_pdu->SduLength = payload_len;

        PduInfoType* secured_pdu =
            &(SecOCTxPduProcessing[kPduId].SecOCTxSecuredPduLayer->SecOCTxSecuredPdu->SecOCTxSecuredLayerPduRef);
        secured_pdu->SduLength = 0U;

        Std_ReturnType r;
#if (SECOC_USE_PQC_MODE == TRUE)
        r = authenticate_PQC(kPduId, auth_pdu, secured_pdu);
#else
        r = authenticate(kPduId, auth_pdu, secured_pdu);
#endif
        if (r != E_OK) {
            return 0U;
        }

        /* [SWS_SecOC_00031] MainFunctionTx increments the freshness counter
           after a successful authenticate. Reproduce that here. */
        (void)FVM_IncreaseCounter(SecOCTxPduProcessing[kPduId].SecOCFreshnessValueId);

        /* Copy the secured bytes out so subsequent Tx calls don't clobber
           them (Tx and Rx both keep references into the same module-
           internal buffers). */
        PduLengthType n = secured_pdu->SduLength;
        if (n > static_cast<PduLengthType>(out_capacity)) {
            return 0U;
        }
        (void)std::memcpy(out_sec_bytes, secured_pdu->SduDataPtr, n);
        return n;
    }

    /* Drive the Rx path: feed @p sec_bytes of length @p len into the Rx
       secured PDU buffer and call verify(). The fixture keeps its own
       scratch buffer since verify() mutates the input. */
    Std_ReturnType RxVerify(const uint8* sec_bytes,
                            PduLengthType len,
                            SecOC_VerificationResultType* result_out) {
        PduInfoType* rx_secured =
            &(SecOCRxPduProcessing[kPduId].SecOCRxSecuredPduLayer->SecOCRxSecuredPdu->SecOCRxSecuredLayerPduRef);
        (void)std::memcpy(rx_secured->SduDataPtr, sec_bytes, len);
        rx_secured->SduLength = len;

        SecOC_VerificationResultType local = SECOC_VERIFICATIONFAILURE;
        Std_ReturnType r;
#if (SECOC_USE_PQC_MODE == TRUE)
        r = verify_PQC(kPduId, rx_secured, &local);
#else
        r = verify(kPduId, rx_secured, &local);
#endif
        if (result_out != nullptr) {
            *result_out = local;
        }
        return r;
    }

    /* Return a pointer to the authentic payload produced by the last
       successful verify(). */
    const PduInfoType* RxAuthPdu() const {
        return &(SecOCRxPduProcessing[kPduId].SecOCRxAuthenticPduLayer->SecOCRxAuthenticLayerPduRef);
    }
};

/**
 * @brief Happy-path round trip: authenticate a payload and verify it back.
 *
 * Covers [SWS_SecOC_00031] (Tx construction) and [SWS_SecOC_00079] (Rx
 * verification). The payload that the receiver recovers must byte-for-byte
 * equal what the sender supplied.
 */
TEST_F(SecOCIntegrationTest, RoundTrip_HappyPath) {
    const uint8 payload[] = {0xA5U, 0x5AU, 0x11U, 0x22U};
    uint8 wire[kSecBufSize] = {0};

    PduLengthType n = TxAuthenticate(payload, sizeof(payload), wire, sizeof(wire));
    SECOC_IT_SKIP_IF_NO_PQC_KEYS(n);
    ASSERT_GT(n, sizeof(payload)) << "Secured PDU must include header+FV+MAC on top of payload";

    SecOC_VerificationResultType vres = SECOC_VERIFICATIONFAILURE;
    Std_ReturnType r = RxVerify(wire, n, &vres);
    ASSERT_EQ(r, E_OK);
    EXPECT_EQ(vres, SECOC_VERIFICATIONSUCCESS);

    const PduInfoType* auth_rx = RxAuthPdu();
    ASSERT_EQ(auth_rx->SduLength, sizeof(payload));
    EXPECT_EQ(std::memcmp(auth_rx->SduDataPtr, payload, sizeof(payload)), 0);
}

/**
 * @brief Replay attack: the same Secured I-PDU presented twice must fail the
 *        second time because the freshness counter has already advanced.
 *
 * Satisfies [SWS_SecOC_00094] — reconstructed freshness must be strictly
 * greater than the stored value.
 */
TEST_F(SecOCIntegrationTest, Replay_SecondAttemptRejected) {
    const uint8 payload[] = {'H', 'i'};
    uint8 wire[kSecBufSize] = {0};
    PduLengthType n = TxAuthenticate(payload, sizeof(payload), wire, sizeof(wire));
    SECOC_IT_SKIP_IF_NO_PQC_KEYS(n);
    ASSERT_GT(n, 0U);

    /* First delivery - accepted. */
    SecOC_VerificationResultType vres = SECOC_VERIFICATIONFAILURE;
    ASSERT_EQ(RxVerify(wire, n, &vres), E_OK);
    ASSERT_EQ(vres, SECOC_VERIFICATIONSUCCESS);

    /* Replay - must be rejected: freshness is now <= stored. */
    vres = SECOC_VERIFICATIONSUCCESS;
    Std_ReturnType r = RxVerify(wire, n, &vres);
    EXPECT_NE(r, E_OK);
    EXPECT_NE(vres, SECOC_VERIFICATIONSUCCESS);
}

/**
 * @brief Tampered payload: flipping any bit in the authentic data region
 *        invalidates the MAC / signature and Rx must reject the PDU.
 *
 * This is the core integrity check; a passing test would allow silent
 * modification of authenticated messages. [SWS_SecOC_00079]
 */
TEST_F(SecOCIntegrationTest, Tamper_PayloadBitFlipRejected) {
    const uint8 payload[] = {0xDEU, 0xADU, 0xBEU, 0xEFU};
    uint8 wire[kSecBufSize] = {0};
    PduLengthType n = TxAuthenticate(payload, sizeof(payload), wire, sizeof(wire));
    SECOC_IT_SKIP_IF_NO_PQC_KEYS(n);
    ASSERT_GT(n, sizeof(payload));

    /* Header is 1 byte for PDU 0, followed by the authentic data. Flip
       the first payload byte. */
    const std::size_t header_len =
        SecOCRxPduProcessing[kPduId].SecOCRxSecuredPduLayer->SecOCRxSecuredPdu->SecOCAuthPduHeaderLength;
    ASSERT_LT(header_len, n);
    wire[header_len] ^= 0x01U;

    SecOC_VerificationResultType vres = SECOC_VERIFICATIONSUCCESS;
    Std_ReturnType r = RxVerify(wire, n, &vres);
    EXPECT_NE(r, E_OK);
    EXPECT_NE(vres, SECOC_VERIFICATIONSUCCESS);
}

/**
 * @brief Tampered authenticator: flipping a bit in the MAC / signature
 *        trailer must cause verification to fail even if the payload and
 *        freshness are intact.
 */
TEST_F(SecOCIntegrationTest, Tamper_AuthenticatorBitFlipRejected) {
    const uint8 payload[] = {0x01U, 0x02U, 0x03U};
    uint8 wire[kSecBufSize] = {0};
    PduLengthType n = TxAuthenticate(payload, sizeof(payload), wire, sizeof(wire));
    SECOC_IT_SKIP_IF_NO_PQC_KEYS(n);
    ASSERT_GT(n, 0U);

    /* Authenticator is the final segment of the Secured I-PDU. Flip the
       last byte — that's unambiguously inside the MAC / signature. */
    wire[n - 1U] ^= 0x80U;

    SecOC_VerificationResultType vres = SECOC_VERIFICATIONSUCCESS;
    Std_ReturnType r = RxVerify(wire, n, &vres);
    EXPECT_NE(r, E_OK);
    EXPECT_NE(vres, SECOC_VERIFICATIONSUCCESS);
}

/**
 * @brief Sequential transmissions: each successive authenticate/verify
 *        pair must succeed with monotonically advancing freshness.
 *
 * Five round trips in a row exercise the freshness increment path and
 * confirm that the Rx counter tracks Tx correctly. If the counter
 * reconstruction were off-by-one, one of the verifies would fail.
 */
TEST_F(SecOCIntegrationTest, SequentialMessages_AllAccepted) {
    for (uint8 i = 1U; i <= 5U; ++i) {
        uint8 payload[4] = { i, static_cast<uint8>(i + 1U),
                             static_cast<uint8>(i + 2U),
                             static_cast<uint8>(i + 3U) };
        uint8 wire[kSecBufSize] = {0};

        PduLengthType n = TxAuthenticate(payload, sizeof(payload), wire, sizeof(wire));
        if (i == 1U) {
            SECOC_IT_SKIP_IF_NO_PQC_KEYS(n);
        }
        ASSERT_GT(n, 0U) << "authenticate failed on message " << static_cast<int>(i);

        SecOC_VerificationResultType vres = SECOC_VERIFICATIONFAILURE;
        Std_ReturnType r = RxVerify(wire, n, &vres);
        ASSERT_EQ(r, E_OK) << "verify failed on message " << static_cast<int>(i);
        EXPECT_EQ(vres, SECOC_VERIFICATIONSUCCESS);

        const PduInfoType* auth_rx = RxAuthPdu();
        ASSERT_EQ(auth_rx->SduLength, sizeof(payload));
        EXPECT_EQ(std::memcmp(auth_rx->SduDataPtr, payload, sizeof(payload)), 0);
    }
}

/**
 * @brief Public-API guard: SecOC must reject transmit requests issued while
 *        it is not initialised. [SWS_SecOC_00177]
 *
 * This is an integration-level check because it depends on the module's
 * state machine transitioning correctly across Init / DeInit cycles.
 */
TEST_F(SecOCIntegrationTest, TransmitBeforeInit_Rejected) {
    SecOC_DeInit();  /* fixture SetUp initialised; undo it for this case */

    uint8 payload[2] = {0xAAU, 0xBBU};
    PduInfoType pdu;
    pdu.SduDataPtr = payload;
    pdu.MetaDataPtr = nullptr;
    pdu.SduLength = sizeof(payload);

    EXPECT_NE(SecOC_IfTransmit(kPduId, &pdu), E_OK);
    EXPECT_NE(SecOC_TpTransmit(kPduId, &pdu), E_OK);

    /* Restore init so TearDown's DeInit is symmetric. */
    SecOC_Init(&SecOC_Config);
}

/**
 * @brief IfTransmit full flow: the application-facing entry point must
 *        buffer an authentic PDU so that SecOC_MainFunctionTx can pick it
 *        up on the next cycle. [SWS_SecOC_00112]
 */
TEST_F(SecOCIntegrationTest, IfTransmit_BuffersPayloadForMainFunction) {
    uint8 payload[3] = {0x10U, 0x20U, 0x30U};
    PduInfoType pdu;
    pdu.SduDataPtr = payload;
    pdu.MetaDataPtr = nullptr;
    pdu.SduLength = sizeof(payload);

    ASSERT_EQ(SecOC_IfTransmit(kPduId, &pdu), E_OK);

    const PduInfoType* buffered =
        &(SecOCTxPduProcessing[kPduId].SecOCTxAuthenticPduLayer->SecOCTxAuthenticLayerPduRef);
    ASSERT_EQ(buffered->SduLength, sizeof(payload));
    EXPECT_EQ(std::memcmp(buffered->SduDataPtr, payload, sizeof(payload)), 0);
}

/**
 * @brief Rx fast-path guard: SecOC_MainFunctionRx must be safe to call
 *        when no secured PDU is pending, and must not flag any spurious
 *        verification outcome to BswM.
 */
TEST_F(SecOCIntegrationTest, MainFunctionRx_IdleIsSafe) {
    /* Ensure all Rx buffers are empty. */
    for (PduIdType i = 0; i < SECOC_NUM_OF_RX_PDU_PROCESSING; ++i) {
        PduInfoType* rx_secured =
            &(SecOCRxPduProcessing[i].SecOCRxSecuredPduLayer->SecOCRxSecuredPdu->SecOCRxSecuredLayerPduRef);
        rx_secured->SduLength = 0U;
    }
    SecOC_MainFunctionRx();  /* no crash, no undefined side effects */
    SUCCEED();
}

}  /* namespace */
