#include "sim_bus.h"
#include "sim_clock.h"
#include "sim_logger.h"

#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#define BUS_MAX_QUEUE   4096
#define BUS_MAX_FRAME   8192

typedef struct {
    uint64_t ready_ns;   /* when the frame can be delivered */
    uint32_t tag;
    uint32_t len;
    uint8_t  priority;
    uint8_t  data[BUS_MAX_FRAME];
} BusFrame;

struct SimBus {
    uint8_t           id;
    SimBusCfg         cfg;

    pthread_mutex_t   mtx;
    pthread_cond_t    cv;

    BusFrame         *ring;
    uint32_t          head;
    uint32_t          tail;
    uint32_t          capacity;

    SimBusHookFn      hook_fn;
    void             *hook_user;

    /* stats */
    _Atomic uint64_t  frames_tx;
    _Atomic uint64_t  frames_rx;
    _Atomic uint64_t  frames_dropped_ber;
    _Atomic uint64_t  frames_dropped_queue;
    _Atomic uint64_t  bytes_tx;
    _Atomic uint64_t  bytes_rx;

    /* seeded RNG for BER */
    uint64_t          rng_state;
};

/* xoshiro256** tiny PRNG (public domain). */
static uint64_t xoshiro_next(uint64_t *s)
{
    uint64_t x = *s;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    *s = x;
    return x;
}

SimBus *sim_bus_create(uint8_t id, const SimBusCfg *cfg)
{
    SimBus *b = (SimBus *)calloc(1, sizeof(*b));
    if (!b) return NULL;
    b->id = id;
    b->cfg = *cfg;
    b->capacity = BUS_MAX_QUEUE;
    b->ring = (BusFrame *)calloc(b->capacity, sizeof(BusFrame));
    if (!b->ring) { free(b); return NULL; }
    pthread_mutex_init(&b->mtx, NULL);
    pthread_cond_init(&b->cv, NULL);
    b->rng_state = 0x9E3779B97F4A7C15ULL ^ ((uint64_t)id << 32);
    sim_log(SIM_LOG_INFO,
            "bus[%u] kind=%d bitrate=%u MTU=%u ber=%.2e prop_ns=%llu",
            id, (int)cfg->kind, cfg->bitrate_bps, cfg->mtu_bytes,
            cfg->bit_error_rate, (unsigned long long)cfg->propagation_ns);
    return b;
}

void sim_bus_destroy(SimBus *bus)
{
    if (!bus) return;
    pthread_mutex_destroy(&bus->mtx);
    pthread_cond_destroy(&bus->cv);
    free(bus->ring);
    free(bus);
}

void sim_bus_set_hook(SimBus *bus, SimBusHookFn fn, void *user)
{
    if (!bus) return;
    pthread_mutex_lock(&bus->mtx);
    bus->hook_fn   = fn;
    bus->hook_user = user;
    pthread_mutex_unlock(&bus->mtx);
}

static uint64_t compute_transit_ns(const SimBusCfg *cfg, uint32_t bytes)
{
    /* Simplified: bit-serial transit = (bits / bitrate) + propagation. */
    uint64_t bit_ns = (uint64_t)bytes * 8ULL * 1000000000ULL / cfg->bitrate_bps;
    return bit_ns + cfg->propagation_ns;
}

bool sim_bus_tx(SimBus *bus, uint8_t priority, const uint8_t *data,
                uint32_t len, uint32_t tag)
{
    if (!bus || !data || len == 0 || len > BUS_MAX_FRAME) return false;

    /* Fragmentation is handled at higher layers; ensure payload ≤ MTU. */
    if (len > bus->cfg.mtu_bytes) {
        atomic_fetch_add(&bus->frames_dropped_queue, 1);
        sim_log(SIM_LOG_WARN, "bus[%u] payload %u > MTU %u (dropped)",
                bus->id, len, bus->cfg.mtu_bytes);
        return false;
    }

    pthread_mutex_lock(&bus->mtx);

    uint32_t next_tail = (bus->tail + 1) % bus->capacity;
    if (next_tail == bus->head) {
        atomic_fetch_add(&bus->frames_dropped_queue, 1);
        pthread_mutex_unlock(&bus->mtx);
        return false;
    }

    /* Mutable working buffer in case a hook modifies it. */
    BusFrame *slot = &bus->ring[bus->tail];
    memcpy(slot->data, data, len);
    slot->len = len;
    slot->tag = tag;
    slot->priority = priority;

    uint32_t hook_len = len;
    uint32_t hook_tag = tag;
    SimBusHookAction act = SIM_ATTACK_PASS;
    if (bus->hook_fn) {
        act = bus->hook_fn(bus->id, slot->data, &hook_len, &hook_tag,
                           bus->hook_user);
        slot->len = hook_len;
        slot->tag = hook_tag;
    }

    if (act == SIM_ATTACK_DROP) {
        /* silently dropped by attacker */
        pthread_mutex_unlock(&bus->mtx);
        atomic_fetch_add(&bus->frames_tx, 1);
        return true;
    }

    /* BER check: simulate a single corrupted bit flips the frame. */
    if (bus->cfg.bit_error_rate > 0.0) {
        double roll = (double)xoshiro_next(&bus->rng_state) / (double)UINT64_MAX;
        if (roll < bus->cfg.bit_error_rate * (double)len * 8.0) {
            /* flip a random bit */
            uint64_t r = xoshiro_next(&bus->rng_state);
            uint32_t bit = (uint32_t)(r % (len * 8));
            slot->data[bit / 8] ^= (uint8_t)(1U << (bit % 8));
            atomic_fetch_add(&bus->frames_dropped_ber, 1);
            /* we still deliver it – SecOC must detect the corruption */
        }
    }

    slot->ready_ns = sim_now_ns() + compute_transit_ns(&bus->cfg, slot->len);
    bus->tail = next_tail;
    atomic_fetch_add(&bus->frames_tx, 1);
    atomic_fetch_add(&bus->bytes_tx, slot->len);

    pthread_cond_broadcast(&bus->cv);
    pthread_mutex_unlock(&bus->mtx);
    return true;
}

uint32_t sim_bus_rx(SimBus *bus, uint8_t *buf, uint32_t max_len,
                    uint64_t timeout_ns, uint32_t *out_tag)
{
    if (!bus || !buf || max_len == 0) return 0;

    uint64_t deadline = sim_now_ns() + timeout_ns;
    pthread_mutex_lock(&bus->mtx);

    while (bus->head == bus->tail ||
           bus->ring[bus->head].ready_ns > sim_now_ns()) {
        if (bus->head == bus->tail) {
            /* empty queue: wait for a producer. */
            struct timespec ts;
#ifdef _WIN32
            /* Windows pthreads emulation uses absolute realtime; approximate. */
            ts.tv_sec  = (time_t)(deadline / 1000000000ULL);
            ts.tv_nsec = (long)(deadline % 1000000000ULL);
#else
            clock_gettime(CLOCK_REALTIME, &ts);
            uint64_t wall_ns = (uint64_t)ts.tv_sec * 1000000000ULL +
                               (uint64_t)ts.tv_nsec + timeout_ns;
            ts.tv_sec  = (time_t)(wall_ns / 1000000000ULL);
            ts.tv_nsec = (long)(wall_ns % 1000000000ULL);
#endif
            int r = pthread_cond_timedwait(&bus->cv, &bus->mtx, &ts);
            if (r != 0 && sim_now_ns() >= deadline) {
                pthread_mutex_unlock(&bus->mtx);
                return 0;
            }
        } else {
            /* queue has frames but head isn't ready; sleep briefly */
            pthread_mutex_unlock(&bus->mtx);
            uint64_t now = sim_now_ns();
            if (now >= deadline) return 0;
            uint64_t wait = bus->ring[bus->head].ready_ns - now;
            if (wait > 1000000ULL) wait = 1000000ULL; /* cap at 1 ms */
            sim_sleep_ns(wait);
            pthread_mutex_lock(&bus->mtx);
        }
    }

    BusFrame *slot = &bus->ring[bus->head];
    uint32_t copy = (slot->len <= max_len) ? slot->len : max_len;
    memcpy(buf, slot->data, copy);
    uint32_t real_len = slot->len;
    if (out_tag) *out_tag = slot->tag;
    bus->head = (bus->head + 1) % bus->capacity;

    atomic_fetch_add(&bus->frames_rx, 1);
    atomic_fetch_add(&bus->bytes_rx, real_len);

    pthread_mutex_unlock(&bus->mtx);
    return real_len;
}

void sim_bus_get_stats(const SimBus *bus, SimBusStats *out)
{
    if (!bus || !out) return;
    out->frames_tx            = atomic_load(&bus->frames_tx);
    out->frames_rx            = atomic_load(&bus->frames_rx);
    out->frames_dropped_ber   = atomic_load(&bus->frames_dropped_ber);
    out->frames_dropped_queue = atomic_load(&bus->frames_dropped_queue);
    out->bytes_tx             = atomic_load(&bus->bytes_tx);
    out->bytes_rx             = atomic_load(&bus->bytes_rx);
}
