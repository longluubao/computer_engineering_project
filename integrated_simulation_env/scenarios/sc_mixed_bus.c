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
 * sc_mixed_bus_run
 * ----------------
 * Exercises a CAN-FD (legacy) ↔ 100BASE-T1 (backbone) gateway. The ECU
 * on the CAN side transmits a small safety signal; the gateway ECU
 * verifies, re-authenticates and re-emits on Ethernet; the Ethernet ECU
 * verifies. We accumulate per-hop metrics so the thesis can discuss the
 * "double overhead" of gateway re-authentication.
 */

int sc_mixed_bus_run(const SimConfig *cfg)
{
    SimMetrics m_can;
    SimMetrics m_eth;
    sim_metrics_init(&m_can);
    sim_metrics_init(&m_eth);
    m_can.session_ns_start = sim_now_ns();
    m_eth.session_ns_start = sim_now_ns();

    SimBusCfg can_bc = {0}, eth_bc = {0};
    can_bc.kind = SIM_BUS_CAN_FD; can_bc.bitrate_bps = 5000000U;
    can_bc.mtu_bytes = 64U;       can_bc.propagation_ns = 3000U;
    eth_bc.kind = SIM_BUS_ETH_100;eth_bc.bitrate_bps = 100000000U;
    eth_bc.mtu_bytes = 1500U;     eth_bc.propagation_ns = 1000U;

    SimBus *bus_can = sim_bus_create(0, &can_bc);
    SimBus *bus_eth = sim_bus_create(1, &eth_bc);

    SimEcuCfg ec = {0};
    ec.role = SIM_ECU_ROLE_TX;
    ec.protection = cfg->protection;
    ec.seed = cfg->seed;

    ec.name = "tx-can";  ec.primary_bus = bus_can; ec.metrics = &m_can;
    SimEcu *tx_can = sim_ecu_create(&ec);

    ec.name = "gw-can";  ec.role = SIM_ECU_ROLE_GW;
    ec.primary_bus = bus_can; ec.gateway_bus = bus_eth; ec.metrics = &m_can;
    SimEcu *gw_can_side = sim_ecu_create(&ec);

    ec.name = "gw-eth";  ec.primary_bus = bus_eth; ec.gateway_bus = bus_can;
    ec.metrics = &m_eth;
    SimEcu *gw_eth_side = sim_ecu_create(&ec);

    ec.role = SIM_ECU_ROLE_RX; ec.name = "rx-eth";
    ec.primary_bus = bus_eth; ec.gateway_bus = NULL; ec.metrics = &m_eth;
    SimEcu *rx_eth = sim_ecu_create(&ec);

    sim_ecu_init_stack(tx_can);
    sim_ecu_init_stack(gw_can_side);
    sim_ecu_init_stack(gw_eth_side);
    sim_ecu_init_stack(rx_eth);

    if (cfg->protection == SIM_PROT_PQC || cfg->protection == SIM_PROT_HYBRID) {
        /* Same public key across the segment for simplicity. Production
         * deployments would rotate distinct keys per link. */
        (void)sim_ecu_pqc_handshake(tx_can, gw_can_side);
        (void)sim_ecu_pqc_handshake(gw_eth_side, rx_eth);
    }
    sim_ecu_share_keys(tx_can, gw_can_side);
    sim_ecu_share_keys(gw_eth_side, rx_eth);

    /* Both hops use the same signal so the payload size seen by the rx
     * side matches what the gateway actually forwarded. Using a different
     * signal def on the eth hop would make the rx use the ADAS signal's
     * declared payload_bytes (256) to locate the authenticator, which
     * does not match the 8-byte CAN payload the gateway relayed. */
    const SimSignalDef *sig_can = sim_signal_find(0x04); /* Throttle, 8 B */
    const SimSignalDef *sig_eth = sig_can;

    uint8_t payload[1024];
    for (uint32_t i = 0; i < sizeof(payload); ++i) payload[i] = (uint8_t)i;

    for (uint32_t it = 0; it < cfg->iterations; ++it) {
        /* CAN-side transmit */
        uint64_t t0 = sim_now_ns();
        sim_ecu_send_signal(tx_can, sig_can, payload, sig_can->payload_bytes);

        /* Gateway CAN side: receive, verify, forward */
        uint8_t buf[4096];
        uint16_t id = 0; uint32_t n = 0;
        if (sim_ecu_recv_signal(gw_can_side, &id, buf, sizeof(buf), &n,
                                100000000ULL)) {
            sim_hist_add(&m_can.e2e_latency, sim_now_ns() - t0);
            m_can.success_count++;
            sim_ecu_send_signal(gw_eth_side, sig_eth, buf, n);
        }

        /* Eth receiver */
        uint32_t n2 = 0;
        if (sim_ecu_recv_signal(rx_eth, &id, buf, sizeof(buf), &n2,
                                100000000ULL)) {
            sim_hist_add(&m_eth.e2e_latency, sim_now_ns() - t0);
            m_eth.success_count++;
        }
    }

    m_can.session_ns_end = sim_now_ns();
    m_eth.session_ns_end = sim_now_ns();

    sim_ecu_destroy(tx_can);
    sim_ecu_destroy(gw_can_side);
    sim_ecu_destroy(gw_eth_side);
    sim_ecu_destroy(rx_eth);
    sim_bus_destroy(bus_can);
    sim_bus_destroy(bus_eth);

    sim_scenario_finalise(&m_can, "mixed_bus_can", cfg);
    sim_scenario_finalise(&m_eth, "mixed_bus_eth", cfg);
    return 0;
}
