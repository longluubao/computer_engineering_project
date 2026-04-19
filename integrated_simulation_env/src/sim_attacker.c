#include "sim_attacker.h"
#include "sim_logger.h"
#include "sim_clock.h"

#include <stdatomic.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#define ATK_REPLAY_CACHE 16
#define ATK_MAX_FRAME    8192

struct SimAttacker {
    SimAttackCfg cfg;
    SimBus      *attached;

    pthread_mutex_t mtx;

    /* replay cache */
    uint8_t  replay_buf[ATK_REPLAY_CACHE][ATK_MAX_FRAME];
    uint32_t replay_len[ATK_REPLAY_CACHE];
    uint32_t replay_tag[ATK_REPLAY_CACHE];
    uint32_t replay_slot;
    uint64_t last_replay_ns;

    /* flood scheduler */
    uint64_t next_flood_ns;

    /* Tracks the last frame timestamp so we can distinguish the first
     * fragment of a new PDU from continuation fragments. Replay must
     * operate at PDU granularity, not bus-frame granularity. The ECU
     * emits all fragments back-to-back inside one send_signal call, so
     * a short idle gap reliably marks a PDU boundary. */
    uint64_t last_hook_ns;

    _Atomic uint64_t injected;
    _Atomic uint64_t detected;
    _Atomic uint64_t delivered;

    uint64_t rng_state;
};

static uint64_t rng_next(uint64_t *s)
{
    uint64_t x = *s;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    *s = x;
    return x;
}

const char *sim_attack_name(SimAttackKind k)
{
    switch (k) {
        case SIM_ATK_REPLAY:             return "replay";
        case SIM_ATK_TAMPER_PAYLOAD:     return "tamper_payload";
        case SIM_ATK_TAMPER_AUTH:        return "tamper_auth";
        case SIM_ATK_FRESHNESS_ROLLBACK: return "freshness_rollback";
        case SIM_ATK_MITM_KEY_CONFUSE:   return "mitm_key_confuse";
        case SIM_ATK_DOS_FLOOD:          return "dos_flood";
        case SIM_ATK_SIG_FUZZ:           return "sig_fuzz";
        case SIM_ATK_DOWNGRADE_HMAC:     return "downgrade_hmac";
        case SIM_ATK_HARVEST_NOW:        return "harvest_now";
        case SIM_ATK_TIMING_PROBE:       return "timing_probe";
        default:                         return "none";
    }
}

static SimBusHookAction attacker_hook(uint8_t bus_id, uint8_t *data,
                                       uint32_t *len, uint32_t *tag,
                                       void *user)
{
    SimAttacker *atk = (SimAttacker *)user;
    pthread_mutex_lock(&atk->mtx);

    SimBusHookAction act = SIM_ATTACK_PASS;
    SimAttackEvent   evt = {0};
    evt.sim_ns   = sim_now_ns();
    evt.bus_id   = bus_id;
    evt.attack_kind = atk->cfg.kind;
    evt.signal_id = (uint16_t)(*tag & 0xFFFFU);

    /* PDU boundary heuristic: fragments of the same PDU are emitted
     * back-to-back, so any gap > 100 us indicates a new PDU. */
    uint64_t now_ns = sim_now_ns();
    bool is_first_fragment = (atk->last_hook_ns == 0) ||
                             (now_ns - atk->last_hook_ns > 100000ULL);
    atk->last_hook_ns = now_ns;

    switch (atk->cfg.kind) {
    case SIM_ATK_REPLAY: {
        /* Capture the FIRST PDU's first fragment once and replay it on
         * every subsequent PDU after a short warm-up. Using a fixed
         * ancient snapshot (slot 0) guarantees the replayed freshness
         * stays strictly below the receiver's last-accepted counter,
         * regardless of how many frames have gone by. Earlier versions
         * used a sliding 4-back window, which worked for multi-fragment
         * PQC/HYBRID PDUs (signature mismatch caught it) but let
         * single-fragment HMAC PDUs slip through because the "old" pick
         * was still ahead of RX after a few iterations. */
        if (is_first_fragment) {
            if (atk->replay_slot == 0) {
                /* First-ever PDU: snapshot into slot 0 and let it pass
                 * untouched so the receiver records this freshness
                 * value. Subsequent replays will then be strictly
                 * older. */
                memcpy(atk->replay_buf[0], data, *len);
                atk->replay_len[0] = *len;
                atk->replay_tag[0] = *tag;
            }
            atk->replay_slot++;

            if (atk->replay_slot > 3) {
                uint32_t n = atk->replay_len[0];
                if (n > 0 && n == *len) {
                    /* Replace the current fragment with the ancient
                     * snapshot. For HMAC (single fragment) this is a
                     * wholesale replay → freshness check must reject.
                     * For PQC/HYBRID (multi-fragment) only the header
                     * is old while continuation fragments are fresh,
                     * so signature verification catches it too. */
                    memcpy(data, atk->replay_buf[0], n);
                    atk->last_replay_ns = now_ns;
                    atomic_fetch_add(&atk->injected, 1);
                    evt.note = "replay injected";
                    act = SIM_ATTACK_MODIFY;
                }
            }
        }
        break;
    }

    case SIM_ATK_TAMPER_PAYLOAD: {
        if (*len > 4) {
            uint64_t r = rng_next(&atk->rng_state);
            uint32_t bit = (uint32_t)(r % ((*len - 3) * 8));
            data[bit / 8] ^= (uint8_t)(1U << (bit % 8));
            atomic_fetch_add(&atk->injected, 1);
            evt.note = "payload bit-flip";
            act = SIM_ATTACK_MODIFY;
        }
        break;
    }

    case SIM_ATK_TAMPER_AUTH: {
        /* Flip bits at the tail (where the authenticator lives). */
        if (*len > 4) {
            uint32_t tail_off = *len - 1 - (uint32_t)(rng_next(&atk->rng_state) % 4);
            data[tail_off] ^= 0xFFU;
            atomic_fetch_add(&atk->injected, 1);
            evt.note = "authenticator bit-flip";
            act = SIM_ATTACK_MODIFY;
        }
        break;
    }

    case SIM_ATK_FRESHNESS_ROLLBACK: {
        /* Assume freshness counter is the first 8 bytes after the 2-byte header. */
        if (*len >= 10) {
            memset(&data[2], 0, 8);
            atomic_fetch_add(&atk->injected, 1);
            evt.note = "freshness set to zero";
            act = SIM_ATTACK_MODIFY;
        }
        break;
    }

    case SIM_ATK_SIG_FUZZ: {
        /* Randomise the authenticator bytes at the tail of every fragment.
         * On MTU-limited buses (CAN-FD 64 B) a single fragment can be the
         * entire signature tail, so we fuzz up to 16 B of tail per frame. */
        if (*len > 4) {
            uint32_t fuzz_n = (*len >= 16) ? 16U : (*len - 1U);
            for (uint32_t i = *len - fuzz_n; i < *len; ++i) {
                data[i] = (uint8_t)(rng_next(&atk->rng_state) & 0xFF);
            }
            atomic_fetch_add(&atk->injected, 1);
            evt.note = "signature tail fuzz";
            act = SIM_ATTACK_MODIFY;
        }
        break;
    }

    case SIM_ATK_DOS_FLOOD: {
        /* Duplicate the current frame one extra time to saturate the bus. */
        atomic_fetch_add(&atk->injected, 1);
        evt.note = "dos frame amplification";
        /* The hook can't actually inject a second frame from here without
         * risking deadlock on the bus mutex; increment the counter and
         * let the scenario's flood thread do the real work. */
        act = SIM_ATTACK_PASS;
        break;
    }

    case SIM_ATK_MITM_KEY_CONFUSE: {
        /* MITM swapping session-key material: scramble the freshness +
         * authenticator region so the receiver sees a message that was
         * "authenticated" with the wrong key. We leave the 2-byte header
         * intact so downstream parsing still runs. */
        if (*len > 10) {
            for (uint32_t i = 2; i < *len; ++i) {
                data[i] ^= (uint8_t)(rng_next(&atk->rng_state) & 0xFF);
            }
            atomic_fetch_add(&atk->injected, 1);
            evt.note = "key-confuse scramble";
            act = SIM_ATTACK_MODIFY;
        }
        break;
    }

    case SIM_ATK_DOWNGRADE_HMAC: {
        /* Flip the protection-mode tag (MSB 2 bits of header). */
        if (*len >= 2) {
            data[0] = (uint8_t)((data[0] & 0x3F) | 0x40);
            atomic_fetch_add(&atk->injected, 1);
            evt.note = "mode forced to HMAC";
            act = SIM_ATTACK_MODIFY;
        }
        break;
    }

    case SIM_ATK_HARVEST_NOW: {
        /* Passive: just store the ciphertext for later analysis. */
        uint32_t s = atk->replay_slot % ATK_REPLAY_CACHE;
        memcpy(atk->replay_buf[s], data, *len);
        atk->replay_len[s] = *len;
        atk->replay_slot++;
        atomic_fetch_add(&atk->injected, 1);
        evt.note = "ciphertext harvested";
        break;
    }

    case SIM_ATK_TIMING_PROBE: {
        /* Passive observation – no modification. */
        atomic_fetch_add(&atk->injected, 1);
        evt.note = "timing observed";
        break;
    }

    default:
        break;
    }

    if (act != SIM_ATTACK_PASS || evt.note) {
        sim_log_attack(&evt);
    }

    pthread_mutex_unlock(&atk->mtx);
    return act;
}

SimAttacker *sim_attacker_create(const SimAttackCfg *cfg)
{
    SimAttacker *a = (SimAttacker *)calloc(1, sizeof(*a));
    if (!a) return NULL;
    a->cfg = *cfg;
    pthread_mutex_init(&a->mtx, NULL);
    a->rng_state = cfg->seed ? cfg->seed : 0xDEADBEEFCAFEBABEULL;
    return a;
}

void sim_attacker_attach(SimAttacker *atk, SimBus *bus)
{
    if (!atk || !bus) return;
    atk->attached = bus;
    sim_bus_set_hook(bus, attacker_hook, atk);
    sim_log(SIM_LOG_INFO, "attacker[%s] attached to bus",
            sim_attack_name(atk->cfg.kind));
}

void sim_attacker_detach(SimAttacker *atk)
{
    if (!atk || !atk->attached) return;
    sim_bus_set_hook(atk->attached, NULL, NULL);
    atk->attached = NULL;
}

void sim_attacker_destroy(SimAttacker *atk)
{
    if (!atk) return;
    sim_attacker_detach(atk);
    pthread_mutex_destroy(&atk->mtx);
    free(atk);
}

void sim_attacker_notify_verify_outcome(SimAttacker *atk, bool ok,
                                        uint16_t signal_id)
{
    if (!atk) return;
    if (ok) {
        atomic_fetch_add(&atk->delivered, 1);
    } else {
        atomic_fetch_add(&atk->detected, 1);
    }
    (void)signal_id;
}

void sim_attacker_get_stats(const SimAttacker *atk, SimAttackerStats *out)
{
    if (!atk || !out) return;
    out->injected  = atomic_load(&atk->injected);
    out->detected  = atomic_load(&atk->detected);
    out->delivered = atomic_load(&atk->delivered);
}
