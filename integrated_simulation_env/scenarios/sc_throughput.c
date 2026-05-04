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

/* SWS coverage tags (picked up by reports/build_sws_traceability.py):
 *   - SWS_SecOC_00250 (MainFunctionTx periodic invocation)
 *   - SWS_SecOC_00112 (sustained IfTransmit)
 */

/*
 * sc_throughput_run
 * -----------------
 * Sustained-throughput test: pick one representative signal, push as
 * many messages as possible within 2 seconds. Produces:
 *   - msgs/s, MB/s, p99 latency
 *   - bus utilisation (implicit via sim_bus stats)
 */

extern SimBusCfg sc_default_bus_cfg(SimBusKind kind); /* defined once in sc_common_bus.c */

int sc_throughput_run(const SimConfig *cfg)
{
    SimMetrics m;
    sim_metrics_init(&m);
    m.session_ns_start = sim_now_ns();

    SimBusCfg bc;
    memset(&bc, 0, sizeof(bc));
    bc.kind = cfg->bus_kind;
    bc.bit_error_rate = 0.0;
    switch (cfg->bus_kind) {
        case SIM_BUS_CAN_20:   bc.bitrate_bps=1000000U;    bc.mtu_bytes=8U;    bc.propagation_ns=5000U; break;
        case SIM_BUS_CAN_FD:   bc.bitrate_bps=5000000U;    bc.mtu_bytes=64U;   bc.propagation_ns=3000U; break;
        case SIM_BUS_FLEXRAY:  bc.bitrate_bps=10000000U;   bc.mtu_bytes=254U;  bc.propagation_ns=2000U; break;
        case SIM_BUS_ETH_100:  bc.bitrate_bps=100000000U;  bc.mtu_bytes=1500U; bc.propagation_ns=1000U; break;
        case SIM_BUS_ETH_1000: bc.bitrate_bps=1000000000U; bc.mtu_bytes=1500U; bc.propagation_ns=500U;  break;
    }

    SimBus *bus = sim_bus_create((uint8_t)cfg->bus_kind, &bc);

    SimEcuCfg tx_cfg;
    memset(&tx_cfg, 0, sizeof(tx_cfg));
    tx_cfg.name = "tx-stress";
    tx_cfg.role = SIM_ECU_ROLE_TX;
    tx_cfg.protection = cfg->protection;
    tx_cfg.primary_bus = bus;
    tx_cfg.metrics = &m;
    tx_cfg.seed = cfg->seed;

    SimEcuCfg rx_cfg = tx_cfg;
    rx_cfg.name = "rx-stress";
    rx_cfg.role = SIM_ECU_ROLE_RX;
    rx_cfg.seed = cfg->seed ^ 0xA5A5A5A5A5A5A5A5ULL;

    SimEcu *tx = sim_ecu_create(&tx_cfg);
    SimEcu *rx = sim_ecu_create(&rx_cfg);
    sim_ecu_init_stack(tx);
    sim_ecu_init_stack(rx);
    if (cfg->protection == SIM_PROT_PQC || cfg->protection == SIM_PROT_HYBRID) {
        (void)sim_ecu_pqc_handshake(tx, rx);
    }
    sim_ecu_share_keys(tx, rx);

    /* Pick V2X-sized signal (id=0x0D, 1024 B) if running on Ethernet;
     * otherwise use throttle (id=0x04, 8 B). */
    uint16_t sig_id = (cfg->bus_kind == SIM_BUS_ETH_100 ||
                       cfg->bus_kind == SIM_BUS_ETH_1000) ? 0x0D : 0x04;
    const SimSignalDef *sig = sim_signal_find(sig_id);
    uint8_t payload[4096];
    for (uint32_t i = 0; i < sig->payload_bytes && i < sizeof(payload); ++i) {
        payload[i] = (uint8_t)(i + cfg->seed);
    }

    uint64_t run_ns = 2ULL * 1000000000ULL; /* 2 s sustained */
    uint64_t deadline = sim_now_ns() + run_ns;
    uint64_t sent = 0, received = 0;

    while (sim_now_ns() < deadline) {
        uint64_t t0 = sim_now_ns();
        if (sim_ecu_send_signal(tx, sig, payload, sig->payload_bytes)) {
            sent++;
        }
        uint8_t rxbuf[4096];
        uint16_t rx_id = 0;
        uint32_t out_len = 0;
        if (sim_ecu_recv_signal(rx, &rx_id, rxbuf, sizeof(rxbuf),
                                &out_len, 50000000ULL)) {
            received++;
            sim_hist_add(&m.e2e_latency, sim_now_ns() - t0);
        }
    }

    m.session_ns_end = sim_now_ns();

    SimBusStats bs;
    sim_bus_get_stats(bus, &bs);
    double elapsed_s = (double)(m.session_ns_end - m.session_ns_start) / 1e9;
    sim_log(SIM_LOG_INFO, "throughput  sent=%llu  received=%llu  elapsed=%.3fs",
            (unsigned long long)sent, (unsigned long long)received, elapsed_s);
    sim_log(SIM_LOG_INFO, "throughput  msgs/s=%.1f  MB/s=%.2f  bus_tx_frames=%llu",
            (double)received / elapsed_s,
            (double)bs.bytes_tx / (elapsed_s * 1024.0 * 1024.0),
            (unsigned long long)bs.frames_tx);

    sim_ecu_destroy(tx);
    sim_ecu_destroy(rx);
    sim_bus_destroy(bus);

    sim_scenario_finalise(&m, "throughput", cfg);
    return 0;
}
