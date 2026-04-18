#include "sim_ecu.h"
#include "sim_clock.h"
#include "sim_logger.h"
#include "sim_metrics.h"

#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "Std_Types.h"
#include "PQC.h"
#include "PQC_KeyDerivation.h"

#include <openssl/hmac.h>
#include <openssl/sha.h>

/*
 * The ISE virtual ECU uses the real PQC primitives (ML-DSA, ML-KEM,
 * HKDF) exposed by the Autosar_SecOC project. Upper BSW layers (Com,
 * PduR, SecOC, CanIf, SoAd) are exercised by re-building the library
 * when the ISE is compiled with ISE_USE_SECOC_LIB=ON; otherwise this
 * file implements a faithful replica of the SecOC secured-PDU format
 * so scenarios can still run stand-alone for thesis evidence.
 *
 * The replica is kept byte-compatible with the real SecOC secured PDU:
 *
 *   ┌────────┬────────────┬────────────────────────┬──────────────┐
 *   │ Header │ Freshness  │ Authentic PDU payload  │ Authenticator│
 *   │ 2 B    │ 8 B (PQC)  │ N bytes                │ 4/16/3309 B  │
 *   └────────┴────────────┴────────────────────────┴──────────────┘
 *
 * Header byte 0:
 *   bits 7..6 : protection mode (0=NONE, 1=HMAC, 2=PQC, 3=HYBRID)
 *   bits 5..0 : reserved
 * Header byte 1:
 *   signal id (LSB — IDs up to 255 for the demo catalogue)
 */

#define ECU_HEADER_BYTES      2U
#define ECU_FRESHNESS_BYTES   8U
#define ECU_HMAC_BYTES       16U
#define ECU_MAX_PDU       8192U

struct SimEcu {
    SimEcuCfg           cfg;

    PQC_MLDSA_KeyPairType  mldsa;   /* ML-DSA keypair (signing on Tx, verify on Rx) */
    bool                   has_mldsa;

    uint8_t                session_key[32]; /* HKDF-derived (PQC) or static (HMAC) */
    uint64_t               freshness_tx;
    uint64_t               freshness_rx;     /* last accepted value */

    _Atomic bool           running;
    pthread_t              thread;

    /* Scenario-side callback – invoked on successful rx verify. */
    void (*on_signal)(SimEcu *, uint16_t sig_id, const uint8_t *buf, uint32_t len);
};

/*======================================================================*/
/*  Small helpers                                                        */
/*======================================================================*/

static void u64_to_be(uint64_t v, uint8_t *out)
{
    for (int i = 7; i >= 0; --i) { out[i] = (uint8_t)(v & 0xFFU); v >>= 8; }
}

static uint64_t be_to_u64(const uint8_t *in)
{
    uint64_t v = 0;
    for (int i = 0; i < 8; ++i) { v = (v << 8) | in[i]; }
    return v;
}

/* HMAC-SHA256 truncated to 16 bytes. */
static void hmac_sha256_16(const uint8_t *key, size_t key_len,
                           const uint8_t *msg, size_t msg_len,
                           uint8_t out16[16])
{
    unsigned int len = 0;
    uint8_t full[32];
    HMAC(EVP_sha256(), key, (int)key_len, msg, msg_len, full, &len);
    memcpy(out16, full, 16);
}

/*======================================================================*/
/*  PDU build / parse                                                    */
/*======================================================================*/

static int build_secured_pdu(SimEcu *ecu, const SimSignalDef *sig,
                             const uint8_t *payload, uint32_t payload_len,
                             uint8_t *out, uint32_t out_max,
                             uint32_t *out_len,
                             uint64_t *t_auth_ns,
                             uint32_t *auth_bytes)
{
    if (payload_len + ECU_HEADER_BYTES + ECU_FRESHNESS_BYTES +
        PQC_MLDSA_SIGNATURE_BYTES > out_max) return -1;

    ecu->freshness_tx++;

    /* Header */
    out[0] = (uint8_t)((ecu->cfg.protection & 0x3) << 6);
    out[1] = (uint8_t)(sig->id & 0xFFU);

    /* Freshness (64-bit BE) */
    u64_to_be(ecu->freshness_tx, &out[2]);

    /* Authentic payload */
    memcpy(&out[ECU_HEADER_BYTES + ECU_FRESHNESS_BYTES], payload, payload_len);

    uint32_t secured_prefix = ECU_HEADER_BYTES + ECU_FRESHNESS_BYTES + payload_len;
    uint8_t *auth_dst = &out[secured_prefix];
    uint32_t auth_len = 0;

    uint64_t t0 = sim_now_ns();

    switch (ecu->cfg.protection) {
    case SIM_PROT_HMAC: {
        hmac_sha256_16(ecu->session_key, sizeof(ecu->session_key),
                       out, secured_prefix, auth_dst);
        auth_len = ECU_HMAC_BYTES;
        break;
    }
    case SIM_PROT_PQC: {
        uint32 sig_len = PQC_MLDSA_SIGNATURE_BYTES;
        Std_ReturnType r = PQC_MLDSA_Sign(out, secured_prefix,
                                          ecu->mldsa.SecretKey,
                                          auth_dst, &sig_len);
        if (r != PQC_E_OK) return -2;
        auth_len = (uint32_t)sig_len;
        break;
    }
    case SIM_PROT_HYBRID: {
        /* HMAC first for fast reject, then ML-DSA. */
        hmac_sha256_16(ecu->session_key, sizeof(ecu->session_key),
                       out, secured_prefix, auth_dst);
        uint32 sig_len = PQC_MLDSA_SIGNATURE_BYTES;
        Std_ReturnType r = PQC_MLDSA_Sign(out, secured_prefix + ECU_HMAC_BYTES,
                                          ecu->mldsa.SecretKey,
                                          auth_dst + ECU_HMAC_BYTES, &sig_len);
        if (r != PQC_E_OK) return -2;
        auth_len = ECU_HMAC_BYTES + (uint32_t)sig_len;
        break;
    }
    case SIM_PROT_NONE:
    default:
        auth_len = 0;
        break;
    }

    if (t_auth_ns)  *t_auth_ns  = sim_now_ns() - t0;
    if (auth_bytes) *auth_bytes = auth_len;

    *out_len = secured_prefix + auth_len;
    return 0;
}

typedef struct {
    uint16_t signal_id;
    uint64_t freshness;
    uint32_t payload_off;
    uint32_t payload_len;
    uint32_t auth_off;
    uint32_t auth_len;
    uint8_t  protection;
} ParsedPdu;

static int parse_header(const uint8_t *buf, uint32_t len, ParsedPdu *p,
                        SimProtectionMode mode, uint32_t payload_len)
{
    if (len < ECU_HEADER_BYTES + ECU_FRESHNESS_BYTES) return -1;
    p->protection = (buf[0] >> 6) & 0x3U;
    p->signal_id  = buf[1];
    p->freshness  = be_to_u64(&buf[ECU_HEADER_BYTES]);
    p->payload_off = ECU_HEADER_BYTES + ECU_FRESHNESS_BYTES;
    p->payload_len = payload_len;
    p->auth_off   = p->payload_off + payload_len;
    switch (mode) {
        case SIM_PROT_HMAC: p->auth_len = ECU_HMAC_BYTES; break;
        case SIM_PROT_PQC:  p->auth_len = PQC_MLDSA_SIGNATURE_BYTES; break;
        case SIM_PROT_HYBRID:
            p->auth_len = ECU_HMAC_BYTES + PQC_MLDSA_SIGNATURE_BYTES; break;
        default:            p->auth_len = 0; break;
    }
    if (p->auth_off + p->auth_len > len) return -2;
    return 0;
}

static int verify_secured_pdu(SimEcu *ecu, const uint8_t *buf, uint32_t len,
                              uint32_t payload_len, uint64_t *t_verify_ns)
{
    ParsedPdu p;
    if (parse_header(buf, len, &p, ecu->cfg.protection, payload_len) != 0) {
        return -1;
    }
    /* Freshness window: reject if not strictly greater than last accepted. */
    if (p.freshness <= ecu->freshness_rx) {
        return -2; /* replay */
    }

    uint64_t t0 = sim_now_ns();
    int rc = 0;

    switch (ecu->cfg.protection) {
    case SIM_PROT_HMAC: {
        uint8_t mac[16];
        hmac_sha256_16(ecu->session_key, sizeof(ecu->session_key),
                       buf, p.auth_off, mac);
        if (memcmp(mac, &buf[p.auth_off], 16) != 0) rc = -3;
        break;
    }
    case SIM_PROT_PQC: {
        Std_ReturnType r = PQC_MLDSA_Verify(buf, p.auth_off,
                                            &buf[p.auth_off], p.auth_len,
                                            ecu->mldsa.PublicKey);
        if (r != PQC_E_OK) rc = -3;
        break;
    }
    case SIM_PROT_HYBRID: {
        uint8_t mac[16];
        hmac_sha256_16(ecu->session_key, sizeof(ecu->session_key),
                       buf, p.auth_off, mac);
        if (memcmp(mac, &buf[p.auth_off], 16) != 0) { rc = -3; break; }
        Std_ReturnType r = PQC_MLDSA_Verify(buf, p.auth_off + ECU_HMAC_BYTES,
                                            &buf[p.auth_off + ECU_HMAC_BYTES],
                                            p.auth_len - ECU_HMAC_BYTES,
                                            ecu->mldsa.PublicKey);
        if (r != PQC_E_OK) rc = -3;
        break;
    }
    default: break;
    }

    if (t_verify_ns) *t_verify_ns = sim_now_ns() - t0;
    if (rc == 0) ecu->freshness_rx = p.freshness;
    return rc;
}

/*======================================================================*/
/*  Lifecycle                                                            */
/*======================================================================*/

SimEcu *sim_ecu_create(const SimEcuCfg *cfg)
{
    SimEcu *e = (SimEcu *)calloc(1, sizeof(*e));
    if (!e) return NULL;
    e->cfg = *cfg;
    /* Deterministic demo session key: derived from seed so both peers
     * can reproduce it without an actual ML-KEM handshake for the
     * HMAC-only scenarios. */
    for (size_t i = 0; i < sizeof(e->session_key); ++i) {
        e->session_key[i] = (uint8_t)(((cfg->seed >> (i % 56)) ^ 0xA5U) & 0xFFU);
    }
    return e;
}

bool sim_ecu_init_stack(SimEcu *ecu)
{
    if (!ecu) return false;
    if (ecu->cfg.protection == SIM_PROT_PQC ||
        ecu->cfg.protection == SIM_PROT_HYBRID) {
        Std_ReturnType r = PQC_MLDSA_KeyGen(&ecu->mldsa);
        if (r != PQC_E_OK) {
            sim_log(SIM_LOG_ERROR, "ecu[%s] MLDSA keygen failed", ecu->cfg.name);
            return false;
        }
        ecu->has_mldsa = true;
    }
    sim_log(SIM_LOG_INFO, "ecu[%s] stack initialised (protection=%d)",
            ecu->cfg.name, (int)ecu->cfg.protection);
    return true;
}

bool sim_ecu_start(SimEcu *ecu) { (void)ecu; return true; }
void sim_ecu_stop(SimEcu *ecu)  { (void)ecu; }

void sim_ecu_destroy(SimEcu *ecu)
{
    if (!ecu) return;
    free(ecu);
}

void sim_ecu_tick(SimEcu *ecu) { (void)ecu; }

/*======================================================================*/
/*  Public API – send / recv                                             */
/*======================================================================*/

bool sim_ecu_send_signal(SimEcu *ecu, const SimSignalDef *sig,
                         const uint8_t *payload, uint32_t len)
{
    if (!ecu || !sig || !payload) return false;

    uint8_t pdu[ECU_MAX_PDU];
    uint32_t pdu_len = 0;
    uint64_t t_auth_ns = 0;
    uint32_t auth_bytes = 0;
    uint64_t t_app_tx = sim_now_ns();

    if (build_secured_pdu(ecu, sig, payload, len, pdu, sizeof(pdu),
                          &pdu_len, &t_auth_ns, &auth_bytes) != 0) {
        sim_log(SIM_LOG_WARN, "ecu[%s] build_secured_pdu failed", ecu->cfg.name);
        return false;
    }

    /* Tag lower 16 bits = signal id; upper 16 bits = flags (0 here).
     * The attacker relies on the tag to recognise target traffic. */
    uint32_t tag = sig->id;

    /*
     * Fragmentation for CAN / CAN-FD / FlexRay. Simulates CAN-TP.
     * Each fragment is a full bus-MTU frame; we account the per-frame
     * bus transit inside sim_bus.
     */
    uint32_t mtu = ecu->cfg.primary_bus ? 1500U : 1500U;
    SimBusStats tmp; (void)tmp;
    extern uint32_t sim_bus_mtu(const SimBus *); /* not provided – inline below */

    SimBusKind kind = sig->preferred_bus;
    switch (kind) {
        case SIM_BUS_CAN_20:   mtu =   8U; break;
        case SIM_BUS_CAN_FD:   mtu =  64U; break;
        case SIM_BUS_FLEXRAY:  mtu = 254U; break;
        case SIM_BUS_ETH_100:  mtu = 1500U; break;
        case SIM_BUS_ETH_1000: mtu = 1500U; break;
    }

    /*
     * Split the TX-lower-stack timing into:
     *   - t_bus      : time spent inside sim_bus_tx (physical bus transit)
     *   - t_cantp    : CAN-TP fragmentation/book-keeping overhead only
     *                  (total loop time minus bus_tx time)
     * This matches the AUTOSAR layer boundaries used in
     * summary/layer_latency.csv and summary/flow_timeline.md.
     */
    uint64_t t_cantp_start = sim_now_ns();
    uint64_t t_bus_accum = 0;
    uint32_t fragments = 0;
    bool all_ok = true;
    for (uint32_t off = 0; off < pdu_len; off += mtu, ++fragments) {
        uint32_t n = (pdu_len - off > mtu) ? mtu : (pdu_len - off);
        uint64_t tb = sim_now_ns();
        if (!sim_bus_tx(ecu->cfg.primary_bus, sig->asil, &pdu[off], n, tag)) {
            all_ok = false;
            break;
        }
        t_bus_accum += sim_now_ns() - tb;
    }
    uint64_t t_total_lower = sim_now_ns() - t_cantp_start;
    uint64_t t_cantp = (t_total_lower > t_bus_accum)
                       ? (t_total_lower - t_bus_accum) : 0;
    uint64_t t_app_done = sim_now_ns();

    if (ecu->cfg.metrics) {
        sim_hist_add(&ecu->cfg.metrics->secoc_auth,   t_auth_ns);
        sim_hist_add(&ecu->cfg.metrics->cantp,        t_cantp);
        sim_hist_add(&ecu->cfg.metrics->bus_transit,  t_bus_accum);
        sim_hist_add(&ecu->cfg.metrics->pdu_bytes,    pdu_len);
        sim_hist_add(&ecu->cfg.metrics->fragments,    fragments);
        ecu->cfg.metrics->tx_bytes_total += pdu_len;
    }

    SimFrameRecord rec = {0};
    rec.seq             = ecu->freshness_tx;
    rec.tx_ns           = t_app_tx;
    rec.rx_ns           = t_app_done;  /* populated again on rx side */
    rec.secoc_auth_ns   = t_auth_ns;
    rec.cantp_ns        = t_cantp;
    rec.bus_ns          = t_bus_accum;
    rec.pdu_bytes       = pdu_len;
    rec.auth_bytes      = auth_bytes;
    rec.fragments       = fragments;
    rec.signal_id       = sig->id;
    rec.bus_id          = 0;     /* set by scenario */
    rec.asil            = (uint8_t)sig->asil;
    rec.deadline_class  = sig->deadline_class;
    rec.protected_mode  = (uint8_t)ecu->cfg.protection;
    rec.outcome         = all_ok ? 0 : 2;
    sim_log_frame(&rec);
    return all_ok;
}

bool sim_ecu_recv_signal(SimEcu *ecu, uint16_t *signal_id,
                         uint8_t *buf, uint32_t max_len, uint32_t *out_len,
                         uint64_t timeout_ns)
{
    if (!ecu || !signal_id || !buf || !out_len) return false;

    /*
     * Reassemble the PDU: read bus frames until the secured PDU is
     * complete. For the demo we assume the sender emits all fragments
     * back-to-back (true because sim_ecu_send_signal loops in-place).
     */
    uint8_t pdu[ECU_MAX_PDU];
    uint32_t got = 0;
    uint32_t tag = 0;
    uint32_t first_tag = 0;
    uint64_t t_start = sim_now_ns();

    while (got < sizeof(pdu)) {
        uint32_t avail = sizeof(pdu) - got;
        if (avail == 0) break;
        uint64_t rem = (sim_now_ns() - t_start < timeout_ns)
                       ? timeout_ns - (sim_now_ns() - t_start) : 0;
        if (rem == 0) break;
        uint32_t n = sim_bus_rx(ecu->cfg.primary_bus, &pdu[got], avail, rem, &tag);
        if (n == 0) break;
        if (got == 0) first_tag = tag;
        got += n;
        if (got >= ECU_HEADER_BYTES + ECU_FRESHNESS_BYTES) {
            /* break when we've read one MTU-aligned PDU */
            break;
        }
    }
    if (got == 0) return false;

    /* best-effort: keep pulling any remaining fragments without extra waits */
    while (got < sizeof(pdu)) {
        uint32_t n = sim_bus_rx(ecu->cfg.primary_bus, &pdu[got],
                                sizeof(pdu) - got, 500000ULL, &tag);
        if (n == 0) break;
        got += n;
    }

    const SimSignalDef *sig = sim_signal_find((uint16_t)(first_tag & 0xFFFFU));
    uint32_t payload_len = sig ? sig->payload_bytes : (uint32_t)(got -
        ECU_HEADER_BYTES - ECU_FRESHNESS_BYTES -
        ((ecu->cfg.protection == SIM_PROT_PQC) ? PQC_MLDSA_SIGNATURE_BYTES :
         (ecu->cfg.protection == SIM_PROT_HMAC) ? ECU_HMAC_BYTES : 0));

    uint64_t t_verify_ns = 0;
    int rc = verify_secured_pdu(ecu, pdu, got, payload_len, &t_verify_ns);

    if (ecu->cfg.metrics) {
        sim_hist_add(&ecu->cfg.metrics->secoc_verify, t_verify_ns);
        ecu->cfg.metrics->rx_bytes_total += got;
    }

    SimFrameRecord rec = {0};
    rec.rx_ns         = sim_now_ns();
    rec.secoc_verify_ns = t_verify_ns;
    rec.pdu_bytes     = got;
    rec.signal_id     = sig ? sig->id : 0;
    rec.asil          = sig ? (uint8_t)sig->asil : 0;
    rec.deadline_class = sig ? sig->deadline_class : 0;
    rec.protected_mode = (uint8_t)ecu->cfg.protection;

    if (rc == 0) {
        if (out_len) *out_len = payload_len;
        if (signal_id) *signal_id = rec.signal_id;
        memcpy(buf, &pdu[ECU_HEADER_BYTES + ECU_FRESHNESS_BYTES],
               (payload_len < max_len) ? payload_len : max_len);
        rec.outcome = 0;
        if (ecu->cfg.metrics) ecu->cfg.metrics->success_count++;
    } else if (rc == -2) {
        rec.outcome = 3; /* verify fail (replay) */
        if (ecu->cfg.metrics) ecu->cfg.metrics->verify_fail_count++;
    } else {
        rec.outcome = 3; /* verify fail */
        if (ecu->cfg.metrics) ecu->cfg.metrics->verify_fail_count++;
    }
    sim_log_frame(&rec);
    return rc == 0;
}

bool sim_ecu_pqc_handshake(SimEcu *tx, SimEcu *rx)
{
    if (!tx || !rx) return false;

    PQC_MLKEM_KeyPairType kp;
    uint64_t t0 = sim_now_ns();
    if (PQC_MLKEM_KeyGen(&kp) != PQC_E_OK) return false;

    PQC_MLKEM_SharedSecretType ss_enc;
    if (PQC_MLKEM_Encapsulate(kp.PublicKey, &ss_enc) != PQC_E_OK) return false;

    uint8_t ss_dec[PQC_MLKEM_SHARED_SECRET_BYTES];
    if (PQC_MLKEM_Decapsulate(ss_enc.Ciphertext, kp.SecretKey, ss_dec) !=
        PQC_E_OK) return false;

    if (memcmp(ss_enc.SharedSecret, ss_dec,
               PQC_MLKEM_SHARED_SECRET_BYTES) != 0) return false;

    /* Derive session keys from the shared secret (HKDF-SHA256). */
    PQC_SessionKeysType sk_tx, sk_rx;
    if (PQC_DeriveSessionKeys(ss_enc.SharedSecret, 0, &sk_tx) != PQC_E_OK) {
        return false;
    }
    if (PQC_DeriveSessionKeys(ss_dec, 0, &sk_rx) != PQC_E_OK) {
        return false;
    }

    /* Use the auth key as the HMAC session key for mixed-protection scenarios. */
    memcpy(tx->session_key, sk_tx.AuthenticationKey, sizeof(tx->session_key));
    memcpy(rx->session_key, sk_rx.AuthenticationKey, sizeof(rx->session_key));

    uint64_t dt = sim_now_ns() - t0;
    sim_log(SIM_LOG_INFO,
            "PQC handshake complete: keygen+encaps+decaps+HKDF = %.1f us",
            sim_ns_to_us(dt));
    return true;
}

bool sim_ecu_share_keys(SimEcu *src, SimEcu *dst)
{
    if (!src || !dst) return false;
    if (src->has_mldsa) {
        memcpy(dst->mldsa.PublicKey, src->mldsa.PublicKey,
               PQC_MLDSA_PUBLIC_KEY_BYTES);
        dst->has_mldsa = true;
    }
    memcpy(dst->session_key, src->session_key, sizeof(dst->session_key));
    return true;
}
