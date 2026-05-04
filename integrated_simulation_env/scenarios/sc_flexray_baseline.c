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
 * sc_flexray_baseline_run — FlexRay TDMA bus coverage
 * ---------------------------------------------------
 * Earlier campaigns left the FlexRay bus model defined but never
 * actually drove a scenario over it. This scenario closes that gap by
 * running a per-protection baseline (HMAC / PQC / HYBRID) over a
 * 10 Mbps FlexRay link with a 254-byte payload.
 *
 * FlexRay is the time-triggered backbone for ASIL-C/D chassis and
 * powertrain domains in many production architectures (e.g. BMW,
 * Audi). Including it in the thesis report ensures the bus matrix
 * (CAN-20 / CAN-FD / FlexRay / Eth-100 / Eth-1000) is complete.
 */

int sc_flexray_baseline_run(const SimConfig *cfg)
{
    SimMetrics m;
    sim_metrics_init(&m);
    m.session_ns_start = sim_now_ns();

    SimBusCfg bc = {0};
    bc.kind = SIM_BUS_FLEXRAY;
    bc.bitrate_bps = 10000000U;     /* 10 Mbps static segment            */
    bc.mtu_bytes   = 254U;          /* FlexRay max payload per slot      */
    bc.propagation_ns = 5000U;      /* short bus, ~5 µs                  */
    bc.slot_count = 64U;            /* informative — sim_bus uses bitrate */
    bc.slot_duration_ns = 200000U;  /* 200 µs slots (5 kHz schedule)     */
    SimBus *bus = sim_bus_create(0, &bc);

    SimEcuCfg tx_cfg = {0};
    tx_cfg.role = SIM_ECU_ROLE_TX;
    tx_cfg.protection = cfg->protection;
    tx_cfg.primary_bus = bus;
    tx_cfg.metrics = &m;
    tx_cfg.seed = cfg->seed;
    tx_cfg.name = "tx-flexray";

    SimEcuCfg rx_cfg = tx_cfg;
    rx_cfg.role = SIM_ECU_ROLE_RX;
    rx_cfg.seed = cfg->seed ^ 0x4242424242424242ULL;
    rx_cfg.name = "rx-flexray";

    SimEcu *tx = sim_ecu_create(&tx_cfg);
    SimEcu *rx = sim_ecu_create(&rx_cfg);
    sim_ecu_init_stack(tx);
    sim_ecu_init_stack(rx);
    if (cfg->protection == SIM_PROT_PQC || cfg->protection == SIM_PROT_HYBRID) {
        (void)sim_ecu_pqc_handshake(tx, rx);
    }
    sim_ecu_share_keys(tx, rx);

    /* Pick a few representative signals across deadline classes so the
     * compliance matrix has FlexRay numbers per ASIL band. */
    const uint16_t flexray_signal_ids[] = { 0x01, 0x04, 0x07, 0x0B };
    const uint32_t n_sigs = sizeof(flexray_signal_ids) / sizeof(flexray_signal_ids[0]);

    uint8_t payload[254];
    for (uint32_t i = 0; i < sizeof(payload); ++i) payload[i] = (uint8_t)(i ^ 0xA5);

    uint32_t iterations = cfg->iterations < 50 ? 50 : cfg->iterations;
    uint32_t total_ok = 0, total_attempts = 0;

    for (uint32_t s = 0; s < n_sigs; ++s) {
        const SimSignalDef *sig = sim_signal_find(flexray_signal_ids[s]);
        if (!sig) continue;
        uint32_t plen = sig->payload_bytes <= sizeof(payload) ? sig->payload_bytes : sizeof(payload);

        for (uint32_t it = 0; it < iterations; ++it) {
            uint64_t t0 = sim_now_ns();
            sim_ecu_send_signal(tx, sig, payload, plen);
            uint8_t rxbuf[4096];
            uint16_t id = 0;
            uint32_t out_len = 0;
            if (sim_ecu_recv_signal(rx, &id, rxbuf, sizeof(rxbuf), &out_len, 20000000ULL)) {
                total_ok++;
                sim_hist_add(&m.e2e_latency, sim_now_ns() - t0);
            }
            total_attempts++;
        }
    }

    sim_log(SIM_LOG_INFO,
            "flexray_baseline: signals=%u iterations/sig=%u  total_ok=%u/%u",
            n_sigs, iterations, total_ok, total_attempts);

    m.session_ns_end = sim_now_ns();
    sim_scenario_finalise(&m, "flexray_baseline", cfg);

    sim_ecu_destroy(tx);
    sim_ecu_destroy(rx);
    sim_bus_destroy(bus);
    return 0;
}
