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
    /* period_ms=0 → fire on every eligible frame. The replay attack
     * additionally self-gates on its internal cache depth (first 3
     * frames are captured, not replayed) so injections still lag the
     * iteration count slightly. */
    ac.period_ms      = 0;
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
        uint64_t t0 = sim_now_ns();
        sim_ecu_send_signal(tx, sig, payload, sig->payload_bytes);
        uint8_t rxbuf[4096];
        uint16_t id = 0;
        uint32_t n = 0;
        bool ok = sim_ecu_recv_signal(rx, &id, rxbuf, sizeof(rxbuf), &n,
                                      20000000ULL);
        /* Record e2e latency regardless of verify outcome so the thesis
         * can compare baseline vs under-attack timing. */
        sim_hist_add(&m.e2e_latency, sim_now_ns() - t0);
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

    /* Roll per-attack samples into the aggregate so the top-level
     * "attacks_aggregate" row has real latency + PDU numbers. The
     * reservoir holds at most SIM_METRICS_RESERVOIR samples; we stop
     * adding once it's full (percentiles fall back to log2 buckets). */
    #define MERGE_HIST(dst, src) \
        do { \
            for (uint32_t _i = 0; _i < (src).sample_count; ++_i) { \
                sim_hist_add(&(dst), (src).samples[_i]); \
            } \
        } while (0)
    MERGE_HIST(agg->e2e_latency,  m.e2e_latency);
    MERGE_HIST(agg->secoc_auth,   m.secoc_auth);
    MERGE_HIST(agg->secoc_verify, m.secoc_verify);
    MERGE_HIST(agg->cantp,        m.cantp);
    MERGE_HIST(agg->pdu_bytes,    m.pdu_bytes);
    MERGE_HIST(agg->fragments,    m.fragments);
    #undef MERGE_HIST

    sim_log(SIM_LOG_INFO,
            "attack[%s]  injected=%llu detected=%llu delivered=%llu  rate=%.2f%%",
            sim_attack_name(kind),
            (unsigned long long)ast.injected,
            (unsigned long long)ast.detected,
            (unsigned long long)ast.delivered,
            ast.injected ? 100.0 * (double)ast.detected / (double)ast.injected
                         : 0.0);

    /* Write a per-attack summary alongside the aggregate. Tag with the
     * protection mode so that attacks_pqc and attacks_hmac runs produce
     * distinct output files instead of overwriting each other. */
    const char *prot = (cfg->protection == SIM_PROT_PQC)    ? "pqc"
                     : (cfg->protection == SIM_PROT_HYBRID) ? "hybrid"
                                                            : "hmac";
    char label[64];
    snprintf(label, sizeof(label), "attacks_%s_%s",
             sim_attack_name(kind), prot);
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

    /* Active attacks the simulation can evaluate via a detection rate.
     *
     * Excluded by design (each needs a different metric, not detection):
     *   - DOS_FLOOD:    measured via throughput/drop impact, not detection
     *   - HARVEST_NOW:  passive eavesdropping, mitigated by PQC algorithm
     *                   choice (no runtime detection event)
     *   - TIMING_PROBE: side-channel analysis, covered by liboqs constant
     *                   time implementations, not SecOC verify
     * These can still be invoked individually via --attack <kind>. */
    SimAttackKind attacks[] = {
        SIM_ATK_REPLAY,
        SIM_ATK_TAMPER_PAYLOAD,
        SIM_ATK_TAMPER_AUTH,
        SIM_ATK_FRESHNESS_ROLLBACK,
        SIM_ATK_MITM_KEY_CONFUSE,
        SIM_ATK_SIG_FUZZ,
        SIM_ATK_DOWNGRADE_HMAC
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
    const char *agg_prot = (cfg->protection == SIM_PROT_PQC)    ? "pqc"
                         : (cfg->protection == SIM_PROT_HYBRID) ? "hybrid"
                                                                : "hmac";
    char agg_label[64];
    snprintf(agg_label, sizeof(agg_label), "attacks_aggregate_%s", agg_prot);
    sim_scenario_finalise(&agg, agg_label, cfg);
    return 0;
}
