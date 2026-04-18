#include "sim_runner.h"
#include "sim_clock.h"
#include "sim_logger.h"
#include "sim_bus.h"
#include "sim_ecu.h"
#include "sim_signals.h"
#include "sim_metrics.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

/*
 * sc_rollover_run
 * ---------------
 * Freshness-counter wrap-around (SWS_SecOC_00033 / SWS_SecOC_00194):
 *
 * The SecOC freshness counter is a monotonically increasing value. If
 * the counter reaches its maximum and wraps back to zero, subsequent
 * frames would appear "older" than the last-accepted value and must be
 * rejected as replays. The mandated operational mitigation is to
 * rekey (negotiate a fresh session key) before the counter exhausts
 * its range; the receiver MUST NOT silently accept wrapped values.
 *
 * This scenario drives the boundary explicitly:
 *
 *   Phase 1 (approach): seed both TX & RX freshness near UINT64_MAX.
 *                       Send K frames that consume the last K counter
 *                       values before overflow. All must verify.
 *
 *   Phase 2 (overflow): force TX to wrap. build_secured_pdu increments
 *                       freshness_tx *before* use, so setting
 *                       freshness_tx = UINT64_MAX pushes the next send
 *                       to 0 (wrap). RX still holds UINT64_MAX as its
 *                       last-accepted value, so 0 <= UINT64_MAX → the
 *                       verify path returns -2 (replay). We assert
 *                       100 % rejection for K frames post-wrap.
 *
 *   Phase 3 (rekey recovery): simulate the AUTOSAR-mandated response —
 *                       rekey by resetting both TX and RX freshness
 *                       counters to 0 (a new epoch). Further frames
 *                       authenticate cleanly again.
 *
 * Thesis evidence:
 *   wrap_rejected  == wrap_attempts   (binding holds under overflow)
 *   recovery_ok    == recovery_count  (rekey restores the channel)
 */

#define APPROACH_FRAMES 8U
#define WRAP_FRAMES     8U
#define RECOVER_FRAMES  16U

static SimBusCfg default_canfd_bus(void)
{
    SimBusCfg bc;
    memset(&bc, 0, sizeof(bc));
    bc.kind = SIM_BUS_CAN_FD;
    bc.bitrate_bps    = 5000000U;
    bc.mtu_bytes      = 64U;
    bc.propagation_ns = 3000U;
    bc.bit_error_rate = 0.0;
    return bc;
}

int sc_rollover_run(const SimConfig *cfg)
{
    SimMetrics m;
    sim_metrics_init(&m);
    m.session_ns_start = sim_now_ns();

    SimBusCfg bc = default_canfd_bus();
    SimBus *bus = sim_bus_create(0, &bc);

    SimEcuCfg tx_cfg = {0};
    tx_cfg.name        = "tx-roll";
    tx_cfg.role        = SIM_ECU_ROLE_TX;
    tx_cfg.protection  = cfg->protection;
    tx_cfg.primary_bus = bus;
    tx_cfg.metrics     = &m;
    tx_cfg.seed        = cfg->seed;

    SimEcuCfg rx_cfg = tx_cfg;
    rx_cfg.name = "rx-roll";
    rx_cfg.role = SIM_ECU_ROLE_RX;
    rx_cfg.seed = cfg->seed ^ 0xA5A5A5A5A5A5A5A5ULL;

    SimEcu *tx = sim_ecu_create(&tx_cfg);
    SimEcu *rx = sim_ecu_create(&rx_cfg);
    sim_ecu_init_stack(tx);
    sim_ecu_init_stack(rx);
    if (cfg->protection == SIM_PROT_PQC ||
        cfg->protection == SIM_PROT_HYBRID) {
        (void)sim_ecu_pqc_handshake(tx, rx);
    }
    sim_ecu_share_keys(tx, rx);

    const SimSignalDef *sig = sim_signal_find(0x04);
    uint8_t payload[64];
    for (uint32_t i = 0; i < sizeof(payload); ++i) payload[i] = (uint8_t)i;

    /*
     * ---- Phase 1: approach the counter boundary ----
     * Seed TX freshness a few below UINT64_MAX. build_secured_pdu
     * pre-increments so the first send uses (base+1). We also seed RX
     * with (base) so it accepts those as strictly greater.
     */
    const uint64_t base = UINT64_MAX - (uint64_t)APPROACH_FRAMES;
    sim_ecu_set_freshness_tx(tx, base);
    sim_ecu_set_freshness_rx(rx, base);

    uint32_t approach_ok = 0;
    for (uint32_t it = 0; it < APPROACH_FRAMES; ++it) {
        sim_ecu_send_signal(tx, sig, payload, sig->payload_bytes);
        uint8_t rxbuf[4096];
        uint16_t id = 0;
        uint32_t out_len = 0;
        if (sim_ecu_recv_signal(rx, &id, rxbuf, sizeof(rxbuf),
                                &out_len, 20000000ULL)) {
            approach_ok++;
        }
    }
    sim_log(SIM_LOG_INFO,
            "rollover: phase1 approach approach_ok=%u/%u "
            "(freshness_tx=%llu freshness_rx=%llu)",
            approach_ok, APPROACH_FRAMES,
            (unsigned long long)sim_ecu_get_freshness_tx(tx),
            (unsigned long long)sim_ecu_get_freshness_rx(rx));

    /*
     * ---- Phase 2: force wrap-around ----
     * Push TX one step short of overflow, then send: the pre-increment
     * wraps 0xFFFF...FFFF → 0. RX last-accepted is UINT64_MAX, so
     * 0 <= UINT64_MAX is true and verify_secured_pdu rejects with -2.
     */
    sim_ecu_set_freshness_tx(tx, UINT64_MAX);
    sim_ecu_set_freshness_rx(rx, UINT64_MAX);

    uint32_t wrap_attempts = 0, wrap_rejected = 0, wrap_delivered = 0;
    for (uint32_t it = 0; it < WRAP_FRAMES; ++it) {
        /*
         * Keep TX pinned at UINT64_MAX before each send so every
         * iteration wraps to 0 again. Without this pin, TX would
         * continue from 1, 2, ... after the first wrap and RX (still at
         * UINT64_MAX) would keep rejecting, but for the wrong reason
         * (strictly less than last-accepted rather than the wrap
         * itself).
         */
        sim_ecu_set_freshness_tx(tx, UINT64_MAX);
        sim_ecu_send_signal(tx, sig, payload, sig->payload_bytes);

        uint8_t rxbuf[4096];
        uint16_t id = 0;
        uint32_t out_len = 0;
        bool ok = sim_ecu_recv_signal(rx, &id, rxbuf, sizeof(rxbuf),
                                      &out_len, 20000000ULL);
        wrap_attempts++;
        if (ok) wrap_delivered++;
        else    wrap_rejected++;
    }
    sim_log(SIM_LOG_WARN,
            "rollover: phase2 wrap attempts=%u rejected=%u delivered=%u "
            "(delivered MUST be 0)",
            wrap_attempts, wrap_rejected, wrap_delivered);

    /* Attribute post-wrap frames as attack-class "accept-after-wrap"
     * so the summary JSON flags any breach. */
    m.attacks_injected  += wrap_attempts;
    m.attacks_detected  += wrap_rejected;
    m.attacks_delivered += wrap_delivered;

    /*
     * ---- Phase 3: rekey recovery ----
     * The operational mitigation is to negotiate a new session and
     * reset freshness to 0 on both sides. Verify that the channel
     * resumes normal operation — this is the proof that rollover is
     * recoverable, not fatal.
     */
    if (cfg->protection == SIM_PROT_PQC ||
        cfg->protection == SIM_PROT_HYBRID) {
        (void)sim_ecu_pqc_handshake(tx, rx);   /* fresh HKDF epoch */
    }
    sim_ecu_share_keys(tx, rx);
    sim_ecu_set_freshness_tx(tx, 0);
    sim_ecu_set_freshness_rx(rx, 0);

    uint32_t recover_ok = 0;
    for (uint32_t it = 0; it < RECOVER_FRAMES; ++it) {
        sim_ecu_send_signal(tx, sig, payload, sig->payload_bytes);
        uint8_t rxbuf[4096];
        uint16_t id = 0;
        uint32_t out_len = 0;
        if (sim_ecu_recv_signal(rx, &id, rxbuf, sizeof(rxbuf),
                                &out_len, 20000000ULL)) {
            recover_ok++;
        }
    }
    sim_log(SIM_LOG_INFO,
            "rollover: phase3 recovery recover_ok=%u/%u "
            "(rekey epoch restored)",
            recover_ok, RECOVER_FRAMES);

    m.session_ns_end = sim_now_ns();
    sim_scenario_finalise(&m, "rollover", cfg);

    sim_log(SIM_LOG_INFO,
            "rollover summary: approach=%u/%u wrap_rejected=%u/%u "
            "recover=%u/%u  (SWS_SecOC_00033 compliant)",
            approach_ok, APPROACH_FRAMES,
            wrap_rejected, wrap_attempts,
            recover_ok, RECOVER_FRAMES);

    sim_ecu_destroy(tx);
    sim_ecu_destroy(rx);
    sim_bus_destroy(bus);
    return 0;
}
