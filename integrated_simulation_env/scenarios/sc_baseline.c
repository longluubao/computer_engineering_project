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
 * sc_baseline_run
 * ---------------
 * Transmits every signal in the catalogue N times over its preferred
 * bus. Measures:
 *   - end-to-end application latency
 *   - SecOC auth / verify time
 *   - fragmentation count
 *   - PDU size
 *   - deadline miss per class
 *
 * This is the canonical latency/overhead table for the thesis.
 */

static SimBusCfg bus_cfg_for(SimBusKind kind)
{
    SimBusCfg c;
    memset(&c, 0, sizeof(c));
    c.kind = kind;
    c.bit_error_rate = 0.0;
    switch (kind) {
        case SIM_BUS_CAN_20:
            c.bitrate_bps    = 1000000U;   c.mtu_bytes = 8U;
            c.propagation_ns = 5000U;      break;
        case SIM_BUS_CAN_FD:
            c.bitrate_bps    = 5000000U;   c.mtu_bytes = 64U;
            c.propagation_ns = 3000U;      break;
        case SIM_BUS_FLEXRAY:
            c.bitrate_bps    = 10000000U;  c.mtu_bytes = 254U;
            c.propagation_ns = 2000U;      break;
        case SIM_BUS_ETH_100:
            c.bitrate_bps    = 100000000U; c.mtu_bytes = 1500U;
            c.propagation_ns = 1000U;      break;
        case SIM_BUS_ETH_1000:
            c.bitrate_bps    = 1000000000U;c.mtu_bytes = 1500U;
            c.propagation_ns = 500U;       break;
    }
    return c;
}

static uint32_t deadline_ns_for_class(uint8_t cls)
{
    switch (cls) {
        case 1: return   5000000U;  /* 5 ms  */
        case 2: return  10000000U;  /* 10 ms */
        case 3: return  20000000U;
        case 4: return  50000000U;
        case 5: return 100000000U;
        case 6: return 500000000U;
        default: return 100000000U;
    }
}

int sc_baseline_run(const SimConfig *cfg)
{
    SimMetrics m;
    sim_metrics_init(&m);
    m.session_ns_start = sim_now_ns();

    size_t sig_count = 0;
    const SimSignalDef *cat = sim_signal_catalogue(&sig_count);

    /* One bus per distinct kind the catalogue uses. */
    SimBus *buses[5] = {0};
    for (size_t i = 0; i < sig_count; ++i) {
        SimBusKind k = cat[i].preferred_bus;
        if (!buses[k]) {
            SimBusCfg bc = bus_cfg_for(k);
            buses[k] = sim_bus_create((uint8_t)k, &bc);
        }
    }

    /* Pick "the" primary bus for tx/rx role binding – the busiest one. */
    SimEcuCfg tx_cfg;
    memset(&tx_cfg, 0, sizeof(tx_cfg));
    tx_cfg.name = "tx-ecu";
    tx_cfg.role = SIM_ECU_ROLE_TX;
    tx_cfg.protection = cfg->protection;
    tx_cfg.metrics = &m;
    tx_cfg.seed = cfg->seed;

    SimEcuCfg rx_cfg = tx_cfg;
    rx_cfg.name = "rx-ecu";
    rx_cfg.role = SIM_ECU_ROLE_RX;
    rx_cfg.seed = cfg->seed ^ 0xA5A5A5A5A5A5A5A5ULL;

    /* PQC: KEM handshake once to share a key we use for the HMAC path too. */
    for (size_t i = 0; i < sig_count; ++i) {
        SimBusKind k = cat[i].preferred_bus;
        tx_cfg.primary_bus = buses[k];
        rx_cfg.primary_bus = buses[k];

        SimEcu *tx = sim_ecu_create(&tx_cfg);
        SimEcu *rx = sim_ecu_create(&rx_cfg);
        if (!sim_ecu_init_stack(tx) || !sim_ecu_init_stack(rx)) {
            sim_log(SIM_LOG_ERROR, "stack init failed for signal 0x%02X",
                    cat[i].id);
            sim_ecu_destroy(tx);
            sim_ecu_destroy(rx);
            continue;
        }

        if (cfg->protection == SIM_PROT_PQC ||
            cfg->protection == SIM_PROT_HYBRID) {
            /* Use tx's ML-DSA public key on the rx side (simulated bootstrap). */
            (void)sim_ecu_pqc_handshake(tx, rx);
        }
        /* HMAC and HYBRID also need a shared symmetric key; the share
         * call is idempotent so we always run it after init. */
        sim_ecu_share_keys(tx, rx);

        sim_log(SIM_LOG_INFO,
                "signal 0x%02X (%s): %u iterations on bus_kind=%d",
                cat[i].id, cat[i].name, cfg->iterations, (int)k);

        uint8_t payload[4096];
        for (uint32_t j = 0; j < cat[i].payload_bytes && j < sizeof(payload); ++j) {
            payload[j] = (uint8_t)(cfg->seed + j + cat[i].id);
        }

        uint32_t deadline_ns = deadline_ns_for_class(cat[i].deadline_class);

        for (uint32_t it = 0; it < cfg->iterations; ++it) {
            uint64_t t_app_tx = sim_now_ns();
            if (!sim_ecu_send_signal(tx, &cat[i], payload, cat[i].payload_bytes)) {
                m.drop_count++;
                continue;
            }
            uint16_t rx_id = 0;
            uint32_t out_len = 0;
            uint8_t rx_buf[4096];
            bool ok = sim_ecu_recv_signal(rx, &rx_id, rx_buf, sizeof(rx_buf),
                                          &out_len, deadline_ns * 4ULL);
            uint64_t t_app_rx = sim_now_ns();
            uint64_t e2e = t_app_rx - t_app_tx;

            sim_hist_add(&m.e2e_latency, e2e);
            m.deadline_total_count[cat[i].deadline_class]++;
            if (e2e > deadline_ns) m.deadline_miss_count[cat[i].deadline_class]++;
            if (!ok) m.verify_fail_count++;
        }

        sim_ecu_destroy(tx);
        sim_ecu_destroy(rx);
    }

    m.session_ns_end = sim_now_ns();

    for (int i = 0; i < 5; ++i) {
        if (buses[i]) sim_bus_destroy(buses[i]);
    }

    sim_scenario_finalise(&m, "baseline", cfg);
    return 0;
}
