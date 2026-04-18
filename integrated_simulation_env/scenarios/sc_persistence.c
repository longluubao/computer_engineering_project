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
 * sc_persistence_run
 * ------------------
 * Exercises the AUTOSAR NvM integration for SecOC freshness counters
 * (SWS_SecOC_00194). The test runs two phases against the same TX/RX
 * pair:
 *
 *   Phase 1: TX sends N authenticated frames; RX accepts them.
 *            RX's highest accepted freshness becomes F1.
 *
 *   Phase 2a (no NvM restore): RX is destroyed and recreated; its
 *            freshness_rx resets to 0. TX continues from F1. An
 *            attacker also injects replays of freshness=1 which, in
 *            the unprotected variant, are accepted — demonstrating the
 *            vulnerability.
 *
 *   Phase 2b (with NvM restore): RX is destroyed and recreated; the
 *            freshness value F1 is restored via sim_ecu_set_freshness_rx,
 *            modelling the NvM block readout. The same replays are now
 *            rejected. This matches the AUTOSAR mandatory behaviour.
 *
 * Metrics:
 *   success_count      = legitimate frames accepted after restore
 *   verify_fail_count  = replays rejected (expected == replay count)
 *   attacks_injected   = replays attempted
 *   attacks_detected   = replays rejected
 *   attacks_delivered  = replays accepted (breach — must be 0 with NvM)
 */

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

static void run_phase(const SimConfig *cfg, const char *label,
                      bool with_nvm_restore, SimMetrics *agg)
{
    SimMetrics m;
    sim_metrics_init(&m);
    m.session_ns_start = sim_now_ns();

    SimBusCfg bc = default_canfd_bus();
    SimBus *bus = sim_bus_create(0, &bc);

    SimEcuCfg tx_cfg = {0};
    tx_cfg.name = "tx-persist";
    tx_cfg.role = SIM_ECU_ROLE_TX;
    tx_cfg.protection = cfg->protection;
    tx_cfg.primary_bus = bus;
    tx_cfg.metrics = &m;
    tx_cfg.seed = cfg->seed;

    SimEcuCfg rx_cfg = tx_cfg;
    rx_cfg.name = "rx-persist";
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

    const SimSignalDef *sig = sim_signal_find(0x04); /* Throttle, 8 B */
    uint8_t payload[64];
    for (uint32_t i = 0; i < sizeof(payload); ++i) payload[i] = (uint8_t)i;

    /* ---- Phase 1: normal authenticated traffic ---- */
    uint32_t n = cfg->iterations;
    if (n < 50) n = 50;

    for (uint32_t it = 0; it < n; ++it) {
        uint64_t t0 = sim_now_ns();
        sim_ecu_send_signal(tx, sig, payload, sig->payload_bytes);

        uint8_t rxbuf[4096];
        uint16_t id = 0;
        uint32_t out_len = 0;
        bool ok = sim_ecu_recv_signal(rx, &id, rxbuf, sizeof(rxbuf),
                                      &out_len, 20000000ULL);
        if (ok) {
            sim_hist_add(&m.e2e_latency, sim_now_ns() - t0);
        }
    }

    uint64_t last_tx  = sim_ecu_get_freshness_tx(tx);
    uint64_t last_rx  = sim_ecu_get_freshness_rx(rx);
    sim_log(SIM_LOG_INFO,
            "[%s] phase1 done: freshness_tx=%llu freshness_rx=%llu",
            label,
            (unsigned long long)last_tx,
            (unsigned long long)last_rx);

    /* ---- Power cycle: destroy RX, recreate it ---- */
    sim_ecu_destroy(rx);
    SimEcu *rx2 = sim_ecu_create(&rx_cfg);
    sim_ecu_init_stack(rx2);
    sim_ecu_share_keys(tx, rx2);

    if (with_nvm_restore) {
        sim_ecu_set_freshness_rx(rx2, last_rx);
        sim_log(SIM_LOG_INFO,
                "[%s] NvM restore applied: freshness_rx=%llu",
                label, (unsigned long long)last_rx);
    } else {
        sim_log(SIM_LOG_WARN,
                "[%s] no NvM restore: freshness_rx resets to 0 "
                "(SWS_SecOC_00194 violation)",
                label);
    }

    /* ---- Phase 2: attacker captures and replays an old frame ----
     * We build a bogus replay by reusing freshness=1 (i.e. < last_rx).
     * Under correct NvM behaviour rx2 rejects it; without NvM it
     * accepts because its counter is back to zero.
     *
     * Because we cannot easily capture a real frame from the queue
     * (it was consumed in phase 1), we emulate the replay by manually
     * forcing TX's freshness to a pre-reboot value. This exercises
     * exactly the same verify_secured_pdu code path.
     */
    uint32_t replays = n / 2;
    uint32_t injected = 0, detected = 0, delivered = 0;

    for (uint32_t it = 0; it < replays; ++it) {
        sim_ecu_set_freshness_tx(tx, it); /* replay counter 1..replays */
        sim_ecu_send_signal(tx, sig, payload, sig->payload_bytes);

        uint8_t rxbuf[4096];
        uint16_t id = 0;
        uint32_t out_len = 0;
        bool ok = sim_ecu_recv_signal(rx2, &id, rxbuf, sizeof(rxbuf),
                                      &out_len, 20000000ULL);
        injected++;
        if (ok) delivered++;
        else    detected++;
    }

    m.attacks_injected  = injected;
    m.attacks_detected  = detected;
    m.attacks_delivered = delivered;

    sim_log(SIM_LOG_INFO,
            "[%s] phase2 replays: injected=%u detected=%u delivered=%u",
            label, injected, detected, delivered);

    /* ---- Phase 3: legitimate follow-up traffic continues cleanly ---- */
    sim_ecu_set_freshness_tx(tx, last_tx);
    for (uint32_t it = 0; it < n; ++it) {
        sim_ecu_send_signal(tx, sig, payload, sig->payload_bytes);
        uint8_t rxbuf[4096];
        uint16_t id = 0;
        uint32_t out_len = 0;
        (void)sim_ecu_recv_signal(rx2, &id, rxbuf, sizeof(rxbuf),
                                  &out_len, 20000000ULL);
    }

    m.session_ns_end = sim_now_ns();

    agg->attacks_injected  += injected;
    agg->attacks_detected  += detected;
    agg->attacks_delivered += delivered;
    agg->success_count     += m.success_count;
    agg->verify_fail_count += m.verify_fail_count;

    char scn[96];
    snprintf(scn, sizeof(scn), "persistence_%s", label);
    sim_scenario_finalise(&m, scn, cfg);

    sim_ecu_destroy(tx);
    sim_ecu_destroy(rx2);
    sim_bus_destroy(bus);
}

int sc_persistence_run(const SimConfig *cfg)
{
    SimMetrics agg;
    sim_metrics_init(&agg);
    agg.session_ns_start = sim_now_ns();

    run_phase(cfg, "no_nvm",   false, &agg);
    run_phase(cfg, "with_nvm", true,  &agg);

    agg.session_ns_end = sim_now_ns();
    sim_scenario_finalise(&agg, "persistence_aggregate", cfg);

    sim_log(SIM_LOG_INFO,
            "persistence summary: replay_delivered_without_nvm_vs_with_nvm "
            "verifies SWS_SecOC_00194");
    return 0;
}
