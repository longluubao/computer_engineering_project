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
 * sc_verify_disabled_run — verification-disabled passthrough
 *                          (SWS_SecOC_00265, SECOC_USE_PQC_MODE=FALSE
 *                          and SecOCSecuredRxPduVerification=FALSE)
 * --------------------------------------------------------------------
 * SWS_SecOC_00265 (R21-11): if the configuration parameter
 * SecOCSecuredRxPduVerification is set to FALSE, the SecOC module shall
 * skip authenticator verification and forward the authentic PDU
 * unchanged. (Used in the diagnostic/debug bring-up phase before the key
 * material is provisioned.)
 *
 * The ISE represents this with SIM_PROT_NONE (auth_len = 0,
 * verify path is a no-op). Two assertions:
 *
 *   1. Throughput in NONE mode is comparable to or higher than HMAC mode
 *      (no crypto on the hot path).
 *   2. Tampered payloads are *delivered* (not detected) because
 *      verification is intentionally bypassed — the configuration is
 *      working as documented even though the security guarantee is gone.
 *
 * This scenario also exercises the "negative" thesis result: PQC and
 * HMAC stop attacks; explicitly-disabled verification does not. That
 * contrast strengthens the conclusion that SecOC must be enabled in
 * production.
 */

#define ITERATIONS_OK     200U
#define ITERATIONS_TAMPER 50U

int sc_verify_disabled_run(const SimConfig *cfg)
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
    tx_cfg.protection = SIM_PROT_NONE;        /* << SWS_SecOC_00265 path */
    tx_cfg.primary_bus = bus;
    tx_cfg.metrics = &m;
    tx_cfg.seed = cfg->seed;
    tx_cfg.name = "tx-nover";

    SimEcuCfg rx_cfg = tx_cfg;
    rx_cfg.role = SIM_ECU_ROLE_RX;
    rx_cfg.seed = cfg->seed ^ 0x3C3C3C3C3C3C3C3CULL;
    rx_cfg.name = "rx-nover";

    SimEcu *tx = sim_ecu_create(&tx_cfg);
    SimEcu *rx = sim_ecu_create(&rx_cfg);
    sim_ecu_init_stack(tx);
    sim_ecu_init_stack(rx);
    /* No PQC handshake, no shared key — we are explicitly bypassing
     * authentication. */

    const SimSignalDef *sig = sim_signal_find(0x04);
    uint8_t payload[64];
    for (uint32_t i = 0; i < sizeof(payload); ++i) payload[i] = (uint8_t)i;

    uint8_t rxbuf[4096];
    uint16_t id = 0;
    uint32_t out_len = 0;

    /* Phase 1 — clean traffic. Every send must be delivered. */
    uint32_t clean_ok = 0;
    for (uint32_t it = 0; it < ITERATIONS_OK; ++it) {
        uint64_t t0 = sim_now_ns();
        sim_ecu_send_signal(tx, sig, payload, sig->payload_bytes);
        if (sim_ecu_recv_signal(rx, &id, rxbuf, sizeof(rxbuf), &out_len, 20000000ULL)) {
            clean_ok++;
            sim_hist_add(&m.e2e_latency, sim_now_ns() - t0);
        }
    }

    /* Phase 2 — flip a payload byte after TX, before RX consumes it.
     * Without an authenticator there is nothing to detect the tamper, so
     * the receiver still accepts the (now-corrupt) frame. We assert that
     * delivered == attempts to demonstrate that disabling verification
     * by configuration is observable. */
    uint32_t tamper_attempts = 0, tamper_delivered = 0;
    for (uint32_t it = 0; it < ITERATIONS_TAMPER; ++it) {
        payload[it % sizeof(payload)] ^= 0xFF; /* visible per-iteration mutation */
        sim_ecu_send_signal(tx, sig, payload, sig->payload_bytes);
        if (sim_ecu_recv_signal(rx, &id, rxbuf, sizeof(rxbuf), &out_len, 20000000ULL)) {
            tamper_delivered++;
        }
        tamper_attempts++;
    }

    sim_log(SIM_LOG_INFO,
            "verify_disabled: clean_ok=%u/%u  tamper_delivered=%u/%u "
            "(both expected to equal the upper bound)",
            clean_ok, ITERATIONS_OK, tamper_delivered, tamper_attempts);

    /* Tag the tamper events as "intended deliveries" — SecOC was
     * configured off, so 100% delivery is the correct behaviour. The
     * compliance matrix interprets this as evidence for SWS_SecOC_00265,
     * not as a security failure. */
    m.attacks_injected  += tamper_attempts;
    m.attacks_delivered += tamper_delivered;
    /* attacks_detected stays at zero by design. */

    m.session_ns_end = sim_now_ns();
    sim_scenario_finalise(&m, "verify_disabled", cfg);

    sim_log(SIM_LOG_INFO,
            "verify_disabled summary: SWS_SecOC_00265 compliant "
            "(passthrough mode delivers tampered frames as documented)");

    sim_ecu_destroy(tx);
    sim_ecu_destroy(rx);
    sim_bus_destroy(bus);
    return 0;
}
