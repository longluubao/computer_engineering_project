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
 * sc_deadline_stress_run
 * ----------------------
 * Forces deadline violations and proves the metrics pipeline records
 * them per-ASIL-class. The scenario is engineered to place tight
 * safety-critical signals (ASIL-D, 5 ms budget) on the slowest bus
 * (classical CAN, 1 Mbps, 8 B MTU) so the ML-DSA signature (3309 B)
 * is guaranteed to exceed the cycle time, while also running a
 * comfortably-met ASIL-B signal on Ethernet for contrast.
 *
 * Expected thesis evidence:
 *   deadline_miss_count[D1] > 0   (ASIL-D over classical CAN)
 *   deadline_miss_count[D5] == 0  (ASIL-B over 100BASE-T1)
 *
 * This is the only ISE scenario that intentionally blows the budget.
 * All other tests track deadlines without actively trying to miss them.
 */

static uint32_t deadline_ns_for_class(uint8_t cls)
{
    switch (cls) {
        case 1: return   5000000U;
        case 2: return  10000000U;
        case 3: return  20000000U;
        case 4: return  50000000U;
        case 5: return 100000000U;
        case 6: return 500000000U;
        default: return 100000000U;
    }
}

static void run_pair(const SimConfig *cfg, SimBusCfg bc, const SimSignalDef *sig,
                     uint32_t iterations, SimMetrics *m)
{
    SimBus *bus = sim_bus_create((uint8_t)bc.kind, &bc);

    SimEcuCfg ec = {0};
    ec.role = SIM_ECU_ROLE_TX;
    ec.protection = cfg->protection;
    ec.seed = cfg->seed;
    ec.primary_bus = bus;
    ec.metrics = m;
    ec.name = "tx-dl";
    SimEcu *tx = sim_ecu_create(&ec);
    ec.role = SIM_ECU_ROLE_RX; ec.name = "rx-dl";
    ec.seed = cfg->seed ^ 0x1234567812345678ULL;
    SimEcu *rx = sim_ecu_create(&ec);
    sim_ecu_init_stack(tx);
    sim_ecu_init_stack(rx);
    if (cfg->protection == SIM_PROT_PQC ||
        cfg->protection == SIM_PROT_HYBRID) {
        (void)sim_ecu_pqc_handshake(tx, rx);
    }
    sim_ecu_share_keys(tx, rx);

    uint8_t payload[4096];
    for (uint32_t i = 0; i < sig->payload_bytes && i < sizeof(payload); ++i) {
        payload[i] = (uint8_t)(i ^ sig->id);
    }
    uint32_t deadline_ns = deadline_ns_for_class(sig->deadline_class);

    for (uint32_t it = 0; it < iterations; ++it) {
        uint64_t t0 = sim_now_ns();
        sim_ecu_send_signal(tx, sig, payload, sig->payload_bytes);
        uint8_t rxbuf[4096];
        uint16_t id = 0;
        uint32_t out_len = 0;
        (void)sim_ecu_recv_signal(rx, &id, rxbuf, sizeof(rxbuf),
                                  &out_len, deadline_ns * 10ULL);
        uint64_t e2e = sim_now_ns() - t0;

        sim_hist_add(&m->e2e_latency, e2e);
        m->deadline_total_count[sig->deadline_class]++;
        if (e2e > (uint64_t)deadline_ns) {
            m->deadline_miss_count[sig->deadline_class]++;
        }
    }

    sim_ecu_destroy(tx);
    sim_ecu_destroy(rx);
    sim_bus_destroy(bus);
}

int sc_deadline_stress_run(const SimConfig *cfg)
{
    SimMetrics m_tight;   /* ASIL-D on CAN 2.0, expected to miss */
    SimMetrics m_relaxed; /* ASIL-B on 100BASE-T1, expected to meet */
    sim_metrics_init(&m_tight);
    sim_metrics_init(&m_relaxed);
    m_tight.session_ns_start   = sim_now_ns();
    m_relaxed.session_ns_start = sim_now_ns();

    /* ASIL-D BrakeCmd (D1=5ms) on 1 Mbps classical CAN with 8 B MTU. */
    SimBusCfg can20 = {0};
    can20.kind = SIM_BUS_CAN_20;
    can20.bitrate_bps    = 1000000U;
    can20.mtu_bytes      = 8U;
    can20.propagation_ns = 5000U;
    const SimSignalDef *brake = sim_signal_find(0x01);
    if (brake) run_pair(cfg, can20, brake, cfg->iterations ? cfg->iterations : 100,
                        &m_tight);
    m_tight.session_ns_end = sim_now_ns();

    /* ASIL-B Speedometer (D4=50ms) on 100BASE-T1 Ethernet 100 Mbps. */
    SimBusCfg eth100 = {0};
    eth100.kind = SIM_BUS_ETH_100;
    eth100.bitrate_bps    = 100000000U;
    eth100.mtu_bytes      = 1500U;
    eth100.propagation_ns = 1000U;
    const SimSignalDef *speedo = sim_signal_find(0x09);
    if (speedo) run_pair(cfg, eth100, speedo,
                         cfg->iterations ? cfg->iterations : 100,
                         &m_relaxed);
    m_relaxed.session_ns_end = sim_now_ns();

    for (int i = 1; i <= 6; ++i) {
        if (m_tight.deadline_total_count[i] > 0) {
            sim_log(SIM_LOG_INFO,
                    "deadline_tight   D%d miss=%llu / %llu",
                    i,
                    (unsigned long long)m_tight.deadline_miss_count[i],
                    (unsigned long long)m_tight.deadline_total_count[i]);
        }
        if (m_relaxed.deadline_total_count[i] > 0) {
            sim_log(SIM_LOG_INFO,
                    "deadline_relaxed D%d miss=%llu / %llu",
                    i,
                    (unsigned long long)m_relaxed.deadline_miss_count[i],
                    (unsigned long long)m_relaxed.deadline_total_count[i]);
        }
    }

    sim_scenario_finalise(&m_tight,   "deadline_tight",   cfg);
    sim_scenario_finalise(&m_relaxed, "deadline_relaxed", cfg);
    return 0;
}
