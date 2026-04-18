#include "sim_runner.h"
#include "sim_clock.h"
#include "sim_logger.h"
#include "sim_bus.h"
#include "sim_ecu.h"
#include "sim_signals.h"
#include "sim_metrics.h"
#include "sim_attacker.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*
 * sc_attacks_run
 * --------------
 * Runs every attack kind (1..10) in sequence against the same
 * authenticated channel. For each attack, the scenario records:
 *   - attacks_injected
 *   - attacks_detected (SecOC rejected the PDU)
 *   - attacks_delivered (PDU erroneously passed verification — should be 0)
 *
 * The resulting CSV/JSON lets the thesis tabulate detection rates per
 * attack class (the UN R155 Annex-5 mapping).
 */

static int run_one_attack(const SimConfig *cfg, SimAttackKind kind,
                          SimMetrics *agg)
{
    SimMetrics m;
    sim_metrics_init(&m);
    m.session_ns_start = sim_now_ns();

    SimBusCfg bc = {0};
    bc.kind = SIM_BUS_CAN_FD;
    bc.bitrate_bps    = 5000000U;
    bc.mtu_bytes      = 64U;
    bc.propagation_ns = 3000U;
    SimBus *bus = sim_bus_create(0, &bc);

    SimAttackCfg ac = {0};
    ac.kind           = kind;
    ac.period_ms      = 5;
    ac.target_signal  = 0;
    ac.target_bus_id  = 0xFF;
    ac.seed           = cfg->seed;
    SimAttacker *atk = sim_attacker_create(&ac);
    sim_attacker_attach(atk, bus);

    SimEcuCfg ec = {0};
    ec.role = SIM_ECU_ROLE_TX;
    ec.protection = cfg->protection;
    ec.seed = cfg->seed;
    ec.primary_bus = bus;
    ec.metrics = &m;
    ec.name = "tx-atk";
    SimEcu *tx = sim_ecu_create(&ec);
    ec.role = SIM_ECU_ROLE_RX; ec.name = "rx-atk";
    ec.seed = cfg->seed ^ 0x55AA55AA55AA55AAULL;
    SimEcu *rx = sim_ecu_create(&ec);
    sim_ecu_init_stack(tx);
    sim_ecu_init_stack(rx);
    if (cfg->protection == SIM_PROT_PQC || cfg->protection == SIM_PROT_HYBRID) {
        (void)sim_ecu_pqc_handshake(tx, rx);
    }
    sim_ecu_share_keys(tx, rx);

    const SimSignalDef *sig = sim_signal_find(0x04); /* throttle */
    uint8_t payload[64];
    for (uint32_t i = 0; i < sizeof(payload); ++i) payload[i] = (uint8_t)(i + kind);

    uint32_t iters = cfg->iterations < 40 ? 40 : cfg->iterations;
    for (uint32_t it = 0; it < iters; ++it) {
        sim_ecu_send_signal(tx, sig, payload, sig->payload_bytes);
        uint8_t rxbuf[4096];
        uint16_t id = 0;
        uint32_t n = 0;
        bool ok = sim_ecu_recv_signal(rx, &id, rxbuf, sizeof(rxbuf), &n,
                                      20000000ULL);
        sim_attacker_notify_verify_outcome(atk, ok, sig->id);
    }

    m.session_ns_end = sim_now_ns();
    SimAttackerStats ast = {0};
    sim_attacker_get_stats(atk, &ast);

    m.attacks_injected  = ast.injected;
    m.attacks_detected  = ast.detected;
    m.attacks_delivered = ast.delivered;

    agg->attacks_injected  += ast.injected;
    agg->attacks_detected  += ast.detected;
    agg->attacks_delivered += ast.delivered;
    agg->verify_fail_count += m.verify_fail_count;
    agg->success_count     += m.success_count;

    sim_log(SIM_LOG_INFO,
            "attack[%s]  injected=%llu detected=%llu delivered=%llu  rate=%.2f%%",
            sim_attack_name(kind),
            (unsigned long long)ast.injected,
            (unsigned long long)ast.detected,
            (unsigned long long)ast.delivered,
            ast.injected ? 100.0 * (double)ast.detected / (double)ast.injected
                         : 0.0);

    /* Write a per-attack summary alongside the aggregate. */
    char label[64];
    snprintf(label, sizeof(label), "attacks_%s", sim_attack_name(kind));
    sim_scenario_finalise(&m, label, cfg);

    sim_ecu_destroy(tx);
    sim_ecu_destroy(rx);
    sim_attacker_destroy(atk);
    sim_bus_destroy(bus);
    return 0;
}

int sc_attacks_run(const SimConfig *cfg)
{
    SimMetrics agg;
    sim_metrics_init(&agg);
    agg.session_ns_start = sim_now_ns();

    SimAttackKind attacks[] = {
        SIM_ATK_REPLAY,
        SIM_ATK_TAMPER_PAYLOAD,
        SIM_ATK_TAMPER_AUTH,
        SIM_ATK_FRESHNESS_ROLLBACK,
        SIM_ATK_MITM_KEY_CONFUSE,
        SIM_ATK_DOS_FLOOD,
        SIM_ATK_SIG_FUZZ,
        SIM_ATK_DOWNGRADE_HMAC,
        SIM_ATK_HARVEST_NOW,
        SIM_ATK_TIMING_PROBE
    };

    if (cfg->attack_kind != SIM_ATK_NONE) {
        /* Single attack mode. */
        run_one_attack(cfg, cfg->attack_kind, &agg);
    } else {
        for (size_t i = 0; i < sizeof(attacks)/sizeof(attacks[0]); ++i) {
            run_one_attack(cfg, attacks[i], &agg);
        }
    }

    agg.session_ns_end = sim_now_ns();
    sim_scenario_finalise(&agg, "attacks_aggregate", cfg);
    return 0;
}
