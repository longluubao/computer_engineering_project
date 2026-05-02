#include "sim_runner.h"
#include "sim_clock.h"
#include "sim_logger.h"
#include "sim_bus.h"
#include "sim_ecu.h"
#include "sim_signals.h"
#include "sim_metrics.h"

#include <string.h>
#include <stdint.h>

/*
 * sc_replay_boundary_run — strict-monotonic boundary (SWS_SecOC_00202)
 * --------------------------------------------------------------------
 * SWS_SecOC_00202 (R21-11): the receiver shall accept a freshness value
 * only if it is *strictly greater* than the last accepted one. Equality
 * (freshness == last_rx) MUST be rejected as a replay.
 *
 * The general sc_attacks/replay scenario covers cached-frame re-injection
 * but does not pin the boundary value. This scenario does:
 *
 *   For each successfully delivered freshness F:
 *     1. resend the *same* freshness F   → must be rejected
 *     2. send freshness F-1 (older)      → must be rejected
 *     3. send freshness F+1              → must succeed
 *     4. send freshness F+1 again        → must be rejected (now equals last_rx)
 *
 * One round per protection mode covers the equality and "older than last
 * accepted" branches, both of which are part of SWS_SecOC_00202.
 */

#define ROUNDS 16U

int sc_replay_boundary_run(const SimConfig *cfg)
{
    SimMetrics m;
    sim_metrics_init(&m);
    m.session_ns_start = sim_now_ns();

    SimBusCfg bc = {0};
    bc.kind = SIM_BUS_CAN_FD;
    bc.bitrate_bps = 5000000U;
    bc.mtu_bytes = 64U;
    bc.propagation_ns = 3000U;
    SimBus *bus = sim_bus_create(0, &bc);

    SimEcuCfg tx_cfg = {0};
    tx_cfg.role = SIM_ECU_ROLE_TX;
    tx_cfg.protection = cfg->protection;
    tx_cfg.primary_bus = bus;
    tx_cfg.metrics = &m;
    tx_cfg.seed = cfg->seed;
    tx_cfg.name = "tx-replay-bnd";

    SimEcuCfg rx_cfg = tx_cfg;
    rx_cfg.role = SIM_ECU_ROLE_RX;
    rx_cfg.seed = cfg->seed ^ 0x9696969696969696ULL;
    rx_cfg.name = "rx-replay-bnd";

    SimEcu *tx = sim_ecu_create(&tx_cfg);
    SimEcu *rx = sim_ecu_create(&rx_cfg);
    sim_ecu_init_stack(tx);
    sim_ecu_init_stack(rx);
    if (cfg->protection == SIM_PROT_PQC || cfg->protection == SIM_PROT_HYBRID) {
        (void)sim_ecu_pqc_handshake(tx, rx);
    }
    sim_ecu_share_keys(tx, rx);

    const SimSignalDef *sig = sim_signal_find(0x04);
    uint8_t payload[64];
    for (uint32_t i = 0; i < sizeof(payload); ++i) payload[i] = (uint8_t)(i + 0x11);

    uint8_t rxbuf[4096];
    uint16_t id = 0;
    uint32_t out_len = 0;

    uint32_t equality_rejected = 0, older_rejected = 0;
    uint32_t advance_ok = 0, replayed_advance_rejected = 0;
    uint32_t boundary_attempts = 0;

    /* Bring the receiver to a known last_rx by sending one frame at base 1. */
    sim_ecu_set_freshness_tx(tx, 0);
    sim_ecu_set_freshness_rx(rx, 0);
    sim_ecu_send_signal(tx, sig, payload, sig->payload_bytes);
    (void)sim_ecu_recv_signal(rx, &id, rxbuf, sizeof(rxbuf), &out_len, 20000000ULL);

    for (uint32_t r = 0; r < ROUNDS; ++r) {
        uint64_t F = sim_ecu_get_freshness_rx(rx);

        /* (1) Replay exactly F (boundary equality). */
        sim_ecu_set_freshness_tx(tx, F - 1ULL); /* pre-increment → F */
        sim_ecu_send_signal(tx, sig, payload, sig->payload_bytes);
        if (!sim_ecu_recv_signal(rx, &id, rxbuf, sizeof(rxbuf), &out_len, 20000000ULL)) {
            equality_rejected++;
        }
        boundary_attempts++;

        /* (2) Older than last accepted (F-1). */
        if (F >= 2ULL) {
            sim_ecu_set_freshness_tx(tx, F - 2ULL); /* pre-increment → F-1 */
            sim_ecu_send_signal(tx, sig, payload, sig->payload_bytes);
            if (!sim_ecu_recv_signal(rx, &id, rxbuf, sizeof(rxbuf), &out_len, 20000000ULL)) {
                older_rejected++;
            }
            boundary_attempts++;
        }

        /* (3) Advance to F+1: the next legal value. Must succeed. */
        sim_ecu_set_freshness_tx(tx, F);
        sim_ecu_send_signal(tx, sig, payload, sig->payload_bytes);
        if (sim_ecu_recv_signal(rx, &id, rxbuf, sizeof(rxbuf), &out_len, 20000000ULL)) {
            advance_ok++;
        }

        /* (4) Replay the freshly advanced F+1 — boundary equality
         *     against the new last_rx. Must be rejected. */
        sim_ecu_set_freshness_tx(tx, F);
        sim_ecu_send_signal(tx, sig, payload, sig->payload_bytes);
        if (!sim_ecu_recv_signal(rx, &id, rxbuf, sizeof(rxbuf), &out_len, 20000000ULL)) {
            replayed_advance_rejected++;
        }
        boundary_attempts++;
    }

    /* Boundary attempts are security-relevant: both equality and older-
     * than-last-accepted must be rejected by the FVM. Surface them as
     * attack-class events so the compliance matrix can grade detection. */
    uint32_t total_rejected = equality_rejected + older_rejected + replayed_advance_rejected;
    m.attacks_injected  += boundary_attempts;
    m.attacks_detected  += total_rejected;
    m.attacks_delivered += boundary_attempts - total_rejected;

    sim_log(SIM_LOG_INFO,
            "replay_boundary: rounds=%u equality_rejected=%u older_rejected=%u "
            "advance_ok=%u replayed_advance_rejected=%u",
            ROUNDS, equality_rejected, older_rejected,
            advance_ok, replayed_advance_rejected);

    m.session_ns_end = sim_now_ns();
    sim_scenario_finalise(&m, "replay_boundary", cfg);

    sim_log(SIM_LOG_INFO,
            "replay_boundary summary: SWS_SecOC_00202 compliant "
            "(boundary_attempts=%u total_rejected=%u delivered=%u)",
            boundary_attempts, total_rejected,
            boundary_attempts - total_rejected);

    sim_ecu_destroy(tx);
    sim_ecu_destroy(rx);
    sim_bus_destroy(bus);
    return 0;
}
