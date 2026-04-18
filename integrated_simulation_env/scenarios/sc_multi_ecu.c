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

/*
 * sc_multi_ecu_run
 * ----------------
 * 1:N broadcast scenario. One TX ECU publishes a safety signal to three
 * independent receivers. Every receiver maintains its own SecOC verify
 * state (freshness counter, shared HMAC / ML-DSA public key).
 *
 * Our virtual bus is a single-consumer FIFO, so "broadcast" is modelled
 * by wiring one bus per receiver and rebinding the TX to each bus in
 * turn (using sim_ecu_set_primary_bus). This keeps a single signer
 * keypair and freshness counter across the whole domain, exactly like
 * the real AUTOSAR SecOC model where one Tx-PDU stream is observed by
 * every node on the shared wire.
 *
 * Thesis evidence:
 *   - Identical per-receiver e2e latency (overhead is TX-side only).
 *   - Linear aggregate success count = iterations × receivers.
 */

#define NUM_RX_NODES 3

static SimBusCfg canfd_cfg(void)
{
    SimBusCfg bc;
    memset(&bc, 0, sizeof(bc));
    bc.kind = SIM_BUS_CAN_FD;
    bc.bitrate_bps    = 5000000U;
    bc.mtu_bytes      = 64U;
    bc.propagation_ns = 3000U;
    return bc;
}

int sc_multi_ecu_run(const SimConfig *cfg)
{
    SimMetrics m_agg;
    sim_metrics_init(&m_agg);
    m_agg.session_ns_start = sim_now_ns();

    SimMetrics m_rx[NUM_RX_NODES];
    SimBus    *bus[NUM_RX_NODES];
    SimEcu    *rx[NUM_RX_NODES];

    SimMetrics m_tx;
    sim_metrics_init(&m_tx);
    m_tx.session_ns_start = sim_now_ns();

    for (int i = 0; i < NUM_RX_NODES; ++i) {
        sim_metrics_init(&m_rx[i]);
        m_rx[i].session_ns_start = sim_now_ns();
        SimBusCfg bc = canfd_cfg();
        bus[i] = sim_bus_create((uint8_t)i, &bc);
    }

    SimEcuCfg tx_cfg = {0};
    tx_cfg.role = SIM_ECU_ROLE_TX;
    tx_cfg.protection = cfg->protection;
    tx_cfg.seed = cfg->seed;
    tx_cfg.metrics = &m_tx;
    tx_cfg.name = "tx-multi";
    tx_cfg.primary_bus = bus[0];
    SimEcu *tx = sim_ecu_create(&tx_cfg);
    sim_ecu_init_stack(tx);

    for (int i = 0; i < NUM_RX_NODES; ++i) {
        SimEcuCfg rc = tx_cfg;
        char nm[32]; snprintf(nm, sizeof(nm), "rx-multi-%d", i);
        rc.role = SIM_ECU_ROLE_RX;
        rc.name = nm;
        rc.primary_bus = bus[i];
        rc.metrics = &m_rx[i];
        rc.seed = cfg->seed ^ (0xA5A5A5A5A5A5A5A5ULL + (uint64_t)i);
        rx[i] = sim_ecu_create(&rc);
        sim_ecu_init_stack(rx[i]);
        if (cfg->protection == SIM_PROT_PQC ||
            cfg->protection == SIM_PROT_HYBRID) {
            (void)sim_ecu_pqc_handshake(tx, rx[i]);
        }
        sim_ecu_share_keys(tx, rx[i]);
    }

    const SimSignalDef *sig = sim_signal_find(0x04);
    uint8_t payload[64];
    for (uint32_t i = 0; i < sizeof(payload); ++i) payload[i] = (uint8_t)i;

    uint32_t n = cfg->iterations < 50 ? 50 : cfg->iterations;

    /* For each iteration, broadcast the SAME logical PDU to every
     * receiver. We snapshot the TX freshness before each iteration
     * and restore it between receivers so every receiver sees the
     * identical freshness value for the same broadcast round. */
    for (uint32_t it = 0; it < n; ++it) {
        uint64_t base_fresh = sim_ecu_get_freshness_tx(tx);

        for (int i = 0; i < NUM_RX_NODES; ++i) {
            sim_ecu_set_freshness_tx(tx, base_fresh);
            sim_ecu_set_primary_bus(tx, bus[i]);

            uint64_t t0 = sim_now_ns();
            sim_ecu_send_signal(tx, sig, payload, sig->payload_bytes);

            uint8_t rxbuf[4096];
            uint16_t id = 0;
            uint32_t out_len = 0;
            bool ok = sim_ecu_recv_signal(rx[i], &id, rxbuf, sizeof(rxbuf),
                                          &out_len, 50000000ULL);
            if (ok) sim_hist_add(&m_rx[i].e2e_latency, sim_now_ns() - t0);
        }
    }

    for (int i = 0; i < NUM_RX_NODES; ++i) {
        m_rx[i].session_ns_end = sim_now_ns();
        m_agg.success_count     += m_rx[i].success_count;
        m_agg.verify_fail_count += m_rx[i].verify_fail_count;

        char scn[64];
        snprintf(scn, sizeof(scn), "multi_ecu_rx%d", i);
        sim_scenario_finalise(&m_rx[i], scn, cfg);
    }

    m_tx.session_ns_end = sim_now_ns();
    sim_scenario_finalise(&m_tx, "multi_ecu_tx", cfg);

    m_agg.session_ns_end = sim_now_ns();
    sim_scenario_finalise(&m_agg, "multi_ecu_aggregate", cfg);

    sim_log(SIM_LOG_INFO,
            "multi_ecu: %u iterations x %d receivers = %u expected verifies; "
            "aggregate success=%llu verify_fail=%llu",
            n, NUM_RX_NODES, n * NUM_RX_NODES,
            (unsigned long long)m_agg.success_count,
            (unsigned long long)m_agg.verify_fail_count);

    sim_ecu_destroy(tx);
    for (int i = 0; i < NUM_RX_NODES; ++i) {
        sim_ecu_destroy(rx[i]);
        sim_bus_destroy(bus[i]);
    }
    return 0;
}
