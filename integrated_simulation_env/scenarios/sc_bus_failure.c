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
 * sc_bus_failure_run
 * ------------------
 * Injects physical-layer bit errors on the CAN-FD bus and verifies
 * that SecOC detects the resulting tampered frames (AUTOSAR CanSM
 * / EthSM bus-error recovery surface).
 *
 * Per-phase metrics captured:
 *   session_ns        — elapsed wall time
 *   success_count     — frames verified cleanly
 *   verify_fail_count — frames rejected because MAC / signature failed
 *   bus.frames_dropped_ber — number of bit flips injected on the wire
 *
 * We ramp the BER across three phases so the thesis can plot detection
 * rate vs BER (SWS_CanIf / CanSM noisy-link requirement).
 */

typedef struct {
    const char *label;
    double      ber;   /* bit error rate */
} BerPhase;

static int run_one(const SimConfig *cfg, const BerPhase *ph, SimMetrics *agg)
{
    SimMetrics m;
    sim_metrics_init(&m);
    m.session_ns_start = sim_now_ns();

    SimBusCfg bc;
    memset(&bc, 0, sizeof(bc));
    bc.kind = SIM_BUS_CAN_FD;
    bc.bitrate_bps    = 5000000U;
    bc.mtu_bytes      = 64U;
    bc.propagation_ns = 3000U;
    bc.bit_error_rate = ph->ber;
    SimBus *bus = sim_bus_create(0, &bc);

    SimEcuCfg ec = {0};
    ec.role = SIM_ECU_ROLE_TX;
    ec.protection = cfg->protection;
    ec.seed = cfg->seed;
    ec.primary_bus = bus;
    ec.metrics = &m;
    ec.name = "tx-ber";
    SimEcu *tx = sim_ecu_create(&ec);
    ec.role = SIM_ECU_ROLE_RX; ec.name = "rx-ber";
    ec.seed = cfg->seed ^ 0xDEADBEEFDEADBEEFULL;
    SimEcu *rx = sim_ecu_create(&ec);
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

    uint32_t n = cfg->iterations < 100 ? 100 : cfg->iterations;
    for (uint32_t it = 0; it < n; ++it) {
        uint64_t t0 = sim_now_ns();
        sim_ecu_send_signal(tx, sig, payload, sig->payload_bytes);
        uint8_t rxbuf[4096];
        uint16_t id = 0;
        uint32_t out_len = 0;
        bool ok = sim_ecu_recv_signal(rx, &id, rxbuf, sizeof(rxbuf),
                                      &out_len, 20000000ULL);
        if (ok) sim_hist_add(&m.e2e_latency, sim_now_ns() - t0);
    }
    m.session_ns_end = sim_now_ns();

    SimBusStats bs;
    sim_bus_get_stats(bus, &bs);

    sim_log(SIM_LOG_INFO,
            "bus_failure[%s] BER=%.2e  tx=%llu rx=%llu "
            "ber_flipped=%llu queue_drops=%llu  "
            "success=%llu verify_fail=%llu",
            ph->label, ph->ber,
            (unsigned long long)bs.frames_tx,
            (unsigned long long)bs.frames_rx,
            (unsigned long long)bs.frames_dropped_ber,
            (unsigned long long)bs.frames_dropped_queue,
            (unsigned long long)m.success_count,
            (unsigned long long)m.verify_fail_count);

    agg->success_count     += m.success_count;
    agg->verify_fail_count += m.verify_fail_count;

    char scn[96];
    snprintf(scn, sizeof(scn), "bus_failure_%s", ph->label);
    sim_scenario_finalise(&m, scn, cfg);

    sim_ecu_destroy(tx);
    sim_ecu_destroy(rx);
    sim_bus_destroy(bus);
    return 0;
}

int sc_bus_failure_run(const SimConfig *cfg)
{
    SimMetrics agg;
    sim_metrics_init(&agg);
    agg.session_ns_start = sim_now_ns();

    /* Three representative bit-error rates: clean link, noisy link,
     * heavily corrupted link. The "heavy" case is close to what a
     * cable fault or EMI burst would produce. */
    BerPhase phases[] = {
        { "clean",   0.0    },
        { "noisy",   1e-6   },
        { "heavy",   1e-4   }
    };
    for (size_t i = 0; i < sizeof(phases)/sizeof(phases[0]); ++i) {
        run_one(cfg, &phases[i], &agg);
    }

    agg.session_ns_end = sim_now_ns();
    sim_scenario_finalise(&agg, "bus_failure_aggregate", cfg);
    return 0;
}
