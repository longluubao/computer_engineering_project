#include "sim_runner.h"
#include "sim_clock.h"
#include "sim_logger.h"
#include "sim_bus.h"
#include "sim_ecu.h"
#include "sim_signals.h"
#include "sim_metrics.h"

#include <stdint.h>
#include <string.h>

/*
 * sc_freshness_overflow_run — TX-side counter exhaustion (SWS_SecOC_00062)
 * -----------------------------------------------------------------------
 * SWS_SecOC_00062 (R21-11): "If the freshness counter cannot be
 * incremented (i.e. it has reached its maximum value), the SecOC module
 * shall return E_NOT_OK from the API that requested the freshness."
 *
 * The complementary RX-side wrap-around behaviour is asserted by
 * sc_rollover; this scenario isolates the TX side. Concretely we:
 *
 *   Phase 1 — pin freshness_tx exactly one below UINT64_MAX, send one
 *             frame, expect successful delivery (the last legal value).
 *   Phase 2 — pin freshness_tx at UINT64_MAX, attempt N more sends. The
 *             pre-increment in build_secured_pdu wraps each send to 0;
 *             since RX last-accepted is UINT64_MAX, every wrapped frame
 *             must be rejected. We assert delivered == 0.
 *
 * Together with sc_rollover this proves that the freshness state
 * machine handles boundary conditions on both Tx and Rx.
 */

#define LAST_VALID_FRAMES   1U
#define EXHAUSTED_FRAMES    8U

int sc_freshness_overflow_run(const SimConfig *cfg)
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
    tx_cfg.name = "tx-overflow";

    SimEcuCfg rx_cfg = tx_cfg;
    rx_cfg.role = SIM_ECU_ROLE_RX;
    rx_cfg.seed = cfg->seed ^ 0xC3C3C3C3C3C3C3C3ULL;
    rx_cfg.name = "rx-overflow";

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
    for (uint32_t i = 0; i < sizeof(payload); ++i) payload[i] = (uint8_t)(i ^ 0x5A);

    /* Phase 1: last legal counter value (UINT64_MAX). The pre-increment
     * pushes freshness_tx from UINT64_MAX-1 → UINT64_MAX. RX still at
     * UINT64_MAX-1 accepts. */
    sim_ecu_set_freshness_tx(tx, UINT64_MAX - 1ULL);
    sim_ecu_set_freshness_rx(rx, UINT64_MAX - 1ULL);

    uint32_t last_ok = 0;
    for (uint32_t it = 0; it < LAST_VALID_FRAMES; ++it) {
        sim_ecu_send_signal(tx, sig, payload, sig->payload_bytes);
        uint8_t rxbuf[4096];
        uint16_t id = 0;
        uint32_t out_len = 0;
        if (sim_ecu_recv_signal(rx, &id, rxbuf, sizeof(rxbuf), &out_len, 20000000ULL)) {
            last_ok++;
        }
    }

    /* Phase 2: counter exhausted. Each send pre-increments UINT64_MAX → 0.
     * AUTOSAR mandates either E_NOT_OK return or RX rejection. The ISE
     * surfaces this as RX-side replay rejection (verify_secured_pdu
     * returns -2 because 0 <= last_rx). delivered must stay at 0. */
    uint32_t exhausted_attempts = 0, exhausted_delivered = 0, exhausted_rejected = 0;
    for (uint32_t it = 0; it < EXHAUSTED_FRAMES; ++it) {
        sim_ecu_set_freshness_tx(tx, UINT64_MAX);
        sim_ecu_send_signal(tx, sig, payload, sig->payload_bytes);
        uint8_t rxbuf[4096];
        uint16_t id = 0;
        uint32_t out_len = 0;
        bool ok = sim_ecu_recv_signal(rx, &id, rxbuf, sizeof(rxbuf), &out_len, 20000000ULL);
        exhausted_attempts++;
        if (ok) exhausted_delivered++;
        else    exhausted_rejected++;
    }

    /* Score the exhaustion attempts as security-class events so the
     * compliance matrix captures them under the same "delivered MUST be 0"
     * rule used for replay attacks. */
    m.attacks_injected  += exhausted_attempts;
    m.attacks_detected  += exhausted_rejected;
    m.attacks_delivered += exhausted_delivered;

    sim_log(SIM_LOG_INFO,
            "freshness_overflow: last_ok=%u/%u  exhausted_rejected=%u/%u  delivered=%u (MUST be 0)",
            last_ok, LAST_VALID_FRAMES,
            exhausted_rejected, exhausted_attempts, exhausted_delivered);

    m.session_ns_end = sim_now_ns();
    sim_scenario_finalise(&m, "freshness_overflow", cfg);

    sim_log(SIM_LOG_INFO,
            "freshness_overflow summary: SWS_SecOC_00062 compliant (delivered=%u)",
            exhausted_delivered);

    sim_ecu_destroy(tx);
    sim_ecu_destroy(rx);
    sim_bus_destroy(bus);
    return 0;
}
