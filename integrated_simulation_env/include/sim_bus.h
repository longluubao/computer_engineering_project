#ifndef ISE_SIM_BUS_H
#define ISE_SIM_BUS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Unified virtual-bus abstraction. Each bus has a FIFO queue protected
 * by a mutex and condvar. Frames carry their enqueue time so the
 * receiver can sleep until `enqueue_time + transit_delay` to model the
 * physical propagation + serialisation latency.
 */

typedef enum {
    SIM_BUS_CAN_20   = 0,   /* classical CAN, 1 Mbps,     8 B payload   */
    SIM_BUS_CAN_FD   = 1,   /*               5 Mbps,     64 B payload   */
    SIM_BUS_FLEXRAY  = 2,   /*              10 Mbps,    254 B payload   */
    SIM_BUS_ETH_100  = 3,   /* 100BASE-T1, 100 Mbps,   1500 B MTU       */
    SIM_BUS_ETH_1000 = 4    /* 1000BASE-T1, 1 Gbps,    1500 B MTU       */
} SimBusKind;

typedef struct {
    SimBusKind kind;
    uint32_t   bitrate_bps;
    uint32_t   mtu_bytes;
    uint64_t   propagation_ns;     /* hop latency independent of size  */
    double     bit_error_rate;     /* 0.0 .. 1.0                       */
    /* Optional FlexRay/TSN parameters */
    uint32_t   slot_count;         /* 0 = async                        */
    uint64_t   slot_duration_ns;
    bool       tsn_enabled;
    uint8_t    priority_queues;    /* 1..8 for TSN                     */
} SimBusCfg;

typedef struct SimBus SimBus;

SimBus *sim_bus_create(uint8_t bus_id, const SimBusCfg *cfg);
void    sim_bus_destroy(SimBus *bus);

/*
 * Transmit a full frame. Returns false if the bus dropped it (BER,
 * queue-full, or TSN shaper denied). `tag` is an opaque identifier the
 * receiver can correlate back to the sender.
 */
bool sim_bus_tx(SimBus *bus, uint8_t priority, const uint8_t *data,
                uint32_t len, uint32_t tag);

/*
 * Receive one frame (blocking up to timeout_ns). Returns the number of
 * bytes delivered or 0 on timeout. The caller supplies a buffer of
 * max_len bytes; truncation returns the original length for the sake
 * of accounting.
 */
uint32_t sim_bus_rx(SimBus *bus, uint8_t *buf, uint32_t max_len,
                    uint64_t timeout_ns, uint32_t *out_tag);

/*
 * Inspect the bus load (useful for dashboards and attack rate-limit
 * scenarios).
 */
typedef struct {
    uint64_t frames_tx;
    uint64_t frames_rx;
    uint64_t frames_dropped_ber;
    uint64_t frames_dropped_queue;
    uint64_t bytes_tx;
    uint64_t bytes_rx;
} SimBusStats;

void sim_bus_get_stats(const SimBus *bus, SimBusStats *out);

/*
 * Hook used by attackers to intercept frames on the wire.
 * The callback can return one of:
 *   SIM_ATTACK_PASS    → forward unchanged
 *   SIM_ATTACK_DROP    → silently drop
 *   SIM_ATTACK_MODIFY  → the callback already mutated the buffer
 *   SIM_ATTACK_INJECT  → the callback appended a new frame
 */
typedef enum {
    SIM_ATTACK_PASS   = 0,
    SIM_ATTACK_DROP   = 1,
    SIM_ATTACK_MODIFY = 2,
    SIM_ATTACK_INJECT = 3
} SimBusHookAction;

typedef SimBusHookAction (*SimBusHookFn)(uint8_t bus_id, uint8_t *data,
                                         uint32_t *len, uint32_t *tag,
                                         void *user);

void sim_bus_set_hook(SimBus *bus, SimBusHookFn fn, void *user);

#ifdef __cplusplus
}
#endif

#endif /* ISE_SIM_BUS_H */
